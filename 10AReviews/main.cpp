#include "Header.h"

int main() {

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
			std::cout << "---Single exchange complete!" << std::endl;
		}
		else {
			std::cout << "---Single exchange error!" << std::endl;
		}
	}
	else if (mode==2) {
		while (!kbhit()) {
			if (!SingleExchange()) {
				std::cout << "---Continuous exchange complete! block: "<<++blockNum<<"\n";
				
				//_____выводим принятые 25 слов
				for (int i = 0; i < VALFORM2;i++) {
					std::cout<<i+1<<" -> "<< dataExchangeRet[i] << std:: endl;
				}

				//____анализ угла отклонения
				wA= (short)dataExchangeRet[1]*CMR; //угол отклонения луча антенны в горизонтальной плоскости
				wCK=(short)dataExchangeRet[2]*CMR; //угол сканирования

				//пишем логи углов
				//std::cout << "wA = " << wA << "\twCK = " << wCK << std::endl;
				//out<<std::fixed<<std::setprecision(5)<< blockNum << "\t\t" <<t.elapsed()<<"\t\t" << std::setprecision(5) << wA << "\t\t" << wCK << "\n";

				if (wA <= -39 && (!(dataExchange[3]&0x1000))) { //меняем направление движения антенны при достижении крайнего положения
					dataExchange[3] ^= 0x1000;
				}
				if (wA >= 39 && (dataExchange[3] & 0x1000)) { //меняем направление движения антенны при достижении крайнего положения
					dataExchange[3] ^= 0x1000;
				}
				/*/////////////////////////////*/

				//____проверяем наличие формуляров целей
				if (dataExchangeRet[10]) { //если есть формуляры
					for (int i = 0; i < dataExchangeRet[10]; i++) {
						err = OUtoKK(dataExchangeRet, 4, 3); //в режиме обзор 3 слова в формуляре
						formsReviews[i + 1] = std::vector<uint16_t>(dataExchangeRet, dataExchangeRet+3);
					}
					for (auto& form : formsReviews) {
						//std::cout << "\nForm" << form.first << " -> ";
						out << std::fixed << std::setprecision(5) << form.first << "\t\t";
						for (auto& words:form.second) {
							//std::cout << words << " ";
							out << std::fixed << std::setprecision(5) << words << "\t";
						}
						out << "\n";
					}
				}

				//system("pause");
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

	system("pause");
	return 0;
}