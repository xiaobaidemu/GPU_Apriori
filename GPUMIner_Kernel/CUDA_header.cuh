#ifndef _CUDA_HEADER_CU_
#define _CUDA_HEADER_CU_

#include <stdlib.h>
#include "C:/CUDASDK/C/common/inc/cutil.h"
#include "GPUMiner_Kernel.h"

//Global variables on the GPU device memory



//defined utiliy macro
#define GPUMALLOC(D_POINTER, SIZE) CUDA_SAFE_CALL( cudaMalloc( D_POINTER, SIZE) )
#define CPUMALLOC(H_POINTER, SIZE) CUDA_SAFE_CALL(cudaMallocHost (H_POINTER, SIZE))
#define CPUFREE(H_POINTER) if(H_POINTER!=NULL) CUDA_SAFE_CALL(cudaFreeHost (H_POINTER))
#define GPUFREE(D_POINTER) CUDA_SAFE_CALL( cudaFree( D_POINTER) )
#define TOGPU(D_POINTER,H_POINTER, SIZE)  CUDA_SAFE_CALL(cudaMemcpy(D_POINTER,H_POINTER, SIZE, cudaMemcpyHostToDevice))
#define FROMGPU(H_POINTER, D_POINTER, SIZE)  CUDA_SAFE_CALL(cudaMemcpy(H_POINTER, D_POINTER, SIZE, cudaMemcpyDeviceToHost))
#define GPUTOGPU(D_TO, D_FROM, SIZE)  CUDA_SAFE_CALL(cudaMemcpy(D_TO, D_FROM, SIZE, cudaMemcpyDeviceToDevice))
#define SYNC() CUDA_SAFE_CALL(cudaThreadSynchronize())

void startTimer(unsigned *timer);

double endTimer(char *info, unsigned *timer);

#endif