#include "Header.h"

int Get_ADC_1chan(t_l502_hnd& create, double* adc_out_buf, uint32_t adc_out_buf_size)
{
	
	L502_StreamsEnable(create, L502_STREAM_ADC);
	int err = 0;

	uint32_t* out_buffer=new uint32_t[adc_out_buf_size];

	err = L502_Recv(create, out_buffer, adc_out_buf_size, 100);
	if (err<0)
	{
		cout << "Ошибка приема данных АЦП: " << L502_GetErrorString(err) << endl;
		system("pause");
		return err;
	}
	else if(err < adc_out_buf_size)
	{
		cout << "Прочитаны не все данные!" << endl;
		return err;
	}

	L502_StreamsDisable(create, L502_STREAM_ADC);

	err = L502_ProcessAdcData(create, out_buffer, adc_out_buf, &adc_out_buf_size, L502_DAC_FLAGS_VOLT);
	if (err != L502_ERR_OK)
	{
		cout << "Ошибка обработки данных с АЦП: " << L502_GetErrorString(err) << endl;
		system("pause");
		return err;
	}

	delete[] out_buffer;
	return err;
}