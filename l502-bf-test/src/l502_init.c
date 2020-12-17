#include <cdefBF523.h>
#include <ccblkfn.h>
#include <bfrom.h>
#include <sys/exception.h>
#include <stdlib.h>
#include <stdint.h>




#include "l502_cdefs.h"
#include "l502_fpga.h"
#include "l502_hdma.h"

ISR(isr_sport_dma_rx);
ISR(isr_sport_dma_tx);


void l502_stream_init(void);

/* fVCO = 530 MHZ = (20/2)*53, CDIV=1, SDIV = 4 => SCLK = 132.5 MHz */
#define L502_PLL_CTL         (SET_MSEL(53) | DF)
#define L502_PLL_DIV         (SET_SSEL(4) | CSEL_DIV1)

/* ���������� SDRAM
 * RDIV=((117964,8*64ms)/8192)-(6+3)=912 // �� ������ - ��� ��������� ������������
 * ������ - �������� �� 8192, � 4096, � ����� ������ 1834
 */
#define L502_SDRAM_SDRRC (((132500000 / 1000) * 64) / 8192 - (6 + 3))
/* ������ ������ - 32 ��, 9 ��� - ��� ����� ������� */
#define L502_SDRAM_SDBCTL (EBE | EBSZ_32 | EBCAW_9)
/* CAS latency=3, ���� ����� � 2 - ��� ������ ��� ����� (������ ��� ���� ����� ����������!!!!)
 * PASR_ALL - ���� ��� SDRAM � 2.5 � - �������� �������, ������� �������� ���
 * tRAS(min)=45 �� (��� ������� 120 ��� - 6 ������)
 * tRP(min)=20 �� (��� ������� 120 ��� - 3 ������)
 * tRCD(min)=20 �� (��� ������� 120 ��� - 3 ������)
 * tWR - ��, �� ������� 2
 * POWER startup delay - �� �����
 * PSS - power SDRAM - ������ ����
 * SRFS - ����� ��� �������� SDRAM � ����� ����������� ����������������� 0 �� �����
 * EBUFE=0 - ������ ���� ��� SDRAM
 * FBBRW=0 - ��� ����, ����� ������ ����� ��� �� �������, ����� �� �������� - ����������� �����
 * EMREN=0 - ���� ��� SDRAM � 2.5 � - �������� �������
 * TCSR=0 - ���� ��� SDRAM � 2.5 � - �������� �������
 * CDDBG=0 - �� ����� ������� signals �� ���������
 */
#define L502_SDRAM_SDGCTL (SCTLE | CL_2 | PASR_ALL | TRAS_6 | TRP_3 | TRCD_3 | TWR_2 | PSS)


uint32_t l502_otp_make_invalid(uint32_t page) {
    uint32_t err = bfrom_OtpCommand(OTP_INIT, (0x0A548800 | 133));
    if(!err) {
        uint64_t val = (uint64_t)3 << OTP_INVALID_P;
        err = bfrom_OtpWrite(page, OTP_LOWER_HALF | OTP_NO_ECC, &val);
    }
    return err;
}

/* ��������� ������� BlackFin'a */
void l502_setup_pll(void) {
    ADI_SYSCTRL_VALUES sysctl;
    sysctl.uwPllCtl = L502_PLL_CTL;
    bfrom_SysControl(SYSCTRL_WRITE | SYSCTRL_PLLCTL, &sysctl, 0);
}

/* ������ �������� PLL � SDRAM � ���� OTP, ������� � �������� �������� */
uint32_t l502_otp_write_cfg(uint32_t first_page) {
    uint32_t err = bfrom_OtpCommand(OTP_INIT, (0x0A548800 | 133));
    uint64_t val = 0;
    if (!err) {
        val = ((uint64_t)L502_PLL_DIV << OTP_PLL_DIV_P) | ((uint64_t)L502_PLL_CTL << OTP_PLL_CTL_P)
                 | ((uint64_t)OTP_SET_PLL_M<< 32)| ((uint64_t)OTP_LOAD_PBS02L_M<<32);
        err = bfrom_OtpWrite(first_page, OTP_LOWER_HALF | OTP_CHECK_FOR_PREV_WRITE, &val);
        if (!err) {
            val = ((uint64_t)L502_SDRAM_SDRRC << OTP_EBIU_SDRCC_P) | ((uint64_t)L502_SDRAM_SDBCTL << OTP_EBIU_SDBCTL_P)
                    | ((uint64_t)L502_SDRAM_SDGCTL << OTP_EBIU_SDGCTL_P);
            err = bfrom_OtpWrite(PBS02-PBS00+first_page, OTP_LOWER_HALF
                                    | OTP_CHECK_FOR_PREV_WRITE, &val);
        }

        /* ���� ���� ������ - ������ ���������������� ���� ���� */
        if (err)
            l502_otp_make_invalid(first_page);
    }
    return err;

}



/* ���������, ���� �� �������������� ��������� PLL � SDRAM � OTP. ���� ���,
   �� ���������� �� � OTP � �������������� PLL ������� */
void l502_otp_init(void) {
    uint32_t err=0, page, fnd=0, pll_setup=0;

    //err = l502_otp_make_invalid(PBS00);

    /* ���� ������ �������������� ���� ��������� �������� */
    for (page = PBS00; !(fnd && !err) && (page < 0xD8); page += 4) {
        uint64_t val;
        err = bfrom_OtpRead(page, OTP_LOWER_HALF, &val);
        if (!err && !((val>>OTP_INVALID_P)&0x3)) {
            fnd = 1;
            if (!val) {
                /* ���� ���� � ����������� �� ��� ������� => PLL ����������
                   ������� � ���������� ��������� ��� ���������� �������������
                   � ���������� */
                if (!pll_setup) {
                    l502_setup_pll();
                    pll_setup = 1;
                }
                err = l502_otp_write_cfg(page);
                page+=4;
                if (!err && (page< 0xD8)) {
                    /* ���� ���� ����� - �� ���������� ������ �����, ����� ������
                       ���� ���������, ��� ���� ���� ��� ������ ������ ���� ������,
                       ��� ���������� ��������� */
                    err = l502_otp_write_cfg(page);
                }
            }
        }
    }

    /* ���� ��� �������� �������� ���������, �� �������������� PLL,
        ��� ��� ������ ����� ��� ������� �� ������������������� */
    if (!fnd && !pll_setup) {
        l502_setup_pll();
    }

}

void l502_init(void) {
    /* ������������� OTP-������ � PLL, ���� ��� �������� �� ���� ���
     * �������������������� �� �����. ���� � OTP ��� ���� ������ ��������, ��
     * SDRAM � PLL ������������������� ��� ����������� ����� BlackFin */
    l502_otp_init();


    /* ��������� SPI */
    fpga_spi_init();

    /* ��������� SPORT0 */
    *pSPORT0_TCLKDIV = 0;
    *pSPORT0_RCLKDIV = 0;

    /* clk - internal, fs - external, req, active high, early */
    *pSPORT0_TCR1 = ITCLK | TFSR;  //TCKFE-???
    *pSPORT0_RCR1 = IRCLK | RFSR | RCKFE;
    /* len = 16 bit, secondary enable */
    *pSPORT0_TCR2 = SLEN(15) | TXSE;
    *pSPORT0_RCR2 = SLEN(15) | RXSE;

    *pPORTF_MUX = (*pPORTF_MUX & 0xFFFC) | 1;
    *pPORTF_FER |= PF0 | PF1 | PF2 | PF3 | PF4 | PF5 | PF6 | PF7;

    /* ���������� SPORT RX �� IVG7 */
    *pSIC_IAR2 = (*pSIC_IAR2 & 0xFFFFFFF0UL) | P16_IVG(7);
    REGISTER_ISR(7, isr_sport_dma_rx);
    /* SPORT TX ��������� �� IVG9 */
    REGISTER_ISR(9, isr_sport_dma_tx);

    /* ��������� HostDMA-���������� */
    hdma_init();


    /* ���� SDRAM �� ���������, �� �������������� �� */
    if (*pEBIU_SDSTAT & SDRS) {
        uint32_t* a=0;
        *pEBIU_SDRRC  = L502_SDRAM_SDRRC;
        *pEBIU_SDBCTL = L502_SDRAM_SDBCTL;
        *pEBIU_SDGCTL = L502_SDRAM_SDGCTL;
        ssync();

        *a = 0; /* ���������� �� �������� ������ ������������ �����, ����� ������������ ������ */

        while (*pEBIU_SDSTAT & SDRS) {}
    }

    /* �������������� ��������� ��� ������� �����/������ */
    l502_stream_init();


}
