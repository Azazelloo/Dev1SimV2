/** @defgroup fpga_regs ������ � ��������� ���� */

/***************************************************************************//**
    @addtogroup fpga_regs
    @{
    @file l502_fpga.h
    ���� �������� ������� ��� ������ � ������ ��������� ���� �� ���������� ���
*******************************************************************************/


#ifndef L502_FPGA_H_
#define L502_FPGA_H_

#include <stdint.h>
#include "l502_fpga_regs.h"

/** ������������� SPI ���������� */
void fpga_spi_init(void);

/** ������ �������� � ������� ���� �������� �� ���������� SPI
    @param[in] addr    ����� �������� ����
    @param[in] value   ������������ �������� */
void fpga_reg_write(uint16_t addr, uint32_t value);
/** ������ �������� �� �������� ���� �� ���������� SPI
    @param[in] addr    ����� �������� ����
    @return            ����������� �������� */
uint32_t fpga_reg_read(uint16_t addr);

#endif

/** @} */
