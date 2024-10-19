#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <mpi/mpi.h>

#define MARKED true
#define UNMARKED false

int max;
int *primes_to_check;

void SequentialSieve(int len) {
    bool *is_prime = (bool*)calloc(len, sizeof(bool));

    for (int i = 0; i < len; i++) {
        is_prime[i] = UNMARKED;
    }

    is_prime[0] = MARKED; // 0 is not prime
    is_prime[1] = MARKED; // 1 is not prime
    

    for (int k = 2; k * k < len; k++) {
        if (is_prime[k] == UNMARKED) {
            for (int i = k * k; i < len; i += k) {
                is_prime[i] = MARKED;
            }
        }
    }

    int num_primes = 0;

    // Count the number of primes to know how much memory to allocate
    for (int i = 2; i < len; i++) {
        if (is_prime[i] == UNMARKED) {
            num_primes++;
        }
    }
    printf("Number of primes up to sqrt(max): %d\n", num_primes);

    // Allocate memory for the primes
    primes_to_check = (int*)calloc((num_primes + 1), sizeof(int));

    int primes_idx = 0;
    // populate the primes array
    for (int i = 2; i < len; i++) {
        if (is_prime[i] == UNMARKED) {
            primes_to_check[primes_idx] = i;
            primes_idx++;
        }
    }
    
    primes_to_check[num_primes] = 0;  // Fix: ensure termination of the array

    // Free is_prime array
    free(is_prime);
}

int main(int argc, char *argv[]) {
    int rank, size;
    double start_time, end_time;
    int *numbers = NULL;
    MPI_Init(&argc, &argv);               
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size); 

    int max = 10000;

    int sqrt_max = (int)sqrt(max) + 1;

    if (rank == 0) {
        printf("sqrt_max: %d\n", sqrt_max);
        printf("Fetching first primes up to sqrt_max...\n");
    }

    // Allocate the numbers array on all processes before broadcasting
    numbers = (int*)calloc(max + 1, sizeof(int));

    // root populerar listan
    if (rank == 0) {
        for (int i = 0; i < max; ++i) {
            numbers[i] = i;
        }
    }
    
    MPI_Bcast(numbers, max + 1, MPI_INT, 0, MPI_COMM_WORLD); // bara root får köra bordcast

    if (rank == 0) {
        SequentialSieve(sqrt_max);
        printf("First primes fetched!\n");
    }

    //räkna antalet primes så vi vet hur mycket vi ska skicka
    int num_primes = 0;
    if (rank == 0) {
        while (primes_to_check[num_primes] != 0) {
            num_primes++;
        }
    }

    MPI_Bcast(&num_primes, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //allokera minne för primes_to_check för de som inte är root
    if (rank != 0) {
        primes_to_check = (int*)calloc((num_primes + 1), sizeof(int)); 
        if (primes_to_check == NULL) {
            perror("Failed to allocate memory for primes_to_check");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Bcast(primes_to_check, num_primes + 1, MPI_INT, 0, MPI_COMM_WORLD);  // Broadcast primes_to_check, +1 på num_primes pga primes_to_check är null-terminated

    // dela upp arbetet
    int slice_size = (max - sqrt_max) / size;

    int start_value = sqrt_max + rank * slice_size;
    int end_value = sqrt_max + (rank + 1) * slice_size - 1;

    if (rank == size - 1) {  // Last process takes the rest
        end_value = max - 1;
    }

    int local_size = end_value - start_value + 1;

    //varje process allokerar minne för sina markers
    bool *markers = (bool*)calloc(local_size, sizeof(bool));
    if (markers == NULL) {
        perror("Failed to allocate memory for markers");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (int i = 0; i < local_size; i++) {
        markers[i] = UNMARKED;
    }

    // Start timing
    MPI_Barrier(MPI_COMM_WORLD);  
    start_time = MPI_Wtime();

    // parallel sieve
    int *curr_prime = primes_to_check;

    while(*curr_prime != 0){
        for (int i = start_value; i <= end_value; i++) {
            if (numbers[i] % *curr_prime == 0) {
                markers[i - start_value] = MARKED; 
            }
        }
        curr_prime++;
    }

    // End timing
    MPI_Barrier(MPI_COMM_WORLD);  
    end_time = MPI_Wtime();

    int total_local_primes = 0;
    for (int i = 0; i < local_size; i++) {
        if (markers[i] == UNMARKED) {
            total_local_primes++;
        }
    }

    int total_primes = 0;
    MPI_Reduce(&total_local_primes, &total_primes, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        total_primes += num_primes;
        printf("Total number of primes less than %d: %d\n", max, total_primes);
        printf("Finished in %f seconds (wall clock).\n", end_time - start_time);
    }

    free(markers);
    free(numbers);
    free(primes_to_check);

    MPI_Finalize();
    return 0;
}