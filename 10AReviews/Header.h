#pragma once

#include <iostream>
#include <thread>
#include <chrono>
#include "windows.h"
#include <conio.h>
#include <fstream>
#include <iomanip>
#include <map>
#include <vector>

#include "Timer.h"

#include "WDMTMKv2.cpp"

#define	  stop __asm nop
#define GoodRN10H 0x001E
#define GoodRNVULC 0xF000
#define CMR 0.01171875
#define VALFORM1 17		//количество слов отправляемых в прибор (согласно протоколу)
#define VALFORM2 25		//количество слов принимаемых из прибора (согласно протоколу)
#define kvant 300


using namespace std::chrono_literals;

HANDLE hBcEvent; //обработчик событий
TTmkEventData tmkEvD;

size_t bcnum = 0, //база
	   addrOU = 1; //контроллер канала

uint16_t dataExchange[VALFORM1] = { 0 };
uint16_t dataExchangeRet[VALFORM2] = { 0 };
std::map<int,std::vector<uint16_t>> formsReviews;

uint16_t freqWord = 0x0FFF; //частота
size_t revCounter = 0;


int Init10A();
int OUtoKK(uint16_t* word, unsigned short subAddr, unsigned short numWords, unsigned short startWord);
int KKtoOU(uint16_t* word, unsigned short subAddr, unsigned short numWords);
int SingleExchange();


//______инициализация 10А
int Init10A() {
	int err = 0;

	if (!TmkOpen()) {
		std::cout << "---TmK is open!" << std::endl;
	}
	else {
		std::cout << "---Error: TmK is not open!" << std::endl;
	}

	err = tmkconfig(bcnum);

	err = bcreset(); // обнуляем биты

	hBcEvent = CreateEvent(NULL, TRUE, FALSE, NULL);//создаем событие для прерываний
	tmkdefevent(hBcEvent, TRUE); //устанавливаем событие для прерываний
	ResetEvent(hBcEvent);
	err = bcdefirqmode(0x00000000); //все прерывания разрешены

	if (!err) {
		std::cout << "---Setting complete!" << std::endl;
	}
	else {
		std::cout << "---Error: setting not complete!" << std::endl;
	}

	//______Инициализация 10А
	uint16_t sendRcv[16] = { 0 };
	err = OUtoKK(sendRcv, 5, 1,2); //проверка экспресс диагностики
								   //начинаем читать со 2 слова
	if (!err && sendRcv[0] == 1) {
		std::cout << "---Express diagnostic complete!" << std::endl;
	}
	else {
		std::cout << "---Error: express diagnostic not complete!" << std::endl;
	}

	//команда на инициализацию
	sendRcv[0] = 0x01F5;
	err = KKtoOU(sendRcv, 4, 1);
	std::cout << "Waiting initialization..." << std::endl;
	std::this_thread::sleep_for(4s);//ожидание после инициализации не менее 3х секунд

	if (!err) {
		addrOU = 12; //согласно протоколу, адрес прибора меняется на 14(в восьмеричной системе)
		for (int i = 0; i < 5; i++) {
			OUtoKK(sendRcv, 2, 2,2); //начинаем читать со 2 слова
			if ((sendRcv[0] & GoodRN10H) && (sendRcv[1] & GoodRNVULC)) {
				std::cout << "---Initialization complete!" << std::endl;
				break;
			}
			else {
				std::cout << "---Error: initialization not complete!" << std::endl;
			}
		}
	}
	else {
		std::cout << "---Error: initialization not complete!" << std::endl;
	}

	return err;
}

//_____отправляем массив на манчестер
int OUtoKK(uint16_t* word, unsigned short subAddr, unsigned short numWords, unsigned short startWord) {
	bcdefbase(0);
	bcputw(0, CW(addrOU, RT_TRANSMIT, subAddr, numWords));
	bcstart(0, DATA_RT_BC);

	WaitForSingleObject(hBcEvent, INFINITE); //ждем прерывания
	bcgetblk(startWord, word, numWords); // читаем со второго слова
	ResetEvent(hBcEvent);// обновляем событие

	tmkgetevd(&tmkEvD);
	if (!tmkEvD.bc.wResult) {
		return 0;
	}

	return 1;
}

//_____принимаем массив с манчестера
int KKtoOU(uint16_t* word, unsigned short subAddr, unsigned short numWords) {
	bcdefbase(0);
	bcputw(0, CW(addrOU, RT_RECEIVE, subAddr, numWords));
	bcputblk(1, word, numWords);
	bcstart(0, DATA_BC_RT);

	WaitForSingleObject(hBcEvent, INFINITE); //ждем прерывания
	ResetEvent(hBcEvent);

	tmkgetevd(&tmkEvD);
	if (!tmkEvD.bc.wResult) {
		return 0;
	}

	return 1;
}

//_____одиночный обмен
int SingleExchange() {
	int err = 0;
	err = KKtoOU(dataExchange, 2, VALFORM1); //записываем 17 слов во второй подадрес

	err = KKtoOU(&freqWord, 1, 1); //передаем сигнал тактовой частоты

	err = OUtoKK(dataExchangeRet, 3, VALFORM2,2); //принимаем 25 слов с третьего подадреса
												  //начинаем читать со 2 слова
	return err;
}

//_____логируем результаты обзоров
bool GetReview(std::ofstream& out) {

	int err = 0;
	uint16_t startWord = 2;//начинаем чтение формуляров из 4 подадреса со второго слова

	//____проверяем наличие формуляров целей
	if (dataExchangeRet[10]) { //если есть формуляры
		for (int i = 0; i < dataExchangeRet[10]; i++) {
			err = OUtoKK(dataExchangeRet, 4, 3, startWord); //в режиме обзор 3 слова в формуляре
			formsReviews[i + 1] = std::vector<uint16_t>(dataExchangeRet, dataExchangeRet + 3);
			startWord += 3;
		}

		for (auto& form : formsReviews) {
			out << std::setw(5) << form.first << "\t\t";

			//______в обзоре всегда три слова (дальность, угол начала, угол конца)
			out << std::setw(10)<<((form.second[0]) >> 4)*kvant + 0 << "\t"; //дальность -> сдвинутое слово умножаем на квант + дальность начала зоны обнаружения
			out << std::setw(10)<<(short)form.second[1] * CMR / 2 << "\t"; //угол начала
			out << std::setw(10)<<(short)form.second[2] * CMR / 2 << "\t"; //угол конца
			out << "\n";
		}
		out << "\n";

		return true;
	}
	return false;
}