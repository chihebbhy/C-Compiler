.SILENT:

CXX = g++
CXXFLAGS = -Wall -std=c++17 -g

COMPILER = main.cpp
FILE_TO_COMPILE = test.c
COMPILER_EXEC = main	

TEST_DIR = tests
TEST_EXEC = run_tests
TEST_SRCS = $(TEST_DIR)/test_main.cpp $(TEST_DIR)/test_lexer.cpp $(TEST_DIR)/test_parser.cpp
TEST_DEPS = lexer.hpp parser.hpp $(TEST_DIR)/test_framework.hpp

all: $(COMPILER_EXEC)

run: $(COMPILER_EXEC)
	./$(COMPILER_EXEC) $(FILE_TO_COMPILE)

# Get an executable from the .o file
$(COMPILER_EXEC): main.o
	$(CXX) $(CXXFLAGS) -o $(COMPILER_EXEC) main.o

# Compile the compiler to get a .o file
main.o: $(COMPILER)
	$(CXX) $(CXXFLAGS) -c $(COMPILER)

# Build and run the lexer/parser unit tests. Fails (non-zero exit) if any
# test case fails, so it can be used as a CI-style confirmation step.
test: $(TEST_EXEC)
	./$(TEST_EXEC)

$(TEST_EXEC): $(TEST_SRCS) $(TEST_DEPS)
	$(CXX) $(CXXFLAGS) -o $(TEST_EXEC) $(TEST_SRCS)

.PHONY: all run clean test

clean:
	rm -f *.o $(COMPILER_EXEC) $(TEST_EXEC)