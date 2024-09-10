#include <iostream>
#include <chrono>

int max;

void SiveOfEratosthenes(int max){

    //start with assuming every number is a prime
    bool is_prime[max]; // index of the prime number so needs to be of size n +1
    for (int i = 0; i < max; i++){
        is_prime[i] = true;
    }

    for (int k = 2; k * k < max; k++) {
        //if a numer is a prime every multiple of that number is set to false
            if (is_prime[k] == true) {
                for (int i = k * k; i <= max; i += k)
                    is_prime[i] = false;
            }
        }
    
    for (int p = 2; p < max; p++){
        if (is_prime[p])
        {
            std::cout << p << std::endl;
        }
    }   
}

void usage(char *program)
{
  std::cout << "Usage: " << program << " input a positiv integer" << std::endl;
  exit(1);
}

int main(int argc, char *argv[]){

    max = atoi(argv[1]);

    if (argc != 2)
    {
      usage(argv[0]);
    }

    if (max <= 0)
    {
      usage(argv[0]);
    }

    std::cout << "starting timer!" << std::endl;
    auto start_time = std::chrono::system_clock::now();

    SiveOfEratosthenes(max);

    std::chrono::duration<double> duration =
    (std::chrono::system_clock::now() - start_time);
  // *** timing ends here ***
  std::cout << "Finished in " << duration.count() << " seconds (wall clock)." << std::endl;
    return 0;
}