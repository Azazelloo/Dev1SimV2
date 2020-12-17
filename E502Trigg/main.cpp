#include "Header.h"
#include "Timer.h"
int main() {
	int err = 0;

	//_____подключаемся и настраиваем

	char serials[maxCountDev][X502_SERIAL_SIZE];
	err = E502_UsbGetSerialList(serials, maxCountDev,NULL,NULL); //получаем список доступных устройств

	t_x502_hnd hnd = X502_Create(); //создаем описатель

	err = E502_OpenUsb(hnd, serials[0]);

	double f_acq = 500 * pow(10, 3); //для сигнала2
	err=X502_SetDinFreq(hnd, &f_acq);

	double f_acq2 = 1000* pow(10, 3);
	err = X502_SetOutFreq(hnd, &f_acq2);

	//err = X502_SetStreamBufSize(hnd, X502_STREAM_CH_OUT, BUFSIZE);

	err = X502_Configure(hnd,0);

	err = X502_StreamsEnable(hnd, X502_STREAM_DIN | X502_STREAM_DOUT);

	//включаем потоки
	err = X502_StreamsStart(hnd);

	//_____прием данных
		
	//готовим память для приема цифровых входов
	uint32_t wrdsIn[BUFSIZE];

	uint32_t wrdsOut[BUFSIZE];
	uint32_t tmp[SizeAMpulse];

	//готовим импульс АМ для последующей выдачи
	uint32_t arrayAMwrdsOut[SizeAMpulse];
	uint32_t arrayAMwrds[SizeAMpulse] = {0};

	for (int i = 0; i < AMpulseWidth;i++) {
		arrayAMwrds[i] = 1;
	}

	X502_OutCycleLoadStart(hnd, SizeAMpulse);
	err = X502_Send(hnd, arrayAMwrds, SizeAMpulse, 1000);

	uint32_t prevWordAMfront,nextWordAMfront,wrdsOutSize = BUFSIZE, sizeFMseq= SIZESEQFM;
	bool findNull = false, findFront = false, startAM=false;

	std::bitset<SIZESEQFM> chkWordSeq;
	std::bitset<SIZESEQFM> findWords = 0x78DD4259;

	int frontCount = 0,block=0;
	int i, j, k;
	size_t lostSize = 0;

	//std::ofstream out;
	//out.open("disp.txt");
	 
	std::cout << "Started...\n" << std::endl;

	while (!startAM) {

		err = X502_Recv(hnd, wrdsIn, BUFSIZE, 1000);

		//если на предыдущей итерации было записано неполное слово, дозаписываем
		if (lostSize!=0) {

			for (int m = 1; m < lostSize; m++) {
				chkWordSeq[(SIZESEQFM - 1) - (k - j)] = ((wrdsIn[m] & 0x0002) >> 1);
				k++;
			}

			i = lostSize+1;
			lostSize = 0;

			std::cout << ++frontCount << " -> " << chkWordSeq << std::endl << std::endl;
		}
		else {
			i = 0;
		}

		//ищем нужное слово, запускаем АМ
		if (chkWordSeq == findWords) {
				err = X502_OutCycleSetup(hnd, X502_OUT_CYCLE_FLAGS_FORCE);
				startAM = true;
			}

		//ищем первый ноль
		if (!findNull && !startAM) {
			for (; i < BUFSIZE; i++) {
				if ((wrdsIn[i] & 0x0001) == 0) {
					findNull = true;
					break;
				}
			}
		}

		//ищем перепад из 0 в 1
		if (findNull && !startAM) {
			for (j = i; j < BUFSIZE; j++) {
				if ((wrdsIn[j] & 0x0001) == 1) {
					findFront = true;
					break;
				}
			}
		} 
		//обрабатываем
		if (findFront && !startAM) {

			for (k = j;(k-j)< SIZESEQFM && (k<= BUFSIZE);k++) {
				chkWordSeq[(SIZESEQFM - 1) - (k-j)] = (wrdsIn[k] & 0x0002) >> 1;
			}
			//проверяем полное ли слово записано
			if ((k-j)!=31) {
				lostSize = SIZESEQFM-(k - j);
			}
			else {
				std::cout << ++frontCount << " -> " << chkWordSeq << std::endl << std::endl;
			}

			findNull = false;
			findFront = false;
		}

	}

	/*while (!_kbhit()) {

		std::cout << "\r";

		err = X502_Recv(hnd, wrdsIn, BUFSIZE, 1000);
		err = X502_ProcessData(hnd, wrdsIn, BUFSIZE, X502_PROC_FLAGS_VOLT, NULL, NULL, &nextWordAMfront, &wrdsOutSize);

		if ((prevWordAMfront & 0x0001)==0 && (nextWordAMfront & 0x0001)==1) {

			if (searchWordFound) {
				//Выводим импульс АМ на цифровой выход
				err = X502_OutCycleSetup(hnd, X502_OUT_CYCLE_FLAGS_FORCE);

				std::cout << "\n-----Required word found!\n";
				break;
			}

			chkWordSeq[SIZESEQFM] = (nextWordAMfront & 0x0002)>>1; //пишем слово, полученное одновременно с фронтом АМ
			//out << ++block << "\t\t" << ((nextWordAMfront & 0x0002) >> 1) << "\n";

			err = X502_Recv(hnd, wrdsIn, SIZESEQFM, 1000);
			err = X502_ProcessData(hnd, wrdsIn, SIZESEQFM, X502_PROC_FLAGS_VOLT, NULL, NULL, wrdsFMseq, &sizeFMseq);
						
			for (int i = 0; i < SIZESEQFM;i++) {
				chkWordSeq[(SIZESEQFM-1)-i] = (wrdsFMseq[i] & 0x0002)>>1;
			}

			std::cout<<++frontCount <<" -> "<< chkWordSeq<<std::endl;
			if (frontCount == 31) {
				frontCount = 0;
			}

			if (chkWordSeq == searchWord) { //ищем нужное слово в последовательности
				searchWordFound = true;
			}

			prevWordAMfront = nextWordAMfront;
		}
		else {
			prevWordAMfront = nextWordAMfront;
		}
	}*/

	system("pause");
	//освобождение модуля

	//out.close();

	X502_OutCycleStop(hnd, X502_OUT_CYCLE_FLAGS_FORCE);
	X502_StreamsStop(hnd);
	X502_Close(hnd);
	X502_Free(hnd);

	
	return 0;
}