#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <mpi/mpi.h>
#include <string.h>

#define MARKED true
#define UNMARKED false

int max;
int *primes_to_check;

void SequentialSieve(int len)
{
    bool *is_prime = (bool *)calloc(len, sizeof(bool));

    for (int i = 0; i < len; i++)
    {
        is_prime[i] = UNMARKED;
    }

    is_prime[0] = MARKED; // 0 is not prime
    is_prime[1] = MARKED; // 1 is not prime

    for (int k = 2; k * k < len; k++)
    {
        if (is_prime[k] == UNMARKED)
        {
            for (int i = k * k; i < len; i += k)
            {
                is_prime[i] = MARKED;
            }
        }
    }

    int num_primes = 0;

    // Count the number of primes to know how much memory to allocate
    for (int i = 2; i < len; i++)
    {
        if (is_prime[i] == UNMARKED)
        {
            num_primes++;
        }
    }

    // if (rank == 0) printf("Number of primes up to sqrt(max): %d\n", num_primes); // i dont want to many prints

    // Allocate memory for the primes
    primes_to_check = (int *)calloc((num_primes + 1), sizeof(int));

    int primes_idx = 0;
    // populate the primes array
    for (int i = 2; i < len; i++)
    {
        if (is_prime[i] == UNMARKED)
        {
            primes_to_check[primes_idx] = i;
            primes_idx++;
        }
    }

    primes_to_check[num_primes] = 0; // Fix: ensure termination of the array

    // Free is_prime array
    free(is_prime);
}

int main(int argc, char *argv[])
{
    int rank, size;
    double start_time, end_time;
    int *numbers = NULL;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int max_msg = 100;
    char msg[100];

    int max = 100;

    bool IS_ROOT = rank == 0;

    int sqrt_max = (int)sqrt(max) + 1;

    if (IS_ROOT)
    {
        printf("sqrt_max: %d\n", sqrt_max);
        printf("Fetching first primes up to sqrt_max...\n");
    }

    numbers = (int *)calloc(max + 1, sizeof(int));
    for (int i = 0; i < max; ++i)
    {
        numbers[i] = i;
    }

    SequentialSieve(sqrt_max);
    printf("First primes fetched!\n");

    // PRINT THE SLICES
    // ################################################################################
    if (IS_ROOT)
    {
        for (int i = 0; i < size; i++)
        {
            int slice_size = (max - sqrt_max) / size;

            int start_value = sqrt_max + i * slice_size;
            int end_value = sqrt_max + (i + 1) * slice_size - 1;

            if (i == size - 1)
            { // Last process takes the rest
                end_value = max - 1;
            }
            printf("Process %d slice: %d - %d\n", i, start_value, end_value);
        }
    }
    // ################################################################################

    // dela upp arbetet
    int slice_size = (max - sqrt_max) / size;

    int start_value = sqrt_max + rank * slice_size;
    int end_value = sqrt_max + (rank + 1) * slice_size - 1;

    if (rank == size - 1)
    { // Last process takes the rest
        end_value = max - 1;
    }

    int local_size = end_value - start_value + 1;

    // varje process allokerar minne för sina markers
    bool *markers = (bool *)calloc(local_size, sizeof(bool));
    if (markers == NULL)
    {
        perror("Failed to allocate memory for markers");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (int i = 0; i < local_size; i++)
    {
        markers[i] = UNMARKED;
    }

    // Start timing
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    // parallel sieve
    int *curr_prime = primes_to_check;

    while (*curr_prime != 0)
    {
        for (int i = start_value; i <= end_value; i++)
        {
            if (numbers[i] % *curr_prime == 0)
            {
                markers[i - start_value] = MARKED;
            }
        }
        curr_prime++;
    }

    // End timing
    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();

    int total_local_primes = 0;
    for (int i = 0; i < local_size; i++)
    {
        if (markers[i] == UNMARKED)
        {            
            total_local_primes++;
        }
    }

    int total_primes = total_local_primes;

    // Root process collects the results, sums them up and prints the total
    // Workers send their local count to the root process
    if (rank == 0)
    {
        curr_prime = primes_to_check;
        while (*curr_prime != 0)
        {
            total_primes += 1 ; // count the primes that was counted at the start (before the timing)
            curr_prime++;
        }
        
        for (int i = 1; i < size; i++)
        {
            int received;
            MPI_Recv(&received, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("Root received %d amount of primes from process %d\n", received, i);
            total_primes += received;
        }
    }
    else
    {
        MPI_Send(&total_local_primes, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    if (rank == 0)
    {
        printf("Total number of primes less than %d: %d\n", max, total_primes);
        printf("Finished in %f seconds (wall clock).\n", end_time - start_time);
    }

    free(markers);
    free(numbers);
    free(primes_to_check);

    MPI_Finalize();
    return 0;
}