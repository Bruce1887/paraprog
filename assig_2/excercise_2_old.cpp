#include <iostream>
#include <chrono>
#include <cassert>
#include <cmath>
#include <list>

int max;

#define MARKED true
#define UNMARKED false


struct {
  bool marked;
  int value;
} typedef number;

number new_number(int value)
{
  number n;
  n.value = value;
  n.marked = UNMARKED;
  return n;
}


// used to verify
bool isPrime(number n)
{
  if (n.value <= 1)
  {
    return false;
  }
  for (int i = 2; i <= std::sqrt(n.value); ++i)
  {
    if (n.value % i == 0)
    {
      return false;
    }
  }
  return true;
}

int find_lowest_unmarked(int start, number *numbers, int max)
{
  std::cout << "start: " << start << std::endl;
  std::cout << "max: " << max << std::endl;

  for (int i = start; i < max; i++)
  {
    std::cout << "i: " << i << ", value: " << numbers[i].value << ", marked: " << numbers[i].marked << std::endl;
    if (numbers[i].marked == UNMARKED)
    { //
      return i;
    }
  }
  return -1;
}

std::list<number> SiveOfEratosthenes(number* numbers, int max) // numbers must be in increasing order!
{ 
  int lowest_unmarked = numbers[0].value;

  while (lowest_unmarked * lowest_unmarked < max)
  {
    for (int i = 0; i < max; i++)
    {
      int num = lowest_unmarked * lowest_unmarked + i * lowest_unmarked;
      if (num > max)
      {
        break;
      }
      numbers[num].marked = MARKED;
    }

    lowest_unmarked = find_lowest_unmarked(lowest_unmarked, numbers, max);
    if (lowest_unmarked == -1)
    {
      std::cout << "error: lowest_unmarked is -1" << std::endl;
      exit(-1);
    }

    std::cout << "lowest_unmarked: " << lowest_unmarked << std::endl;
  }

  std::list<number> num_list;

  for (int i = 2; i < max; i++)
  {
    if (numbers[i].marked == UNMARKED)
    {
      assert(isPrime(numbers[i]));
      num_list.push_back(numbers[i]);      
    }
  }

  return num_list;
}

void usage(char *program)
{
  std::cout << "Usage: " << program << " M T, where M is the max number to check primes for and T is the amount of threads." << std::endl;
  exit(1);
}

int main(int argc, char *argv[])
{

  if (argc != 3)
  {
    usage(argv[0]);
  }
  int num_threads = atoi(argv[1]);
  max = atoi(argv[2]);

  if (max <= 0 || num_threads <= 0)
  {
    usage(argv[0]);
  }
  
  int sqrt_max = std::sqrt(max);
  std::cout << "sqrt_max: " << sqrt_max << std::endl;
  pthread_t threads[num_threads];
  int slice_idx = 0;
  int slice_size = sqrt_max / num_threads;
  int remainder = sqrt_max % num_threads;

  number numbers[max];
  for (int i = 1; i <= sqrt_max; i++)
  {
    numbers[i] = new_number(i);
  } 
  std::list<number> primes_under_sqrt = SiveOfEratosthenes(numbers, sqrt_max);

  exit(0);
  
  for (int i = 0; i < num_threads; i++)
  {
    int *slice_arr = new int[2];
    slice_arr[0] = slice_idx;
    slice_arr[1] = slice_idx + slice_size;
    if (i == num_threads - 1)
    {
      slice_arr[1] += remainder;
    }
    slice_idx += slice_size;
    void *ptr = slice_arr;  
    // pthread_create(&threads[i], NULL, SiveOfEratosthenes(), ptr);
  }
  

  std::cout << "starting timer!" << std::endl;
  auto start_time = std::chrono::system_clock::now();
  

  //SiveOfEratosthenes(max);

  std::chrono::duration<double> duration =
      (std::chrono::system_clock::now() - start_time);
  // *** timing ends here ***
  std::cout << "Finished in " << duration.count() << " seconds (wall clock)." << std::endl;
  return 0;
}