#ifndef __xfft_vop_h__
#define __xfft_vop_h__

#include"xfft_kernel.h"

void vfft_bki( xfft_kernel_t* const, CUmodule, CUdeviceptr, int, int, int, int ); 

#endif