#
# example Makefile to build an executable named myprog from myprog.cpp
#
PROG=performance
PROG1=non-determinism
PROG2=shared-variable
PROG3=dining

all: $(PROG).cpp
	g++ -std=c++11 -Wall -pthread $(PROG).cpp -o $(PROG)
	g++ -std=c++11 -Wall -pthread $(PROG1).cpp -o $(PROG1)
	g++ -std=c++11 -Wall -pthread $(PROG2).cpp -o $(PROG2)
	g++ -std=c++11 -Wall -pthread $(PROG3).cpp -o $(PROG3)

clean:
	$(RM) $(PROG)
	$(RM) $(PROG1)
	$(RM) $(PROG2)
	$(RM) $(PROG3)
