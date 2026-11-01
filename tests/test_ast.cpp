#include "../src/AST.h"
#include <iostream>
#include <cassert>

using namespace bminor;

int main() {
    std::cout << "======================================================\n";
    std::cout << " B-Minor Compiler AST Hierarchy Test Runner\n";
    std::cout << "======================================================\n\n";

    // ------------------------------------------------------------------------
    // Part 1: Construct the exact ReturnStmt from Course Specification (Page 12)
    // Code snippet: "return n * factorial(n - 1);" at line 3
    // ------------------------------------------------------------------------
    std::cout << "--- 1. AST for 'return n * factorial(n - 1);' (Course Example) ---\n";

    SourceLocation locReturn{"example.bm", 3, 5};
    SourceLocation locLeftN{"example.bm", 3, 12};
    SourceLocation locCallee{"example.bm", 3, 16};
    SourceLocation locArgN{"example.bm", 3, 26};
    SourceLocation locArgMinus{"example.bm", 3, 28};
    SourceLocation locArgOne{"example.bm", 3, 30};

    // Sub-expression: (n - 1)
    auto argMinus = std::make_unique<BinaryExprNode>(
        "-",
        std::make_unique<IdentifierExprNode>("n", locArgN),
        std::make_unique<IntLiteralNode>(1, locArgOne),
        locArgMinus
    );

    // Call expression: factorial(n - 1)
    std::vector<std::unique_ptr<ExprNode>> callArgs;
    callArgs.push_back(std::move(argMinus));

    auto callExpr = std::make_unique<CallExprNode>(
        "factorial",
        std::move(callArgs),
        locCallee,
        locCallee
    );

    // Binary expression: n * factorial(n - 1)
    auto multExpr = std::make_unique<BinaryExprNode>(
        "*",
        std::make_unique<IdentifierExprNode>("n", locLeftN),
        std::move(callExpr),
        SourceLocation{"example.bm", 3, 14}
    );

    // Return statement: return n * factorial(n - 1);
    auto returnStmt = std::make_unique<ReturnStmtNode>(
        std::move(multExpr),
        locReturn
    );

    // Print the ReturnStmt subtree
    returnStmt->print(std::cout, 0);

    // ------------------------------------------------------------------------
    // Part 2: Construct the Full Program & Function AST for factorial()
    //
    // factorial: function integer ( n: integer ) = {
    //     if (n <= 1) return 1;
    //     return n * factorial(n - 1);
    // }
    // ------------------------------------------------------------------------
    std::cout << "\n--- 2. Full AST for factorial() Function Definition ---\n";

    auto program = std::make_unique<ProgramNode>(SourceLocation{"example.bm", 1, 1});

    // Param: n: integer
    std::vector<std::unique_ptr<ParamDeclNode>> params;
    params.push_back(std::make_unique<ParamDeclNode>(
        "n",
        TypeNode::makeInteger(),
        SourceLocation{"example.bm", 1, 31}
    ));

    // Function type: function integer (integer)
    std::vector<std::shared_ptr<TypeNode>> paramTypes = { TypeNode::makeInteger() };
    auto fnType = TypeNode::makeFunction(TypeNode::makeInteger(), paramTypes);

    // Body Block
    auto bodyBlock = std::make_unique<BlockStmtNode>(SourceLocation{"example.bm", 1, 45});

    // Statement 1: if (n <= 1) return 1;
    auto ifCond = std::make_unique<BinaryExprNode>(
        "<=",
        std::make_unique<IdentifierExprNode>("n", SourceLocation{"example.bm", 2, 9}),
        std::make_unique<IntLiteralNode>(1, SourceLocation{"example.bm", 2, 14}),
        SourceLocation{"example.bm", 2, 11}
    );
    auto ifThen = std::make_unique<ReturnStmtNode>(
        std::make_unique<IntLiteralNode>(1, SourceLocation{"example.bm", 2, 24}),
        SourceLocation{"example.bm", 2, 17}
    );
    auto ifStmt = std::make_unique<IfStmtNode>(
        std::move(ifCond),
        std::move(ifThen),
        nullptr,
        SourceLocation{"example.bm", 2, 5}
    );
    bodyBlock->statements.push_back(std::move(ifStmt));

    // Statement 2: return n * factorial(n - 1);
    // (Reconstruct another copy for the full tree)
    auto argMinus2 = std::make_unique<BinaryExprNode>(
        "-",
        std::make_unique<IdentifierExprNode>("n", locArgN),
        std::make_unique<IntLiteralNode>(1, locArgOne),
        locArgMinus
    );
    std::vector<std::unique_ptr<ExprNode>> callArgs2;
    callArgs2.push_back(std::move(argMinus2));
    auto callExpr2 = std::make_unique<CallExprNode>(
        "factorial",
        std::move(callArgs2),
        locCallee,
        locCallee
    );
    auto multExpr2 = std::make_unique<BinaryExprNode>(
        "*",
        std::make_unique<IdentifierExprNode>("n", locLeftN),
        std::move(callExpr2),
        SourceLocation{"example.bm", 3, 14}
    );
    auto returnStmt2 = std::make_unique<ReturnStmtNode>(
        std::move(multExpr2),
        locReturn
    );
    bodyBlock->statements.push_back(std::move(returnStmt2));

    // Function Declaration
    auto funcDecl = std::make_unique<FuncDeclNode>(
        "factorial",
        fnType,
        std::move(params),
        std::move(bodyBlock),
        SourceLocation{"example.bm", 1, 1}
    );

    program->declarations.push_back(std::move(funcDecl));

    // Print the entire Program AST
    program->print(std::cout, 0);

    // ------------------------------------------------------------------------
    // Part 3: Verify Type Annotation Reservation (Phase 1 checks)
    // ------------------------------------------------------------------------
    assert(program->resolvedType == nullptr && "Phase 1: resolvedType must initialize to nullptr");
    assert(program->declarations[0]->resolvedType == nullptr && "Phase 1: resolvedType must initialize to nullptr");

    std::cout << "\n======================================================\n";
    std::cout << "[+] AST verification passed with 0 errors.\n";
    std::cout << "======================================================\n";

    return 0;
}
