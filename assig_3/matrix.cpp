#include <omp.h>
#include <chrono>
#include <iostream>


void matrixMultiply_i(int dim, double** a, double** b, double** c) {
    #pragma omp parallel for schedule(static) shared(a, b, c, dim) default(none) 
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            for (int k = 0; k < dim; k++) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

void matrixMultiply_ii(int dim, double** a, double** b, double** c) {
    #pragma omp parallel for schedule(static) collapse(2) shared(a, b, c, dim) default(none)
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            double sum = 0.0; 
            for (int k = 0; k < dim; k++) {
                sum += a[i][k] * b[k][j];
            }
            c[i][j] = sum;
        }
    }
}

void matrixMultiply_iii(int dim, double** a, double** b, double** c) {
    #pragma omp parallel for schedule(static) collapse(3) shared(a, b, c, dim) default(none)
    for (int i = 0; i < dim; i++) {
        for (int k = 0; k < dim; k++) {
            for (int j = 0; j < dim; j++) {
                double temp = a[i][k] * b[k][j];
                #pragma omp atomic
                c[i][j] += temp;
            }
        }
    }
}

//create matrix
double** createMatrix(int rows, int cols) {
    double** matrix = new double*[rows];
    for (int i = 0; i < rows; ++i) {
        matrix[i] = new double[cols];
        std::fill(matrix[i], matrix[i] + cols, 0.0);
    }
    return matrix;
}

//delete a matrix 
void deleteMatrix(double** matrix, int rows) {
    for (int i = 0; i < rows; ++i) {
        delete[] matrix[i];
    }
    delete[] matrix;
}

int main(){
    int rows = 2048;
    int cols = 2048;

    double** matrix_a = createMatrix(rows, cols);
    double** matrix_b = createMatrix(rows, cols);
    double** matrix_c = createMatrix(rows, cols);

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            matrix_a[i][j] = i * cols + j + 1; 
            matrix_b[i][j] = i * cols + j + 1; 
        }
    }
    //i
    std::cout << "starting timer! for i)" << std::endl;
    auto start_time_1 = std::chrono::system_clock::now();
    matrixMultiply_i(rows, matrix_a, matrix_b, matrix_c);
    std::chrono::duration<double> duration_1 =
        (std::chrono::system_clock::now() - start_time_1);
    std::cout << "Finished in " << duration_1.count() << " seconds (wall clock)." << std::endl;

    //ii
    std::cout << "starting timer! for ii)" << std::endl;
    auto start_time_2 = std::chrono::system_clock::now();
    matrixMultiply_ii(rows, matrix_a, matrix_b, matrix_c);
    std::chrono::duration<double> duration_2 =
        (std::chrono::system_clock::now() - start_time_2);
    std::cout << "Finished in " << duration_2.count() << " seconds (wall clock)." << std::endl;
    
    //iii
    std::cout << "starting timer! for iii)" << std::endl;
    auto start_time_3 = std::chrono::system_clock::now();
    matrixMultiply_iii(rows, matrix_a, matrix_b, matrix_c);
    std::chrono::duration<double> duration_3 =
        (std::chrono::system_clock::now() - start_time_3);
    std::cout << "Finished in " << duration_3.count() << " seconds (wall clock)." << std::endl;
    
    deleteMatrix(matrix_a, rows);
    deleteMatrix(matrix_b, rows);
    deleteMatrix(matrix_c, rows);

    return 0;
}