/** @addtogroup sport_tx
    @{
    @file l502_sport_tx.c
    Файл содержит логику для управления передачей по SPORT0.
    Изначально DMA должен быть проинициализирован с помощью sport_tx_init(),
    Передача начинается с помщью sport_tx_start_req(). Можно поставить до 16
    запросов одновременно.
    Останвить все текущие передачи можно с помощью sport_tx_stop().
    ***********************************************************************/
    


#include <stdint.h>
#include <stdlib.h>


#include <cdefBF523.h>
#include <sys/exception.h>
#include <ccblkfn.h>
#include "l502_sport_tx.h"
#include "l502_cdefs.h"
#include "l502_fpga_regs.h"
#include "l502_fpga.h"
#include "l502_bf_cmd_defs.h"
#include "l502_defs.h"
#include "l502_stream.h"

#define SPORT_TX_DESCR_CNT 16

#define SPORT_TX_DMA_CFG_WAIT  (DMAEN | DI_EN | WDSIZE_16 | SYNC)
#define SPORT_TX_DMA_CFG_START (SPORT_TX_DMA_CFG_WAIT | NDSIZE_5 | FLOW_SMALL)



/* дескриптор DMA размера 5 16-битных слов */
typedef struct {
    uint16_t ndpl;
    uint16_t sal;
    uint16_t sah;
    uint16_t cfg;
    uint16_t xcnt;
} t_sport_dma_descr;


static t_sport_dma_descr f_descrs[SPORT_TX_DESCR_CNT];
static uint8_t f_put_descr, f_done_descr;
static volatile int f_put_cnt, f_done_cnt;
static int f_first;

static volatile int f_tx_was_empty = 0;

void sport_tx_done(uint32_t* addr, uint32_t size);

/***************************************************************************//**
  @brief Чтение флагов статуса вывода
  @return                Флаги статуса --- набор битов из #t_x502_out_status_flags,
                           объединенных через логическое “ИЛИ”.
 ******************************************************************************/
uint32_t sport_tx_out_status(void) {
    uint32_t ret = 0;
    if (f_put_cnt == f_done_cnt)
        ret |= X502_OUT_STATUS_FLAG_BUF_IS_EMPTY;
    if (f_tx_was_empty) {
        ret |= X502_OUT_STATUS_FLAG_BUF_WAS_EMPTY;
        f_tx_was_empty = 0;
    }
    return ret;
}



/** @brief Начальная инициализация канала DMA на передачу по SPORT.

    Функция устанавливает параметры DMA которые не меняются при последующей работе
    прошивки */
void sport_tx_init(void) {
    int i;
    for (i=0; i < SPORT_TX_DESCR_CNT; i++) {
        f_descrs[i].cfg = SPORT_TX_DMA_CFG_WAIT;
        f_descrs[i].ndpl = i==(SPORT_TX_DESCR_CNT-1) ?  (uint32_t)&f_descrs[0]&0xFFFF :
                                                        (uint32_t)&f_descrs[i+1]&0xFFFF;
    }

    *pDMA4_NEXT_DESC_PTR = f_descrs;
    *pDMA4_X_MODIFY = 2;
    f_put_cnt = f_done_cnt = 0;
    f_put_descr = f_done_descr = 0;
    f_first = 1;
}


/*  Функция разрешает канал DMA для передачи данных в SPORT0 и саму передачу
 *  для SPORT 0 */
static void f_sport_tx_start(void) {
    /* разрешаем передачу по SPORT'у */
    *pSPORT0_TCR1 |= TSPEN;
    f_first = 0;
    f_tx_was_empty = 0;
}


/** @brief Останов сбора по SPORT0.
 *
 *  Функция запрещает прием по SPORT0 и соответствующий канал DMA на прем */
void sport_tx_stop(void) {
    *pSIC_IMASK0 &= ~IRQ_DMA4;

    *pDMA4_CONFIG = 0;
    /* нужно подождать до 3-х циклов, чтобы DMA действительно завершился */
    ssync();
    ssync();
    ssync();
    /* запрет передачи по SPORT */
    *pSPORT0_TCR1 &= ~TSPEN;
    /* сбрасываем признаки прерываний от DMA */
    *pDMA4_IRQ_STATUS = DMA_DONE | DMA_ERR;

    /* заново инициализируем все дескрипторы */
    sport_tx_init();
}

/**************************************************************************//**
    @brief Получить количество свободных дескрипторов на передачу

    Функция возвращает количество запросов, которое можно поставить в очередь
    на передачу с помощью sport_tx_start_req().
    @return Количество запросов на передачу, которое можно поставить в очередь
 ******************************************************************************/
int sport_tx_req_rdy(void) {
    return SPORT_TX_DESCR_CNT - (f_put_cnt - f_done_cnt + 1);
}


/**************************************************************************//**
    @brief  Поставить запрос на передачу по SPORT0

    Функция ставит запрос на передачу указанных данных. Сами данные не копируются,
    т.е. буфер нельзя будет использовать до того, как данные не будут переданы!
    Для постановки запроса необходимо, чтобы был свободный дескриптор (можно
    узнать через sport_tx_req_rdy()).

    Количество слов в запросе не должно превышать #SPORT_TX_REQ_SIZE_MAX

    При добавление первого дескриптора автомтически разрешается передача по
    DMA и SPORT0.

    @param[in] buf       Указатель на массив на передачу.
    @param[in] size      Количество 32-битных слов на передачу
*******************************************************************************/
void sport_tx_start_req(uint32_t* buf, uint32_t size) {
    /* запрещаем прерывание от DMA на случай гонок с изменением позиций */
    *pSIC_IMASK0 &= ~IRQ_DMA4;

    f_descrs[f_put_descr].sal = (uint32_t)buf & 0xFFFF;
    f_descrs[f_put_descr].sah = ((uint32_t)buf >> 16) & 0xFFFF;
    f_descrs[f_put_descr].xcnt = size*2;
    f_descrs[f_put_descr].cfg = SPORT_TX_DMA_CFG_WAIT;
    /* убеждаемся, что данные будут уже записаны в память к моменту запуска DMA */
    ssync();

    /* если DMA сейчас остановлен => запускаем его */
    if (f_put_cnt==f_done_cnt) {
        uint32_t cfg_wrd = SPORT_TX_DMA_CFG_START;
        *pDMA4_CONFIG = cfg_wrd;
        if (f_first)
            f_sport_tx_start();
    }

    if (f_put_cnt!=f_done_cnt) {
        uint8_t prev_descr = (f_put_descr==0) ? SPORT_TX_DESCR_CNT-1 : f_put_descr-1;
        f_descrs[prev_descr].cfg = SPORT_TX_DMA_CFG_START;
    }

    if (++f_put_descr==SPORT_TX_DESCR_CNT)
        f_put_descr=0;
    f_put_cnt++;

    *pSIC_IMASK0 |= IRQ_DMA4;
}


/** @brief Обработчик прерывания по SPORT0 на завершение передачи.

    Прерывание возникает, когда был передан блок данных по SPORT0, соответствующий
    одному дескриптору.
    Функция рассчитывает количество переданных данных и вызывает sport_tx_done().
    Также функция помечает дескриптор как свободный и при необходимости
    снова запускает DMA (если он был остановлен, но при этом уже добавлен новый
    дескриптор на передачу) */
ISR( isr_sport_dma_tx) {
    if (*pDMA4_IRQ_STATUS & DMA_DONE) {
        uint32_t* addr;
        uint32_t size;
        uint32_t status;

        *pDMA4_IRQ_STATUS = DMA_DONE;

        /* обработку выполняем только в случае, если есть начатые, но не завершенные
           дескрипторы */
        if (f_done_cnt != f_put_cnt) {
            f_done_cnt++;

            f_descrs[f_done_descr].cfg = SPORT_TX_DMA_CFG_WAIT;

            addr = ((uint32_t*)(f_descrs[f_done_descr].sal |
                    ((uint32_t)f_descrs[f_done_descr].sah<<16)) + f_descrs[f_done_descr].xcnt*2);
            size = f_descrs[f_done_descr].xcnt/2;
            if (!size)
                size = 0x8000;
            sport_tx_done(addr, size);

            if (++f_done_descr==SPORT_TX_DESCR_CNT)
                f_done_descr=0;

            ssync();

            status = *pDMA4_IRQ_STATUS;
            /* если DMA остановлен, а дескрипторы на передачу еще есть => запускаем снова */
            if ((f_done_cnt != f_put_cnt) && !(status & DMA_RUN)) {
                *pDMA4_CONFIG = SPORT_TX_DMA_CFG_START;
            }

            if (f_done_cnt == f_put_cnt) {
                ssync();
            }

           if ((g_stream_out_state == OUT_STREAM_RUN) && (f_done_cnt == f_put_cnt))
                f_tx_was_empty = 1;
        }
    }

    if (*pDMA4_IRQ_STATUS & DMA_ERR) {
        *pDMA4_IRQ_STATUS = DMA_ERR;
    }

}

/** @} */
