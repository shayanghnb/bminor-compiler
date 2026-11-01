#include "Lexer.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <stdexcept>

namespace bminor {

const std::unordered_map<std::string, TokenType> Lexer::s_keywords = {
    {"array",    TokenType::KEYWORD_ARRAY},
    {"auto",     TokenType::KEYWORD_AUTO},
    {"boolean",  TokenType::KEYWORD_BOOLEAN},
    {"char",     TokenType::KEYWORD_CHAR},
    {"else",     TokenType::KEYWORD_ELSE},
    {"false",    TokenType::BOOLEAN_LITERAL},
    {"for",      TokenType::KEYWORD_FOR},
    {"function", TokenType::KEYWORD_FUNCTION},
    {"if",       TokenType::KEYWORD_IF},
    {"integer",  TokenType::KEYWORD_INTEGER},
    {"print",    TokenType::KEYWORD_PRINT},
    {"return",   TokenType::KEYWORD_RETURN},
    {"string",   TokenType::KEYWORD_STRING},
    {"true",     TokenType::BOOLEAN_LITERAL},
    {"void",     TokenType::KEYWORD_VOID},
    {"while",    TokenType::KEYWORD_WHILE}
};

Lexer::Lexer(std::string source, std::string filename, bool preserveComments)
    : m_source(std::move(source)),
      m_filename(std::move(filename)),
      m_cursor(0),
      m_line(1),
      m_column(1),
      m_preserveComments(preserveComments) {}

Lexer Lexer::fromFile(const std::string& filepath, bool preserveComments) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Lexer: Unable to open source file: " + filepath);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return Lexer(ss.str(), filepath, preserveComments);
}

void Lexer::reset() {
    m_cursor = 0;
    m_line = 1;
    m_column = 1;
    m_errors.clear();
}

bool Lexer::isAtEnd() const {
    return m_cursor >= m_source.size();
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return m_source[m_cursor];
}

char Lexer::peekAhead(size_t offset) const {
    if (m_cursor + offset >= m_source.size()) return '\0';
    return m_source[m_cursor + offset];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    char c = m_source[m_cursor++];
    m_column++;
    return c;
}

bool Lexer::match(char expected) {
    if (isAtEnd() || m_source[m_cursor] != expected) {
        return false;
    }
    m_cursor++;
    m_column++;
    return true;
}

void Lexer::reportError(int line, int col, const std::string& message) {
    std::string err = m_filename + ":" + std::to_string(line) + ":" + std::to_string(col) + ": lexical error: " + message;
    m_errors.push_back(err);
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\v' || c == '\f') {
            advance();
        } else if (c == '\r') {
            advance();
            if (peek() == '\n') {
                advance();
            }
            m_line++;
            m_column = 1;
        } else if (c == '\n') {
            advance();
            m_line++;
            m_column = 1;
        } else {
            break;
        }
    }
}

Token Lexer::scanSingleLineComment() {
    int startLine = m_line;
    int startCol = m_column;
    size_t startPos = m_cursor;
    std::string lexeme;
    lexeme += advance(); // '/'
    lexeme += advance(); // '/'

    while (!isAtEnd() && peek() != '\n' && peek() != '\r') {
        lexeme += advance();
    }

    return Token{TokenType::COMMENT_SINGLE, lexeme, SourceLocation{m_filename, startLine, startCol}, startPos, m_cursor};
}

Token Lexer::scanBlockComment() {
    int startLine = m_line;
    int startCol = m_column;
    size_t startPos = m_cursor;
    std::string lexeme;
    lexeme += advance(); // '/'
    lexeme += advance(); // '*'
    bool closed = false;

    while (!isAtEnd()) {
        if (peek() == '*' && peekAhead(1) == '/') {
            lexeme += advance(); // '*'
            lexeme += advance(); // '/'
            closed = true;
            break;
        }
        if (peek() == '\r') {
            lexeme += advance();
            if (peek() == '\n') {
                lexeme += advance();
            }
            m_line++;
            m_column = 1;
        } else if (peek() == '\n') {
            lexeme += advance();
            m_line++;
            m_column = 1;
        } else {
            lexeme += advance();
        }
    }

    if (!closed) {
        reportError(startLine, startCol, "Unterminated block comment");
        return Token{TokenType::INVALID, lexeme, SourceLocation{m_filename, startLine, startCol}, startPos, m_cursor};
    }

    return Token{TokenType::COMMENT_BLOCK, lexeme, SourceLocation{m_filename, startLine, startCol}, startPos, m_cursor};
}

Token Lexer::scanIdentifierOrKeyword() {
    int startLine = m_line;
    int startCol = m_column;
    size_t startPos = m_cursor;

    while (!isAtEnd()) {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            advance();
        } else {
            break;
        }
    }

    std::string lexeme = m_source.substr(startPos, m_cursor - startPos);
    SourceLocation loc{m_filename, startLine, startCol};

    // Keyword lookup (Keyword-over-identifier precedence)
    auto it = s_keywords.find(lexeme);
    if (it != s_keywords.end()) {
        return Token{it->second, lexeme, loc, startPos, m_cursor};
    }

    return Token{TokenType::IDENTIFIER, lexeme, loc, startPos, m_cursor};
}

Token Lexer::scanNumber() {
    int startLine = m_line;
    int startCol = m_column;
    size_t startPos = m_cursor;

    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }

    std::string lexeme = m_source.substr(startPos, m_cursor - startPos);
    return Token{TokenType::INTEGER_LITERAL, lexeme, SourceLocation{m_filename, startLine, startCol}, startPos, m_cursor};
}

bool Lexer::processEscapeSequence(std::string& rawLexeme, std::string& evaluatedValue) {
    if (isAtEnd()) return false;
    char esc = advance();
    rawLexeme += esc;

    switch (esc) {
        case 'n':  evaluatedValue += '\n'; break;
        case 't':  evaluatedValue += '\t'; break;
        case 'r':  evaluatedValue += '\r'; break;
        case '0':  evaluatedValue += '\0'; break;
        case '\\': evaluatedValue += '\\'; break;
        case '\'': evaluatedValue += '\''; break;
        case '\"': evaluatedValue += '\"'; break;
        case 'a':  evaluatedValue += '\a'; break;
        case 'b':  evaluatedValue += '\b'; break;
        case 'f':  evaluatedValue += '\f'; break;
        case 'v':  evaluatedValue += '\v'; break;
        default:
            // Unknown escape sequence
            evaluatedValue += esc;
            return false;
    }
    return true;
}

Token Lexer::scanStringLiteral() {
    int startLine = m_line;
    int startCol = m_column;
    size_t startPos = m_cursor;

    advance(); // Consume opening quote '"'
    std::string rawLexeme = "\"";
    std::string evaluated;
    bool closed = false;

    while (!isAtEnd()) {
        char c = peek();
        if (c == '\"') {
            rawLexeme += advance();
            closed = true;
            break;
        }
        if (c == '\n' || c == '\r') {
            // Unescaped newline in string literal is illegal in B-Minor
            reportError(startLine, startCol, "Unterminated string literal (unescaped newline)");
            return Token{TokenType::INVALID, rawLexeme, SourceLocation{m_filename, startLine, startCol}, startPos, m_cursor};
        }
        if (c == '\\') {
            rawLexeme += advance(); // consume '\\'
            if (!processEscapeSequence(rawLexeme, evaluated)) {
                reportError(m_line, m_column - 1, "Invalid escape sequence in string literal");
            }
        } else {
            rawLexeme += advance();
            evaluated += c;
        }
    }

    if (!closed) {
        reportError(startLine, startCol, "Unterminated string literal at end of file");
        return Token{TokenType::INVALID, rawLexeme, SourceLocation{m_filename, startLine, startCol}, startPos, m_cursor};
    }

    return Token{TokenType::STRING_LITERAL, rawLexeme, SourceLocation{m_filename, startLine, startCol}, startPos, m_cursor};
}

Token Lexer::scanCharacterLiteral() {
    int startLine = m_line;
    int startCol = m_column;
    size_t startPos = m_cursor;

    advance(); // Consume opening quote '\''
    std::string rawLexeme = "\'";
    std::string evaluated;

    if (isAtEnd() || peek() == '\n' || peek() == '\r') {
        reportError(startLine, startCol, "Unterminated character literal");
        return Token{TokenType::INVALID, rawLexeme, SourceLocation{m_filename, startLine, startCol}, startPos, m_cursor};
    }

    if (peek() == '\'') {
        // Empty character constant ''
        rawLexeme += advance();
        reportError(startLine, startCol, "Empty character literal");
        return Token{TokenType::INVALID, rawLexeme, SourceLocation{m_filename, startLine, startCol}, startPos, m_cursor};
    }

    if (peek() == '\\') {
        rawLexeme += advance(); // '\\'
        if (!processEscapeSequence(rawLexeme, evaluated)) {
            reportError(m_line, m_column - 1, "Invalid escape sequence in character literal");
        }
    } else {
        char c = advance();
        rawLexeme += c;
        evaluated += c;
    }

    if (isAtEnd() || peek() != '\'') {
        reportError(startLine, startCol, "Unterminated character literal, expected closing quote");
        return Token{TokenType::INVALID, rawLexeme, SourceLocation{m_filename, startLine, startCol}, startPos, m_cursor};
    }

    rawLexeme += advance(); // Consume closing '\''
    return Token{TokenType::CHARACTER_LITERAL, rawLexeme, SourceLocation{m_filename, startLine, startCol}, startPos, m_cursor};
}

Token Lexer::nextToken() {
    while (true) {
        skipWhitespace();

        if (isAtEnd()) {
            return Token{TokenType::EOF_TOKEN, "", SourceLocation{m_filename, m_line, m_column}, m_cursor, m_cursor};
        }

        // Check for comments
        if (peek() == '/') {
            if (peekAhead(1) == '/') {
                Token commentTok = scanSingleLineComment();
                if (m_preserveComments) {
                    return commentTok;
                }
                continue; // Skip comment and look for next token
            }
            if (peekAhead(1) == '*') {
                Token commentTok = scanBlockComment();
                if (commentTok.type == TokenType::INVALID) {
                    return commentTok; // Propagate error
                }
                if (m_preserveComments) {
                    return commentTok;
                }
                continue; // Skip comment and look for next token
            }
        }

        // Not whitespace or a skipped comment
        break;
    }

    int startLine = m_line;
    int startCol = m_column;
    size_t startPos = m_cursor;
    char c = peek();

    // 1. Identifiers and Keywords
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return scanIdentifierOrKeyword();
    }

    // 2. Numeric Literals
    if (std::isdigit(static_cast<unsigned char>(c))) {
        return scanNumber();
    }

    // 3. String Literals
    if (c == '\"') {
        return scanStringLiteral();
    }

    // 4. Character Literals
    if (c == '\'') {
        return scanCharacterLiteral();
    }

    // 5. Operators and Delimiters (Maximal Munch logic)
    SourceLocation loc{m_filename, startLine, startCol};
    advance(); // Consume the character

    switch (c) {
        // Delimiters
        case '(': return Token{TokenType::LPAREN, "(", loc, startPos, m_cursor};
        case ')': return Token{TokenType::RPAREN, ")", loc, startPos, m_cursor};
        case '[': return Token{TokenType::LBRACKET, "[", loc, startPos, m_cursor};
        case ']': return Token{TokenType::RBRACKET, "]", loc, startPos, m_cursor};
        case '{': return Token{TokenType::LBRACE, "{", loc, startPos, m_cursor};
        case '}': return Token{TokenType::RBRACE, "}", loc, startPos, m_cursor};
        case ';': return Token{TokenType::SEMICOLON, ";", loc, startPos, m_cursor};
        case ':': return Token{TokenType::COLON, ":", loc, startPos, m_cursor};
        case ',': return Token{TokenType::COMMA, ",", loc, startPos, m_cursor};

        // Arithmetic Operators
        case '+':
            if (match('+')) return Token{TokenType::OP_INC, "++", loc, startPos, m_cursor};
            return Token{TokenType::OP_PLUS, "+", loc, startPos, m_cursor};
        case '-':
            if (match('-')) return Token{TokenType::OP_DEC, "--", loc, startPos, m_cursor};
            return Token{TokenType::OP_MINUS, "-", loc, startPos, m_cursor};
        case '*':
            return Token{TokenType::OP_MUL, "*", loc, startPos, m_cursor};
        case '/':
            // Single '/' division operator
            return Token{TokenType::OP_DIV, "/", loc, startPos, m_cursor};
        case '%':
            return Token{TokenType::OP_MOD, "%", loc, startPos, m_cursor};
        case '^':
            return Token{TokenType::OP_EXP, "^", loc, startPos, m_cursor};

        // Relational and Assignment Operators
        case '=':
            if (match('=')) return Token{TokenType::OP_EQ, "==", loc, startPos, m_cursor};
            return Token{TokenType::OP_ASSIGN, "=", loc, startPos, m_cursor};
        case '<':
            if (match('=')) return Token{TokenType::OP_LEQ, "<=", loc, startPos, m_cursor};
            return Token{TokenType::OP_LT, "<", loc, startPos, m_cursor};
        case '>':
            if (match('=')) return Token{TokenType::OP_GEQ, ">=", loc, startPos, m_cursor};
            return Token{TokenType::OP_GT, ">", loc, startPos, m_cursor};
        case '!':
            if (match('=')) return Token{TokenType::OP_NEQ, "!=", loc, startPos, m_cursor};
            return Token{TokenType::OP_LNOT, "!", loc, startPos, m_cursor};

        // Logical Operators
        case '&':
            if (match('&')) return Token{TokenType::OP_LAND, "&&", loc, startPos, m_cursor};
            reportError(startLine, startCol, "Unexpected character '&', did you mean '&&'?");
            return Token{TokenType::INVALID, "&", loc, startPos, m_cursor};
        case '|':
            if (match('|')) return Token{TokenType::OP_LOR, "||", loc, startPos, m_cursor};
            reportError(startLine, startCol, "Unexpected character '|', did you mean '||'?");
            return Token{TokenType::INVALID, "|", loc, startPos, m_cursor};

        default:
            // Unrecognized character
            reportError(startLine, startCol, std::string("Unrecognized character '") + c + "'");
            return Token{TokenType::INVALID, std::string(1, c), loc, startPos, m_cursor};
    }
}

std::vector<Token> Lexer::tokenizeAll() {
    std::vector<Token> tokens;
    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        if (tok.type == TokenType::EOF_TOKEN) {
            break;
        }
    }
    return tokens;
}

} // namespace bminor
