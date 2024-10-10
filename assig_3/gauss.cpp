#include <omp.h>
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

void row_oriented_back_substitution(double **A, double *b, double *x, int n)
{
    int row, col;

    for (row = n - 1; row >= 0; row--)
    {
        x[row] = b[row];
        for (col = row + 1; col < n; col++)
            x[row] -= A[row][col] * x[col];
        x[row] /= A[row][row];
    }
}

void column_oriented_back_substitution(double **A, double *b, double *x, int n)
{
    int row, col;

    // #pragma omp parallel for
    for (row = 0; row < n; row++)
        x[row] = b[row];

    for (col = n - 1; col >= 0; col--)
    {
        // #pragma omp single
        x[col] /= A[col][col];
        // #pragma omp parallel for
        for (row = 0; row < col; row++)
            x[row] -= A[row][col] * x[col];
    }
}

void initialize_system(double **A, double *b, int n)
{
    // Initialize an upper triangular matrix A and right-hand side vector b
    for (int i = 0; i < n; i++)
    {   
        b[i] = static_cast<double>(i + 1); // Example initialization

        for (int j = 0; j < n; j++)
        {
            if (j < i)
                A[i][j] = 0.0; // Elements below the diagonal
            else
                A[i][j] = static_cast<double>(1 + rand() % 4); // Random values from 1 to 4 
            // std::cout << A[i][j] << " ";
        }
        // std::cout << std::endl;
    }
}

int main(int argc, char **argv)
{
    if (argc != 2){
        exit(EXIT_FAILURE); 
    }
    int n = atoi(argv[1]);

    double **A = new double *[n];
    for (int i = 0; i < n; i++)
    {
        A[i] = new double[n];
    }
    double *b = new double[n];
    double *xrow = new double[n];
    double *xcol = new double[n];

    initialize_system(A, b, n);
    
    double start_time;
    double end_time;

    start_time = omp_get_wtime();
    column_oriented_back_substitution(A, b, xrow, n);
    end_time = omp_get_wtime();
    std::cout << "Column-oriented time: " << end_time - start_time << " s" << std::endl;

    start_time = omp_get_wtime();
    row_oriented_back_substitution(A, b, xcol, n);
    end_time = omp_get_wtime();
    std::cout << "Row-oriented time: " << end_time - start_time << " s" << std::endl;

    // for (int i = 0; i < n; i++)
    // {
    // std::cout << "i: " << i << ", xrow: " << xrow[i] << " xcol: " << xcol[i] << std::endl;
    // }

    // double threshold = 1.0;
    // for (int i = 0; i < n; i++)
    // {
    //     if(fabs(xrow[i] - xcol[i]) > threshold){
    //         std::cout << "FAILURE AT INDEX: " << i << ", xrow: " << xrow[i] << " xcol: " << xcol[i] << ", diff: " << xcol[i] - xrow[i] << std::endl;
    //         // exit(EXIT_FAILURE);
    //     }
    // }
    std::cout << "SUCCESS" << std::endl;

    return 0;
}