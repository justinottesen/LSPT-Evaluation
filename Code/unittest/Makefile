EVAL_SRC =../evaluation/src

BIN = bin

test_logger_SOURCES = test_logger.cpp $(EVAL_SRC)/Logger.cpp $(EVAL_SRC)/TimeUtil.cpp

test_tcp_SOURCES = test_tcp.cpp $(EVAL_SRC)/TCPSocket.cpp $(EVAL_SRC)/Logger.cpp $(EVAL_SRC)/TimeUtil.cpp $(EVAL_SRC)/Util.cpp

CXX = clang++
LD = clang++
CXXFLAGS = -std=c++20
CPPFLAGS = -Wall -Wextra -Werror -g -DREUSEADDR # REUSEADDR defined locally for testing, we probably don't want this for release
INCLUDES = -I../sqlite -I../nlohmann_json/single_include -I../evaluation/src -I../googletest/googletest/include -I../googletest/googlemock/include
LDLIBS = -lgtest_main -lgtest
LDFLAGS = -L../googletest/build/lib

COMPILE_CMD = $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) $(LDLIBS) $(LDFLAGS) -o $@

.PHONY: all
all : bin/test_example bin/test_logger

.PHONY : run
run : bin/test_logger bin/test_tcp
	bin/test_logger
	bin/test_tcp

$(BIN)/test_logger: $(test_logger_SOURCES) 
	mkdir -p $(BIN)
	$(COMPILE_CMD) $(test_logger_SOURCES)

$(BIN)/test_tcp : $(test_tcp_SOURCES)
	mkdir -p $(BIN)
	$(COMPILE_CMD) $(test_tcp_SOURCES)

.PHONY : clean
clean : 
	$(RM) -r $(BIN)

.PHONY: format
format:
	clang-format *.cpp -i