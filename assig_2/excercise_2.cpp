#include <iostream>
#include <chrono>
#include <cassert>
#include <cmath>
#include <list>
#include <mutex>

std::mutex out;
int max;

#define MARKED true
#define UNMARKED false

int *numbers; // threads only read from this
bool *markers; // threads only write to this
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
            markers[0] = MARKED;
        }
        if (numbers[1] == 1)
        {
            is_prime[1] = MARKED;
            markers[1] = MARKED;
        }
    }

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

void *ParallelSiveOfEratosthenes(void *ptr)
{
    int *slice = reinterpret_cast<int *>(ptr);
    int start_idx = slice[0];     // fetch start index
    int end_idx = slice[1]; // fetch end index

    // out.lock();
    // std::cout << "Thread: " << pthread_self() << " start_idx: " << start_idx << " end_idx: " << end_idx << std::endl;
    // out.unlock();

    int *curr_prime = primes_to_check;

    while(*curr_prime != 0){
        for (int i = start_idx; i <= end_idx; i++)
        {
            if (numbers[i] % *curr_prime == 0)
            {
                markers[i] = MARKED;
            }
        }
        curr_prime++;
    }
    return NULL;
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

    SequencialSive(numbers, sqrt_max + 1);

    int prime_idx = 0;
    while (primes_to_check[prime_idx] != 0)
    {
        std::cout << primes_to_check[prime_idx] << ",";
        prime_idx++;
    }
    std::cout << std::endl;
    std::cout << "First primes fetched!" << std::endl;

    pthread_t threads[num_threads];

    int slice_size = (max - sqrt_max) / num_threads;
    int remainder = (max - sqrt_max) % num_threads;

    std::cout << "starting timer!" << std::endl;
    auto start_time = std::chrono::system_clock::now();

    int slice_idx = sqrt_max +1;
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

    // wait for all threads to finish
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    std::chrono::duration<double> duration =
        (std::chrono::system_clock::now() - start_time);
    // *** timing ends here ***
    std::cout << "Finished in " << duration.count() << " seconds (wall clock)." << std::endl;
    
    std::cout << "Primes: ";
    for (int i = 1; i < max; i++) {
        if (markers[i] == UNMARKED) {
            std::cout << numbers[i] << " ";
        }
    }
    std::cout << std::endl;
    // cleanup
    delete[] numbers;
    delete[] markers;
    delete[] primes_to_check;

    return 0;
}