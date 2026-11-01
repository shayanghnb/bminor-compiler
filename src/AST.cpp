#include "AST.h"
#include <sstream>

namespace bminor {

static std::string getIndent(int indent) {
    return std::string(indent * 2, ' ');
}

std::string_view typeKindToString(TypeKind kind) {
    switch (kind) {
        case TypeKind::INTEGER:  return "integer";
        case TypeKind::BOOLEAN:  return "boolean";
        case TypeKind::CHAR:     return "char";
        case TypeKind::STRING:   return "string";
        case TypeKind::VOID:     return "void";
        case TypeKind::AUTO:     return "auto";
        case TypeKind::ARRAY:    return "array";
        case TypeKind::FUNCTION: return "function";
        default:                 return "unknown";
    }
}

std::shared_ptr<TypeNode> TypeNode::makeArray(std::shared_ptr<TypeNode> elemType, std::unique_ptr<ExprNode> size) {
    auto node = std::make_shared<TypeNode>(TypeKind::ARRAY, std::move(elemType));
    node->arraySizeExpr = std::move(size);
    return node;
}

std::shared_ptr<TypeNode> TypeNode::makeFunction(std::shared_ptr<TypeNode> retType, std::vector<std::shared_ptr<TypeNode>> params) {
    auto node = std::make_shared<TypeNode>(TypeKind::FUNCTION, std::move(retType));
    node->paramTypes = std::move(params);
    return node;
}

std::string TypeNode::toString() const {
    switch (kind) {
        case TypeKind::INTEGER:  return "integer";
        case TypeKind::BOOLEAN:  return "boolean";
        case TypeKind::CHAR:     return "char";
        case TypeKind::STRING:   return "string";
        case TypeKind::VOID:     return "void";
        case TypeKind::AUTO:     return "auto";
        case TypeKind::ARRAY: {
            std::string s = "array [";
            // In full AST, arraySizeExpr could be printed if present
            s += "] ";
            s += (subtype ? subtype->toString() : "void");
            return s;
        }
        case TypeKind::FUNCTION: {
            std::string s = "function ";
            s += (subtype ? subtype->toString() : "void");
            s += " (";
            for (size_t i = 0; i < paramTypes.size(); ++i) {
                if (i > 0) s += ", ";
                s += (paramTypes[i] ? paramTypes[i]->toString() : "void");
            }
            s += ")";
            return s;
        }
        default:
            return "unknown";
    }
}

// ============================================================================
// ProgramNode Implementation
// ============================================================================

void ProgramNode::print(std::ostream& out, int indent) const {
    out << getIndent(indent) << "Program(loc=" << location.toString() << ")\n";
    for (const auto& decl : declarations) {
        if (decl) {
            decl->print(out, indent + 1);
        }
    }
}

// ============================================================================
// Declarations Implementation
// ============================================================================

void VarDeclNode::print(std::ostream& out, int indent) const {
    out << getIndent(indent) << "VarDecl(name='" << name << "', type="
        << (declaredType ? declaredType->toString() : "null")
        << ", loc=" << location.toString() << ")\n";
    if (initializer) {
        out << getIndent(indent + 1) << "init: ";
        initializer->print(out, indent + 1);
    }
}

void ParamDeclNode::print(std::ostream& out, int indent) const {
    out << getIndent(indent) << "ParamDecl(name='" << name << "', type="
        << (declaredType ? declaredType->toString() : "null")
        << ", loc=" << location.toString() << ")\n";
}

void FuncDeclNode::print(std::ostream& out, int indent) const {
    out << getIndent(indent) << "FuncDecl(name='" << name << "', type="
        << (declaredType ? declaredType->toString() : "null")
        << ", loc=" << location.toString() << ")\n";

    if (!parameters.empty()) {
        out << getIndent(indent + 1) << "parameters:\n";
        for (const auto& param : parameters) {
            if (param) {
                param->print(out, indent + 2);
            }
        }
    }

    if (body) {
        out << getIndent(indent + 1) << "body:\n";
        body->print(out, indent + 2);
    } else {
        out << getIndent(indent + 1) << "body: <prototype>\n";
    }
}

// ============================================================================
// Statements Implementation
// ============================================================================

void BlockStmtNode::print(std::ostream& out, int indent) const {
    out << getIndent(indent) << "BlockStmt(loc=" << location.toString() << ")\n";
    for (const auto& stmt : statements) {
        if (stmt) {
            stmt->print(out, indent + 1);
        }
    }
}

void DeclStmtNode::print(std::ostream& out, int indent) const {
    if (declaration) {
        declaration->print(out, indent);
    }
}

void ExprStmtNode::print(std::ostream& out, int indent) const {
    if (expression) {
        out << getIndent(indent) << "ExprStmt(loc=" << location.toString() << ")\n";
        out << getIndent(indent + 1) << "expr: ";
        expression->print(out, indent + 1);
    } else {
        out << getIndent(indent) << "EmptyStmt(loc=" << location.toString() << ")\n";
    }
}

void IfStmtNode::print(std::ostream& out, int indent) const {
    out << getIndent(indent) << "IfStmt(loc=" << location.toString() << ")\n";
    if (condition) {
        out << getIndent(indent + 1) << "condition: ";
        condition->print(out, indent + 1);
    }
    if (thenBranch) {
        out << getIndent(indent + 1) << "then:\n";
        thenBranch->print(out, indent + 2);
    }
    if (elseBranch) {
        out << getIndent(indent + 1) << "else:\n";
        elseBranch->print(out, indent + 2);
    }
}

void WhileStmtNode::print(std::ostream& out, int indent) const {
    out << getIndent(indent) << "WhileStmt(loc=" << location.toString() << ")\n";
    if (condition) {
        out << getIndent(indent + 1) << "condition: ";
        condition->print(out, indent + 1);
    }
    if (body) {
        out << getIndent(indent + 1) << "body:\n";
        body->print(out, indent + 2);
    }
}

void ForStmtNode::print(std::ostream& out, int indent) const {
    out << getIndent(indent) << "ForStmt(loc=" << location.toString() << ")\n";
    if (init) {
        out << getIndent(indent + 1) << "init: ";
        init->print(out, indent + 1);
    }
    if (condition) {
        out << getIndent(indent + 1) << "condition: ";
        condition->print(out, indent + 1);
    }
    if (increment) {
        out << getIndent(indent + 1) << "increment: ";
        increment->print(out, indent + 1);
    }
    if (body) {
        out << getIndent(indent + 1) << "body:\n";
        body->print(out, indent + 2);
    }
}

void ReturnStmtNode::print(std::ostream& out, int indent) const {
    out << getIndent(indent) << "ReturnStmt(loc=" << location.toString() << ")\n";
    if (value) {
        out << getIndent(indent + 1) << "value: ";
        value->print(out, indent + 1);
    }
}

void PrintStmtNode::print(std::ostream& out, int indent) const {
    out << getIndent(indent) << "PrintStmt(loc=" << location.toString() << ")\n";
    for (size_t i = 0; i < expressions.size(); ++i) {
        out << getIndent(indent + 1) << "arg[" << i << "]: ";
        if (expressions[i]) {
            expressions[i]->print(out, indent + 1);
        }
    }
}

// ============================================================================
// Expressions Implementation
// ============================================================================

void BinaryExprNode::print(std::ostream& out, int indent) const {
    out << "BinaryExpr(op='" << op << "')\n";
    if (left) {
        out << getIndent(indent + 1) << "left:  ";
        left->print(out, indent + 1);
    }
    if (right) {
        out << getIndent(indent + 1) << "right: ";
        right->print(out, indent + 1);
    }
}

void UnaryExprNode::print(std::ostream& out, int indent) const {
    out << "UnaryExpr(op='" << op << "', postfix=" << (isPostfix ? "true" : "false")
        << ", loc=" << location.toString() << ")\n";
    if (operand) {
        out << getIndent(indent + 1) << "operand: ";
        operand->print(out, indent + 1);
    }
}

void CallExprNode::print(std::ostream& out, int indent) const {
    out << "CallExpr(callee='" << callee << "', loc=" << calleeLocation.toString() << ")\n";
    for (size_t i = 0; i < arguments.size(); ++i) {
        out << getIndent(indent + 1) << "args[" << i << "]: ";
        if (arguments[i]) {
            arguments[i]->print(out, indent + 1);
        }
    }
}

void SubscriptExprNode::print(std::ostream& out, int indent) const {
    out << "SubscriptExpr(loc=" << location.toString() << ")\n";
    if (array) {
        out << getIndent(indent + 1) << "array: ";
        array->print(out, indent + 1);
    }
    if (index) {
        out << getIndent(indent + 1) << "index: ";
        index->print(out, indent + 1);
    }
}

void IdentifierExprNode::print(std::ostream& out, int /*indent*/) const {
    out << "Identifier(name='" << name << "', loc=" << location.toString() << ")\n";
}

void IntLiteralNode::print(std::ostream& out, int /*indent*/) const {
    out << "IntLiteral(value=" << value << ", loc=" << location.toString() << ")\n";
}

void BoolLiteralNode::print(std::ostream& out, int /*indent*/) const {
    out << "BoolLiteral(value=" << (value ? "true" : "false") << ", loc=" << location.toString() << ")\n";
}

void CharLiteralNode::print(std::ostream& out, int /*indent*/) const {
    out << "CharLiteral(value='" << value << "', loc=" << location.toString() << ")\n";
}

void StringLiteralNode::print(std::ostream& out, int /*indent*/) const {
    out << "StringLiteral(value=\"" << value << "\", loc=" << location.toString() << ")\n";
}

void ArrayInitNode::print(std::ostream& out, int indent) const {
    out << "ArrayInit(count=" << elements.size() << ", loc=" << location.toString() << ")\n";
    for (size_t i = 0; i < elements.size(); ++i) {
        out << getIndent(indent + 1) << "elem[" << i << "]: ";
        if (elements[i]) {
            elements[i]->print(out, indent + 1);
        }
    }
}

} // namespace bminor
