#include "BenLib.h"

#ifdef __CUDA__

#ifdef __DEBUG__
size_t d_dmemUsage = 0;
#endif //__DEBUG__
 
//---------------------------------------------
//GPU memory operations
//---------------------------------------------
char *D_MALLOC(size_t size)
{	
	char *buf = NULL;
	CUDA_SAFE_CALL(cudaMalloc((void**)&buf, size));
	CUDA_SAFE_CALL(cudaMemset(buf, 0, size));
#ifdef __DEBUG__
#	ifdef __ALLOC__
	BenLog("+d%d bytes\n", size);
#	endif //__ALLOC__
	d_dmemUsage += size;
#endif
	return buf;
}

//------------------------------------------------
//free memory on device and set the pointer to NULL
//
//param	: buf
//------------------------------------------------
void D_FREE(void *buf, size_t size)
{
	CUDA_SAFE_CALL(cudaFree(buf));
	buf = NULL;
#ifdef __DEBUG__
#	ifdef __ALLOC__
	BenLog("-d%d bytes\n", size);
#	endif //__ALLOC__
	d_dmemUsage -= size;
#endif
}

//-------------------------------------------------------
//copy a buffer from host memory to device memory
//
//param	: des
//param	: src
//param	: size
//-------------------------------------------------------
void D_MEMCPY_H2D(void *des, void *src, size_t size)
{
	CUDA_SAFE_CALL(cudaMemcpy(des, src, size, cudaMemcpyHostToDevice));
}

//-------------------------------------------------------
//copy a buffer from device memory to host memory
//
//param	: des
//param	: src
//param	: size
//-------------------------------------------------------
void D_MEMCPY_D2H(void *des, void *src, size_t size)
{
	CUDA_SAFE_CALL(cudaMemcpy(des, src, size, cudaMemcpyDeviceToHost));
}

void D_ENTER_FUNC(char *func)
{
#ifdef __DEBUG__
	BenLog("=====GPU Enter %s=====\n", func);
#endif
}

void D_LEAVE_FUNC(char *func)
{
#ifdef __DEBUG__
	BenLog("=====GPU Leave %s: %d bytes=====\n", func, d_dmemUsage);
#endif// __DEBUG__
}

void D_ANYDEVICE()
{
 /*   int deviceCount;                                                         \
    CUDA_SAFE_CALL_NO_SYNC(cudaGetDeviceCount(&deviceCount));                \
    if (deviceCount == 0) {                                                  \
        fprintf(stderr, "There is no device.\n");                            \
        exit(EXIT_FAILURE);                                                  \
    }                                                                        \
    int dev;                                                                 \
    for (dev = 0; dev < deviceCount; ++dev) {                                \
        cudaDeviceProp deviceProp;                                           \
        CUDA_SAFE_CALL_NO_SYNC(cudaGetDeviceProperties(&deviceProp, dev));   \
        if (deviceProp.major >= 1)                                           \
            break;                                                           \
    }                                                                        \
    if (dev == deviceCount) {                                                \
        fprintf(stderr, "There is no device supporting CUDA.\n");            \
        exit(EXIT_FAILURE);                                                  \
    }                                                                        \
    else                                                                     \
        CUDA_SAFE_CALL(cudaSetDevice(dev)); */
}
#endif //__CUDA__