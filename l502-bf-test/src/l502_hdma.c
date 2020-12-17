/***************************************************************************//**
    @addtogroup hdma
    @{
    @file l502_hdma.c
    Файл содержит логику работы с hdma на прием и на передачу данных
    как из, так и в BlackFin.    
 ***************************************************************************/

/******************************************************************************
 При запуске должно вызываться один раз hdma_stream_init().
Вызов hdma_xxx_start() инициализирует прием или передачу, а hdma_xxx_stop()
останавливает все текущие передачи.

Для запуска обмена блоком данных нужно проверить готовность
(наличие свободных дескрипторов) с помощью hdma_xxx_req_rdy()
и вызвать hdma_xxx_req_start(), указав массив для передачи или приема
данных и его размер.

Можно устанавливать до 31 запроса в очередь.

По завершению обмена будет вызвана функция hdma_xxx_done(), которая
должна быть реализована в другом файле. Для передачи BF->PC
функция вызывается один раз на каждый поставленный запрос,
а при приеме PC->BF может вызываться по несколько раз - при приеме
части запроса (в случае, если от PC пока нет больше данных)
*******************************************************************************/


#include "l502_global.h"
#include "l502_cdefs.h"
#include "l502_cmd.h"


#include <cdefBF523.h>
#include <sys/exception.h>
#include <ccblkfn.h>


#define STREAM_IN_EN()  *pPORTFIO_CLEAR = PF14
#define STREAM_IN_DIS() *pPORTFIO_SET = PF14
#define STREAM_OUT_EN()  *pPORTFIO_CLEAR = PF15
#define STREAM_OUT_DIS() *pPORTFIO_SET = PF15

#define STREAM_IN_SET_REQ() *pPORTGIO_TOGGLE = PG5
#define STREAM_OUT_SET_REQ() *pPORTGIO_TOGGLE = PG6



#define L502_HDMA_STEP_SIZE_MAX        256


static uint16_t f_snd_start_id, f_rcv_start_id;
static uint16_t f_snd_done_id, f_rcv_done_id;
static uint8_t f_snd_next_descr, f_rcv_next_descr;
static uint8_t f_rcv_done_descr;


void hdma_send_done(uint32_t* addr, uint32_t size);
void hdma_recv_done(uint32_t* addr, uint32_t size);


ISR(hdma_isr);
ISR(hdma_rd_isr);




inline static void hdma_set_descr(t_hdma_stream_descr* descr_arr, uint8_t* pos,
                                  uint16_t* id, uint32_t* addr, uint32_t size, uint32_t flags) {
    descr_arr[*pos].flags = flags;
    descr_arr[*pos].addr =  addr;
    descr_arr[*pos].id = *id;
    descr_arr[*pos].full_size = size*2;
    descr_arr[*pos].xcnt = size > L502_HDMA_STEP_SIZE_MAX ? 2*L502_HDMA_STEP_SIZE_MAX : 2*size;
    descr_arr[*pos].udata = size;
    *id = *id+1;

    *pos=*pos+1;
    if (*pos==    L502_IN_HDMA_DESCR_CNT)
        *pos = 0;
}


/** @brief Инициализация интерфейса HostDMA

    Настройка параметров HostDMA и инициализация неизменяемых полей дескрипторов
    для организации потока по данных по HostDMA */
void hdma_init(void) {
    int d;

    /* запрещаем ПЛИС отслеживать запросы на обмен по HDMA */
    STREAM_IN_DIS();
    STREAM_OUT_DIS();

    *pPORTGIO_DIR |= PG5 | PG6;
    *pPORTFIO_DIR |= PF14 | PF15;

    /*****************         настройка HOST DMA         ***********************/
    //настройка портов
    *pPORTG_MUX |= 0x2800;
    *pPORTG_FER |= 0xF800;
    *pPORTH_MUX = 0x2A;
    *pPORTH_FER = 0xFFFF;


    //настройка прерываний
    REGISTER_ISR(11, hdma_isr);
    //*pSIC_IAR3 = (*pSIC_IAR6 & 0xFFF0FFFFUL) | (3 << 16);
    *pSIC_IAR6 = (*pSIC_IAR6 & 0xFFFFF0FFUL) | P50_IVG(10); //назначение HDMARD на IVG10
    REGISTER_ISR(10, hdma_rd_isr);
    *pSIC_IMASK0 |= IRQ_DMA1; //разрешение прерывания HOSTDP на запись
    *pSIC_IMASK1 |= IRQ_HOSTRD_DONE;   //разрешение прерывания HOSTDP на чтение;
    //разрешение HDMA
    *pHOST_CONTROL =  BDR | EHR | EHW | HOSTDP_EN | HOSTDP_DATA_SIZE; //burst, ehr, ehw, en

    //g_state.cmd.data[100] = L502_BF_CMD_STATUS_DONE;

    /* инициалзация полей дескрипторов, которые не будут изменяться
       во время работы */
    for (d=0; d < L502_IN_HDMA_DESCR_CNT; d++) {
        g_state.hdma.in[d].xmod = 2;
        g_state.hdma.in[d].valid = 1;
        g_state.hdma.in[d].next_descr = d==(    L502_IN_HDMA_DESCR_CNT-1) ?
                    (void*)&g_state.hdma.in[0] :    (void*)&g_state.hdma.in[d+1];
    }

    for (d=0; d < L502_OUT_HDMA_DESCR_CNT; d++) {
        g_state.hdma.out[d].xmod = 2;
        g_state.hdma.out[d].valid = 1;
        g_state.hdma.out[d].next_descr = d==(    L502_IN_HDMA_DESCR_CNT-1) ?
                (void*)&g_state.hdma.out[0] :    (void*)&g_state.hdma.out[d+1];
    }
}



/** @brief  Запуск потока на передачу по HostDMA.

    Функция сбрасывает логику обработки заданий на передачу данных по HostDMA
    и разрешает передачу. Должна вызываться до добавления первого задания с помощью
    hdma_send_req_start() */
void hdma_send_start(void) {
    f_snd_start_id = 0;
    f_snd_done_id = 0;
    f_snd_next_descr = 0;

    g_state.hdma.in_lb.valid = 0;

    STREAM_IN_EN();
}

/** @brief  Останов потока на передачу по HostDMA.

    Запрет передачи по HostDMA с остановом всех текущих заданий */
void hdma_send_stop(void) {
    STREAM_IN_DIS();
}

/** @brief  Запуск потока на прием по HostDMA

    Функция сбрасывает логику обработки заданий на прием данных по HostDMA
    и разрешает прием. Должна вызываться до добавления первого задания с помощью
    hdma_recv_req_start() */
void hdma_recv_start(void) {
    f_rcv_start_id = 0;
    f_rcv_done_id = 0;
    f_rcv_next_descr = 0;
    f_rcv_done_descr = 0;

    g_state.hdma.out_lb.valid = 0;
    STREAM_OUT_EN();
}

/** @brief  Останов потока на прием по HostDMA

    Запрет приема по HostDMA с остановом всех текущих заданий */
void hdma_recv_stop(void) {
    STREAM_OUT_DIS();
}



/**************************************************************************//**
    @brief Получить количество свободных запросов на передачу.

    Фунция позволяет узнать, сколько запросов можно еще поставить в очередь на
    передачу с помощью hdma_send_start().
    @return Количество запросов на передачу, которое можно поставить в очередь
 ******************************************************************************/
int hdma_send_req_rdy(void) {
    return  L502_IN_HDMA_DESCR_CNT - (uint16_t)(f_snd_start_id - f_snd_done_id);
}

/**************************************************************************//**
    @brief Получить количество свободных запросов на прием

    Фунция позволяет узнать, сколько запросов можно еще поставить в очередь на
    прием с помощью hdma_recv_start().
    @return Количество запросов на прием, которое можно поставить в очередь
 ******************************************************************************/
int hdma_recv_req_rdy(void) {
    return L502_OUT_HDMA_DESCR_CNT - (uint16_t)(f_rcv_start_id - f_rcv_done_id);
}

/**************************************************************************//**
    @brief  Поставить запрос на передачу по HostDMA

    Функция ставит запрос на передачу указанных данных. Сами данные не копируются,
    т.е. буфер нельзя будет использовать до того, как данные не будут переданы!
    Для постановки запроса необходимо, чтобы был свободный дескриптор (можно
    узнать через hdma_send_req_rdy())

    @param[in] buf   Указатель на массив на передачу.
    @param[in] size      Количество 32-битных слов на передачу
    @param[in] flags     Флаги из #t_hdma_send_flags
    @return              < 0 при ошибке, >= 0 - id передачи при успехе
*******************************************************************************/
int hdma_send_req_start(const uint32_t* buf, uint32_t size, uint32_t flags) {
    if (hdma_send_req_rdy() > 0) {
        hdma_set_descr((t_hdma_stream_descr*)g_state.hdma.in, &f_snd_next_descr, &f_snd_start_id,
                       (uint32_t*)buf, size, flags);
        STREAM_IN_SET_REQ();
        return f_snd_start_id-1;
    }
    return -1;
}

/**************************************************************************//**
    @brief  Поставить запрос на передачу по HostDMA

    Функция ставит запрос на прием данных в указанный буфер.
    Сами данные будут в буфере только по завершению запроса.
    Для постановки запроса необходимо, чтобы был свободный дескриптор (можно
    узнать через hdma_recv_req_rdy())

    @param[in] buf   Указатель на массив на передачу.
    @param[in] size      Количество 32-битных слов на передачу
    @return              < 0 при ошибке, >= 0 - id передачи при успехе
*******************************************************************************/
int hdma_recv_req_start(uint32_t* buf, uint32_t size) {
    if (hdma_recv_req_rdy() > 0) {
        hdma_set_descr((t_hdma_stream_descr*)g_state.hdma.out, &f_rcv_next_descr, &f_rcv_start_id, buf, size, 0);
        STREAM_OUT_SET_REQ();
        return f_rcv_start_id-1;
    }
    return -1;
}


/**
   @brief Обработчик прерывания на завершения записи в память BF по HDMA

   Обработчик вызывается по завершению приема блока по HostDMA. Обработчик
   выполняет установку необходимых флагов для разрешения приема следующего блока
   и, кроме того, проверяет наличие новой команды и завершения передачи или приема
   блока из потока данных */
ISR(hdma_isr) {
    if ((*pDMA1_IRQ_STATUS & DMA_DONE) != 0) {
        /* проверяем, не была ли записана команда */
        if (g_state.cmd.status == L502_BF_CMD_STATUS_REQ) {
            l502_cmd_set_req();
        }
        /* проверяем, не был ли записан результат передачи
           по HDMA из BF в PC */
        if (g_state.hdma.in_lb.valid) {
            /* обновляем id завершенной передачи и вызываем callback */
            f_snd_done_id = g_state.hdma.in_lb.id;
            g_state.hdma.in_lb.valid = 0;
            hdma_send_done(g_state.hdma.in_lb.addr, g_state.hdma.in_lb.udata);
        }
        /* проверяем, не был ли записан результат приема данных по
            HDMA из PC в BF */
        if (g_state.hdma.out_lb.valid) {
            /* может быть записан и при не полностью завершенном запросе.
               определяем сперва резмер, сколько было реально принято */
            uint32_t size = (g_state.hdma.out[f_rcv_done_descr].full_size -
                            g_state.hdma.out_lb.full_size)/2;

            g_state.hdma.out_lb.valid = 0;

            hdma_recv_done(g_state.hdma.out_lb.addr, size);

            /* если была завершена только часть дескриптора -
               обновляем оставшийся размер для приема */
            if (g_state.hdma.out_lb.full_size) {
                g_state.hdma.out[f_rcv_done_descr].full_size =
                            g_state.hdma.out_lb.full_size;
            } else {
                /* если завершен весь дескриптор - переходим
                   к следующему */
                if (++f_rcv_done_descr==    L502_OUT_HDMA_DESCR_CNT)
                    f_rcv_done_descr = 0;
                f_rcv_done_id = g_state.hdma.out_lb.id;
            }
        }

        *pDMA1_IRQ_STATUS = DMA_DONE;
        csync();
        *pHOST_STATUS = DMA_CMPLT;
    }
    ssync();
}

/**
    @brief Обработчик прерывания на завершение чтения по HDMA.

    Данный обработчик вызывается по завершению передачи блока данных по HostDMA.
    Выполняет только установку необходимых флагов для разрешения следующей
    передачи */
ISR(hdma_rd_isr) {
    if ((*pHOST_STATUS & HOSTRD_DONE) != 0) {
        *pHOST_STATUS &= ~((unsigned short)HOSTRD_DONE);
        *pHOST_STATUS |= DMA_CMPLT;
    }
    ssync();
}


/** @} */
