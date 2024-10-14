#include <omp.h>
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <sys/time.h>

int N;
std::vector<std::vector<float>> A;
std::vector<float> *b;
std::vector<float> *xrow_seq;
std::vector<float> *xrow_par;
std::vector<float> *xcol_seq;
std::vector<float> *xcol_par;

void initialise()
{
    std::vector<std::vector<float>> nested_array(N, std::vector<float>(N));
    A = nested_array;

    // Allocate memory for the right-hand side vector and solution vectors
    b = new std::vector<float>(N);
    xrow_seq = new std::vector<float>(N);
    xrow_par = new std::vector<float>(N);
    xcol_seq = new std::vector<float>(N);
    xcol_par = new std::vector<float>(N);

    // Initialize an upper triangular matrix A and right-hand side vector b
    for (int i = 0; i < N; i++)
    {
        (*b)[i] = static_cast<float>(i + 1);

        for (int j = 0; j < N; j++)
        {
            if (j < i)
                A[i][j] = 0.0; // Elements below the diagonal
            else
                A[i][j] = static_cast<float>(2);
            // A[i][j] = static_cast<float>(1 + rand() * % 4); // Random values from 1 to 4
            // A[i][j] = static_cast<float>(2 + (rand() * 2) % 4); // Randomly 2 or 4
        }
    }
}

void cleanup()
{
    A.clear();
    b->clear();
    xrow_seq->clear();
    xrow_par->clear();
    xcol_par ->clear();
    xcol_seq ->clear();
}

void row_oriented_back_substitution_sequential(std::vector<std::vector<float>> *A, std::vector<float> *b, std::vector<float> *x, int N)
{
    int row, col;
    for (row = N - 1; row >= 0; row--)
    {
        (*x)[row] = (*b)[row];
        for (col = row + 1; col < N; col++)
            (*x)[row] -= (*A)[row][col] * (*x)[col];
        (*x)[row] /= (*A)[row][row];
    }
}

void row_oriented_back_substitution_parallel(std::vector<std::vector<float>> *A, std::vector<float> *b, std::vector<float> *x, int N)
{
    int row, col;
    for (row = N - 1; row >= 0; row--)
    {
        (*x)[row] = (*b)[row];  
        float temp = 0.0f;      

        #pragma omp parallel for private(col) reduction(-:temp) schedule(dynamicd)
        for (col = row + 1; col < N; col++)
        {
            temp -= (*A)[row][col] * (*x)[col];
        }

        (*x)[row] += temp;
        (*x)[row] /= (*A)[row][row];
    }
}

void column_oriented_back_substitution_sequential(std::vector<std::vector<float>> *A, std::vector<float> *b, std::vector<float> *x, int N)
{
    int row, col;
    for (row = 0; row < N; row++)
        (*x)[row] = (*b)[row];
    for (col = N - 1; col >= 0; col--)
    {
        (*x)[col] /= (*A)[col][col];
        for (row = 0; row < col; row++)
            (*x)[row] -= (*A)[row][col] * (*x)[col];
    }
}
void column_oriented_back_substitution_parallel(std::vector<std::vector<float>> *A, std::vector<float> *b, std::vector<float> *x, int N)
{
    int row, col;
#pragma omp parallel for schedule(static)
    for (row = 0; row < N; row++)
        (*x)[row] = (*b)[row];
    for (col = N - 1; col >= 0; col--)
    {
#pragma omp single
        (*x)[col] /= (*A)[col][col];
#pragma omp parallel for schedule(static)
        for (row = 0; row < col; row++)
            (*x)[row] -= (*A)[row][col] * (*x)[col];
    }
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <N> <num_threads>" << std::endl;
        exit(EXIT_FAILURE);
    }

    N = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    omp_set_num_threads(num_threads);

    std::cout << "Initialising system of equations..." << std::endl;
    initialise();
    std::cout << "System of equations initialised." << std::endl;

    std::cout << std::endl;
    // variables for timing
    float row_time_seq;
    float row_time_par;
    float col_time_seq;
    float col_time_par;
    struct timeval row_ts_seq, row_tf_seq;
    struct timeval row_ts_par, row_tf_par;
    struct timeval col_ts_seq, col_tf_seq;
    struct timeval col_ts_par, col_tf_par;

    std::cout << "Starting column-oriented-sequential approach timer..." << std::endl;
    gettimeofday(&col_ts_seq, NULL);
    column_oriented_back_substitution_sequential(&A, b, xcol_seq, N);
    gettimeofday(&col_tf_seq, NULL);
    col_time_seq = (col_tf_seq.tv_sec - col_ts_seq.tv_sec) + (col_tf_seq.tv_usec - col_ts_seq.tv_usec) * 0.000001;
    printf("Column-oriented-sequential time: %lf\n", col_time_seq);

    std::cout << std::endl;
    
    std::cout << "Starting column-oriented-parallel approach timer..." << std::endl;
    gettimeofday(&col_ts_par, NULL);
    column_oriented_back_substitution_parallel(&A, b, xcol_par, N);
    gettimeofday(&col_tf_par, NULL);
    col_time_par = (col_tf_par.tv_sec - col_ts_par.tv_sec) + (col_tf_par.tv_usec - col_ts_par.tv_usec) * 0.000001;
    printf("Column-oriented-parallel time: %lf\n", col_time_par);
    
    std::cout << std::endl;

    std::cout << "Starting row-oriented-sequential approach timer..." << std::endl;
    gettimeofday(&row_ts_seq, NULL);
    row_oriented_back_substitution_sequential(&A, b, xrow_seq, N);
    gettimeofday(&row_tf_seq, NULL);
    row_time_seq = (row_tf_seq.tv_sec - row_ts_seq.tv_sec) + (row_tf_seq.tv_usec - row_ts_seq.tv_usec) * 0.000001;
    printf("Row-oriented-sequential time: %lf\n", row_time_seq);
    
    std::cout << std::endl;

    std::cout << "Starting row-oriented-parallel approach timer..." << std::endl;
    gettimeofday(&row_ts_par, NULL);
    row_oriented_back_substitution_parallel(&A, b, xrow_par, N);
    gettimeofday(&row_tf_par, NULL);
    row_time_par = (row_tf_par.tv_sec - row_ts_par.tv_sec) + (row_tf_par.tv_usec - row_ts_par.tv_usec) * 0.000001;
    printf("Row-oriented-parallel time: %lf\n", row_time_par);
    
    std::cout << std::endl;

    float threshold = 1.0;
    int ineq_counter = 0;
    for (int i = 0; i < N; i++)
    {
        if (fabs((*xrow_par)[i] - (*xcol_par)[i]) > threshold)
        {
            ineq_counter++;
            // std::cout << "INEQUALITY AT INDEX: " << i << ", xrow: " << xrow[i] << " xcol: " << xcol[i] << ", diff: " << xcol[i] - xrow[i] << std::endl;
            // exit(EXIT_FAILURE);
        }
    }

    if (ineq_counter == 0)
        std::cout << "SUCCESS! Both parallel approaches are equal." << std::endl;
    else
        std::cout << "FAILURE! Number of inequalities between x values from col and row oriented parallel approach: " << ineq_counter << std::endl;

    cleanup();
    return 0;
}