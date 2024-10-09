#include <omp.h>
#include <chrono>
#include <iostream>
#include <cassert>

void matrixMultiply_i(int dim, long double** a, long double** b, long double** c) {
    #pragma omp parallel for schedule(static) shared(a, b, c, dim) 
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            for (int k = 0; k < dim; k++) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

void matrixMultiply_ii(int dim, long double** a, long double** b, long double** c) {
    #pragma omp parallel for schedule(static) collapse(2) shared(a, b, c, dim)
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            long double sum = 0.0; 
            for (int k = 0; k < dim; k++) {
                sum += a[i][k] * b[k][j];
            }
            c[i][j] = sum;
        }
    }
}

void matrixMultiply_iii(int dim, long double** a, long double** b, long double** c) {
    #pragma omp parallel for schedule(static) collapse(3) shared(a, b, c, dim)
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            for (int k = 0; k < dim; k++) {
                long double temp = a[i][j] * b[j][k];
                //#pragma omp atomic
                c[i][k] += temp;
            }
        }
    }
}

//create matrix
long double** createMatrix(int rows, int cols) {
    long double** matrix = new long double*[rows];
    for (int i = 0; i < rows; ++i) {
        matrix[i] = new long double[cols];
        std::fill(matrix[i], matrix[i] + cols, 0.0);
    }
    return matrix;
}

//delete a matrix 
void deleteMatrix(long double** matrix, int rows) {
    for (int i = 0; i < rows; ++i) {
        delete[] matrix[i];
    }
    delete[] matrix;
}

int main(int argc, char* argv[]) {
    int rows;
    int cols;
    int num_threads;
    if(argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <rows> <cols> <num_threads>" << std::endl;
        return 1;
    }
    rows = atoi(argv[1]);
    cols = atoi(argv[2]);
    num_threads = atoi(argv[3]);

    omp_set_num_threads(num_threads);
    assert(omp_get_max_threads() == num_threads);

    long double** matrix_a = createMatrix(rows, cols);
    long double** matrix_b = createMatrix(rows, cols);
    long double** matrix_ci = createMatrix(rows, cols);
    long double** matrix_cii = createMatrix(rows, cols);
    long double** matrix_ciii = createMatrix(rows, cols);



    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            matrix_a[i][j] = i * cols + j + 1; 
            matrix_b[i][j] = i * cols + j + 1; 
        }
    }
    
    //i
    std::cout << "starting timer! for i)" << std::endl;
    auto start_time_1 = std::chrono::system_clock::now();
    matrixMultiply_i(rows, matrix_a, matrix_b, matrix_ci);
    std::chrono::duration<double> duration_1 =
        (std::chrono::system_clock::now() - start_time_1);
    std::cout << "Finished in " << duration_1.count() << " seconds (wall clock)." << std::endl;

    //ii
    std::cout << "starting timer! for ii)" << std::endl;
    auto start_time_2 = std::chrono::system_clock::now();
    matrixMultiply_ii(rows, matrix_a, matrix_b, matrix_cii);
    std::chrono::duration<double> duration_2 =
        (std::chrono::system_clock::now() - start_time_2);
    std::cout << "Finished in " << duration_2.count() << " seconds (wall clock)." << std::endl;
    
    //iii
    std::cout << "starting timer! for iii)" << std::endl;
    auto start_time_3 = std::chrono::system_clock::now();
    matrixMultiply_iii(rows, matrix_a, matrix_b, matrix_ciii);
    std::chrono::duration<double> duration_3 =
        (std::chrono::system_clock::now() - start_time_3);
    std::cout << "Finished in " << duration_3.count() << " seconds (wall clock)." << std::endl;
    
 
    long double threshold = 0.000001; // we are comparing floating point numbers, thus we allow for some error
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
           // std::cout << "[" << i << "," << j << "]" << std::endl;
            if(abs(matrix_ci[i][j] - matrix_cii[i][j]) > threshold) {
                std::cerr << "Matrices are not 1 and 2 equal! " << matrix_ci[i][j] << " != " << matrix_cii[i][j] << std::endl;
                return 1;
            }
            if(abs(matrix_ci[i][j] - matrix_ciii[i][j]) > threshold) {
                std::cerr << "Matrices are not 1 and 3 equal! " << matrix_ci[i][j] << " != " << matrix_cii[i][j] << std::endl;
                return 1;
            }
            // if(matrix_ci[i][j] != matrix_ciii[i][j]) {
            //     std::cout << "[i,j]: " << "[" << i << "," << j << "]"  << " diff: " << abs(matrix_ci[i][j] - matrix_cii[i][j])<< std::endl;
            // }
            // assert(matrix_ci[i][j] == matrix_cii[i][j]); // assert first is equal to second
            // assert(matrix_ci[i][j] == matrix_ciii[i][j]); // assert first is equal to third
        }
    }
    std::cout << "All asserts passed! All matrices are equal!" << std::endl;

    deleteMatrix(matrix_a, rows);
    deleteMatrix(matrix_b, rows);
    deleteMatrix(matrix_ci, rows);
    deleteMatrix(matrix_cii, rows);
    deleteMatrix(matrix_ciii, rows);

    return 0;
}