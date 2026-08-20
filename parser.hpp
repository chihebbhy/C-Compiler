#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "lexer.hpp" // Assume this header defines the Token and TokenType structures

// --- 2. AST (Abstract Syntax Tree) Nodes ---
struct ASTNode { virtual ~ASTNode() = default; };

struct LiteralNode : public ASTNode {
    std::string value;
    LiteralNode(std::string v) : value(v) {}
};

struct UnaryNode : public ASTNode {
    TokenType op;
    std::unique_ptr<ASTNode> expr;
    UnaryNode(TokenType o, std::unique_ptr<ASTNode> e) : op(o), expr(std::move(e)) {}
};

struct BinaryNode : public ASTNode {
    TokenType op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    BinaryNode(TokenType o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
};

// --- 3. Parser State and Precedence Binding Powers ---
enum Precedence {
    PREC_NONE,
    PREC_OR,            // ||
    PREC_AND,           // &&
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < > <= >=
    PREC_TERM,          // + -
    PREC_FACTOR,        // * / %
    PREC_UNARY          // ! - (unary)
};

class Parser {
    std::vector<Token> tokens;
    size_t index = 0;

    const Token& peek() const {
        if (index >= tokens.size()) {
            throw std::runtime_error(
                "Unexpected end of token stream"
            );
        }

        return tokens[index];
    }
    Token advance() {
    if (index >= tokens.size()) {
        throw std::runtime_error(
            "Unexpected end of token stream"
        );
    }

    return tokens[index++];
    }

    // Maps a token type to its binding power for infix operations
    Precedence get_precedence(TokenType type) {
        switch (type) {
            case TokenType::AND_AND:        return PREC_AND;
            case TokenType::OR_OR:          return PREC_OR;
            case TokenType::EQUAL_EQUAL:      
            case TokenType::NOT_EQUAL:      return PREC_EQUALITY;
            case TokenType::LESS:      
            case TokenType::LESS_EQUAL:
            case TokenType::GREATER:   
            case TokenType::GREATER_EQUAL:  return PREC_COMPARISON;
            case TokenType::PLUS:
            case TokenType::MINUS:          return PREC_TERM;
            case TokenType::PERCENT: 
            case TokenType::STAR:           return PREC_FACTOR;

            default:                     
                return PREC_NONE;
        }
    }

public:
    Parser(std::vector<Token> t) : tokens(t) {}

    // --- Pratt Parsing Core Engine ---
    std::unique_ptr<ASTNode> parse_expression(Precedence min_precedence) {
        Token token = advance();
        std::unique_ptr<ASTNode> left;

        // 1. Prefix Handling (Unary or Literals)
        switch (token.type) {
            case TokenType::STRING_LITERAL:
            case TokenType::CHAR_LITERAL:
            case TokenType::INT_LITERAL:
            case TokenType::FLOAT_LITERAL:
                if (!token.lexeme) {
                    throw std::runtime_error(
                        "Literal token missing lexeme"
                    );
                }

                left = std::make_unique<LiteralNode>(
                    *token.lexeme
                );
                break;

            case TokenType::MINUS:
            case TokenType::NOT:
            case TokenType::PLUS: 
                left = std::make_unique<UnaryNode>(
                    token.type,
                    parse_expression(PREC_UNARY)
                );
                break;
            
            case TokenType::LEFT_PAREN: {
                left = parse_expression(PREC_NONE);
                if (peek().type != TokenType::RIGHT_PAREN) {
                    throw std::runtime_error(
                        "Expected closing parenthesis"
                    );
                }
                advance(); // consume RIGHT_PAREN
                break;
            }

            default:
                throw std::runtime_error(
                    "Unexpected token"
                );
        }

        // 2. Infix Handling (Loops as long as the next operator binds tighter)
        while (min_precedence < get_precedence(peek().type)) {
            Token op_token = advance();

            Precedence op_prec = get_precedence(op_token.type);

            auto right = parse_expression(static_cast<Precedence>(op_prec + 1)); // Right-associative for same precedence

            left = std::make_unique<BinaryNode>(
                op_token.type,
                std::move(left),
                std::move(right)
            );
        }

        return left;
    }

    // --- Top-Level Recursive Descent Hand-off ---
    std::unique_ptr<ASTNode> parse_statement() {
        // In a real compiler, you would check for 'if', 'while', 'return', etc.
        // For a raw expression statement, hand off to Pratt parser with lowest precedence.
        return parse_expression(PREC_NONE);
    }
};

inline std::string opToString(TokenType op) {
    switch (op) {
        case TokenType::PLUS:          return "+";
        case TokenType::MINUS:         return "-";
        case TokenType::STAR:          return "*";
        case TokenType::SLASH:         return "/";
        case TokenType::PERCENT:       return "%";

        case TokenType::EQUAL_EQUAL:   return "==";
        case TokenType::NOT_EQUAL:     return "!=";

        case TokenType::LESS:          return "<";
        case TokenType::LESS_EQUAL:    return "<=";
        case TokenType::GREATER:       return ">";
        case TokenType::GREATER_EQUAL: return ">=";

        case TokenType::AND_AND:       return "&&";
        case TokenType::OR_OR:         return "||";

        case TokenType::NOT:           return "!";

        default:
            return "unknown";
    }
}

inline void print_ast(ASTNode* node, int depth = 0) {
    // Helper function to print AST for verification
    if (!node) return;
    std::string indent(depth * 2, ' ');
    if (auto n = dynamic_cast<LiteralNode*>(node)) {
        std::cout << indent << "Literal: " << n->value << "\n";
    } else if (auto n = dynamic_cast<UnaryNode*>(node)) {
        std::cout << indent << "Unary Op: " << opToString(n->op) << "\n";
        print_ast(n->expr.get(), depth + 1);
    } else if (auto n = dynamic_cast<BinaryNode*>(node)) {
        std::cout << indent << "Binary Op: " << opToString(n->op) << "\n";
        print_ast(n->left.get(), depth + 1);
        print_ast(n->right.get(), depth + 1);
    }
}