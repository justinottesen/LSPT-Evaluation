EVAL_SRC =../evaluation/src

BIN = bin

TESTS = test_example test_logger

EXECUTABLES = $(addprefix $(BIN) /, $(TESTS))

test_example_SOURCES = test_example.cpp

test_logger_SOURCES = test_logger.cpp $(EVAL_SRC)/Logger.cpp $(EVAL_SRC)/TimeUtil.cpp

CXX = clang++
LD = clang++
CXXFLAGS = -std=c++20
CPPFLAGS = -Wall -Wextra -Werror -g
INCLUDES = -I../sqlite -I../nlohmann_json/single_include -I../evaluation/src -I../googletest/googletest/include -I../googletest/googlemock/include
LDLIBS = -lgtest_main -lgtest
LDFLAGS = -L../googletest/build/lib

COMPILE_CMD = $(CXX) $(CXXFLAGS) $(INCLUDES) $(LDLIBS) $(LDFLAGS) -o $@

.PHONY: all
all : bin/test_example bin/test_logger

.PHONY : run
run : bin/test_example bin/test_logger
	bin/test_example
	bin/test_logger

$(BIN)/test_logger: $(test_logger_SOURCES) 
	mkdir -p $(BIN)
	$(COMPILE_CMD) $(test_logger_SOURCES)

$(BIN)/test_example : $(test_example_SOURCES)
	mkdir -p $(BIN)
	$(COMPILE_CMD) $(test_example_SOURCES)

.PHONY : clean
clean : 
	$(RM) -r $(BIN)

