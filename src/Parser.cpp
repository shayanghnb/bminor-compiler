#include "Parser.h"
#include <algorithm>

namespace bminor {

Parser::Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens)), m_current(0) {
    filterComments();
}

Parser::Parser(Lexer& lexer)
    : m_current(0) {
    m_tokens = lexer.tokenizeAll();
    filterComments();
}

void Parser::filterComments() {
    m_tokens.erase(
        std::remove_if(m_tokens.begin(), m_tokens.end(), [](const Token& tok) {
            return tok.type == TokenType::COMMENT_SINGLE || tok.type == TokenType::COMMENT_BLOCK;
        }),
        m_tokens.end()
    );
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOKEN;
}

const Token& Parser::peek() const {
    if (m_current < m_tokens.size()) {
        return m_tokens[m_current];
    }
    static const Token eofToken{TokenType::EOF_TOKEN, "", SourceLocation{}};
    return eofToken;
}

const Token& Parser::peekAhead(size_t offset) const {
    if (m_current + offset < m_tokens.size()) {
        return m_tokens[m_current + offset];
    }
    static const Token eofToken{TokenType::EOF_TOKEN, "", SourceLocation{}};
    return eofToken;
}

const Token& Parser::previous() const {
    if (m_current > 0 && m_current - 1 < m_tokens.size()) {
        return m_tokens[m_current - 1];
    }
    return peek();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return type == TokenType::EOF_TOKEN;
    return peek().type == type;
}

const Token& Parser::advance() {
    if (!isAtEnd()) {
        m_current++;
    }
    return previous();
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType t : types) {
        if (check(t)) {
            advance();
            return true;
        }
    }
    return false;
}

const Token& Parser::consume(TokenType type, const std::string& errorMessage) {
    if (check(type)) return advance();
    throw error(peek(), errorMessage);
}

ParseError Parser::error(const Token& token, const std::string& message) {
    recordError(token, message);
    return ParseError(message);
}

void Parser::recordError(const Token& token, const std::string& message) {
    m_errors.push_back(SyntaxError{token.location, message});
}

void Parser::synchronize() {
    advance();

    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;

        switch (peek().type) {
            case TokenType::KEYWORD_FUNCTION:
            case TokenType::KEYWORD_IF:
            case TokenType::KEYWORD_WHILE:
            case TokenType::KEYWORD_FOR:
            case TokenType::KEYWORD_RETURN:
            case TokenType::KEYWORD_PRINT:
            case TokenType::RBRACE:
                return;
            default:
                break;
        }
        advance();
    }
}

// =============================================================================
// Program & Declarations
// =============================================================================

std::unique_ptr<ProgramNode> Parser::parseProgram() {
    SourceLocation loc = peek().location;
    auto program = std::make_unique<ProgramNode>(loc);

    while (!isAtEnd()) {
        try {
            auto decl = parseDeclaration();
            if (decl) {
                program->declarations.push_back(std::move(decl));
            }
        } catch (const ParseError&) {
            synchronize();
        }
    }

    return program;
}

std::unique_ptr<DeclNode> Parser::parseDeclaration() {
    if (!check(TokenType::IDENTIFIER)) {
        throw error(peek(), "Expected identifier at start of declaration");
    }

    Token nameTok = advance();
    consume(TokenType::COLON, "Expected ':' after identifier in declaration");

    if (check(TokenType::KEYWORD_FUNCTION)) {
        return parseFuncDecl(nameTok.lexeme, nameTok.location);
    } else {
        return parseVarDecl(nameTok.lexeme, nameTok.location);
    }
}

std::unique_ptr<DeclNode> Parser::parseVarDecl(std::string name, SourceLocation loc) {
    auto type = parseType();
    std::unique_ptr<ExprNode> init = nullptr;

    if (match(TokenType::OP_ASSIGN)) {
        // Spec Page 13 error recovery test: missing expression after '=' (e.g. "x: integer = ;")
        if (check(TokenType::SEMICOLON)) {
            recordError(peek(), "Expected expression after '='");
        } else if (check(TokenType::LBRACE)) {
            init = parseArrayInitializer();
        } else {
            init = parseExpression();
        }
    }

    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    return std::make_unique<VarDeclNode>(std::move(name), std::move(type), std::move(init), loc);
}

std::unique_ptr<DeclNode> Parser::parseFuncDecl(std::string name, SourceLocation loc) {
    consume(TokenType::KEYWORD_FUNCTION, "Expected 'function' keyword");
    auto retType = parseType();

    consume(TokenType::LPAREN, "Expected '(' after function return type");
    std::vector<std::unique_ptr<ParamDeclNode>> params;
    std::vector<std::shared_ptr<TypeNode>> paramTypes;

    if (!check(TokenType::RPAREN)) {
        params = parseParameterList();
        for (const auto& p : params) {
            paramTypes.push_back(p->declaredType);
        }
    }
    consume(TokenType::RPAREN, "Expected ')' after parameter list");

    auto fnType = TypeNode::makeFunction(std::move(retType), std::move(paramTypes));
    std::unique_ptr<BlockStmtNode> body = nullptr;

    if (match(TokenType::OP_ASSIGN)) {
        // Function Definition: = { ... }
        if (!check(TokenType::LBRACE)) {
            throw error(peek(), "Expected '{' for function body definition");
        }
        body = parseBlockStmt();
    } else if (match(TokenType::SEMICOLON)) {
        // Function Prototype: ends with ';'
        body = nullptr;
    } else {
        throw error(peek(), "Expected '=' with block or ';' for function prototype");
    }

    return std::make_unique<FuncDeclNode>(
        std::move(name),
        std::move(fnType),
        std::move(params),
        std::move(body),
        loc
    );
}

std::vector<std::unique_ptr<ParamDeclNode>> Parser::parseParameterList() {
    std::vector<std::unique_ptr<ParamDeclNode>> params;
    params.push_back(parseParameter());

    while (match(TokenType::COMMA)) {
        params.push_back(parseParameter());
    }

    return params;
}

std::unique_ptr<ParamDeclNode> Parser::parseParameter() {
    const Token& idTok = consume(TokenType::IDENTIFIER, "Expected parameter name");
    consume(TokenType::COLON, "Expected ':' after parameter name");
    auto type = parseType();
    return std::make_unique<ParamDeclNode>(idTok.lexeme, std::move(type), idTok.location);
}

// =============================================================================
// Type System
// =============================================================================

std::shared_ptr<TypeNode> Parser::parseType() {
    if (check(TokenType::KEYWORD_ARRAY)) {
        return parseArrayType();
    }
    if (check(TokenType::KEYWORD_FUNCTION)) {
        return parseFunctionType();
    }
    return parsePrimitiveType();
}

std::shared_ptr<TypeNode> Parser::parsePrimitiveType() {
    if (match(TokenType::KEYWORD_INTEGER)) return TypeNode::makeInteger();
    if (match(TokenType::KEYWORD_BOOLEAN)) return TypeNode::makeBoolean();
    if (match(TokenType::KEYWORD_CHAR))    return TypeNode::makeChar();
    if (match(TokenType::KEYWORD_STRING))  return TypeNode::makeString();
    if (match(TokenType::KEYWORD_VOID))    return TypeNode::makeVoid();
    if (match(TokenType::KEYWORD_AUTO))    return TypeNode::makeAuto();

    throw error(peek(), "Expected type specifier (integer, boolean, char, string, void, array, function)");
}

std::shared_ptr<TypeNode> Parser::parseArrayType() {
    consume(TokenType::KEYWORD_ARRAY, "Expected 'array'");
    consume(TokenType::LBRACKET, "Expected '[' after 'array'");

    std::unique_ptr<ExprNode> sizeExpr = nullptr;
    if (!check(TokenType::RBRACKET)) {
        sizeExpr = parseExpression();
    }
    consume(TokenType::RBRACKET, "Expected ']' in array type");

    auto elemType = parseType();
    return TypeNode::makeArray(std::move(elemType), std::move(sizeExpr));
}

std::shared_ptr<TypeNode> Parser::parseFunctionType() {
    consume(TokenType::KEYWORD_FUNCTION, "Expected 'function'");
    auto retType = parseType();

    consume(TokenType::LPAREN, "Expected '(' in function type");
    std::vector<std::shared_ptr<TypeNode>> paramTypes;

    if (!check(TokenType::RPAREN)) {
        // Can be parameter declarations or just type list
        while (true) {
            if (check(TokenType::IDENTIFIER) && peekAhead(1).type == TokenType::COLON) {
                advance(); // IDENT
                advance(); // ':'
            }
            paramTypes.push_back(parseType());
            if (!match(TokenType::COMMA)) break;
        }
    }
    consume(TokenType::RPAREN, "Expected ')' after function type parameters");

    return TypeNode::makeFunction(std::move(retType), std::move(paramTypes));
}

// =============================================================================
// Statements
// =============================================================================

std::unique_ptr<StmtNode> Parser::parseStatement() {
    // 1. Block Statement
    if (check(TokenType::LBRACE)) {
        return parseBlockStmt();
    }
    // 2. Control Flow & Builtin Statements
    if (check(TokenType::KEYWORD_IF)) {
        return parseIfStmt();
    }
    if (check(TokenType::KEYWORD_WHILE)) {
        return parseWhileStmt();
    }
    if (check(TokenType::KEYWORD_FOR)) {
        return parseForStmt();
    }
    if (check(TokenType::KEYWORD_RETURN)) {
        return parseReturnStmt();
    }
    if (check(TokenType::KEYWORD_PRINT)) {
        return parsePrintStmt();
    }

    // 3. Local Declaration or Expression Statement
    return parseExprOrDeclStmt();
}

std::unique_ptr<BlockStmtNode> Parser::parseBlockStmt() {
    SourceLocation loc = consume(TokenType::LBRACE, "Expected '{'").location;
    auto block = std::make_unique<BlockStmtNode>(loc);

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        try {
            auto stmt = parseStatement();
            if (stmt) {
                block->statements.push_back(std::move(stmt));
            }
        } catch (const ParseError&) {
            synchronize();
        }
    }

    consume(TokenType::RBRACE, "Expected '}' after block");
    return block;
}

std::unique_ptr<StmtNode> Parser::parseIfStmt() {
    SourceLocation loc = consume(TokenType::KEYWORD_IF, "Expected 'if'").location;
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    auto condition = parseExpression();

    // Spec Page 13 error recovery: missing ')' before '{'
    if (check(TokenType::RPAREN)) {
        advance();
    } else {
        recordError(peek(), "Missing ')' after if condition");
        if (!check(TokenType::LBRACE)) {
            throw ParseError("Unrecoverable error in if condition");
        }
    }

    auto thenBranch = parseStatement();
    std::unique_ptr<StmtNode> elseBranch = nullptr;

    if (match(TokenType::KEYWORD_ELSE)) {
        elseBranch = parseStatement();
    }

    return std::make_unique<IfStmtNode>(std::move(condition), std::move(thenBranch), std::move(elseBranch), loc);
}

std::unique_ptr<StmtNode> Parser::parseWhileStmt() {
    SourceLocation loc = consume(TokenType::KEYWORD_WHILE, "Expected 'while'").location;
    consume(TokenType::LPAREN, "Expected '(' after 'while'");
    auto condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after condition");
    auto body = parseStatement();
    return std::make_unique<WhileStmtNode>(std::move(condition), std::move(body), loc);
}

std::unique_ptr<StmtNode> Parser::parseForStmt() {
    SourceLocation loc = consume(TokenType::KEYWORD_FOR, "Expected 'for'").location;
    consume(TokenType::LPAREN, "Expected '(' after 'for'");

    std::unique_ptr<ExprNode> init = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        init = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after for-init");

    std::unique_ptr<ExprNode> condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after for-condition");

    std::unique_ptr<ExprNode> increment = nullptr;
    if (!check(TokenType::RPAREN)) {
        increment = parseExpression();
    }
    consume(TokenType::RPAREN, "Expected ')' after for-clause");

    auto body = parseStatement();
    return std::make_unique<ForStmtNode>(std::move(init), std::move(condition), std::move(increment), std::move(body), loc);
}

std::unique_ptr<StmtNode> Parser::parseReturnStmt() {
    SourceLocation loc = consume(TokenType::KEYWORD_RETURN, "Expected 'return'").location;
    std::unique_ptr<ExprNode> value = nullptr;

    if (!check(TokenType::SEMICOLON)) {
        value = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after return statement");
    return std::make_unique<ReturnStmtNode>(std::move(value), loc);
}

std::unique_ptr<StmtNode> Parser::parsePrintStmt() {
    SourceLocation loc = consume(TokenType::KEYWORD_PRINT, "Expected 'print'").location;
    std::vector<std::unique_ptr<ExprNode>> exprs;

    if (!check(TokenType::SEMICOLON)) {
        exprs = parseArgumentList();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after print statement");
    return std::make_unique<PrintStmtNode>(std::move(exprs), loc);
}

std::unique_ptr<StmtNode> Parser::parseExprOrDeclStmt() {
    // LL(2) check: IDENT ':' denotes a scoped local declaration
    if (check(TokenType::IDENTIFIER) && peekAhead(1).type == TokenType::COLON) {
        auto decl = parseDeclaration();
        return std::make_unique<DeclStmtNode>(std::move(decl), decl ? decl->location : SourceLocation{});
    }

    SourceLocation loc = peek().location;
    if (match(TokenType::SEMICOLON)) {
        return std::make_unique<ExprStmtNode>(nullptr, loc);
    }

    auto expr = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression");
    return std::make_unique<ExprStmtNode>(std::move(expr), loc);
}

// =============================================================================
// Expressions & Precedence Hierarchy
// =============================================================================

std::unique_ptr<ExprNode> Parser::parseExpression() {
    return parseAssignment();
}

// Precedence 1: Assignment '=' (Right-Associative)
std::unique_ptr<ExprNode> Parser::parseAssignment() {
    auto expr = parseLogicalOr();

    if (match(TokenType::OP_ASSIGN)) {
        SourceLocation loc = previous().location;
        auto right = parseAssignment(); // Right recursion
        return std::make_unique<BinaryExprNode>("=", std::move(expr), std::move(right), loc);
    }

    return expr;
}

// Precedence 2: Logical OR '||'
std::unique_ptr<ExprNode> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();

    while (match(TokenType::OP_LOR)) {
        SourceLocation loc = previous().location;
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryExprNode>("||", std::move(expr), std::move(right), loc);
    }

    return expr;
}

// Precedence 3: Logical AND '&&'
std::unique_ptr<ExprNode> Parser::parseLogicalAnd() {
    auto expr = parseEquality();

    while (match(TokenType::OP_LAND)) {
        SourceLocation loc = previous().location;
        auto right = parseEquality();
        expr = std::make_unique<BinaryExprNode>("&&", std::move(expr), std::move(right), loc);
    }

    return expr;
}

// Precedence 4: Equality '==' | '!='
std::unique_ptr<ExprNode> Parser::parseEquality() {
    auto expr = parseRelational();

    while (match({TokenType::OP_EQ, TokenType::OP_NEQ})) {
        std::string op = previous().lexeme;
        SourceLocation loc = previous().location;
        auto right = parseRelational();
        expr = std::make_unique<BinaryExprNode>(op, std::move(expr), std::move(right), loc);
    }

    return expr;
}

// Precedence 5: Relational '<' | '<=' | '>' | '>='
std::unique_ptr<ExprNode> Parser::parseRelational() {
    auto expr = parseAdditive();

    while (match({TokenType::OP_LT, TokenType::OP_LEQ, TokenType::OP_GT, TokenType::OP_GEQ})) {
        std::string op = previous().lexeme;
        SourceLocation loc = previous().location;
        auto right = parseAdditive();
        expr = std::make_unique<BinaryExprNode>(op, std::move(expr), std::move(right), loc);
    }

    return expr;
}

// Precedence 6: Additive '+' | '-'
std::unique_ptr<ExprNode> Parser::parseAdditive() {
    auto expr = parseMultiplicative();

    while (match({TokenType::OP_PLUS, TokenType::OP_MINUS})) {
        std::string op = previous().lexeme;
        SourceLocation loc = previous().location;
        auto right = parseMultiplicative();
        expr = std::make_unique<BinaryExprNode>(op, std::move(expr), std::move(right), loc);
    }

    return expr;
}

// Precedence 7: Multiplicative '*' | '/' | '%'
std::unique_ptr<ExprNode> Parser::parseMultiplicative() {
    auto expr = parseExponentiation();

    while (match({TokenType::OP_MUL, TokenType::OP_DIV, TokenType::OP_MOD})) {
        std::string op = previous().lexeme;
        SourceLocation loc = previous().location;
        auto right = parseExponentiation();
        expr = std::make_unique<BinaryExprNode>(op, std::move(expr), std::move(right), loc);
    }

    return expr;
}

// Precedence 8: Exponentiation '^' (Right-Associative)
std::unique_ptr<ExprNode> Parser::parseExponentiation() {
    auto expr = parseUnary();

    if (match(TokenType::OP_EXP)) {
        SourceLocation loc = previous().location;
        auto right = parseExponentiation(); // Right recursion
        return std::make_unique<BinaryExprNode>("^", std::move(expr), std::move(right), loc);
    }

    return expr;
}

// Precedence 9: Prefix Unary '-' | '!'
std::unique_ptr<ExprNode> Parser::parseUnary() {
    if (match({TokenType::OP_MINUS, TokenType::OP_LNOT})) {
        std::string op = previous().lexeme;
        SourceLocation loc = previous().location;
        auto operand = parseUnary();
        return std::make_unique<UnaryExprNode>(op, std::move(operand), /*isPostfix=*/false, loc);
    }

    return parsePostfix();
}

// Precedence 10: Postfix '++', '--', '[...]', '(args)'
std::unique_ptr<ExprNode> Parser::parsePostfix() {
    auto expr = parsePrimary();

    while (true) {
        if (match(TokenType::LBRACKET)) {
            SourceLocation loc = previous().location;
            auto index = parseExpression();
            consume(TokenType::RBRACKET, "Expected ']' after array index");
            expr = std::make_unique<SubscriptExprNode>(std::move(expr), std::move(index), loc);
        } else if (match(TokenType::LPAREN)) {
            SourceLocation loc = previous().location;
            std::vector<std::unique_ptr<ExprNode>> args;
            if (!check(TokenType::RPAREN)) {
                args = parseArgumentList();
            }
            consume(TokenType::RPAREN, "Expected ')' after argument list");

            std::string calleeName = "anonymous";
            SourceLocation calleeLoc = expr->location;
            if (auto idNode = dynamic_cast<IdentifierExprNode*>(expr.get())) {
                calleeName = idNode->name;
            }
            expr = std::make_unique<CallExprNode>(calleeName, std::move(args), calleeLoc, loc);
        } else if (match(TokenType::OP_INC)) {
            SourceLocation loc = previous().location;
            expr = std::make_unique<UnaryExprNode>("++", std::move(expr), /*isPostfix=*/true, loc);
        } else if (match(TokenType::OP_DEC)) {
            SourceLocation loc = previous().location;
            expr = std::make_unique<UnaryExprNode>("--", std::move(expr), /*isPostfix=*/true, loc);
        } else {
            break;
        }
    }

    return expr;
}

// Precedence 11: Primary
std::unique_ptr<ExprNode> Parser::parsePrimary() {
    if (match(TokenType::INTEGER_LITERAL)) {
        int64_t val = std::stoll(previous().lexeme);
        return std::make_unique<IntLiteralNode>(val, previous().location);
    }
    if (match(TokenType::BOOLEAN_LITERAL)) {
        bool val = (previous().lexeme == "true");
        return std::make_unique<BoolLiteralNode>(val, previous().location);
    }
    if (match(TokenType::KEYWORD_TRUE)) {
        return std::make_unique<BoolLiteralNode>(true, previous().location);
    }
    if (match(TokenType::KEYWORD_FALSE)) {
        return std::make_unique<BoolLiteralNode>(false, previous().location);
    }
    if (match(TokenType::CHARACTER_LITERAL)) {
        char val = previous().lexeme.size() >= 2 ? previous().lexeme[1] : '\0';
        return std::make_unique<CharLiteralNode>(val, previous().lexeme, previous().location);
    }
    if (match(TokenType::STRING_LITERAL)) {
        // Strip outer quotes if desired, or store full string
        std::string raw = previous().lexeme;
        std::string content = (raw.size() >= 2) ? raw.substr(1, raw.size() - 2) : raw;
        return std::make_unique<StringLiteralNode>(content, previous().location);
    }
    if (match(TokenType::IDENTIFIER)) {
        return std::make_unique<IdentifierExprNode>(previous().lexeme, previous().location);
    }
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after parenthesized expression");
        return expr;
    }
    if (check(TokenType::LBRACE)) {
        return parseArrayInitializer();
    }

    throw error(peek(), "Expected expression, got '" + peek().lexeme + "'");
}

std::vector<std::unique_ptr<ExprNode>> Parser::parseArgumentList() {
    std::vector<std::unique_ptr<ExprNode>> args;
    args.push_back(parseExpression());

    while (match(TokenType::COMMA)) {
        args.push_back(parseExpression());
    }

    return args;
}

std::unique_ptr<ArrayInitNode> Parser::parseArrayInitializer() {
    SourceLocation loc = consume(TokenType::LBRACE, "Expected '{' for array initializer").location;
    std::vector<std::unique_ptr<ExprNode>> elements;

    if (!check(TokenType::RBRACE)) {
        elements.push_back(parseExpression());
        while (match(TokenType::COMMA)) {
            elements.push_back(parseExpression());
        }
    }

    consume(TokenType::RBRACE, "Expected '}' after array initializer");
    return std::make_unique<ArrayInitNode>(std::move(elements), loc);
}

} // namespace bminor
