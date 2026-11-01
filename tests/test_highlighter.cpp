#include "../src/Highlighter.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <regex>
#include <string>

using namespace bminor;

// Helper to strip ANSI codes and verify exact layout preservation
static std::string stripANSI(const std::string& input) {
    std::regex ansiRegex("\033\\[[0-9;]*m");
    return std::regex_replace(input, ansiRegex, "");
}

// Helper to unescape HTML characters
static std::string unescapeHTML(const std::string& input) {
    std::string s = input;
    auto replaceAll = [&](const std::string& from, const std::string& to) {
        size_t start = 0;
        while ((start = s.find(from, start)) != std::string::npos) {
            s.replace(start, from.length(), to);
            start += to.length();
        }
    };
    replaceAll("&lt;", "<");
    replaceAll("&gt;", ">");
    replaceAll("&quot;", "\"");
    replaceAll("&#39;", "'");
    replaceAll("&amp;", "&");
    return s;
}

// Helper to extract text from <pre>...</pre> in HTML
static std::string extractPreContent(const std::string& html) {
    size_t start = html.find("<pre>");
    if (start == std::string::npos) return "";
    start += 5;
    size_t end = html.find("</pre>", start);
    if (end == std::string::npos) return "";
    std::string pre = html.substr(start, end - start);

    // Strip <span ...> and </span>
    std::regex spanOpen("<span class=\"[^\"]*\">");
    pre = std::regex_replace(pre, spanOpen, "");
    std::regex spanClose("</span>");
    pre = std::regex_replace(pre, spanClose, "");

    return unescapeHTML(pre);
}

int main() {
    std::cout << "======================================================\n";
    std::cout << " B-Minor AST-Aware Syntax Highlighter Test Suite\n";
    std::cout << " Table 1 Context-Sensitive Color Palette & Layout\n";
    std::cout << "======================================================\n\n";

    // A comprehensive B-Minor test program containing:
    // - Function prototype declaration
    // - Recursive function definition
    // - Function calls (including recursive call)
    // - Variable declarations (scalars & array)
    // - Types (integer, boolean, char, string, void)
    // - Literals (int, bool, char, string)
    // - Single-line and multi-line comments
    // - An intentional lexical error token '@'
    const std::string testSource =
        "// Prototype declaration of utility function\n"
        "computeSum: function integer ( a: integer, b: integer );\n"
        "\n"
        "/* Recursive factorial function definition\n"
        "   Demonstrating AST-driven function name coloring */\n"
        "factorial: function integer ( n: integer ) = {\n"
        "    if (n <= 1) {\n"
        "        return 1;\n"
        "    } else {\n"
        "        return n * factorial(n - 1); // Recursive call to factorial\n"
        "    }\n"
        "}\n"
        "\n"
        "// Global variables and literals\n"
        "maxLimit: integer = 100;\n"
        "isActive: boolean = true;\n"
        "flag: boolean = false;\n"
        "prefix: char = '>';\n"
        "banner: string = \"B-Minor Syntax Highlighter v1.0\\n\";\n"
        "scores: array [3] integer = { 10, 20, 30 };\n"
        "\n"
        "/* Main function execution */\n"
        "main: function void () = {\n"
        "    inputVal: integer = 5;\n"
        "    result: integer = factorial(inputVal);\n"
        "    total: integer = computeSum(result, scores[0]);\n"
        "    \n"
        "    print banner;\n"
        "    print prefix, \" Factorial(\", inputVal, \") = \", result, \"\\n\";\n"
        "    print \"Total: \", total, \"\\n\";\n"
        "    \n"
        "    @ // Intentional invalid token to verify red underline error styling\n"
        "}\n";

    // 1. Initialize Highlighter and execute AST-aware analysis
    Highlighter highlighter(testSource, "test_program.bminor");

    // 2. Obtain ANSI Terminal Output
    std::string ansiOutput = highlighter.toANSIString();

    // 3. Obtain HTML Output
    std::string htmlOutput = highlighter.toHTMLString("B-Minor AST-Aware Syntax Highlighter Demo");

    // ------------------------------------------------------------------------
    // Verification 1: Exact Source Layout Preservation
    // ------------------------------------------------------------------------
    std::cout << "[1] Verifying exact source layout preservation...\n";

    // ANSI: Stripping escape sequences must reproduce the exact original source
    std::string strippedANSI = stripANSI(ansiOutput);
    assert(strippedANSI == testSource && "ANSI stripped output must match original source exactly!");
    std::cout << "    [+] ANSI exact layout preservation PASSED.\n";

    // HTML: Extracting text inside <pre> and stripping tags must match original source
    std::string extractedHTML = extractPreContent(htmlOutput);
    assert(extractedHTML == testSource && "HTML stripped <pre> text must match original source exactly!");
    std::cout << "    [+] HTML exact layout preservation PASSED.\n";

    // ------------------------------------------------------------------------
    // Verification 2: Context-Sensitive Function vs Variable Classification
    // ------------------------------------------------------------------------
    std::cout << "\n[2] Verifying AST-driven Function vs Variable classification...\n";

    // Function identifiers should have class="fn"
    assert(htmlOutput.find("<span class=\"fn\">computeSum</span>") != std::string::npos &&
           "Prototype 'computeSum' must be classified as fn!");
    assert(htmlOutput.find("<span class=\"fn\">factorial</span>") != std::string::npos &&
           "Function 'factorial' must be classified as fn!");
    assert(htmlOutput.find("<span class=\"fn\">main</span>") != std::string::npos &&
           "Function 'main' must be classified as fn!");

    // Variable and parameter identifiers should have class="ident", NEVER class="fn"
    assert(htmlOutput.find("<span class=\"ident\">n</span>") != std::string::npos &&
           "Parameter 'n' must be classified as ident!");
    assert(htmlOutput.find("<span class=\"ident\">maxLimit</span>") != std::string::npos &&
           "Variable 'maxLimit' must be classified as ident!");
    assert(htmlOutput.find("<span class=\"ident\">inputVal</span>") != std::string::npos &&
           "Variable 'inputVal' must be classified as ident!");
    assert(htmlOutput.find("<span class=\"ident\">result</span>") != std::string::npos &&
           "Variable 'result' must be classified as ident!");
    assert(htmlOutput.find("<span class=\"ident\">total</span>") != std::string::npos &&
           "Variable 'total' must be classified as ident!");

    assert(htmlOutput.find("<span class=\"fn\">n</span>") == std::string::npos &&
           "Parameter 'n' must NOT be classified as fn!");
    assert(htmlOutput.find("<span class=\"fn\">maxLimit</span>") == std::string::npos &&
           "Variable 'maxLimit' must NOT be classified as fn!");
    assert(htmlOutput.find("<span class=\"fn\">inputVal</span>") == std::string::npos &&
           "Variable 'inputVal' must NOT be classified as fn!");

    std::cout << "    [+] Function names classified as 'fn' (gold/yellow): computeSum, factorial, main.\n";
    std::cout << "    [+] Variables/Parameters strictly classified as 'ident' (white): n, maxLimit, inputVal, result, total.\n";

    // ------------------------------------------------------------------------
    // Verification 3: Table 1 Categories (Keywords, Types, Literals, Comments, Errors)
    // ------------------------------------------------------------------------
    std::cout << "\n[3] Verifying Table 1 token categories...\n";

    // Keywords: class="kw"
    assert(htmlOutput.find("<span class=\"kw\">function</span>") != std::string::npos);
    assert(htmlOutput.find("<span class=\"kw\">if</span>") != std::string::npos);
    assert(htmlOutput.find("<span class=\"kw\">else</span>") != std::string::npos);
    assert(htmlOutput.find("<span class=\"kw\">return</span>") != std::string::npos);
    assert(htmlOutput.find("<span class=\"kw\">print</span>") != std::string::npos);
    assert(htmlOutput.find("<span class=\"kw\">array</span>") != std::string::npos);
    std::cout << "    [+] Keywords classified as 'kw' (bold blue).\n";

    // Types: class="type"
    assert(htmlOutput.find("<span class=\"type\">integer</span>") != std::string::npos);
    assert(htmlOutput.find("<span class=\"type\">boolean</span>") != std::string::npos);
    assert(htmlOutput.find("<span class=\"type\">char</span>") != std::string::npos);
    assert(htmlOutput.find("<span class=\"type\">string</span>") != std::string::npos);
    assert(htmlOutput.find("<span class=\"type\">void</span>") != std::string::npos);
    std::cout << "    [+] Types classified as 'type' (teal/cyan).\n";

    // Literals: class="lit" and class="str"
    assert(htmlOutput.find("<span class=\"lit\">1</span>") != std::string::npos);
    assert(htmlOutput.find("<span class=\"lit\">100</span>") != std::string::npos);
    assert(htmlOutput.find("<span class=\"lit\">true</span>") != std::string::npos);
    assert(htmlOutput.find("<span class=\"lit\">false</span>") != std::string::npos);
    assert(htmlOutput.find("<span class=\"str\">") != std::string::npos);
    std::cout << "    [+] Literals classified as 'lit' (orange) and 'str' (warm green).\n";

    // Comments: class="cmt"
    assert(htmlOutput.find("<span class=\"cmt\">// Prototype declaration of utility function</span>") != std::string::npos);
    std::cout << "    [+] Comments classified as 'cmt' (dim gray italic).\n";

    // Error token: class="err"
    assert(htmlOutput.find("<span class=\"err\">@</span>") != std::string::npos);
    std::cout << "    [+] Invalid token '@' classified as 'err' (red underline).\n";

    // ------------------------------------------------------------------------
    // Verification 4: ANSI Escape Sequences
    // ------------------------------------------------------------------------
    std::cout << "\n[4] Verifying ANSI escape sequences...\n";
    assert(ansiOutput.find("\033[33mfactorial\033[0m") != std::string::npos &&
           "ANSI output must color function 'factorial' in yellow/gold (\\033[33m)!");
    assert(ansiOutput.find("\033[37mn\033[0m") != std::string::npos &&
           "ANSI output must color variable 'n' in white (\\033[37m)!");
    assert(ansiOutput.find("\033[1;34mfunction\033[0m") != std::string::npos &&
           "ANSI output must color keyword 'function' in bold blue (\\033[1;34m)!");
    assert(ansiOutput.find("\033[36minteger\033[0m") != std::string::npos &&
           "ANSI output must color type 'integer' in cyan (\\033[36m)!");
    assert(ansiOutput.find("\033[38;5;208m100\033[0m") != std::string::npos &&
           "ANSI output must color integer literal in orange (\\033[38;5;208m)!");
    assert(ansiOutput.find("\033[90;3m") != std::string::npos &&
           "ANSI output must color comments in dim gray italic (\\033[90;3m)!");
    assert(ansiOutput.find("\033[4;31m@\033[0m") != std::string::npos &&
           "ANSI output must color error token '@' in red underline (\\033[4;31m)!");
    std::cout << "    [+] All ANSI escape sequences verified.\n";

    // ------------------------------------------------------------------------
    // Verification 5: Generate sample_output.html
    // ------------------------------------------------------------------------
    std::cout << "\n[5] Writing standalone HTML to 'sample_output.html'...\n";
    std::ofstream htmlFile("sample_output.html", std::ios::out | std::ios::trunc);
    assert(htmlFile.is_open() && "Failed to create sample_output.html!");
    highlighter.highlightHTML(htmlFile, "B-Minor Syntax Highlighter - Sample Output");
    htmlFile.close();
    std::cout << "    [+] 'sample_output.html' successfully generated.\n";

    // ------------------------------------------------------------------------
    // Render ANSI Output to Terminal
    // ------------------------------------------------------------------------
    std::cout << "\n======================================================\n";
    std::cout << " ANSI TERMINAL HIGHLIGHTING OUTPUT:\n";
    std::cout << "======================================================\n";
    highlighter.highlightANSI(std::cout);
    std::cout << "======================================================\n";

    std::cout << "\n[SUCCESS] All Highlighter tests passed successfully!\n";
    return 0;
}
