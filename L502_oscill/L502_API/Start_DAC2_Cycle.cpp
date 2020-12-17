#include "Header.h"

int Start_DAC2_Cycle(t_l502_hnd& create,double* buf,int buf_size)
{
	int err = 0;
	uint32_t* out_buf=new uint32_t[buf_size];

	//_____выделяем место в драйвере под циклический буфер
	err = L502_OutCycleLoadStart(create, buf_size);
	if (err != L502_ERR_OK)
	{
		cout << "Ошибка выделения памяти в драйвере: " << L502_GetErrorString(err) << endl;
		return err;
	}

	//_____подготовка блока данных для записи
	err = L502_PrepareData(create, NULL, buf, NULL, buf_size, L502_DAC_FLAGS_VOLT, out_buf);
	if (err != L502_ERR_OK)
	{
		cout << "Ошибка подготовка блока данных для записи: " << L502_GetErrorString(err) << endl;
		return err;
	}

	//_____запись подготовленного блока
	err = L502_Send(create, out_buf, buf_size, 500);
	if (err < 0)
	{
		cout << "Ошибка передачи данных: " << L502_GetErrorString(err) << endl;
		return err;
	}
	else if (err < 0)
	{
		if (err < buf_size)
		{
			cout << "Ошибка передачи данных: передано слов = " << err << endl;
			return err;
		}
	}

	//_____делаем активным ранее загруженный буфер
	err = L502_OutCycleSetup(create, L502_OUT_CYCLE_FLAGS_FORCE);
	if (err != L502_ERR_OK)
	{
		cout << "Ошибка активации буфера: " << L502_GetErrorString(err) << endl;
		return err;
	}

	delete[] out_buf;

	return err;
}