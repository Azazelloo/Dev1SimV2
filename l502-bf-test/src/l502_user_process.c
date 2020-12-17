	/***************************************************************************//**
	    @addtogroup user_process
	    @{
	    @file l502_user_process.c

	    Файл содержит простейший вариант реализации пользовательских функций, в котором
	    потоки данных передаются без изменения и никакие пользовательские команды не
	    обрабатываются. Пользователь может изменить этот файл и добавить здесь свою
	    обработку.
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

	static uint32_t ERRORS[2]= { L502_BF_ERR_SUCCESS, ERR_SUCCESS };	// Ошибка ([0] - X502, [1] - программы)

	static uint32_t countFront=0,seq=0;

	bool findNull=false, findFront=false,writeFM=false;

	uint32_t searchWord= 0x78dd4259;

	#include "l502_sdram_noinit.h"
	static uint32_t seqArray;//[31];

	static uint32_t AMarray[1000];

	static uint32_t testArray[2]={1,0};

	//-----------------------------------------------------------------------------
	// Сравнение знаков двух разных значений. Если знаки разные, возвращается 1
	//																						
	inline uint32_t sign_compare(uint32_t a, uint32_t b)
	{
		return ( ( a & 0x00800000 ) ^ ( b & 0x00800000 ) ) >> 23;
	}

	//-----------------------------------------------------------------------------
	// По количеству элементов с разными знаками возвращается фаза синусоиды (+1, -1)
	// +1 - менее 50% знаков одинаковые
	// -1 - более 50% знаков разные									
	inline float sign(uint32_t count)									
	{
		return count > ( ADC_BUF_SIZE_MAX / NUM_CH_ADC / 2 ) ? -1. : 1.;
	}


	inline int32_t as_int(const uint32_t value)
	{
		/* проверяем формат - пришло откалиброванное 24-битное слово,
							  или неоткалиброванное 16-битное */

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
	    @brief Обработка принятого массива данных АЦП/DIN.

	    Функция вызывается каждый раз, когда обнаружены новые данные от
	    АЦП/цифровых входов, пришедшие по SPORT0.

	    Функция должна обработать данные и вернуть количество обработанных данных,
	    однако эти данные все еще считаются использованными (не могут быть переписаны
	    новыми пришедшими данными) до тех пор пока не будет вызвана функция
	    stream_in_buf_free()).

	    Если функция вернет значение меньше чем size, то функция будут вызванна при
	    следующем проходе еще раз с указателем на необработанные данные.

	    В текущей реализации просто запускается передача данных по HDMA в ПК

	    @param[in] data   Указатель на массив с принятыми данными
	    @param[in] size   Количество принятых данных в 32-битных словах
	    @return           Функция возвращает количество обработанных данных (от 0 до size).
	                      На эти данные не будет вызываться повторно usr_in_proc_data(),
	                      но они считаются еще используемыми
	*******************************************************************************/
	int i,j,k,c;
	size_t lostSize = 0;

	uint32_t usr_in_proc_data(uint32_t* data, uint32_t size) {
	    /* если есть свободные дескрипторы на передачу по HDMA - ставим блок на
	       передачу. Иначе возвращаем 0, чтобы на обработку этих данных функцию
	       вызвали бы позже */

		// if ( ( STATE != _start ) && ( STATE != _control ) ){
		// 	stream_in_buf_free( size );	// Данные больше не используются
		// 	return size;    	
		// }

		// uint32_t tmp=(*data) & 0x0001;
		// sport_tx_start_req(&tmp, 1);

		// stream_in_buf_free(size);
		// stream_out_buf_free(size);

		//_____анализируем
		seqArray=0;
		ищем перепад из 0 в 1
		if (!findFront) {
			for (j = 1; j < size; j++) {
				if ( ((data[j-1] & 0x0001) == 0) && ((data[j] & 0x0001) == 1)) {

					//findFront = true;

					//_____если нужное слово уже найдено, то по следующему фронту подаем АМ
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
	    @brief Обработка принятого массива с данными ЦАП/DOUT

	    Функция вызывается каждый раз, когда обнаружены новые данные, принятые от
	    ПК по HDMA.
	    Функция должна обработать данные и вернуть количество обработанных данных,
	    однако эти данные все еще считаются использованными (не могут быть переписаны
	    новыми пришедшими данными) до тех пор пока не будет вызвана функция
	    stream_out_buf_free()).

	    Если функция вернет значение меньше чем size, то функция будут
	    вызвана после еще раз с указателем на необработанные данные.

	    В текущей реализации просто запускается передача данных по SPORT
	    для вывода на ЦАП/цифровые выходы.

	    @param[in] data   Указатель на массив с принятыми данными
	    @param[in] size   Количество принятых данных в 32-битных словах
	    @return           Функция возвращает количество обработанных данных (от 0 до size).
	                      На эти данные не будет вызываться повторно usr_out_proc_data(),
	                      но они считаются еще используемыми
	 ******************************************************************************/
	uint32_t usr_out_proc_data(uint32_t* data, uint32_t size) {

	    /* если есть свободные дескрипторы на передачу по HDMA - ставим блок на
	       передачу. Иначе возвращаем 0, чтобы на обработку этих данных функцию
	       вызвали бы позже */
		if (sport_tx_req_rdy()) {

	        /* за один раз можем передать в SPORT не более
	           SPORT_TX_REQ_SIZE_MAX слов */
			if (size > SPORT_TX_REQ_SIZE_MAX)
				size = SPORT_TX_REQ_SIZE_MAX;

			sport_tx_start_req(data, size);

			return size;
		}
		return 0;
	}






	/****************************************************************************//**
	    @brief Обработка завершения передачи по HostDMA

	    Функция вызывается из обработчика прерывания, когда завершилась передача
	    блока данных по HDMA в ПК, поставленного до этого на передачу с
	    помощью hdma_send_req_start().

	    @param[in] addr    Адрес слова, сразу за последним переданным словом
	    @param[in] size    Размер переданных данных в 32-битных словах
	   ****************************************************************************/
	void hdma_send_done(uint32_t* addr, uint32_t size) {
		stream_in_buf_free(size);
	}


	/***************************************************************************//**
	    @brief Обработка завершения передачи по SPORT

	    Функция вызывается из обработчика прерывания при завершении передачи блока данных
	    по SPORT'у на цифровые выходы/ЦАП, поставленного до этого на передачу с
	    помощью sport_tx_start_req().

	    
	    @param[in] addr   Адрес слова, сразу за последним переданным словом
	    @param[in] size   Размер переданных данных в 32-битных словах */
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
	    @brief Обработка пользовательских команд.

	    Функция вызывается при приеме команды от ПК с кодом большим или равным
	    #L502_BF_CMD_CODE_USER.

	    По завершению обработки необходимо обязательно вызвать
	    l502_cmd_done(), указав код завершения команды и
	    при необходимости передать данные с результатом

	    @param[in] cmd  Структура с описанием принятой команды
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

						ERRORS[0]= usr_init(); // Инициализация ЦАП, АЦП, режимов оцифровки и т.д. 

						if ( ERRORS[0] == L502_BF_ERR_SUCCESS )
							STATE= _start;
						else
							STATE= _error;
					}
					else
						ERRORS[1]=ERR_INIT_IS_ALREADY_DONE; 

					l502_cmd_done( L502_BF_ERR_SUCCESS, &ERRORS[0], sizeof(ERRORS)/sizeof(ERRORS[0]) );

					break;
					
				//-------------- +2 *завершение обработки
				case L502_BF_CMD_CODE_USER + 2:	// Завершение обработки	
				
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








