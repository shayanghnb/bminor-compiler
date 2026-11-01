#include "Highlighter.h"
#include <fstream>
#include <sstream>

namespace bminor {

std::string_view highlightCategoryToClass(HighlightCategory cat) {
    switch (cat) {
        case HighlightCategory::KEYWORD:             return "kw";
        case HighlightCategory::TYPE_NAME:           return "type";
        case HighlightCategory::FUNCTION_NAME:       return "fn";
        case HighlightCategory::VARIABLE_IDENTIFIER: return "ident";
        case HighlightCategory::LITERAL_NUMBER_BOOL: return "lit";
        case HighlightCategory::LITERAL_STRING_CHAR: return "str";
        case HighlightCategory::OPERATOR_DELIMITER:  return "op";
        case HighlightCategory::COMMENT:             return "cmt";
        case HighlightCategory::ERROR:               return "err";
        default:                                     return "plain";
    }
}

std::string_view highlightCategoryToANSI(HighlightCategory cat) {
    switch (cat) {
        case HighlightCategory::KEYWORD:             return "\033[1;34m";      // Bold blue
        case HighlightCategory::TYPE_NAME:           return "\033[36m";        // Teal / cyan
        case HighlightCategory::FUNCTION_NAME:       return "\033[33m";        // Yellow / gold
        case HighlightCategory::VARIABLE_IDENTIFIER: return "\033[37m";        // White / default
        case HighlightCategory::LITERAL_NUMBER_BOOL: return "\033[38;5;208m";  // Orange
        case HighlightCategory::LITERAL_STRING_CHAR: return "\033[32m";        // Warm green
        case HighlightCategory::OPERATOR_DELIMITER:  return "\033[38;5;250m";  // Light gray
        case HighlightCategory::COMMENT:             return "\033[90;3m";      // Dim gray, italic
        case HighlightCategory::ERROR:               return "\033[4;31m";      // Red underline
        default:                                     return "\033[0m";
    }
}

Highlighter::Highlighter(std::string source, std::string filename)
    : m_source(std::move(source)), m_filename(std::move(filename)) {
    analyze();
}

Highlighter Highlighter::fromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Highlighter: Unable to open file: " + filepath);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return Highlighter(ss.str(), filepath);
}

void Highlighter::analyze() {
    m_functionNameLocations.clear();
    m_errorLocations.clear();

    // 1. Tokenize preserving comments for full-text reconstruction
    Lexer lexer(m_source, m_filename, /*preserveComments=*/true);
    m_tokens = lexer.tokenizeAll();

    // 2. Parse source to construct AST for context-sensitive identification
    Lexer parseLexer(m_source, m_filename, /*preserveComments=*/false);
    Parser parser(parseLexer);
    auto ast = parser.parseProgram();

    setASTContext(ast.get(), parser.getErrors());
}

void Highlighter::setASTContext(const ProgramNode* ast, const std::vector<SyntaxError>& errors) {
    m_functionNameLocations.clear();
    m_errorLocations.clear();

    for (const auto& err : errors) {
        m_errorLocations.insert({err.location.line, err.location.column});
    }

    if (ast) {
        collectASTContext(ast);
    }
}

void Highlighter::collectASTContext(const ASTNode* node) {
    if (!node) return;

    if (auto program = dynamic_cast<const ProgramNode*>(node)) {
        for (const auto& decl : program->declarations) {
            collectFromDecl(decl.get());
        }
    }
}

void Highlighter::collectFromDecl(const DeclNode* decl) {
    if (!decl) return;

    if (auto fn = dynamic_cast<const FuncDeclNode*>(decl)) {
        // Record function declaration identifier coordinate
        m_functionNameLocations.insert({fn->location.line, fn->location.column});
        for (const auto& param : fn->parameters) {
            collectFromDecl(param.get());
        }
        if (fn->body) {
            collectFromStmt(fn->body.get());
        }
    } else if (auto v = dynamic_cast<const VarDeclNode*>(decl)) {
        if (v->initializer) {
            collectFromExpr(v->initializer.get());
        }
    }
}

void Highlighter::collectFromStmt(const StmtNode* stmt) {
    if (!stmt) return;

    if (auto b = dynamic_cast<const BlockStmtNode*>(stmt)) {
        for (const auto& s : b->statements) {
            collectFromStmt(s.get());
        }
    } else if (auto i = dynamic_cast<const IfStmtNode*>(stmt)) {
        collectFromExpr(i->condition.get());
        collectFromStmt(i->thenBranch.get());
        if (i->elseBranch) {
            collectFromStmt(i->elseBranch.get());
        }
    } else if (auto w = dynamic_cast<const WhileStmtNode*>(stmt)) {
        collectFromExpr(w->condition.get());
        collectFromStmt(w->body.get());
    } else if (auto f = dynamic_cast<const ForStmtNode*>(stmt)) {
        if (f->init) collectFromExpr(f->init.get());
        if (f->condition) collectFromExpr(f->condition.get());
        if (f->increment) collectFromExpr(f->increment.get());
        collectFromStmt(f->body.get());
    } else if (auto r = dynamic_cast<const ReturnStmtNode*>(stmt)) {
        if (r->value) collectFromExpr(r->value.get());
    } else if (auto p = dynamic_cast<const PrintStmtNode*>(stmt)) {
        for (const auto& expr : p->expressions) {
            collectFromExpr(expr.get());
        }
    } else if (auto e = dynamic_cast<const ExprStmtNode*>(stmt)) {
        if (e->expression) collectFromExpr(e->expression.get());
    } else if (auto d = dynamic_cast<const DeclStmtNode*>(stmt)) {
        collectFromDecl(d->declaration.get());
    }
}

void Highlighter::collectFromExpr(const ExprNode* expr) {
    if (!expr) return;

    if (auto call = dynamic_cast<const CallExprNode*>(expr)) {
        // Record function call callee identifier coordinate
        m_functionNameLocations.insert({call->calleeLocation.line, call->calleeLocation.column});
        for (const auto& arg : call->arguments) {
            collectFromExpr(arg.get());
        }
    } else if (auto bin = dynamic_cast<const BinaryExprNode*>(expr)) {
        collectFromExpr(bin->left.get());
        collectFromExpr(bin->right.get());
    } else if (auto un = dynamic_cast<const UnaryExprNode*>(expr)) {
        collectFromExpr(un->operand.get());
    } else if (auto sub = dynamic_cast<const SubscriptExprNode*>(expr)) {
        collectFromExpr(sub->array.get());
        collectFromExpr(sub->index.get());
    } else if (auto arr = dynamic_cast<const ArrayInitNode*>(expr)) {
        for (const auto& elem : arr->elements) {
            collectFromExpr(elem.get());
        }
    }
}

HighlightCategory Highlighter::classifyToken(const Token& tok) const {
    // 1. Errors & Invalid tokens
    if (tok.type == TokenType::INVALID || m_errorLocations.count({tok.location.line, tok.location.column}) > 0) {
        return HighlightCategory::ERROR;
    }

    // 2. Comments
    if (tok.type == TokenType::COMMENT_SINGLE || tok.type == TokenType::COMMENT_BLOCK) {
        return HighlightCategory::COMMENT;
    }

    // 3. Identifiers: AST-Aware distinction between function names and variables
    if (tok.type == TokenType::IDENTIFIER) {
        if (m_functionNameLocations.count({tok.location.line, tok.location.column}) > 0) {
            return HighlightCategory::FUNCTION_NAME;
        }
        return HighlightCategory::VARIABLE_IDENTIFIER;
    }

    // 4. Type names vs Structural Keywords
    switch (tok.type) {
        case TokenType::KEYWORD_INTEGER:
        case TokenType::KEYWORD_BOOLEAN:
        case TokenType::KEYWORD_CHAR:
        case TokenType::KEYWORD_STRING:
        case TokenType::KEYWORD_VOID:
        case TokenType::KEYWORD_AUTO:
            return HighlightCategory::TYPE_NAME;

        case TokenType::KEYWORD_ARRAY:
        case TokenType::KEYWORD_FUNCTION:
        case TokenType::KEYWORD_IF:
        case TokenType::KEYWORD_ELSE:
        case TokenType::KEYWORD_FOR:
        case TokenType::KEYWORD_WHILE:
        case TokenType::KEYWORD_RETURN:
        case TokenType::KEYWORD_PRINT:
            return HighlightCategory::KEYWORD;

        // 5. Literals
        case TokenType::INTEGER_LITERAL:
        case TokenType::BOOLEAN_LITERAL:
        case TokenType::KEYWORD_TRUE:
        case TokenType::KEYWORD_FALSE:
            return HighlightCategory::LITERAL_NUMBER_BOOL;

        case TokenType::STRING_LITERAL:
        case TokenType::CHARACTER_LITERAL:
            return HighlightCategory::LITERAL_STRING_CHAR;

        default:
            // 6. Operators & Delimiters
            return HighlightCategory::OPERATOR_DELIMITER;
    }
}

std::string Highlighter::escapeHTML(const std::string& str) {
    std::string out;
    out.reserve(str.size() * 1.2);
    for (char c : str) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '\"': out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;        break;
        }
    }
    return out;
}

void Highlighter::highlightANSI(std::ostream& out) const {
    size_t lastPos = 0;

    for (const auto& tok : m_tokens) {
        if (tok.type == TokenType::EOF_TOKEN) break;

        // Output raw unhighlighted whitespace/newlines preceding token
        if (tok.startOffset > lastPos && tok.startOffset <= m_source.size()) {
            out << m_source.substr(lastPos, tok.startOffset - lastPos);
        }

        HighlightCategory cat = classifyToken(tok);
        out << highlightCategoryToANSI(cat) << tok.lexeme << "\033[0m";

        lastPos = tok.endOffset;
    }

    // Trailing content
    if (lastPos < m_source.size()) {
        out << m_source.substr(lastPos);
    }
}

void Highlighter::highlightHTML(std::ostream& out, const std::string& title) const {
    out << "<!DOCTYPE html>\n"
        << "<html lang=\"en\">\n"
        << "<head>\n"
        << "  <meta charset=\"UTF-8\">\n"
        << "  <title>" << escapeHTML(title) << "</title>\n"
        << "  <style>\n"
        << "    body {\n"
        << "      background-color: #1e1e2e;\n"
        << "      color: #cdd6f4;\n"
        << "      font-family: 'Consolas', 'Fira Code', 'Cascadia Code', 'Courier New', monospace;\n"
        << "      font-size: 14px;\n"
        << "      line-height: 1.5;\n"
        << "      margin: 24px;\n"
        << "    }\n"
        << "    pre {\n"
        << "      background-color: #181825;\n"
        << "      border: 1px solid #313244;\n"
        << "      border-radius: 8px;\n"
        << "      padding: 16px;\n"
        << "      overflow-x: auto;\n"
        << "    }\n"
        << "    .kw    { color: #89b4fa; font-weight: bold; }          /* Bold blue */\n"
        << "    .type  { color: #94e2d5; font-weight: 600; }           /* Teal / cyan */\n"
        << "    .fn    { color: #f9e2af; font-weight: bold; }          /* Yellow / gold */\n"
        << "    .ident { color: #cdd6f4; }                             /* White / default */\n"
        << "    .lit   { color: #fab387; }                             /* Orange */\n"
        << "    .str   { color: #a6e3a1; }                             /* Warm green */\n"
        << "    .op    { color: #a6adc8; }                             /* Light gray */\n"
        << "    .cmt   { color: #6c7086; font-style: italic; }         /* Dim gray, italic */\n"
        << "    .err   { color: #f38ba8; text-decoration: underline wavy #f38ba8; } /* Red underline */\n"
        << "  </style>\n"
        << "</head>\n"
        << "<body>\n"
        << "  <pre>";

    size_t lastPos = 0;

    for (const auto& tok : m_tokens) {
        if (tok.type == TokenType::EOF_TOKEN) break;

        // Output raw unhighlighted whitespace/newlines preceding token
        if (tok.startOffset > lastPos && tok.startOffset <= m_source.size()) {
            out << escapeHTML(m_source.substr(lastPos, tok.startOffset - lastPos));
        }

        HighlightCategory cat = classifyToken(tok);
        std::string_view cls = highlightCategoryToClass(cat);

        out << "<span class=\"" << cls << "\">" << escapeHTML(tok.lexeme) << "</span>";

        lastPos = tok.endOffset;
    }

    // Trailing content
    if (lastPos < m_source.size()) {
        out << escapeHTML(m_source.substr(lastPos));
    }

    out << "</pre>\n"
        << "</body>\n"
        << "</html>\n";
}

std::string Highlighter::toANSIString() const {
    std::ostringstream ss;
    highlightANSI(ss);
    return ss.str();
}

std::string Highlighter::toHTMLString(const std::string& title) const {
    std::ostringstream ss;
    highlightHTML(ss, title);
    return ss.str();
}

} // namespace bminor
