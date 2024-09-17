#
# example Makefile to build an executable named myprog from myprog.cpp
#
PROG=excercise_1
PROG1=excercise_2
PROG4=excercise_4
BENCH=benchmark_example

all: $(PROG).cpp
	g++ -std=c++11 -g -Wall -pthread $(PROG).cpp -o $(PROG)
	g++ -std=c++11 -g -Wall -pthread $(PROG1).cpp -o $(PROG1)
	g++ -std=c++11 -g -Wall -pthread $(PROG4).cpp -o $(PROG4)
	g++ -std=c++11 -g -Wall -pthread $(BENCH).cpp -o $(BENCH)
	
clean:
	$(RM) $(PROG)
	$(RM) $(PROG1)