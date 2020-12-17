/***************************************************************************//**
    @addtogroup hdma
    @{
    @file l502_hdma.c
    ���� �������� ������ ������ � hdma �� ����� � �� �������� ������
    ��� ��, ��� � � BlackFin.    
 ***************************************************************************/

/******************************************************************************
 ��� ������� ������ ���������� ���� ��� hdma_stream_init().
����� hdma_xxx_start() �������������� ����� ��� ��������, � hdma_xxx_stop()
������������� ��� ������� ��������.

��� ������� ������ ������ ������ ����� ��������� ����������
(������� ��������� ������������) � ������� hdma_xxx_req_rdy()
� ������� hdma_xxx_req_start(), ������ ������ ��� �������� ��� ������
������ � ��� ������.

����� ������������� �� 31 ������� � �������.

�� ���������� ������ ����� ������� ������� hdma_xxx_done(), �������
������ ���� ����������� � ������ �����. ��� �������� BF->PC
������� ���������� ���� ��� �� ������ ������������ ������,
� ��� ������ PC->BF ����� ���������� �� ��������� ��� - ��� ������
����� ������� (� ������, ���� �� PC ���� ��� ������ ������)
*******************************************************************************/


#include "l502_global.h"
#include "l502_cdefs.h"
#include "l502_cmd.h"


#include <cdefBF523.h>
#include <sys/exception.h>
#include <ccblkfn.h>


#define STREAM_IN_EN()  *pPORTFIO_CLEAR = PF14
#define STREAM_IN_DIS() *pPORTFIO_SET = PF14
#define STREAM_OUT_EN()  *pPORTFIO_CLEAR = PF15
#define STREAM_OUT_DIS() *pPORTFIO_SET = PF15

#define STREAM_IN_SET_REQ() *pPORTGIO_TOGGLE = PG5
#define STREAM_OUT_SET_REQ() *pPORTGIO_TOGGLE = PG6



#define L502_HDMA_STEP_SIZE_MAX        256


static uint16_t f_snd_start_id, f_rcv_start_id;
static uint16_t f_snd_done_id, f_rcv_done_id;
static uint8_t f_snd_next_descr, f_rcv_next_descr;
static uint8_t f_rcv_done_descr;


void hdma_send_done(uint32_t* addr, uint32_t size);
void hdma_recv_done(uint32_t* addr, uint32_t size);


ISR(hdma_isr);
ISR(hdma_rd_isr);




inline static void hdma_set_descr(t_hdma_stream_descr* descr_arr, uint8_t* pos,
                                  uint16_t* id, uint32_t* addr, uint32_t size, uint32_t flags) {
    descr_arr[*pos].flags = flags;
    descr_arr[*pos].addr =  addr;
    descr_arr[*pos].id = *id;
    descr_arr[*pos].full_size = size*2;
    descr_arr[*pos].xcnt = size > L502_HDMA_STEP_SIZE_MAX ? 2*L502_HDMA_STEP_SIZE_MAX : 2*size;
    descr_arr[*pos].udata = size;
    *id = *id+1;

    *pos=*pos+1;
    if (*pos==    L502_IN_HDMA_DESCR_CNT)
        *pos = 0;
}


/** @brief ������������� ���������� HostDMA

    ��������� ���������� HostDMA � ������������� ������������ ����� ������������
    ��� ����������� ������ �� ������ �� HostDMA */
void hdma_init(void) {
    int d;

    /* ��������� ���� ����������� ������� �� ����� �� HDMA */
    STREAM_IN_DIS();
    STREAM_OUT_DIS();

    *pPORTGIO_DIR |= PG5 | PG6;
    *pPORTFIO_DIR |= PF14 | PF15;

    /*****************         ��������� HOST DMA         ***********************/
    //��������� ������
    *pPORTG_MUX |= 0x2800;
    *pPORTG_FER |= 0xF800;
    *pPORTH_MUX = 0x2A;
    *pPORTH_FER = 0xFFFF;


    //��������� ����������
    REGISTER_ISR(11, hdma_isr);
    //*pSIC_IAR3 = (*pSIC_IAR6 & 0xFFF0FFFFUL) | (3 << 16);
    *pSIC_IAR6 = (*pSIC_IAR6 & 0xFFFFF0FFUL) | P50_IVG(10); //���������� HDMARD �� IVG10
    REGISTER_ISR(10, hdma_rd_isr);
    *pSIC_IMASK0 |= IRQ_DMA1; //���������� ���������� HOSTDP �� ������
    *pSIC_IMASK1 |= IRQ_HOSTRD_DONE;   //���������� ���������� HOSTDP �� ������;
    //���������� HDMA
    *pHOST_CONTROL =  BDR | EHR | EHW | HOSTDP_EN | HOSTDP_DATA_SIZE; //burst, ehr, ehw, en

    //g_state.cmd.data[100] = L502_BF_CMD_STATUS_DONE;

    /* ������������ ����� ������������, ������� �� ����� ����������
       �� ����� ������ */
    for (d=0; d < L502_IN_HDMA_DESCR_CNT; d++) {
        g_state.hdma.in[d].xmod = 2;
        g_state.hdma.in[d].valid = 1;
        g_state.hdma.in[d].next_descr = d==(    L502_IN_HDMA_DESCR_CNT-1) ?
                    (void*)&g_state.hdma.in[0] :    (void*)&g_state.hdma.in[d+1];
    }

    for (d=0; d < L502_OUT_HDMA_DESCR_CNT; d++) {
        g_state.hdma.out[d].xmod = 2;
        g_state.hdma.out[d].valid = 1;
        g_state.hdma.out[d].next_descr = d==(    L502_IN_HDMA_DESCR_CNT-1) ?
                (void*)&g_state.hdma.out[0] :    (void*)&g_state.hdma.out[d+1];
    }
}



/** @brief  ������ ������ �� �������� �� HostDMA.

    ������� ���������� ������ ��������� ������� �� �������� ������ �� HostDMA
    � ��������� ��������. ������ ���������� �� ���������� ������� ������� � �������
    hdma_send_req_start() */
void hdma_send_start(void) {
    f_snd_start_id = 0;
    f_snd_done_id = 0;
    f_snd_next_descr = 0;

    g_state.hdma.in_lb.valid = 0;

    STREAM_IN_EN();
}

/** @brief  ������� ������ �� �������� �� HostDMA.

    ������ �������� �� HostDMA � ��������� ���� ������� ������� */
void hdma_send_stop(void) {
    STREAM_IN_DIS();
}

/** @brief  ������ ������ �� ����� �� HostDMA

    ������� ���������� ������ ��������� ������� �� ����� ������ �� HostDMA
    � ��������� �����. ������ ���������� �� ���������� ������� ������� � �������
    hdma_recv_req_start() */
void hdma_recv_start(void) {
    f_rcv_start_id = 0;
    f_rcv_done_id = 0;
    f_rcv_next_descr = 0;
    f_rcv_done_descr = 0;

    g_state.hdma.out_lb.valid = 0;
    STREAM_OUT_EN();
}

/** @brief  ������� ������ �� ����� �� HostDMA

    ������ ������ �� HostDMA � ��������� ���� ������� ������� */
void hdma_recv_stop(void) {
    STREAM_OUT_DIS();
}



/**************************************************************************//**
    @brief �������� ���������� ��������� �������� �� ��������.

    ������ ��������� ������, ������� �������� ����� ��� ��������� � ������� ��
    �������� � ������� hdma_send_start().
    @return ���������� �������� �� ��������, ������� ����� ��������� � �������
 ******************************************************************************/
int hdma_send_req_rdy(void) {
    return  L502_IN_HDMA_DESCR_CNT - (uint16_t)(f_snd_start_id - f_snd_done_id);
}

/**************************************************************************//**
    @brief �������� ���������� ��������� �������� �� �����

    ������ ��������� ������, ������� �������� ����� ��� ��������� � ������� ��
    ����� � ������� hdma_recv_start().
    @return ���������� �������� �� �����, ������� ����� ��������� � �������
 ******************************************************************************/
int hdma_recv_req_rdy(void) {
    return L502_OUT_HDMA_DESCR_CNT - (uint16_t)(f_rcv_start_id - f_rcv_done_id);
}

/**************************************************************************//**
    @brief  ��������� ������ �� �������� �� HostDMA

    ������� ������ ������ �� �������� ��������� ������. ���� ������ �� ����������,
    �.�. ����� ������ ����� ������������ �� ����, ��� ������ �� ����� ��������!
    ��� ���������� ������� ����������, ����� ��� ��������� ���������� (�����
    ������ ����� hdma_send_req_rdy())

    @param[in] buf   ��������� �� ������ �� ��������.
    @param[in] size      ���������� 32-������ ���� �� ��������
    @param[in] flags     ����� �� #t_hdma_send_flags
    @return              < 0 ��� ������, >= 0 - id �������� ��� ������
*******************************************************************************/
int hdma_send_req_start(const uint32_t* buf, uint32_t size, uint32_t flags) {
    if (hdma_send_req_rdy() > 0) {
        hdma_set_descr((t_hdma_stream_descr*)g_state.hdma.in, &f_snd_next_descr, &f_snd_start_id,
                       (uint32_t*)buf, size, flags);
        STREAM_IN_SET_REQ();
        return f_snd_start_id-1;
    }
    return -1;
}

/**************************************************************************//**
    @brief  ��������� ������ �� �������� �� HostDMA

    ������� ������ ������ �� ����� ������ � ��������� �����.
    ���� ������ ����� � ������ ������ �� ���������� �������.
    ��� ���������� ������� ����������, ����� ��� ��������� ���������� (�����
    ������ ����� hdma_recv_req_rdy())

    @param[in] buf   ��������� �� ������ �� ��������.
    @param[in] size      ���������� 32-������ ���� �� ��������
    @return              < 0 ��� ������, >= 0 - id �������� ��� ������
*******************************************************************************/
int hdma_recv_req_start(uint32_t* buf, uint32_t size) {
    if (hdma_recv_req_rdy() > 0) {
        hdma_set_descr((t_hdma_stream_descr*)g_state.hdma.out, &f_rcv_next_descr, &f_rcv_start_id, buf, size, 0);
        STREAM_OUT_SET_REQ();
        return f_rcv_start_id-1;
    }
    return -1;
}


/**
   @brief ���������� ���������� �� ���������� ������ � ������ BF �� HDMA

   ���������� ���������� �� ���������� ������ ����� �� HostDMA. ����������
   ��������� ��������� ����������� ������ ��� ���������� ������ ���������� �����
   �, ����� ����, ��������� ������� ����� ������� � ���������� �������� ��� ������
   ����� �� ������ ������ */
ISR(hdma_isr) {
    if ((*pDMA1_IRQ_STATUS & DMA_DONE) != 0) {
        /* ���������, �� ���� �� �������� ������� */
        if (g_state.cmd.status == L502_BF_CMD_STATUS_REQ) {
            l502_cmd_set_req();
        }
        /* ���������, �� ��� �� ������� ��������� ��������
           �� HDMA �� BF � PC */
        if (g_state.hdma.in_lb.valid) {
            /* ��������� id ����������� �������� � �������� callback */
            f_snd_done_id = g_state.hdma.in_lb.id;
            g_state.hdma.in_lb.valid = 0;
            hdma_send_done(g_state.hdma.in_lb.addr, g_state.hdma.in_lb.udata);
        }
        /* ���������, �� ��� �� ������� ��������� ������ ������ ��
            HDMA �� PC � BF */
        if (g_state.hdma.out_lb.valid) {
            /* ����� ���� ������� � ��� �� ��������� ����������� �������.
               ���������� ������ ������, ������� ���� ������� ������� */
            uint32_t size = (g_state.hdma.out[f_rcv_done_descr].full_size -
                            g_state.hdma.out_lb.full_size)/2;

            g_state.hdma.out_lb.valid = 0;

            hdma_recv_done(g_state.hdma.out_lb.addr, size);

            /* ���� ���� ��������� ������ ����� ����������� -
               ��������� ���������� ������ ��� ������ */
            if (g_state.hdma.out_lb.full_size) {
                g_state.hdma.out[f_rcv_done_descr].full_size =
                            g_state.hdma.out_lb.full_size;
            } else {
                /* ���� �������� ���� ���������� - ���������
                   � ���������� */
                if (++f_rcv_done_descr==    L502_OUT_HDMA_DESCR_CNT)
                    f_rcv_done_descr = 0;
                f_rcv_done_id = g_state.hdma.out_lb.id;
            }
        }

        *pDMA1_IRQ_STATUS = DMA_DONE;
        csync();
        *pHOST_STATUS = DMA_CMPLT;
    }
    ssync();
}

/**
    @brief ���������� ���������� �� ���������� ������ �� HDMA.

    ������ ���������� ���������� �� ���������� �������� ����� ������ �� HostDMA.
    ��������� ������ ��������� ����������� ������ ��� ���������� ���������
    �������� */
ISR(hdma_rd_isr) {
    if ((*pHOST_STATUS & HOSTRD_DONE) != 0) {
        *pHOST_STATUS &= ~((unsigned short)HOSTRD_DONE);
        *pHOST_STATUS |= DMA_CMPLT;
    }
    ssync();
}


/** @} */
