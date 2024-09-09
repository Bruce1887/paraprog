#include <iostream>
#include <pthread.h>

void usage(char *program)
{
  std::cout << "Usage: " << program << " num_threads num_trapezoids (both are integers >0)" << std::endl;
  exit(1);
}

double inner_function(double x) {
    return 4 / (1+x*x);
}

// calculate the area of a trapezoid
double calc_trap_area(double x1, double x2) {
    // TODO: Is this the proper area calculation? Verify!
    return (inner_function(x1) + inner_function(x2)) / 2 * (x2 - x1);
}

int main(int argc,char* argv[]) {
    if (argc == 2)
    {
        if(std::string(argv[1]) == "-h") // this was a requirement. kind of uneccesary since the usage function is called whenever input is wrong
        {
            usage(argv[0]);
        }
    }

    if (argc != 3)
    {
      usage(argv[0]);
    }

    int num_threads = atoi(argv[1]);
    int num_trapezoids = atoi(argv[2]);

    if (num_threads <= 0 || num_trapezoids <= 0)
    {
      usage(argv[0]);
    }
    // now all input checks are done and we can start the actual program
    
    std::cout << "Number of threads: " << num_threads << std::endl;
    std::cout << "Number of trapezoids: " << num_trapezoids << std::endl;
 
    
    // define trapezoids (only their width i.e. the x values)
    double trapezoid_width = (double) 1 / num_trapezoids;
    double** trapezoids_xs = new double*[num_trapezoids];
    for (int i = 0; i < num_trapezoids; i++) {
        trapezoids_xs[i] = new double[2];
    }
    for (int i = 0; i < num_trapezoids; i++) {
        trapezoids_xs[i][0] = trapezoid_width * i;
        trapezoids_xs[i][1] = trapezoid_width * (i+1);
        std::cout << "Trapezoid " << i << ": " << trapezoids_xs[i][0] << " - " << trapezoids_xs[i][1] << std::endl;
    }

    double area_sum = 0;
    for (int i = 0; i < num_trapezoids; i++) {
        area_sum += calc_trap_area(trapezoids_xs[i][0], trapezoids_xs[i][1]);
    }
    std::cout << "Area: " << area_sum << std::endl;

    //cleanup
    for (int i = 0; i < num_trapezoids; i++) {
        delete[] trapezoids_xs[i];
    }
    delete[] trapezoids_xs;

    return 0;
}

