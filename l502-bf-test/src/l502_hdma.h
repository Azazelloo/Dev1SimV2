/** @defgroup hdma ������ � ����������� HostDMA */

/***************************************************************************//**
    @addtogroup hdma
    @{
    @file l502_hdma.h

    ���� �������� �������� ������� ��� ������ � ����������� HostDMA
 ******************************************************************************/

#ifndef L502_HDMA_H_
#define L502_HDMA_H_


void hdma_init(void);

/** ����� ��� �������� �� #t_hdma_send_flags */
typedef enum {
    /** ����, �����������, ��� ���������� ��������� ����� ������. �� ����������
        �������� ����� ���������� ����� � ������������� ���������� ��� �� */
    L502_HDMA_FLAGS_SEND_LAST    = 0x1
} t_hdma_send_flags;


void hdma_send_start(void);
void hdma_send_stop(void);


int hdma_send_req_start(const uint32_t* snd_buf, uint32_t size, uint32_t flags);
int hdma_send_req_rdy(void);


void hdma_recv_start(void);

void hdma_recv_stop(void);

int hdma_recv_req_start(uint32_t* buf, uint32_t size);

int hdma_recv_req_rdy(void);


#endif

/** @} */
