# B-Minor Compiler Front-End (Phase 1)

A complete, standard **C++17** compiler front-end for the **B-Minor** programming language, developed for the Compiler Design course at Amirkabir University of Technology (Fall 1405 / 2026).

---

## 1. Architectural Overview

The compiler front-end is divided into modular, decoupled subsystems following classical compiler architecture principles:

```
Source Code (.bminor)
       │
       ▼
 ┌───────────┐     Token Stream
 │   Lexer   │ ──────────────────┐
 └───────────┘ (preserveComments) │
       │                          │
       │ Token Stream             ▼
       ▼                   ┌─────────────┐
 ┌───────────┐             │             │
 │  Parser   │ ──────────> │ Highlighter │ ──> ANSI Terminal Output
 └───────────┘   AST &     │             │ ──> Standalone HTML5 Output
       │        Errors     └─────────────┘
       ▼
 ┌───────────┐
 │ AST Tree  │
 └───────────┘
```

### 1.1 Lexical Analyzer (`src/Lexer.h`, `src/Lexer.cpp`, `src/Token.h`)
- **DFA Implementation**: Hand-written deterministic finite automaton scanner operating character-by-character.
- **Maximal Munch**: Greedily matches the longest valid lexeme before emitting a token (e.g., `++` over `+`, `<=` over `<`).
- **Keyword Precedence**: Disambiguates identifiers and keywords using an internal hash lookup table where keywords take precedence over identifiers of the same spelling.
- **Escape Sequence Handling**: Validates and unescapes character and string escape sequences (`\n`, `\t`, `\\`, `\"`, `\'`).
- **Resilient Lexical Error Recovery**: Flags unrecognized characters and unterminated literals as `TokenType::INVALID` with exact line and column coordinates without aborting scanning.
- **Dual-Mode Comment Preservation**: Supports `preserveComments = false` (stripping comments for Parser efficiency) and `preserveComments = true` (emitting `COMMENT_SINGLE` and `COMMENT_BLOCK` for Syntax Highlighting).

### 1.2 Formal Grammar (`grammar.ebnf`)
- **LL(1) Compatibility**: Unambiguous, context-free EBNF grammar with all direct and indirect left-recursion eliminated.
- **11-Level Stratified Precedence**:
  1. Assignment (`=`, right-associative)
  2. Logical OR (`||`, left-associative)
  3. Logical AND (`&&`, left-associative)
  4. Equality (`==`, `!=`, non-associative)
  5. Relational (`<`, `<=`, `>`, `>=`, non-associative)
  6. Additive (`+`, `-`, left-associative)
  7. Multiplicative (`*`, `/`, `%`, left-associative)
  8. Exponentiation (`^`, right-associative)
  9. Prefix Unary (`-`, `!`)
  10. Postfix (`++`, `--`, subscript `[expr]`, call `(args)`)
  11. Primary (literals, identifiers, parenthesized expressions, array initializers)

### 1.3 Abstract Syntax Tree Hierarchy (`src/AST.h`, `src/AST.cpp`)
- **Type Representation (`TypeNode`)**: Strongly typed model covering atomic types (`integer`, `boolean`, `char`, `string`, `void`, `auto`), multi-dimensional arrays (`array [N] type`), and function signatures (`function returnType (paramTypes)`).
- **Inheritance Hierarchy**:
  - `ASTNode`: Base class holding `SourceLocation` and nullable `resolvedType` (reserved for Phase 2 Semantic Analysis).
  - `DeclNode`: `VarDeclNode`, `ParamDeclNode`, `FuncDeclNode` (handling prototypes and full definitions).
  - `StmtNode`: `BlockStmtNode`, `IfStmtNode`, `WhileStmtNode`, `ForStmtNode`, `ReturnStmtNode`, `PrintStmtNode`, `ExprStmtNode`, `DeclStmtNode`.
  - `ExprNode`: `BinaryExprNode`, `UnaryExprNode`, `CallExprNode`, `SubscriptExprNode`, `IdentifierExprNode`, `IntLiteralNode`, `BoolLiteralNode`, `CharLiteralNode`, `StringLiteralNode`, `ArrayInitNode`.
- **Formatted AST Visualization**: Recursive `print(out, indent)` implementation matching the course specification.

### 1.4 Recursive-Descent Parser (`src/Parser.h`, `src/Parser.cpp`)
- **Panic-Mode Error Recovery**: Captures syntax errors, records diagnostic messages with source coordinates, and executes `synchronize()` to safely advance to statement boundaries (`;`, `}`) or declaration keywords (`function`, `if`, `while`, `for`, `return`, `print`).
- **Resilient Recovery Rules**: Implements specific error recovery cases such as missing expressions after `=` (`x: integer = ;`) and missing closing parentheses in control structures (`if (cond { ... }`).

### 1.5 AST-Aware Syntax Highlighter (`src/Highlighter.h`, `src/Highlighter.cpp`)
- **Context-Sensitive Identifier Classification**: Traverses the parsed AST before rendering to extract coordinates of function declarations (`FuncDeclNode`) and call sites (`CallExprNode`). Function identifiers are rendered as `fn` (yellow/gold), while variable identifiers remain `ident` (white).
- **Exact Layout Preservation**: Utilizes byte offset intervals (`startOffset`, `endOffset`) to preserve 100% of the original whitespace, indentation, and newlines.
- **Table 1 Color Palette**:
  - Keywords (`class="kw"`): Bold blue (`\033[1;34m`)
  - Type names (`class="type"`): Teal/cyan (`\033[36m`)
  - Function names (`class="fn"`): Yellow/gold (`\033[33m`)
  - Variable identifiers (`class="ident"`): White/default (`\033[37m`)
  - Numbers & Booleans (`class="lit"`): Orange (`\033[38;5;208m`)
  - Strings & Chars (`class="str"`): Warm green (`\033[32m`)
  - Operators & Delimiters (`class="op"`): Light gray (`\033[38;5;250m`)
  - Comments (`class="cmt"`): Dim gray, italic (`\033[90;3m`)
  - Errors (`class="err"`): Red wavy underline (`\033[4;31m`)
- **Dual Outputs**:
  - ANSI Terminal Output (`highlightANSI`)
  - Self-contained HTML5/CSS Output (`highlightHTML`, zero JavaScript, embedded dark theme)

---

## 2. Building the Project

### Prerequisites
- Modern C++17 compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.16+ (optional, or Make)

### Build with CMake
```bash
cmake -B build -S .
cmake --build build
```

### Build with Makefile
```bash
# Build main bminor binary and test executables
make

# Run the full test suite
make run_tests

# Clean build artifacts
make clean
```

### Direct Compilation with GCC
```bash
g++ -std=c++17 -Wall -Wextra -I src \
    src/Lexer.cpp src/AST.cpp src/Parser.cpp src/Highlighter.cpp src/main.cpp \
    -o bminor
```

---

## 3. Command-Line Interface (CLI)

The compiled executable `bminor` supports the following invocation modes:

```bash
# Default mode: ANSI highlight in terminal, generate <file>.html, output diagnostics
./bminor examples/factorial.bminor

# Token stream view: Prints location, token type, and lexeme table
./bminor --tokens examples/factorial.bminor

# AST hierarchy view: Parses and prints the formatted AST tree
./bminor --ast examples/factorial.bminor

# ANSI terminal view: Outputs colored syntax directly to terminal
./bminor --ansi examples/factorial.bminor

# HTML generation view: Generates standalone HTML (custom output via -o)
./bminor --html examples/factorial.bminor -o output.html

# Help and usage
./bminor --help
```

---

## 4. Test Suite and Verification

The test suite thoroughly validates every compiler stage with unit tests and integration tests:

| Test Binary | Target Component | Coverage |
| :--- | :--- | :--- |
| `test_lexer` | Lexical Analysis | Maximal munch, keyword priority, escape sequences, comment preservation, lexical errors |
| `test_ast` | AST Hierarchy | Type specifications, declaration nodes, statements, 11-level expressions, tree printer |
| `test_parser` | Recursive Descent Parser | Precedence stratification, panic-mode recovery (`x: integer = ;` and `if (y > 0 {`) |
| `test_highlighter` | Syntax Highlighter | Function vs. variable distinction, layout preservation, ANSI codes, HTML rendering |

To run all unit tests:
```bash
./test_lexer
./test_ast
./test_parser
./test_highlighter
```

### Example Programs (`examples/`)
- `examples/factorial.bminor`: Valid program showcasing recursive functions, loops, arrays, strings, booleans, and comments.
- `examples/syntax_recovery.bminor`: Program with intentional syntax errors demonstrating Panic-Mode recovery and `.err` highlighting.
- `examples/lexical_errors.bminor`: Program with invalid characters (`@`, `$`) and unterminated strings demonstrating scanner resilience.
