#include "Header.h"

int main(int argc, char *argv[]) {

	//_______Предварительная настройка Tmk
	/**/
	int err = 0;
	
	err = Init10A(); //инициализация 10А

	//считываем из файла массив для отправки
	std::ifstream in("words.txt"); //если считать не удастся отправится массив 0
	if (in.is_open()) {
		std::cout << "---Read words from txt complete!" << std::endl;
		for (int i = 0; i < VALFORM1; i++) {
			in >>std::hex>> dataExchange[i];
		}
	}
	else {
		std::cout << "---Read words from txt not complete!" << std::endl;
	}
	in.close();


	addrOU = 12; // заглушка инициализации (чтобы при каждом перезапуске программы не проводить инициализацию)

	//Выбор режима обмена данными
	size_t mode = 0,blockNum=0;
	double wA = 0, wCK = 0;

	std::ofstream out;
	out.open("disp.txt");
	//out << "Num\t\ttime\t\twA\t\twCK\n";

	std::cout << "Select exchange mode: \n1 - single mode\n2 - continuous mode\nSelect: ";
	std::cin >> mode;

	Timer t;

	if (mode == 1) {
		if (!SingleExchange()) {
			std::cout << "---Single exchange complete!\n" << std::endl;
			//_____выводим принятые 25 слов
			for (int i = 0; i < VALFORM2; i++) {
				std::cout << i + 1 << " -> " << dataExchangeRet[i] << std::endl;
			}
		}
		else {
			std::cout << "---Single exchange error!" << std::endl;
		}
	}
	else if (mode==2) {

		//_______инициализация приводов
		int p_countReviews = atoi(argv[1]);
		uint16_t tmpRK4 = dataExchange[3];
		dataExchange[3] ^= 0xA800; //оставляем только привод и порог1
		SingleExchange();

		dataExchange[3] = tmpRK4;
		system("pause"); // пауза для ручной синхронизации

		while (revCounter< p_countReviews) {
			if (!SingleExchange()) {
				//std::cout << "---Continuous exchange complete! review: "<< revCounter <<"\r";

				//____анализ угла отклонения
				wA= (short)dataExchangeRet[1]*CMR; //угол отклонения луча антенны в горизонтальной плоскости
				wCK=(short)dataExchangeRet[2]*CMR; //угол сканирования

				std::cout << "---wA: " << wA << "\r";
				//пишем логи углов
				//std::cout << "wA = " << wA << "\twCK = " << wCK << std::endl;
				//out<<std::fixed<<std::setprecision(5)<< blockNum << "\t\t" <<t.elapsed()<<"\t\t" << std::setprecision(5) << wA << "\t\t" << wCK << "\n";

				if (wA <= -39 && (!(dataExchange[3] & 0x1000))) { //меняем направление движения антенны при достижении крайнего положения
					out << "Review: "<<revCounter<<"\n";
					++revCounter;
					GetReview(out); 
					dataExchange[3] ^= 0x1000;
				}
				else if (wA >= 39 && (dataExchange[3] & 0x1000)) { //меняем направление движения антенны при достижении крайнего положения
					out << "Review: " << revCounter << "\n";
					++revCounter;
					GetReview(out);
					dataExchange[3] ^= 0x1000;
				}
				/*/////////////////////////////*/

				

			}
			else {
				std::cout << "---Continuous exchange error!" << std::endl;
			}
		}
	}
	else {
		std::cout << "---Error input mode!" << std::endl;
	}
	std::cout << "---Continuous exchange complete! block: " << ++blockNum << std::endl;
	stop

	out.close();
	tmkdone(ALL_TMKS);
	TmkClose();
	CloseHandle(hBcEvent);

	//system("pause");
	return 0;
}