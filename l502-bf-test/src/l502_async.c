/***************************************************************************//**
    @addtogroup async_io
    @{
    @file l502_async.c
    Файл содержит реализацию функций для асинхронного ввода/вывода
    (пока только вывода)
 ******************************************************************************/
#include <stdlib.h>
    
#include "l502_cmd.h"
#include "l502_global.h"
#include "l502_fpga.h"
#include "l502_defs.h"
#include "l502_async.h"
#include "l502_fpga_regs.h"




void async_dac_out(uint8_t ch, int32_t val) {
    val &= 0xFFFF;
    if (ch==L502_DAC_CH1) {
        val |= L502_STREAM_OUT_WORD_TYPE_DAC1;
    } else {
        val |= L502_STREAM_OUT_WORD_TYPE_DAC2;
    }
    fpga_reg_write(L502_REGS_IOHARD_ASYNC_OUT, val);
}


void async_dout(uint32_t val, uint32_t msk) {
    static uint32_t last_out = L502_DIGOUT_WORD_DIS_H | L502_DIGOUT_WORD_DIS_L;
    if (msk != 0) {
        val &= ~msk;
        val |= last_out & msk;
    }
    val &= 0xFFFF;

    fpga_reg_write(L502_REGS_IOHARD_ASYNC_OUT, val);
    last_out = val;
}

/** @} */
