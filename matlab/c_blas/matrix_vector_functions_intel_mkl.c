/* high level matrix/vector functions using Intel MKL for blas */
/* Sergey Voronin, 2014 - 2016 */

#include "matrix_vector_functions_intel_mkl.h"

/* initialize new matrix and set all entries to zero */
mat * matrix_new(int64_t nrows, int64_t ncols)
{
    mat *M = malloc(sizeof(mat));
    //M->d = (double*)mkl_calloc(nrows*ncols, sizeof(double), 64);
    //M->d = (double*)calloc(nrows*ncols, sizeof(double));
    M->d = (double*)malloc(nrows*ncols*sizeof(double));
    memset(M->d, 0, nrows*ncols*sizeof(double));
    M->nrows = nrows;
    M->ncols = ncols;
    return M;
}


/* initialize new vector and set all entries to zero */
vec * vector_new(int64_t nrows)
{
    vec *v = malloc(sizeof(vec));
    //v->d = (double*)mkl_calloc(nrows,sizeof(double), 64);
    v->d = (double*)calloc(nrows,sizeof(double));
    v->nrows = nrows;
    return v;
}


void matrix_delete(mat *M)
{
    //mkl_free(M->d);
    free(M->d);
    free(M);
}


void vector_delete(vec *v)
{
    //mkl_free(v->d);
    free(v->d);
    free(v);
}


// column major format
void matrix_set_element(mat *M, int64_t row_num, int64_t col_num, double val){
    //M->d[row_num*(M->ncols) + col_num] = val;
    M->d[col_num*(M->nrows) + row_num] = val;
}

double matrix_get_element(mat *M, int64_t row_num, int64_t col_num){
    //return M->d[row_num*(M->ncols) + col_num];
    return M->d[col_num*(M->nrows) + row_num];
}


void vector_set_element(vec *v, int64_t row_num, double val){
    v->d[row_num] = val;
}


double vector_get_element(vec *v, int64_t row_num){
    return v->d[row_num];
}


/* load matrix from binary file 
 * the nonzeros are in order of double loop over rows and columns
format:
num_rows (int64_t) 
num_columns (int64_t)
nnz (double)
...
nnz (double)
*/
mat * matrix_load_from_binary_file(char *fname){
    int64_t i, j, num_rows, num_columns, row_num, col_num;
    double nnz_val;
    size_t one = 1;
    FILE *fp;
    mat *M;

    fp = fopen(fname,"r");
    fread(&num_rows,sizeof(int64_t),one,fp); //read m
    fread(&num_columns,sizeof(int64_t),one,fp); //read n
    printf("initializing M of size %d by %d\n", num_rows, num_columns);
    M = matrix_new(num_rows,num_columns);
    printf("done..\n");

    // read and set elements
    for(i=0; i<num_rows; i++){
        for(j=0; j<num_columns; j++){
            fread(&nnz_val,sizeof(double),one,fp); //read nnz
            matrix_set_element(M,i,j,nnz_val);
        }
    }
    fclose(fp);

    return M;
}
  


/* write matrix to binary file 
 * the nonzeros are in order of double loop over rows and columns
format:
num_rows (int64_t) 
num_columns (int64_t)
nnz (double)
...
nnz (double)
*/
void matrix_write_to_binary_file(mat *M, char *fname){
    int64_t i, j, num_rows, num_columns, row_num, col_num;
    double nnz_val;
    size_t one = 1;
    FILE *fp;
    num_rows = M->nrows; num_columns = M->ncols;
    
    fp = fopen(fname,"w");
    fwrite(&num_rows,sizeof(int64_t),one,fp); //write m
    fwrite(&num_columns,sizeof(int64_t),one,fp); //write n

    // write the elements
    for(i=0; i<num_rows; i++){
        for(j=0; j<num_columns; j++){
            nnz_val = matrix_get_element(M,i,j);
            fwrite(&nnz_val,sizeof(double),one,fp); //write nnz
        }
    }
    fclose(fp);
}



void matrix_print(mat * M){
    int64_t i,j;
    double val;
    for(i=0; i<M->nrows; i++){
        for(j=0; j<M->ncols; j++){
            val = matrix_get_element(M, i, j);
            printf("%f  ", val);
        }
        printf("\n");
    }
}


void vector_print(vec * v){
    int64_t i;
    double val;
    for(i=0; i<v->nrows; i++){
        val = vector_get_element(v, i);
        printf("%f\n", val);
    }
}


/* v(:) = data */
void vector_set_data(vec *v, double *data){
    int64_t i;
    #pragma omp parallel shared(v) private(i) 
    {
    #pragma omp for
    for(i=0; i<(v->nrows); i++){
        v->d[i] = data[i];
    }
    }
}


/* scale vector by a constant */
void vector_scale(vec *v, double scalar){
    int64_t i;
    #pragma omp parallel shared(v,scalar) private(i) 
    {
    #pragma omp for
    for(i=0; i<(v->nrows); i++){
        v->d[i] = scalar*(v->d[i]);
    }
    }
}


/* scale matrix by a constant */
void matrix_scale(mat *M, double scalar){
    int64_t i;
    #pragma omp parallel shared(M,scalar) private(i) 
    {
    #pragma omp for
    for(i=0; i<((M->nrows)*(M->ncols)); i++){
        M->d[i] = scalar*(M->d[i]);
    }
    }
}




/* copy contents of vec s to d  */
void vector_copy(vec *d, vec *s){
    int64_t i;
    //#pragma omp parallel for
    #pragma omp parallel shared(d,s) private(i) 
    {
    #pragma omp for 
    for(i=0; i<(s->nrows); i++){
        d->d[i] = s->d[i];
    }
    }
}


/* copy contents of mat S to D  */
void matrix_copy(mat *D, mat *S){
    int64_t i;
    //#pragma omp parallel for
    #pragma omp parallel shared(D,S) private(i) 
    {
    #pragma omp for 
    for(i=0; i<((S->nrows)*(S->ncols)); i++){
        D->d[i] = S->d[i];
    }
    }
}



/* hard threshold matrix entries  */
void matrix_hard_threshold(mat *M, double TOL){
    int64_t i;
    #pragma omp parallel shared(M) private(i) 
    {
    #pragma omp for 
    for(i=0; i<((M->nrows)*(M->ncols)); i++){
        if(fabs(M->d[i]) < TOL){
            M->d[i] = 0;
        }
    }
    }
}


/* build transpose of matrix : Mt = M^T */
void matrix_build_transpose(mat *Mt, mat *M){
    int64_t i,j;
    for(i=0; i<(M->nrows); i++){
        for(j=0; j<(M->ncols); j++){
            matrix_set_element(Mt,j,i,matrix_get_element(M,i,j)); 
        }
    }
}





/* subtract b from a and save result in a  */
void vector_sub(vec *a, vec *b){
    int64_t i;
    //#pragma omp parallel for
    #pragma omp parallel shared(a,b) private(i) 
    {
    #pragma omp for 
    for(i=0; i<(a->nrows); i++){
        a->d[i] = a->d[i] - b->d[i];
    }
    }
}


/* subtract B from A and save result in A  */
void matrix_sub(mat *A, mat *B){
    int64_t i;
    #pragma omp parallel shared(A,B) private(i) 
    {
    #pragma omp for 
    for(i=0; i<((A->nrows)*(A->ncols)); i++){
        A->d[i] = A->d[i] - B->d[i];
    }
    }
}


/* A = A - u*v where u is a column vec and v is a row vec */
void matrix_sub_column_times_row_vector(mat *A, vec *u, vec *v){
    int64_t i,j;
    #pragma omp parallel for shared(A,u,v) private(j)
    for(i=0; i<(A->nrows); i++){
        for(j=0; j<(A->ncols); j++){
            matrix_set_element(A,i,j,matrix_get_element(A,i,j) - vector_get_element(u,i)*vector_get_element(v,j));
        }
    }
}


/* compute euclidean norm of vector */
double vector_get2norm(vec *v){
    int64_t i;
    double val, normval = 0;
    #pragma omp parallel shared(v,normval) private(i,val) 
    {
    #pragma omp for reduction(+:normval)
    for(i=0; i<(v->nrows); i++){
        val = v->d[i];
        normval += val*val;
    }
    }
    return sqrt(normval);
}


void vector_get_min_element(vec *v, int64_t *minindex, double *minval){
    int64_t i, val;
    *minindex = 0;
    *minval = v->d[0];
    for(i=0; i<(v->nrows); i++){
        val = v->d[i];
        if(val < *minval){
            *minval = val;
            *minindex = i;
        }
    }
}


void vector_get_max_element(vec *v, int64_t *maxindex, double *maxval){
    int64_t i, val;
    *maxindex = 0;
    *maxval = v->d[0];
    for(i=0; i<(v->nrows); i++){
        val = v->d[i];
        if(val > *maxval){
            *maxval = val;
            *maxindex = i;
        }
    }
}



/* returns the dot product of two vectors */
double vector_dot_product(vec *u, vec *v){
    int64_t i;
    double dotval = 0;
    #pragma omp parallel shared(u,v,dotval) private(i) 
    {
    #pragma omp for reduction(+:dotval)
    for(i=0; i<u->nrows; i++){
        dotval += (u->d[i])*(v->d[i]);
    }
    }
    return dotval;
}



/* matrix frobenius norm */
double get_matrix_frobenius_norm(mat *M){
    int64_t i;
    double val, normval = 0;
    #pragma omp parallel shared(M,normval) private(i,val) 
    {
    #pragma omp for reduction(+:normval)
    for(i=0; i<((M->nrows)*(M->ncols)); i++){
        val = M->d[i];
        normval += val*val;
    }
    }
    return sqrt(normval);
}


/* matrix max abs val */
double get_matrix_max_abs_element(mat *M){
    int64_t i;
    double val, max = 0;
    for(i=0; i<((M->nrows)*(M->ncols)); i++){
        val = fabs(M->d[i]);
        if( val > max )
            max = val;
    }
    return max;
}



/* calculate percent error between A and B 
in terms of Frobenius norm: 100*norm(A - B)/norm(A) */
double get_percent_error_between_two_mats(mat *A, mat *B){
    int64_t m,n;
    double normA, normB, normA_minus_B;
    mat *A_minus_B;
    m = A->nrows;
    n = A->ncols;
    A_minus_B = matrix_new(m,n);
    matrix_copy(A_minus_B, A);
    matrix_sub(A_minus_B, B);
    normA = get_matrix_frobenius_norm(A);
    normB = get_matrix_frobenius_norm(B);
    normA_minus_B = get_matrix_frobenius_norm(A_minus_B);
    matrix_delete(A_minus_B);
    return 100.0*normA_minus_B/normA;
}


double get_matrix_column_norm_squared(mat *M, int64_t colnum){
    int64_t i, m, n;
    double val,colnorm;
    m = M->nrows;
    n = M->ncols;
    colnorm = 0;
    for(i=0; i<m; i++){
        val = matrix_get_element(M,i,colnum);
        colnorm += val*val;
    }
    return colnorm;
}


double matrix_getmaxcolnorm(mat *M){
    int64_t i,m,n;
    vec *col_vec;
    double vecnorm, maxnorm;
    m = M->nrows; n = M->ncols;
    col_vec = vector_new(m);

    maxnorm = 0;    
    #pragma omp parallel for
    for(i=0; i<n; i++){
        matrix_get_col(M,i,col_vec);
        vecnorm = vector_get2norm(col_vec);
        #pragma omp critical
        if(vecnorm > maxnorm){
            maxnorm = vecnorm;
        }
    }

    vector_delete(col_vec);
    return maxnorm;
}


void compute_matrix_column_norms(mat *M, vec *column_norms){
    int64_t j;
    #pragma omp parallel shared(column_norms,M) private(j) 
    {
    #pragma omp parallel for
    for(j=0; j<(M->ncols); j++){
        vector_set_element(column_norms,j, get_matrix_column_norm_squared(M,j)); 
    }
    }
}



#include <math.h>
#include <stdlib.h>

double gaussrand()
{
    static double V1, V2, S;
    static int phase = 0;
    double X; eps=0.001;

    if(phase == 0) {
        do {
            double U1 = (double)rand() / RAND_MAX;
            double U2 = (double)rand() / RAND_MAX;

            V1 = 2 * U1 - 1;
            V2 = 2 * U2 - 1;
            S = V1 * V1 + V2 * V2;
            } while(S >= 1.0-eps || S < eps );

        X = V1 * sqrt(-2 * log(S) / S);
    } else
        X = V2 * sqrt(-2 * log(S) / S);

    phase = 1 - phase;

    return X;
}

/* initialize a random matrix */
void initialize_random_matrix(mat *M){
    int64_t i,m,n;
    double val;
    m = M->nrows;
    n = M->ncols;
    float a=0.0,sigma=1.0;
    int64_t N = m*n;
    // @zkk
    srand(time(NULL));

    #pragma omp parallel shared(M,N) private(i)
    {
    #pragma omp parallel for
    for(i=0; i<N; i++){
        M->d[i] = gaussrand();
    }
    }

    /*
    float *r;
    VSLStreamStatePtr stream;
    
    r = (float*)malloc(N*sizeof(float));
   
    vslNewStream( &stream, BRNG,  time(NULL) );
    //vslNewStream( &stream, BRNG,  SEED );

    vsRngGaussian( METHOD, stream, N, r, a, sigma );

    // read and set elements
    #pragma omp parallel shared(M,N,r) private(i,val) 
    {
    #pragma omp parallel for
    for(i=0; i<N; i++){
        val = r[i];
        M->d[i] = val;
    }
    }
    
    free(r);
    */
}


/* initialize diagonal matrix from vector data */
void initialize_diagonal_matrix(mat *D, vec *data){
    int64_t i;
    #pragma omp parallel shared(D) private(i)
    { 
    #pragma omp parallel for
    for(i=0; i<(D->nrows); i++){
        matrix_set_element(D,i,i,data->d[i]);
    }
    }
}



/* initialize identity */
void initialize_identity_matrix(mat *D){
    int64_t i;
    matrix_scale(D, 0);
    #pragma omp parallel shared(D) private(i)
    { 
    #pragma omp parallel for
    for(i=0; i<(D->nrows); i++){
        matrix_set_element(D,i,i,1.0);
    }
    }
}



/* invert diagonal matrix */
void invert_diagonal_matrix(mat *Dinv, mat *D){
    int64_t i;
    double v;
    #pragma omp parallel shared(D,Dinv) private(i)
    {
    #pragma omp parallel for
    for(i=0; i<(D->nrows); i++){
        v = matrix_get_element(D,i,i);
        if (fabs(v)<=1e-14){v = 0.0;}
        else {v = 1.0/v;} 
        matrix_set_element(Dinv,i,i, v);
    }
    }
}


/* overwrites supplied upper triangular matrix by its inverse */
void invert_upper_triangular_matrix(mat *Minv){
    LAPACKE_dtrtri( LAPACK_COL_MAJOR, 'U', 'N', Minv->nrows, Minv->d, Minv->nrows);
}


/* C = A*B ; column major */
void matrix_matrix_mult(mat *A, mat *B, mat *C){
    double alpha, beta;
    char CblasNoTrans='N';char CblasTrans='T';
    alpha = 1.0; beta = 0.0;
    //cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, A->nrows, B->ncols, A->ncols, alpha, A->d, A->ncols, B->d, B->ncols, beta, C->d, C->ncols);
    //cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, A->nrows, B->ncols, A->ncols, alpha, A->d, A->nrows, B->d, B->nrows, beta, C->d, C->nrows);
    dgemm(&CblasNoTrans, &CblasNoTrans, &A->nrows, &B->ncols, &A->ncols, &alpha, A->d, &A->nrows, B->d, &B->nrows, &beta, C->d, &C->nrows);
}


/* C = A^T*B ; column major */
void matrix_transpose_matrix_mult(mat *A, mat *B, mat *C){
    double alpha, beta;
    char CblasNoTrans='N';char CblasTrans='T';
    alpha = 1.0; beta = 0.0;
    //cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, A->ncols, B->ncols, A->nrows, alpha, A->d, A->ncols, B->d, B->ncols, beta, C->d, C->ncols);
    //cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, A->ncols, B->ncols, A->nrows, alpha, A->d, A->nrows, B->d, B->nrows, beta, C->d, C->nrows);
    dgemm(&CblasTrans, &CblasNoTrans, &A->ncols, &B->ncols, &A->nrows, &alpha, A->d, &A->nrows, B->d, &B->nrows, &beta, C->d, &C->nrows);
}


/* C = A*B^T ; column major */
void matrix_matrix_transpose_mult(mat *A, mat *B, mat *C){
    double alpha, beta;
    char CblasNoTrans='N';char CblasTrans='T';
    alpha = 1.0; beta = 0.0;
    //cblas_dgemm(CblasColMajor, CblasNoTrans, CblasTrans, A->nrows, B->nrows, A->ncols, alpha, A->d, A->ncols, B->d, B->ncols, beta, C->d, C->ncols);
    //cblas_dgemm(CblasColMajor, CblasNoTrans, CblasTrans, A->nrows, B->nrows, A->ncols, alpha, A->d, A->nrows, B->d, B->nrows, beta, C->d, C->nrows);
    dgemm(&CblasNoTrans, &CblasTrans, &A->nrows, &B->nrows, &A->ncols, &alpha, A->d, &A->nrows, B->d, &B->nrows, &beta, C->d, &C->nrows);
}


/* y = M*x ; column major */
void matrix_vector_mult(mat *M, vec *x, vec *y){
    double alpha, beta;
    int64_t inc=1; 
    char CblasNoTrans='N';char CblasTrans='T';
    alpha = 1.0; beta = 0.0;
    //cblas_dgemv (CblasColMajor, CblasNoTrans, M->nrows, M->ncols, alpha, M->d, M->nrows, x->d, 1, beta, y->d, 1);
    dgemv (&CblasNoTrans, &M->nrows, &M->ncols, &alpha, M->d, &M->nrows, x->d, &inc, &beta, y->d, &inc);
}


/* y = M^T*x ; column major */
void matrix_transpose_vector_mult(mat *M, vec *x, vec *y){
    double alpha, beta;
    int64_t inc=1;
    char CblasNoTrans='N';char CblasTrans='T';
    alpha = 1.0; beta = 0.0;
    //cblas_dgemv (CblasColMajor, CblasTrans, M->nrows, M->ncols, alpha, M->d, M->nrows, x->d, 1, beta, y->d, 1);
    dgemv (&CblasTrans, &M->nrows, &M->ncols, &alpha, M->d, &M->nrows, x->d, &inc, &beta, y->d, &inc);
}



/* set column of matrix to vector */
void matrix_set_col(mat *M, int64_t j, vec *column_vec){
    int64_t i;
    #pragma omp parallel shared(column_vec,M,j) private(i) 
    {
    #pragma omp for
    for(i=0; i<M->nrows; i++){
        matrix_set_element(M,i,j,vector_get_element(column_vec,i));
    }
    }
}


/* extract column of a matrix into a vector */
void matrix_get_col(mat *M, int64_t j, vec *column_vec){
    int64_t i;
    #pragma omp parallel shared(column_vec,M,j) private(i) 
    {
    #pragma omp parallel for
    for(i=0; i<M->nrows; i++){ 
        vector_set_element(column_vec,i,matrix_get_element(M,i,j));
    }
    }
}


/* extract row i of a matrix into a vector */
void matrix_get_row(mat *M, int64_t i, vec *row_vec){
    int64_t j;
    #pragma omp parallel shared(row_vec,M,i) private(j) 
    {
    #pragma omp parallel for
    for(j=0; j<M->ncols; j++){ 
        vector_set_element(row_vec,j,matrix_get_element(M,i,j));
    }
    }
}


/* put vector row_vec as row i of a matrix */
void matrix_set_row(mat *M, int64_t i, vec *row_vec){
    int64_t j;
    #pragma omp parallel shared(row_vec,M,i) private(j) 
    {
    #pragma omp parallel for
    for(j=0; j<M->ncols; j++){ 
        matrix_set_element(M,i,j,vector_get_element(row_vec,j));
    }
    }
}



/* Mc = M(:,inds) */
/*void matrix_get_selected_columns(mat *M, int64_t *inds, mat *Mc){
    int64_t i;
    vec *col_vec = vector_new(M->nrows); 
    for(i=0; i<(Mc->ncols); i++){
        matrix_get_col(M,inds[i],col_vec);
        matrix_set_col(Mc,i,col_vec);
    }
    vector_delete(col_vec);
}*/


/* Mc = M(:,inds) */
void matrix_get_selected_columns(mat *M, int64_t *inds, mat *Mc){
    int64_t i;
    vec *col_vec; 
    #pragma omp parallel shared(M,Mc,inds) private(i,col_vec) 
    {
    col_vec = vector_new(M->nrows);
    #pragma omp parallel for
    for(i=0; i<(Mc->ncols); i++){
        matrix_get_col(M,inds[i],col_vec);
        matrix_set_col(Mc,i,col_vec);
    }
    vector_delete(col_vec);
    }
}




/* M(:,inds) = Mc */
/*void matrix_set_selected_columns(mat *M, int64_t *inds, mat *Mc){
    int64_t i;
    vec *col_vec = vector_new(M->nrows); 
    for(i=0; i<(Mc->ncols); i++){
        matrix_get_col(Mc,i,col_vec);
        matrix_set_col(M,inds[i],col_vec);
    }
    vector_delete(col_vec);
}*/


/* M(:,inds) = Mc */
void matrix_set_selected_columns(mat *M, int64_t *inds, mat *Mc){
    int64_t i;
    vec *col_vec; 
    #pragma omp parallel shared(M,Mc,inds) private(i,col_vec) 
    {
    col_vec = vector_new(M->nrows); 
    #pragma omp parallel for
    for(i=0; i<(Mc->ncols); i++){
        matrix_get_col(Mc,i,col_vec);
        matrix_set_col(M,inds[i],col_vec);
    }
    vector_delete(col_vec);
    }
}



/* Mr = M(inds,:) */
/*void matrix_get_selected_rows(mat *M, int64_t *inds, mat *Mr){
    int64_t i;
    vec *row_vec = vector_new(M->ncols); 
    for(i=0; i<(Mr->nrows); i++){
        matrix_get_row(M,inds[i],row_vec);
        matrix_set_row(Mr,i,row_vec);
    }
    vector_delete(row_vec);
}*/



/* Mr = M(inds,:) */
void matrix_get_selected_rows(mat *M, int64_t *inds, mat *Mr){
    int64_t i;
    vec *row_vec; 
    #pragma omp parallel shared(M,Mr,inds) private(i,row_vec) 
    {
    row_vec = vector_new(M->ncols); 
    #pragma omp parallel for
    for(i=0; i<(Mr->nrows); i++){
        matrix_get_row(M,inds[i],row_vec);
        matrix_set_row(Mr,i,row_vec);
    }
    vector_delete(row_vec);
    }
}



/* M(inds,:) = Mr */
/*void matrix_set_selected_rows(mat *M, int64_t *inds, mat *Mr){
    int64_t i;
    vec *row_vec = vector_new(M->ncols); 
    for(i=0; i<(Mr->nrows); i++){
        matrix_get_row(Mr,i,row_vec);
        matrix_set_row(M,inds[i],row_vec);
    }
    vector_delete(row_vec);
}*/



/* M(inds,:) = Mr */
void matrix_set_selected_rows(mat *M, int64_t *inds, mat *Mr){
    int64_t i;
    vec *row_vec; 
    #pragma omp parallel shared(M,Mr,inds) private(i,row_vec) 
    {
    #pragma omp parallel for
    for(i=0; i<(Mr->nrows); i++){
        row_vec = vector_new(M->ncols); 
        matrix_get_row(Mr,i,row_vec);
        matrix_set_row(M,inds[i],row_vec);
        vector_delete(row_vec);
    }
    }
}


/* copy only upper triangular matrix part as for symmetric matrix */
void matrix_copy_symmetric(mat *S, mat *M){
    int64_t i,j,n,m;
    m = M->nrows;
    n = M->ncols;
    for(i=0; i<m; i++){
        for(j=0; j<n; j++){
            if(j>=i){
                matrix_set_element(S,i,j,matrix_get_element(M,i,j));
            }
        }
    }
}



/* copy only upper triangular matrix part as for symmetric matrix */
void matrix_keep_only_upper_triangular(mat *M){
    int64_t i,j,n,m;
    m = M->nrows;
    n = M->ncols;
    for(i=0; i<m; i++){
        for(j=0; j<n; j++){
            if(j<i){
                matrix_set_element(M,i,j,0);
            }
        }
    }
}





/*
% project v in direction of u
function p=project_vec(v,u)
p = (dot(v,u)/norm(u)^2)*u;
*/
void project_vector(vec *v, vec *u, vec *p){
    double dot_product_val, vec_norm, scalar_val; 
    dot_product_val = vector_dot_product(v, u);
    vec_norm = vector_get2norm(u);
    scalar_val = dot_product_val/(vec_norm*vec_norm);
    vector_copy(p, u);
    vector_scale(p, scalar_val); 
}


/* build orthonormal basis matrix
Q = Y;
for j=1:k
    vj = Q(:,j);
    for i=1:(j-1)
        vi = Q(:,i);
        vj = vj - project_vec(vj,vi);
    end
    vj = vj/norm(vj);
    Q(:,j) = vj;
end
*/
void build_orthonormal_basis_from_mat(mat *A, mat *Q){
    int64_t m,n,i,j,ind,num_ortos=2;
    double vec_norm;
    vec *vi,*vj,*p;
    m = A->nrows;
    n = A->ncols;
    vi = vector_new(m);
    vj = vector_new(m);
    p = vector_new(m);
    matrix_copy(Q, A);

    for(ind=0; ind<num_ortos; ind++){
        for(j=0; j<n; j++){
            matrix_get_col(Q, j, vj);
            for(i=0; i<j; i++){
                matrix_get_col(Q, i, vi);
                project_vector(vj, vi, p);
                vector_sub(vj, p);
            }
            vec_norm = vector_get2norm(vj);
            vector_scale(vj, 1.0/vec_norm);
            matrix_set_col(Q, j, vj);
        }
    }
    vector_delete(vi);
    vector_delete(vj);
    vector_delete(p);
}


/* output = input[inds] */
void fill_vector_from_row_list(vec *input, vec *inds, vec *output){
    int64_t i,col_num;
    for(i=0; i<(input->nrows); i++){
        vector_set_element(output,i,vector_get_element(input,vector_get_element(inds,i)));
    }
}




/* copy the first k rows of M into M_out where k = M_out->nrows (M_out pre-initialized) */
void matrix_copy_first_rows(mat *M_out, mat *M){
    int64_t i,k;
    k = M_out->nrows;
    vec * row_vec;
    for(i=0; i<k; i++){
        row_vec = vector_new(M->ncols);
        matrix_get_row(M,i,row_vec);
        matrix_set_row(M_out,i,row_vec);
        vector_delete(row_vec);
    }
} 



/* copy the first k columns of M into M_out where k = M_out->ncols (M_out pre-initialized) */
void matrix_copy_first_columns(mat *M_out, mat *M){
    int64_t i,k;
    k = M_out->ncols;
    vec * col_vec;
    for(i=0; i<k; i++){
        col_vec = vector_new(M->nrows);
        matrix_get_col(M,i,col_vec);
        matrix_set_col(M_out,i,col_vec);
        vector_delete(col_vec);
    }
} 

/* copy contents of mat S to D */
void matrix_copy_first_columns_with_param(mat *D, mat *S, int64_t num_columns){
    int64_t i,j;
    for(i=0; i<(S->nrows); i++){
        for(j=0; j<num_columns; j++){
            matrix_set_element(D,i,j,matrix_get_element(S,i,j));
        }
    }
}



/* copy the first k rows and columns of M into M_out is kxk where k = M_out->ncols (M_out pre-initialized) 
M_out = M(1:k,1:k) */
void matrix_copy_first_k_rows_and_columns(mat *M_out, mat *M){
    int64_t i,j,k;
    k = M_out->ncols;
    vec * col_vec;
    for(i=0; i<k; i++){
        for(j=0; j<k; j++){
            matrix_set_element(M_out,i,j,matrix_get_element(M,i,j));
        }
    }
} 


/* M_out = M(:,k+1:end) */
void matrix_copy_all_rows_and_last_columns_from_indexk(mat *M_out, mat *M, int64_t k){
    int64_t i,j,i_out,j_out;
    vec * col_vec;
    for(i=0; i<(M->nrows); i++){
        for(j=k; j<(M->ncols); j++){
            i_out = i; j_out = j - k;
            matrix_set_element(M_out,i_out,j_out,matrix_get_element(M,i,j));
        }
    }
}


/* M_k = M(1:k,:) */
void fill_matrix_from_first_rows(mat *M, int64_t k, mat *M_k){
    int64_t i;
    vec *row_vec;
    #pragma omp parallel shared(M,M_k,k) private(i,row_vec) 
    {
    #pragma omp for
    for(i=0; i<k; i++){
        row_vec = vector_new(M->ncols);
        matrix_get_row(M,i,row_vec);
        matrix_set_row(M_k,i,row_vec);
        vector_delete(row_vec);
    }
    }
}


/* M_k = M((k+1):end,:) */
void fill_matrix_from_last_rows(mat *M, int64_t k, mat *M_k){
    int64_t i;
    vec *row_vec;
    #pragma omp parallel shared(M,M_k,k) private(i,row_vec) 
    {
    #pragma omp for
    for(i=0; i<k; i++){
        row_vec = vector_new(M->nrows);
        matrix_get_row(M,M->nrows - k +i,row_vec);
        matrix_set_row(M_k,i,row_vec);
        vector_delete(row_vec);
    }
    }
}


void fill_matrix_from_first_columns(mat *M, int64_t k, mat *M_k){
    int64_t i;
    vec *col_vec;
    #pragma omp parallel shared(M,M_k,k) private(i,col_vec) 
    {
    #pragma omp for
    for(i=0; i<k; i++){
        col_vec = vector_new(M->nrows);
        matrix_get_col(M,i,col_vec);
        matrix_set_col(M_k,i,col_vec);
        vector_delete(col_vec);
    }
    }
}


/* M_k = M(:,(end-k:end) */
void fill_matrix_from_last_columns(mat *M, int64_t k, mat *M_k){
    int64_t i;
    vec *col_vec;
    #pragma omp parallel shared(M,M_k,k) private(i,col_vec) 
    {
    #pragma omp for
    for(i=0; i<k; i++){
        col_vec = vector_new(M->nrows);
        matrix_get_col(M,M->ncols - k +i,col_vec);
        matrix_set_col(M_k,i,col_vec);
        vector_delete(col_vec);
    }
    }
}



/* M_k = M(:,k:end) */
void fill_matrix_from_last_columns_from_specified_one(mat *M, int64_t k, mat *M_k){
    int64_t i;
    vec *col_vec;
    #pragma omp parallel shared(M,M_k,k) private(i,col_vec) 
    {
    #pragma omp for
    for(i=k; i<(M->ncols); i++){
        col_vec = vector_new(M->nrows);
        matrix_get_col(M,i,col_vec);
        matrix_set_col(M_k,i-k,col_vec);
        vector_delete(col_vec);
    }
    }
}



/* M_k = M(:,I(1:k)) */
void fill_matrix_from_first_columns_from_list(mat *M, vec *I, int64_t k, mat *M_k){
    int64_t i;
    vec *col_vec;
    #pragma omp parallel shared(M,M_k,I,k) private(i,col_vec)
    { 
    #pragma omp for
    for(i=0; i<k; i++){
        col_vec = vector_new(M->nrows);
        matrix_get_col(M,vector_get_element(I,i),col_vec);
        matrix_set_col(M_k,i,col_vec);
        vector_delete(col_vec);
    }
    }
}


/* M_k = M(I(1:k),:) */
void fill_matrix_from_first_rows_from_list(mat *M, vec *I, int64_t k, mat *M_k){
    int64_t i;
    vec *row_vec;
    #pragma omp parallel shared(M,M_k,I,k) private(i,row_vec)
    {
    #pragma omp for
    for(i=0; i<k; i++){
        row_vec = vector_new(M->ncols);
        matrix_get_row(M,(int64_t)vector_get_element(I,i),row_vec);
        matrix_set_row(M_k,i,row_vec);
        vector_delete(row_vec);
    }
    }
}



void fill_matrix_from_last_columns_from_list(mat *M, vec *I, int64_t k, mat *M_k){
    int64_t i,ind=0;
    vec *col_vec;
    //#pragma omp parallel shared(M,M_k,I,k,ind) private(i,col_vec)
    {
    //#pragma omp for
    for(i=k; i<M->ncols; i++){
        col_vec = vector_new(M->nrows);
        matrix_get_col(M,vector_get_element(I,i),col_vec);
        matrix_set_col(M_k,ind,col_vec);
        ind++;
        vector_delete(col_vec);
    }
    }
}



/* Mout = M((k+1):end,(k+1):end) in matlab notation */
void fill_matrix_from_lower_right_corner(mat *M, int64_t k, mat *M_out){
    int64_t i,j,i_out,j_out;
    for(i=k; i<M->nrows; i++){
        for(j=k; j<M->ncols; j++){
            i_out = i-k;
            j_out = j-k;
            //printf("setting element %d, %d of M_out\n", i_out, j_out);
            matrix_set_element(M_out,i_out,j_out,matrix_get_element(M,i,j));
        }
    }
}

/* M = M(:,1:k); */
void resize_matrix_by_columns(mat **M, int64_t k){
    int64_t j;
    mat *R;
    R = matrix_new((*M)->nrows, k);
    fill_matrix_from_first_columns(*M, k, R);
    matrix_delete(*M);
    *M = matrix_new(R->nrows, R->ncols);
    matrix_copy(*M,R);
    matrix_delete(R);
}  


/* M = M(:,(end-k):end); */
void resize_matrix_by_columns_from_end(mat **M, int64_t k){
    int64_t j;
    mat *R;
    R = matrix_new((*M)->nrows, k);
    fill_matrix_from_last_columns(*M, k, R);
    matrix_delete(*M);
    *M = matrix_new(R->nrows, R->ncols);
    matrix_copy(*M,R);
    matrix_delete(R);
}  



/* M = M(1:k,:); */
void resize_matrix_by_rows(mat **M, int64_t k){
    int64_t j;
    mat *R;
    R = matrix_new(k, (*M)->ncols);
    fill_matrix_from_first_rows(*M, k, R);
    matrix_delete(*M);
    *M = matrix_new(R->nrows, R->ncols);
    matrix_copy(*M,R);
    matrix_delete(R);
}



/* M = M((end-k+1):end,:); */
void resize_matrix_by_rows_from_end(mat **M, int64_t k){
    int64_t j;
    mat *R;
    R = matrix_new(k, (*M)->ncols);
    fill_matrix_from_last_rows(*M, k, R);
    matrix_delete(*M);
    *M = matrix_new(R->nrows, R->ncols);
    matrix_copy(*M,R);
    matrix_delete(R);
}



/* append matrices side by side: C = [A, B] */
void append_matrices_horizontally(mat *A, mat *B, mat *C){
    int64_t i,j;

    #pragma omp parallel shared(C,A) private(i) 
    {
    #pragma omp for 
    for(i=0; i<((A->nrows)*(A->ncols)); i++){
        C->d[i] = A->d[i];
    }
    }

    #pragma omp parallel shared(C,B,A) private(i) 
    {
    #pragma omp for 
    for(i=0; i<((B->nrows)*(B->ncols)); i++){
        C->d[i + (A->nrows)*(A->ncols)] = B->d[i];
    }
    }

    /* 
    for(i=0; i<A->nrows; i++){
        for(j=0; j<A->ncols; j++){
            matrix_set_element(C,i,j,matrix_get_element(A,i,j));
        }
    }

    for(i=0; i<B->nrows; i++){
        for(j=0; j<B->ncols; j++){
            matrix_set_element(C,i,A->ncols + j,matrix_get_element(B,i,j));
        }
    }*/
}



/* append matrices vertically: C = [A; B] */
void append_matrices_vertically(mat *A, mat *B, mat *C){
    int64_t i,j;

    #pragma omp parallel shared(C,A) private(i,j) 
    {
    #pragma omp for 
    for(i=0; i<A->nrows; i++){
        for(j=0; j<A->ncols; j++){
            matrix_set_element(C,i,j,matrix_get_element(A,i,j));
        }
    }
    }

    #pragma omp parallel shared(C,B,A) private(i,j) 
    {
    #pragma omp for 
    for(i=0; i<B->nrows; i++){
        for(j=0; j<B->ncols; j++){
            matrix_set_element(C,A->nrows+i,j,matrix_get_element(B,i,j));
        }
    }
    }
}


/* builds Iinv(I) = [0:n-1] */
void vector_build_rewrapped(vec *Iinv, vec *I){
    int64_t i,ind;
    for(i=0; i<(I->nrows); i++){
        ind = (int64_t)vector_get_element(I,i);
        vector_set_element(Iinv,ind,(double)i);
    }
}


/* compute eigendecomposition of symmetric matrix M
*/
void compute_evals_and_evecs_of_symm_matrix(mat *S, vec *evals){
    //LAPACKE_dsyev( LAPACK_ROW_MAJOR, 'V', 'U', S->nrows, S->d, S->nrows, evals->d);
    LAPACKE_dsyev( LAPACK_COL_MAJOR, 'V', 'U', S->nrows, S->d, S->ncols, evals->d);
}


/* Performs [Q,R] = qr(M,'0') compact QR factorization 
M is mxn ; Q is mxn ; R is min(m,n) x min(m,n) */ 
void compact_QR_factorization(mat *M, mat *Q, mat *R){
    int64_t i,j,m,n,k;
    m = M->nrows; n = M->ncols;
    k = min(m,n);
    printf("doing QR with m = %d, n = %d, k = %d\n", m,n,k);
    mat *R_full = matrix_new(m,n);
    matrix_copy(R_full,M);
    //vec *tau = vector_new(n);
    vec *tau = vector_new(k);

    // get R
    //printf("get R..\n");
    //LAPACKE_dgeqrf(CblasColMajor, m, n, R_full->d, n, tau->d);
    LAPACKE_dgeqrf(LAPACK_COL_MAJOR, R_full->nrows, R_full->ncols, R_full->d, R_full->nrows, tau->d);
    
    for(i=0; i<k; i++){
        for(j=0; j<k; j++){
            if(j>=i){
                matrix_set_element(R,i,j,matrix_get_element(R_full,i,j));
            }
        }
    }

    // get Q
    matrix_copy(Q,R_full); 
    //printf("dorgqr..\n");
    LAPACKE_dorgqr(LAPACK_COL_MAJOR, Q->nrows, Q->ncols, min(Q->ncols,Q->nrows), Q->d, Q->nrows, tau->d);
	
    // clean up
    matrix_delete(R_full);
    vector_delete(tau);
}



/* returns Q from [Q,R] = qr(M,'0') compact QR factorization 
M is mxn ; Q is mxn ; R is min(m,n) x min(m,n) */ 
void QR_factorization_getQ(mat *M, mat *Q){
    int64_t i,j,m,n,k;
    m = M->nrows; n = M->ncols;
    k = min(m,n);
    matrix_copy(Q,M);
    vec *tau = vector_new(k);

    LAPACKE_dgeqrf(LAPACK_COL_MAJOR, m, n, Q->d, m, tau->d);
    LAPACKE_dorgqr(LAPACK_COL_MAJOR, m, n, n, Q->d, m, tau->d);
    
    // clean up
    vector_delete(tau);
}





/* computes SVD: M = U*S*Vt; note Vt = V^T */
void singular_value_decomposition(mat *M, mat *U, mat *S, mat *Vt){
    int64_t m,n,k;
    m = M->nrows; n = M->ncols;
    k = min(m,n);
    vec * work = vector_new(2*max(3*min(m, n)+max(m, n), 5*min(m,n)));
    //vec * work = vector_new(k);
    vec * svals = vector_new(k);

    LAPACKE_dgesvd( LAPACK_COL_MAJOR, 'S', 'S', m, n, M->d, m, svals->d, U->d, m, Vt->d, k, work->d );
    
    initialize_diagonal_matrix(S, svals);

    vector_delete(work);
    vector_delete(svals);
}


/*original void singular_value_decomposition(mat *M, mat *U, mat *S, mat *Vt){
    int64_t m,n,k;
    m = M->nrows; n = M->ncols;
    k = min(m,n);
    vec * work = vector_new(2*max(3*min(m, n)+max(m, n), 5*min(m,n)));
    vec * svals = vector_new(k);

    LAPACKE_dgesvd( LAPACK_COL_MAJOR, 'S', 'S', m, n, M->d, m, svals->d, U->d, m, Vt->d, k, work->d );

    initialize_diagonal_matrix(S, svals);

    vector_delete(work);
    vector_delete(svals);
}*/


/* P = U * S * Vt */
void form_svd_product_matrix(mat *U, mat *S, mat *V, mat *P){
    int64_t k,m,n;
    m = P->nrows;
    n = P->ncols;
    k = S->nrows;
    mat * SVt = matrix_new(k,n);

    // form SVt = S*V^T
    matrix_matrix_transpose_mult(S,V,SVt);

    // form P = U*S*V^T
    matrix_matrix_mult(U,SVt,P);

    matrix_delete(SVt);
}


/* P = C * U * R */
void form_cur_product_matrix(mat *C, mat *U, mat *R, mat *P){
    int64_t k,m,n;
    m = P->nrows;
    n = P->ncols;
    k = U->nrows;
    mat * CU = matrix_new(m,k);

    // form CU = C*U
    matrix_matrix_mult(C,U,CU);

    // form P = CU*R
    matrix_matrix_mult(CU,R,P);

    matrix_delete(CU);
}


void estimate_rank_and_buildQ(mat *M, double frac_of_max_rank, double TOL, mat **Q, int64_t *good_rank){
    int64_t m,n,i,j,ind,maxdim;
    double vec_norm;
    mat *RN,*Y,*Qbig,*Qsmall;
    vec *vi,*vj,*p,*p1;
    m = M->nrows;
    n = M->ncols;
    maxdim = round(min(m,n)*frac_of_max_rank);

    vi = vector_new(m);
    vj = vector_new(m);
    p = vector_new(m);
    p1 = vector_new(m);

    // build random matrix
    printf("form RN..\n");
    RN = matrix_new(n, maxdim);
    initialize_random_matrix(RN);

    // multiply to get matrix of random samples Y
    printf("form Y: %d x %d..\n",m,maxdim);
    Y = matrix_new(m, maxdim);
    matrix_matrix_mult(M, RN, Y);

    // estimate rank k and build Q from Y
    printf("form Qbig..\n");
    Qbig = matrix_new(m, maxdim);

    matrix_copy(Qbig, Y);

    printf("estimate rank with TOL = %f..\n", TOL);
    *good_rank = maxdim;
    int64_t forbreak = 0;
    for(j=0; !forbreak && j<maxdim; j++){
        matrix_get_col(Qbig, j, vj);
        for(i=0; i<j; i++){
            matrix_get_col(Qbig, i, vi);
            project_vector(vj, vi, p);
            vector_sub(vj, p);
            if(vector_get2norm(p) < TOL && vector_get2norm(p1) < TOL){
                *good_rank = j;
                forbreak = 1;
                break;
            }
            vector_copy(p1,p);
        }
        vec_norm = vector_get2norm(vj);
        vector_scale(vj, 1.0/vec_norm);
        matrix_set_col(Qbig, j, vj);
    }

    printf("estimated rank = %d\n", *good_rank);
    Qsmall = matrix_new(m, *good_rank);
    *Q = matrix_new(m, *good_rank);
    matrix_copy_first_columns(Qsmall, Qbig);
    QR_factorization_getQ(Qsmall, *Q);

    matrix_delete(RN);
    matrix_delete(Y);
    matrix_delete(Qsmall);
    matrix_delete(Qbig);
}



void estimate_rank_and_buildQ2(mat *M, int64_t kblock, double TOL, mat **Y, mat **Q, int64_t *good_rank){
    int64_t m,n,i,j,ind,exit_loop = 0;
    double error_norm;
    mat *RN,*Y_new,*Y_big,*QtM,*QQtM;
    vec *vi,*vj,*p,*p1;
    m = M->nrows;
    n = M->ncols;

    // build random matrix
    printf("form RN..\n");
    RN = matrix_new(n,kblock);
    initialize_random_matrix(RN);

    // multiply to get matrix of random samples Y
    printf("form Y: %d x %d..\n",m,kblock);
    *Y = matrix_new(m, kblock);
    matrix_matrix_mult(M, RN, *Y);

    ind = 0;
    while(!exit_loop){
        printf("form Q..\n");
        if(ind > 0){
            matrix_delete(*Q);
        }
        *Q = matrix_new((*Y)->nrows, (*Y)->ncols);
        QR_factorization_getQ(*Y, *Q);

        // compute QtM
        QtM = matrix_new((*Q)->ncols, M->ncols);
        matrix_transpose_matrix_mult(*Q,M,QtM);

        // compute QQtM
        QQtM = matrix_new(M->nrows, M->ncols); 
        matrix_matrix_mult(*Q,QtM,QQtM);

        error_norm = 0.01*get_percent_error_between_two_mats(QQtM, M);

        printf("Y is of size %d x %d and error_norm = %f\n", (*Y)->nrows, (*Y)->ncols, error_norm);
        *good_rank = (*Y)->ncols;
       
        // add more samples if needed
        if(error_norm > TOL){
            Y_new = matrix_new(m, kblock);
            initialize_random_matrix(RN);
            matrix_matrix_mult(M, RN, Y_new);

            Y_big = matrix_new((*Y)->nrows, (*Y)->ncols + Y_new->ncols); 
            append_matrices_horizontally(*Y, Y_new, Y_big);
            matrix_delete(*Y);
            *Y = matrix_new(Y_big->nrows,Y_big->ncols);
            matrix_copy(*Y,Y_big);
            
            matrix_delete(Y_big);
            matrix_delete(Y_new);
            matrix_delete(QtM);
            matrix_delete(QQtM);
            ind++;
        }
        else{
            matrix_delete(RN);
            exit_loop = 1;
        }    
    }
}


/* solve A X = B where A is upper triangular matrix and X is a matrix 
invert different ways
1. using tridiagonal matrix system solve
2. using inverse of tridiagonal matrix solve
3. Use SVD of A to compute inverse 
default: solve column by column with tridiagonal system
*/
void upper_triangular_system_solve(mat *A, mat *B, mat *X, int64_t solve_type){
    int64_t j;
    double alpha = 1.0;
    char CblasLeft  ='L' ;
    char CblasUpper ='U' ;
    char CblasNoTrans = 'N';
    char CblasNonUnit = 'N';
    int64_t inc = 1;
    vec *col_vec;
    mat *S;

    //printf("A is %d by %d\n", A->nrows, A->ncols);
    //printf("X is %d by %d\n", X->nrows, X->ncols);
    //printf("B is %d by %d\n", B->nrows, B->ncols);

    if(solve_type == 1){
        S = matrix_new(B->nrows,B->ncols);
        matrix_copy(S,B);
        //cblas_dtrsm(CblasColMajor, CblasLeft, CblasUpper, CblasNoTrans, CblasNonUnit, B->nrows, B->ncols, alpha, A->d, A->nrows, S->d, S->nrows);
        dtrsm(&CblasLeft, &CblasUpper, &CblasNoTrans, &CblasNonUnit, &B->nrows, &B->ncols, &alpha, A->d, &A->nrows, S->d, &S->nrows);

        matrix_copy(X,S);
        matrix_delete(S);
    }
    else if(solve_type == 2){
        invert_upper_triangular_matrix(A);
        matrix_matrix_mult(A,B,X);
    }
    else if(solve_type == 3){
        mat *U, *S, *Sinv, *Vt, *SinvUt, *VSinvUt;
        U = matrix_new(A->nrows, A->nrows);
        S = matrix_new(A->nrows, A->nrows);
        Sinv = matrix_new(A->nrows, A->nrows);
        Vt = matrix_new(A->nrows, A->nrows);
        SinvUt = matrix_new(A->nrows, A->nrows);
        VSinvUt = matrix_new(A->nrows, A->nrows);
        //singular_value_decomposition(A, &U, &S, &Vt);
        singular_value_decomposition(A, U, S, Vt);
        invert_diagonal_matrix(Sinv,S);
        matrix_matrix_transpose_mult(Sinv,U,SinvUt); 
        matrix_transpose_matrix_mult(Vt,SinvUt,VSinvUt);
        matrix_matrix_mult(VSinvUt, B, X);
    }
    else{
        col_vec = vector_new(B->nrows);
        for(j=0; j<B->ncols; j++){
            matrix_get_col(B,j,col_vec);
            //cblas_dtrsv (CblasColMajor, CblasUpper, CblasNoTrans, CblasNonUnit, A->ncols, A->d, A->ncols, col_vec->d, 1);
            dtrsv (&CblasUpper, &CblasNoTrans, &CblasNonUnit, &A->ncols, A->d, &A->ncols, col_vec->d, &inc);
            matrix_set_col(X,j,col_vec);     
        }
        vector_delete(col_vec);
    }
}


void square_matrix_system_solve(mat *A, mat *X, mat *B){
    //LAPACKE_dtrtri( LAPACK_COL_MAJOR, 'U', 'N', Minv->nrows, Minv->d, Minv->nrows);
    int * ipiv = (int*)malloc((A->nrows)*sizeof(int)); 
    LAPACKE_dgesv(LAPACK_COL_MAJOR , A->nrows , B->ncols , A->d , A->ncols , ipiv , B->d , B->ncols );
    matrix_copy(X,B);
    free(ipiv);
}


/* timing function */
double get_seconds_frac(struct timeval start_timeval, struct timeval end_timeval){
    long secs_used, micros_used;
    secs_used= end_timeval.tv_sec - start_timeval.tv_sec;
    micros_used= end_timeval.tv_usec - start_timeval.tv_usec;
	
    return (double)(secs_used + micros_used/1000000.0); 
}

