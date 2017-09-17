#!/bin/bash


RSVD_INCLUDE="../c_blas/"
if  [  `uname` = 'Linux'  ]; then

    MATLAB_ROOT=/opt/MATLAB/R2015b
    MATLAB_LIB=/opt/MATLAB/R2015b/bin/glnxa64/
    cc=gcc
    openmp=-fopenmp
    suffix=mexa64

elif [ `uname` = 'Darwin' ] ; then

    MATLAB_ROOT=/Applications/MATLAB_R2015b.app
    MATLAB_LIB=$MATLAB_ROOT/bin/maci64/
    cc=clang
    openmp=
    suffix=mexmaci64
else
	echo " not tested on windows"
	exit 1
fi

MATLAB_INC=$MATLAB_ROOT/extern/include/

$cc $openmp  -w -fpic -shared -I "$RSVD_INCLUDE" -I "$MATLAB_INC" low_rank_svd_rand_decomp_fixed_rank_mkl_mex.c \
$RSVD_INCLUDE/rank_revealing_algorithms_intel_mkl.c \
$RSVD_INCLUDE/matrix_vector_functions_intel_mkl.c -L"$MATLAB_LIB" -L"$MKL_LIB"  \
 -largeArrayDims -lmwlapack -lmwblas -lmex -lmx -o low_rank_svd_rand_decomp_fixed_rank_mkl_mex.$suffix

