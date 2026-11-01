#ifndef BMINOR_LEXER_H
#define BMINOR_LEXER_H

#include "Token.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace bminor {

class Lexer {
public:
    explicit Lexer(std::string source, std::string filename = "<input>", bool preserveComments = false);

    // Factory method to initialize a Lexer directly from a file path
    static Lexer fromFile(const std::string& filepath, bool preserveComments = false);

    // Comment preservation control (for Syntax Highlighting vs Parser)
    void setPreserveComments(bool preserve) { m_preserveComments = preserve; }
    bool preservesComments() const { return m_preserveComments; }

    // Scans and returns the next token from the input stream
    Token nextToken();

    // Utility: tokenizes the entire stream up to EOF_TOKEN
    std::vector<Token> tokenizeAll();

    // Diagnostics and error reporting
    bool hasErrors() const { return !m_errors.empty(); }
    const std::vector<std::string>& getErrors() const { return m_errors; }

    // Resets the lexer state to the beginning of the source
    void reset();

private:
    std::string m_source;
    std::string m_filename;
    size_t m_cursor{0};
    int m_line{1};
    int m_column{1};
    bool m_preserveComments{false};

    std::vector<std::string> m_errors;
    static const std::unordered_map<std::string, TokenType> s_keywords;

    // Character inspection and consumption helpers
    bool isAtEnd() const;
    char peek() const;
    char peekAhead(size_t offset) const;
    char advance();
    bool match(char expected);

    // Whitespace and comment processing
    void skipWhitespace();
    Token scanSingleLineComment();
    Token scanBlockComment();

    // Sub-scanners for specific token categories
    Token scanIdentifierOrKeyword();
    Token scanNumber();
    Token scanStringLiteral();
    Token scanCharacterLiteral();

    // Helper for escape sequences
    bool processEscapeSequence(std::string& rawLexeme, std::string& evaluatedValue);

    // Error logging helper
    void reportError(int line, int col, const std::string& message);
};

} // namespace bminor

#endif // BMINOR_LEXER_H
