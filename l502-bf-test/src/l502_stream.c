/** 
    @addtogroup streams
    @{

    @file l502_stream.c
    Файл содержит функции обработки потоков от АЦП/DIN в ПК и от ПК в ЦАП/DOUT.
    Также в этот файл внесена реализация функций приема по SPORT0, так как они
    связаны с логикой управления потоков.
 */   


#include <stdint.h>
#include <stdlib.h>

#include <cdefBF523.h>
#include <sys/exception.h>
#include <ccblkfn.h>

#include "l502_cdefs.h"
#include "l502_cmd.h"
#include "l502_fpga.h"
#include "l502_defs.h"
#include "l502_global.h"
#include "l502_hdma.h"
#include "l502_sport_tx.h"
#include "l502_user_process.h"
#include "l502_stream.h"
#include "l502_sport_rx.h"


/** Размер буфера на прием данных по SPORT0 в 32-битных словах */
#define L502_SPORT_IN_BUF_SIZE           (2048*1024)
/** Размер буфера для приема данных по HostDMA на вывод в 32-битных словах */
#define L502_HDMA_OUT_BUF_SIZE           (1024*1024)

/** Шаг прерываний для приема данных по SPORT0 по-умолчанию */
#define L502_DEFAULT_SPORT_RX_BLOCK_SIZE    (32*1024)




t_in_stream_state  g_stream_in_state = IN_STREAM_STOP;
t_out_stream_state g_stream_out_state = OUT_STREAM_STOP;
/** Режим работы - определяет, запущен ли синхронный сбор или тест */
volatile int g_mode = L502_BF_MODE_IDLE;

int g_streams = L502_STREAM_ADC;
static int f_bf_reg = 0;

/* слово, обозначающее, что произошло переполнение */
static const uint32_t f_overflow_wrd = L502_STREAM_IN_MSG_OVERFLOW;



/* буфер для приема digin и данных АЦП от SPORT (в неинициализируемой области) */
#include "l502_sdram_noinit.h"
static volatile uint32_t f_sport_in_buf[L502_SPORT_IN_BUF_SIZE];
/* позиция в буфере в которую будет записан следующий принятых отсчет */
static volatile uint32_t f_sport_in_put_pos = 0;
/* позиция в буфере за последним обработанным отсчетом */
static uint32_t f_sport_in_proc_pos = 0;
/* позиция в буфере за последним изятым из буфера отсчетом (позиция первого занятого отсчета) */
static volatile uint32_t f_sport_in_get_pos = 0;
/* шаг прерываний на прием по SPORT0 */
static uint32_t f_sport_in_block_size = L502_DEFAULT_SPORT_RX_BLOCK_SIZE;
/* реально используемый размер входного буфера на прием */
static uint32_t f_sport_in_buf_size = L502_SPORT_IN_BUF_SIZE;


/* буфер для прв кинятых данных по HDMA для вывода на ЦАП и DIGOUT */
#include "l502_sdram_noinit.h"
static volatile uint32_t f_hdma_out_buf[L502_HDMA_OUT_BUF_SIZE];
static volatile uint32_t f_hdma_out_put_pos = 0; /* указатель на позицию за последним принятым по HDMA */
static uint32_t f_hdma_out_start_pos; /* указатель на позицию за последним поставленным на прием словом
                                    (с нее будет стартовать следующий запрос) */
static volatile uint32_t f_hdma_out_get_pos = 0; /* указатель на позицию за последним обработанным словом
                                     (с нее будет взято следующее слово для обработки, когда будет готово */
static int f_hdma_out_block_size = 0x8000;
static int f_hdma_out_proc_pos;

static uint32_t f_recv_size = 0;







/* функция вызывается при возникновении переполнения буфера на прием
   по SPORT'у данных АЦП/DIGIN */
static void f_stream_in_set_overflow(void) {
    /* останавливаем прием данных от АЦП */
    sport_rx_stop();
    /* устанавливаем флаг, что нужно передать сообщение о
       переполнении */
    g_stream_in_state = IN_STREAM_OV_ALERT;
}





/** @brief Начальная инициализация параметров для синхронных потоков */
void l502_stream_init(void) {
    /* останавливаем сбор данных, если он был запущен */
    fpga_reg_write(L502_REGS_IOHARD_GO_SYNC_IO, 0);
    fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, 0);
    /* настройка DMA на передачу по SPORT */
    sport_tx_init();
}


/** @brief Запуск предзагрузки данных на вывода

   Данная функция запускает предзагрузку данных потока на вывод.
   Используется, чтобы загрузить данные в буфер BlackFin до запуска синхронного
   ввода-вывода.
   @return Код ошибки */
int32_t stream_out_preload(void) {
    int32_t err = g_stream_out_state==OUT_STREAM_RUN ? L502_BF_ERR_STREAM_RUNNING :
                  0;
    if (!err) {
        f_hdma_out_get_pos = f_hdma_out_start_pos =
            f_hdma_out_put_pos = f_hdma_out_proc_pos = 0;

        /* разрешаем прием по HDMA */
        hdma_recv_start();
        /* запускаем первый блок на прием (остальные будут
           добавлены из stream_proc() */
        f_hdma_out_start_pos+=f_hdma_out_block_size;
        hdma_recv_req_start((uint32_t*)f_hdma_out_buf, f_hdma_out_block_size);

        g_stream_out_state = OUT_STREAM_PRELOAD;
    }
    return err;
}


static void f_set_streams(uint32_t streams) {
    uint32_t wrd_en = 0;

    /* если уже запущен потоковый режим и разрешается один из
       потоков на ввод, то инициализируем прием по SPORT и передачу по hdma */
    if (g_mode == L502_BF_MODE_STREAM) {
        if ((streams & L502_STREAM_ALL_IN) && !(g_streams & L502_STREAM_ALL_IN)) {
            sport_rx_start();
        }

        if (!(streams & L502_STREAM_ALL_IN) && (g_streams & L502_STREAM_ALL_IN)) {
            sport_rx_stop();
        }
    }

    /* изменяем разрешенные потоки в регистре FPGA */
    if (streams & L502_STREAM_ADC)
        wrd_en |= 0x1;
    if (streams & L502_STREAM_DIN)
        wrd_en |= 0x2;
    fpga_reg_write(L502_REGS_IOARITH_IN_STREAM_ENABLE, wrd_en);


    /** @todo: разрешение на лету выходных потоков */

    g_streams = streams;
}

/** @brief Разрешение указанных синхронных потоков
    @param[in] streams   Битовая маска из #t_l502_streams, указывающая какие потоки
                         должны быть разрешены (в дополнения к уже разрешенным)
    @return Код ошибки */
int32_t stream_enable(uint32_t streams) {
    f_set_streams(g_streams | streams);
    return 0;
}


/** @brief Запрещение указанных синхронных потоков
    @param[in] streams   Битовая маска из #t_l502_streams, указывающая какие потоки
                         должны быть запрещены
    @return Код ошибки */
int32_t stream_disable(uint32_t streams) {
    f_set_streams(g_streams & ~streams);
    return 0;
}

/***************************************************************************//**
    @brief Запуск синхронного ввода-вывода

    Функция запускает синхронный ввод-вывод платы.
    При этом начинается передача по всем ранее разрешенным потокам с помощью
    stream_enable().
    После вызова этой функции изменять настройки модуля уже нельязя, однако
    можно дополнительно разрешать или запрещать потоки через stream_enable()
    или stream_disable().

    @return Код ошибки.
    ***************************************************************************/
int32_t streams_start(void) {
    int32_t err = g_mode != L502_BF_MODE_IDLE ? L502_BF_ERR_STREAM_RUNNING : 0;
    if (!err) {
        /* прием по SPORT инициализируется вместе с началом запуска
           синхронного сбора */
        f_sport_in_put_pos = f_sport_in_get_pos = f_sport_in_proc_pos = 0;


        if (g_streams & L502_STREAM_ALL_IN) {
            sport_rx_start();
            g_stream_in_state = IN_STREAM_RUN;
        }

        hdma_send_start();

        f_set_streams(g_streams);

        if (g_streams & L502_STREAM_ALL_OUT) {
            /* разрешаем ПЛИС генерацию TFS по SPORT */
            f_bf_reg |= L502_REGBIT_IOHARD_OUT_TFS_EN_Msk;
            fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, f_bf_reg);


            /* если не было предзагрузки до запуска,
               то запускаем прием сейчас */
            if ((g_stream_out_state == OUT_STREAM_STOP) ||
                (g_stream_out_state == OUT_STREAM_ERR)) {
                stream_out_preload();
            }

            if (g_stream_out_state == OUT_STREAM_PRELOAD) {
                /* выполняем предзагрузку данных */
                fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, f_bf_reg | 1);
                g_stream_out_state = OUT_STREAM_RUN;
            }
        }



        /* Так как конвейер автомата управления входной коммутации АЦП состоит
            из 2-х стадий, для корректного синхронного старта необходимо в
            ыполнить два раза предзагрузку. В противном случае,
            время момента первого отсчета может не совпадать с моментом
            запуска синхронизации
         */
        fpga_reg_write(L502_REGS_IOHARD_PRELOAD_ADC, 1);
        fpga_reg_write(L502_REGS_IOHARD_PRELOAD_ADC, 1);


        fpga_reg_write(L502_REGS_IOHARD_GO_SYNC_IO, 1);

        g_mode = L502_BF_MODE_STREAM;
    }
    return err;
}




/** @brief Останов синхронных потоков ввода-вывода.

    По этой функции останавливаются все синхронные потоки.
    Запрещается передача потоков по SPORT и по HostDMA

    @return Код ошибки */
int32_t streams_stop(void) {
    int32_t err = g_mode != L502_BF_MODE_STREAM ? L502_BF_ERR_STREAM_STOPPED : 0;
    if (!err) {
        hdma_send_stop();
        hdma_recv_stop();
        fpga_reg_write(L502_REGS_IOHARD_GO_SYNC_IO, 0);

        /* запрещаем прием и передачу по SPORT'у */
        sport_rx_stop();
        sport_tx_stop();

        /* запрещаем генерацию TFS и RFS */
        f_bf_reg = 0;
        fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, f_bf_reg);


        g_mode = L502_BF_MODE_IDLE;
        g_stream_in_state = IN_STREAM_STOP;
        g_stream_out_state = OUT_STREAM_STOP;
    }
    return err;
}



/** @brief Фоновая обработка потокой ввода-вывода

   Функция переодически вызывается из основного цикла программы.
   При рабочем режиме, проверяются, есть
   ли необработанные данные пришедшие от АЦП/DIGIN и/или пришедшие от ПК данные
   на ЦАП или DOUT. При их наличии вызывается соответствующая функция пользовательской
   обработки данных.
   Также, если было переполнение и все данные до переполнения были обработаны,
   то в ПК посылается слово о том, что в этом месте произошло переполнение */
void stream_proc(void) {
    //если запущен поток ввода 
    if ((g_stream_in_state != IN_STREAM_STOP) && (g_stream_in_state != IN_STREAM_ERR)) {
		
        uint32_t sport_rdy_size;
        uint32_t put_pos = f_sport_in_put_pos;

        //смотрим, сколько принято необработанных данных по sport'у 
        sport_rdy_size = put_pos  >= f_sport_in_proc_pos ?
             put_pos - f_sport_in_proc_pos : f_sport_in_buf_size - f_sport_in_proc_pos;
         //если есть необработанные данные - вызываем функцию для обработки 
         if (sport_rdy_size) {
             uint32_t processed = usr_in_proc_data(
                         (uint32_t*)&f_sport_in_buf[f_sport_in_proc_pos],
                         sport_rdy_size);
            //обновляем счетчик обработанных данных 
             f_sport_in_proc_pos += processed;
             if (f_sport_in_proc_pos==f_sport_in_buf_size)
                 f_sport_in_proc_pos = 0;
         }
	
         //если было переполнение - нужно передать слово о переполнениии.
         //   передаем его после того, как передадим все слова до переполнения 
        if ((g_stream_in_state == IN_STREAM_OV_ALERT) && !sport_rdy_size && hdma_send_req_rdy()) {
            hdma_send_req_start(&f_overflow_wrd, 1, 1);
            g_stream_in_state = IN_STREAM_ERR;
        }
    }

    //если есть поток на вывод 
    if ((g_stream_out_state == OUT_STREAM_PRELOAD) ||
        (g_stream_out_state == OUT_STREAM_RUN)) {
		
        uint32_t hdma_rdy_size;
        uint32_t put_pos = f_hdma_out_put_pos;

        //проверяем, сколько есть необработанных данных, принятых по HDMA 
        hdma_rdy_size = put_pos >= f_hdma_out_proc_pos ?
             put_pos - f_hdma_out_proc_pos : L502_HDMA_OUT_BUF_SIZE - f_hdma_out_proc_pos;

         //если такие есть -> пробуем обработать 
         if (hdma_rdy_size != 0) {
             uint32_t processed = usr_out_proc_data(
                             (uint32_t*)&f_hdma_out_buf[f_hdma_out_proc_pos],
                              hdma_rdy_size);

             //обновляем счетчик обработанных данных 
            f_hdma_out_proc_pos += processed;
             if (f_hdma_out_proc_pos==L502_HDMA_OUT_BUF_SIZE)
                 f_hdma_out_proc_pos = 0;
         }


         //если есть свободное место в буфере на прием и есть свободные
         //   дескрипторы => ставим новый запрос на прием даннх 
         if (hdma_recv_req_rdy()) {
			 
             uint32_t get_pos = f_hdma_out_get_pos;

             hdma_rdy_size = f_hdma_out_start_pos >= get_pos ?
                                 L502_HDMA_OUT_BUF_SIZE - f_hdma_out_start_pos + get_pos :
                                 get_pos -  f_hdma_out_start_pos;
             if (hdma_rdy_size > f_hdma_out_block_size) {
                 hdma_recv_req_start((uint32_t*)&f_hdma_out_buf[f_hdma_out_start_pos],
                                     f_hdma_out_block_size);

                 f_hdma_out_start_pos+=f_hdma_out_block_size;
                 if (f_hdma_out_start_pos==L502_HDMA_OUT_BUF_SIZE)
                     f_hdma_out_start_pos = 0;
             }
         }
    }
}

/** @brief Освобождение size слов из буфера приема по SPORT0

   Функция помечает, что size слов из начала той части буфера SPORT0, в которую
   были приняты данные, но не освобождены, как освобожденные. Т.е. в эту область
   снова можно будет принимать данные со SPORT0.
   При этом надо всегда следить, чтобы количество освобожденных данных не
   привышало количество обработанных!

   @param[in] size Размер освобожденных данных в 32-битных словах */
void stream_in_buf_free(uint32_t size) {
    /* обновляем позицию переданного слова */
    uint32_t get_pos = f_sport_in_get_pos;
    get_pos += size;
    if (get_pos >= f_sport_in_buf_size)
        get_pos-= f_sport_in_buf_size;
    f_sport_in_get_pos = get_pos;
}

/** @brief Освобождение size слов из буфера приема по HostDMA

   Функция помечает, что size слов из начала той части буфера HostDMA, в которую
   были приняты данные от ПК, но не освобождены, как освобожденные. Т.е. в эту область
   снова можно будет принимать данные от ПК по HostDMA.
   При этом надо всегда следить, чтобы количество освобожденных данных не
   привышало количество обработанных!

   @param[in] size Размер освобожденных данных в 32-битных словах */
void stream_out_buf_free(uint32_t size) {
    uint32_t get_pos = f_hdma_out_get_pos;
    get_pos += size;
    if (get_pos >= L502_HDMA_OUT_BUF_SIZE)
        get_pos -= L502_HDMA_OUT_BUF_SIZE;
    f_hdma_out_get_pos = get_pos;
}

/** @brief Обработка завершения приема по HostDMA

    Функция вызывается из обработчика прерывания, когда завершился прием
    блока данных по HDMA в ПК, поставленного до этого на передачу с
    помощью hdma_recv_req_start().
    Функция просто обновляет счетчик принятых данных (а обработка будет уже
    из фоновой функции stream_proc().

    @param[in] addr    Адрес слова, сразу за последним принятым словом.
    @param[in] size     Количество принятых 32-битных слов */
void hdma_recv_done(uint32_t* addr, uint32_t size) {
    /* обновляем позицию принятого слова */
    uint32_t put_pos = f_hdma_out_put_pos;
    put_pos += size;
    if (put_pos == L502_HDMA_OUT_BUF_SIZE)
        put_pos = 0;
    f_hdma_out_put_pos = put_pos;
}





/** @brief Размер буфера на прием.

    Функция возвращает размер буфера на прием по SPORT0
    @return размер буфера на прием в 32-битных словах */
extern uint32_t sport_in_buffer_size(void) {
    return L502_SPORT_IN_BUF_SIZE;
}

/** @} */


/** @addtogroup sport_rx
    @{  */

/** @brief Установка шага прерывания для према по SPORT0

    Функция устанавливает шаг прерываний для DMA, использующегося для
    приема данных синхронного ввода.
    При этом размер шага должен быть как имнимум в 4 раза меньше размера
    буфера #L502_SPORT_IN_BUF_SIZE.
    После установки шага определяется реально используемый размер буфера,
    как наибольшее число кратное шагу и не превышающее  #L502_SPORT_IN_BUF_SIZE.

    @param[in] size   Размер шага прерывания в 32-битных словах
    @return           Код ошибки */
int32_t sport_in_set_step_size(uint32_t size) {
    int mul;
    if ((size >= L502_SPORT_IN_BUF_SIZE/4) || (size > 0x8000))
        return L502_BF_ERR_INVALID_CMD_PARAMS;
    mul = L502_SPORT_IN_BUF_SIZE/size;
    /* не может быть боьше 0x8000 шагов, так как регистр
       YCNT 16-разрядный (+ еще умножаем на 2) */
    if (mul > 0x8000)
        mul = 0x8000;
    f_sport_in_buf_size = size*mul;
    f_sport_in_block_size = size;
    return 0;
}


/** @brief Запуск сбора данных по SPORT0

    Функция настраивает DMA3 на режим автобуфера с 2D, размер шага выбирается
    равным f_sport_in_block_size. После чего разрешается канал DMA и
    прием по SPORT0 */
void sport_rx_start(void) {
    static volatile int dummy;
    /* запрещаем DMA */
    *pDMA3_CONFIG = 0;
    ssync();
    /* вычитываем все данные из буфера, если они были */
    while (*pSPORT0_STAT & RXNE) {
        dummy = *pSPORT0_RX16;
        ssync();
    }

    /* настраиваем DMA */
    *pDMA3_START_ADDR = (void*)f_sport_in_buf;
    *pDMA3_X_COUNT = 2*f_sport_in_block_size; /* так как SPORT настроен на 16 бит (хоть и 2 канала),
                                                  а размер в 32 битных словах => умножаем на 2) */
    *pDMA3_X_MODIFY = 2;
    *pDMA3_Y_COUNT = f_sport_in_buf_size/f_sport_in_block_size;;
    *pDMA3_Y_MODIFY = 2;
    *pDMA3_CURR_ADDR = (void*)f_sport_in_buf;
    *pDMA3_CONFIG = FLOW_AUTO | DI_EN | DI_SEL | SYNC | DMA2D | WNR | WDSIZE_16; 
    
    
    ssync();
    /* разрешаем DMA */
    *pSIC_IMASK0 |= IRQ_DMA3;
    *pDMA3_CONFIG |= DMAEN;
    /* разрешаем прием по SPORT'у */
    *pSPORT0_RCR1 |= RSPEN;

    /* разрешаем генерацию RFS на SPORT0 */
    f_bf_reg |= L502_REGBIT_IOHARD_OUT_RFS_EN_Msk;
    fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, f_bf_reg);
}

 

/** @brief Останов сбора данных по SPORT0

    Функция запрещает прием по SPORT0 и останавливает DMA */
void sport_rx_stop(void) {
    /* останавливаем генерацию RFS */
    f_bf_reg &= ~L502_REGBIT_IOHARD_OUT_RFS_EN_Msk;
    fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, f_bf_reg);

    ssync();
    ssync();

    /* запрещаем прием по SPORT */
    *pSPORT0_RCR1 &= ~RSPEN;
    /* запрещаем DMA */
    *pDMA3_CONFIG =0; //&= ~DMAEN;
    *pSIC_IMASK0 &= ~IRQ_DMA3;
    //ssync();
}

/** @brief Обработчик прерывания по SPORT0 на прием.

    Прерывание возникает, когда был принят блок данных по SPORT0.
    Обновляем указатель принятых данных и проверяем переполнение */
ISR(isr_sport_dma_rx) {
    if (*pDMA3_IRQ_STATUS & DMA_DONE) {
        uint32_t rdy_put_pos;
        /* сбрасываем прерывание от DMA */
        *pDMA3_IRQ_STATUS = DMA_DONE;

        /* обновляем количество принятых данных на размер блока */
        f_sport_in_put_pos +=  f_sport_in_block_size;
        if (f_sport_in_put_pos == f_sport_in_buf_size)
            f_sport_in_put_pos = 0;

        /* смотрим, сколько свободно места в буфере на прием */
        uint32_t get_pos = f_sport_in_get_pos;
        rdy_put_pos = f_sport_in_put_pos > get_pos ?
                               f_sport_in_buf_size -f_sport_in_put_pos + get_pos :
                               get_pos - f_sport_in_put_pos;

        f_recv_size += f_sport_in_block_size;

        /* если осталось не больше блока - то считаем за переполнение,
            т.к. тогда при следующем прерывании уже могут быть испорчены
            принятые ранее данные */
        if (rdy_put_pos < 2*f_sport_in_block_size) {
            f_stream_in_set_overflow();
        }
    }
}

/** @} */









