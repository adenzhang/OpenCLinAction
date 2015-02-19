
#define MATNAME(a) _a##__mat
#define MATDEF(a, TYPE, M, N, IDX_START, ROW_MAJOR) \
    TYPE *MATNAME(a) = a; \
    int   MATNAME(a)##_M = M, MATNAME(a)##_N = N; \
    int   MATNAME(a)##_idx0 = IDX_START, MATNAME(a)##_rowmaj = ROW_MAJOR;

#define MATASSERT(a, i, j) \
    assert(i >= MATNAME(a)##_idx0 && i < MATNAME(a)##_idx0+MATNAME(a)##_M); \
    assert(j >= MATNAME(a)##_idx0 && j < MATNAME(a)##_idx0+MATNAME(a)##_N)

#define CMATDEF(a, TYPE, M, N) MATDEF(a, TYPE, M, N, 0, 1)
#define FMATDEF(a, TYPE, M, N) MATDEF(a, TYPE, M, N, 1, 0)

// c style row major storage
#define CMATAT(a,M,N) MATNAME(a)[(MATNAME(a)##_N-MATNAME(a)##_idx0) *(M) + (N-MATNAME(a)##_idx0)]
// fortran style column major storage
#define FMATAT(a,M,N) MATNAME(a)[(MATNAME(a)##_M-MATNAME(a)##_idx0) *(N) + (M-MATNAME(a)##_idx0)]

#define MATAT(a, M, N)  \
    (MATNAME(a)##_rowmaj ? CMATAT(a,M,N) : FMATAT(a,M,N))

#define VECAT(a, M)  MATNAME(a)[(M-MATNAME(a)##_idx0)]


__kernel void matvec_mult00(__global float4* matrix,
                          __global float4* vector,
                          __global float* result) {
   
   int i = get_global_id(0);
   result[i] = dot(matrix[i], vector[0]);
}
__kernel void matvec_mult(__global float* matrix,
                          __global float* vector,
                          __global float* result, uint M, uint N) {
   
	int i = get_global_id(0);  // global size = nrow
	uint k;
	uint n = N/4;
	uint d = N - n; 
	result[i] = 0;
	__global float4 *m4 = (__global float4 *)(matrix + i*N);
	__global float4 *v4 = (__global float4 *) vector;
	/*
	__global float *m = matrix + i*N + n*4;
	__global float *v = vector + n*4;
	for(k=0; k< n; ++k, ++m4, ++v4) {
		result[i] += dot(*m4, *v4);
	}
	*/
	__global float *m = matrix + i*N;
	__global float *v = vector;
	d = N;
	for(k=0; k<d; ++k, ++m, ++v) {
	    result[i] += (*m) * (*v);
	}
}

__kernel void matmat_mult(__global float* matrix1,
                          __global float* matrix2,
                          __global float* result, uint M, uint N, uint K) 
{
	int i = get_global_id(0);
	int j = get_global_id(1);
	uint c, r;
	__global float *m1 = matrix1;
	__global float *m2 = matrix2;
	__global float *res = result + i*K + j;
	//CMATDEF(m1, float, M, N);
	//CMATDEF(m2, float, N, K);
	
	for(c=0; c < N; ++c) {
		//*res += CMATAT(m1,i,c) * CMATAT(m2,c,j);
		*res += m1[i*M+c] * m2[c*K+j];
	}   
}

/* method 2 */
__kernel void matvecmult_mult(__global float* m,
                          __global float* v,
						  uint nrow, uint ncol) {
   
   uint i = get_global_id(0); /* global size = nrow*ncol */
   m[i] *=  v[i%ncol];
}

__kernel void matvecmult_sum(__global float* v,
                          __global float* r,
						  uint nrow, uint ncol) {
   
   uint i = get_global_id(0); /* global size = nrow */
   r[i] = 0;
   v += i*ncol;
   for(uint k=1; k< ncol; ++k)
		r[i] += v[k];
}

