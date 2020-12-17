/** @defgroup fpga_regs Доступ к регистрам ПЛИС */

/***************************************************************************//**
    @addtogroup fpga_regs
    @{
    @file l502_fpga.h
    Файл содержит функции для записи и чтения регистров ПЛИС по интерфейсу АЦП
*******************************************************************************/


#ifndef L502_FPGA_H_
#define L502_FPGA_H_

#include <stdint.h>
#include "l502_fpga_regs.h"

/** Инициализация SPI интерфейса */
void fpga_spi_init(void);

/** Запись регистра в регистр ПЛИС значения по интерфейсу SPI
    @param[in] addr    Адрес регистра ПЛИС
    @param[in] value   Записываемое значение */
void fpga_reg_write(uint16_t addr, uint32_t value);
/** Чтение значения из регистра ПЛИС по интерфейсу SPI
    @param[in] addr    Адрес регистра ПЛИС
    @return            Прочитанное значение */
uint32_t fpga_reg_read(uint16_t addr);

#endif

/** @} */
