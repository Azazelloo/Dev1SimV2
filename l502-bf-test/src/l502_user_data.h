/** @defgroup user_process Пользовательские данные */

#ifndef L502_USER_DATA_H_
#define L502_USER_DATA_H_

#include "l502_user_types.h"

				// Пользовательская инициализация
int32_t usr_init(void);
				// Окончание обработки сигналов
int32_t usr_stop(void);
				// Инициализация массива на выдачу (DOUT, АЦП1 и АЦП2)
void usr_create_output_buffer(void);
				// Вернуть указатель на массив выдачи
uint32_t * usr_get_output_buffer(void);
				// Вернуть указатель на отладочную структуру
t_debug_data * usr_get_debug_data(void);

#endif

/** @} */
