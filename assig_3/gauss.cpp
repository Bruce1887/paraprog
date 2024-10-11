#include <omp.h>
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <sys/time.h>

int N;
std::vector<std::vector<float>> A;
std::vector<float> *b;
std::vector<float> *xrow;
std::vector<float> *xcol;

void initialise()
{
    std::vector<std::vector<float>> nested_array(N, std::vector<float>(N)); 
    A = nested_array;

    // Allocate memory for the right-hand side vector and solution vectors
    b = new std::vector<float>(N);
    xrow = new std::vector<float>(N);
    xcol = new std::vector<float>(N);
    

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
    xrow->clear();
    xcol->clear();
}

void row_oriented_back_substitution(std::vector<std::vector<float>> *A, std::vector<float> *b, std::vector<float> *x, int N)
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

void column_oriented_back_substitution(std::vector<std::vector<float>> *A, std::vector<float> *b, std::vector<float>  *x, int N)
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
     //variables for timing
	float row_time;
    float col_time;
    struct timeval row_ts,row_tf;
	struct timeval col_ts,col_tf;


    std::cout << "Starting column-oriented approach timer..." << std::endl;
    gettimeofday(&col_ts,NULL);
    column_oriented_back_substitution(&A, b, xcol, N);
    gettimeofday(&col_tf,NULL);
	col_time = (col_tf.tv_sec-col_ts.tv_sec)+(col_tf.tv_usec-col_ts.tv_usec)*0.000001;
	printf("Column-oriented time: %lf\n", col_time);

    std::cout << std::endl;

    std::cout << "Starting row-oriented approach timer..." << std::endl;
    gettimeofday(&row_ts,NULL);
    row_oriented_back_substitution(&A, b, xrow, N);
    gettimeofday(&row_tf,NULL);
	row_time = (row_tf.tv_sec-row_ts.tv_sec)+(row_tf.tv_usec-row_ts.tv_usec)*0.000001;
	printf("Row-oriented time: %lf\n", row_time);

    std::cout << std::endl;

    float threshold = 1.0;
    int ineq_counter = 0;
    for (int i = 0; i < N; i++)
    {
        if (fabs((*xrow)[i] - (*xcol)[i]) > threshold)
        {
            ineq_counter++;
            // std::cout << "INEQUALITY AT INDEX: " << i << ", xrow: " << xrow[i] << " xcol: " << xcol[i] << ", diff: " << xcol[i] - xrow[i] << std::endl;
            // exit(EXIT_FAILURE);
        }
    }

    if (ineq_counter == 0)
        std::cout << "SUCCESS! Both approaches are equal." << std::endl;
    else
        std::cout << "FAILURE! Number of inequalities between x values from col and row oriented approach: " << ineq_counter << std::endl;

    cleanup();
    return 0;
}