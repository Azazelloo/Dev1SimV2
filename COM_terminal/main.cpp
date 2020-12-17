#include "Header.h"

int main(int argc, char* argv[]) {

	HANDLE hSerial;
	char sPortName[32];
	sprintf(sPortName, "\\\\.\\COM%d", atoi(argv[1]));

	//______создание обработчика COM порта

	hSerial = CreateFile(sPortName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (hSerial == INVALID_HANDLE_VALUE){
		if (GetLastError() == ERROR_FILE_NOT_FOUND){
			cout << "serial port does not exist.\n";
			system("pause");
			return 0;
		}
	}
	//______записываем настройки COM порта
	int r=SetComPort(hSerial);

	if (*argv[2] == '-r' || *argv[2] == 'r') { //отправить команду на resert микроконтроллера
		Write2Bytes(hSerial, 0x8000); //0x8000 - включаем старший бит
	}else {
		//______отправка сообщений на порт (по 2 байта)

		unsigned short data = atoi(argv[2]);

		//______выставляем изначальный delay, ждем

		Write2Bytes(hSerial, data);
		system("pause");

		//______тестирование на 10А, имитация движения цели
		cout << "Working...Press any key for stop!\n";

		unsigned short counter = 0;
		unsigned short speed = atoi(argv[3]);

		if (data < speed) return 0;

		while (counter <= data) {
			Write2Bytes(hSerial, data - counter);
			counter += speed;
			Sleep(1000);

			if (_kbhit()) break;
		}
	}
	

	return 0;
}