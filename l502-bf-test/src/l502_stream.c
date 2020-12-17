/** 
    @addtogroup streams
    @{

    @file l502_stream.c
    ���� �������� ������� ��������� ������� �� ���/DIN � �� � �� �� � ���/DOUT.
    ����� � ���� ���� ������� ���������� ������� ������ �� SPORT0, ��� ��� ���
    ������� � ������� ���������� �������.
 */   


#include <stdint.h>
#include <stdlib.h>

#include <cdefBF523.h>
#include <sys/exception.h>
#include <ccblkfn.h>

#include "l502_cdefs.h"
#include "l502_cmd.h"
#include "l502_fpga.h"
#include "l502_defs.h"
#include "l502_global.h"
#include "l502_hdma.h"
#include "l502_sport_tx.h"
#include "l502_user_process.h"
#include "l502_stream.h"
#include "l502_sport_rx.h"


/** ������ ������ �� ����� ������ �� SPORT0 � 32-������ ������ */
#define L502_SPORT_IN_BUF_SIZE           (2048*1024)
/** ������ ������ ��� ������ ������ �� HostDMA �� ����� � 32-������ ������ */
#define L502_HDMA_OUT_BUF_SIZE           (1024*1024)

/** ��� ���������� ��� ������ ������ �� SPORT0 ��-��������� */
#define L502_DEFAULT_SPORT_RX_BLOCK_SIZE    (32*1024)




t_in_stream_state  g_stream_in_state = IN_STREAM_STOP;
t_out_stream_state g_stream_out_state = OUT_STREAM_STOP;
/** ����� ������ - ����������, ������� �� ���������� ���� ��� ���� */
volatile int g_mode = L502_BF_MODE_IDLE;

int g_streams = L502_STREAM_ADC;
static int f_bf_reg = 0;

/* �����, ������������, ��� ��������� ������������ */
static const uint32_t f_overflow_wrd = L502_STREAM_IN_MSG_OVERFLOW;



/* ����� ��� ������ digin � ������ ��� �� SPORT (� ������������������ �������) */
#include "l502_sdram_noinit.h"
static volatile uint32_t f_sport_in_buf[L502_SPORT_IN_BUF_SIZE];
/* ������� � ������ � ������� ����� ������� ��������� �������� ������ */
static volatile uint32_t f_sport_in_put_pos = 0;
/* ������� � ������ �� ��������� ������������ �������� */
static uint32_t f_sport_in_proc_pos = 0;
/* ������� � ������ �� ��������� ������ �� ������ �������� (������� ������� �������� �������) */
static volatile uint32_t f_sport_in_get_pos = 0;
/* ��� ���������� �� ����� �� SPORT0 */
static uint32_t f_sport_in_block_size = L502_DEFAULT_SPORT_RX_BLOCK_SIZE;
/* ������� ������������ ������ �������� ������ �� ����� */
static uint32_t f_sport_in_buf_size = L502_SPORT_IN_BUF_SIZE;


/* ����� ��� ��� ������� ������ �� HDMA ��� ������ �� ��� � DIGOUT */
#include "l502_sdram_noinit.h"
static volatile uint32_t f_hdma_out_buf[L502_HDMA_OUT_BUF_SIZE];
static volatile uint32_t f_hdma_out_put_pos = 0; /* ��������� �� ������� �� ��������� �������� �� HDMA */
static uint32_t f_hdma_out_start_pos; /* ��������� �� ������� �� ��������� ������������ �� ����� ������
                                    (� ��� ����� ���������� ��������� ������) */
static volatile uint32_t f_hdma_out_get_pos = 0; /* ��������� �� ������� �� ��������� ������������ ������
                                     (� ��� ����� ����� ��������� ����� ��� ���������, ����� ����� ������ */
static int f_hdma_out_block_size = 0x8000;
static int f_hdma_out_proc_pos;

static uint32_t f_recv_size = 0;







/* ������� ���������� ��� ������������� ������������ ������ �� �����
   �� SPORT'� ������ ���/DIGIN */
static void f_stream_in_set_overflow(void) {
    /* ������������� ����� ������ �� ��� */
    sport_rx_stop();
    /* ������������� ����, ��� ����� �������� ��������� �
       ������������ */
    g_stream_in_state = IN_STREAM_OV_ALERT;
}





/** @brief ��������� ������������� ���������� ��� ���������� ������� */
void l502_stream_init(void) {
    /* ������������� ���� ������, ���� �� ��� ������� */
    fpga_reg_write(L502_REGS_IOHARD_GO_SYNC_IO, 0);
    fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, 0);
    /* ��������� DMA �� �������� �� SPORT */
    sport_tx_init();
}


/** @brief ������ ������������ ������ �� ������

   ������ ������� ��������� ������������ ������ ������ �� �����.
   ������������, ����� ��������� ������ � ����� BlackFin �� ������� �����������
   �����-������.
   @return ��� ������ */
int32_t stream_out_preload(void) {
    int32_t err = g_stream_out_state==OUT_STREAM_RUN ? L502_BF_ERR_STREAM_RUNNING :
                  0;
    if (!err) {
        f_hdma_out_get_pos = f_hdma_out_start_pos =
            f_hdma_out_put_pos = f_hdma_out_proc_pos = 0;

        /* ��������� ����� �� HDMA */
        hdma_recv_start();
        /* ��������� ������ ���� �� ����� (��������� �����
           ��������� �� stream_proc() */
        f_hdma_out_start_pos+=f_hdma_out_block_size;
        hdma_recv_req_start((uint32_t*)f_hdma_out_buf, f_hdma_out_block_size);

        g_stream_out_state = OUT_STREAM_PRELOAD;
    }
    return err;
}


static void f_set_streams(uint32_t streams) {
    uint32_t wrd_en = 0;

    /* ���� ��� ������� ��������� ����� � ����������� ���� ��
       ������� �� ����, �� �������������� ����� �� SPORT � �������� �� hdma */
    if (g_mode == L502_BF_MODE_STREAM) {
        if ((streams & L502_STREAM_ALL_IN) && !(g_streams & L502_STREAM_ALL_IN)) {
            sport_rx_start();
        }

        if (!(streams & L502_STREAM_ALL_IN) && (g_streams & L502_STREAM_ALL_IN)) {
            sport_rx_stop();
        }
    }

    /* �������� ����������� ������ � �������� FPGA */
    if (streams & L502_STREAM_ADC)
        wrd_en |= 0x1;
    if (streams & L502_STREAM_DIN)
        wrd_en |= 0x2;
    fpga_reg_write(L502_REGS_IOARITH_IN_STREAM_ENABLE, wrd_en);


    /** @todo: ���������� �� ���� �������� ������� */

    g_streams = streams;
}

/** @brief ���������� ��������� ���������� �������
    @param[in] streams   ������� ����� �� #t_l502_streams, ����������� ����� ������
                         ������ ���� ��������� (� ���������� � ��� �����������)
    @return ��� ������ */
int32_t stream_enable(uint32_t streams) {
    f_set_streams(g_streams | streams);
    return 0;
}


/** @brief ���������� ��������� ���������� �������
    @param[in] streams   ������� ����� �� #t_l502_streams, ����������� ����� ������
                         ������ ���� ���������
    @return ��� ������ */
int32_t stream_disable(uint32_t streams) {
    f_set_streams(g_streams & ~streams);
    return 0;
}

/***************************************************************************//**
    @brief ������ ����������� �����-������

    ������� ��������� ���������� ����-����� �����.
    ��� ���� ���������� �������� �� ���� ����� ����������� ������� � �������
    stream_enable().
    ����� ������ ���� ������� �������� ��������� ������ ��� �������, ������
    ����� ������������� ��������� ��� ��������� ������ ����� stream_enable()
    ��� stream_disable().

    @return ��� ������.
    ***************************************************************************/
int32_t streams_start(void) {
    int32_t err = g_mode != L502_BF_MODE_IDLE ? L502_BF_ERR_STREAM_RUNNING : 0;
    if (!err) {
        /* ����� �� SPORT ���������������� ������ � ������� �������
           ����������� ����� */
        f_sport_in_put_pos = f_sport_in_get_pos = f_sport_in_proc_pos = 0;


        if (g_streams & L502_STREAM_ALL_IN) {
            sport_rx_start();
            g_stream_in_state = IN_STREAM_RUN;
        }

        hdma_send_start();

        f_set_streams(g_streams);

        if (g_streams & L502_STREAM_ALL_OUT) {
            /* ��������� ���� ��������� TFS �� SPORT */
            f_bf_reg |= L502_REGBIT_IOHARD_OUT_TFS_EN_Msk;
            fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, f_bf_reg);


            /* ���� �� ���� ������������ �� �������,
               �� ��������� ����� ������ */
            if ((g_stream_out_state == OUT_STREAM_STOP) ||
                (g_stream_out_state == OUT_STREAM_ERR)) {
                stream_out_preload();
            }

            if (g_stream_out_state == OUT_STREAM_PRELOAD) {
                /* ��������� ������������ ������ */
                fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, f_bf_reg | 1);
                g_stream_out_state = OUT_STREAM_RUN;
            }
        }



        /* ��� ��� �������� �������� ���������� ������� ���������� ��� �������
            �� 2-� ������, ��� ����������� ����������� ������ ���������� �
            �������� ��� ���� ������������. � ��������� ������,
            ����� ������� ������� ������� ����� �� ��������� � ��������
            ������� �������������
         */
        fpga_reg_write(L502_REGS_IOHARD_PRELOAD_ADC, 1);
        fpga_reg_write(L502_REGS_IOHARD_PRELOAD_ADC, 1);


        fpga_reg_write(L502_REGS_IOHARD_GO_SYNC_IO, 1);

        g_mode = L502_BF_MODE_STREAM;
    }
    return err;
}




/** @brief ������� ���������� ������� �����-������.

    �� ���� ������� ��������������� ��� ���������� ������.
    ����������� �������� ������� �� SPORT � �� HostDMA

    @return ��� ������ */
int32_t streams_stop(void) {
    int32_t err = g_mode != L502_BF_MODE_STREAM ? L502_BF_ERR_STREAM_STOPPED : 0;
    if (!err) {
        hdma_send_stop();
        hdma_recv_stop();
        fpga_reg_write(L502_REGS_IOHARD_GO_SYNC_IO, 0);

        /* ��������� ����� � �������� �� SPORT'� */
        sport_rx_stop();
        sport_tx_stop();

        /* ��������� ��������� TFS � RFS */
        f_bf_reg = 0;
        fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, f_bf_reg);


        g_mode = L502_BF_MODE_IDLE;
        g_stream_in_state = IN_STREAM_STOP;
        g_stream_out_state = OUT_STREAM_STOP;
    }
    return err;
}



/** @brief ������� ��������� ������� �����-������

   ������� ������������ ���������� �� ��������� ����� ���������.
   ��� ������� ������, �����������, ����
   �� �������������� ������ ��������� �� ���/DIGIN �/��� ��������� �� �� ������
   �� ��� ��� DOUT. ��� �� ������� ���������� ��������������� ������� ����������������
   ��������� ������.
   �����, ���� ���� ������������ � ��� ������ �� ������������ ���� ����������,
   �� � �� ���������� ����� � ���, ��� � ���� ����� ��������� ������������ */
void stream_proc(void) {
    //���� ������� ����� ����� 
    if ((g_stream_in_state != IN_STREAM_STOP) && (g_stream_in_state != IN_STREAM_ERR)) {
		
        uint32_t sport_rdy_size;
        uint32_t put_pos = f_sport_in_put_pos;

        //�������, ������� ������� �������������� ������ �� sport'� 
        sport_rdy_size = put_pos  >= f_sport_in_proc_pos ?
             put_pos - f_sport_in_proc_pos : f_sport_in_buf_size - f_sport_in_proc_pos;
         //���� ���� �������������� ������ - �������� ������� ��� ��������� 
         if (sport_rdy_size) {
             uint32_t processed = usr_in_proc_data(
                         (uint32_t*)&f_sport_in_buf[f_sport_in_proc_pos],
                         sport_rdy_size);
            //��������� ������� ������������ ������ 
             f_sport_in_proc_pos += processed;
             if (f_sport_in_proc_pos==f_sport_in_buf_size)
                 f_sport_in_proc_pos = 0;
         }
	
         //���� ���� ������������ - ����� �������� ����� � �������������.
         //   �������� ��� ����� ����, ��� ��������� ��� ����� �� ������������ 
        if ((g_stream_in_state == IN_STREAM_OV_ALERT) && !sport_rdy_size && hdma_send_req_rdy()) {
            hdma_send_req_start(&f_overflow_wrd, 1, 1);
            g_stream_in_state = IN_STREAM_ERR;
        }
    }

    //���� ���� ����� �� ����� 
    if ((g_stream_out_state == OUT_STREAM_PRELOAD) ||
        (g_stream_out_state == OUT_STREAM_RUN)) {
		
        uint32_t hdma_rdy_size;
        uint32_t put_pos = f_hdma_out_put_pos;

        //���������, ������� ���� �������������� ������, �������� �� HDMA 
        hdma_rdy_size = put_pos >= f_hdma_out_proc_pos ?
             put_pos - f_hdma_out_proc_pos : L502_HDMA_OUT_BUF_SIZE - f_hdma_out_proc_pos;

         //���� ����� ���� -> ������� ���������� 
         if (hdma_rdy_size != 0) {
             uint32_t processed = usr_out_proc_data(
                             (uint32_t*)&f_hdma_out_buf[f_hdma_out_proc_pos],
                              hdma_rdy_size);

             //��������� ������� ������������ ������ 
            f_hdma_out_proc_pos += processed;
             if (f_hdma_out_proc_pos==L502_HDMA_OUT_BUF_SIZE)
                 f_hdma_out_proc_pos = 0;
         }


         //���� ���� ��������� ����� � ������ �� ����� � ���� ���������
         //   ����������� => ������ ����� ������ �� ����� ����� 
         if (hdma_recv_req_rdy()) {
			 
             uint32_t get_pos = f_hdma_out_get_pos;

             hdma_rdy_size = f_hdma_out_start_pos >= get_pos ?
                                 L502_HDMA_OUT_BUF_SIZE - f_hdma_out_start_pos + get_pos :
                                 get_pos -  f_hdma_out_start_pos;
             if (hdma_rdy_size > f_hdma_out_block_size) {
                 hdma_recv_req_start((uint32_t*)&f_hdma_out_buf[f_hdma_out_start_pos],
                                     f_hdma_out_block_size);

                 f_hdma_out_start_pos+=f_hdma_out_block_size;
                 if (f_hdma_out_start_pos==L502_HDMA_OUT_BUF_SIZE)
                     f_hdma_out_start_pos = 0;
             }
         }
    }
}

/** @brief ������������ size ���� �� ������ ������ �� SPORT0

   ������� ��������, ��� size ���� �� ������ ��� ����� ������ SPORT0, � �������
   ���� ������� ������, �� �� �����������, ��� �������������. �.�. � ��� �������
   ����� ����� ����� ��������� ������ �� SPORT0.
   ��� ���� ���� ������ �������, ����� ���������� ������������� ������ ��
   ��������� ���������� ������������!

   @param[in] size ������ ������������� ������ � 32-������ ������ */
void stream_in_buf_free(uint32_t size) {
    /* ��������� ������� ����������� ����� */
    uint32_t get_pos = f_sport_in_get_pos;
    get_pos += size;
    if (get_pos >= f_sport_in_buf_size)
        get_pos-= f_sport_in_buf_size;
    f_sport_in_get_pos = get_pos;
}

/** @brief ������������ size ���� �� ������ ������ �� HostDMA

   ������� ��������, ��� size ���� �� ������ ��� ����� ������ HostDMA, � �������
   ���� ������� ������ �� ��, �� �� �����������, ��� �������������. �.�. � ��� �������
   ����� ����� ����� ��������� ������ �� �� �� HostDMA.
   ��� ���� ���� ������ �������, ����� ���������� ������������� ������ ��
   ��������� ���������� ������������!

   @param[in] size ������ ������������� ������ � 32-������ ������ */
void stream_out_buf_free(uint32_t size) {
    uint32_t get_pos = f_hdma_out_get_pos;
    get_pos += size;
    if (get_pos >= L502_HDMA_OUT_BUF_SIZE)
        get_pos -= L502_HDMA_OUT_BUF_SIZE;
    f_hdma_out_get_pos = get_pos;
}

/** @brief ��������� ���������� ������ �� HostDMA

    ������� ���������� �� ����������� ����������, ����� ���������� �����
    ����� ������ �� HDMA � ��, ������������� �� ����� �� �������� �
    ������� hdma_recv_req_start().
    ������� ������ ��������� ������� �������� ������ (� ��������� ����� ���
    �� ������� ������� stream_proc().

    @param[in] addr    ����� �����, ����� �� ��������� �������� ������.
    @param[in] size     ���������� �������� 32-������ ���� */
void hdma_recv_done(uint32_t* addr, uint32_t size) {
    /* ��������� ������� ��������� ����� */
    uint32_t put_pos = f_hdma_out_put_pos;
    put_pos += size;
    if (put_pos == L502_HDMA_OUT_BUF_SIZE)
        put_pos = 0;
    f_hdma_out_put_pos = put_pos;
}





/** @brief ������ ������ �� �����.

    ������� ���������� ������ ������ �� ����� �� SPORT0
    @return ������ ������ �� ����� � 32-������ ������ */
extern uint32_t sport_in_buffer_size(void) {
    return L502_SPORT_IN_BUF_SIZE;
}

/** @} */


/** @addtogroup sport_rx
    @{  */

/** @brief ��������� ���� ���������� ��� ����� �� SPORT0

    ������� ������������� ��� ���������� ��� DMA, ��������������� ���
    ������ ������ ����������� �����.
    ��� ���� ������ ���� ������ ���� ��� ������� � 4 ���� ������ �������
    ������ #L502_SPORT_IN_BUF_SIZE.
    ����� ��������� ���� ������������ ������� ������������ ������ ������,
    ��� ���������� ����� ������� ���� � �� �����������  #L502_SPORT_IN_BUF_SIZE.

    @param[in] size   ������ ���� ���������� � 32-������ ������
    @return           ��� ������ */
int32_t sport_in_set_step_size(uint32_t size) {
    int mul;
    if ((size >= L502_SPORT_IN_BUF_SIZE/4) || (size > 0x8000))
        return L502_BF_ERR_INVALID_CMD_PARAMS;
    mul = L502_SPORT_IN_BUF_SIZE/size;
    /* �� ����� ���� ����� 0x8000 �����, ��� ��� �������
       YCNT 16-��������� (+ ��� �������� �� 2) */
    if (mul > 0x8000)
        mul = 0x8000;
    f_sport_in_buf_size = size*mul;
    f_sport_in_block_size = size;
    return 0;
}


/** @brief ������ ����� ������ �� SPORT0

    ������� ����������� DMA3 �� ����� ���������� � 2D, ������ ���� ����������
    ������ f_sport_in_block_size. ����� ���� ����������� ����� DMA �
    ����� �� SPORT0 */
void sport_rx_start(void) {
    static volatile int dummy;
    /* ��������� DMA */
    *pDMA3_CONFIG = 0;
    ssync();
    /* ���������� ��� ������ �� ������, ���� ��� ���� */
    while (*pSPORT0_STAT & RXNE) {
        dummy = *pSPORT0_RX16;
        ssync();
    }

    /* ����������� DMA */
    *pDMA3_START_ADDR = (void*)f_sport_in_buf;
    *pDMA3_X_COUNT = 2*f_sport_in_block_size; /* ��� ��� SPORT �������� �� 16 ��� (���� � 2 ������),
                                                  � ������ � 32 ������ ������ => �������� �� 2) */
    *pDMA3_X_MODIFY = 2;
    *pDMA3_Y_COUNT = f_sport_in_buf_size/f_sport_in_block_size;;
    *pDMA3_Y_MODIFY = 2;
    *pDMA3_CURR_ADDR = (void*)f_sport_in_buf;
    *pDMA3_CONFIG = FLOW_AUTO | DI_EN | DI_SEL | SYNC | DMA2D | WNR | WDSIZE_16; 
    
    
    ssync();
    /* ��������� DMA */
    *pSIC_IMASK0 |= IRQ_DMA3;
    *pDMA3_CONFIG |= DMAEN;
    /* ��������� ����� �� SPORT'� */
    *pSPORT0_RCR1 |= RSPEN;

    /* ��������� ��������� RFS �� SPORT0 */
    f_bf_reg |= L502_REGBIT_IOHARD_OUT_RFS_EN_Msk;
    fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, f_bf_reg);
}

 

/** @brief ������� ����� ������ �� SPORT0

    ������� ��������� ����� �� SPORT0 � ������������� DMA */
void sport_rx_stop(void) {
    /* ������������� ��������� RFS */
    f_bf_reg &= ~L502_REGBIT_IOHARD_OUT_RFS_EN_Msk;
    fpga_reg_write(L502_REGS_IOHARD_OUTSWAP_BFCTL, f_bf_reg);

    ssync();
    ssync();

    /* ��������� ����� �� SPORT */
    *pSPORT0_RCR1 &= ~RSPEN;
    /* ��������� DMA */
    *pDMA3_CONFIG =0; //&= ~DMAEN;
    *pSIC_IMASK0 &= ~IRQ_DMA3;
    //ssync();
}

/** @brief ���������� ���������� �� SPORT0 �� �����.

    ���������� ���������, ����� ��� ������ ���� ������ �� SPORT0.
    ��������� ��������� �������� ������ � ��������� ������������ */
ISR(isr_sport_dma_rx) {
    if (*pDMA3_IRQ_STATUS & DMA_DONE) {
        uint32_t rdy_put_pos;
        /* ���������� ���������� �� DMA */
        *pDMA3_IRQ_STATUS = DMA_DONE;

        /* ��������� ���������� �������� ������ �� ������ ����� */
        f_sport_in_put_pos +=  f_sport_in_block_size;
        if (f_sport_in_put_pos == f_sport_in_buf_size)
            f_sport_in_put_pos = 0;

        /* �������, ������� �������� ����� � ������ �� ����� */
        uint32_t get_pos = f_sport_in_get_pos;
        rdy_put_pos = f_sport_in_put_pos > get_pos ?
                               f_sport_in_buf_size -f_sport_in_put_pos + get_pos :
                               get_pos - f_sport_in_put_pos;

        f_recv_size += f_sport_in_block_size;

        /* ���� �������� �� ������ ����� - �� ������� �� ������������,
            �.�. ����� ��� ��������� ���������� ��� ����� ���� ���������
            �������� ����� ������ */
        if (rdy_put_pos < 2*f_sport_in_block_size) {
            f_stream_in_set_overflow();
        }
    }
}

/** @} */









