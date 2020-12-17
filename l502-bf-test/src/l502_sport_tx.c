/** @addtogroup sport_tx
    @{
    @file l502_sport_tx.c
    ���� �������� ������ ��� ���������� ��������� �� SPORT0.
    ���������� DMA ������ ���� ������������������ � ������� sport_tx_init(),
    �������� ���������� � ������ sport_tx_start_req(). ����� ��������� �� 16
    �������� ������������.
    ��������� ��� ������� �������� ����� � ������� sport_tx_stop().
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



/* ���������� DMA ������� 5 16-������ ���� */
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
  @brief ������ ������ ������� ������
  @return                ����� ������� --- ����� ����� �� #t_x502_out_status_flags,
                           ������������ ����� ���������� ���Ȕ.
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



/** @brief ��������� ������������� ������ DMA �� �������� �� SPORT.

    ������� ������������� ��������� DMA ������� �� �������� ��� ����������� ������
    �������� */
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


/*  ������� ��������� ����� DMA ��� �������� ������ � SPORT0 � ���� ��������
 *  ��� SPORT 0 */
static void f_sport_tx_start(void) {
    /* ��������� �������� �� SPORT'� */
    *pSPORT0_TCR1 |= TSPEN;
    f_first = 0;
    f_tx_was_empty = 0;
}


/** @brief ������� ����� �� SPORT0.
 *
 *  ������� ��������� ����� �� SPORT0 � ��������������� ����� DMA �� ���� */
void sport_tx_stop(void) {
    *pSIC_IMASK0 &= ~IRQ_DMA4;

    *pDMA4_CONFIG = 0;
    /* ����� ��������� �� 3-� ������, ����� DMA ������������� ���������� */
    ssync();
    ssync();
    ssync();
    /* ������ �������� �� SPORT */
    *pSPORT0_TCR1 &= ~TSPEN;
    /* ���������� �������� ���������� �� DMA */
    *pDMA4_IRQ_STATUS = DMA_DONE | DMA_ERR;

    /* ������ �������������� ��� ����������� */
    sport_tx_init();
}

/**************************************************************************//**
    @brief �������� ���������� ��������� ������������ �� ��������

    ������� ���������� ���������� ��������, ������� ����� ��������� � �������
    �� �������� � ������� sport_tx_start_req().
    @return ���������� �������� �� ��������, ������� ����� ��������� � �������
 ******************************************************************************/
int sport_tx_req_rdy(void) {
    return SPORT_TX_DESCR_CNT - (f_put_cnt - f_done_cnt + 1);
}


/**************************************************************************//**
    @brief  ��������� ������ �� �������� �� SPORT0

    ������� ������ ������ �� �������� ��������� ������. ���� ������ �� ����������,
    �.�. ����� ������ ����� ������������ �� ����, ��� ������ �� ����� ��������!
    ��� ���������� ������� ����������, ����� ��� ��������� ���������� (�����
    ������ ����� sport_tx_req_rdy()).

    ���������� ���� � ������� �� ������ ��������� #SPORT_TX_REQ_SIZE_MAX

    ��� ���������� ������� ����������� ������������ ����������� �������� ��
    DMA � SPORT0.

    @param[in] buf       ��������� �� ������ �� ��������.
    @param[in] size      ���������� 32-������ ���� �� ��������
*******************************************************************************/
void sport_tx_start_req(uint32_t* buf, uint32_t size) {
    /* ��������� ���������� �� DMA �� ������ ����� � ���������� ������� */
    *pSIC_IMASK0 &= ~IRQ_DMA4;

    f_descrs[f_put_descr].sal = (uint32_t)buf & 0xFFFF;
    f_descrs[f_put_descr].sah = ((uint32_t)buf >> 16) & 0xFFFF;
    f_descrs[f_put_descr].xcnt = size*2;
    f_descrs[f_put_descr].cfg = SPORT_TX_DMA_CFG_WAIT;
    /* ����������, ��� ������ ����� ��� �������� � ������ � ������� ������� DMA */
    ssync();

    /* ���� DMA ������ ���������� => ��������� ��� */
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


/** @brief ���������� ���������� �� SPORT0 �� ���������� ��������.

    ���������� ���������, ����� ��� ������� ���� ������ �� SPORT0, ���������������
    ������ �����������.
    ������� ������������ ���������� ���������� ������ � �������� sport_tx_done().
    ����� ������� �������� ���������� ��� ��������� � ��� �������������
    ����� ��������� DMA (���� �� ��� ����������, �� ��� ���� ��� �������� �����
    ���������� �� ��������) */
ISR( isr_sport_dma_tx) {
    if (*pDMA4_IRQ_STATUS & DMA_DONE) {
        uint32_t* addr;
        uint32_t size;
        uint32_t status;

        *pDMA4_IRQ_STATUS = DMA_DONE;

        /* ��������� ��������� ������ � ������, ���� ���� �������, �� �� �����������
           ����������� */
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
            /* ���� DMA ����������, � ����������� �� �������� ��� ���� => ��������� ����� */
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
