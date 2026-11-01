#include "../src/Lexer.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

void printTokenHeader() {
    std::cout << std::left
              << std::setw(12) << "Location"
              << std::setw(22) << "Token Type"
              << "Lexeme" << "\n";
    std::cout << std::string(60, '-') << "\n";
}

void printTokenRow(const bminor::Token& tok) {
    std::string locStr = std::to_string(tok.location.line) + ":" + std::to_string(tok.location.column);
    std::cout << std::left
              << std::setw(12) << locStr
              << std::setw(22) << bminor::tokenTypeToString(tok.type)
              << "\"" << tok.lexeme << "\"\n";
}

void runLexerTest(const std::string& testName, const std::string& source, bool preserveComments = false) {
    std::cout << "\n======================================================\n";
    std::cout << " TEST CASE: " << testName << "\n";
    std::cout << " (preserveComments = " << (preserveComments ? "true [Highlighter Mode]" : "false [Parser Mode]") << ")\n";
    std::cout << "======================================================\n";
    std::cout << "--- Source Code ---\n" << source << "\n";
    std::cout << "--- Token Stream ---\n";

    printTokenHeader();

    bminor::Lexer lexer(source, testName, preserveComments);
    std::vector<bminor::Token> tokens = lexer.tokenizeAll();

    for (const auto& tok : tokens) {
        printTokenRow(tok);
    }

    if (lexer.hasErrors()) {
        std::cout << "\n[!] Diagnostic Lexical Errors (" << lexer.getErrors().size() << "):\n";
        for (const auto& err : lexer.getErrors()) {
            std::cout << "    " << err << "\n";
        }
    } else {
        std::cout << "\n[+] Scanning completed successfully with 0 errors.\n";
    }
}

int main(int argc, char* argv[]) {
    std::cout << "B-Minor Compiler Front-End: Lexical Analyzer Test Suite\n";
    std::cout << "Standard C++17 Student Implementation\n";

    // If an external file is provided via CLI
    if (argc > 1) {
        std::string filepath = argv[1];
        bool preserve = (argc > 2 && std::string(argv[2]) == "--preserve-comments");
        try {
            bminor::Lexer fileLexer = bminor::Lexer::fromFile(filepath, preserve);
            std::cout << "\nTokenizing file: " << filepath << " (preserveComments=" << preserve << ")\n";
            printTokenHeader();
            auto tokens = fileLexer.tokenizeAll();
            for (const auto& tok : tokens) {
                printTokenRow(tok);
            }
            if (fileLexer.hasErrors()) {
                std::cout << "\nErrors encountered:\n";
                for (const auto& err : fileLexer.getErrors()) {
                    std::cout << "  " << err << "\n";
                }
            }
        } catch (const std::exception& ex) {
            std::cerr << "Error reading file: " << ex.what() << "\n";
            return 1;
        }
        return 0;
    }

    // Common B-Minor test snippet with comments
    std::string sampleProgram = 
        "// Recursive factorial calculation in B-Minor\n"
        "factorial: function integer ( n: integer ) = {\n"
        "    if (n <= 1) return 1;\n"
        "    return n * factorial(n - 1);\n"
        "}\n"
        "\n"
        "/* Global data structures and array initializers */\n"
        "scores: array [5] integer = {10, 20, 30, 40, 50};\n"
        "greeting: string = \"Hello, B-Minor\\n\\tWelcome!\";\n"
        "letter: char = 'A';\n"
        "isValid: boolean = true;\n";

    // Test Case 1: Parser Mode (Default: preserveComments = false)
    runLexerTest("Parser Mode (Comments Skipped)", sampleProgram, false);

    // Test Case 2: Syntax Highlighter Mode (preserveComments = true)
    runLexerTest("Syntax Highlighter Mode (Comments Preserved)", sampleProgram, true);

    // Test Case 3: Maximal Munch & Operator Disambiguation
    std::string operatorTest =
        "x++ + y-- - z;\n"
        "a <= b && c >= d || e == f != g;\n"
        "2^3 * 4 / 5 % 6;\n"
        "arr[i] = !flag;\n";

    runLexerTest("Maximal Munch & Compound Operators", operatorTest, false);

    // Test Case 4: Escape sequences in strings and characters
    std::string escapeTest =
        "str1: string = \"Tab:\\t Newline:\\n Backslash:\\\\ Quote:\\\"\";\n"
        "ch1: char = '\\n';\n"
        "ch2: char = '\\'';\n"
        "ch3: char = '\\\\';\n";

    runLexerTest("Escape Sequences", escapeTest, false);

    // Test Case 5: Intentional Lexical Errors & Panic Recovery
    std::string errorRecoveryTest =
        "valid_var: integer = 42;\n"
        "bad_chars: @ $ # ~;\n"
        "next_valid: integer = 100;\n"
        "bad_str: string = \"unterminated string literal\n"
        "after_bad_str: integer = 200;\n"
        "bad_comment: /* unterminated block comment at end of input";

    runLexerTest("Error Detection and Resilient Recovery", errorRecoveryTest, false);

    std::cout << "\n======================================================\n";
    std::cout << "All automated lexer test scenarios executed successfully.\n";
    std::cout << "======================================================\n";

    return 0;
}
