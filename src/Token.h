#ifndef BMINOR_TOKEN_H
#define BMINOR_TOKEN_H

#include <string>
#include <string_view>
#include <iostream>

namespace bminor {

enum class TokenType {
    // Keywords
    KEYWORD_ARRAY,
    KEYWORD_AUTO,
    KEYWORD_BOOLEAN,
    KEYWORD_CHAR,
    KEYWORD_ELSE,
    KEYWORD_FALSE,
    KEYWORD_FOR,
    KEYWORD_FUNCTION,
    KEYWORD_IF,
    KEYWORD_INTEGER,
    KEYWORD_PRINT,
    KEYWORD_RETURN,
    KEYWORD_STRING,
    KEYWORD_TRUE,
    KEYWORD_VOID,
    KEYWORD_WHILE,

    // Identifiers & Literals
    IDENTIFIER,
    INTEGER_LITERAL,
    BOOLEAN_LITERAL,
    CHARACTER_LITERAL,
    STRING_LITERAL,

    // Arithmetic Operators
    OP_PLUS,        // +
    OP_MINUS,       // -
    OP_MUL,         // *
    OP_DIV,         // /
    OP_MOD,         // %
    OP_EXP,         // ^
    OP_INC,         // ++
    OP_DEC,         // --

    // Relational & Equality Operators
    OP_LT,          // <
    OP_LEQ,         // <=
    OP_GT,          // >
    OP_GEQ,         // >=
    OP_EQ,          // ==
    OP_NEQ,         // !=

    // Logical Operators
    OP_LAND,        // &&
    OP_LOR,         // ||
    OP_LNOT,        // !

    // Assignment Operator
    OP_ASSIGN,      // =

    // Delimiters & Punctuation
    LPAREN,         // (
    RPAREN,         // )
    LBRACKET,       // [
    RBRACKET,       // ]
    LBRACE,         // {
    RBRACE,         // }
    COLON,          // :
    SEMICOLON,      // ;
    COMMA,          // ,

    // Comments (Preserved for Syntax Highlighting)
    COMMENT_SINGLE, // // single-line comment
    COMMENT_BLOCK,  // /* multi-line comment */

    // Special Tokens
    EOF_TOKEN,      // End of Input
    INVALID         // Lexical Error
};

struct SourceLocation {
    std::string filename;
    int line{1};
    int column{1};

    std::string toString() const {
        if (filename.empty()) {
            return std::to_string(line) + ":" + std::to_string(column);
        }
        return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
};

struct Token {
    TokenType type{TokenType::INVALID};
    std::string lexeme;
    SourceLocation location;
    size_t startOffset{0};
    size_t endOffset{0};

    std::string toString() const;
};

inline std::string_view tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD_ARRAY:    return "KEYWORD_ARRAY";
        case TokenType::KEYWORD_AUTO:     return "KEYWORD_AUTO";
        case TokenType::KEYWORD_BOOLEAN:  return "KEYWORD_BOOLEAN";
        case TokenType::KEYWORD_CHAR:     return "KEYWORD_CHAR";
        case TokenType::KEYWORD_ELSE:     return "KEYWORD_ELSE";
        case TokenType::KEYWORD_FALSE:    return "KEYWORD_FALSE";
        case TokenType::KEYWORD_FOR:      return "KEYWORD_FOR";
        case TokenType::KEYWORD_FUNCTION: return "KEYWORD_FUNCTION";
        case TokenType::KEYWORD_IF:       return "KEYWORD_IF";
        case TokenType::KEYWORD_INTEGER:  return "KEYWORD_INTEGER";
        case TokenType::KEYWORD_PRINT:    return "KEYWORD_PRINT";
        case TokenType::KEYWORD_RETURN:   return "KEYWORD_RETURN";
        case TokenType::KEYWORD_STRING:   return "KEYWORD_STRING";
        case TokenType::KEYWORD_TRUE:     return "KEYWORD_TRUE";
        case TokenType::KEYWORD_VOID:     return "KEYWORD_VOID";
        case TokenType::KEYWORD_WHILE:    return "KEYWORD_WHILE";

        case TokenType::IDENTIFIER:       return "IDENTIFIER";
        case TokenType::INTEGER_LITERAL:   return "INTEGER_LITERAL";
        case TokenType::BOOLEAN_LITERAL:   return "BOOLEAN_LITERAL";
        case TokenType::CHARACTER_LITERAL: return "CHARACTER_LITERAL";
        case TokenType::STRING_LITERAL:    return "STRING_LITERAL";

        case TokenType::OP_PLUS:          return "OP_PLUS";
        case TokenType::OP_MINUS:         return "OP_MINUS";
        case TokenType::OP_MUL:           return "OP_MUL";
        case TokenType::OP_DIV:           return "OP_DIV";
        case TokenType::OP_MOD:           return "OP_MOD";
        case TokenType::OP_EXP:           return "OP_EXP";
        case TokenType::OP_INC:           return "OP_INC";
        case TokenType::OP_DEC:           return "OP_DEC";

        case TokenType::OP_LT:            return "OP_LT";
        case TokenType::OP_LEQ:           return "OP_LEQ";
        case TokenType::OP_GT:            return "OP_GT";
        case TokenType::OP_GEQ:           return "OP_GEQ";
        case TokenType::OP_EQ:            return "OP_EQ";
        case TokenType::OP_NEQ:           return "OP_NEQ";

        case TokenType::OP_LAND:          return "OP_LAND";
        case TokenType::OP_LOR:           return "OP_LOR";
        case TokenType::OP_LNOT:          return "OP_LNOT";

        case TokenType::OP_ASSIGN:        return "OP_ASSIGN";

        case TokenType::LPAREN:           return "LPAREN";
        case TokenType::RPAREN:           return "RPAREN";
        case TokenType::LBRACKET:         return "LBRACKET";
        case TokenType::RBRACKET:         return "RBRACKET";
        case TokenType::LBRACE:           return "LBRACE";
        case TokenType::RBRACE:           return "RBRACE";
        case TokenType::COLON:            return "COLON";
        case TokenType::SEMICOLON:        return "SEMICOLON";
        case TokenType::COMMA:            return "COMMA";

        case TokenType::COMMENT_SINGLE:   return "COMMENT_SINGLE";
        case TokenType::COMMENT_BLOCK:    return "COMMENT_BLOCK";

        case TokenType::EOF_TOKEN:        return "EOF_TOKEN";
        case TokenType::INVALID:          return "INVALID";
        default:                          return "UNKNOWN";
    }
}

inline std::string Token::toString() const {
    return std::string(tokenTypeToString(type)) + "(\"" + lexeme + "\") at " + location.toString();
}

} // namespace bminor

#endif // BMINOR_TOKEN_H
