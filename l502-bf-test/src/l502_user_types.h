	/** @defgroup user_process ���������������� ���� */

	#ifndef L502_USER_TYPES_H_
	#define L502_USER_TYPES_H_

	#ifndef M_PI
		#define M_PI 3.14159265358979323846
	#endif

	#define	OUT_BUFF_SIZE				1						//����� ������ ������ (!)
	#define IRQ_STEP					OUT_BUFF_SIZE			// ��� ���������� ��� ������ �� SPORT0



	#define ADC_BUF_SIZE_MAX			500								// ������������ ���������� ������������ ����							
	#define NUM_CH_ADC					1								// ���������� ��� ������� ��� ���������
	#define NUM_CH_OUT					1								// ���������� ���������� ������� (DOUT, ���1, ���2)
	#define NUM_SIN						2								// ���������� �������� �� 1 ����� ������							// ���������� ����� �� ����� (1000 - ��� �������� ��� DOUT)
														// ������� ����� ��� ( �������/������� �� �����/���-�� ������� )
	#define ADC_FREQ					2000000 / 150000 / NUM_CH_ADC
	#define SIN_AMPL					1000.							// ��������� ���������� ���������, ��
	#define OUT_IMPULSE_MAX				64								// ������������ ����� ��������, ����� (<) 													// ��������������� ���������� ������													
	#define PH_SIN1						31					
	#define PH_COS1						 6
	#define PH_SIN						15								// ��������� ������
	#define PH_SIN2						16
	#define PH_COS2						17
	#define SIN_IN						 0					 			// ����� ����������� ������, �� ������� 
																		// �������� �������� ��������� (������ � 0)

	//!!! ��������� ��� �������
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
	// ������
	//
	#define ERR_SUCCESS									0x0000
	#define ERR_INIT_IS_ALREADY_DONE					0x1000	// ������������� ��� ���������
	#define ERR_STATE_IS_NOT_INIT_OR_STOP				0x1001
	#define ERR_COMMAND_IS_ALLOWED_ONLY_AT_INIT_OR_STOP	0x1002
	#define ERR_OVERFLOW_OF_ANGLES						0x1003
	#define ERR_COMMAND_IS_ALLOWED_ONLY_AT_START		0x1004
	//-----------------------------------------------------------------------------
	//
	//
	typedef enum
	{
		_empty	    = 0,		// �������� ���������
		_init	    = 1,		// ������������� �������� ���������, ������ �� �������, ��������� �������
		_start      = 2,		// ������ �������, ������������ ���������� ����������, ���������� ���������� ������� ���������
		_control    = 3,		// ������ �������, ���������� ���������� ������� ��������� � ���������� ��������
		_stop       = 4,		// ������ ���������, ��������� �������	
		_error	    = 5
		
	} t_state;


	#endif

	/** @} */
