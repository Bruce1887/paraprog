#include <iostream>
#include <mutex>
#include <chrono>
#include <cassert>

double **trapezoids_xs;

double area_sum = 0;
std::mutex area_sum_lock;

// only read these variables
int num_threads;
int num_trapezoids;

void usage(char *program)
{
  std::cout << "Usage: " << program << " num_threads num_trapezoids (both are integers >0)" << std::endl;
  exit(1);
}

double inner_function(double x)
{
  return 4 / (1 + x * x);
}

// calculate the area of a trapezoid
double calc_trap_area(double x1, double x2)
{
  // TODO: Is this the proper area calculation? Verify!
  return (inner_function(x1) + inner_function(x2)) / 2 * (x2 - x1);
}

void *thread_fn(void *ptr)
{
  int* slice = reinterpret_cast<int*>(ptr);
  int idx = slice[0]; // fetch start index
  int end_idx = slice[1]; // fetch end index

  double local_sum = 0;

  while (idx < end_idx)
  {
    double *trapezoid = trapezoids_xs[idx];
    double area = calc_trap_area(trapezoid[0], trapezoid[1]);

    local_sum += area;
    idx++;
  }

  area_sum_lock.lock();
  area_sum += local_sum;
  area_sum_lock.unlock();
  return NULL;
}

int main(int argc, char *argv[])
{
  if (argc == 2)
  {
    if (std::string(argv[1]) == "-h") // this was a requirement. kind of uneccesary since the usage function is called whenever input is wrong
    {
      usage(argv[0]);
    }
  }

  if (argc != 3)
  {
    usage(argv[0]);
  }

  // this should be the only writes to these variables
  num_threads = atoi(argv[1]);
  num_trapezoids = atoi(argv[2]);

  if (num_threads <= 0 || num_trapezoids <= 0)
  {
    usage(argv[0]);
  }
  // now all input checks are done and we can start the actual program

  std::cout << "Number of threads: " << num_threads << std::endl;
  std::cout << "Number of trapezoids: " << num_trapezoids << std::endl;

  // define trapezoids (only their width i.e. the x values)
  double trapezoid_width = (double)1 / num_trapezoids;

  // this is the only write to trapexoids_xs
  trapezoids_xs = new double *[num_trapezoids];

  for (int i = 0; i < num_trapezoids; i++)
  {
    trapezoids_xs[i] = new double[2];
  }
  for (int i = 0; i < num_trapezoids; i++)
  {
    trapezoids_xs[i][0] = trapezoid_width * i;
    trapezoids_xs[i][1] = trapezoid_width * (i + 1);
    // std::cout << "Trapezoid " << i << ": " << trapezoids_xs[i][0] << " - " << trapezoids_xs[i][1] << std::endl;
  }

  pthread_t threads[num_threads];

  int slice_size = num_trapezoids / num_threads;
  int remainder = num_trapezoids % num_threads;

  std::cout << "starting timer!" << std::endl;

  int slice_idx = 0;
  // *** timing begins here *** 
  auto start_time = std::chrono::system_clock::now();

  // create threads
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
    pthread_create(&threads[i], NULL, thread_fn, ptr);
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

  // cleanup
  std::cout << "cleaning up the memory!" << std::endl;
  for (int i = 0; i < num_trapezoids; i++)
  {
    delete[] trapezoids_xs[i];
  }
  delete[] trapezoids_xs;


  std::cout << "Area: " << area_sum << std::endl;
  return 0;
}