#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include "windows.h"
#include "conio.h"
#include <iostream>

using namespace std;

struct Settings {
	DWORD BaudRate = 9600;
	BYTE ByteSize = 8;
	BYTE StopBits = ONESTOPBIT;
	BYTE Parity = NOPARITY;
}ComSet;


int SetComPort(HANDLE& hSerial) {

	int err = 0;
	DCB dcbSerialParams = { 0 };
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	err = GetCommState(hSerial, &dcbSerialParams);
	if (!err) {
		cout << "-----Getting state error\n";
		return err;
	}

	dcbSerialParams.BaudRate = ComSet.BaudRate;
	dcbSerialParams.ByteSize = ComSet.ByteSize;
	dcbSerialParams.StopBits = ComSet.StopBits;
	dcbSerialParams.Parity = ComSet.Parity;

	err = SetCommState(hSerial, &dcbSerialParams);
	if (!err) {
		cout << "error setting serial port state\n";
		return err;
	}

	return err;
}

DWORD Write2Bytes(HANDLE& hSerial, unsigned short data){

	DWORD dwBytesWritten;
	WriteFile(hSerial, &data, sizeof(data) / 2, &dwBytesWritten, NULL); //пишем первый байт data
	data = data >> 8; //достаем второй байт
	WriteFile(hSerial, &data, sizeof(data) / 2, &dwBytesWritten, NULL);//пишем второй байт data

	return dwBytesWritten;
}