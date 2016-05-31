#ifndef TEST_VECTOR_AND_CU
#define TEST_VECTOR_AND_CU

#include "CUDA_header.cu"
#include "gpu_vectorAnd.cu"
#include "GPUMiner_Kernel.h"

void test_array_And_wS_woC()
{
	int in1Len = 2;
	int in2Len = in1Len*3;
	bytes_t* h_in1 = (bytes_t*)malloc( sizeof(bytes_t)*in1Len );
	bytes_t* h_in2 = (bytes_t*)malloc( sizeof(bytes_t)*in2Len );

	h_in1[0] = 2047;
	h_in1[1] = 65024;

	h_in2[0] = 65343;
	h_in2[1] = 32768;
	h_in2[2] = 0;
	h_in2[3] = 0;
	h_in2[4] = 65528;
	h_in2[5] = 256;	

	bytes_t* d_in1;
	bytes_t* d_in2;
	copyToGPU<bytes_t>( &d_in1, h_in1, in1Len );
	copyToGPU<bytes_t>( &d_in2, h_in2, in2Len );

	int listLen = 2;
	int* h_list = (int*)malloc( sizeof(int)*listLen );
	h_list[0] = 0;
	h_list[1] = 2;
	bytes_t* d_out;	
	int outLen = in1Len*listLen;
	GPUMALLOC( (void**)&d_out, sizeof(bytes_t)*outLen );
	
	arrayAndInit( listLen, in1Len );
	
	/*array_And_wS_woC( d_in1, in1Len, 
					   d_in2, h_list, listLen, 
					   d_out );*/
	table_t* d_countOut;
	GPUMALLOC( (void**)&d_countOut, sizeof(table_t)*outLen );
	array_And_woS_wC( d_in1, in1Len, 
					   d_in2, h_list, listLen, 
					   d_countOut ); 

	table_t* h_countOut;
	copyFromGPU( &h_countOut, d_countOut, outLen );
}


#endif