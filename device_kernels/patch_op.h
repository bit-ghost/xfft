typedef struct patchconv_param{	
	unsigned int dat_nx;
	unsigned int dat_ny;
	unsigned int ker_nx;
	unsigned int ker_ny;
	unsigned int out_nx;
	unsigned int out_ny;
	unsigned int valid_nx;
	unsigned int valid_ny;
	unsigned int patch_nx;
	unsigned int patch_ny;	
	unsigned int npx;
	unsigned int npy;
	unsigned int dat_pitch;
	unsigned int out_pitch;
} patchconv_param_t;

__constant__ static patchconv_param_t c_patchconv_params;

__device__ __forceinline__ void d_postproc( float2& a, float2& b, const float2& RF )
{
	float hax=0.5f*a.x;
	float hay=0.5f*a.y;
    float p0=( 0.5f)*b.x+hax;
    float p1=(-0.5f)*b.y+hay;
    float q0=( 0.5f)*b.y+hay;
    float q1=(-0.5f)*b.x+hax;
    a.x=__fmaf_rn( q0, RF.x, __fmaf_rn( q1, RF.y, p0));
    a.y=__fmaf_rn( q0, RF.y, __fmaf_rn(-q1, RF.x, p1));
    b.x=__fmaf_rn(-q0, RF.x, __fmaf_rn(-q1, RF.y, p0));
    b.y=__fmaf_rn( q0, RF.y, __fmaf_rn(-q1, RF.x,-p1));
}
__device__ __forceinline__ void d_preproc( float2& a, float2& b, const float2& RF )
{
    float p0=a.x+b.x;
    float p1=a.y-b.y;
    float q0=a.y+b.y;
    float q1=a.x-b.x;
    a.x=__fmaf_rn(-q0, RF.x, __fmaf_rn( q1, RF.y, p0));
    a.y=__fmaf_rn( q1, RF.x, __fmaf_rn( q0, RF.y, p1));
    b.x=__fmaf_rn( q0, RF.x, __fmaf_rn(-q1, RF.y, p0));
    b.y=__fmaf_rn( q1, RF.x, __fmaf_rn( q0, RF.y,-p1));
}
__device__ __forceinline__ float2 d_cmul( const float2& a, const float2& b, float c, float sign )
{
	float ay=sign*a.y;
	return make_float2(c*(a.x*b.x-ay*b.y),c*(a.x*b.y+ay*b.x));
}
__device__ __forceinline__ void operator+=( float2& a, const float2& b )
{
	a.x+=b.x; a.y+=b.y;
}

__device__ __forceinline__ void d_padding_patch( float* d_dst, const float* d_src, unsigned int px, unsigned int py, unsigned int src_pitch, unsigned int valid_nx, unsigned int valid_ny, unsigned int patch_nx, unsigned int patch_ny )
{
	unsigned int y=0;
	const unsigned int cx=(threadIdx.x>=px)&(threadIdx.x<valid_nx);
	do{
		float e=0.f;
		if(cx&(y>=py)&(y<valid_ny)){
			e=d_src[0]; d_src+=src_pitch;
		}
		d_dst[0]=e; d_dst+=patch_nx;
	}while((++y)<patch_ny);
}
__device__ __forceinline__ void d_postpreacc_patch( float2* d_out, const float2 *d_dat, const float2* d_ker, const float2* d_RF, unsigned int x, unsigned int y, unsigned int patch_nx, unsigned int patch_ny, unsigned int patch_size, unsigned int nppc, unsigned int n_patches, float scale, float sign )
{
	unsigned int h, p, q, u, v;
	float2 a0, a1, b0, b1, c0, c1, s0, s1, s2, s3;
	s0=make_float2(0.f,0.f);
	s1=make_float2(0.f,0.f);
	s2=make_float2(0.f,0.f);
	s3=make_float2(0.f,0.f);
	const float2 RF=d_RF[x];
	const unsigned int slice_size=nppc*patch_size;
	h=patch_ny>>1;	
	p=y*patch_nx+x;
	q=(y?(patch_ny-y):y)*patch_nx+(x?(patch_nx-x):x);
	if(y==0){
		u=h*patch_nx+x;
		v=h*patch_nx+(x?(patch_nx-x):x);	
	}
	for( int i=0; i<n_patches; ++i )
	{	
		a0=d_dat[p];
		a1=d_dat[q];		
		b0=d_ker[p];
		b1=d_ker[q];
		d_postproc(a0,a1,RF);
		d_postproc(b0,b1,RF);
		c0=d_cmul(a0,b0,scale,sign);
		c1=d_cmul(a1,b1,scale,sign);
		d_preproc(c0,c1,RF);
		s0+=c0; s1+=c1;
		if(y==0)
		{
			a0=d_dat[u];
			a1=d_dat[v];		
			b0=d_ker[u];
			b1=d_ker[v];
			d_postproc(a0,a1,RF);
			d_postproc(b0,b1,RF);
			c0=d_cmul(a0,b0,scale,sign);
			c1=d_cmul(a1,b1,scale,sign);
			d_preproc(c0,c1,RF);
			s2+=c0; s3+=c1;
		}
		d_dat+=slice_size, d_ker+=patch_size;
	}
	d_out[p]=s0;
	d_out[q]=s1;
	if(y==0){
		d_out[u]=s2;
		d_out[v]=s3;
	}
}

__global__ void d_get_patches( float* d_dst, const float* d_src, int padding )
{
	unsigned int dat_nx, dat_ny, ker_nx, ker_ny, valid_nx, valid_ny, patch_nx, patch_ny, npx, npy, src_pitch, icp, nppc, ipx, ipy, ox, oy, px, py, qx, qy;

	icp=blockIdx.x*blockDim.y+threadIdx.y;
	dat_nx		=c_patchconv_params.dat_nx;
	dat_ny		=c_patchconv_params.dat_ny;
	ker_nx		=c_patchconv_params.ker_nx;
	ker_ny		=c_patchconv_params.ker_ny;
	valid_nx	=c_patchconv_params.valid_nx;
	valid_ny	=c_patchconv_params.valid_ny;
	patch_nx	=c_patchconv_params.patch_nx;
	patch_ny	=c_patchconv_params.patch_ny;
	npx			=c_patchconv_params.npx;
	npy			=c_patchconv_params.npy;	
	src_pitch	=c_patchconv_params.dat_pitch;
	if(icp>=(nppc=npx*npy)) return;

	ipx=icp%npx;
	ipy=icp/npx;	
	ox=ker_nx-1;
	oy=ker_ny-1;
	ox=padding?ox:0;
	oy=padding?oy:0;
	px=(ipx==0)?ox:0;
	py=(ipy==0)?oy:0;	
	qx=(ipx==0)?0:(ipx*valid_nx-ox);
	qy=(ipy==0)?0:(ipy*valid_ny-oy);
	--npx; --npy;
	valid_nx=(ipx<npx)?patch_nx:(dat_nx-npx*valid_nx+ox);
	valid_ny=(ipy<npy)?patch_ny:(dat_ny-npy*valid_ny+oy);
	d_dst+=(blockIdx.y*nppc+icp)*patch_nx*patch_ny+threadIdx.x;
	d_src+=blockIdx.y*dat_ny*src_pitch+qy*src_pitch+qx+threadIdx.x-px;
	d_padding_patch( d_dst, d_src, px, py, src_pitch, valid_nx, valid_ny, patch_nx, patch_ny );
}
__global__ void d_padding_patches( float* d_patch, const float* __restrict__ d_ker, unsigned int n_kers )
{
	unsigned int patch_id, ker_nx, ker_ny, patch_nx, patch_ny;
	patch_id=blockIdx.x*blockDim.y+threadIdx.y;
	if(patch_id>=n_kers) return;
	ker_nx	=c_patchconv_params.ker_nx;
	ker_ny	=c_patchconv_params.ker_ny;
	patch_nx=c_patchconv_params.patch_nx;
	patch_ny=c_patchconv_params.patch_ny;
	d_patch+=patch_id*patch_nx*patch_ny+threadIdx.x;
	d_ker+=patch_id*ker_nx*ker_ny+threadIdx.x;
	d_padding_patch( d_patch, d_ker, 0, 0, ker_nx, ker_nx, ker_ny, patch_nx, patch_ny );
}
__global__ void d_postpreacc_patches( float2* d_out, const float2* d_dat, const float2* d_ker, const float2* d_RF, unsigned int inc, float scale, float sign )
{
	unsigned int patch_nx, patch_ny, npx, npy, tidx, icp, patch_size, nppc, i, x, y;
	
	tidx=blockIdx.x*blockDim.x+threadIdx.x;
	patch_nx	=c_patchconv_params.patch_nx>>1;
	patch_ny	=c_patchconv_params.patch_ny;
	npx			=c_patchconv_params.npx;
	npy			=c_patchconv_params.npy;
	patch_size	=patch_nx*patch_ny;
	nppc		=npx*npy;
	icp=tidx>>(__ffs(patch_size)-1);
	if(icp>=nppc) return;
	d_out+=(blockIdx.y*nppc+icp)*patch_size;
	d_dat+=icp*patch_size;
	d_ker+=blockIdx.y*inc*patch_size;	
	if(tidx>=(nppc*patch_size)) return;
	i=tidx&(patch_size-1);
	x=i&(patch_nx-1);
	y=i>>(__ffs(patch_nx)-1);
	d_postpreacc_patch( d_out, d_dat, d_ker, d_RF, x, y, patch_nx, patch_ny, patch_size, nppc, inc, scale, sign );
}
__global__ void d_splice( float* d_dst, const float* __restrict__ d_src, int op )
{
	unsigned int out_nx, out_ny, ker_nx, ker_ny, valid_nx, valid_ny, patch_nx, patch_ny, npx, npy, out_pitch, tidx, x, y, p, ipx, ipy;
	int o;

	ker_nx		=c_patchconv_params.ker_nx;
	ker_ny		=c_patchconv_params.ker_ny;	
	out_nx		=c_patchconv_params.out_nx;
	out_ny		=c_patchconv_params.out_ny;
	valid_nx	=c_patchconv_params.valid_nx;
	valid_ny	=c_patchconv_params.valid_ny;
	patch_nx	=c_patchconv_params.patch_nx;
	patch_ny	=c_patchconv_params.patch_ny;
	npx			=c_patchconv_params.npx;
	npy			=c_patchconv_params.npy;
	out_pitch	=c_patchconv_params.out_pitch;

	tidx=blockIdx.x*blockDim.x+threadIdx.x;	
	x=tidx%out_pitch; 
	ipy=tidx/out_pitch;
	if((x>=out_nx)|(ipy>=npy)) return;		
	ipx=x/valid_nx; x%=valid_nx;
	d_dst+=(blockIdx.y*out_ny+ipy*valid_ny)*out_pitch+x;
	d_src+=((blockIdx.y*npy*npx+ipy*npx)*patch_ny+ipx)*patch_nx;
	valid_ny=(ipy<(npy-1))?valid_ny:(out_ny-valid_ny*(npy-1));
	if(op!=0){
		p=(x!=0)?(patch_nx-x-1):0;
	} else {
		p=(ker_ny-1)*patch_nx+ker_nx-1+x;	
	}	
	y=valid_ny;
	o=(op!=0)?-((int)patch_nx):patch_nx;	
	d_dst[0]=(d_src+=p)[0];
	while((--y)>0){
		d_dst[0]=d_src[0]; d_dst+=out_pitch; d_src+=o;
	}
}
