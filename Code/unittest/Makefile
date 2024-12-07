EVAL_SRC =../evaluation/src

BIN = bin

common_SOURCES = $(EVAL_SRC)/Logger.cpp $(EVAL_SRC)/TimeUtil.cpp $(EVAL_SRC)/Util.cpp

test_logger_SOURCES = test_logger.cpp $(common_SOURCES)
test_tcp_SOURCES = test_tcp.cpp $(EVAL_SRC)/TCPSocket.cpp $(common_SOURCES)
test_http_SOURCES = test_http.cpp $(EVAL_SRC)/HTTPServer.cpp $(EVAL_SRC)/TCPSocket.cpp $(common_SOURCES)

CXX = clang++
LD = clang++
CXXFLAGS = -std=c++20
CPPFLAGS = -Wall -Wextra -Werror -g -DREUSEADDR -DUNITTEST
INCLUDES = -I../sqlite -I../nlohmann_json/single_include -I../evaluation/src -I../googletest/googletest/include -I../googletest/googlemock/include
LDLIBS = -lgtest_main -lgtest
LDFLAGS = -L../googletest/build/lib

COMPILE_CMD = $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) $(LDLIBS) $(LDFLAGS) -o $@

.PHONY: all
all : bin/test_logger bin/test_tcp bin/test_http

.PHONY : run
run : bin/test_logger bin/test_tcp bin/test_http
	bin/test_logger
	bin/test_tcp
	bin/test_http

$(BIN)/test_logger: $(test_logger_SOURCES) 
	mkdir -p $(BIN)
	$(COMPILE_CMD) $(test_logger_SOURCES)

$(BIN)/test_tcp : $(test_tcp_SOURCES)
	mkdir -p $(BIN)
	$(COMPILE_CMD) $(test_tcp_SOURCES)

$(BIN)/test_http : $(test_http_SOURCES)
	mkdir -p $(BIN)
	$(COMPILE_CMD) $(test_http_SOURCES)

.PHONY : clean
clean : 
	$(RM) -r $(BIN)

.PHONY: format
format:
	clang-format *.cpp -i