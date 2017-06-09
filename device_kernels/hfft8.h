#include"hfft.h"

__global__ void d_hfft8x( float2* d_o, float2* d_i, int bat )
{											
	float2 c[8], temp;
	int tidx=blockIdx.x*blockDim.x+threadIdx.x;
	if(tidx>=bat) return;
	d_i+=(tidx<<3);
	d_o+=(tidx<<3);
	mLOAD8(c,d_i,1,)
	mFFT8(c,)
	mISTORE8(d_o,c,1,)
}
__global__ void d_hifft8x( float2* d_o, float2* d_i, int bat )
{											
	float2 c[8], temp;
	int tidx=blockIdx.x*blockDim.x+threadIdx.x;
	if(tidx>=bat) return;
	d_i+=(tidx<<3);
	d_o+=(tidx<<3);
	mLOAD8(c,d_i,1,)
	mFFT8(c,i)
	mISTORE8(d_o,c,1,)
}