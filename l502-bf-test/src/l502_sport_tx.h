/** @defgroup sport_tx �������� ������ ������ �� SPORT0 */

/***************************************************************************//**
    @addtogroup sport_tx
    @{
    @file l502_sport_tx.h

    ���� �������� �������� ������� ��� ���������� �������� ������ �� SPORT0
 ******************************************************************************/

#ifndef L502_SPORT_TX_H
#define L502_SPORT_TX_H

/** ������������ ������ ��� �������� � ������� sport_tx_start_req() */
#define SPORT_TX_REQ_SIZE_MAX   (16*1024)

void sport_tx_init(void);
void sport_tx_stop(void);
int sport_tx_req_rdy(void);


void sport_tx_start_req(uint32_t* addr, uint32_t size);

uint32_t sport_tx_out_status(void);

#endif

/** @} */
