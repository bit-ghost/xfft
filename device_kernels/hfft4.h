#include"hfft.h"

__global__ void d_hfft4x( float2* d_o, float2* d_i, int bat )
{											
	float2 c[4], temp;
	int tidx=blockIdx.x*blockDim.x+threadIdx.x;
	if(tidx>=bat) return;
	d_i+=(tidx<<2);
	d_o+=(tidx<<2);
	mLOAD4(c,d_i,1,)
	mFFT4(c,)
	mISTORE4(d_o,c,1,)
}
__global__ void d_hifft4x( float2* d_o, float2* d_i, int bat )
{											
	float2 c[4], temp;
	int tidx=blockIdx.x*blockDim.x+threadIdx.x;
	if(tidx>=bat) return;
	d_i+=(tidx<<2);
	d_o+=(tidx<<2);
	mLOAD4(c,d_i,1,)
	mFFT4(c,i)
	mISTORE4(d_o,c,1,)
}