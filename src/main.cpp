#include "Token.h"
#include "Lexer.h"
#include "AST.h"
#include "Parser.h"
#include "Highlighter.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>

using namespace bminor;

static void printUsage(const char* progName) {
    std::cout << "B-Minor Compiler Front-End (Phase 1 CLI)\n"
              << "Usage: " << progName << " [options] <source.bminor>\n\n"
              << "Options:\n"
              << "  --tokens <file>              Scan source, print token stream table and lexical errors\n"
              << "  --ast <file>                 Parse source, print AST hierarchy and syntax errors\n"
              << "  --ansi <file>                Render AST-aware ANSI syntax-highlighted code to terminal\n"
              << "  --html <file> [-o <out.html>]  Render standalone HTML/CSS highlighted file\n"
              << "  -h, --help                   Display this usage message\n\n"
              << "Default behavior (when given just <file>):\n"
              << "  Prints ANSI highlighting to terminal, generates <file>.html, and outputs diagnostics.\n";
}

static bool fileExists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

static std::string readFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[!] Error: Unable to open source file: " << filepath << "\n";
        return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static std::string getHtmlFilename(const std::string& inputPath) {
    size_t lastDot = inputPath.find_last_of('.');
    size_t lastSlash = inputPath.find_last_of("/\\");
    if (lastDot != std::string::npos && (lastSlash == std::string::npos || lastDot > lastSlash)) {
        return inputPath.substr(0, lastDot) + ".html";
    }
    return inputPath + ".html";
}

static int runTokens(const std::string& filepath) {
    if (!fileExists(filepath)) {
        std::cerr << "[!] Error: File not found: " << filepath << "\n";
        return 1;
    }
    std::string source = readFile(filepath);

    Lexer lexer(source, filepath, /*preserveComments=*/true);
    std::vector<Token> tokens = lexer.tokenizeAll();

    std::cout << std::left
              << std::setw(16) << "Location"
              << std::setw(24) << "Token Type"
              << "Lexeme\n";
    std::cout << std::string(64, '-') << "\n";

    for (const auto& tok : tokens) {
        if (tok.type == TokenType::EOF_TOKEN) continue;
        std::string loc = std::to_string(tok.location.line) + ":" + std::to_string(tok.location.column);
        std::cout << std::left
                  << std::setw(16) << loc
                  << std::setw(24) << tokenTypeToString(tok.type)
                  << "\"" << tok.lexeme << "\"\n";
    }

    if (lexer.hasErrors()) {
        std::cerr << "\n[!] Diagnostic Lexical Errors (" << lexer.getErrors().size() << "):\n";
        for (const auto& err : lexer.getErrors()) {
            std::cerr << "    " << err << "\n";
        }
        return 1;
    }

    std::cout << "\n[+] Lexical analysis completed with 0 errors.\n";
    return 0;
}

static int runAST(const std::string& filepath) {
    if (!fileExists(filepath)) {
        std::cerr << "[!] Error: File not found: " << filepath << "\n";
        return 1;
    }
    std::string source = readFile(filepath);

    Lexer lexer(source, filepath, /*preserveComments=*/false);
    Parser parser(lexer);
    auto ast = parser.parseProgram();

    std::cout << "--- Abstract Syntax Tree (AST) Hierarchy ---\n";
    if (ast) {
        ast->print(std::cout, 0);
    } else {
        std::cout << "<Null AST>\n";
    }

    bool hasErrors = false;
    if (lexer.hasErrors()) {
        hasErrors = true;
        std::cerr << "\n[!] Diagnostic Lexical Errors (" << lexer.getErrors().size() << "):\n";
        for (const auto& err : lexer.getErrors()) {
            std::cerr << "    " << err << "\n";
        }
    }

    if (parser.hasErrors()) {
        hasErrors = true;
        std::cerr << "\n[!] Reported Syntax Errors (" << parser.getErrors().size() << "):\n";
        for (const auto& err : parser.getErrors()) {
            std::cerr << "    " << err.toString() << "\n";
        }
    }

    if (!hasErrors) {
        std::cout << "\n[+] Parsed successfully with 0 errors.\n";
        return 0;
    }
    return 1;
}

static int runANSI(const std::string& filepath) {
    if (!fileExists(filepath)) {
        std::cerr << "[!] Error: File not found: " << filepath << "\n";
        return 1;
    }
    std::string source = readFile(filepath);

    Highlighter highlighter(source, filepath);
    highlighter.highlightANSI(std::cout);
    return 0;
}

static int runHTML(const std::string& filepath, const std::string& outputPath) {
    if (!fileExists(filepath)) {
        std::cerr << "[!] Error: File not found: " << filepath << "\n";
        return 1;
    }
    std::string source = readFile(filepath);

    std::string outPath = outputPath.empty() ? getHtmlFilename(filepath) : outputPath;
    std::ofstream outFile(outPath, std::ios::out | std::ios::trunc);
    if (!outFile.is_open()) {
        std::cerr << "[!] Error: Unable to open output file for writing: " << outPath << "\n";
        return 1;
    }

    Highlighter highlighter(source, filepath);
    highlighter.highlightHTML(outFile, "B-Minor Source: " + filepath);
    outFile.close();

    std::cout << "[+] Standalone HTML/CSS highlighting generated: " << outPath << "\n";
    return 0;
}

static int runDefault(const std::string& filepath) {
    if (!fileExists(filepath)) {
        std::cerr << "[!] Error: File not found: " << filepath << "\n";
        return 1;
    }
    std::string source = readFile(filepath);

    Highlighter highlighter(source, filepath);

    // 1. Prints ANSI highlighted code directly to the terminal
    std::cout << "======================================================\n";
    std::cout << " B-Minor ANSI Syntax Highlighting: " << filepath << "\n";
    std::cout << "======================================================\n";
    highlighter.highlightANSI(std::cout);
    std::cout << "\n======================================================\n";

    // 2. Generates <file>.html
    std::string outPath = getHtmlFilename(filepath);
    std::ofstream outFile(outPath, std::ios::out | std::ios::trunc);
    if (outFile.is_open()) {
        highlighter.highlightHTML(outFile, "B-Minor Source: " + filepath);
        outFile.close();
        std::cout << "[+] Standalone HTML file created: " << outPath << "\n";
    } else {
        std::cerr << "[!] Warning: Unable to write HTML file to: " << outPath << "\n";
    }

    // 3. Prints diagnostic summary of lexical and syntax errors
    Lexer lexer(source, filepath, /*preserveComments=*/false);
    Parser parser(lexer);
    auto ast = parser.parseProgram();

    size_t lexErrors = lexer.getErrors().size();
    size_t synErrors = parser.getErrors().size();

    if (lexErrors > 0 || synErrors > 0) {
        std::cerr << "\n[!] Diagnostic Error Summary (" << (lexErrors + synErrors) << " errors found):\n";
        if (lexErrors > 0) {
            std::cerr << "    Lexical Errors (" << lexErrors << "):\n";
            for (const auto& err : lexer.getErrors()) {
                std::cerr << "      - " << err << "\n";
            }
        }
        if (synErrors > 0) {
            std::cerr << "    Syntax Errors (" << synErrors << "):\n";
            for (const auto& err : parser.getErrors()) {
                std::cerr << "      - " << err.toString() << "\n";
            }
        }
        return 1;
    }

    std::cout << "[+] Source verified successfully: 0 lexical and syntax errors.\n";
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "-h" || mode == "--help") {
        printUsage(argv[0]);
        return 0;
    }

    if (mode == "--tokens") {
        if (argc < 3) {
            std::cerr << "[!] Error: Missing input file for --tokens\n";
            return 1;
        }
        return runTokens(argv[2]);
    }

    if (mode == "--ast") {
        if (argc < 3) {
            std::cerr << "[!] Error: Missing input file for --ast\n";
            return 1;
        }
        return runAST(argv[2]);
    }

    if (mode == "--ansi") {
        if (argc < 3) {
            std::cerr << "[!] Error: Missing input file for --ansi\n";
            return 1;
        }
        return runANSI(argv[2]);
    }

    if (mode == "--html") {
        if (argc < 3) {
            std::cerr << "[!] Error: Missing input file for --html\n";
            return 1;
        }
        std::string inputFile = argv[2];
        std::string outputFile = "";
        for (int i = 3; i < argc; ++i) {
            if (std::string(argv[i]) == "-o" && i + 1 < argc) {
                outputFile = argv[i + 1];
                i++;
            }
        }
        return runHTML(inputFile, outputFile);
    }

    // Default mode: argument is the input file
    return runDefault(argv[1]);
}
