#ifndef _TEST_MULTI_VECTOR_AND_CU_
#define _TEST_MULTI_VECTOR_AND_CU_

#include "stdlib.h"

#include "CUDA_header.cu"
#include "gpu_multiVectorAnd.cu"
#include "GPUMiner_Kernel.h"
#include "gpu_vectorAnd.cu"
#include "gather.cu"

/*
void multi_AND_wS_woC(bytes_t* d_in1, 
					  cons t int* d_in1List,
					  const int in1ListLen, 
					  const bytes_t* d_in2, 
					  const int* d_in2List, const int in2ListLen, 
					  const int* d_in2ListBound,
					  const int segLen, 
					  bytes_t* d_out,
					  const unsigned int numBlock = 256, 
					  const unsigned int numThread = 256
					  )

*/

void test_multi_AND_wS_woC1()
{
	int segLen = 2;

	//cpu
	bytes_t h_in1[] = { 24, 56, 78, 97, 102, 562 };
	bytes_t h_in2[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	int in1Len = segLen*3;
	int in2Len = segLen*4;
	int h_in1List[] = { 0, 2 };
	int in1ListLen = 2;
	int h_in2List[] = { 0, 3, 1, 2, 3 };
	int in2ListLen = 5;
	int h_in2ListBound[] = { 2, 3 };	

	//gpu
	bytes_t *d_in1, *d_in2;
	copyToGPU<bytes_t>( &d_in1, h_in1, in1Len );
	copyToGPU<bytes_t>( &d_in2, h_in2, in2Len );
	int *d_in1List, *d_in2List, *d_in2ListBound;
	copyToGPU<int>( &d_in1List, h_in1List, in1ListLen );
	copyToGPU<int>( &d_in2List, h_in2List, in2ListLen );
	copyToGPU<int>( &d_in2ListBound, h_in2ListBound, in1ListLen );
	bytes_t* d_out;
	int outLen = segLen*in2ListLen;
	GPUMALLOC( (void**)&d_out, sizeof(bytes_t)*outLen );

	multi_AND_wS_woC( d_in1, d_in1List, in1ListLen, d_in2, d_in2List, in2ListLen, d_in2ListBound, segLen, d_out );

	bytes_t* h_out;
	copyFromGPU<bytes_t>( &h_out, d_out, outLen );
}


void test_multi_AND_wS_woC2()
{
	unsigned int segLen = 10000; //100K/8
	unsigned int numSeg = 10;
	unsigned int inLen = segLen*numSeg;

	int numTest = 1;

	//cpu
	bytes_t* h_in1 = (bytes_t*)malloc( sizeof(bytes_t)*inLen );
	bytes_t* h_in2 = (bytes_t*)malloc( sizeof(bytes_t)*inLen );
	randArray<bytes_t>( h_in1, inLen, 0, 65535, 777 );
	randArray<bytes_t>( h_in2, inLen, 0, 65535, 999 );
	int in1ListLen = 20;
	int* h_in1List = (int*)malloc( sizeof(int)*in1ListLen );
	int* h_in2ListBound = (int*)malloc( sizeof(int)*in1ListLen );
	randArrayByOrder<int>( h_in1List, in1ListLen, 0, numSeg - 1, 0 );
	randArray<int>( h_in2ListBound, in1ListLen, 60, 80, 1 ); //each segment in in1 matchs 'low' to 'high' segments in in2
	int in2ListLen = 0;
	for( int i = 0; i < in1ListLen; i++ )
		in2ListLen += h_in2ListBound[i];
	int* h_in2List = (int*)malloc( sizeof(int)*in2ListLen );
	int offset = 0;
	for( int i = 0; i < in1ListLen; i++ )
	{
		randArrayByOrder<int>( h_in2List + offset, h_in2ListBound[i], 0, numSeg - 1, 2 );
		offset += h_in2ListBound[i];
	}
	
	//gpu
	int outLen = in2ListLen*segLen;
	bytes_t* d_in1;
	bytes_t* d_in2;
	copyToGPU<bytes_t>( &d_in1, h_in1, inLen );
	copyToGPU<bytes_t>( &d_in2, h_in2, inLen );
	int *d_in1List, *d_in2List, *d_in2ListBound;
	copyToGPU<int>( &d_in1List, h_in1List, in1ListLen );
	copyToGPU<int>( &d_in2List, h_in2List, in2ListLen );
	copyToGPU<int>( &d_in2ListBound, h_in2ListBound, in1ListLen );
	bytes_t* d_out;
	GPUMALLOC( (void**)&d_out, sizeof(bytes_t)*outLen );

	//run multiAnd wSwoC on the GPU
	printTitle( "start multiAND wSwoC on the GPU..." );
	unsigned int gpuTimer1 = 0;
	startTimer( &gpuTimer1 );
	for( int i = 0; i < numTest; i++ )
	{
		multi_AND_wS_woC( d_in1, d_in1List, in1ListLen, d_in2, d_in2List, in2ListLen, d_in2ListBound, segLen, d_out );
	}	
	SYNC();
	endTimer( "GPU multiple", &gpuTimer1 );
	bytes_t* h_out;
	copyFromGPU<bytes_t>( &h_out, d_out, outLen );

	//run singleAnd wSwoC on the GPU
	arrayAndInit( in2ListLen, segLen );
	bytes_t* d_goldOut;
	GPUMALLOC( (void**)&d_goldOut, sizeof(bytes_t)*outLen );
	printTitle( "start singleAND wSwoC on the GPU..." );
	unsigned int gpuTimer2 = 0;
	startTimer( &gpuTimer2 );
	for( int i = 0; i < numTest; i++ )
	{
		offset = 0;
		int outOffset = 0;
		for( int i = 0; i < in1ListLen; i++ )
		{
			array_And_wS_woC( d_in1 + segLen*h_in1List[i], segLen, 
						   d_in2, h_in2List + offset, h_in2ListBound[i], 
						   d_goldOut + outOffset );
			outOffset += ( segLen*h_in2ListBound[i] );
			offset +=  h_in2ListBound[i];
		}
	}
	SYNC();
	endTimer( "GPU single", &gpuTimer2 );
	bytes_t* h_goldOut;
	copyFromGPU<bytes_t>( &h_goldOut, d_goldOut, outLen );

	checkResult( h_out, h_goldOut, outLen, "AND" );
}


void test_multi_AND_woS_wC2()
{
	unsigned int segLen = 10000; //100K/8
	unsigned int numSeg = 10;
	unsigned int inLen = segLen*numSeg;

	int numTest = 1;

	//cpu
	bytes_t* h_in1 = (bytes_t*)malloc( sizeof(bytes_t)*inLen );
	bytes_t* h_in2 = (bytes_t*)malloc( sizeof(bytes_t)*inLen );
	randArray<bytes_t>( h_in1, inLen, 0, 65535, 777 );
	randArray<bytes_t>( h_in2, inLen, 0, 65535, 999 );
	int in1ListLen = 20;
	int* h_in1List = (int*)malloc( sizeof(int)*in1ListLen );
	int* h_in2ListBound = (int*)malloc( sizeof(int)*in1ListLen );
	randArrayByOrder<int>( h_in1List, in1ListLen, 0, numSeg - 1, 0 );
	randArray<int>( h_in2ListBound, in1ListLen, 60, 80, 1 ); //each segment in in1 matchs 'low' to 'high' segments in in2
	int in2ListLen = 0;
	for( int i = 0; i < in1ListLen; i++ )
		in2ListLen += h_in2ListBound[i];
	int* h_in2List = (int*)malloc( sizeof(int)*in2ListLen );
	int offset = 0;
	for( int i = 0; i < in1ListLen; i++ )
	{
		randArrayByOrder<int>( h_in2List + offset, h_in2ListBound[i], 0, numSeg - 1, 2 );
		offset += h_in2ListBound[i];
	}

	printf( "in2ListLen = %d\n", in2ListLen );

	arrayAndInit( in2ListLen, segLen );
	
	//gpu
	int outLen = in2ListLen*segLen;
	bytes_t* d_in1;
	bytes_t* d_in2;
	copyToGPU<bytes_t>( &d_in1, h_in1, inLen );
	copyToGPU<bytes_t>( &d_in2, h_in2, inLen );
	int *d_in1List, *d_in2List, *d_in2ListBound;
	copyToGPU<int>( &d_in1List, h_in1List, in1ListLen );
	copyToGPU<int>( &d_in2List, h_in2List, in2ListLen );
	copyToGPU<int>( &d_in2ListBound, h_in2ListBound, in1ListLen );
	table_t* d_countOut;
	GPUMALLOC( (void**)&d_countOut, sizeof(table_t)*outLen );
	CUDA_SAFE_CALL( cudaMemset( d_countOut, 0, sizeof(table_t)*outLen ) );

	//run multiAnd wSwoC on the GPU
	//printTitle( "start multiAND woSwC on the GPU..." );
	unsigned int gpuTimer1 = 0;
	startTimer( &gpuTimer1 );
		for( int i = 0; i < numTest; i++ )
		{
			multi_AND_woS_wC( d_in1, d_in1List, in1ListLen, d_in2, d_in2List, in2ListLen, d_in2ListBound, segLen, d_countOut );
		}	
		table_t* h_countOut;
		copyFromGPU<table_t>( &h_countOut, d_countOut, outLen );
		SYNC();
	endTimer( "GPU multiple", &gpuTimer1 );

	//run singleAnd wSwoC on the GPU
	table_t* d_goldCountOut;
	GPUMALLOC( (void**)&d_goldCountOut, sizeof(table_t)*outLen );
	//printTitle( "start singleAND woSwC on the GPU..." );
	table_t* h_goldCountOut;
	unsigned int gpuTimer2 = 0;
	startTimer( &gpuTimer2 );
	for( int i = 0; i < numTest; i++ )
	{
		offset = 0;
		int outOffset = 0;
		for( int i = 0; i < in1ListLen; i++ )
		{
			array_And_woS_wC( d_in1 + segLen*h_in1List[i], segLen, 
						   d_in2, h_in2List + offset, h_in2ListBound[i], 
						   d_goldCountOut + outOffset );
			outOffset += ( segLen*h_in2ListBound[i] );
			offset +=  h_in2ListBound[i];
		}		
		copyFromGPU<table_t>( &h_goldCountOut, d_goldCountOut, outLen );
	}
	SYNC();
	endTimer( "GPU single", &gpuTimer2 );

	//count on the CPU
	//run multiAnd wSwoC on the GPU
	/*
	bytes_t* d_out;
	table_t* d_cOut;
	GPUMALLOC( (void**)&d_cOut, sizeof(table_t)*outLen );
	GPUMALLOC( (void**)&d_out, sizeof(bytes_t)*outLen );
	//printTitle( "start multiAND wSwoC on the GPU..." );	
	table_t* cpu_out = (table_t*)malloc( sizeof(table_t)*outLen );
	unsigned int gpuTimer3 = 0;
	startTimer( &gpuTimer3 );
		for( int i = 0; i < numTest; i++ )
		{
			multi_AND_wS_woC( d_in1, d_in1List, in1ListLen, d_in2, d_in2List, in2ListLen, d_in2ListBound, segLen, d_out );
			
			//bytes_t* h_out;
			//copyFromGPU<bytes_t>( &h_out, d_out, outLen );
			//for( int i = 0; i < outLen; i++ )
			//	cpu_out[i] = h_byteTable[h_out[i]];
			

			gahter<table_t, table_t, bytes_t>( g_byteTable, outLen, d_cOut, d_out, outLen );			
			copyFromGPU<table_t>( &cpu_out, d_cOut, outLen );
		}	
		SYNC();
	endTimer( "GPU multiple", &gpuTimer3 );
	*/

	checkResult( h_countOut, h_goldCountOut, outLen, "AND" );
	//checkResult( h_goldCountOut, cpu_out, outLen, "AND" );
}

#endif