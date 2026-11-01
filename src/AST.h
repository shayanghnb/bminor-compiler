#ifndef BMINOR_AST_H
#define BMINOR_AST_H

#include "Token.h"
#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace bminor {

// Forward declarations
class ASTNode;
class DeclNode;
class StmtNode;
class ExprNode;
class BlockStmtNode;

// ============================================================================
// Type System Representation
// ============================================================================

enum class TypeKind {
    INTEGER,
    BOOLEAN,
    CHAR,
    STRING,
    VOID,
    AUTO,
    ARRAY,
    FUNCTION
};

std::string_view typeKindToString(TypeKind kind);

struct TypeNode {
    TypeKind kind{TypeKind::VOID};
    std::shared_ptr<TypeNode> subtype{nullptr}; // For array element type or function return type
    std::unique_ptr<ExprNode> arraySizeExpr{nullptr}; // Dimension expression for array
    std::vector<std::shared_ptr<TypeNode>> paramTypes; // For function parameter types

    TypeNode() = default;
    explicit TypeNode(TypeKind k) : kind(k) {}
    TypeNode(TypeKind k, std::shared_ptr<TypeNode> sub) : kind(k), subtype(std::move(sub)) {}

    // Factory methods
    static std::shared_ptr<TypeNode> makeInteger()  { return std::make_shared<TypeNode>(TypeKind::INTEGER); }
    static std::shared_ptr<TypeNode> makeBoolean()  { return std::make_shared<TypeNode>(TypeKind::BOOLEAN); }
    static std::shared_ptr<TypeNode> makeChar()     { return std::make_shared<TypeNode>(TypeKind::CHAR); }
    static std::shared_ptr<TypeNode> makeString()   { return std::make_shared<TypeNode>(TypeKind::STRING); }
    static std::shared_ptr<TypeNode> makeVoid()     { return std::make_shared<TypeNode>(TypeKind::VOID); }
    static std::shared_ptr<TypeNode> makeAuto()     { return std::make_shared<TypeNode>(TypeKind::AUTO); }
    static std::shared_ptr<TypeNode> makeArray(std::shared_ptr<TypeNode> elemType, std::unique_ptr<ExprNode> size = nullptr);
    static std::shared_ptr<TypeNode> makeFunction(std::shared_ptr<TypeNode> retType, std::vector<std::shared_ptr<TypeNode>> params);

    std::string toString() const;
};

// ============================================================================
// Base AST Node
// ============================================================================

class ASTNode {
public:
    SourceLocation location;
    // Semantic Analysis Type Annotation field (initialized to nullptr in Phase 1)
    std::shared_ptr<TypeNode> resolvedType{nullptr};

    explicit ASTNode(SourceLocation loc = {}) : location(std::move(loc)) {}
    virtual ~ASTNode() = default;

    virtual void print(std::ostream& out, int indent = 0) const = 0;
};

// ============================================================================
// Root Node
// ============================================================================

class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<DeclNode>> declarations;

    explicit ProgramNode(SourceLocation loc = {}) : ASTNode(std::move(loc)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

// ============================================================================
// Declarations Hierarchy
// ============================================================================

class DeclNode : public ASTNode {
public:
    std::string name;
    std::shared_ptr<TypeNode> declaredType;

    DeclNode(std::string name, std::shared_ptr<TypeNode> type, SourceLocation loc)
        : ASTNode(std::move(loc)), name(std::move(name)), declaredType(std::move(type)) {}
};

class VarDeclNode : public DeclNode {
public:
    std::unique_ptr<ExprNode> initializer{nullptr};

    VarDeclNode(std::string name, std::shared_ptr<TypeNode> type, std::unique_ptr<ExprNode> init, SourceLocation loc)
        : DeclNode(std::move(name), std::move(type), std::move(loc)), initializer(std::move(init)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class ParamDeclNode : public DeclNode {
public:
    ParamDeclNode(std::string name, std::shared_ptr<TypeNode> type, SourceLocation loc)
        : DeclNode(std::move(name), std::move(type), std::move(loc)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class FuncDeclNode : public DeclNode {
public:
    std::vector<std::unique_ptr<ParamDeclNode>> parameters;
    std::unique_ptr<BlockStmtNode> body{nullptr}; // nullptr for function prototypes

    FuncDeclNode(std::string name,
                 std::shared_ptr<TypeNode> fnType,
                 std::vector<std::unique_ptr<ParamDeclNode>> params,
                 std::unique_ptr<BlockStmtNode> body,
                 SourceLocation loc)
        : DeclNode(std::move(name), std::move(fnType), std::move(loc)),
          parameters(std::move(params)),
          body(std::move(body)) {}

    bool isPrototype() const { return body == nullptr; }

    void print(std::ostream& out, int indent = 0) const override;
};

// ============================================================================
// Statements Hierarchy
// ============================================================================

class StmtNode : public ASTNode {
public:
    explicit StmtNode(SourceLocation loc = {}) : ASTNode(std::move(loc)) {}
};

class BlockStmtNode : public StmtNode {
public:
    std::vector<std::unique_ptr<StmtNode>> statements;

    explicit BlockStmtNode(SourceLocation loc = {}) : StmtNode(std::move(loc)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class DeclStmtNode : public StmtNode {
public:
    std::unique_ptr<DeclNode> declaration;

    explicit DeclStmtNode(std::unique_ptr<DeclNode> decl, SourceLocation loc = {})
        : StmtNode(std::move(loc)), declaration(std::move(decl)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class ExprStmtNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> expression{nullptr};

    explicit ExprStmtNode(std::unique_ptr<ExprNode> expr, SourceLocation loc = {})
        : StmtNode(std::move(loc)), expression(std::move(expr)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class IfStmtNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<StmtNode> thenBranch;
    std::unique_ptr<StmtNode> elseBranch{nullptr};

    IfStmtNode(std::unique_ptr<ExprNode> cond,
               std::unique_ptr<StmtNode> thenB,
               std::unique_ptr<StmtNode> elseB,
               SourceLocation loc)
        : StmtNode(std::move(loc)),
          condition(std::move(cond)),
          thenBranch(std::move(thenB)),
          elseBranch(std::move(elseB)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class WhileStmtNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<StmtNode> body;

    WhileStmtNode(std::unique_ptr<ExprNode> cond, std::unique_ptr<StmtNode> body, SourceLocation loc)
        : StmtNode(std::move(loc)), condition(std::move(cond)), body(std::move(body)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class ForStmtNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> init{nullptr};
    std::unique_ptr<ExprNode> condition{nullptr};
    std::unique_ptr<ExprNode> increment{nullptr};
    std::unique_ptr<StmtNode> body;

    ForStmtNode(std::unique_ptr<ExprNode> init,
                std::unique_ptr<ExprNode> cond,
                std::unique_ptr<ExprNode> inc,
                std::unique_ptr<StmtNode> body,
                SourceLocation loc)
        : StmtNode(std::move(loc)),
          init(std::move(init)),
          condition(std::move(cond)),
          increment(std::move(inc)),
          body(std::move(body)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class ReturnStmtNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> value{nullptr};

    explicit ReturnStmtNode(std::unique_ptr<ExprNode> val, SourceLocation loc)
        : StmtNode(std::move(loc)), value(std::move(val)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class PrintStmtNode : public StmtNode {
public:
    std::vector<std::unique_ptr<ExprNode>> expressions;

    PrintStmtNode(std::vector<std::unique_ptr<ExprNode>> exprs, SourceLocation loc)
        : StmtNode(std::move(loc)), expressions(std::move(exprs)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

// ============================================================================
// Expressions Hierarchy
// ============================================================================

class ExprNode : public ASTNode {
public:
    explicit ExprNode(SourceLocation loc = {}) : ASTNode(std::move(loc)) {}
};

class BinaryExprNode : public ExprNode {
public:
    std::string op;
    std::unique_ptr<ExprNode> left;
    std::unique_ptr<ExprNode> right;

    BinaryExprNode(std::string op, std::unique_ptr<ExprNode> left, std::unique_ptr<ExprNode> right, SourceLocation loc)
        : ExprNode(std::move(loc)), op(std::move(op)), left(std::move(left)), right(std::move(right)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class UnaryExprNode : public ExprNode {
public:
    std::string op;
    std::unique_ptr<ExprNode> operand;
    bool isPostfix{false};

    UnaryExprNode(std::string op, std::unique_ptr<ExprNode> operand, bool postfix, SourceLocation loc)
        : ExprNode(std::move(loc)), op(std::move(op)), operand(std::move(operand)), isPostfix(postfix) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class CallExprNode : public ExprNode {
public:
    std::string callee;
    std::vector<std::unique_ptr<ExprNode>> arguments;
    SourceLocation calleeLocation;

    CallExprNode(std::string callee,
                 std::vector<std::unique_ptr<ExprNode>> args,
                 SourceLocation calleeLoc,
                 SourceLocation loc)
        : ExprNode(std::move(loc)),
          callee(std::move(callee)),
          arguments(std::move(args)),
          calleeLocation(std::move(calleeLoc)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class SubscriptExprNode : public ExprNode {
public:
    std::unique_ptr<ExprNode> array;
    std::unique_ptr<ExprNode> index;

    SubscriptExprNode(std::unique_ptr<ExprNode> arr, std::unique_ptr<ExprNode> idx, SourceLocation loc)
        : ExprNode(std::move(loc)), array(std::move(arr)), index(std::move(idx)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class IdentifierExprNode : public ExprNode {
public:
    std::string name;

    IdentifierExprNode(std::string name, SourceLocation loc)
        : ExprNode(std::move(loc)), name(std::move(name)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class IntLiteralNode : public ExprNode {
public:
    int64_t value{0};

    IntLiteralNode(int64_t val, SourceLocation loc)
        : ExprNode(std::move(loc)), value(val) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class BoolLiteralNode : public ExprNode {
public:
    bool value{false};

    BoolLiteralNode(bool val, SourceLocation loc)
        : ExprNode(std::move(loc)), value(val) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class CharLiteralNode : public ExprNode {
public:
    char value{'\0'};
    std::string rawLexeme;

    CharLiteralNode(char val, std::string raw, SourceLocation loc)
        : ExprNode(std::move(loc)), value(val), rawLexeme(std::move(raw)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class StringLiteralNode : public ExprNode {
public:
    std::string value;

    StringLiteralNode(std::string val, SourceLocation loc)
        : ExprNode(std::move(loc)), value(std::move(val)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class ArrayInitNode : public ExprNode {
public:
    std::vector<std::unique_ptr<ExprNode>> elements;

    ArrayInitNode(std::vector<std::unique_ptr<ExprNode>> elems, SourceLocation loc)
        : ExprNode(std::move(loc)), elements(std::move(elems)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

} // namespace bminor

#endif // BMINOR_AST_H
