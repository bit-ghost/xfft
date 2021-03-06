#include"cuintrin.h"
#include"hfft.h"

/*
128x2  : block( 32,n)
128x4  : block( 64,1)
128x8  : block( 64,1)
128x16 : block(128,1)
128x32 : block( 32,8)
128x64 : block( 64,8)
*/
__global__ void d_fft128x8x( float2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[168*8];
	float2 c[16], RF[15], temp;
	unsigned int lane=threadIdx.x&7;
	unsigned int slot=threadIdx.x>>3;
	float* spx=&smem[threadIdx.x];
	float* spy=&smem[168*slot+lane];
	float* spz=&smem[168*slot+9*lane];
	d_c+=(blockIdx.x<<10)+threadIdx.x;
	RF[0]=d_RF[lane];
	mLOAD8x2(c,d_c,64,128,)
	mCALRF16(RF)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mPERMUTE_S8x2_L16(spx,spy,c,64,168,8,0xf)
	mFFT16(c,)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spy,spz,c,9,72,1,0xf)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mPERMUTE8x2(spy,spx,c,8,16,64,168,0x7)
	mSTORE8x2(d_c,c,64,128,)
}
__global__ void d_ifft128x8x( float2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[168*8];
	float2 c[16], RF[15], temp;
	unsigned int lane=threadIdx.x&7;
	unsigned int slot=threadIdx.x>>3;
	float* spx=&smem[threadIdx.x];
	float* spy=&smem[168*slot+lane];
	float* spz=&smem[168*slot+9*lane];
	d_c+=(blockIdx.x<<10)+threadIdx.x;
	RF[0]=d_RF[lane];
	RF[0].y=-RF[0].y;
	mLOAD8x2(c,d_c,64,128,)
	mCALRF16(RF)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mPERMUTE_S8x2_L16(spx,spy,c,64,168,8,0xf)
	mFFT16(c,i)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spy,spz,c,9,72,1,0xf)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mPERMUTE8x2(spy,spx,c,8,16,64,168,0x7)
	mSTORE8x2(d_c,c,64,128,)
}
__global__ void d_fft128x16x( float2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[168*16];
	float2 c[16], RF[15], temp;
	unsigned int lane=threadIdx.x&7;
	unsigned int slot=threadIdx.x>>3;
	float* spx=&smem[threadIdx.x];
	float* spy=&smem[168*slot+  lane];
	float* spz=&smem[168*slot+9*lane];
	d_c+=(blockIdx.x<<11)+threadIdx.x;
	RF[0]=d_RF[lane];
	mLOAD16(c,d_c,128,)
	mCALRF16(RF)
	mFFT16(c,)
	mPERMUTE(16,spx,spy,c,168,8,0xf)
	mFFT16(c,)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spy,spz,c,9,72,1,0xf)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mPERMUTE_S8x2_L16(spy,spx,c,8,16,168,0x7)
	mSTORE16(d_c,c,128,)
}
__global__ void d_ifft128x16x( float2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[168*16];
	float2 c[16], RF[15], temp;
	unsigned int lane=threadIdx.x&7;
	unsigned int slot=threadIdx.x>>3;
	float* spx=&smem[threadIdx.x];
	float* spy=&smem[168*slot+  lane];
	float* spz=&smem[168*slot+9*lane];
	d_c+=(blockIdx.x<<11)+threadIdx.x;
	RF[0]=d_RF[lane];
	RF[0].y=-RF[0].y;
	mLOAD16(c,d_c,128,)
	mCALRF16(RF)
	mFFT16(c,i)
	mPERMUTE(16,spx,spy,c,168,8,0xf)
	mFFT16(c,i)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spy,spz,c,9,72,1,0xf)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mPERMUTE_S8x2_L16(spy,spx,c,8,16,168,0x7)
	mSTORE16(d_c,c,128,)
}
__global__ void d_fft128x64x( float2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[168*64];
	float2 c[16], RF[15], temp;
	unsigned int tidx=(threadIdx.y<<7)+threadIdx.x;
	unsigned int lane=threadIdx.x&7;
	unsigned int slot=(threadIdx.y<<3)+(threadIdx.x>>3);
	float* spx=&smem[ 168*threadIdx.y+threadIdx.x];
	float* spy=&smem[1344*threadIdx.y+threadIdx.x];
	float* spu=&smem[168*slot+  lane];
	float* spv=&smem[168*slot+9*lane];
	d_c+=(blockIdx.x<<13)+tidx;
	RF[0]=d_RF[threadIdx.y<<1];
	mLOAD8x2(c,d_c,64,1024,)
	mCALRF8(RF)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mHMRF8(&c[0],RF)
	mHMRF8(&c[8],RF)
	mPERMUTE8x2(spx,spy,c,64,1344,64,168,0xf)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	RF[0]=d_RF[lane];
	mCALRF16(RF)
	mPERMUTE_S8x2_L16(spy,spu,c,64,168,8,0xf)
	mFFT16(c,)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spu,spv,c,9,72,1,0xf)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mPERMUTE8x2(spu,spy,c,8,16,64,168,0x7)
	mSTORE8x2(d_c,c,64,1024,)
}
__global__ void d_ifft128x64x( float2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[168*64];
	float2 c[16], RF[15], temp;
	unsigned int tidx=(threadIdx.y<<7)+threadIdx.x;
	unsigned int lane=threadIdx.x&7;
	unsigned int slot=(threadIdx.y<<3)+(threadIdx.x>>3);
	float* spx=&smem[ 168*threadIdx.y+threadIdx.x];
	float* spy=&smem[1344*threadIdx.y+threadIdx.x];
	float* spu=&smem[168*slot+  lane];
	float* spv=&smem[168*slot+9*lane];
	d_c+=(blockIdx.x<<13)+tidx;
	RF[0]=d_RF[threadIdx.y<<1];
	RF[0].y=-RF[0].y;
	mLOAD8x2(c,d_c,64,1024,)
	mCALRF8(RF)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mHMRF8(&c[0],RF)
	mHMRF8(&c[8],RF)
	mPERMUTE8x2(spx,spy,c,64,1344,64,168,0xf)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	RF[0]=d_RF[lane];
	RF[0].y=-RF[0].y;
	mCALRF16(RF)
	mPERMUTE_S8x2_L16(spy,spu,c,64,168,8,0xf)
	mFFT16(c,i)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spu,spv,c,9,72,1,0xf)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mPERMUTE8x2(spu,spy,c,8,16,64,168,0x7)
	mSTORE8x2(d_c,c,64,1024,)
}

//==============================================================================================================================================================

__global__ void d_mfft128x8x( ushort2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[168*8];
	float2 c[16], RF[15], temp;
	ushort2 hc[16];
	unsigned int lane=threadIdx.x&7;
	unsigned int slot=threadIdx.x>>3;
	float* spx=&smem[threadIdx.x];
	float* spy=&smem[168*slot+lane];
	float* spz=&smem[168*slot+9*lane];
	d_c+=(blockIdx.x<<10)+threadIdx.x;
	RF[0]=d_RF[lane];
	mLOAD8x2(hc,d_c,64,128,)
	mH2Sx16(c,hc)
	mCALRF16(RF)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mPERMUTE_S8x2_L16(spx,spy,c,64,168,8,0xf)
	mFFT16(c,)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spy,spz,c,9,72,1,0xf)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mPERMUTE8x2(spy,spx,c,8,16,64,168,0x7)
	mS2Hx16(hc,c)
	mSTORE8x2(d_c,hc,64,128,)
}
__global__ void d_imfft128x8x( ushort2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[168*8];
	float2 c[16], RF[15], temp;
	ushort2 hc[16];
	unsigned int lane=threadIdx.x&7;
	unsigned int slot=threadIdx.x>>3;
	float* spx=&smem[threadIdx.x];
	float* spy=&smem[168*slot+lane];
	float* spz=&smem[168*slot+9*lane];
	d_c+=(blockIdx.x<<10)+threadIdx.x;
	RF[0]=d_RF[lane];
	RF[0].y=-RF[0].y;
	mLOAD8x2(hc,d_c,64,128,)
	mH2Sx16(c,hc)
	mCALRF16(RF)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mPERMUTE_S8x2_L16(spx,spy,c,64,168,8,0xf)
	mFFT16(c,i)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spy,spz,c,9,72,1,0xf)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mPERMUTE8x2(spy,spx,c,8,16,64,168,0x7)
	mS2Hx16(hc,c)
	mSTORE8x2(d_c,hc,64,128,)
}
__global__ void d_mfft128x16x( ushort2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[168*16];
	float2 c[16], RF[15], temp;
	ushort2 hc[16];
	unsigned int lane=threadIdx.x&7;
	unsigned int slot=threadIdx.x>>3;
	float* spx=&smem[threadIdx.x];
	float* spy=&smem[168*slot+  lane];
	float* spz=&smem[168*slot+9*lane];
	d_c+=(blockIdx.x<<11)+threadIdx.x;
	RF[0]=d_RF[lane];
	mLOAD16(hc,d_c,128,)
	mH2Sx16(c,hc)
	mCALRF16(RF)
	mFFT16(c,)
	mPERMUTE(16,spx,spy,c,168,8,0xf)
	mFFT16(c,)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spy,spz,c,9,72,1,0xf)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mPERMUTE_S8x2_L16(spy,spx,c,8,16,168,0x7)
	mS2Hx16(hc,c)
	mSTORE16(d_c,hc,128,)
}
__global__ void d_imfft128x16x( ushort2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[168*16];
	float2 c[16], RF[15], temp;
	ushort2 hc[16];
	unsigned int lane=threadIdx.x&7;
	unsigned int slot=threadIdx.x>>3;
	float* spx=&smem[threadIdx.x];
	float* spy=&smem[168*slot+  lane];
	float* spz=&smem[168*slot+9*lane];
	d_c+=(blockIdx.x<<11)+threadIdx.x;
	RF[0]=d_RF[lane];
	RF[0].y=-RF[0].y;
	mLOAD16(hc,d_c,128,)
	mH2Sx16(c,hc)
	mCALRF16(RF)
	mFFT16(c,i)
	mPERMUTE(16,spx,spy,c,168,8,0xf)
	mFFT16(c,i)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spy,spz,c,9,72,1,0xf)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mPERMUTE_S8x2_L16(spy,spx,c,8,16,168,0x7)
	mS2Hx16(hc,c)
	mSTORE16(d_c,hc,128,)
}
__global__ void d_mfft128x64x( ushort2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[168*64];
	float2 c[16], RF[15], temp;
	ushort2 hc[16];
	unsigned int tidx=(threadIdx.y<<7)+threadIdx.x;
	unsigned int lane=threadIdx.x&7;
	unsigned int slot=(threadIdx.y<<3)+(threadIdx.x>>3);
	float* spx=&smem[ 168*threadIdx.y+threadIdx.x];
	float* spy=&smem[1344*threadIdx.y+threadIdx.x];
	float* spu=&smem[168*slot+  lane];
	float* spv=&smem[168*slot+9*lane];
	d_c+=(blockIdx.x<<13)+tidx;
	RF[0]=d_RF[threadIdx.y<<1];
	mLOAD8x2(hc,d_c,64,1024,)
	mH2Sx16(c,hc)
	mCALRF8(RF)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mHMRF8(&c[0],RF)
	mHMRF8(&c[8],RF)
	mPERMUTE8x2(spx,spy,c,64,1344,64,168,0xf)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	RF[0]=d_RF[lane];
	mCALRF16(RF)
	mPERMUTE_S8x2_L16(spy,spu,c,64,168,8,0xf)
	mFFT16(c,)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spu,spv,c,9,72,1,0xf)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mPERMUTE8x2(spu,spy,c,8,16,64,168,0x7)
	mS2Hx16(hc,c)
	mSTORE8x2(d_c,hc,64,1024,)
}
__global__ void d_imfft128x64x( ushort2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[168*64];
	float2 c[16], RF[15], temp;
	ushort2 hc[16];
	unsigned int tidx=(threadIdx.y<<7)+threadIdx.x;
	unsigned int lane=threadIdx.x&7;
	unsigned int slot=(threadIdx.y<<3)+(threadIdx.x>>3);
	float* spx=&smem[ 168*threadIdx.y+threadIdx.x];
	float* spy=&smem[1344*threadIdx.y+threadIdx.x];
	float* spu=&smem[168*slot+  lane];
	float* spv=&smem[168*slot+9*lane];
	d_c+=(blockIdx.x<<13)+tidx;
	RF[0]=d_RF[threadIdx.y<<1];
	RF[0].y=-RF[0].y;
	mLOAD8x2(hc,d_c,64,1024,)
	mH2Sx16(c,hc)
	mCALRF8(RF)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mHMRF8(&c[0],RF)
	mHMRF8(&c[8],RF)
	mPERMUTE8x2(spx,spy,c,64,1344,64,168,0xf)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	RF[0]=d_RF[lane];
	RF[0].y=-RF[0].y;
	mCALRF16(RF)
	mPERMUTE_S8x2_L16(spy,spu,c,64,168,8,0xf)
	mFFT16(c,i)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spu,spv,c,9,72,1,0xf)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mPERMUTE8x2(spu,spy,c,8,16,64,168,0x7)
	mS2Hx16(hc,c)
	mSTORE8x2(d_c,hc,64,1024,)
}
