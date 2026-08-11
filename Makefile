.SILENT:

CXX = g++
CXXFLAGS = -Wall -std=c++17 -g

COMPILER = main.cpp
FILE_TO_COMPILE = test.c
COMPILER_EXEC = main	

all: $(COMPILER_EXEC)

run: $(COMPILER_EXEC)
	./$(COMPILER_EXEC) $(FILE_TO_COMPILE)

# Get an executable from the .o file
$(COMPILER_EXEC): main.o
	$(CXX) $(CXXFLAGS) -o $(COMPILER_EXEC) main.o

# Compile the compiler to get a .o file
main.o: $(COMPILER)
	$(CXX) $(CXXFLAGS) -c $(COMPILER)


.PHONY: all run clean
clean:
	rm -f *.o $(COMPILER_EXEC)
