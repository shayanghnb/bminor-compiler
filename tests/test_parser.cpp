#include "../src/Lexer.h"
#include "../src/Parser.h"
#include "../src/AST.h"
#include <iostream>
#include <cassert>

using namespace bminor;

void runParserTest(const std::string& testName, const std::string& source) {
    std::cout << "\n======================================================\n";
    std::cout << " TEST CASE: " << testName << "\n";
    std::cout << "======================================================\n";
    std::cout << "--- Source Code ---\n" << source << "\n";

    Lexer lexer(source, testName, /*preserveComments=*/true);
    Parser parser(lexer);

    auto ast = parser.parseProgram();

    std::cout << "--- Generated Abstract Syntax Tree (AST) ---\n";
    if (ast) {
        ast->print(std::cout, 0);
    } else {
        std::cout << "<Null AST>\n";
    }

    if (parser.hasErrors()) {
        std::cout << "\n[!] Reported Syntax Errors (" << parser.getErrors().size() << "):\n";
        for (const auto& err : parser.getErrors()) {
            std::cout << "    " << err.toString() << "\n";
        }
    } else {
        std::cout << "\n[+] Parsed successfully with 0 errors.\n";
    }
}

int main() {
    std::cout << "======================================================\n";
    std::cout << " B-Minor Compiler Front-End: Parser Test Suite\n";
    std::cout << " Standard C++17 Recursive-Descent with Panic-Mode\n";
    std::cout << "======================================================\n";

    // ------------------------------------------------------------------------
    // Test Case 1: Valid Complex Program (Functions, Arrays, Loops, Precedence)
    // ------------------------------------------------------------------------
    std::string validProgram = 
        "// Recursive factorial\n"
        "factorial: function integer ( n: integer ) = {\n"
        "    if (n <= 1) return 1;\n"
        "    return n * factorial(n - 1);\n"
        "}\n"
        "\n"
        "// Array and global declaration\n"
        "scores: array [5] integer = {10, 20, 30, 40, 50};\n"
        "\n"
        "// Function with for-loop, while-loop, and operator precedence\n"
        "compute: function void ( limit: integer ) = {\n"
        "    i: integer = 0;\n"
        "    total: integer = 2 + 3 * 4 ^ 2;\n"
        "    for (i = 0; i < limit; i++) {\n"
        "        scores[i] = scores[i] + total;\n"
        "    }\n"
        "    while (limit > 0) {\n"
        "        print \"Score: \", limit, scores[limit - 1];\n"
        "        limit--;\n"
        "    }\n"
        "}\n";

    runParserTest("Valid Complex Program (Precedence, Loops, Arrays)", validProgram);

    // ------------------------------------------------------------------------
    // Test Case 2: Expression Precedence Verification
    // Precedence: 2 + 3 * 4 ^ 2 should be: + (left: 2, right: * (left: 3, right: ^ (left: 4, right: 2)))
    // ------------------------------------------------------------------------
    std::string precedenceTest =
        "test_precedence: function void () = {\n"
        "    res: integer = 2 + 3 * 4 ^ 2;\n"
        "    logic: boolean = a <= b && c == d || !flag;\n"
        "}\n";

    runParserTest("Stratified Operator Precedence Hierarchy", precedenceTest);

    // ------------------------------------------------------------------------
    // Test Case 3: Course Specification Panic-Mode Recovery (PDF Page 13)
    //
    // x: integer = ;       /* Error: expected expression after '=' -> skip to ';' */
    // y: integer = 42;     /* Successfully parsed despite error */
    // if (y > 0 {          /* Error: missing ')' before '{' -> recovered! */
    //     return y;
    // }
    // ------------------------------------------------------------------------
    std::string courseRecoveryTest =
        "main: function void () = {\n"
        "    x: integer = ;\n"
        "    y: integer = 42;\n"
        "    if (y > 0 {\n"
        "        return y;\n"
        "    }\n"
        "}\n";

    runParserTest("Course Spec Panic-Mode Recovery (Page 13)", courseRecoveryTest);

    // ------------------------------------------------------------------------
    // Test Case 4: Multiple Consecutive Syntax Errors in Top-level & Statements
    // ------------------------------------------------------------------------
    std::string multiErrorTest =
        "bad_decl: ; // missing type\n"
        "good_var: integer = 100;\n"
        "bad_func: function () = {} // missing return type\n"
        "good_func: function void () = {\n"
        "    return;\n"
        "}\n";

    runParserTest("Top-Level Synchronization Across Multiple Errors", multiErrorTest);

    std::cout << "\n======================================================\n";
    std::cout << "All parser test scenarios executed.\n";
    std::cout << "======================================================\n";

    return 0;
}
