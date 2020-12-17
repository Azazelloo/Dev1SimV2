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
#define VALFORM1 17		//���������� ���� ������������ � ������ (�������� ���������)
#define VALFORM2 25		//���������� ���� ����������� �� ������� (�������� ���������)


using namespace std::chrono_literals;

HANDLE hBcEvent; //���������� �������
TTmkEventData tmkEvD;

size_t bcnum = 0, //����
	   addrOU = 1; //���������� ������

uint16_t dataExchange[VALFORM1] = { 0 };
uint16_t dataExchangeRet[VALFORM2] = { 0 };
std::map<int,std::vector<uint16_t>> formsReviews;

uint16_t freqWord = 0x0FFF; //�������


int Init10A();
int OUtoKK(uint16_t* word, size_t subAddr, size_t numWords);
int KKtoOU(uint16_t* word, size_t subAddr, size_t numWords);
int SingleExchange();


//______������������� 10�
int Init10A() {
	int err = 0;

	if (!TmkOpen()) {
		std::cout << "---TmK is open!" << std::endl;
	}
	else {
		std::cout << "---Error: TmK is not open!" << std::endl;
	}

	err = tmkconfig(bcnum);

	err = bcreset(); // �������� ����

	hBcEvent = CreateEvent(NULL, TRUE, FALSE, NULL);//������� ������� ��� ����������
	tmkdefevent(hBcEvent, TRUE); //������������� ������� ��� ����������
	ResetEvent(hBcEvent);
	err = bcdefirqmode(0x00000000); //��� ���������� ���������

	if (!err) {
		std::cout << "---Setting complete!" << std::endl;
	}
	else {
		std::cout << "---Error: setting not complete!" << std::endl;
	}

	//______������������� 10�
	uint16_t sendRcv[16] = { 0 };
	err = OUtoKK(sendRcv, 5, 1); //�������� �������� �����������
	if (!err && sendRcv[0] == 1) {
		std::cout << "---Express diagnostic complete!" << std::endl;
	}
	else {
		std::cout << "---Error: express diagnostic not complete!" << std::endl;
	}

	//������� �� �������������
	sendRcv[0] = 0x01F5;
	err = KKtoOU(sendRcv, 4, 1);
	std::cout << "Waiting initialization..." << std::endl;
	std::this_thread::sleep_for(4s);//�������� ����� ������������� �� ����� 3� ������

	if (!err) {
		addrOU = 12; //�������� ���������, ����� ������� �������� �� 14(� ������������ �������)
		for (int i = 0; i < 5; i++) {
			OUtoKK(sendRcv, 2, 2);
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

//_____���������� ������ �� ���������
int OUtoKK(uint16_t* word, size_t subAddr, size_t numWords) {
	bcdefbase(0);
	bcputw(0, CW(addrOU, RT_TRANSMIT, subAddr, numWords));
	bcstart(0, DATA_RT_BC);

	WaitForSingleObject(hBcEvent, INFINITE); //���� ����������
	bcgetblk(2, word, numWords); // ������ �� ������� �����
	ResetEvent(hBcEvent);// ��������� �������

	tmkgetevd(&tmkEvD);
	if (!tmkEvD.bc.wResult) {
		return 0;
	}

	return 1;
}

//_____��������� ������ � ����������
int KKtoOU(uint16_t* word, size_t subAddr, size_t numWords) {
	bcdefbase(0);
	bcputw(0, CW(addrOU, RT_RECEIVE, subAddr, numWords));
	bcputblk(1, word, numWords);
	bcstart(0, DATA_BC_RT);

	WaitForSingleObject(hBcEvent, INFINITE); //���� ����������
	ResetEvent(hBcEvent);

	tmkgetevd(&tmkEvD);
	if (!tmkEvD.bc.wResult) {
		return 0;
	}

	return 1;
}

//_____��������� �����
int SingleExchange() {
	int err = 0;
	err = KKtoOU(dataExchange, 2, VALFORM1); //���������� 17 ���� �� ������ ��������

	err = KKtoOU(&freqWord, 1, 1); //�������� ������ �������� �������

	err = OUtoKK(dataExchangeRet, 3, VALFORM2); //��������� 25 ���� � �������� ���������

	return err;
}