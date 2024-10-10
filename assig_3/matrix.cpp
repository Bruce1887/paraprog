#include <omp.h>
#include <chrono>
#include <iostream>
#include <cassert>
#include <cmath>

void matrixMultiply_i(int dim, int **a, int **b, int **c)
{
#pragma omp parallel for schedule(static) shared(a, b, c, dim)
    for (int i = 0; i < dim; i++)
    {
        for (int j = 0; j < dim; j++)
        {
            for (int k = 0; k < dim; k++)
            {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

void matrixMultiply_ii(int dim, int **a, int **b, int **c)
{
#pragma omp parallel for schedule(static) collapse(2) shared(a, b, c, dim)
    for (int i = 0; i < dim; i++)
    {
        for (int j = 0; j < dim; j++)
        {
            int sum = 0.0;
            for (int k = 0; k < dim; k++)
            {
                sum += a[i][k] * b[k][j];
            }
            c[i][j] = sum;
        }
    }
}

void matrixMultiply_iii(int dim, int **a, int **b, int **c)
{
    // std::cout << "hello from matrixMultiply_iii" << std::endl;
#pragma omp parallel for schedule(dynamic) collapse(3) shared(a, b, c, dim)
    for (int i = 0; i < dim; i++)
    {
        for (int j = 0; j < dim; j++)
        {
            for (int k = 0; k < dim; k++)
            {
                // std::cout << "matrixMultiply_iii, third for loop" << std::endl;
                // std::cout << "i: " << i << " j: " << j << " k: " << k << std::endl;
                int temp = a[i][j] * b[j][k];
                c[i][k] += temp; // data dependency, cant be parallelized
            }
        }
    }
    // std::cout << "end of matrixMultiply_iii" << std::endl;
}

// create matrix
int **createMatrix(int rows, int cols)
{
    int **matrix = new int *[rows];
    for (int i = 0; i < rows; ++i)
    {
        matrix[i] = new int[cols];
        std::fill(matrix[i], matrix[i] + cols, 0.0);
    }
    return matrix;
}

// delete a matrix
void deleteMatrix(int **matrix, int rows)
{
    for (int i = 0; i < rows; ++i)
    {
        delete[] matrix[i];
    }
    delete[] matrix;
}

int main(int argc, char *argv[])
{
    int rows;
    int cols;
    int num_threads;
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <rows> <cols> <num_threads>" << std::endl;
        return 1;
    }
    rows = atoi(argv[1]);
    cols = atoi(argv[2]);
    num_threads = atoi(argv[3]);

    omp_set_num_threads(num_threads);
    assert(omp_get_max_threads() == num_threads);

    int **matrix_a = createMatrix(rows, cols);
    int **matrix_b = createMatrix(rows, cols);
    int **matrix_ci = createMatrix(rows, cols);
    int **matrix_cii = createMatrix(rows, cols);
    int **matrix_ciii = createMatrix(rows, cols);

    // initialize matrices a and b with numbers 1-5
    int numba = 1;
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            matrix_a[i][j] = numba;
            matrix_b[i][j] = numba + 1;
            numba < 5 ? numba++ : numba = 1;
        }
    }

    // i
    std::cout << "starting timer! for i)" << std::endl;
    auto start_time_1 = std::chrono::system_clock::now();
    matrixMultiply_i(rows, matrix_a, matrix_b, matrix_ci);
    std::chrono::duration<double> duration_1 =
        (std::chrono::system_clock::now() - start_time_1);
    std::cout << "Finished in " << duration_1.count() << " seconds (wall clock)." << std::endl;

    // ii
    std::cout << "starting timer! for ii)" << std::endl;
    auto start_time_2 = std::chrono::system_clock::now();
    matrixMultiply_ii(rows, matrix_a, matrix_b, matrix_cii);
    std::chrono::duration<double> duration_2 =
        (std::chrono::system_clock::now() - start_time_2);
    std::cout << "Finished in " << duration_2.count() << " seconds (wall clock)." << std::endl;

    // iii
    std::cout << "starting timer! for iii)" << std::endl;
    auto start_time_3 = std::chrono::system_clock::now();
    matrixMultiply_iii(rows, matrix_a, matrix_b, matrix_ciii);
    std::chrono::duration<double> duration_3 =
        (std::chrono::system_clock::now() - start_time_3);
    std::cout << "Finished in " << duration_3.count() << " seconds (wall clock)." << std::endl;

    // std::cout << matrix_ci[0][0] << std::endl;
    // std::cout << matrix_cii[0][0] << std::endl;
    // std::cout << matrix_ciii[0][0] << std::endl;

    int zero_count = 0;
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            if (matrix_ciii[i][j] == 0.0)
            {
                zero_count++;
            }
        }
    }
    std::cout << "Number of zeros in matrix_ciii: " << zero_count << std::endl; // should be 0, but might be a gazillion

    // matrix_ciii[0][0] = 0.0;
    // std::cout << matrix_ciii[0][0] << std::endl;
    // std::cout << abs(matrix_ci[0][0] - matrix_ciii[0][0]) << std::endl;

    int* equalities = new int[3]{0};
    int threshold = 0.001; // we are comparing floating point numbers, thus we allow for some error
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            if (abs(matrix_ci[i][j] - matrix_cii[i][j]) > threshold)
            { // compare first and second
                if (equalities[0] == 0) std::cerr << "Matrices are not 1 and 2 equal! " << "[" << i << "," << j << "], " << matrix_ci[i][j] << " != " << matrix_cii[i][j] << std::endl;
                equalities[0] +=1;
            }
            if (abs(matrix_ci[i][j] - matrix_ciii[i][j]) > threshold)
            { // compare first and third
                if (equalities[1] == 0) std::cerr << "Matrices are not 1 and 3 equal! " << "[" << i << "," << j << "], " << matrix_ci[i][j] << " != " << matrix_ciii[i][j] << std::endl;
                equalities[1] += 1;
            }
           
            if (abs(matrix_cii[i][j] - matrix_ciii[i][j]) > threshold)
            { // compare second and third
                if (equalities[2] == 0) std::cerr << "Matrices are not 2 and 3 equal! " << "[" << i << "," << j << "], " << matrix_cii[i][j] << " != " << matrix_ciii[i][j] << std::endl;
                equalities[2] += 1;
            }
        }
    }

    std::cout << "inequalities 1 and 2: " << equalities[0] << std::endl;
    std::cout << "inequalities 1 and 3: " << equalities[1] << std::endl;
    std::cout << "inequalities 2 and 3: " << equalities[2] << std::endl;

    deleteMatrix(matrix_a, rows);
    deleteMatrix(matrix_b, rows);
    deleteMatrix(matrix_ci, rows);
    deleteMatrix(matrix_cii, rows);
    deleteMatrix(matrix_ciii, rows);

    return 0;
}