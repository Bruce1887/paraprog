PROG=excercise_1
PROG1=excercise_2
PROG4=excercise_4
BENCH=benchmark_example
CFLAGS = -g -Wall -pthread -O3

SANITIZER = false
ifeq ($(SANITIZER),true)
    CFLAGS += -fsanitize=thread
#-fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize=thread
endif

all: $(PROG).cpp
	g++ -std=c++11 $(CFLAGS) $(PROG).cpp -o $(PROG)
	g++ -std=c++11 $(CFLAGS) $(PROG1).cpp -o $(PROG1)
	g++ -std=c++11 $(CFLAGS) $(PROG4).cpp -o $(PROG4)
	g++ -std=c++11 $(CFLAGS) $(BENCH).cpp -o $(BENCH)
	
clean:
	$(RM) $(PROG)
	$(RM) $(PROG1)
	$(RM) $(PROG4)
	$(RM) $(BENCH)
	
	