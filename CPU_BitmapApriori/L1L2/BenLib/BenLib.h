#ifndef __BENLIB_H__
#define __BENLIB_H__

//#define __GPUIO__
#define __TIMER__
#define __DEBUG__
#define __CUDA__
#define __MEM__
//#define __ALLOC__

#ifdef __UNIX__
#	include <unistd.h>
#	include <arpa/inet.h>
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <sys/stat.h>
#	include <netinet/in.h>
#	include <fcntl.h>
#	include <netinet/in.h>
#	include <stdbool.h>
#	include <sys/mman.h>
#	include <sys/time.h>
#	include <pthread.h>
#else // __UNIX__
#	include <windows.h>
#	pragma warning(disable: 4267)
#	pragma warning(disable: 4996)
#	pragma warning(disable: 4311)
#	pragma warning(disable: 4312)
#	pragma warning(disable: 4244)
#	pragma warning(disable: 4018)
#	pragma warning(disable: 4309)
#endif // __WIN32__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>

#ifdef __CUDA__
#	include <cutil.h>
#	include <cuda.h>
#	include <cuda_runtime.h>
#	ifndef __UNIX__
#		pragma comment(lib, "cuda.lib")
#		pragma comment(lib, "cudart.lib")
#		pragma comment(lib, "cutil32.lib")
#	endif //__UNIX__
#endif //__CUDA__

#ifdef __GPUIO__
#	include <db.h>
#	ifndef __UNIX__
#		pragma comment(lib, "libdb47.lib")
#	endif //__UNIX__
#endif

#define IN
#define OUT

typedef unsigned char uchar_t;
typedef unsigned char byte_t;
typedef unsigned short ushort_t;
typedef unsigned long ulong_t;
typedef unsigned int uint_t;

//----------------------------------------
//BenLog
//----------------------------------------
extern "C"
void BenLog(IN char *, IN ...);

#define dprint(...) //printf(__VA_ARGS__)

//----------------------------------------
//BenTime
//----------------------------------------
#ifdef __UNIX__
	typedef long long i64;
#else //__UNIX__
	typedef __int64 i64 ;
#endif //__WIN32__

typedef struct CPerfCounterRec
{
    i64 _freq;
    i64 _clocks;
    i64 _start;
} CPerfCounter;

extern "C"
void BenSetup(IN int);
extern "C"
void BenStart(IN int);
extern "C"
void BenStop(IN int);
extern "C"
void BenReset(IN int);
extern "C"
double BenGetElapsedTime(IN int);
extern "C"
void BenPrintTime(IN char *, IN int);

#ifdef __UNIX__
typedef struct timeval BenTimeVal_t;
#else //__unix__
typedef LARGE_INTEGER BenTimeVal_t;
#endif //__WIN32__

extern "C"
void BenStartTimer(OUT BenTimeVal_t *start_tv);
extern "C"
double BenEndTimer(OUT BenTimeVal_t *start_tv);

//----------------------------------------
//BenAssert
//----------------------------------------
#ifdef __DEBUG__
#define BEN_ASSERT(x) \
do{if (!(x)) { \
            fprintf(stdout, "\nASSERT FAILED in file %s, line %d\n\n", __FILE__, __LINE__); \
            fflush(stdout); \
			exit(-1); \
}}while(0)
#else //__DEBUG__
#define BEN_ASSERT(x) do{(x);}while(0)/* do nothing */
#endif

//----------------------------------------
//BenMem
//----------------------------------------
#ifdef __DEBUG__
extern size_t d_memUsage;
extern size_t d_memTransfer;
#endif //__DEBUG__

extern "C"
void *BenMalloc(IN size_t size);
extern "C"
void BenFree(OUT char **buf, IN size_t size);
extern "C"
void BenMemset(OUT void *buf, IN int c, IN size_t size);
extern "C"
void BenMemcpy(OUT void *dest, IN void *src, IN size_t size);
extern "C"
void *BenRealloc(OUT void *buf, IN size_t oldSize, IN size_t newSize);
extern "C"
char *BenStrDup(IN char *str);
extern "C"
int BenStrLen(IN char *str);
extern "C"
void EnterFunc(IN char *funcName);
extern "C"
void LeaveFunc(IN char *funcName);

//--------------------------------------------------
//BenThread
//--------------------------------------------------
#ifdef __UNIX__
typedef pthread_t BenThread_t;
#else //__UNIX__
typedef HANDLE BenThread_t;
#endif //__WIN32__

extern "C"
BenThread_t BenNewThread(IN void *(*start_routine)(void*),  IN void *arg);
extern "C"
void BenExitThread(IN void *status);
extern "C"
void BenWaitForSingle(IN BenThread_t id);
extern "C"
void BenWaitForMul(IN BenThread_t* ids, IN int num);

//--------------------------------------------------
//BenFile
//--------------------------------------------------
typedef struct
{
	char *addr;
#ifndef __UNIX__
	HANDLE hMap;
	__int64 fileSize;
#endif
} BenMM_t;

extern "C"
void BenWriteFile(IN char *fileName, IN void *buf, IN size_t size);
extern "C"
void BenAppendFile(IN char *fileName, IN void *buf, IN size_t size);
extern "C"
char *BenReadFile(IN char *fileName, IN size_t offset, IN size_t size);
extern "C"
BenMM_t *BenStartMMap(IN char *fileName);
extern "C"
void BenEndMMap(IN BenMM_t *fileName);
extern "C"
size_t BenGetFileSize(IN char *fileName);

typedef struct
{
#ifndef __UNIX__
	HANDLE hDir;
	WIN32_FIND_DATA wfd;
#endif //__UNIX__
} BenDir_t;

typedef struct
{
	char *filename;
	int fileId;
	size_t fileSize;
	char *h_fileBuf;
	char *d_fileBuf;
#ifndef __UNIX__
#endif //__UNIX__
} BenFileInDir_t;
extern "C"
BenDir_t *BenEnterDir(IN char *dirname);
extern "C"
char BenFindDir(IN BenDir_t *dir, IN BenFileInDir_t *file);
extern "C"
void BenLeaveDir(IN BenDir_t *dir);

#ifdef __UNIX__
typedef FILE File_t 
#else
typedef HANDLE File_t;
#endif //__UNIX__
#define R	0x00	//read
#define W	0x01	//write
#define A	0x02	//append
extern "C"
File_t BenOpen(IN char *fileName);
extern "C"
size_t BenRead(IN File_t handle, OUT char *buf, IN size_t fileOffset, IN size_t size);
extern "C"
size_t BenWrite(IN File_t handle, IN char *buf, IN size_t size);
//---------------------------------------------------
//BenCUDAMem
//---------------------------------------------------
#ifdef __CUDA__
extern "C"
char *D_MALLOC(IN size_t size);
extern "C"
void D_FREE(IN void *buf, IN size_t size);
extern "C"
void D_MEMCPY_H2D(OUT void *des, IN void *src, IN size_t size);
extern "C"
void D_MEMCPY_D2H(OUT void *des, IN void *src, IN size_t size);
extern "C"
void D_ENTER_FUNC(IN char *func);
extern "C"
void D_LEAVE_FUNC(IN char *func);
extern "C"
void D_ANYDEVICE();
#endif //__CUDA__

//---------------------------------------------------
//BenCUDAFile
//---------------------------------------------------
#ifdef __CUDA__

typedef struct
{
	char *filename;
#ifndef __UNIX__
	HANDLE hFile;
#endif //__UNIX__
	size_t fileSize;
	size_t offset;
} FStream_t;

extern "C"
FStream_t *D_OPEN_FILE(char *filename);
extern "C"
size_t D_ITERATE_CHUNK(FStream_t *fs, char *gpubuf, size_t chunkSize);
extern "C"
char D_READ_CHUNK(FStream_t *fs, char *gpubuf, 
				  size_t offset, size_t size, size_t *read_size);
extern "C"
char D_WRITE_CHUNK(FStream_t *fs, char *gpubuf, 
				   size_t offset, size_t size);
extern "C"
char D_APPEND_CHUNK(FStream_t *fs, char *gpubuf, size_t size);
extern "C"
void D_RESET_ITERATOR(FStream_t *fs);
extern "C"
void D_CLOSE_FILE(FStream_t *fs);
#endif
//---------------------------------------------------
//BenArg
//---------------------------------------------------
extern "C"
bool BenCheckArg(IN int argc, IN char **argvs, IN char *arg);
extern "C"
bool BenGetArgInt(IN int argc, IN char **argvs, IN char *argv, OUT int *val);
extern "C"
bool BenGetArgFloat(IN int argc, IN char **argvs, IN char *argv, OUT float *val);
extern "C"
bool BenGetArgStr(IN int argc, IN char **argvs, IN char *argv, OUT char **val);
extern "C"
bool BenGetArgStrs(IN int argc, IN char **argvs, IN char *argv, OUT char **val, OUT int *len);
extern "C"
void BenPrintUsage(IN char *fmt, IN ...);

//-------------------------------------------------
//BenMath
//-------------------------------------------------
extern unsigned char g_bitCountUchar[];
extern unsigned short g_bitCountUshort[];

extern "C"
float *BenGenMatrixf(IN int rowNum, IN int colNum);
extern "C"
double *BenGenMatrixd(IN int rowNum, IN int colNum);
extern "C"
int BenCeilInt(size_t x, size_t y);

//-------------------------------------------------
//BenGPUIO
//-------------------------------------------------
#ifdef __GPUIO__
typedef int (*bt_cmp_func_t)(DB *db, const DBT *key1, const DBT *key2);

#define CPU_MEM		0x00000000
#define GPU_MEM		0x00000001

typedef struct
{
	DB *dbp;
	int mode;

	bt_cmp_func_t cmpFunc;	
}GPUIO_t;

typedef struct
{
	void *key;
	size_t keySize;
	void *gpuData;
	void *cpuData;
	size_t dataSize;

	int readMode;
	int writeMode;
} Bulk_t;

extern "C"
GPUIO_t *BenOpenGPUIO(IN char *fileName, IN bt_cmp_func_t cmpFunc);
extern "C"
int BenReadBulk(IN GPUIO_t *gpuIo, OUT Bulk_t *bulk);
extern "C"
int BenWriteBulk(IN GPUIO_t *gpuIo, IN Bulk_t *bulk);
extern "C"
void BenResetBulk(IN GPUIO_t *gpuIo, OUT Bulk_t *bulk);
extern "C"
void BenCloseGPUIO(IN GPUIO_t *gpuIo);
#endif //GPUIO

#endif //__BENLIB_H__