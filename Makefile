.SILENT:

CXX = g++
CXXFLAGS = -Wall -std=c++17

all: main

main: main.o
	$(CXX) $(CXXFLAGS) -o main main.o

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

.PHONY: clean
clean:
	@rm -f *.o main
