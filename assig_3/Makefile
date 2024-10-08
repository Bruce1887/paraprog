# Compiler and flags
CC = g++
CCV = -std=c++11
CFLAGS = -g -Wall -fopenmp

# Target programs
PROG1 = Sieve_Of_Eratosthenes
PROG2 = matrix

# Sanitizer
SANITIZER = false
ifeq ($(SANITIZER),true)
    CFLAGS += -fsanitize=thread
endif

all: $(PROG1).cpp
	g++ $(CCV) $(CFLAGS) $(PROG1).cpp -o $(PROG1)
	g++ $(CCV) $(CFLAGS) $(PROG2).cpp -o $(PROG2)

# Clean up built files
clean:
	rm -f $(TARGETS) *.o