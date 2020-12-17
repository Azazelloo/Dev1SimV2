/** @defgroup user_process ���������������� ��������� ������ */

/***************************************************************************//**
    @addtogroup user_process
    @{
    @file l502_user_process.h

    ���� �������� �������� �������, ������� ������������� ��� ���������
    ������������� ��� ��������� ����� ���������� ��������� ������ � ����������
    ���������������� ������.
 ******************************************************************************/



#ifndef L502_USER_PROCESS_H_
#define L502_USER_PROCESS_H_


uint32_t usr_in_proc_data(uint32_t* data, uint32_t size);
uint32_t usr_out_proc_data(uint32_t* data, uint32_t size);

void usr_cmd_process(t_l502_bf_cmd *cmd);

#endif

/** @} */
