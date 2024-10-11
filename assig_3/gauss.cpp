#include <omp.h>
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

int N;
double **A = new double *[N];
double *b = new double[N];
double *xrow = new double[N];
double *xcol = new double[N];

void initialise()
{
    // Allocate memory for the left hand side (nested arrays)
    A = new double *[N];
    for (int i = 0; i < N; i++)
    {
        A[i] = new double[N];
    }
    // Allocate memory for the right-hand side vector and solution vectors
    b = new double[N];
    xrow = new double[N];
    xcol = new double[N];
    

    // Initialize an upper triangular matrix A and right-hand side vector b
    for (int i = 0; i < N; i++)
    {
        b[i] = static_cast<double>(i + 1);

        for (int j = 0; j < N; j++)
        {
            if (j < i)
                A[i][j] = 0.0; // Elements below the diagonal
            else
                A[i][j] = static_cast<double>(2);
                // A[i][j] = static_cast<double>(1 + rand() * % 4); // Random values from 1 to 4
                // A[i][j] = static_cast<double>(2 + (rand() * 2) % 4); // Randomly 2 or 4
        }
    }
}

void cleanup()
{
    // Deallocate memory
    for (int i = 0; i < N; i++)
    {
        delete[] A[i];
    }
    delete[] A;
    delete[] b;
    delete[] xrow;
    delete[] xcol;
}

void row_oriented_back_substitution(double **A, double *b, double *x, int N)
{
    int row, col;
#pragma omp parallel for schedule(static)
    for (row = N - 1; row >= 0; row--)
    {
        // #pragma omp single
        x[row] = b[row];
        for (col = row + 1; col < N; col++)
            x[row] -= A[row][col] * x[col];
        x[row] /= A[row][row];
    }
}

void column_oriented_back_substitution(double **A, double *b, double *x, int N)
{
    int row, col;

#pragma omp parallel for schedule(runtime)
    for (row = 0; row < N; row++)
        x[row] = b[row];

    for (col = N - 1; col >= 0; col--)
    {
#pragma omp single
        x[col] /= A[col][col];
#pragma omp parallel for
        for (row = 0; row < col; row++)
            x[row] -= A[row][col] * x[col];
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <N>" << std::endl;
        exit(EXIT_FAILURE);
    }
    N = atoi(argv[1]);

    std::cout << "Initialising system of equations..." << std::endl;
    initialise();
    std::cout << "System of equations initialised." << std::endl;

    std::cout << std::endl;

    std::cout << "Starting column-oriented approach timer..." << std::endl;
    double start_time_col = omp_get_wtime();
    column_oriented_back_substitution(A, b, xrow, N);
    double end_time_col = omp_get_wtime();
    std::cout << "Column-oriented time: " << end_time_col - start_time_col << " s" << std::endl;

    std::cout << std::endl;

    std::cout << "Starting row-oriented approach timer..." << std::endl;
    double start_time_row = omp_get_wtime();
    row_oriented_back_substitution(A, b, xcol, N);
    double end_time_row = omp_get_wtime();
    std::cout << "Row-oriented time: " << end_time_row - start_time_row << " s" << std::endl;

    std::cout << std::endl;

    double threshold = 1.0;
    int ineq_counter = 0;
    for (int i = 0; i < N; i++)
    {
        if (fabs(xrow[i] - xcol[i]) > threshold)
        {
            ineq_counter++;
            std::cout << "INEQUALITY AT INDEX: " << i << ", xrow: " << xrow[i] << " xcol: " << xcol[i] << ", diff: " << xcol[i] - xrow[i] << std::endl;
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