CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -I src -O2

SRCS = src/Lexer.cpp src/AST.cpp src/Parser.cpp src/Highlighter.cpp
OBJS = $(SRCS:.cpp=.o)

MAIN_SRC = src/main.cpp
MAIN_OBJ = $(MAIN_SRC:.cpp=.o)

BIN = bminor
TEST_TARGETS = test_lexer test_ast test_parser test_highlighter

ifeq ($(OS),Windows_NT)
    EXE_EXT = .exe
    RM = del /Q /F 2>nul || true
else
    EXE_EXT =
    RM = rm -f
endif

.PHONY: all clean tests run_tests

all: $(BIN) tests

$(BIN): $(OBJS) $(MAIN_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(MAIN_OBJ)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

tests: $(TEST_TARGETS)

test_lexer: $(OBJS) tests/test_lexer.o
	$(CXX) $(CXXFLAGS) -o $@ tests/test_lexer.o $(OBJS)

test_ast: $(OBJS) tests/test_ast.o
	$(CXX) $(CXXFLAGS) -o $@ tests/test_ast.o $(OBJS)

test_parser: $(OBJS) tests/test_parser.o
	$(CXX) $(CXXFLAGS) -o $@ tests/test_parser.o $(OBJS)

test_highlighter: $(OBJS) tests/test_highlighter.o
	$(CXX) $(CXXFLAGS) -o $@ tests/test_highlighter.o $(OBJS)

tests/%.o: tests/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run_tests: tests
	./test_lexer$(EXE_EXT)
	./test_ast$(EXE_EXT)
	./test_parser$(EXE_EXT)
	./test_highlighter$(EXE_EXT)

clean:
	$(RM) src/*.o tests/*.o *.exe $(BIN) $(TEST_TARGETS) sample_output.html examples/*.html
