#
# example Makefile to build an executable named myprog from myprog.cpp
#
PROG=excercise_1
PROG1=excercise_2

all: $(PROG).cpp
	g++ -std=c++11 -g -Wall -pthread $(PROG).cpp -o $(PROG)
	g++ -std=c++11 -g -Wall -pthread $(PROG1).cpp -o $(PROG1)

clean:
	$(RM) $(PROG)
	$(RM) $(PROG1)