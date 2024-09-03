#include <iostream>
#include <thread>
#include <chrono>

const int iterations = 100;

void loop(char *data, int size)
{
  for (int i=0; i<iterations; ++i)
    {
      for (int j=0; j<size; ++j)
        {
          ++data[j];  // read and update data[j]
        }
    }
}

void usage(char *program)
{
  std::cout << "Usage: " << program << " T N" << std::endl;
  std::cout << std::endl;
  std::cout << "  T: number of threads" << std::endl;
  std::cout << "  N: array size in MB" << std::endl;
  exit(1);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
    {
      usage(argv[0]);
    }

  // threads = argv[1]
  int threads;
  try
    {
      threads = std::stoi(argv[1]);
    }
  catch (std::exception const&)
    {
      usage(argv[0]);
    }
  if (threads < 1)
    {
      usage(argv[0]);
    }

  // size = argv[2]
  int size;
  try
    {
      size = std::stoi(argv[2]);
    }
  catch (std::exception const&)
    {
      usage(argv[0]);
    }
  if (size < 1)
    {
      usage(argv[0]);
    }

  // *** timing begins here ***
  auto start_time = std::chrono::system_clock::now();

  // allocate and initialize data[]
  size *= 1024 * 1024; // convert size from MB to bytes
  char *data = new char[size]();

  // create and join threads
  std::thread *t = new std::thread[threads];
  int size_per_thread = size / threads;
  char *thread_data = data;
  for (int i=0; i<threads; ++i)
    {
      t[i] = std::thread(loop, thread_data, size_per_thread);
      thread_data += size_per_thread;
    }

  for (int i=0; i<threads; ++i)
    {
      t[i].join();
    }

  delete[] data;

  std::chrono::duration<double> duration =
    (std::chrono::system_clock::now() - start_time);
  // *** timing ends here ***

  std::cout << "Finished in " << duration.count() << " seconds (wall clock)." << std::endl;

  return 0;
}
