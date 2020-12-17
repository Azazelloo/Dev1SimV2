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

/* конфигурим SDRAM
 * RDIV=((117964,8*64ms)/8192)-(6+3)=912 // по логике - это последняя конфигурация
 * кстати - возможно не 8192, а 4096, в таком случае 1834
 */
#define L502_SDRAM_SDRRC (((132500000 / 1000) * 64) / 8192 - (6 + 3))
/* размер памяти - 32 Мб, 9 бит - под адрес колонки */
#define L502_SDRAM_SDBCTL (EBE | EBSZ_32 | EBCAW_9)
/* CAS latency=3, хотя можно и 2 - чем меньше тем лучше (правда при этом глюки появляются!!!!)
 * PASR_ALL - тоже для SDRAM с 2.5 В - экономия энергии, поэтому рефрешим все
 * tRAS(min)=45 нс (при частоте 120 Мгц - 6 тактов)
 * tRP(min)=20 нс (при частоте 120 Мгц - 3 тактов)
 * tRCD(min)=20 нс (при частоте 120 Мгц - 3 тактов)
 * tWR - хз, на вскидку 2
 * POWER startup delay - не нужна
 * PSS - power SDRAM - должно быть
 * SRFS - нужна для перевода SDRAM в режим пониженного энергопотребления 0 не нужно
 * EBUFE=0 - только один чип SDRAM
 * FBBRW=0 - для того, чтобы чтение сразу шло за записью, может не работать - попробовать позже
 * EMREN=0 - тоже для SDRAM с 2.5 В - экономия энергии
 * TCSR=0 - тоже для SDRAM с 2.5 В - экономия энергии
 * CDDBG=0 - по моему сотекщд signals не расшарены
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

/* Настройка частоты BlackFin'a */
void l502_setup_pll(void) {
    ADI_SYSCTRL_VALUES sysctl;
    sysctl.uwPllCtl = L502_PLL_CTL;
    bfrom_SysControl(SYSCTRL_WRITE | SYSCTRL_PLLCTL, &sysctl, 0);
}

/* Запись настроек PLL и SDRAM в блок OTP, начиная с заданной страницы */
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

        /* если была ошибка - делаем недействительным весь блок */
        if (err)
            l502_otp_make_invalid(first_page);
    }
    return err;

}



/* Проверяем, есть ли действтиельные настройки PLL и SDRAM в OTP. Если нет,
   то записываем их в OTP и инициализируем PLL вручную */
void l502_otp_init(void) {
    uint32_t err=0, page, fnd=0, pll_setup=0;

    //err = l502_otp_make_invalid(PBS00);

    /* ищем первый действительный блок настройки загрузки */
    for (page = PBS00; !(fnd && !err) && (page < 0xD8); page += 4) {
        uint64_t val;
        err = bfrom_OtpRead(page, OTP_LOWER_HALF, &val);
        if (!err && !((val>>OTP_INVALID_P)&0x3)) {
            fnd = 1;
            if (!val) {
                /* если блок с настройками не был записан => PLL записываем
                   вручную и записываем настройки для корректной инициализации
                   в дальнейшем */
                if (!pll_setup) {
                    l502_setup_pll();
                    pll_setup = 1;
                }
                err = l502_otp_write_cfg(page);
                page+=4;
                if (!err && (page< 0xD8)) {
                    /* если есть место - то дописываем вторую копию, чтобы всегда
                       быть уверенным, что если даже при первом чтении была ошибка,
                       все загрузится нормально */
                    err = l502_otp_write_cfg(page);
                }
            }
        }
    }

    /* если все страницы настроек испорчены, то инициализируем PLL,
        так как скорее всего его система не проинициализировала */
    if (!fnd && !pll_setup) {
        l502_setup_pll();
    }

}

void l502_init(void) {
    /* инициализация OTP-памяти и PLL, если эти значения не были уже
     * проинициализированны до этого. Если в OTP уже были нужные значения, то
     * SDRAM и PLL проинициализированы уже загрузочным кодом BlackFin */
    l502_otp_init();


    /* настройка SPI */
    fpga_spi_init();

    /* настройки SPORT0 */
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

    /* назначение SPORT RX на IVG7 */
    *pSIC_IAR2 = (*pSIC_IAR2 & 0xFFFFFFF0UL) | P16_IVG(7);
    REGISTER_ISR(7, isr_sport_dma_rx);
    /* SPORT TX оставляем на IVG9 */
    REGISTER_ISR(9, isr_sport_dma_tx);

    /* настройка HostDMA-интерфейса */
    hdma_init();


    /* если SDRAM не настроена, то инициализируем ее */
    if (*pEBIU_SDSTAT & SDRS) {
        uint32_t* a=0;
        *pEBIU_SDRRC  = L502_SDRAM_SDRRC;
        *pEBIU_SDBCTL = L502_SDRAM_SDBCTL;
        *pEBIU_SDGCTL = L502_SDRAM_SDGCTL;
        ssync();

        *a = 0; /* записываем по нулевому адресу произвольное слово, чтобы активировать память */

        while (*pEBIU_SDSTAT & SDRS) {}
    }

    /* инициализируем параметры для потоков ввода/вывода */
    l502_stream_init();


}
