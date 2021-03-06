#include"hfft.h"
#include"cuintrin.h"

/*
8x2   : block( n,1)
8x4   : block( 8,n)
8x8   : block( 8,n)
8x16  : block( 8,n)
8x32  : block(32,n)
8x64  : block(64,1)
8x128 : block(64,1)
*/
__global__ void d_fft8x2x( float2* d_c, int bat )
{													
	float2 c[16], temp;						
	unsigned int tidx=blockIdx.x*blockDim.x+threadIdx.x;
	if(tidx>=bat) return;
	d_c+=(tidx<<4)+threadIdx.x;
	mLOAD16(c,d_c,1,)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	BFLYU10(c[0],c[ 8])
	BFLYU10(c[1],c[ 9])
	BFLYU10(c[2],c[10])
	BFLYU10(c[3],c[11])
	BFLYU10(c[4],c[12])
	BFLYU10(c[5],c[13])
	BFLYU10(c[6],c[14])
	BFLYU10(c[7],c[15])
	mISTORE8x2(d_c,c,8,1,)
}
__global__ void d_ifft8x2x( float2* d_c, int bat )
{													
	float2 c[16], temp;						
	unsigned int tidx=blockIdx.x*blockDim.x+threadIdx.x;
	if(tidx>=bat) return;
	d_c+=(tidx<<4)+threadIdx.x;
	mLOAD16(c,d_c,1,)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	BFLYU10(c[0],c[ 8])
	BFLYU10(c[1],c[ 9])
	BFLYU10(c[2],c[10])
	BFLYU10(c[3],c[11])
	BFLYU10(c[4],c[12])
	BFLYU10(c[5],c[13])
	BFLYU10(c[6],c[14])
	BFLYU10(c[7],c[15])
	mISTORE8x2(d_c,c,8,1,)
}
__global__ void d_fft8x4x( float2* d_c, const float2* d_RF, int bat )
{											
	extern __shared__ float smem[];		
	float2 c[4], RF[3], temp;						
	unsigned int slice_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(slice_id>=bat) return;
	d_c+=(slice_id<<5)+threadIdx.x;
	unsigned int lane=threadIdx.x&1;
	unsigned int slot=threadIdx.x>>1;
	float* sbase=&smem[72*threadIdx.y];
	float* spx=&sbase[threadIdx.x];
	float* spy=&sbase[18*slot+lane];
	float* spz=&sbase[18*slot+3*lane];
	RF[0]=d_RF[lane];
	mLOAD4(c,d_c,8,)
	mCALRF4(RF)
	mFFT4(c,)
	mPERMUTE(4,spx,spy,c,18,2,0)
	mFFT4(c,)
	mHMRF4(c,RF)
	mISTORE4(spy,c,3,.x)
	mLOAD2(&c[0],&spz[0],1,.x)
	mLOAD2(&c[2],&spz[4],1,.x)
	mISTORE4(spy,c,3,.y)
	mLOAD2(&c[0],&spz[0],1,.y)
	mLOAD2(&c[2],&spz[4],1,.y)
	mFFT2(&c[0],)
	mFFT2(&c[2],)
	mPERMUTE(4,spy,spx,c,2,18,0)
	mSTORE4(d_c,c,8,)						
}
__global__ void d_ifft8x4x( float2* d_c, const float2* d_RF, int bat )
{											
	extern __shared__ float smem[];		
	float2 c[4], RF[3], temp;						
	unsigned int slice_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(slice_id>=bat) return;
	d_c+=(slice_id<<5)+threadIdx.x;
	unsigned int lane=threadIdx.x&1;
	unsigned int slot=threadIdx.x>>1;
	float* sbase=&smem[72*threadIdx.y];
	float* spx=&sbase[threadIdx.x];
	float* spy=&sbase[18*slot+lane];
	float* spz=&sbase[18*slot+3*lane];
	RF[0]=d_RF[lane];
	RF[0].y=-RF[0].y;
	mLOAD4(c,d_c,8,)	
	mCALRF4(RF)
	mFFT4(c,i)
	mPERMUTE(4,spx,spy,c,18,2,0)
	mFFT4(c,i)
	mHMRF4(c,RF)
	mISTORE4(spy,c,3,.x)
	mLOAD2(&c[0],&spz[0],1,.x)
	mLOAD2(&c[2],&spz[4],1,.x)
	mISTORE4(spy,c,3,.y)
	mLOAD2(&c[0],&spz[0],1,.y)
	mLOAD2(&c[2],&spz[4],1,.y)
	mFFT2(&c[0],)
	mFFT2(&c[2],)
	mPERMUTE(4,spy,spx,c,2,18,0)
	mSTORE4(d_c,c,8,)
}
__global__ void d_fft8x8x( float2* d_c, int bat )
{											
	extern __shared__ float smem[];		
	float2 c[8], temp;						
	unsigned int slice_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(slice_id>=bat) return;
	d_c+=(slice_id<<6)+threadIdx.x;	
	float* sst=&smem[72*threadIdx.y+threadIdx.x];			
	float* sld=&smem[72*threadIdx.y+9*threadIdx.x];		
	mLOAD8(c,d_c,8,)						
	mFFT8(c,)							
	mPERMUTE(8,sst,sld,c,9,1,0)			
	mFFT8(c,)						
	mPERMUTE(8,sst,sld,c,9,1,0)			
	mSTORE8(d_c,c,8,)						
}
__global__ void d_ifft8x8x( float2* d_c, int bat )
{											
	extern __shared__ float smem[];		
	float2 c[8], temp;						
	unsigned int slice_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(slice_id>=bat) return;
	d_c+=(slice_id<<6)+threadIdx.x;	
	float* sst=&smem[72*threadIdx.y+threadIdx.x];
	float* sld=&smem[72*threadIdx.y+9*threadIdx.x];
	mLOAD8(c,d_c,8,)						
	mFFT8(c,i);							
	mPERMUTE(8,sst,sld,c,9,1,0);			
	mFFT8(c,i);							
	mPERMUTE(8,sst,sld,c,9,1,0);			
	mSTORE8(d_c,c,8,)						
}
__global__ void d_fft8x16x( float2* d_c, int bat )
{											
	extern __shared__ float smem[];		
	float2 c[16], temp;						
	unsigned int slice_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(slice_id>=bat) return;
	d_c+=(slice_id<<7)+threadIdx.x;	
	float* spx=&smem[168*threadIdx.y+threadIdx.x];			
	float* spy=&smem[168*threadIdx.y+9*threadIdx.x];
	mLOAD16(c,d_c,8,)						
	mFFT16(c,)
	mPERMUTE_S16_L8x2(spx,spy,c,9,72,1,0)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mPERMUTE_S8x2_L16(spy,spx,c,72,1,9,0)
	mSTORE16(d_c,c,8,)						
}
__global__ void d_ifft8x16x( float2* d_c, int bat )
{											
	extern __shared__ float smem[];		
	float2 c[16], temp;						
	unsigned int slice_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(slice_id>=bat) return;
	d_c+=(slice_id<<7)+threadIdx.x;	
	float* spx=&smem[168*threadIdx.y+threadIdx.x];			
	float* spy=&smem[168*threadIdx.y+9*threadIdx.x];
	mLOAD16(c,d_c,8,)						
	mFFT16(c,i)	
	mPERMUTE_S16_L8x2(spx,spy,c,9,72,1,0)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mPERMUTE_S8x2_L16(spy,spx,c,72,1,9,0)
	mSTORE16(d_c,c,8,)						
}
/*bank conflict*/
__global__ void d_fft8x32x( float2* d_c, const float2* d_RF, int bat )
{
	extern __shared__ float smem[];
	float2 c[8], RF[7], temp;
	unsigned int patch_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(patch_id>=bat) return;
	unsigned int slot=threadIdx.x>>3;
	unsigned int lane=threadIdx.x&7;
	float* sbase=&smem[threadIdx.y*320];
	float* spx=&sbase[ threadIdx.x];
	float* spy=&sbase[40*slot+lane];
	float* spz=&sbase[40*slot+9*lane];
	d_c+=(patch_id<<8)+threadIdx.x;
	RF[0]=d_RF[slot];
	mCALRF8(RF)
	mLOAD8(c,d_c,32,)
	mFFT8(c,)
	mHMRF8(c,RF)
	mISTORE8(spx,c,40,.x)
	mLOAD4x2(c,spy,160,8,.x)
	mISTORE8(spx,c,40,.y)
	mLOAD4x2(c,spy,160,8,.y)
	mFFT4(&c[0],)
	mFFT4(&c[4],)
	mISTORE4x2(spy,c,160,9,.x)
	mLOAD8(c,spz,1,.x)
	mISTORE4x2(spy,c,160,9,.y)
	mLOAD8(c,spz,1,.y)
	mFFT8(c,)
	mISTORE8(spz,c,1,.x)
	mLOAD4x2(c,spy,160,9,.x)
	mISTORE8(spz,c,1,.y)
	mLOAD4x2(c,spy,160,9,.y)
	mSTORE4x2(d_c,c,32,64)
}
__global__ void d_ifft8x32x( float2* d_c, const float2* d_RF, int bat )
{
	extern __shared__ float smem[];
	float2 c[8], RF[7], temp;
	unsigned int patch_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(patch_id>=bat) return;
	unsigned int slot=threadIdx.x>>3;
	unsigned int lane=threadIdx.x&7;
	float* sbase=&smem[threadIdx.y*320];
	float* spx=&sbase[ threadIdx.x];
	float* spy=&sbase[40*slot+lane];
	float* spz=&sbase[40*slot+9*lane];
	d_c+=(patch_id<<8)+threadIdx.x;
	RF[0]=d_RF[slot];
	RF[0].y=-RF[0].y;
	mCALRF8(RF)
	mLOAD8(c,d_c,32,)
	mFFT8(c,i)
	mHMRF8(c,RF)
	mISTORE8(spx,c,40,.x)
	mLOAD4x2(c,spy,160,8,.x)
	mISTORE8(spx,c,40,.y)
	mLOAD4x2(c,spy,160,8,.y)
	mFFT4(&c[0],i)
	mFFT4(&c[4],i)
	mISTORE4x2(spy,c,160,9,.x)
	mLOAD8(c,spz,1,.x)
	mISTORE4x2(spy,c,160,9,.y)
	mLOAD8(c,spz,1,.y)
	mFFT8(c,i)
	mISTORE8(spz,c,1,.x)
	mLOAD4x2(c,spy,160,9,.x)
	mISTORE8(spz,c,1,.y)
	mLOAD4x2(c,spy,160,9,.y)
	mSTORE4x2(d_c,c,32,64)
}
__global__ void d_fft8x64x( float2* d_c, const float2* d_RF )
{
	__shared__ float smem[576];
	float2 c[8], RF[7], temp;
	unsigned int slot=threadIdx.x>>3;
	unsigned int lane=threadIdx.x&7;
	float* spx=&smem[ threadIdx.x];
	float* spy=&smem[72*slot+lane];
	float* spz=&smem[72*slot+9*lane];
	d_c+=(blockIdx.x<<9)+threadIdx.x;
	RF[0]=d_RF[slot];
	mCALRF8(RF)
	mLOAD8(c,d_c,64,)
	mFFT8(c,)
	mHMRF8(c,RF)
	mPERMUTE(8,spx,spy,c,72,8,0xf)
	mFFT8(c,)
	mPERMUTE(8,spy,spz,c,9,1,0xf)
	mFFT8(c,)
	mPERMUTE(8,spz,spy,c,1,9,0x7)
	mSTORE8(d_c,c,64,)
}
__global__ void d_ifft8x64x( float2* d_c, const float2* d_RF )
{
	__shared__ float smem[576];
	float2 c[8], RF[7], temp;
	unsigned int slot=threadIdx.x>>3;
	unsigned int lane=threadIdx.x&7;
	float* spx=&smem[ threadIdx.x];
	float* spy=&smem[72*slot+lane];
	float* spz=&smem[72*slot+9*lane];
	d_c+=(blockIdx.x<<9)+threadIdx.x;
	RF[0]=d_RF[slot];
	RF[0].y=-RF[0].y;
	mCALRF8(RF)
	mLOAD8(c,d_c,64,)
	mFFT8(c,i)
	mHMRF8(c,RF)
	mPERMUTE(8,spx,spy,c,72,8,0xf)
	mFFT8(c,i)
	mPERMUTE(8,spy,spz,c,9,1,0xf)
	mFFT8(c,i)
	mPERMUTE(8,spz,spy,c,1,9,0x7)
	mSTORE8(d_c,c,64,)
}
__global__ void d_fft8x128x( float2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[16*72];
	float2 c[16], RF[15], temp;
	d_c+=(blockIdx.x<<10)+threadIdx.x;
	unsigned int slot=threadIdx.x>>3;
	unsigned int lane=threadIdx.x&7;
	float* spx=&smem[threadIdx.x];
	float* spy=&smem[72*slot+lane];
	float* spz=&smem[72*slot+9*lane];
	RF[0]=d_RF[slot];
	mLOAD16(c,d_c,64,)
	mCALRF16(RF)
	mFFT16(c,)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spx,spy,c,72,576,8,0xf)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mPERMUTE8x2(spy,spz,c,576,9,576,1,0xf)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mPERMUTE8x2(spz,spy,c,576,1,576,9,0x7)
	mSTORE8x2(d_c,c,64,128,)
}
__global__ void d_ifft8x128x( float2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[16*72];
	float2 c[16], RF[15], temp;
	d_c+=(blockIdx.x<<10)+threadIdx.x;
	unsigned int slot=threadIdx.x>>3;
	unsigned int lane=threadIdx.x&7;
	float* spx=&smem[threadIdx.x];
	float* spy=&smem[72*slot+lane];
	float* spz=&smem[72*slot+9*lane];
	RF[0]=d_RF[slot];
	RF[0].y=-RF[0].y;
	mLOAD16(c,d_c,64,)
	mCALRF16(RF)
	mFFT16(c,i)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spx,spy,c,72,576,8,0xf)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mPERMUTE8x2(spy,spz,c,576,9,576,1,0xf)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mPERMUTE8x2(spz,spy,c,576,1,576,9,0x7)
	mSTORE8x2(d_c,c,64,128,)
}

//==============================================================================================================================================================

__global__ void d_mfft8x2x( ushort2* d_c, int bat )
{													
	float2 c[16], temp;
	ushort2 hc[16];
	unsigned int tidx=blockIdx.x*blockDim.x+threadIdx.x;
	if(tidx>=bat) return;
	d_c+=(tidx<<4)+threadIdx.x;
	mLOAD16(hc,d_c,1,)
	mH2Sx16(c,hc)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	BFLYU10(c[0],c[ 8])
	BFLYU10(c[1],c[ 9])
	BFLYU10(c[2],c[10])
	BFLYU10(c[3],c[11])
	BFLYU10(c[4],c[12])
	BFLYU10(c[5],c[13])
	BFLYU10(c[6],c[14])
	BFLYU10(c[7],c[15])
	mS2Hx16(hc,c)
	mISTORE8x2(d_c,hc,8,1,)
}
__global__ void d_imfft8x2x( ushort2* d_c, int bat )
{													
	float2 c[16], temp;
	ushort2 hc[16];
	unsigned int tidx=blockIdx.x*blockDim.x+threadIdx.x;
	if(tidx>=bat) return;
	d_c+=(tidx<<4)+threadIdx.x;
	mLOAD16(hc,d_c,1,)
	mH2Sx16(c,hc)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	BFLYU10(c[0],c[ 8])
	BFLYU10(c[1],c[ 9])
	BFLYU10(c[2],c[10])
	BFLYU10(c[3],c[11])
	BFLYU10(c[4],c[12])
	BFLYU10(c[5],c[13])
	BFLYU10(c[6],c[14])
	BFLYU10(c[7],c[15])
	mS2Hx16(hc,c)
	mISTORE8x2(d_c,hc,8,1,)
}
__global__ void d_mfft8x4x( ushort2* d_c, const float2* d_RF, int bat )
{											
	extern __shared__ float smem[];		
	float2 c[4], RF[3], temp;
	ushort2 hc[4];
	unsigned int slice_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(slice_id>=bat) return;
	d_c+=(slice_id<<5)+threadIdx.x;
	unsigned int lane=threadIdx.x&1;
	unsigned int slot=threadIdx.x>>1;
	float* sbase=&smem[72*threadIdx.y];
	float* spx=&sbase[threadIdx.x];
	float* spy=&sbase[18*slot+lane];
	float* spz=&sbase[18*slot+3*lane];
	RF[0]=d_RF[lane];
	mLOAD4(hc,d_c,8,)
	mH2Sx4(c,hc)
	mCALRF4(RF)
	mFFT4(c,)
	mPERMUTE(4,spx,spy,c,18,2,0)
	mFFT4(c,)
	mHMRF4(c,RF)
	mISTORE4(spy,c,3,.x)
	mLOAD2(&c[0],&spz[0],1,.x)
	mLOAD2(&c[2],&spz[4],1,.x)
	mISTORE4(spy,c,3,.y)
	mLOAD2(&c[0],&spz[0],1,.y)
	mLOAD2(&c[2],&spz[4],1,.y)
	mFFT2(&c[0],)
	mFFT2(&c[2],)
	mPERMUTE(4,spy,spx,c,2,18,0)
	mS2Hx4(hc,c)
	mSTORE4(d_c,hc,8,)						
}
__global__ void d_imfft8x4x( ushort2* d_c, const float2* d_RF, int bat )
{											
	extern __shared__ float smem[];		
	float2 c[4], RF[3], temp;
	ushort2 hc[4];
	unsigned int slice_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(slice_id>=bat) return;
	d_c+=(slice_id<<5)+threadIdx.x;
	unsigned int lane=threadIdx.x&1;
	unsigned int slot=threadIdx.x>>1;
	float* sbase=&smem[72*threadIdx.y];
	float* spx=&sbase[threadIdx.x];
	float* spy=&sbase[18*slot+lane];
	float* spz=&sbase[18*slot+3*lane];
	RF[0]=d_RF[lane];
	RF[0].y=-RF[0].y;
	mLOAD4(hc,d_c,8,)
	mH2Sx4(c,hc)
	mCALRF4(RF)
	mFFT4(c,i)
	mPERMUTE(4,spx,spy,c,18,2,0)
	mFFT4(c,i)
	mHMRF4(c,RF)
	mISTORE4(spy,c,3,.x)
	mLOAD2(&c[0],&spz[0],1,.x)
	mLOAD2(&c[2],&spz[4],1,.x)
	mISTORE4(spy,c,3,.y)
	mLOAD2(&c[0],&spz[0],1,.y)
	mLOAD2(&c[2],&spz[4],1,.y)
	mFFT2(&c[0],)
	mFFT2(&c[2],)
	mPERMUTE(4,spy,spx,c,2,18,0)
	mS2Hx4(hc,c)
	mSTORE4(d_c,hc,8,)
}
__global__ void d_mfft8x8x( ushort2* d_c, int bat )
{			
	extern __shared__ float smem[];		
	float2 c[8], temp;
	ushort2 hc[8];
	unsigned int slice_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(slice_id>=bat) return;
	d_c+=(slice_id<<6)+threadIdx.x;	
	float* sst=&smem[72*threadIdx.y+threadIdx.x];			
	float* sld=&smem[72*threadIdx.y+9*threadIdx.x];		
	mLOAD8(hc,d_c,8,)
	mH2Sx8(c,hc)
	mFFT8(c,)							
	mPERMUTE(8,sst,sld,c,9,1,0)			
	mFFT8(c,)						
	mPERMUTE(8,sst,sld,c,9,1,0)
	mS2Hx8(hc,c)
	mSTORE8(d_c,hc,8,)						
}
__global__ void d_imfft8x8x( ushort2* d_c, int bat )
{											
	extern __shared__ float smem[];		
	float2 c[8], temp;
	ushort2 hc[8];
	unsigned int slice_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(slice_id>=bat) return;
	d_c+=(slice_id<<6)+threadIdx.x;	
	float* sst=&smem[72*threadIdx.y+threadIdx.x];
	float* sld=&smem[72*threadIdx.y+9*threadIdx.x];
	mLOAD8(hc,d_c,8,)
	mH2Sx8(c,hc)
	mFFT8(c,i);							
	mPERMUTE(8,sst,sld,c,9,1,0)			
	mFFT8(c,i);							
	mPERMUTE(8,sst,sld,c,9,1,0)
	mS2Hx8(hc,c)
	mSTORE8(d_c,hc,8,)						
}
__global__ void d_mfft8x16x( ushort2* d_c, int bat )
{											
	extern __shared__ float smem[];		
	float2 c[16], temp;
	ushort2 hc[16];
	unsigned int slice_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(slice_id>=bat) return;
	d_c+=(slice_id<<7)+threadIdx.x;	
	float* spx=&smem[168*threadIdx.y+threadIdx.x];			
	float* spy=&smem[168*threadIdx.y+9*threadIdx.x];
	mLOAD16(hc,d_c,8,)
	mH2Sx16(c,hc)
	mFFT16(c,)
	mPERMUTE_S16_L8x2(spx,spy,c,9,72,1,0)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mPERMUTE_S8x2_L16(spy,spx,c,72,1,9,0)
	mS2Hx16(hc,c)
	mSTORE16(d_c,hc,8,)						
}
__global__ void d_imfft8x16x( ushort2* d_c, int bat )
{											
	extern __shared__ float smem[];		
	float2 c[16], temp;
	ushort2 hc[16];
	unsigned int slice_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(slice_id>=bat) return;
	d_c+=(slice_id<<7)+threadIdx.x;	
	float* spx=&smem[168*threadIdx.y+threadIdx.x];			
	float* spy=&smem[168*threadIdx.y+9*threadIdx.x];
	mLOAD16(hc,d_c,8,)
	mH2Sx16(c,hc)
	mFFT16(c,i)	
	mPERMUTE_S16_L8x2(spx,spy,c,9,72,1,0)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mPERMUTE_S8x2_L16(spy,spx,c,72,1,9,0)
	mS2Hx16(hc,c)
	mSTORE16(d_c,hc,8,)						
}
__global__ void d_mfft8x32x( ushort2* d_c, const float2* __restrict__ d_RF, int bat )
{
	extern __shared__ float smem[];
	float2 c[8], RF[7], temp;
	ushort2 hc[8];
	unsigned int patch_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(patch_id>=bat) return;
	unsigned int slot=threadIdx.x>>3;
	unsigned int lane=threadIdx.x&7;
	float* sbase=&smem[threadIdx.y*320];
	float* spx=&sbase[ threadIdx.x];
	float* spy=&sbase[40*slot+lane];
	float* spz=&sbase[40*slot+9*lane];
	d_c+=(patch_id<<8)+threadIdx.x;
	RF[0]=d_RF[slot];
	mCALRF8(RF)
	mLOAD8(hc,d_c,32,)
	mH2Sx8(c,hc)
	mFFT8(c,)
	mHMRF8(c,RF)
	mISTORE8(spx,c,40,.x)
	mLOAD4x2(c,spy,160,8,.x)
	mISTORE8(spx,c,40,.y)
	mLOAD4x2(c,spy,160,8,.y)
	mFFT4(&c[0],)
	mFFT4(&c[4],)
	mISTORE4x2(spy,c,160,9,.x)
	mLOAD8(c,spz,1,.x)
	mISTORE4x2(spy,c,160,9,.y)
	mLOAD8(c,spz,1,.y)
	mFFT8(c,)
	mISTORE8(spz,c,1,.x)
	mLOAD4x2(c,spy,160,9,.x)
	mISTORE8(spz,c,1,.y)
	mLOAD4x2(c,spy,160,9,.y)
	mS2Hx8(hc,c)
	mSTORE4x2(d_c,hc,32,64)
}
__global__ void d_imfft8x32x( ushort2* d_c, const float2* __restrict__ d_RF, int bat )
{
	extern __shared__ float smem[];
	float2 c[8], RF[7], temp;
	ushort2 hc[8];
	unsigned int patch_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(patch_id>=bat) return;
	unsigned int slot=threadIdx.x>>3;
	unsigned int lane=threadIdx.x&7;
	float* sbase=&smem[threadIdx.y*320];
	float* spx=&sbase[ threadIdx.x];
	float* spy=&sbase[40*slot+lane];
	float* spz=&sbase[40*slot+9*lane];
	d_c+=(patch_id<<8)+threadIdx.x;
	RF[0]=d_RF[slot];
	RF[0].y=-RF[0].y;
	mCALRF8(RF)
	mLOAD8(hc,d_c,32,)
	mH2Sx8(c,hc)
	mFFT8(c,i)
	mHMRF8(c,RF)
	mISTORE8(spx,c,40,.x)
	mLOAD4x2(c,spy,160,8,.x)
	mISTORE8(spx,c,40,.y)
	mLOAD4x2(c,spy,160,8,.y)
	mFFT4(&c[0],i)
	mFFT4(&c[4],i)
	mISTORE4x2(spy,c,160,9,.x)
	mLOAD8(c,spz,1,.x)
	mISTORE4x2(spy,c,160,9,.y)
	mLOAD8(c,spz,1,.y)
	mFFT8(c,i)
	mISTORE8(spz,c,1,.x)
	mLOAD4x2(c,spy,160,9,.x)
	mISTORE8(spz,c,1,.y)
	mLOAD4x2(c,spy,160,9,.y)
	mS2Hx8(hc,c)
	mSTORE4x2(d_c,hc,32,64)
}
__global__ void d_mfft8x64x( ushort2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[576];
	float2 c[8], RF[7], temp;
	ushort2 hc[8];
	unsigned int slot=threadIdx.x>>3;
	unsigned int lane=threadIdx.x&7;
	float* spx=&smem[ threadIdx.x];
	float* spy=&smem[72*slot+lane];
	float* spz=&smem[72*slot+9*lane];
	d_c+=(blockIdx.x<<9)+threadIdx.x;
	RF[0]=d_RF[slot];
	mCALRF8(RF)
	mLOAD8(hc,d_c,64,)
	mH2Sx8(c,hc)
	mFFT8(c,)
	mHMRF8(c,RF)
	mPERMUTE(8,spx,spy,c,72,8,0xf)
	mFFT8(c,)
	mPERMUTE(8,spy,spz,c,9,1,0xf)
	mFFT8(c,)
	mPERMUTE(8,spz,spy,c,1,9,0x7)
	mS2Hx8(hc,c)
	mSTORE8(d_c,hc,64,)
}
__global__ void d_imfft8x64x( ushort2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[576];
	float2 c[8], RF[7], temp;
	ushort2 hc[8];
	unsigned int slot=threadIdx.x>>3;
	unsigned int lane=threadIdx.x&7;
	float* spx=&smem[ threadIdx.x];
	float* spy=&smem[72*slot+lane];
	float* spz=&smem[72*slot+9*lane];
	d_c+=(blockIdx.x<<9)+threadIdx.x;
	RF[0]=d_RF[slot];
	RF[0].y=-RF[0].y;
	mCALRF8(RF)
	mLOAD8(hc,d_c,64,)
	mH2Sx8(c,hc)
	mFFT8(c,i)
	mHMRF8(c,RF)
	mPERMUTE(8,spx,spy,c,72,8,0xf)
	mFFT8(c,i)
	mPERMUTE(8,spy,spz,c,9,1,0xf)
	mFFT8(c,i)
	mPERMUTE(8,spz,spy,c,1,9,0x7)
	mS2Hx8(hc,c)
	mSTORE8(d_c,hc,64,)
}
__global__ void d_mfft8x128x( ushort2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[16*72];
	float2 c[16], RF[15], temp;
	ushort2 hc[16];
	d_c+=(blockIdx.x<<10)+threadIdx.x;
	unsigned int slot=threadIdx.x>>3;
	unsigned int lane=threadIdx.x&7;
	float* spx=&smem[threadIdx.x];
	float* spy=&smem[72*slot+lane];
	float* spz=&smem[72*slot+9*lane];
	RF[0]=d_RF[slot];
	mLOAD16(hc,d_c,64,)
	mH2Sx16(c,hc)
	mCALRF16(RF)
	mFFT16(c,)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spx,spy,c,72,576,8,0xf)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mPERMUTE8x2(spy,spz,c,576,9,576,1,0xf)
	mFFT8(&c[0],)
	mFFT8(&c[8],)
	mPERMUTE8x2(spz,spy,c,576,1,576,9,0x7)
	mS2Hx16(hc,c)
	mSTORE8x2(d_c,hc,64,128,)
}
__global__ void d_imfft8x128x( ushort2* d_c, const float2* __restrict__ d_RF )
{
	__shared__ float smem[16*72];
	float2 c[16], RF[15], temp;
	ushort2 hc[16];
	d_c+=(blockIdx.x<<10)+threadIdx.x;
	unsigned int slot=threadIdx.x>>3;
	unsigned int lane=threadIdx.x&7;
	float* spx=&smem[threadIdx.x];
	float* spy=&smem[72*slot+lane];
	float* spz=&smem[72*slot+9*lane];
	RF[0]=d_RF[slot];
	RF[0].y=-RF[0].y;
	mLOAD16(hc,d_c,64,)
	mH2Sx16(c,hc)
	mCALRF16(RF)
	mFFT16(c,i)
	mHMRF16(c,RF)
	mPERMUTE_S16_L8x2(spx,spy,c,72,576,8,0xf)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mPERMUTE8x2(spy,spz,c,576,9,576,1,0xf)
	mFFT8(&c[0],i)
	mFFT8(&c[8],i)
	mPERMUTE8x2(spz,spy,c,576,1,576,9,0x7)
	mS2Hx16(hc,c)
	mSTORE8x2(d_c,hc,64,128,)
}
