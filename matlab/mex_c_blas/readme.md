

1. compile 

```
 sh compile_mex.sh
```


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

