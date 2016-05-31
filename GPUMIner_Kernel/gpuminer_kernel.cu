#ifndef _GPU_MINER_KERNEL_CUH_
#define _GPU_MINER_KERNEL_CUH_


//#include "gpu_multiVectorAnd.cu"
#include "gpu_vectorAndInit.cuh"
#include "CUDA_header.cuh"

////////////////////////////////////// Global Variable /////////////////////////
double copyTime = 0.0;
double countTime = 0.0;
double kernelTime = 0.0;

static __device__ __constant__ table_t c_byteTable[TABLE_SIZE];//常量内存不用cudaMalloc
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
	//感觉这里可以不使用threadIdx.x
	for( int i = threadIdx.x; i < in1ListLen; i += blockDim.x )
	{
		s_in1List[i] = d_in1List[i];
		s_in2ListBound[i] = d_in2ListBound[i];
	}
	for( int i = threadIdx.x; i < in2ListLen; i += blockDim.x )
	{
		//s_in2List中每一项记录的是一个itemid值
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
		//d_segIn1 指向的是d_in1当前要处理的片段，即当前父节点片段所有的事务的01值
		d_segIn1 = d_in1 + segLen*(s_in1List[id1]);
		//下面三行代码，表示内循环将处理父节点为id1的子节点，并且是通过GPU同时处理s_in2ListBound[id1]（大约在200-400左右）对父子节点的与运算,每个线程处理16位
		//数据的与运算
		d_segIn2List = s_in2List + outOffset;		
		segIn2ListLen = s_in2ListBound[id1];

		segOutLen = segLen*segIn2ListLen;//segIn2ListLen是小于1000的值
		//for each segment,每个线程处理16位的数据与运算		
		//index的取值范围是0-1536，delta为1536，当前需要处理segOutLen个16位数据，因为同时要计算s_in2ListBound[id1]对父子节点的与运算，所以处理索引比较复杂
		for( int i = index; i < segOutLen; i += delta )
		{
			//idid2 表示当前这个线程属于哪一个子节点
			idid2 = i/segLen;
			//offset在d_segIn1和d_in2中是同步的 offsetd的取值范围是0-(segLen-1),保证作与运算的是两个项
			offset = i - idid2*segLen;
			//posInIn2 表示在当前父节点下，所要计算的子节点的16位数据的索引，这个值要和对应的父节点相对应，其中d_segIn2List[idid2]记录的是子节点对应的item的id的值
			//offset要保证两个做与运算的父子节点的偏移要一致
			posInIn2 = d_segIn2List[idid2]*segLen + offset;
			//d_out 存放中间结果，其容量是2*segLen*1500*15,(1500应该和1000是有关系),15的意思是最大level的值，关于15的用意需要理解levelPos这个数组
			//segOutLen < d_out的容量，
			//另外获取d_out可以放在常量存储器中
			//将与运算的值放入d_out中，outOffset*segLen是为了确定当前父节点所包含的全部子节点在中间结果的位置
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
			//offset在d_segIn1和d_in2中是同步的 offsetd的取值范围是0-(segLen-1),保证作与运算的是两个项
			offset = i - idid2*segLen;
			posInIn2 = d_segIn2List[idid2]*segLen + offset;
			out = d_segIn1[offset]&d_in2[posInIn2];
			//当计算到最后的level时，直接对最终结果求值，计算有多少个1
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
	//这里使用常量内存来提高运行效率和时间，主要将h_byteTable这个放到GPU的常量内存中查询表
	CUDA_SAFE_CALL( cudaMemcpyToSymbol( c_byteTable, h_byteTable, sizeof(table_t)*TABLE_SIZE ) );	
	GPUMALLOC( (void**)&d_id2List, sizeof(int)*maxListLen );	//maxListLen 初始传入的是频繁1项集个数

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
//初始化GPU来进行挖掘
/*
* h_numTran:当前事物的数量
* h_itemSize:频繁1项集的个数
*/
extern "C"
void initGPUMiner( bytes_t* h_matrix, const unsigned int h_itemSize, const unsigned int h_numTran )
{
	numTran = h_numTran;
	numTranInByte = (int)ceil((float)numTran/NUM_BIT_IN_BYTE);//NUM_BIT_IN_BYTE = 16，向上取整，为什么要定义一个字节中有16位
	itemSize = h_itemSize;	

	unsigned int memTimer = 0;
	startTimer( &memTimer );
	//分配GPU的内存，分配的大小为基本上为事务所占字节数*频繁1项个数
	GPUMALLOC( (void**)&d_bitmap, sizeof(bytes_t)*numTranInByte*itemSize );//因为刚刚多除了2，所以这里乘以sizeof(bytes_t),但是为什么要这么做
	//调用cudaMemcpy将CPU中的h_matrix复制到d_bitmap
	TOGPU( d_bitmap, h_matrix, sizeof(bytes_t)*numTranInByte*itemSize);
	//为中间结果分配GPU内存空间midRes
	//GPUMALLOC( (void**)&d_midRes, sizeof(bytes_t)*numTranInByte*itemSize*NUM_MAX_LEVEL );
	//GPUInit<bytes_t>( d_midRes, numTranInByte*itemSize, 0 );
	GPUMALLOC( (void**)&d_midRes, sizeof(bytes_t)*numTranInByte*NUM_MAX_NODE_PER_CALL*NUM_MAX_LEVEL );
	CUDA_SAFE_CALL( cudaMemset(d_midRes, 0, sizeof(bytes_t)*numTranInByte*NUM_MAX_NODE_PER_CALL*NUM_MAX_LEVEL) );

	//initialize for the andVector初始化，为交运算准备
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
//取得levelIdx开始地址的偏移
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
		//需要从中间结果开始
		//当level=3时，d_in1=d_midRes+gtLevelPos(1,0)，max_level=15,d_in1指向中间结果开始的位置
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
		//将d_multiCountOut从GPU中拷贝到CPU中
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
		//当midRltStoreLevel = 2
		bytes_t* d_out = d_midRes + levelPos[midRltStoreLevel - 1];

		multi_AND_wS_woC_hostList( d_in1, parentList, pairNum, 
					  d_in2, 
					  itemIdList, itemIdListLen, 
					  itemLenList,
					  numTranInByte, 
					  d_out
					  );
		//中间结果占据了itemIdListLen*numTranInByte个双字节
		/*
		inline void updateLevelSize( int levelIdx, const int levelSize )
		{
			levelPos[levelIdx] = levelPos[levelIdx - 1] + levelSize;
		}
		*/
		//updateLevelSize 更新levelPos(当前计算深度)的值，由前一个深度加上itemIdListLen*numTranInByte
		updateLevelSize( midRltStoreLevel, itemIdListLen*numTranInByte );//更新的是下一个中间结果开始的位置
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