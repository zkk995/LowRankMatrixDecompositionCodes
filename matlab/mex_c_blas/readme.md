

1. compile 

```
 sh compile_mex.sh
```

or call from mex 

```matlab
mex -I../c_blas/  low_rank_svd_rand_decomp_fixed_rank_mkl_mex.c ../c_blas/rank_revealing_algorithms_intel_mkl.c  ../c_blas/matrix_vector_functions_intel_mkl.c    -largeArrayDims -lmwlapack -lmwblas  CFLAGS="\$CFLAGS -fopenmp" LDFLAGS="\$LDFLAGS -fopenmp"
```

Important: macOS do not support openmp, one may need to modify the above code.


2. test

``` test
A = randn(2000,5000);
k = 1900;
p = 20;
vnum = 2;
q = 3;
s = 1;
[U,S,V] = low_rank_svd_rand_decomp_fixed_rank_mkl_ifce(A,k,p,vnum,q,s);
norm(A - U*S*V')/norm(A)
```

