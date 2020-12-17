/** @defgroup user_process ���������������� ������ */

#ifndef L502_USER_DATA_H_
#define L502_USER_DATA_H_

#include "l502_user_types.h"

				// ���������������� �������������
int32_t usr_init(void);
				// ��������� ��������� ��������
int32_t usr_stop(void);
				// ������������� ������� �� ������ (DOUT, ���1 � ���2)
void usr_create_output_buffer(void);
				// ������� ��������� �� ������ ������
uint32_t * usr_get_output_buffer(void);
				// ������� ��������� �� ���������� ���������
t_debug_data * usr_get_debug_data(void);

#endif

/** @} */
