#include <iostream>
#include <chrono>
#include <cassert>
#include <cmath>
#include <list>

int max;

#define MARKED true
#define UNMARKED false

int *numbers;
bool *markers;
int *primes_to_check;

void usage(char *program)
{
    std::cout << "Usage: " << program << " M T, where M is the max number to check primes for and T is the amount of threads." << std::endl;
    exit(1);
}

// this is a paper bag with dog shit in it thats on fire. dont look at this code.
void SequencialSive(int *numbers, int len)
{
    bool is_prime[len] = {UNMARKED};


    // mark 0 and 1 if they are in the array
    if (len > 2)
    {
        if (numbers[0] == 0)
        {
            is_prime[0] = MARKED;
        }
        if (numbers[1] == 1)
        {
            is_prime[1] = MARKED;
        }
    }

    for (int k = 2; k * k <= len; k++)
    {
        if (is_prime[k] == UNMARKED)
        {
            for (int i = k * k; i <= len; i += k)
            {
                is_prime[i] = MARKED;
            }
        }
    }

    int num_primes = 0;

    // count the number of primes to know how much memory to allocate
    for (int i = 0; i <= len; i++)
    {
        if (is_prime[i] == UNMARKED)
        { // primes are unmarked
            num_primes++;
        }
    }
    std::cout << "num_primes: " << num_primes << std::endl;

    // allocate memory for the primes
    int *primes = new int[num_primes + 1]{0};

    int primes_idx = 0;
    // fill the primes array
    for (int i = 0; i <= len; i++)
    {
        if (is_prime[i] == UNMARKED)
        { // PRIMES ARE UNMARKED
            primes[primes_idx] = numbers[i];
            primes_idx++;
        }
    }

    primes_to_check = primes;
}

void *ParallelSiveOfEratosthenes(void *data)
{
  
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        usage(argv[0]);
    }
    int num_threads = atoi(argv[1]);
    max = atoi(argv[2]) + 1;

    if (max <= 0 || num_threads <= 0)
    {
        usage(argv[0]);
    }

    // allocate memory for numbers and markers
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

    SequencialSive(numbers, sqrt_max +1);

    int prime_idx = 0;
    while (primes_to_check[prime_idx] != 0)
    {
        std::cout << primes_to_check[prime_idx] << std::endl;
        prime_idx++;
    }

    std::cout << "First primes fetched!" << std::endl;
    exit(0);

    pthread_t threads[num_threads];
    int slice_size = (max - sqrt_max) / num_threads;
    int remainder = (max - sqrt_max) % num_threads;

    std::cout << "starting timer!" << std::endl;
    auto start_time = std::chrono::system_clock::now();

    int slice_idx = 0;
    for (int i = 0; i < num_threads; i++)
    {
        int *slice_arr = new int[2];
        slice_arr[0] = slice_idx;
        slice_arr[1] = slice_idx + slice_size;
        if (i == num_threads - 1) // last thread gets the remainder
        {
            slice_arr[1] += remainder;
        }
        slice_idx += slice_size;
        void *ptr = slice_arr;
        pthread_create(&threads[i], NULL, ParallelSiveOfEratosthenes, ptr);
    }

    // cleanup
    delete[] numbers;
    delete[] markers;
    delete[] primes_to_check;

    return 0;
}