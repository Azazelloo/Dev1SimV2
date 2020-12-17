/***************************************************************************//**
    @addtogroup user_data
    @{
    @file l502_user_data.c

    ������������� ����������������� ������� �� ������
*******************************************************************************/

#include <stdint.h>
#include <math.h>

#include "l502_bf_cmd_defs.h"
#include "l502_params.h"
#include "l502_async.h"
#include "l502_sport_rx.h"
#include "l502_stream.h"

#include "l502_user_data.h"

//-----------------------------------------------------------------------------
// 
//							
#include "l502_sdram_noinit.h"	
static t_debug_data DEBUG_DATA;

#include "l502_sdram_noinit.h"
static volatile uint32_t OUT_BUFF[OUT_BUFF_SIZE];	// ������ ��� ������ DOUT, ���1 � ���2

/***************************************************************************//**
    @brief �������������� ���������� - ���

*******************************************************************************/
uint16_t float_to_cod(float volt, float Uop)
{
	return (uint16_t)( (int16_t)( volt * (float)0x8000 / Uop ) );
}

/***************************************************************************//**
    @brief ���������������� �������������

*******************************************************************************/
int32_t usr_init(void) {

	memset(OUT_BUFF,0,sizeof(OUT_BUFF)*OUT_BUFF_SIZE);

	int32_t err= L502_BF_ERR_SUCCESS;	
											// ����������� ���������� ���������
	DEBUG_DATA.counter= 0;		
	DEBUG_DATA.error  = 0;		
	DEBUG_DATA.tg_x	  = 0.;
	DEBUG_DATA.tg_y   = 0.;
	DEBUG_DATA.angle_x= 0.;
	DEBUG_DATA.angle_y= 0.;			
	DEBUG_DATA.offs1  = 0.; 
	DEBUG_DATA.k1     = 0.;	
	DEBUG_DATA.offs2  = 0.; 
	DEBUG_DATA.k2     = 0.;	
	DEBUG_DATA.adc_freq_div	  = g_set.adc_freq_div;
	DEBUG_DATA.adc_frame_delay= g_set.adc_frame_delay;	

	err= params_set_lch_cnt( NUM_CH_ADC );	// ���������� ���������� ���������� �������
	if ( err != L502_BF_ERR_SUCCESS )
		return err;

	async_dac_out( L502_DAC_CH1, 0 );		// �������� ����
	async_dac_out( L502_DAC_CH2, 0 );

	async_dout( 0x00000000, 0x00000000 );	// �������� �������� ������
	
											// ���������������� ������� 
											// ����. ���������� �� ������� ���. ������ = 1
											// ����� �����, +-5 �����

	// err= params_set_lch( 0, 0, L502_LCH_MODE_DIFF, L502_ADC_RANGE_5, 1, 0 ); //������ �������� - ����� ����������� ������ (0?)
	// if ( err != L502_BF_ERR_SUCCESS )
	// 	return err;	
	
	// err= params_set_adc_freq_div(2000000/500000);	// ���������� ������� ����� ��� ���
	// if ( err != L502_BF_ERR_SUCCESS )
	// 	return err;
	
	// err= params_set_ref_freq( L502_REF_FREQ_2000KHZ );
	// if ( err != L502_BF_ERR_SUCCESS )
	// 	return err;

	err= params_set_din_freq_div(2000000/500000);//(2000000/500000);	// ���������� ������� ����� ��� DIN
	if ( err != L502_BF_ERR_SUCCESS )
		return err;

	err= params_set_adc_interframe_delay(0);
	if ( err != L502_BF_ERR_SUCCESS )
		return err;								

	err= params_set_dac_freq_div(2000000/1000000);		// ��������� �������� ������� ������, 2��� / 2 = 1��� (!!!!)
	if ( err != L502_BF_ERR_SUCCESS )
		return err;	

	err= sport_in_set_step_size(IRQ_STEP);	// ��������� ���� ���������� ��� ������ �� SPORT0
	if ( err != L502_BF_ERR_SUCCESS )
		return err;			

	err= configure();						// ������������
	if ( err != L502_BF_ERR_SUCCESS )
		return err;								

	err= stream_enable(L502_STREAM_DIN | L502_STREAM_DOUT);	// ��������� ������
	if ( err != L502_BF_ERR_SUCCESS )
		return err;	

	err= stream_disable(L502_STREAM_ADC);   
	if ( err != L502_BF_ERR_SUCCESS )
		return err;	   

	// err = streams_start();
	// if ( err != L502_BF_ERR_SUCCESS )
	// 	return err;

	return err;
}


/***************************************************************************//**
    @brief ��������� ��������� ��������

*******************************************************************************/
int32_t usr_stop(void) {

	int32_t err= L502_BF_ERR_SUCCESS;	
											// �������� ����
	async_dac_out( L502_DAC_CH1, 0 );
	async_dac_out( L502_DAC_CH2, 0 );
											// �������� �������� ������
	async_dout( 0x00000000, 0x00000000 );		

	return err;
}


/***************************************************************************//**
    @brief ������������� ������� �� ������.

*******************************************************************************/
void usr_create_output_buffer(void) {

	uint32_t i;	
	
							// ��� ���� �������
	for (i=0; i<OUT_BUFF_SIZE; i++)
	{
    			 						// ������ ��� DOUT
    	//OUT_BUFF[i * NUM_CH_OUT + 0]= 0x00000001;    	
		
    									// ������������ �������� ��������� (� ������ ������������� �������������)
		float dac       = SIN_AMPL * sin( 2. * M_PI * i / (float)( OUT_BUFF_SIZE / NUM_SIN ) );
		float dac_kalib1= ( dac + g_module_info.dac_cbr[0].offs ) * g_module_info.dac_cbr[0].k;
		float dac_kalib2= ( dac + g_module_info.dac_cbr[1].offs ) * g_module_info.dac_cbr[1].k;
		
    		    		    			// ������ ��� ���1
		OUT_BUFF[i * NUM_CH_OUT + 1]= 0x40000000 | float_to_cod( dac_kalib1, 5000.);   
    		    		    			// ������ ��� ���2
    	//OUT_BUFF[i * NUM_CH_OUT + 2]= 0x80000000 | float_to_cod( dac_kalib2, 5000.);
	} 

	DEBUG_DATA.offs1= g_module_info.dac_cbr[0].offs;	
	DEBUG_DATA.k1   = g_module_info.dac_cbr[0].k;	              
	
	DEBUG_DATA.offs2= g_module_info.dac_cbr[1].offs;	
	DEBUG_DATA.k2   = g_module_info.dac_cbr[1].k;
}

/**************************************************************************//**
    @brief ��������� �� ������ ������.

*******************************************************************************/
uint32_t * usr_get_output_buffer(void) {
	return (uint32_t *)&OUT_BUFF[0]; 
}

/***************************************************************************//**
    @brief ��������� �� ���������� ���������.

*******************************************************************************/
t_debug_data * usr_get_debug_data(void) {
	return &DEBUG_DATA;
};

/** @} */








