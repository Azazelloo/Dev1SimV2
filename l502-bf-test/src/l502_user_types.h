	/** @defgroup user_process Пользовательские типы */

	#ifndef L502_USER_TYPES_H_
	#define L502_USER_TYPES_H_

	#ifndef M_PI
		#define M_PI 3.14159265358979323846
	#endif

	#define	OUT_BUFF_SIZE				1						//САМЫЙ ВАЖНЫЙ ДЕФАЙН (!)
	#define IRQ_STEP					OUT_BUFF_SIZE			// Шаг прерывания для приема по SPORT0



	#define ADC_BUF_SIZE_MAX			500								// Максимальное количество оцифрованных слов							
	#define NUM_CH_ADC					1								// Количество АЦП каналов для оцифровки
	#define NUM_CH_OUT					1								// Количество выдаваемых каналов (DOUT, ЦАП1, ЦАП2)
	#define NUM_SIN						2								// Количество синусоид на 1 фрейм выдачи							// Количество точек на канал (1000 - нет дрожания для DOUT)
														// Частота сбора АЦП ( базовая/частота на канал/кол-во каналов )
	#define ADC_FREQ					2000000 / 150000 / NUM_CH_ADC
	#define SIN_AMPL					1000.							// Амплитуда выдаваемой синусоиды, мВ
	#define OUT_IMPULSE_MAX				64								// Максимальная длина импульса, мксек (<) 													// Задействованные физические каналы													
	#define PH_SIN1						31					
	#define PH_COS1						 6
	#define PH_SIN						15								// Эталонный сигнал
	#define PH_SIN2						16
	#define PH_COS2						17
	#define SIN_IN						 0					 			// Номер логического канала, на который 
																		// приходит задающая синусоида (отсчет с 0)

	//!!! Структура для отладки
	typedef struct
	{
		uint32_t size;
		uint32_t data[ ADC_BUF_SIZE_MAX ];	
		
		uint32_t counter;			// +1								
		uint32_t error;				// +2
		
		float    tg_x;				// +3
		float    tg_y;				// +4
		float	 angle_x;			// +5
		float	 angle_y;			// +6

		float	 offs1; 			// +7
		float	 k1;				// +8
		
		float	 offs2; 			// +9
		float	 k2;				// +10
		
		uint32_t adc_freq_div;		// +11
		uint32_t adc_frame_delay;	// +12
		
		uint32_t sum1;				// +13
		uint32_t sum2;				// +14
		uint32_t sum3;				// +15
		uint32_t sum4;				// +16

	} t_debug_data;


	//-----------------------------------------------------------------------------
	// Ошибки
	//
	#define ERR_SUCCESS									0x0000
	#define ERR_INIT_IS_ALREADY_DONE					0x1000	// Инициализация уже проведена
	#define ERR_STATE_IS_NOT_INIT_OR_STOP				0x1001
	#define ERR_COMMAND_IS_ALLOWED_ONLY_AT_INIT_OR_STOP	0x1002
	#define ERR_OVERFLOW_OF_ANGLES						0x1003
	#define ERR_COMMAND_IS_ALLOWED_ONLY_AT_START		0x1004
	//-----------------------------------------------------------------------------
	//
	//
	typedef enum
	{
		_empty	    = 0,		// Прошивка загружена
		_init	    = 1,		// Инициализация прошивки выполнена, потоки не активны, обнуление выходов
		_start      = 2,		// Потоки активны, инциализация глобальных переменных, происходит вычисление текущих координат
		_control    = 3,		// Потоки активны, происходит вычисление текущих координат и управление приводом
		_stop       = 4,		// Потоки отключены, обнуление выходов	
		_error	    = 5
		
	} t_state;


	#endif

	/** @} */
