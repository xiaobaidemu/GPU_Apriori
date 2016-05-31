#ifndef _GPU_MINER_KERNEL_CUH_
#define _GPU_MINER_KERNEL_CUH_


//#include "gpu_multiVectorAnd.cu"
#include "gpu_vectorAndInit.cuh"
#include "CUDA_header.cuh"

////////////////////////////////////// Global Variable /////////////////////////
double copyTime = 0.0;
double countTime = 0.0;
double kernelTime = 0.0;

static __device__ __constant__ table_t c_byteTable[TABLE_SIZE];//�����ڴ治��cudaMalloc
int* d_id2List;
bytes_t* d_buf;
table_t* g_byteTable;
int* d_in1List;
int* d_in2List;
int* d_in2ListBound;



__global__
void multi_AND_wS_woC_kernel(bytes_t* d_in1, 
							 const int* d_in1List,
							 const int in1ListLen, 
							 const bytes_t* d_in2, 
							 const int* d_in2List, const int in2ListLen, 
							 const int* d_in2ListBound,
							 const int segLen, 
							 bytes_t* d_out)
{
	//1. load the d_in1List, d_in2List and d_in2ListBound to the shared memory
	extern __shared__ int s_data[];
	int* s_in1List = s_data; //len = in1ListLen	
	int* s_in2ListBound = s_data + in1ListLen; //len = in1ListLen
	int* s_in2List = s_data + 2*in1ListLen; //len = in2ListLen
	//�о�������Բ�ʹ��threadIdx.x
	for( int i = threadIdx.x; i < in1ListLen; i += blockDim.x )
	{
		s_in1List[i] = d_in1List[i];
		s_in2ListBound[i] = d_in2ListBound[i];
	}
	for( int i = threadIdx.x; i < in2ListLen; i += blockDim.x )
	{
		//s_in2List��ÿһ���¼����һ��itemidֵ
		s_in2List[i] = d_in2List[i];
	}
	__syncthreads();

	//2.
	int outOffset = 0; //for output, also for get the d_segIn2List
	bytes_t* d_segIn1;//pointer to d_in1 for current segment
	int* d_segIn2List; //pointer to d_in2List for current segment
	int segIn2ListLen;
	int segOutLen = 0;
	const unsigned int index = blockIdx.x*blockDim.x + threadIdx.x;
	const unsigned int delta = blockDim.x*gridDim.x;
	int idid2, offset, posInIn2;
	for( int id1 = 0; id1 < in1ListLen; id1++ )
	{
		//d_segIn1 ָ�����d_in1��ǰҪ�����Ƭ�Σ�����ǰ���ڵ�Ƭ�����е������01ֵ
		d_segIn1 = d_in1 + segLen*(s_in1List[id1]);
		//�������д��룬��ʾ��ѭ���������ڵ�Ϊid1���ӽڵ㣬������ͨ��GPUͬʱ����s_in2ListBound[id1]����Լ��200-400���ң��Ը��ӽڵ��������,ÿ���̴߳���16λ
		//���ݵ�������
		d_segIn2List = s_in2List + outOffset;		
		segIn2ListLen = s_in2ListBound[id1];

		segOutLen = segLen*segIn2ListLen;//segIn2ListLen��С��1000��ֵ
		//for each segment,ÿ���̴߳���16λ������������		
		//index��ȡֵ��Χ��0-1536��deltaΪ1536����ǰ��Ҫ����segOutLen��16λ���ݣ���ΪͬʱҪ����s_in2ListBound[id1]�Ը��ӽڵ�������㣬���Դ��������Ƚϸ���
		for( int i = index; i < segOutLen; i += delta )
		{
			//idid2 ��ʾ��ǰ����߳�������һ���ӽڵ�
			idid2 = i/segLen;
			//offset��d_segIn1��d_in2����ͬ���� offsetd��ȡֵ��Χ��0-(segLen-1),��֤�����������������
			offset = i - idid2*segLen;
			//posInIn2 ��ʾ�ڵ�ǰ���ڵ��£���Ҫ������ӽڵ��16λ���ݵ����������ֵҪ�Ͷ�Ӧ�ĸ��ڵ����Ӧ������d_segIn2List[idid2]��¼�����ӽڵ��Ӧ��item��id��ֵ
			//offsetҪ��֤������������ĸ��ӽڵ��ƫ��Ҫһ��
			posInIn2 = d_segIn2List[idid2]*segLen + offset;
			//d_out ����м�������������2*segLen*1500*15,(1500Ӧ�ú�1000���й�ϵ),15����˼�����level��ֵ������15��������Ҫ���levelPos�������
			//segOutLen < d_out��������
			//�����ȡd_out���Է��ڳ����洢����
			//���������ֵ����d_out�У�outOffset*segLen��Ϊ��ȷ����ǰ���ڵ���������ȫ���ӽڵ����м�����λ��
			d_out[i + outOffset*segLen] = d_segIn1[offset]&d_in2[posInIn2];
		}
		outOffset += (s_in2ListBound[id1]);
	}
}


__global__
void multi_AND_woS_wC_kernel(bytes_t* d_in1, 
							 const int* d_in1List,
							 const int in1ListLen, 
							 const bytes_t* d_in2, 
							 const int* d_in2List, const int in2ListLen, 
							 const int* d_in2ListBound,
							 const int segLen, 
							 table_t* d_countOut )
{
	//1. load the d_in1List, d_in2List and d_in2ListBound to the shared memory
	extern __shared__ int s_data[];
	int* s_in1List = s_data; //len = in1ListLen	
	int* s_in2ListBound = s_data + in1ListLen; //len = in1ListLen
	int* s_in2List = s_data + 2*in1ListLen; //len = in2ListLen

	for( int i = threadIdx.x; i < in1ListLen; i += blockDim.x )
	{
		s_in1List[i] = d_in1List[i];
		s_in2ListBound[i] = d_in2ListBound[i];
	}
	for( int i = threadIdx.x; i < in2ListLen; i += blockDim.x )
	{
		s_in2List[i] = d_in2List[i];
	}
	__syncthreads();

	//2.
	int outOffset = 0; //for output, also for get the d_segIn2List
	bytes_t* d_segIn1;//pointer to d_in1 for current segment
	int* d_segIn2List; //pointer to d_in2List for current segment
	int segIn2ListLen;
	int segOutLen = 0;
	const unsigned int index = blockIdx.x*blockDim.x + threadIdx.x;
	const unsigned int delta = blockDim.x*gridDim.x;
	int idid2, offset, posInIn2;
	bytes_t out;
	for( int id1 = 0; id1 < in1ListLen; id1++ )
	{
		d_segIn1 = d_in1 + segLen*(s_in1List[id1]);
		d_segIn2List = s_in2List + outOffset;		
		segIn2ListLen = s_in2ListBound[id1];
		segOutLen = segLen*segIn2ListLen;
		//for each segment		
		for( int i = index; i < segOutLen; i += delta )
		{
			idid2 = i/segLen;
			//offset��d_segIn1��d_in2����ͬ���� offsetd��ȡֵ��Χ��0-(segLen-1),��֤�����������������
			offset = i - idid2*segLen;
			posInIn2 = d_segIn2List[idid2]*segLen + offset;
			out = d_segIn1[offset]&d_in2[posInIn2];
			//�����㵽����levelʱ��ֱ�Ӷ����ս����ֵ�������ж��ٸ�1
			d_countOut[i + outOffset*segLen] = c_byteTable[out];
			//d_out[i + outOffset*segLen] = d_segIn1[offset]&d_in2[posInIn2];
		}

		outOffset += (s_in2ListBound[id1]);
	}
}


////////////////////////////////////////////////////////////////////////////////
//! GPU primitive for vector and operation for multiple d_in1 and multiple d_in2
//! @param d_in1
//! @param d_in1List list contains the id in d_in1
//! @param in1ListLen
//! @param d_in2
//! @param d_in2List list contains the id in d_in2
//! @param in2ListLen in2ListLen >= in1ListLen
//! @param d_in2ListBoud number of corresponding ids in d_in2 for each id in d_in1, len = in1ListLen
//! @param segLen size of each segment for AND operation
//! @param d_out, outLen = in2ListLen*segLen
////////////////////////////////////////////////////////////////////////////////
void multi_AND_wS_woC(bytes_t* d_in1, 
					  const int* d_in1List,
					  const int in1ListLen, 
					  const bytes_t* d_in2, 
					  const int* d_in2List, const int in2ListLen, 
					  const int* d_in2ListBound,
					  const int segLen, 
					  bytes_t* d_out,
					  const unsigned int numBlock = 8, 
					  const unsigned int numThread = 192
					  )
{
	//store the d_in1List, d_in2List and d_in2ListBound to the shared memory
	unsigned int sharedSize = sizeof(int)*( 2*in1ListLen + in2ListLen );
	multi_AND_wS_woC_kernel<<<numBlock, numThread, sharedSize>>>( d_in1, d_in1List, in1ListLen,
		d_in2, d_in2List, in2ListLen, d_in2ListBound, segLen, d_out);
	CUT_CHECK_ERROR( "multi_AND_wS_woC_kernel" );
	SYNC();
}


void multi_AND_wS_woC_hostList(bytes_t* d_in1, 
							   const int* h_in1List,
							   const int in1ListLen, 
							   const bytes_t* d_in2, 
							   const int* h_in2List, const int in2ListLen, 
							   const int* h_in2ListBound,
							   const int segLen, 
							   bytes_t* d_out,
							   const unsigned int numBlock = 8, 
							   const unsigned int numThread = 192
							   )
{
	unsigned int copyTimer = 0;
	startTimer( &copyTimer );
	TOGPU( d_in1List, h_in1List, sizeof(int)*in1ListLen );
	TOGPU( d_in2ListBound, h_in2ListBound, sizeof(int)*in1ListLen );
	TOGPU( d_in2List, h_in2List, sizeof(int)*in2ListLen );
	copyTime += endTimer( "", &copyTimer );

	unsigned int kernelTimer = 0;
	startTimer( &kernelTimer );
	multi_AND_wS_woC(d_in1, d_in1List,
		in1ListLen, 
		d_in2, 
		d_in2List, in2ListLen, 
		d_in2ListBound,
		segLen, 
		d_out);
	kernelTime += endTimer( "", &kernelTimer );
}

//!!!!!
void multi_AND_woS_wC(bytes_t* d_in1, 
					  const int* d_in1List,
					  const int in1ListLen, 
					  const bytes_t* d_in2, 
					  const int* d_in2List, const int in2ListLen /**/, 
					  const int* d_in2ListBound,
					  const int segLen, 
					  table_t* d_countOut,
					  const unsigned int numBlock = 8, 
					  const unsigned int numThread = 192
					  )
{

	//store the d_in1List, d_in2List and d_in2ListBound to the shared memory
	unsigned int sharedSize = sizeof(int)*( 2*in1ListLen + in2ListLen );

	multi_AND_woS_wC_kernel<<<numBlock, numThread, sharedSize>>>( d_in1, d_in1List, in1ListLen,
		d_in2, d_in2List, in2ListLen, d_in2ListBound, segLen, d_countOut);
	CUT_CHECK_ERROR( "multi_AND_wS_woC_kernel" );
	SYNC();			

}


void multi_AND_woS_wC_hostList(bytes_t* d_in1, 
							   const int* h_in1List,
							   const int in1ListLen, 
							   const bytes_t* d_in2, 
							   const int* h_in2List, const int in2ListLen /**/, 
							   const int* h_in2ListBound,
							   const int segLen, 
							   table_t* d_countOut,
							   const unsigned int numBlock = 8, 
							   const unsigned int numThread = 192
							   )
{
	unsigned int copyTimer = 0;
	startTimer( &copyTimer );
	TOGPU( d_in1List, h_in1List, sizeof(int)*in1ListLen );
	TOGPU( d_in2ListBound, h_in2ListBound, sizeof(int)*in1ListLen );
	TOGPU( d_in2List, h_in2List, sizeof(int)*in2ListLen );
	copyTime += endTimer( "", &copyTimer );
	unsigned int kernelTimer = 0;
	startTimer( &kernelTimer );
	multi_AND_woS_wC(d_in1, 
		d_in1List,
		in1ListLen, 
		d_in2, 
		d_in2List, in2ListLen /**/, 
		d_in2ListBound,
		segLen, 
		d_countOut
		);
	kernelTime += endTimer( "", &kernelTimer );
}

bytes_t* d_bitmap;
bytes_t* d_midRes;
table_t* d_byteTable;
int itemSize;
int numTran;
int numTranInByte;
bytes_t* d_multiOut;
bytes_t* h_multiOut;
table_t* d_multiCountOut;
table_t* h_multiCountOut;
int* d_countBuf; 
#define CPU_COUNT
//#define GPU_MINER_DEBUG

template<class T>
void copyFromGPU( T** h_out, T* d_in, unsigned int len )
{
	CPUMALLOC( (void**)&(*h_out), sizeof(T)*len );
	FROMGPU( (*h_out), d_in, sizeof(T)*len );
}

template<class T>
void copyToGPU( T** d_out, T* h_in, unsigned int len )
{
	GPUMALLOC( (void**)&(*d_out), sizeof(T)*len );
	TOGPU( (*d_out), h_in, sizeof(T)*len );
}


//////////////////////////////////////// The basic GPU functions //////////////////

extern "C"
void GPUInit( int argc, char** argv )
{
	CUT_DEVICE_INIT( argc, argv );
}

extern "C"
void gpuMalloc( void** gpu_data, unsigned int sizeInByte )
{
	GPUMALLOC( gpu_data, sizeInByte );
}

extern "C"
void copyCPUToGPU( void* cpu_data, void* gpu_data, unsigned int sizeInByte )
{
	TOGPU( gpu_data, cpu_data, sizeInByte );
}

extern "C"
void copyCPUToGPUConstant( void* cpu_data, void* gpu_data, unsigned int sizeInByte )
{
	CUDA_SAFE_CALL( cudaMemcpyToSymbol( gpu_data, cpu_data, sizeInByte ) );
}

extern "C"
void copyGPUToCPU( void* gpu_data, void* cpu_data, unsigned int sizeInByte )
{
	FROMGPU( cpu_data, gpu_data, sizeInByte );
}

extern "C"
void GPUFree( void* gpu_data )
{
	CUDA_SAFE_CALL( cudaFree( gpu_data ) );
}

/////////////////////////////////////////////// GPUMiner ////////////////////////

extern "C"
void GPUMiner_Free()
{
	GPUFREE( d_bitmap );
	GPUFREE( d_midRes );
	GPUFREE( d_byteTable );
	GPUFREE( d_multiCountOut );
	
#ifdef CPU_COUNT
	CPUFREE( h_multiCountOut );
#endif
}



extern "C"
void arrayAndInit( const unsigned int maxListLen, const unsigned int numTranInByte )
{
	//����ʹ�ó����ڴ����������Ч�ʺ�ʱ�䣬��Ҫ��h_byteTable����ŵ�GPU�ĳ����ڴ��в�ѯ��
	CUDA_SAFE_CALL( cudaMemcpyToSymbol( c_byteTable, h_byteTable, sizeof(table_t)*TABLE_SIZE ) );	
	GPUMALLOC( (void**)&d_id2List, sizeof(int)*maxListLen );	//maxListLen ��ʼ�������Ƶ��1�����

	GPUMALLOC( (void**)&d_in1List, sizeof(int)*NUM_MAX_NODE_PER_CALL );
	GPUMALLOC( (void**)&d_in2List, sizeof(int)*NUM_MAX_NODE_PER_CALL );
	GPUMALLOC( (void**)&d_in2ListBound, sizeof(int)*NUM_MAX_NODE_PER_CALL );

#ifdef USE_GATHER_FOR_COUNT
	GPUMALLOC( (void**)&g_byteTable, sizeof(table_t)*TABLE_SIZE );
	TOGPU( g_byteTable, h_byteTable, sizeof(table_t)*TABLE_SIZE  );
	GPUMALLOC( (void**)&d_multiOut, sizeof(bytes_t)*numTranInByte*NUM_MAX_NODE_PER_CALL );
#endif

#ifdef USE_CONSTANT_BUF
	CPUMALLOC( (void**)&hh_in, sizeof(bytes_t)*IN_SIZE );
#endif
#ifdef USE_COPY
	GPUMALLOC( (void**)&d_buf, sizeof(bytes_t)*maxListLen*numTranInByte );
#endif
}
//��ʼ��GPU�������ھ�
/*
* h_numTran:��ǰ���������
* h_itemSize:Ƶ��1��ĸ���
*/
extern "C"
void initGPUMiner( bytes_t* h_matrix, const unsigned int h_itemSize, const unsigned int h_numTran )
{
	numTran = h_numTran;
	numTranInByte = (int)ceil((float)numTran/NUM_BIT_IN_BYTE);//NUM_BIT_IN_BYTE = 16������ȡ����ΪʲôҪ����һ���ֽ�����16λ
	itemSize = h_itemSize;	

	unsigned int memTimer = 0;
	startTimer( &memTimer );
	//����GPU���ڴ棬����Ĵ�СΪ������Ϊ������ռ�ֽ���*Ƶ��1�����
	GPUMALLOC( (void**)&d_bitmap, sizeof(bytes_t)*numTranInByte*itemSize );//��Ϊ�ոն����2�������������sizeof(bytes_t),����ΪʲôҪ��ô��
	//����cudaMemcpy��CPU�е�h_matrix���Ƶ�d_bitmap
	TOGPU( d_bitmap, h_matrix, sizeof(bytes_t)*numTranInByte*itemSize);
	//Ϊ�м�������GPU�ڴ�ռ�midRes
	//GPUMALLOC( (void**)&d_midRes, sizeof(bytes_t)*numTranInByte*itemSize*NUM_MAX_LEVEL );
	//GPUInit<bytes_t>( d_midRes, numTranInByte*itemSize, 0 );
	GPUMALLOC( (void**)&d_midRes, sizeof(bytes_t)*numTranInByte*NUM_MAX_NODE_PER_CALL*NUM_MAX_LEVEL );
	CUDA_SAFE_CALL( cudaMemset(d_midRes, 0, sizeof(bytes_t)*numTranInByte*NUM_MAX_NODE_PER_CALL*NUM_MAX_LEVEL) );

	//initialize for the andVector��ʼ����Ϊ������׼��
	arrayAndInit( itemSize, numTranInByte);
	GPUMALLOC( (void**)&d_multiCountOut, sizeof(table_t)*numTranInByte*NUM_MAX_NODE_PER_CALL ); ///////////////!!!!!!! can be deleted

#ifdef CPU_COUNT
	GPUMALLOC( (void**)&d_multiOut, sizeof(bytes_t)*numTranInByte*NUM_MAX_NODE_PER_CALL );
	CPUMALLOC( (void**)&h_multiOut, sizeof(bytes_t)*numTranInByte*NUM_MAX_NODE_PER_CALL );
	CPUMALLOC( (void**)&h_multiCountOut, sizeof(table_t)*numTranInByte*NUM_MAX_NODE_PER_CALL );
#else
	GPUMALLOC( (void**)&d_countBuf, sizeof(int)*NUM_MAX_NODE_PER_CALL );
#endif
	copyTime = endTimer("", &memTimer);
}

int levelPos[NUM_MAX_LEVEL] = {0};

//update the level size
inline void updateLevelSize( int levelIdx, const int levelSize )
{
	levelPos[levelIdx] = levelPos[levelIdx - 1] + levelSize;
}

//get the starting address of offset in levelIdx
//ȡ��levelIdx��ʼ��ַ��ƫ��
inline int getLevelPos( const int levelIdx, const int offset )
{
	return levelPos[levelIdx] + offset*numTranInByte;
}


//left: int midRltBeginPos, int *parentList
//if midRltBeginPos = -1, parentList->matrix
//if midRltBeginPos != -1, midRest's offset, last level
//right: 
//itemIdList: matrix
//int *itemLenList, int pairNum
//midRltStoreLevel: if no counting, store
//|paraentList| and |itemLenList| = pairNum
//|itemIdList| = itemIdListLen
extern "C"
void gpuBoundVectorAnd( const int midRltBeginPos, const int *parentList, const int *itemLenList, const int pairNum,
						const int *itemIdList, const int itemIdListLen, const int midRltStoreLevel, 
						const bool countSup, int *supList)
{
	bytes_t* d_in1;
	bytes_t* d_in2;

	if( midRltBeginPos == -1 )
	//if(!countSup)
	{
		d_in1 = d_bitmap; //from the bitmap
	}
	else
	{
		//��Ҫ���м�����ʼ
		//��level=3ʱ��d_in1=d_midRes+gtLevelPos(1,0)��max_level=15,d_in1ָ���м�����ʼ��λ��
		/*
		inline int getLevelPos( const int levelIdx, const int offset )
		{
			return levelPos[levelIdx] + offset*numTranInByte;
		}
		*/
		//
		d_in1 = d_midRes + getLevelPos( midRltStoreLevel - 2, midRltBeginPos );//from the midRes	
	}

	d_in2 = d_bitmap;

#ifdef GPU_MINER_DEBUG
	bytes_t* h_in1;
	bytes_t* h_in2;
	copyFromGPU<bytes_t>( &h_in1, d_in1, numTranInByte*pairNum );
	copyFromGPU<bytes_t>( &h_in2, d_in2, numTranInByte*itemSize );
	int a = 1;
#endif

	if( countSup )//no store, do counting
	{
         //unsigned int kernelTimer = 0;
		 //startTimer( &kernelTimer );
		 multi_AND_woS_wC_hostList(d_in1, parentList, pairNum, 
							  d_in2, 
							  itemIdList, itemIdListLen, 
							  itemLenList,
							  numTranInByte, 
							  d_multiCountOut );
		 //kernelTime += endTimer( "", &kernelTimer );

#ifdef CPU_COUNT
		unsigned int copyTimer = 0;
		startTimer( &copyTimer );
		//��d_multiCountOut��GPU�п�����CPU��
			FROMGPU( h_multiCountOut, d_multiCountOut, sizeof(table_t)*numTranInByte*itemIdListLen );
		copyTime += endTimer("", &copyTimer);
		
		unsigned int countTimer = 0;
		startTimer( &countTimer );
			for( int i = 0; i < itemIdListLen; i++ )
			{
				int sum = 0;
				for( int j = 0; j < numTranInByte; j++ )
				{
					sum += h_multiCountOut[i*numTranInByte + j];
				}
				supList[i] = sum;
			}
		countTime += endTimer( "", &countTimer );		
#else
		unsigned int countTimer = 0;
		startTimer( &countTimer );
			fixlenSum_v2<table_t, int>( d_multiCountOut, d_countBuf, numTranInByte, itemIdListLen, supList ); 		 
		countTime += endTimer( "", &countTimer );
#endif
		
	}
	else //store, no counting
	{
		//��midRltStoreLevel = 2
		bytes_t* d_out = d_midRes + levelPos[midRltStoreLevel - 1];

		multi_AND_wS_woC_hostList( d_in1, parentList, pairNum, 
					  d_in2, 
					  itemIdList, itemIdListLen, 
					  itemLenList,
					  numTranInByte, 
					  d_out
					  );
		//�м���ռ����itemIdListLen*numTranInByte��˫�ֽ�
		/*
		inline void updateLevelSize( int levelIdx, const int levelSize )
		{
			levelPos[levelIdx] = levelPos[levelIdx - 1] + levelSize;
		}
		*/
		//updateLevelSize ����levelPos(��ǰ�������)��ֵ����ǰһ����ȼ���itemIdListLen*numTranInByte
		updateLevelSize( midRltStoreLevel, itemIdListLen*numTranInByte );//���µ�����һ���м�����ʼ��λ��
	}

#ifdef GPU_MINER_DEBUG
		bytes_t* h_midRes;
		copyFromGPU<bytes_t>( &h_midRes, d_midRes, numTranInByte*NUM_MAX_NODE_PER_CALL*NUM_MAX_LEVEL );
		a = 1;
#endif
}



extern "C"
double getCopyTime()
{
	double timer = copyTime;
	copyTime = 0.0;
	return timer;
}

extern "C"
double getCountTime()
{
	double timer = countTime;
	countTime = 0.0;
	return timer;
}

extern "C"
double getKernelTime()
{
	double timer = kernelTime;
	kernelTime = 0.0;
	return timer;
}

#endif