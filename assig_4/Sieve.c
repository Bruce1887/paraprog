#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <mpi.h>
#include <stdlib.h>

#define MARKED true
#define UNMARKED false

// This function is optimized for efficiency and clarity.
// returns an array of primes up to len
int* SequencialSive(int *numbers, bool* markers, int len) {
    bool is_prime[len];
    for(int i =0; i < len; i++){
        is_prime[i] = UNMARKED;
    }

    // mark 0 and 1 if they are in the array
    if (len > 2) {
        if (numbers[0] == 0) {
            is_prime[0] = MARKED;
            markers[0] = MARKED;
        }
        if (numbers[1] == 1) {
            is_prime[1] = MARKED;
            markers[1] = MARKED;
        }
    }

    for (int k = 2; k * k <= len; k++) {
        if (is_prime[k] == UNMARKED) {
            for (int i = k * k; i <= len; i += k) {
                is_prime[i] = MARKED;
                markers[i] = MARKED;
            }
        }
    }

    int num_primes = 0;

    // count the number of primes to know how much memory to allocate
    for (int i = 0; i <= len; i++) {
        if (is_prime[i] == UNMARKED) { // primes are unmarked
            //printf("primes: %d\n", numbers[i]);
            num_primes++;
        }
    }
    printf("num_primes: %d\n", num_primes);

    // allocate memory for the primes
    int *primes = (int *)malloc((num_primes + 1) * sizeof(int)); // null terminated
    primes[0] = 0; // Initialize to 0

    int primes_idx = 0;
    // fill the primes array
    for (int i = 0; i <= len; i++) {
        if (is_prime[i] == UNMARKED) { // PRIMES ARE UNMARKED
            primes[primes_idx] = numbers[i];
            primes_idx++;
        }
    }

    return primes;
}

int ParallelSieveOfEratosthenes(int *primes,int *numbers) {
    int len = 0;
    
    while(numbers[len] != 0){
        len++;
    }
    bool markers[len];
    for (int i = 0; i < len; i++) {
        markers[i] = UNMARKED;
    }

    int *curr_prime = primes;
    while (*curr_prime != 0) {
        int curr_num = *numbers;
        while (curr_num != 0) {
            if (curr_num % *curr_prime == 0 && curr_num != *curr_prime) {
                markers[curr_num] = MARKED;
            }
            numbers++;
            curr_num = *numbers;
        }
        curr_prime++;
    }
    int primes_found = 0;
    for (int i = 0; i < len; i++) {
        if (markers[i] == UNMARKED) {
            primes_found++;
        }
    }
    return primes_found;
}

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    int max = 100; // set at compile time and not runtime, as an argument to the program    
    int max_msg_size = max; // To ensure we will never send too muchh data

    if(rank == 0) {
        int *numbers;
        bool *markers;
        int *primes_to_check;

        printf("size: %d",size);
        // Allocate memory for numbers and markers
        numbers = (int *)calloc(max, sizeof(int));
        markers = (bool *)calloc(max, sizeof(bool));
        for (int i = 0; i < max; ++i) {
            numbers[i] = i;
            markers[i] = UNMARKED;
        }
        int sqrt_max = (int)sqrt(max);

        printf("sqrt_max: %d\n", sqrt_max);

        printf("Fetching first primes up to sqrt_max...\n");

        primes_to_check = SequencialSive(numbers, markers, sqrt_max + 1);

        printf("First primes fetched!\n");

        printf("starting timer!\n");
        clock_t start_time = clock();

        // Send the primes to all processes
        for (int i = 1; i < size; i++) {
            MPI_Send(primes_to_check, max_msg_size, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
        // Send the slice of numbers to all processes
        for (int r = 1; r < size; r++) {
            int slice_size = (max - sqrt_max) / size;
            
            int start = sqrt_max + 1 + r * slice_size;
            int end = start + slice_size - 1;

            if (r == size - 1) // Last thread gets the remainder
            {
                end += (max - sqrt_max) % size;
            }
            int *numbers_slice = (int *)malloc((end - start + 1) * sizeof(int));
            for (int i = start; i <= end; i++) {
                numbers_slice[i - start] = numbers[i];
            }
            MPI_Send(numbers_slice, max_msg_size, MPI_INT, r, 0, MPI_COMM_WORLD);
        }

        // wait for all processes to be ready
        MPI_Barrier(MPI_COMM_WORLD);
        
        int primes_count = 0;
        for(int r = 1; r < size; r++) {
            int *num_primes_found;
            MPI_Recv(num_primes_found, 1, MPI_INT, r, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            primes_count += *num_primes_found;
        }
        printf("Finished in %f seconds (wall clock).\n", (double)(clock() - start_time) / CLOCKS_PER_SEC);

        // add the primes found when running the sequential sieve
        int *ptr = primes_to_check;
        while (*ptr != 0) {
            primes_count++;
            ptr++;
        }

        printf("Total number of primes found up to max: %d\n", primes_count);        
        free(numbers);
        free(markers);
        free(primes_to_check);
    }
    else {
        int *primes_to_check;
        int *numbers_slice;
        // Receive the primes
        MPI_Recv(primes_to_check, max_msg_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        // Receive the slice of numbers
        MPI_Recv(numbers_slice, max_msg_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Perform the parallel sieve
        int primes_found_in_slice = ParallelSieveOfEratosthenes(primes_to_check, numbers_slice);

        int *to_send = calloc(1, sizeof(int));
        *to_send = primes_found_in_slice;
        // send the number of primes found to the root process with rank 0
        MPI_Send(to_send, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        // free(to_send);
    }

    MPI_Finalize();
    return 0;
}