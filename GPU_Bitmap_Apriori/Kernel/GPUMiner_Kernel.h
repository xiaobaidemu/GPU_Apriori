#ifndef _GPU_MINER_KERNEL_H_
#define _GPU_MINER_KERNEL_H_

#define GPU_API extern "C" __declspec( dllexport )

////////////////////////////////////// Config ////////////////////////////////

#define NUM_MAX_LEVEL (15) //relate to size of middle result allocated when initialization "initGPUMiner"
#define NUM_MAX_NODE_PER_CALL (1500) //relate to size of middle result allocated when initialization "initGPUMiner"
#define NUM_MAX_TRAN (200000) //200K; maximum number of transcations

//#define USE_GATHER_FOR_COUNT
#define USE_SHORT
//#define USE_CHAR
//////////////////////////////////// Typedef /////////////////////////////////

#ifdef USE_CHAR
typedef unsigned char bytes_t;
#else USE_SHORT
typedef unsigned short bytes_t;
#endif

typedef unsigned char table_t;

////////////////////////////////////// Constant ////////////////////////

#ifdef USE_CHAR
#define NUM_BIT_IN_BYTE (8)
#define TABLE_SIZE 256 //2^8
#else USE_SHORT
#define NUM_BIT_IN_BYTE (16)
#define TABLE_SIZE 65536 //2^16
#endif

#define _IN_
#define _OUT_


////////////////////////////////////// Basic GPU functions with C/C++ interfaces /////////////////

GPU_API void GPUInit( int argc, char** argv );

GPU_API void gpuMalloc( void** gpu_data, unsigned int sizeInByte );

GPU_API void copyCPUToGPU( void* cpu_data, void* gpu_data, unsigned int sizeInByte );

GPU_API void copyCPUToGPUConstant( void* cpu_data, void* gpu_data, unsigned int sizeInByte );

GPU_API void copyGPUToCPU( void* gpu_data, void* cpu_data, unsigned int sizeInByte );

GPU_API void GPUFree( void* gpu_data );

GPU_API void initGPUMiner( bytes_t* h_matrix, const unsigned int h_itemSize, const unsigned int h_numTran );

//////////////////////////////////////////// Primitives ///////////////////////////

GPU_API void initByteTable( table_t** d_byteTable);

GPU_API void array_And_woS_wC( bytes_t* d_in1, bytes_t* d_in2, int* d_count, const unsigned int len, 
							  const table_t* d_byteTable, 
							  unsigned int numBlock = 128, unsigned int numThread = 256, 
							  const unsigned int tableSize = TABLE_SIZE );

GPU_API void array_And_wS_woC( bytes_t* d_in1, bytes_t* d_in2, bytes_t* d_out, const unsigned int len, 
					const table_t* d_byteTable, 
					unsigned int numBlock = 128, unsigned int numThread = 256, 
					const unsigned int tableSize = TABLE_SIZE);

/////////////////////////////////// GPUMiner kernel functions ////////////////////


////////////////////////////////////////////////////////////////////////////////
//! GPU primitive for vector and operation
//! &return count number if isCount = true, or return -1 if isCount = false
//! @param columnId_1: the column id in the matrix
//! @param isMiddleResult: true indicates the id_2 parameter is the id of intermediate result, false means is the column id in the matrix 
//! @param id_2
//! @param storedId: -1 means don't store the result, otherwise storeId is the id of the intermediate result
//! @param isCount: true indicates do counting and return the result, otherwise don't do counting and return -1
////////////////////////////////////////////////////////////////////////////////
GPU_API int gpuVectorAnd( const unsigned int columnId_1, 
				  bool isMiddleResult, 
				  const unsigned int id_2, 
				  const int storeId, 
				  bool isCount );

GPU_API void GPUMiner_Init( char* fileName );

GPU_API void gpuMultiVectorAnd(const int midRltLevel, 
					   const int midRltId, 
					   const int childNum, 
					   const int* childItemList, 
					   const int currentLevelId, 
					   const bool countSup, 
					   int* supList, 
					   const unsigned int numBlock = 256, 
					   const unsigned int numThread = 512);

GPU_API double getCopyTime();
GPU_API double getCountTime();
GPU_API double getKernelTime();

GPU_API int get_num_wo_s_w_c();

GPU_API int get_num_w_s_wo_c();


//left: int midRltBeginPos, int *parentList
//if midRltBeginPos = -1, parentList->matrix
//if midRltBeginPos != -1, midRest's offset
//right: 
//itemIdList: matrix
//int *itemLenList, int pairNum
//midRltStoreLevel: if no counting, store 
GPU_API void gpuBoundVectorAnd( const int midRltBeginPos, const int *parentList, const int *itemLenList, const int pairNum,
						const int *itemIdList, const int itemIdListLen, const int midRltStoreLevel, 
						const bool countSup, int *supList);

#endif