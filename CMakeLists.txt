cmake_minimum_required(VERSION 3.16)
project(bminor-compiler VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra)
endif()

include_directories(src)

# Core compiler front-end library
add_library(bminor_core
    src/Lexer.cpp
    src/AST.cpp
    src/Parser.cpp
    src/Highlighter.cpp
)

# Main compiler CLI binary
add_executable(bminor
    src/main.cpp
)
target_link_libraries(bminor PRIVATE bminor_core)

# Test executables
enable_testing()

add_executable(test_lexer tests/test_lexer.cpp)
target_link_libraries(test_lexer PRIVATE bminor_core)
add_test(NAME LexerTest COMMAND test_lexer)

add_executable(test_ast tests/test_ast.cpp)
target_link_libraries(test_ast PRIVATE bminor_core)
add_test(NAME ASTTest COMMAND test_ast)

add_executable(test_parser tests/test_parser.cpp)
target_link_libraries(test_parser PRIVATE bminor_core)
add_test(NAME ParserTest COMMAND test_parser)

add_executable(test_highlighter tests/test_highlighter.cpp)
target_link_libraries(test_highlighter PRIVATE bminor_core)
add_test(NAME HighlighterTest COMMAND test_highlighter)
