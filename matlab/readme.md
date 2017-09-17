# why I write this code 

Compiling RSVDPACK is not easy since I have no intel compiler.    

# tree of code

```
matlab
├── c_blas
│   ├── matrix_vector_functions_intel_mkl.c
│   ├── matrix_vector_functions_intel_mkl.h
│   ├── rank_revealing_algorithms_intel_mkl.c
│   ├── rank_revealing_algorithms_intel_mkl.h
│   └── readme.md
├── mex_c_blas
│   ├── compile_mex.sh
│   ├── low_rank_svd_rand_decomp_fixed_rank_mkl_ifce.m
│   ├── low_rank_svd_rand_decomp_fixed_rank_mkl_mex.c
│   └── readme.md
└── readme.md
``` 

* `c_blas`:  modified from multi_core_mkl_code_64bit, no longer depend on intel compiler
* `mex_c_blas`: can be compiled by mex function in matlab environment.
