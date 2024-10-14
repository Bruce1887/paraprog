# Compiler and flags
CC = mpicc 
CCV = -std=c++11
CFLAGS = -Wall -fopenmp
# Target programs
PROG1 = hello_mpi

SANITIZER = false
ifeq ($(SANITIZER),true)
    CFLAGS += -fsanitize=thread
endif

all:
	$(CC) $(PROG1).c -o mpitest

# Run the program
run:
	mpiexec --hostfile hosts ./mpitest
		
# Clean up built files
clean:
	rm -f $(PROG1)
