#ifndef BMINOR_HIGHLIGHTER_H
#define BMINOR_HIGHLIGHTER_H

#include "Token.h"
#include "AST.h"
#include "Parser.h"
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <memory>

namespace bminor {

enum class HighlightCategory {
    KEYWORD,             // Bold blue (class="kw")
    TYPE_NAME,           // Teal/cyan (class="type")
    FUNCTION_NAME,       // Yellow/gold (class="fn")
    VARIABLE_IDENTIFIER, // White/default (class="ident")
    LITERAL_NUMBER_BOOL, // Orange (class="lit")
    LITERAL_STRING_CHAR, // Warm green (class="str")
    OPERATOR_DELIMITER,  // Light gray (class="op")
    COMMENT,             // Dim gray, italic (class="cmt")
    ERROR                // Red underline (class="err")
};

std::string_view highlightCategoryToClass(HighlightCategory cat);
std::string_view highlightCategoryToANSI(HighlightCategory cat);

class Highlighter {
public:
    explicit Highlighter(std::string source, std::string filename = "<input>");

    // Factory method to initialize from a file path
    static Highlighter fromFile(const std::string& filepath);

    // Parses the source and builds AST context mappings
    void analyze();

    // Directly bind an already-parsed ProgramNode and syntax errors
    void setASTContext(const ProgramNode* ast, const std::vector<SyntaxError>& errors);

    // Render highlighted output to ANSI terminal stream
    void highlightANSI(std::ostream& out) const;

    // Render highlighted output as a standalone HTML5 page with embedded CSS
    void highlightHTML(std::ostream& out, const std::string& title = "B-Minor Source") const;

    // Helper to get string output
    std::string toANSIString() const;
    std::string toHTMLString(const std::string& title = "B-Minor Source") const;

private:
    std::string m_source;
    std::string m_filename;
    std::vector<Token> m_tokens;
    std::set<std::pair<int, int>> m_functionNameLocations; // (line, column)
    std::set<std::pair<int, int>> m_errorLocations;        // (line, column)

    // AST visitor helpers to collect function identifiers
    void collectASTContext(const ASTNode* node);
    void collectFromDecl(const DeclNode* decl);
    void collectFromStmt(const StmtNode* stmt);
    void collectFromExpr(const ExprNode* expr);

    // Classification helper
    HighlightCategory classifyToken(const Token& tok) const;

    // HTML escape utility
    static std::string escapeHTML(const std::string& str);
};

} // namespace bminor

#endif // BMINOR_HIGHLIGHTER_H
