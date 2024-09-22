#include <iostream>
#include "thread_safe_sorted_lists.hpp"
#include <sstream>
#include "benchmark.hpp"
#include "excercise_4.h"

/*
 * Written by Edvin Bruce and Viktor Wallsten in 2024 as part of an assignment.
 * Many parts in this code are stolen from a benchmarking example written by David Klaftenegger (i think, but may be wrong) in 2015.
 */

static const int DATA_VALUE_RANGE_MIN = 0;
static const int DATA_VALUE_RANGE_MAX = 256;
static const int DATA_PREFILL = 512;

void usage(char *program)
{
    std::cout << "Usage: " << program << " O, N, where O is the option (an integer) and N is the amount of worker threads." << std::endl;
    std::cout << "The Options are: " << std::endl
              << "1: Coarse grained locking (std::mutex)" << std::endl
              << "2: Fine grained locking (std::mutex)" << std::endl
              << "3: Coarse grained locking (TATAS)" << std::endl
              << "4: Fine grained locking(TATAS)" << std::endl
              << "5: Fine grained locking with scalable queue lock algorithm (TODO: specify wether it is CLH or MCS)" << std::endl;

    exit(1);
}

template <typename List>
void read(List &l, int random)
{
    /* read operations: 100% count */
    l.count(random % DATA_VALUE_RANGE_MAX);
}

template <typename List>
void update(List &l, int random)
{
    /* update operations: 50% insert, 50% remove */
    auto choice = (random % (2 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
    if (choice == 0)
    {
        l.insert(random % DATA_VALUE_RANGE_MAX);
    }
    else
    {
        l.remove(random % DATA_VALUE_RANGE_MAX);
    }
}

template <typename List>
void mixed(List &l, int random)
{
    /* mixed operations: 6.25% update, 93.75% count */
    auto choice = (random % (32 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
    if (choice == 0)
    {
        l.insert(random % DATA_VALUE_RANGE_MAX);
    }
    else if (choice == 1)
    {
        l.remove(random % DATA_VALUE_RANGE_MAX);
    }
    else
    {
        l.count(random % DATA_VALUE_RANGE_MAX);
    }
}

void initialise_list(int option, list_superclass<int> *&l, char *argv[])
{
    std::cout << "initialising list..." << std::endl;
    switch (option)
    {
    case 1:
        l = new cg_mutex_sorted_list<int>;
        break;
    case 2:
        l = new fg_mutex_sorted_list<int>;
        break;
    case 3:
        l = new cg_tatas_sorted_list<int>;
        break;
    case 4:
        l = new fg_tatas_sorted_list<int>;
        break;
    case 5:
        std::cout << "Not implemented yet" << std::endl;
        break;
    default:
        std::cerr << u8"Invalid option '" << argv[1] << u8"'\n"; // should never get here
        std::exit(EXIT_FAILURE);
        break;
    }
    
    /* set up random number generator */
    std::random_device rd;
    std::mt19937 engine(rd());
    std::uniform_int_distribution<int> uniform_dist(DATA_VALUE_RANGE_MIN, DATA_VALUE_RANGE_MAX);

    /* prefill list with 1024 elements */
    for (int i = 0; i < DATA_PREFILL; i++)
    {
        l->insert(uniform_dist(engine));
    }
    std::cout << "list initialised." << std::endl;
}

int main(int argc, char *argv[])
{
    // check inputs
    if (argc != 3)
    {
        usage(argv[0]);
    }
    std::istringstream ss(argv[1]);
    int option;
    if (!(ss >> option))
    {
        std::cerr << u8"Invalid option '" << argv[1] << u8"'\n";
        std::exit(EXIT_FAILURE);
    }
    if (option < 1 || option > 5)
    {
        std::cerr << u8"Invalid option '" << argv[1] << u8"'\n";
        std::exit(EXIT_FAILURE);
    }
    std::istringstream ss_2(argv[2]);
    int threadcnt;
    if (!(ss_2 >> threadcnt))
    {
        std::cerr << u8"Invalid number of threads '" << argv[2] << u8"'\n";
        std::exit(EXIT_FAILURE);
    }
    // input checks complete

    // declare list
    list_superclass<int> *l;

    /* start with fresh list: update test left list in random size */    
    initialise_list(option, l, argv);

    std::cout << "running read benchmark..." << std::endl;
    benchmark(threadcnt, u8"non-thread-safe read", [&l](int random)
              { read(*l, random); });
    exit(0);

        std::cout << "running update benchmark..." << std::endl;
    benchmark(threadcnt, u8"non-thread-safe update", [&l](int random)
              { update(*l, random); });

    /* start with fresh list: update test left list in random size */    
    initialise_list(option, l, argv);
    
    std::cout << "running mixed benchmark..." << std::endl;
    benchmark(threadcnt, u8"non-thread-safe mixed", [&l](int random)
              { mixed(*l, random); });

    return EXIT_SUCCESS;
}