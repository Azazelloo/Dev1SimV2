#include "Header.h"

void Start_DAC2_Single(t_l502_hnd& create, double* buf, int buf_size)
{

	//int err = 0;
	uint32_t* out_buf = new uint32_t[buf_size];

	/*----------*/
	L502_PrepareData(create, NULL, buf, NULL, buf_size, L502_DAC_FLAGS_VOLT, out_buf);
	L502_Send(create, out_buf, buf_size, 1000);
	/*---------*/
	//return err;
}