#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "../inc/l502api.h"
#include <iostream>
#include <chrono>
#include <iterator>
#include <fstream>
#include <conio.h>
#include <mutex>

#include<C:/boost_1_72_0/boost/array.hpp>
#include <C:/boost_1_72_0/boost/thread.hpp>
//#include <C:/boost_1_72_0/boost/lockfree/queue.hpp>
using namespace std;

#define	  stop __asm nop
#define M_PI 3.14159265358979323846

#define ADC_ARRAY_SIZE 150000 //4096*200 //в примере
#define F_ACQ 1000
//#define F_ACQ 1/(FREQ*ADC_ARRAY_SIZE)
//static double F_ACQ = 1;

bool connect_L502(t_l502_hnd* create);
int Start_DAC2_Cycle(t_l502_hnd& create,double* buf, int buf_size);
void Start_DAC2_Single(t_l502_hnd& create, double* buf, int buf_size);
int Get_ADC_1chan(t_l502_hnd& create, double* adc_out_buf, uint32_t adc_out_buf_size);
int SetDev(t_l502_hnd& create);

 

 