#include "Header.h"
#define L502_BF_CMD_CODE_USER   0x8000U


int main(int argc, char** argv){

		setlocale(LC_CTYPE, "");

		int err = 0;
		uint32_t errors[2];
		uint32_t testData[1000];

		//_____подключаемся и настраиваем
		char serials[maxCountDev][X502_SERIAL_SIZE];

		err = E502_UsbGetSerialList(serials, maxCountDev, NULL, NULL); //получаем список доступных устройств
		t_x502_hnd hnd = X502_Create(); //создаем описатель
		err = E502_OpenUsb(hnd, serials[0]);

		if (err != X502_ERR_OK)
			cout << "----Error connection E502!\n";
		else
			cout << "----E502 connected!\n";
		
		const char* ldrPath = "C://Users//Azazello//Desktop//l502-bf-test//vdsp//Release//l502-bf.ldr";

		//______загружаем прошивку 

		while ((X502_BfLoadFirmware(hnd, ldrPath)!= X502_ERR_OK)) {
			cout << "-----waiting for bf load\n";
			Sleep(3000);
		}

		if (err != X502_ERR_OK) {
			cout << "----Blackfin load error: " << err << std::endl;
		}	
		else {
			cout << "----Blackfin loaded!\n";
		}
			

		//_______Инициализация BlackFin
		if (err == X502_ERR_OK) {

			err = X502_BfExecCmd(hnd, L502_BF_CMD_CODE_USER + 1, 0, NULL, 0, &errors[0],
				sizeof(errors) / sizeof(errors[0]), 1000, NULL);


			err = X502_BfExecCmd(hnd, L502_BF_CMD_CODE_USER + 3, 0, NULL, 0, testData,
				1000, 1000, NULL);

			//Sleep(1000);
			uint32_t word = 0x00000000;
			uint32_t regs[1];
			err = X502_BfMemRead(hnd, word, regs, 1);
			stop

			err = X502_BfExecCmd(hnd, L502_BF_CMD_CODE_USER + 2, 0, NULL, 0, testData,
				1, 1000, NULL);
		}
		else {
			cout << "----Initialization error!\n";
			//system("pause");
		}

		err=X502_SetMode(hnd,0x0000);

		//______освобождаем модуль
		X502_Close(hnd);
		X502_Free(hnd);

		return 0;
	}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           