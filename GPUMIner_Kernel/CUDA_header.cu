#ifndef _GPU_MINER_KERNEL_CU_
#define _GPU_MINER_KERNEL_CU_
#include "CUDA_header.cuh"



void startTimer(unsigned *timer)
{
	CUT_SAFE_CALL( cutCreateTimer( timer));
	CUT_SAFE_CALL( cutStartTimer( *timer));
}

double endTimer(char *info, unsigned *timer)
{
	cudaThreadSynchronize();
	CUT_SAFE_CALL( cutStopTimer( *timer));
	double result=cutGetTimerValue(*timer);
#ifdef PRINT_TIMER
	printf("***%s costs, %f, ms***\n", info, result);
#endif
	CUT_SAFE_CALL( cutDeleteTimer( *timer));
	return result/1000.0; //sec
}


#endif