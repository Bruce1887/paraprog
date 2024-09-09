#
# example Makefile to build an executable named myprog from myprog.cpp
#
PROG=excercise_1
# PROG1=non-determinism
# PROG2=shared-variable
# PROG3=dining

all: $(PROG).cpp
	g++ -std=c++11 -Wall -pthread $(PROG).cpp -o $(PROG)

clean:
	$(RM) $(PROG)