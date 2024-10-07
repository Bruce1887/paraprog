#include <iostream>
#include <chrono>
#include <cassert>
#include <cmath>
#include <list>
#include <mutex>
#include <omp.h>

std::mutex out;
int max;

#define MARKED true
#define UNMARKED false

int *numbers;  // threads only read from this
bool *markers; // threads only write to this
int *primes_to_check;

void usage(char *program)
{
    std::cout << "Usage: " << program << " M T, where M is the max number to check primes for and T is the amount of threads." << std::endl;
    exit(1);
}

// This function is optimized for efficiency and clarity.
void SequentialSieve(int *numbers, int len)
{
    bool is_prime[len] = {UNMARKED};

    // Mark 0 and 1 if they are in the array
    if (len > 2)
    {
        if (numbers[0] == 0)
        {
            is_prime[0] = MARKED;
            markers[0] = MARKED;
        }
        if (numbers[1] == 1)
        {
            is_prime[1] = MARKED;
            markers[1] = MARKED;
        }
    }

    // Efficiently mark multiples of primes using a loop instead of nested loops
    for (int k = 2; k * k <= len; k++)
    {
        if (is_prime[k] == UNMARKED)
        {
            for (int i = k * k; i <= len; i += k)
            {
                is_prime[i] = MARKED;
                markers[i] = MARKED;
            }
        }
    }

    // Count primes and allocate memory for them
    int num_primes = 0;
    for (int i = 0; i <= len; i++)
    {
        if (is_prime[i] == UNMARKED)
        {
            num_primes++;
        }
    }
    std::cout << "num_primes: " << num_primes << std::endl;

    int *primes = new int[num_primes + 1]{0};

    // Fill the primes array
    int primes_idx = 0;
    for (int i = 0; i <= len; i++)
    {
        if (is_prime[i] == UNMARKED)
        {
            primes[primes_idx] = numbers[i];
            primes_idx++;
        }
    }

    primes_to_check = primes;
}

// void ParallelSieveOfEratosthenes()
// {
//     #pragma omp parallel for schedule(static)
//     for (int i = sqrt_max + 1; i < max; i++)
//     {
//         if (markers[i] == UNMARKED)
//         {
//             for (int j = i * i; j < max; j += i)
//             {
//                 markers[j] = MARKED;
//             }
//         }
//     }
// }

void ParallelSieveOfEratosthenes(int start, int end)
{
    // the following print purpose is to check start and end values. They seem to be ok.
    // out.lock();
    // std::cout << "Thread " << omp_get_thread_num() << " start: " << start << ". end: " << end << std::endl;
    // out.unlock();

    for (long long i = start; i <= end; i++)
    {        
        long j = i * i;
        // out.lock();
        // std::cout << "Thread " << omp_get_thread_num() << " i: " << i << " ,j: "<< j << std::endl;
        // out.unlock();
        // assert(j >= 0);
        
        if (markers[i] == UNMARKED)
        {
            while (j < max)
            {
                markers[j] = MARKED;
                j+=i;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        usage(argv[0]);
    }
    max = atoi(argv[1]);
    int num_threads = atoi(argv[2]);

    if (max <= 0 || num_threads <= 0)
    {
        usage(argv[0]);
    }

    // Allocate memory for numbers and markers
    numbers = new int[max];
    for (int i = 0; i < max; ++i)
    {
        numbers[i] = i;
    }
    markers = new bool[max];
    for (int i = 0; i < max; ++i)
    {
        markers[i] = UNMARKED;
    }
    int sqrt_max = std::sqrt(max);

    std::cout << "sqrt_max: " << sqrt_max << std::endl;

    std::cout << "Fetching first primes up to sqrt_max..." << std::endl;

    SequentialSieve(numbers, sqrt_max + 1);

    // int prime_idx = 0;
    // while (primes_to_check[prime_idx] != 0)
    // {
    //     std::cout << primes_to_check[prime_idx] << ",";
    //     prime_idx++;
    // }
    // std::cout << std::endl;
    std::cout << "First primes fetched!" << std::endl;

    std::cout << "starting timer!" << std::endl;
    auto start_time = std::chrono::system_clock::now();

#pragma omp parallel num_threads(num_threads)
    {
        int thread_id = omp_get_thread_num();
        int slice_size = (max - sqrt_max) / num_threads;
        int start = sqrt_max + 1 + thread_id * slice_size;
        int end = start + slice_size - 1;

        if (thread_id == num_threads - 1) // Last thread gets the remainder
        {
            end += (max - sqrt_max) % num_threads;
        }

        ParallelSieveOfEratosthenes(start, end);
    }

    std::chrono::duration<double> duration =
        (std::chrono::system_clock::now() - start_time);
    // *** timing ends here ***

    // std::cout << "Primes: ";
    // for (int i = 1; i < max; i++) {
    //     if (markers[i] == UNMARKED) {
    //         std::cout << numbers[i] << " ";
    //     }
    // }
    // std::cout << std::endl;

    std::cout << "Finished in " << duration.count() << " seconds (wall clock)." << std::endl;
    // cleanup
    delete[] numbers;
    delete[] markers;
    delete[] primes_to_check;

    return 0;
}