TESTS = test_example

CXX = clang++

CXXFLAGS = -std=c++20 -Wall -Wextra -Werror -g
LDFLAGS = -L../googletest/build/lib
LDLIBS = -lgtest_main -lgtest

all: $(TESTS)

run: bin/$(TESTS)
	./bin/$(TESTS)

clean:
	rm -rf bin obj

bin:
	mkdir -p bin

obj:
	mkdir -p obj

test_example.o: test_example.cpp obj
	$(CXX) $(CXXFLAGS) -I../googletest/googletest/include -c test_example.cpp -o obj/$@

test_example: test_example.o bin
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS) obj/$< -o bin/$@