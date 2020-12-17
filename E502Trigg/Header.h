#pragma once
#include "inc/l502api.h"
#include "inc/e502api.h"
#include "inc/x502api.h"

#include <iostream>
#include  <conio.h>
#include <fstream>
#include <bitset>
#include <vector>
#include <Windows.h>
#include <algorithm>
#include <iterator>


#define stop __asm nop

#define maxCountDev 5
#define BUFSIZE 500
#define SIZESEQFM 31 //для сигнала2 30
//последовательность 31 бит,
//берем на одно меньше т.к первое слово приходит одновременно
//с фронтом АМ

#define SizeAMpulse (1056)-56 // вычитанием точек пытаемся примерно
//соотнести скорость работы программы со скоростью 10А
#define AMpulseWidth 62
