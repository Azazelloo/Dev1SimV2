#include "Header.h"

bool connect_L502(t_l502_hnd* create)
{
	int num_l502;
	uint32_t counter;
	char serials[5][32];
	L502_GetSerialList(serials, 2, NULL, &counter);
	if (counter!=0)
	{
		for (size_t i=0;i<counter;i++)
		{
			std::cout << serials[i] << "->[" << i << "]" << std::endl;
		}
		std::cout << "Enter number L502->";
		//std::cin >> num_l502;
		num_l502 = 0;

		if (num_l502>=0 && num_l502 <=9)
		{
			int err = L502_Open(*create, serials[num_l502]);
			if (err != 0)
			{
				std::cout << "----Error connection!" << std::endl;
				return false;
			}
			else
			{
				std::cout << "----Connection established!" << std::endl;
				return true;
			}
		}
		else
		{
			std::cout << "----Input  error!" << std::endl;
			return false;
		}

		
		
	}
	else
	{
		std::cout << "L502 not found!" << std::endl;
		return false;
	}
}
