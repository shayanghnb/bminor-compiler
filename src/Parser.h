#ifndef BMINOR_PARSER_H
#define BMINOR_PARSER_H

#include "Token.h"
#include "AST.h"
#include "Lexer.h"
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

namespace bminor {

struct SyntaxError {
    SourceLocation location;
    std::string message;

    std::string toString() const {
        return location.toString() + ": syntax error: " + message;
    }
};

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& msg) : std::runtime_error(msg) {}
};

class Parser {
public:
    // Constructors: accept pre-tokenized tokens or a Lexer instance
    explicit Parser(std::vector<Token> tokens);
    explicit Parser(Lexer& lexer);

    // Primary entry point: parses the token stream into a Program AST node
    std::unique_ptr<ProgramNode> parseProgram();

    // Error diagnostics
    bool hasErrors() const { return !m_errors.empty(); }
    const std::vector<SyntaxError>& getErrors() const { return m_errors; }

private:
    std::vector<Token> m_tokens;
    size_t m_current{0};
    std::vector<SyntaxError> m_errors;

    // Token stream navigation & inspection
    bool isAtEnd() const;
    const Token& peek() const;
    const Token& peekAhead(size_t offset) const;
    const Token& previous() const;
    bool check(TokenType type) const;
    const Token& advance();
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    const Token& consume(TokenType type, const std::string& errorMessage);

    // Filter out comments from stream
    void filterComments();

    // Panic-Mode error recording and synchronization
    ParseError error(const Token& token, const std::string& message);
    void recordError(const Token& token, const std::string& message);
    void synchronize();

    // =========================================================================
    // Recursive Descent Grammar Procedures
    // =========================================================================

    // Declarations
    std::unique_ptr<DeclNode> parseDeclaration();
    std::unique_ptr<DeclNode> parseVarDecl(std::string name, SourceLocation loc);
    std::unique_ptr<DeclNode> parseFuncDecl(std::string name, SourceLocation loc);
    std::unique_ptr<ParamDeclNode> parseParameter();
    std::vector<std::unique_ptr<ParamDeclNode>> parseParameterList();

    // Types
    std::shared_ptr<TypeNode> parseType();
    std::shared_ptr<TypeNode> parsePrimitiveType();
    std::shared_ptr<TypeNode> parseArrayType();
    std::shared_ptr<TypeNode> parseFunctionType();

    // Statements
    std::unique_ptr<StmtNode> parseStatement();
    std::unique_ptr<BlockStmtNode> parseBlockStmt();
    std::unique_ptr<StmtNode> parseIfStmt();
    std::unique_ptr<StmtNode> parseWhileStmt();
    std::unique_ptr<StmtNode> parseForStmt();
    std::unique_ptr<StmtNode> parseReturnStmt();
    std::unique_ptr<StmtNode> parsePrintStmt();
    std::unique_ptr<StmtNode> parseExprOrDeclStmt();

    // Expressions & Operator Precedence Hierarchy
    std::unique_ptr<ExprNode> parseExpression();
    std::unique_ptr<ExprNode> parseAssignment();
    std::unique_ptr<ExprNode> parseLogicalOr();
    std::unique_ptr<ExprNode> parseLogicalAnd();
    std::unique_ptr<ExprNode> parseEquality();
    std::unique_ptr<ExprNode> parseRelational();
    std::unique_ptr<ExprNode> parseAdditive();
    std::unique_ptr<ExprNode> parseMultiplicative();
    std::unique_ptr<ExprNode> parseExponentiation();
    std::unique_ptr<ExprNode> parseUnary();
    std::unique_ptr<ExprNode> parsePostfix();
    std::unique_ptr<ExprNode> parsePrimary();

    // Expression lists and initializers
    std::vector<std::unique_ptr<ExprNode>> parseArgumentList();
    std::unique_ptr<ArrayInitNode> parseArrayInitializer();
};

} // namespace bminor

#endif // BMINOR_PARSER_H
