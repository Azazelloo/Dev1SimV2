#include "Header.h"

int main() {

	//_______��������������� ��������� Tmk
	/**/
	int err = 0;
	
	err = Init10A(); //������������� 10�

	//��������� �� ����� ������ ��� ��������
	std::ifstream in("words.txt"); //���� ������� �� ������� ���������� ������ 0
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


	addrOU = 12; // �������� ������������� (����� ��� ������ ����������� ��������� �� ��������� �������������)

	//����� ������ ������ �������
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
				
				//_____������� �������� 25 ����
				for (int i = 0; i < VALFORM2;i++) {
					std::cout<<i+1<<" -> "<< dataExchangeRet[i] << std:: endl;
				}

				//____������ ���� ����������
				wA= (short)dataExchangeRet[1]*CMR; //���� ���������� ���� ������� � �������������� ���������
				wCK=(short)dataExchangeRet[2]*CMR; //���� ������������

				//����� ���� �����
				//std::cout << "wA = " << wA << "\twCK = " << wCK << std::endl;
				//out<<std::fixed<<std::setprecision(5)<< blockNum << "\t\t" <<t.elapsed()<<"\t\t" << std::setprecision(5) << wA << "\t\t" << wCK << "\n";

				if (wA <= -39 && (!(dataExchange[3]&0x1000))) { //������ ����������� �������� ������� ��� ���������� �������� ���������
					dataExchange[3] ^= 0x1000;
				}
				if (wA >= 39 && (dataExchange[3] & 0x1000)) { //������ ����������� �������� ������� ��� ���������� �������� ���������
					dataExchange[3] ^= 0x1000;
				}
				/*/////////////////////////////*/

				//____��������� ������� ���������� �����
				if (dataExchangeRet[10]) { //���� ���� ���������
					for (int i = 0; i < dataExchangeRet[10]; i++) {
						err = OUtoKK(dataExchangeRet, 4, 3); //� ������ ����� 3 ����� � ���������
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