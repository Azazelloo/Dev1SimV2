	/***************************************************************************//**
	    @addtogroup user_process
	    @{
	    @file l502_user_process.c

	    ���� �������� ���������� ������� ���������� ���������������� �������, � �������
	    ������ ������ ���������� ��� ��������� � ������� ���������������� ������� ��
	    ��������������. ������������ ����� �������� ���� ���� � �������� ����� ����
	    ���������.
	*******************************************************************************/
	#include <stdlib.h>
	#include <string.h>
	#include <math.h>

	#include "l502_stream.h"
	#include "l502_hdma.h"
	#include "l502_sport_tx.h"
	#include "l502_cmd.h"
	#include "l502_params.h"

	#include "l502_user_types.h"
	#include "l502_user_data.h"
	#include "l502_async.h"
	//-----------------------------------------------------------------------------
	// 
	//			

	#define MEM_DEBUG 0x00000000
static uint32_t* sdram=(uint32_t*) MEM_DEBUG;


static bool FLAG_INIT= false;

static t_state STATE= _empty;

	static uint32_t ERRORS[2]= { L502_BF_ERR_SUCCESS, ERR_SUCCESS };	// ������ ([0] - X502, [1] - ���������)

	static uint32_t countFront=0,seq=0;

	bool findNull=false, findFront=false,writeFM=false;

	uint32_t searchWord= 0x78dd4259;

	#include "l502_sdram_noinit.h"
	static uint32_t seqArray;//[31];

	static uint32_t AMarray[1000];

	static uint32_t testArray[2]={1,0};

	//-----------------------------------------------------------------------------
	// ��������� ������ ���� ������ ��������. ���� ����� ������, ������������ 1
	//																						
	inline uint32_t sign_compare(uint32_t a, uint32_t b)
	{
		return ( ( a & 0x00800000 ) ^ ( b & 0x00800000 ) ) >> 23;
	}

	//-----------------------------------------------------------------------------
	// �� ���������� ��������� � ������� ������� ������������ ���� ��������� (+1, -1)
	// +1 - ����� 50% ������ ����������
	// -1 - ����� 50% ������ ������									
	inline float sign(uint32_t count)									
	{
		return count > ( ADC_BUF_SIZE_MAX / NUM_CH_ADC / 2 ) ? -1. : 1.;
	}


	inline int32_t as_int(const uint32_t value)
	{
		/* ��������� ������ - ������ ��������������� 24-������ �����,
							  ��� ����������������� 16-������ */

		uint32_t res;
		if (value & 0x40000000) {
			res = value & 0xFFFFFF;
			if (value & 0x800000)
				res |= 0xFF000000;
		}
		else {
			res = value & 0xFFFF;
			if (value & 0x8000)
				res |= 0xFFFF0000;
		}	

		return res;
	}

	uint32_t prepare_dac_wrd(double val)
	{
		int32_t wrd = 0;
		val = (val/5.)*30000;
		wrd = (int32_t)val;
		wrd &= 0xFFFF;
		return wrd;
	}


	/***************************************************************************//**
	    @brief ��������� ��������� ������� ������ ���/DIN.

	    ������� ���������� ������ ���, ����� ���������� ����� ������ ��
	    ���/�������� ������, ��������� �� SPORT0.

	    ������� ������ ���������� ������ � ������� ���������� ������������ ������,
	    ������ ��� ������ ��� ��� ��������� ��������������� (�� ����� ���� ����������
	    ������ ���������� �������) �� ��� ��� ���� �� ����� ������� �������
	    stream_in_buf_free()).

	    ���� ������� ������ �������� ������ ��� size, �� ������� ����� �������� ���
	    ��������� ������� ��� ��� � ���������� �� �������������� ������.

	    � ������� ���������� ������ ����������� �������� ������ �� HDMA � ��

	    @param[in] data   ��������� �� ������ � ��������� �������
	    @param[in] size   ���������� �������� ������ � 32-������ ������
	    @return           ������� ���������� ���������� ������������ ������ (�� 0 �� size).
	                      �� ��� ������ �� ����� ���������� �������� usr_in_proc_data(),
	                      �� ��� ��������� ��� �������������
	*******************************************************************************/
	int i,j,k,c;
	size_t lostSize = 0;

	uint32_t usr_in_proc_data(uint32_t* data, uint32_t size) {
	    /* ���� ���� ��������� ����������� �� �������� �� HDMA - ������ ���� ��
	       ��������. ����� ���������� 0, ����� �� ��������� ���� ������ �������
	       ������� �� ����� */

		// if ( ( STATE != _start ) && ( STATE != _control ) ){
		// 	stream_in_buf_free( size );	// ������ ������ �� ������������
		// 	return size;    	
		// }

		// uint32_t tmp=(*data) & 0x0001;
		// sport_tx_start_req(&tmp, 1);

		// stream_in_buf_free(size);
		// stream_out_buf_free(size);

		//_____�����������
		seqArray=0;
		���� ������� �� 0 � 1
		if (!findFront) {
			for (j = 1; j < size; j++) {
				if ( ((data[j-1] & 0x0001) == 0) && ((data[j] & 0x0001) == 1)) {

					//findFront = true;

					//_____���� ������ ����� ��� �������, �� �� ���������� ������ ������ ��
					if(!writeFM){
						findFront = true;
					}
					else{

						sport_tx_start_req(&AMarray[0], 1000);
						stream_out_buf_free(size);

						c=stream_disable(L502_STREAM_DIN);
						memcpy(sdram,&c,sizeof(int));
					}					
					break;
				}
			}
		} 

		if (findFront) {

			for (k = j;(k-j)< 31 && (k<= size);k++) {
				seqArray |= ((data[k] & 0x0002) >> 1) << (30-(k-j));
			}

			stream_in_buf_free(size);

			if((seqArray ^ searchWord)==0){
				writeFM=true;

				// sport_tx_start_req(&AMarray[0], 1000);
				// stream_out_buf_free(size);

				// memcpy(sdram,&seqArray,sizeof(uint32_t));
				// stream_disable(L502_STREAM_DIN);
			}

			findFront = false;
		}
		else{
			stream_in_buf_free(size);
		}

		return size;
	}



	/***************************************************************************//**
	    @brief ��������� ��������� ������� � ������� ���/DOUT

	    ������� ���������� ������ ���, ����� ���������� ����� ������, �������� ��
	    �� �� HDMA.
	    ������� ������ ���������� ������ � ������� ���������� ������������ ������,
	    ������ ��� ������ ��� ��� ��������� ��������������� (�� ����� ���� ����������
	    ������ ���������� �������) �� ��� ��� ���� �� ����� ������� �������
	    stream_out_buf_free()).

	    ���� ������� ������ �������� ������ ��� size, �� ������� �����
	    ������� ����� ��� ��� � ���������� �� �������������� ������.

	    � ������� ���������� ������ ����������� �������� ������ �� SPORT
	    ��� ������ �� ���/�������� ������.

	    @param[in] data   ��������� �� ������ � ��������� �������
	    @param[in] size   ���������� �������� ������ � 32-������ ������
	    @return           ������� ���������� ���������� ������������ ������ (�� 0 �� size).
	                      �� ��� ������ �� ����� ���������� �������� usr_out_proc_data(),
	                      �� ��� ��������� ��� �������������
	 ******************************************************************************/
	uint32_t usr_out_proc_data(uint32_t* data, uint32_t size) {

	    /* ���� ���� ��������� ����������� �� �������� �� HDMA - ������ ���� ��
	       ��������. ����� ���������� 0, ����� �� ��������� ���� ������ �������
	       ������� �� ����� */
		if (sport_tx_req_rdy()) {

	        /* �� ���� ��� ����� �������� � SPORT �� �����
	           SPORT_TX_REQ_SIZE_MAX ���� */
			if (size > SPORT_TX_REQ_SIZE_MAX)
				size = SPORT_TX_REQ_SIZE_MAX;

			sport_tx_start_req(data, size);

			return size;
		}
		return 0;
	}






	/****************************************************************************//**
	    @brief ��������� ���������� �������� �� HostDMA

	    ������� ���������� �� ����������� ����������, ����� ����������� ��������
	    ����� ������ �� HDMA � ��, ������������� �� ����� �� �������� �
	    ������� hdma_send_req_start().

	    @param[in] addr    ����� �����, ����� �� ��������� ���������� ������
	    @param[in] size    ������ ���������� ������ � 32-������ ������
	   ****************************************************************************/
	void hdma_send_done(uint32_t* addr, uint32_t size) {
		stream_in_buf_free(size);
	}


	/***************************************************************************//**
	    @brief ��������� ���������� �������� �� SPORT

	    ������� ���������� �� ����������� ���������� ��� ���������� �������� ����� ������
	    �� SPORT'� �� �������� ������/���, ������������� �� ����� �� �������� �
	    ������� sport_tx_start_req().

	    
	    @param[in] addr   ����� �����, ����� �� ��������� ���������� ������
	    @param[in] size   ������ ���������� ������ � 32-������ ������ */
	void sport_tx_done(uint32_t* addr, uint32_t size) {
		stream_out_buf_free(size);
	}

	void loadAMpulse(uint32_t* buf,size_t size){
		size_t i;

		for(i=0;i<62;i++){
			buf[i]=0x00000001;
		}

		for(;i<1000;i++){
			buf[i]=0x00000000;
		}
	}

	/****************************************************************************//**
	    @brief ��������� ���������������� ������.

	    ������� ���������� ��� ������ ������� �� �� � ����� ������� ��� ������
	    #L502_BF_CMD_CODE_USER.

	    �� ���������� ��������� ���������� ����������� �������
	    l502_cmd_done(), ������ ��� ���������� ������� �
	    ��� ������������� �������� ������ � �����������

	    @param[in] cmd  ��������� � ��������� �������� �������
	 ******************************************************************************/

	void usr_cmd_process(t_l502_bf_cmd *cmd)
	{		
		switch ( cmd->code )
		{
				//-------------- +1 *INIT	

			case L502_BF_CMD_CODE_USER + 1:						

			loadAMpulse(AMarray,1000);

			if ( ( STATE == _empty ) || ( STATE == _stop ) )							
			{

						ERRORS[0]= usr_init(); // ������������� ���, ���, ������� ��������� � �.�. 

						if ( ERRORS[0] == L502_BF_ERR_SUCCESS )
							STATE= _start;
						else
							STATE= _error;
					}
					else
						ERRORS[1]=ERR_INIT_IS_ALREADY_DONE; 

					l502_cmd_done( L502_BF_ERR_SUCCESS, &ERRORS[0], sizeof(ERRORS)/sizeof(ERRORS[0]) );

					break;
					
				//-------------- +2 *���������� ���������
				case L502_BF_CMD_CODE_USER + 2:	// ���������� ���������	
				
				ERRORS[0]= streams_stop();
				usr_stop();	

				if ( ERRORS[0] == L502_BF_ERR_SUCCESS )
					STATE= _stop;
				else
					STATE= _error;

				async_dout( 0x00000000, 0x00000000 );
				l502_cmd_done( L502_BF_ERR_SUCCESS, &ERRORS[0], sizeof(ERRORS)/sizeof(ERRORS[0]) );			

				break;	


				//-------------- +3 *TESTS
				case L502_BF_CMD_CODE_USER + 3:

				streams_start();

				l502_cmd_done( L502_BF_ERR_SUCCESS, &AMarray[0], 1000);
				break;
			}
			
			
		}








