
#include"hfft.h"

__global__ void d_hfft32x( float2* d_o, float2* d_i, const float2* __restrict__ d_RF, int bat )
{
	extern __shared__ float smem[];
	float2 c[4], RF[3], temp;
	int slice=blockIdx.x*blockDim.y+threadIdx.y;
	if(slice>=bat) return;
	d_o+=((slice<<5)+threadIdx.x);
	d_i+=((slice<<5)+threadIdx.x);
	unsigned int slot=threadIdx.x>>2;
	unsigned int lane=threadIdx.x&3;
	float* base=&smem[40*threadIdx.y];
	float* sst=&base[threadIdx.x];
	float* sld=&base[8*slot+lane];
	RF[0]=d_RF[threadIdx.x];
	mLOAD4(c,d_i,8,);
	mCALRF4(RF)
	mFFT4(c,)
	mHMRF4(c,RF)
	RF[0]=d_RF[lane<<2];
	mISTORE4(sst,c,8,.x)
	mLOAD2(&c[0],sld+0*16,4,.x)
	mLOAD2(&c[2],sld+1*16,4,.x)
	mISTORE4(sst,c,8,.y)
	mLOAD2(&c[0],sld+0*16,4,.y)
	mLOAD2(&c[2],sld+1*16,4,.y)
	mFFT2(&c[0],)
	mFFT2(&c[2],)
	mHMRF2(&c[0],RF)
	mHMRF2(&c[2],RF)
	sst=&base[10*slot+lane];
	sld=&base[10*lane+5*slot];
	mSTORE2(sst+0*20,&c[0],5,.x)
	mSTORE2(sst+1*20,&c[2],5,.x)
	mLOAD4(c,sld,1,.x)
	mSTORE2(sst+0*20,&c[0],5,.y)
	mSTORE2(sst+1*20,&c[2],5,.y)
	mLOAD4(c,sld,1,.y)
	mFFT4(c,)
	mISTORE4(d_o,c,8,)
}
__global__ void d_hifft32x( float2* d_o, float2* d_i, const float2* __restrict__ d_RF, int bat )
{
	extern __shared__ float smem[];
	float2 c[4], RF[3], temp;
	int slice=blockIdx.x*blockDim.y+threadIdx.y;
	if(slice>=bat) return;
	d_o+=((slice<<5)+threadIdx.x);
	d_i+=((slice<<5)+threadIdx.x);
	unsigned int slot=threadIdx.x>>2;
	unsigned int lane=threadIdx.x&3;
	float* base=&smem[40*threadIdx.y];
	float* sst=&base[threadIdx.x];
	float* sld=&base[8*slot+lane];
	RF[0]=d_RF[threadIdx.x];
	RF[0].y=-RF[0].y;
	mLOAD4(c,d_i,8,);
	mCALRF4(RF)
	mFFT4(c,i)
	mHMRF4(c,RF)
	RF[0]=d_RF[lane<<2];
	RF[0].y=-RF[0].y;
	mISTORE4(sst,c,8,.x)
	mLOAD2(&c[0],sld+0*16,4,.x)
	mLOAD2(&c[2],sld+1*16,4,.x)
	mISTORE4(sst,c,8,.y)
	mLOAD2(&c[0],sld+0*16,4,.y)
	mLOAD2(&c[2],sld+1*16,4,.y)
	mFFT2(&c[0],)
	mFFT2(&c[2],)
	mHMRF2(&c[0],RF)
	mHMRF2(&c[2],RF)
	sst=&base[10*slot+lane];
	sld=&base[10*lane+5*slot];
	mSTORE2(sst+0*20,&c[0],5,.x)
	mSTORE2(sst+1*20,&c[2],5,.x)
	mLOAD4(c,sld,1,.x)
	mSTORE2(sst+0*20,&c[0],5,.y)
	mSTORE2(sst+1*20,&c[2],5,.y)
	mLOAD4(c,sld,1,.y)
	mFFT4(c,i)
	mISTORE4(d_o,c,8,)
}
/*
__global__ void d_hfft32x( float2* d_o, const float2* __restrict__ d_i, const float2* __restrict__ d_RF )
{
	__shared__ float smem[32];
	float2 c[8], RF[7], temp;
	d_o+=threadIdx.x;
	d_i+=threadIdx.x;
	float* sst=&smem[threadIdx.x];
	float* sld=&smem[4*threadIdx.x];
	RF[0]=d_RF[threadIdx.x];
	mLOAD8(c,d_i,4,);
	mCALRF8(RF)
	mFFT8(c,)
	mHMRF8(c,RF)
	mISTORE8(sst,c,4,.x) 
	mLOAD4(&c[0],sld+0*16,1,.x)
	mLOAD4(&c[4],sld+1*16,1,.x)
	mISTORE8(sst,c,4,.y)
	mLOAD4(&c[0],sld+0*16,1,.y)
	mLOAD4(&c[4],sld+1*16,1,.y)
	mFFT4(&c[0],)
	mFFT4(&c[4],)
	mISTORE4x2(d_o,c,4,8,)
}
*/