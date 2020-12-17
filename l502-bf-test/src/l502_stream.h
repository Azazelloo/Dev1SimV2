/** @defgroup streams ���������� �������� ����� ������ */

/***************************************************************************//**
    @addtogroup streams
    @{
    @file l502_stream.h

    ���� �������� �������� ������� ��� ������ � ����������� HostDMA
 ******************************************************************************/

#ifndef L502_STREAM_H_
#define L502_STREAM_H_

#include <stdint.h>

/** ��������� ������ �� ���� */
typedef enum {
    IN_STREAM_STOP = 0,     /**< ����� �� ���� ���������� */
    IN_STREAM_RUN = 2,      /**< ����� �� ���� � ������� ������ */
    IN_STREAM_OV_ALERT = 4, /**< ��������� ������������ - ����� �������� � ������ */
    IN_STREAM_ERR= 3        /**< ����� ���������� ��-�� ������/������������ */
} t_in_stream_state;

/** ��������� ������ �� ����� */
typedef enum {
    OUT_STREAM_STOP = 0,    /**< ����� �� ����� ���������� */
    OUT_STREAM_PRELOAD = 1, /**< ���� ������������ ������ - ������ �����������
                                �� ��, �� ��� �� ������� ���������� ����/������ */
    OUT_STREAM_RUN = 2,     /** ����� ������� �� ������ */
    OUT_STREAM_ERR = 3,     /** ����� ���������� �� ������ (������ �� ������������) */
    OUT_STREAM_CYCLE = 4    /** ����� ������������ ������ (������ �� ����������) */
} t_out_stream_state;


/** ��������� ������ �� ���� */
extern t_in_stream_state  g_stream_in_state;
/** ��������� ������ �� ����� */
extern t_out_stream_state g_stream_out_state;
/** ����� ������ �������� �� #t_l502_bf_mode */
extern volatile int g_mode;
/** �����, ������������ ����� ������ ��������� */
extern int g_streams;


int32_t streams_start(void);
int32_t streams_stop(void);

int32_t stream_enable(uint32_t streams);
int32_t stream_disable(uint32_t streams);

int32_t stream_out_preload(void);


void stream_in_buf_free(uint32_t size);
void stream_out_buf_free(uint32_t size);


uint32_t sport_in_buffer_size(void);
int32_t sport_in_set_step_size(uint32_t size);



#endif

/** @} */
