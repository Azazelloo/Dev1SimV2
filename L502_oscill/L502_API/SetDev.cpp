#include "Header.h"

int SetDev(t_l502_hnd& create)
{
	int err = 0;

	//_____количество логических каналов
	err = L502_SetLChannelCount(create, 1);
	if (err != L502_ERR_OK)
	{
		cout << "Ошибка установки количества логических каналов: " << L502_GetErrorString(err) << endl;
		system("pause");
		return err;
	}

	//_____настройка таблицы логических каналов
	err = L502_SetLChannel(create, 0, 0, L502_LCH_MODE_DIFF, L502_ADC_RANGE_10, 1);
	if (err != L502_ERR_OK)
	{
		cout << "Ошибка настройки логических каналов: " << L502_GetErrorString(err) << endl;
		system("pause");
		return err;
	}

	//_____устанавливаем размер буфера драйвера
	L502_SetDmaBufSize(create, L502_DMA_CH_IN | L502_DMA_CH_OUT, ADC_ARRAY_SIZE);

	//_____подбираем делитель частоты АЦП
	double f_acq = F_ACQ*pow(10,3);
	L502_SetAdcFreq(create, &f_acq, nullptr);
	stop
	//_____запись настроек на устройство
	err = L502_Configure(create, 0);
	if (err != L502_ERR_OK)
	{
		cout << "Ошибка отправки настроек: " << L502_GetErrorString(err) << endl;
		system("pause");
		return err;
	}

	//_____разрешаем нужные потоки
	err = L502_StreamsEnable(create, L502_STREAM_ADC | L502_STREAM_DAC2);
	if (err != L502_ERR_OK)
	{
		cout << "Ошибка разрешения потоков: " << L502_GetErrorString(err) << endl;
		system("pause");
		return err;
	}

	//_____стартуем потоки
	err = L502_StreamsStart(create);
	if (err != L502_ERR_OK)
	{
		cout << "Ошибка старта потоков: " << L502_GetErrorString(err) << endl;
		system("pause");
		return err;
	}

	return err;
}