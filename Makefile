CC=g++
CFLAGS=-c -w -g -m64 -Ofast -std=c++0x -fopenmp
LDFLAGS=-w -g -m64 -Ofast -std=c++0x -fopenmp
CPP_SOURCES=fastXML.cpp
OBJECTS=$(CPP_SOURCES:.cpp=.o)

all: clean $(OBJECTS) train predict

.cpp.o:
	$(CC) $(CFLAGS) $(INC) $< -o $@

train:
	$(CC) $(LDFLAGS) $(INC) -o train *.o train.cpp

predict:
	$(CC) $(LDFLAGS) $(INC) -o predict *.o predict.cpp

clean:
	rm -f *.o
	rm -f train predict
