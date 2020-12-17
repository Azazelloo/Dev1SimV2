/** @defgroup sport_rx Прием потока данных по SPORT0 */

/***************************************************************************//**
    @addtogroup sport_rx
    @{
    @file l502_sport_rx.h

    Файл содержит описания функций для управления приемом потока по SPORT0
 ******************************************************************************/

#ifndef L502_SPORT_RX_H
#define L502_SPORT_RX_H

#include <stdint.h>

void sport_rx_start(void);
void sport_rx_stop(void);
int32_t sport_in_set_step_size(uint32_t size);


#endif // L502_SPORT_RX_H

/** @} */
