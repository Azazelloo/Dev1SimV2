/** @file l502_test.c
    ������ ���� �������� ���������� ������ ����������.
    ��� ����� ������������ ��� ������� ������� � �� ����������� � ������� ������ ������,
    ������ ��������� �� � �������� ��������� ��������� �������� � ����� ����� �������
    (������ �� �� ����� ����� ������ � �.�.)
    
    ��� ���������� ������� ������������� ����������� ������� �� PC � BlackFin: L502_BF_CMD_CODE_TEST.
    �������� ���� �������� ���������� ��������: 
        - ��������� ���� � �������� �������
        - ���������� ������� ����
        - �������� ���������� ������������ � ������ ������ ����� ���
            ���������� ������������ ����� (���� ���� ����������).
    
    �� ������-���� ����� ������ ��������� � �������� ����� (���������� g_mode)
    � ��������� �������, ��������������� ��������� ����� (�� ���� ������� ���� 
    ������ ������������ ��������� ������ ������ ������ �� PC).
    ���� ����������� ���� �� ������, ���� �� ������� ������� ��������).
    ������ ��������� ����� ����� �������� � ���������� L502_BF_CMD_TEST_GET_RESULT.
    
    �������� ��������� �����:
        - �������� SDRAM
        - �������� SPI
        - �������� SPORT

**************************************************************************************************/ 


#include <stdlib.h>
#include <cdefBF523.h>
#include <sys/exception.h>

#include "l502_cmd.h"
#include "l502_fpga.h"
#include "l502_sport_tx.h"

#include <string.h>



extern int g_mode;

/* ������ ��������, ������������� ��� ������ SDRAM � SPORT */
#define L502_TEST_CNTR_MODULE 35317

/* ������ ������ ����� SDRAM */
#define SDRAM_BANK_SIZE (4UL*1024*1024*2)

/* ������ ������ SDRAM */
static volatile uint16_t* bank1 = NULL;
static volatile uint16_t* bank2 = (uint16_t*)(SDRAM_BANK_SIZE);
static volatile uint16_t* bank3 = (uint16_t*)(2*SDRAM_BANK_SIZE);
static volatile uint16_t* bank4 = (uint16_t*)(3*SDRAM_BANK_SIZE);


#define SPORT_TEST_START_ADDR 0xFF900000
#define SPORT_TEST_BUF_SIZE   4096

/* ��������� ���������� ����� */
static t_l502_bf_test_res f_test_res;
/* ����� ���������� ������������ ����� */
static int32_t f_cur_test_ind = -1;

/* ���������� ������� ������� ������ */
static int f_sdram_test(void);
static int f_spi_test(void);
static int f_sport_test(void);


/* ���������, ����������� ������������ ���� ����� � ������� ����� */
typedef struct {
    uint32_t test_code;
    int (*start)(void);
    void (*get_result)(t_l502_bf_cmd *cmd);
} t_test_pars;
/* ������� ������������ ������� � ����� ������ */
static t_test_pars f_test_pars[] =  {
    { L502_BF_CMD_TEST_ECHO,  NULL,         NULL},
    { L502_BF_CMD_TEST_SPORT, f_sport_test, NULL},
    { L502_BF_CMD_TEST_SDRAM, f_sdram_test, NULL},
    { L502_BF_CMD_TEST_SPI,   f_spi_test,   NULL}
};





void l502_cmd_test(t_l502_bf_cmd *cmd) {
    /* ��������� ���������� ����� */
    if (cmd->param == L502_BF_CMD_TEST_GET_RESULT) {
        /* ���� �� ���� �������� ����� - ���������� ������ */
        if (f_cur_test_ind == -1) {
            l502_cmd_done(L502_BF_ERR_INVALID_CMD_PARAMS, NULL, 0);
        } else {
            f_test_res.run = (g_mode == L502_BF_MODE_TEST) ? 1 : 0;


            if (f_test_pars[f_cur_test_ind].get_result != NULL) {
                /* ���� ���� � ����� ���� ������� ��� ��������� ���������� =>
                   �������� �� */
                f_test_pars[f_cur_test_ind].get_result(cmd);
            } else {
                /* ����� ������ ������������� ���������� ��� ������ */
                l502_cmd_done(0, (uint32_t*)&f_test_res,
                                sizeof(t_l502_bf_test_res)/sizeof(uint32_t));
            }
        }
    } else if (cmd->param == L502_BF_CMD_TEST_STOP) {
        /* ������� ����� => ���� ���� ������� - ���������� ��� ���������,
           ����� ���������� ������ ��� � ��� ����� ���� */
        if (g_mode == L502_BF_MODE_TEST) {
            g_mode = L502_BF_MODE_IDLE;
            l502_cmd_done(0, NULL, 0);
        } else {
            l502_cmd_done(L502_BF_ERR_NO_TEST_IN_PROGR, NULL, 0);
        }
    } else {
        /* ������ ����� - �������� �� ������� � ���� ������ ���� */
        if (g_mode == L502_BF_MODE_IDLE) {
            uint32_t i;

            for (i=0, f_cur_test_ind=-1; (i<sizeof(f_test_pars)/sizeof(f_test_pars[0])) &&
                      (f_cur_test_ind==-1); i++) {
                if (f_test_pars[i].test_code == cmd->param) {
                    f_cur_test_ind = i;
                    memset(&f_test_res, 0, sizeof(f_test_res));
                    f_test_res.test = cmd->param;
                    l502_cmd_done(0, 0, NULL);

                    if (f_test_pars[i].start != NULL) {
                        g_mode = L502_BF_MODE_TEST;
                        f_test_pars[i].start();
                        g_mode = L502_BF_MODE_IDLE;
                    }
                }
            }

            if (f_cur_test_ind == -1)
                l502_cmd_done(L502_BF_ERR_INVALID_CMD_PARAMS, NULL, 0);
        }
        else
        {
            l502_cmd_done(L502_BF_ERR_STREAM_RUNNING, NULL, 0);
        }
    }
}


#define SPORT_CNTR_INC 0x1


#define TEST_CHECK_OUT(label) \
    do { \
        l502_cmd_check_req(); \
        if ((g_mode!= L502_BF_MODE_TEST) || f_test_res.err) \
            goto label; \
    } while (0);


/* ������������ SPORT'�, ��������� ��� �������� ������� ���/��� 
   �� BlackFin'� � ����. ��������������� ����������� �������� ����� ������
   � ���������� �������, ������� ����������� ��� ������ */
static int f_sport_test(void) {
    #define RX_BUF_SIZE 2048
    static uint16_t rx_buf[RX_BUF_SIZE];
    uint16_t stat=*pSPORT0_STAT;


    uint16_t rx_cntr=0, tx_cntr=0;
    uint16_t rx_val;
    int err = 0, i;

    /* ��������� ����� � �������� �� SPORT'� */
    *pSPORT0_TCR1 |= TSPEN;
    *pSPORT0_RCR1 |= RSPEN;

    while (stat & RXNE) {
        rx_buf[0] =  *pSPORT0_RX16;
        stat=*pSPORT0_STAT;
    }

    /* ������� ��������� ��������� ������� SPORT'� */
    for (i = 0; i < 8; i++) {
        *pSPORT0_TX16 = tx_cntr++;
        //tx_cntr+=SPORT_CNTR_INC;
    }

    /* ������������� � ���� �������� ����� ������ SPORT */
    fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, 0xE);


    while (!f_test_res.err) {// && (g_mode==L502_BF_MODE_TEST))
        stat = *pSPORT0_STAT;
        if (!(stat & TXF)) {
            *pSPORT0_TX16 = tx_cntr++;
        }

        if (stat & RXNE) {
            rx_val = *pSPORT0_RX16;

            if ((rx_val != rx_cntr)) {
                f_test_res.err = L502_BF_ERR_TEST_VALUE;
                f_test_res.last_rd = rx_val;
                f_test_res.last_wr = rx_cntr;
                err = rx_val - rx_cntr;
            }


            if (!(rx_cntr & 0xFFFF)) {
                TEST_CHECK_OUT(sport_test_end);
                if (!rx_cntr)
                    f_test_res.cntr++;
            }
            rx_cntr++;
        }
    }

sport_test_end:
    /* ��������� ����� � �������� �� SPORT'� */
    *pSPORT0_TCR1 = 0;
    *pSPORT0_RCR1 = 0;
    /* ���������� ������� ����� ������ SPORT'� */
    fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, 0);

    return err;
}





/* ���� SPI - ���������� ������� ������� � ������� ����, ���������
   �������� ����� �������� � ������� ���������� */
static int f_spi_test(void) {
    f_test_res.last_addr = L502_REGS_IOHARD_ADC_FRAME_DELAY;
    while (!f_test_res.err && (g_mode==L502_BF_MODE_TEST)) {
        int i;
        for (i = 0; (i < 32) &&  !f_test_res.err; i++) {
            f_test_res.last_wr = (1<<i);
            fpga_reg_write(f_test_res.last_addr, f_test_res.last_wr);
            f_test_res.last_rd = fpga_reg_read(f_test_res.last_addr);

            if (f_test_res.last_wr != f_test_res.last_rd)
                f_test_res.err = L502_BF_ERR_TEST_VALUE;
        }

        if (!f_test_res.err)
            f_test_res.cntr++;

        l502_cmd_check_req();
    }
    return f_test_res.err;
}





/* ���� SDRAM ������ */
static int f_sdram_test(void) {
    int i;
    unsigned short cntr = 0;



    while (!f_test_res.err && (g_mode == L502_BF_MODE_TEST)) {
        f_test_res.stage = 0;

        /* ��������� ��� ������ ��������� */
        for (i = 0, cntr = 0; i < (16UL*1024*1024); i++)  {
            bank1[i] = cntr;
            if (++cntr == L502_TEST_CNTR_MODULE) {
                cntr = 0;
                TEST_CHECK_OUT(sdram_test_end);
            }
        }

        TEST_CHECK_OUT(sdram_test_end);
    
        
         /* ������ ��������������� � ������� ��������� */
        for (i = 0, cntr = 0; i < (16UL*1024*1024); i++) {
            uint16_t word = bank1[i];
            if (word != cntr) {
                f_test_res.err = L502_BF_ERR_TEST_VALUE;
                f_test_res.last_addr = (uint32_t)&bank1[i];
                f_test_res.last_wr = cntr;
                f_test_res.last_rd = word;
                break;
            }

            if (++cntr==L502_TEST_CNTR_MODULE) {
                cntr = 0;
                TEST_CHECK_OUT(sdram_test_end);
            }
        }

        TEST_CHECK_OUT(sdram_test_end);
        f_test_res.stage++;
    
    
        /* ���������� �� ����� � ������ ���� ��� �������� ������������
            ������ �� ������ ������ */
        for (i = 0, cntr = 0; i < (4UL*1024*1024); i++) {
            bank1[i] = cntr;
            bank2[i] = ~cntr;
            bank3[i] = cntr^0xAA55;
            bank4[i] = cntr^0x55AA;
            if (++cntr == L502_TEST_CNTR_MODULE) {
                cntr = 0;
                TEST_CHECK_OUT(sdram_test_end);
            }
        }

        TEST_CHECK_OUT(sdram_test_end);
    
 
        for (i = 0, cntr = 0; i < (4UL*1024*1024); i++) {
            uint16_t word = bank1[i];
            if (word != cntr) {
                f_test_res.err = L502_BF_ERR_TEST_VALUE;
                f_test_res.last_addr = (uint32_t)&bank1[i];
                f_test_res.last_wr = cntr;
                f_test_res.last_rd = word;
                break;
            }

            word = bank2[i];
            if (word != (~cntr & 0xFFFF)) {
                f_test_res.err = L502_BF_ERR_TEST_VALUE;
                f_test_res.last_addr = (uint32_t)&bank2[i];
                f_test_res.last_wr = (~cntr & 0xFFFF);
                f_test_res.last_rd = word;
                break;
            }
            word = bank3[i];
            if (word != ((cntr^0xAA55) & 0xFFFF)) {
                f_test_res.err = L502_BF_ERR_TEST_VALUE;
                f_test_res.last_addr = (uint32_t)&bank3[i];
                f_test_res.last_wr = ((cntr^0xAA55) & 0xFFFF);
                f_test_res.last_rd = word;
                break;
            }

            word = bank4[i];
            if (word != ((cntr^0x55AA) & 0xFFFF)) {
                f_test_res.err = L502_BF_ERR_TEST_VALUE;
                f_test_res.last_addr = (uint32_t)&bank4[i];
                f_test_res.last_wr = ((cntr^0x55AA) & 0xFFFF);
                f_test_res.last_rd = word;
                break;
            }

            if (++cntr == L502_TEST_CNTR_MODULE) {
                cntr = 0;
                TEST_CHECK_OUT(sdram_test_end);
            }
        }


        TEST_CHECK_OUT(sdram_test_end);

        /* ������ � 4 ������ ����� � ����������� ������� */
        f_test_res.stage++;
    
        for (i = 0, cntr=0; i < (4UL*1024*1024); i++) {
            uint16_t word[4], wr_val[4] = {~cntr, cntr, cntr^0x55AA, cntr^0xAA55};

            bank1[i] = wr_val[0];
            bank2[i] = wr_val[1];
            bank3[i] = wr_val[2];
            bank4[i] = wr_val[3];

            word[1] = bank2[i];
            word[0] = bank1[i];
            word[3] = bank4[i];
            word[2] = bank3[i];


            if (word[0] != wr_val[0]) {
                f_test_res.err = L502_BF_ERR_TEST_VALUE;
                f_test_res.last_addr = (uint32_t)&bank1[i];
                f_test_res.last_wr = wr_val[0];
                f_test_res.last_rd = word[0];
                break;
            }
            if (word[1] != wr_val[1])  {
                f_test_res.err = L502_BF_ERR_TEST_VALUE;
                f_test_res.last_addr = (uint32_t)&bank2[i];
                f_test_res.last_wr = wr_val[1];
                f_test_res.last_rd = word[1];
                break;
            }
            if (word[2] != wr_val[2]) {
                f_test_res.err = L502_BF_ERR_TEST_VALUE;
                f_test_res.last_addr = (uint32_t)&bank3[i];
                f_test_res.last_wr = wr_val[2];
                f_test_res.last_rd = word[2];
                break;
            }
            if (word[3] != wr_val[3]) {
                f_test_res.err = L502_BF_ERR_TEST_VALUE;
                f_test_res.last_addr = (uint32_t)&bank4[i];
                f_test_res.last_wr = wr_val[3];
                f_test_res.last_rd = word[3];
                break;
            }

            if (++cntr == L502_TEST_CNTR_MODULE) {
                cntr = 0;
                TEST_CHECK_OUT(sdram_test_end);
            }
        }
        TEST_CHECK_OUT(sdram_test_end);

        f_test_res.cntr++;
    }
sdram_test_end:
    
    return f_test_res.err;
}

