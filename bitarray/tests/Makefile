GOOGLE_TEST_LIB = gtest
GOOGLE_TEST_INCLUDE = /usr/local/include

C++ = g++

G++_FLAGS = -I $(GOOGLE_TEST_INCLUDE)
C++_FLAGS = -Wall -std=c++17
COV_FLAGS = -lgcov -fprofile-arcs -ftest-coverage

LD_FLAGS = -L /usr/local/lib -l $(GOOGLE_TEST_LIB) -l pthread

SOURCEDIR = ../source/
OBJECTS = tests.o bitarray.o
TARGET = tests

all: $(TARGET)
	@echo
	@echo "Memory check with valgrind: output will be at valgrind_log.va"
	@echo
	@valgrind --log-file=valgrind_log.log ./tests --gtest_filter=bitarrayGoodAlloc.* 1> test.log
	@echo
	@echo "Execution without valgrind"
	@echo
	@echo "Code coverage: "
	@echo
	@./tests 1> test.log
	@gcov bitarray.gcda


$(TARGET): $(OBJECTS)
	$(C++) -o $(TARGET) $(OBJECTS) $(COV_FLAGS) $(LD_FLAGS)

tests.o : tests.cc
	$(C++) -c $(G++_FLAGS) $(C++_FLAGS) $<

bitarray.o : $(SOURCEDIR)bitarray.cc
	$(C++) -c $(C++_FLAGS) $(COV_FLAGS) $<

clean:
	rm -f $(TARGET) *.o *.gcno *.gcda *.gcov *.log

.PHONY: all clean