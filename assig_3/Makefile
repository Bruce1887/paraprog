# Compiler and flags
CC = g++
CCV = -std=c++11
CFLAGS = -g -Wall -fopenmp

# Target programs
PROG1 = Sieve_Of_Eratosthenes
PROG2 = conway
# PROG2_EXAMPLE = Game_Of_Life # gcc $(CFLAGS) $(PROG2_EXAMPLE).c -o $(PROG2_EXAMPLE)
PROG3 = matrix
PROG4 = gauss

SANITIZER = false
ifeq ($(SANITIZER),true)
    CFLAGS += -fsanitize=thread
endif

OUTPUT = false
ifeq ($(OUTPUT),true)
    CFLAGS += -DOUTPUT
endif

all:
	g++ $(CCV) $(CFLAGS) $(PROG1).cpp -o $(PROG1)
	g++ $(CCV) $(CFLAGS) $(PROG2).cpp -o $(PROG2)
	g++ $(CCV) $(CFLAGS) $(PROG3).cpp -o $(PROG3)
	g++ $(CCV) $(CFLAGS) $(PROG4).cpp -o $(PROG4)

# Clean up built files
clean:
	rm -f $(PROG1) $(PROG2) $(PROG2_EXAMPLE) $(PROG4) *.gif *.programs
