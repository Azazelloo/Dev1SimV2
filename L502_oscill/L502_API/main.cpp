#include "Header.h"

mutex m;
t_l502_hnd create = L502_Create();
ofstream file_out("D:\\data.txt");

int err = 0;
volatile size_t rcv_size, block;
size_t adc_size;

static bool flag = true;
bool f_out = false;

static uint32_t words[ADC_ARRAY_SIZE];
static uint32_t tmp_words[ADC_ARRAY_SIZE];

static double rcv_buf1[ADC_ARRAY_SIZE] = {0};
static double rcv_buf2[ADC_ARRAY_SIZE] = {0};

double* p_buf = nullptr;

void in()
{
	p_buf = rcv_buf1;
	for (block = 0; (err == L502_ERR_OK) && !f_out; block++)
	{
		//auto begin = chrono::high_resolution_clock::now();

		rcv_size = L502_Recv(create, words, ADC_ARRAY_SIZE,10000);//2000//180
		adc_size = ADC_ARRAY_SIZE;
		L502_ProcessData(create, words, rcv_size, L502_PROC_FLAGS_VOLT, p_buf, &adc_size, NULL, NULL);
		flag = true;

		if (p_buf == rcv_buf1) p_buf = rcv_buf2;
		else p_buf = rcv_buf1;

			/* проверка нажатия клавиши для выхода */
			if (err == L502_ERR_OK) {
				if (_kbhit())
				{
					f_out = 1;
					flag = false;
				}
			}

		//auto end = chrono::high_resolution_clock::now();
		//cout << chrono::duration_cast<chrono::nanoseconds>(end - begin).count() << "ns" << endl;
		stop
	}
}

void out()
{
	
	while (!flag) {} //ожидаем запись первого буфера
	while (flag)
	{
		/*Вывод в *.txt*/
		/*for (size_t i=0;i< ADC_ARRAY_SIZE;i++)
		{
			file_out << p_buf[i] << endl;
		}*/

		L502_PrepareData(create, NULL, p_buf, NULL, ADC_ARRAY_SIZE, L502_DAC_FLAGS_VOLT, tmp_words);
		L502_Send(create, tmp_words, ADC_ARRAY_SIZE, 10000);

		stop
	}

	file_out.close();
}


int main(int argc, char** argv)
	{	
		setlocale(LC_CTYPE, "");
		if (connect_L502(&create))
		{
			/*Load BF*/
			/*err = L502_BfLoadFirmware(create, "../bf/l502-bf.ldr");
			if (err != X502_ERR_OK)
			{
				std::cout << "blackfin load error " << err << std::endl;
			}
			else
			{
				std::cout << "blackfin loaded! " << std::endl;
				system("pause");
				L502_AsyncOutDig(create, 0x00000000, NULL);
			}/**/

			/*Непрерывный опрос АЦП*/
			SetDev(create);

			cout << "Load date. Press any button to stop...." << endl;
			boost::thread_group threads;
			threads.create_thread(&in);
			threads.create_thread(&out);
			threads.join_all();

			
			/*Завершающая часть*/
			L502_StreamsStop(create);
			L502_Close(create);
			L502_Free(create);
		}
		return 0;
	}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           