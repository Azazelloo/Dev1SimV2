/** @addtogroup fpga_regs
    @{
    @file l502_fpga.c
    ���� �������� ������ �������� ������ ��� ������/������ ���������
    ���� �� SPI.
    ���������� SPI ������ ���� ������������������ � ������� fpga_spi_init().
    ����� ����� ����� ������������ ������ � ������� fpga_reg_write(), �
    ������ � ������� fpga_reg_read(). */

#include <cdefBF523.h>

#include "l502_fpga.h"

#define L502_SPI_BIT_START   0x8000UL
#define L502_SPI_BIT_WR      0x4000UL
#define L502_SPI_MSK_ADDR    0x3FFFUL


static uint16_t f_spi_rw(uint16_t word) {
    /* ���� � ���� ������� ��� ����� - �� ������ ���, ����� ���������� ����� ������ */
    if (*pSPI_STAT & RXS) {
        volatile uint16_t dummy;
        dummy = *pSPI_RDBR;
    }

    *pSPI_TDBR = word;
    /* ���� ���� ������ ����� ����� */
    while (!(*pSPI_STAT&RXS)) {
        continue;
    }

    return *pSPI_RDBR;
}


void fpga_spi_init(void) {
        /* ��������� SPI */
    *pSPI_BAUD = 2; /* SPI CLK = 132.5/(2*2) = 33.125 */
    *pSPI_CTL = SPE | MSTR | SIZE | GM | TDBR_CORE; /* ������, MSB first, 16-bit, CPHA=0, CPOL=0 */
    *pSPI_FLG = FLS1;

    *pPORTG_MUX = (*pPORTG_MUX & 0xFFFC) | 2;
    *pPORTG_FER |= PG1 | PG2 | PG3 | PG4;

    fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, 0);
}


void fpga_reg_write(uint16_t addr, uint32_t value) {
    f_spi_rw((addr&L502_SPI_MSK_ADDR) | L502_SPI_BIT_START | L502_SPI_BIT_WR);
    f_spi_rw((value>>24)&0xFF);
    f_spi_rw((value>>16)&0xFF);
    f_spi_rw((value>>8)&0xFF);
    f_spi_rw(value&0xFF);
}


uint32_t fpga_reg_read(uint16_t addr) {
    uint32_t ret = 0;
    f_spi_rw((addr&L502_SPI_MSK_ADDR) | L502_SPI_BIT_START);
    f_spi_rw(0);
    f_spi_rw(0);
    ret = f_spi_rw(0);
    ret <<= 16;
    ret |= f_spi_rw(0);
    return ret;
}

/** @} */
