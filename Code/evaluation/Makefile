# executable name
EXE = evaluation

# C compiler
CC = clang
# C++ compiler
CXX = clang++
# linker
LD = clang++

# C flags
CFLAGS =
# C++ flags
CXXFLAGS = -std=c++20
# C/C++ flags
CPPFLAGS = -Wall -Wextra -Werror -g -DREUSEADDR # REUSEADDR defined locally for testing, we probably don't want this for release
# Includes
INCOUDES = -I../sqlite -I../nlohmann_json/single_include
# dependency-generation flags
DEPFLAGS = -MMD -MP
# linker flags
LDFLAGS = 
# library flags
LDLIBS = 

# build directories
BIN = bin
OBJ = obj
SRC = src

HEADERS := $(wildcard $(SRC)/*.h $(SRC)/*.hpp)

SOURCES := $(wildcard $(SRC)/*.c $(SRC)/*.cc $(SRC)/*.cpp $(SRC)/*.cxx ../sqlite/*.c)

OBJECTS := \
	$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(wildcard $(SRC)/*.c)) \
	$(patsubst $(SRC)/%.cpp, $(OBJ)/%.o, $(wildcard $(SRC)/*.cpp)) \
	../sqlite/sqlite3.o

# include compiler-generated dependency rules
DEPENDS := $(OBJECTS:.o=.d)

# compile C source
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -c -o $@
# compile C++ source
COMPILE.cxx = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) -c -o $@
# link objects
LINK.o = $(LD) $(LDFLAGS) $(LDLIBS) $(OBJECTS) -o $@

.DEFAULT_GOAL = all

.PHONY: all
all: $(BIN)/$(EXE)

$(BIN)/$(EXE): $(SRC) $(OBJ) $(BIN) $(OBJECTS)
	$(LINK.o)

$(SRC):
	mkdir -p $(SRC)

$(OBJ):
	mkdir -p $(OBJ)

$(BIN):
	mkdir -p $(BIN)

$(OBJ)/%.o: $(SRC)/%.c
	$(COMPILE.c) $<

$(OBJ)/%.o: $(SRC)/%.cpp
	$(COMPILE.cxx) $<

# force rebuild
.PHONY: remake
remake:	clean $(BIN)/$(EXE)

# execute the program
.PHONY: run
run: $(BIN)/$(EXE)
	./$(BIN)/$(EXE)

# remove previous build and objects
.PHONY: clean
clean:
	$(RM) $(OBJECTS)
	$(RM) $(DEPENDS)
	$(RM) $(BIN)/$(EXE)

# remove everything except source
.PHONY: reset
reset:
	$(RM) -r $(OBJ)
	$(RM) -r $(BIN)
	$(RM) -r data
	$(RM) -r log

.PHONY: format
format:
	clang-format src/* -i

.PHONY: tidy
tidy:
	clang-tidy src/*.cpp

.PHONY: tidy-fix
tidy-fix:
	clang-tidy src/*.cpp --fix

-include $(DEPENDS)
