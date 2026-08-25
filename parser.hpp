#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "lexer.hpp" // Assume this header defines the Token and TokenType structures

// ===================================================================
// ROADMAP - in order
// ===================================================================

//    ---- DONE ----
// 1. COMPOUND ASSIGNMENT OPERATORS  (+= -= *= /= %=)
//    Add PLUS_EQUAL, MINUS_EQUAL, STAR_EQUAL, SLASH_EQUAL, PERCENT_EQUAL
//    to get_precedence() at PREC_ASSIGN, mark them right-associative
//    (same as EQUAL). Cheapest possible win since assignment infra
//    already exists - just extend it.

//    ---- DONE ----
// 2. COMPARISON + LOGIC OPERATORS (< <= > >= == != && ||)
//    Already wired into get_precedence(). Write a couple of test
//    expressions (a < b && c == d) to confirm precedence looks right
//    end to end. If broken, this is where associativity/precedence
//    bugs like the last two would show up again.

// 3. GROUPING WITH COMMA (,)
//    Not usable yet - nothing consumes COMMA. Needed before you can
//    do function calls or parameter lists (step 8).

//    ---- DONE ----
// 4. BLOCKS  { ... }
//    Add a BlockNode (vector<unique_ptr<ASTNode>>).
//    Add parse_block(): expects LEFT_BRACE, loops calling
//    parse_statement() until RIGHT_BRACE, consumes RIGHT_BRACE.
//    This unlocks LEFT_BRACE/RIGHT_BRACE and is required by if/while/for
//    and function bodies below.

// 5. IF / ELSE
//    Add IfNode { condition, thenBranch, optional elseBranch }.
//    parse_if_statement(): consume IF, expect LEFT_PAREN, parse_expression,
//    expect RIGHT_PAREN, parse_block() for thenBranch, optionally consume
//    ELSE + parse_block() (or another if, for else-if chains).
//    Wire into parse_statement()'s dispatch (like RETURN is now).

// 6. WHILE
//    Add WhileNode { condition, body }.
//    parse_while_statement(): consume WHILE, LEFT_PAREN, expression,
//    RIGHT_PAREN, parse_block(). Wire into parse_statement().

// 7. FOR
//    Add ForNode { init, condition, increment, body }.
//    parse_for_statement(): consume FOR, LEFT_PAREN,
//    init-statement (uses SEMICOLON), condition expression (SEMICOLON),
//    increment expression, RIGHT_PAREN, parse_block().
//    This is the first thing that actually needs COMMA later if you
//    allow multiple init/increment expressions - skip that for now,
//    single init/increment is fine.

// 8. VARIABLE DECLARATIONS with TYPES (int, void, char + IDENTIFIER)
//    INT, VOID, CHAR are lexed as keywords but never consumed by the
//    parser. Add VarDeclNode { typeToken, name, optionalInitializer }.
//    parse_statement(): if peek() is INT/VOID/CHAR, call
//    parse_var_decl(): consume type token, expect IDENTIFIER, optionally
//    EQUAL + expression, expect SEMICOLON.
//    This also finally gives INT/VOID/CHAR a real use beyond sitting
//    unused in the keyword map.

// 9. ARRAYS / INDEXING  ( [ ] )
//     Add IndexNode { array, indexExpr }.
//     In parse_expression's infix loop, after primary parsing, check
//     for LEFT_PAREN... no wait, LEFT_BRACKET: if seen, parse index
//     expression, expect RIGHT_BRACKET, wrap left in IndexNode.
//     Also allow VarDeclNode to optionally have "[N]" for array size.
//     This is what finally uses LEFT_BRACKET/RIGHT_BRACKET.

// 10. FUNCTION CALLS  ( LEFT_PAREN ... COMMA ... RIGHT_PAREN )
//     Add CallNode { calleeName, args (vector<ASTNode>) }.
//     After parsing an IDENTIFIER as a prefix, peek for LEFT_PAREN;
//     if present, parse a comma-separated argument list (finally uses
//     COMMA!) until RIGHT_PAREN.

// 11. FUNCTION DEFINITIONS
//     Add FunctionNode { returnType, name, params, body(Block) }.
//     Top-level parse_program() needs to recognize
//     TYPE IDENTIFIER LEFT_PAREN ... RIGHT_PAREN LEFT_BRACE ... RIGHT_BRACE
//     as a function def instead of falling through to parse_statement().
//     Parameter list reuses the COMMA-separated logic from step 10.

// 12. TERNARY OPERATOR  ( ? : )
//     Add TernaryNode { condition, thenExpr, elseExpr }.
//     After parsing an expression at a precedence below QUESTION's
//     conceptual slot, check for QUESTION: if found, parse thenExpr,
//     expect COLON, parse elseExpr (right-associative, like a ?
//     b : c ? d : e). This is what finally uses QUESTION and COLON
//     (COLON's only other use would be labels/switch, which you don't
//     have here).

// 13. STRUCT/MEMBER ACCESS (.)
//     Only worth doing if you add struct types later - the lexer
//     comment says structs aren't handled by this lexer at all, so
//     DOT is effectively dead unless you extend the lexer too.
//     Lowest priority / optional - skip unless you add structs.

// 14. ERROR TOKEN -> real error handling
//     Right now ERROR just gets silently pushed into the token stream
//     and, when the parser hits it, falls into parse_expression's
//     default case and throws a generic "Unexpected token". Consider
//     giving it a dedicated message ("Invalid token near line X")
//     instead of the generic parser error, so lex errors and parse
//     errors are distinguishable.

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

struct ReturnNode : public ASTNode {
    std::unique_ptr<ASTNode> expr;
    ReturnNode(std::unique_ptr<ASTNode> e) : 
        expr(std::move(e)) {}
};

struct BlockNode : public ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;
};
// --- 3. Parser State and Precedence Binding Powers ---
enum Precedence {
    PREC_NONE,
    PREC_ASSIGNMENT,     // =
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
    bool is_right_associative(TokenType type) {
        switch (type) {
            case TokenType::EQUAL: // assignment is right-assoc
            case TokenType::PLUS_EQUAL:
            case TokenType::MINUS_EQUAL:
            case TokenType::STAR_EQUAL:
            case TokenType::SLASH_EQUAL:
            case TokenType::PERCENT_EQUAL:
                return true;
            default:
                return false;
        }
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
            case TokenType::SLASH:
            case TokenType::STAR:           return PREC_FACTOR;

            case TokenType::EQUAL:          
            case TokenType::PLUS_EQUAL:
            case TokenType::MINUS_EQUAL:
            case TokenType::STAR_EQUAL:
            case TokenType::SLASH_EQUAL:
            case TokenType::PERCENT_EQUAL:  return PREC_ASSIGNMENT;

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
            case TokenType::IDENTIFIER: 
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
                std::cerr << "Unexpected token: " << token.toString() << std::endl;
                throw std::runtime_error(
                    "Unexpected token"
                );
        }

        // 2. Infix Handling (Loops as long as the next operator binds tighter)
        while (min_precedence < get_precedence(peek().type)) {
            Token op_token = advance();

            Precedence op_prec = get_precedence(op_token.type);

            auto right = parse_expression(
                is_right_associative(op_token.type) ? static_cast<Precedence>(op_prec - 1) : op_prec
            ); // Right-associative for same precedence

            left = std::make_unique<BinaryNode>(
                op_token.type,
                std::move(left),
                std::move(right)
            );
        }

        return left;
    }

    std::unique_ptr<ASTNode> parse_return_statement(){
        advance(); // consume RETURN token

        if (peek().type == TokenType::SEMICOLON) {
            advance(); // consume ; token
            return std::make_unique<ReturnNode>(nullptr); // return nullptr
        }
        auto value = parse_expression(PREC_NONE);

        if(peek().type != TokenType::SEMICOLON) {
            throw std::runtime_error(
                "Expected \";\" after return");
        }
        advance(); // consume ; token

        return std::make_unique<ReturnNode>(std::move(value));
    }

    std::unique_ptr<ASTNode> parse_block(){
        advance(); // consume {
        std::unique_ptr<BlockNode> block = std::make_unique<BlockNode>();
        while(peek().type != TokenType::RIGHT_BRACE){
            block->statements.push_back(parse_statement());
        }
        advance(); // consume }
        return block;
    }

    // --- Top-Level Recursive Descent Hand-off ---
    std::unique_ptr<ASTNode> parse_statement() {
        // In a real compiler, you would check for 'if', 'while', 'return', etc.
        // For a raw expression statement, hand off to Pratt parser with lowest precedence.
        if(peek().type == TokenType::RETURN){
            return parse_return_statement();
        }

        if(peek().type == TokenType::LEFT_BRACE){
            return parse_block();
        }
        auto expr =  parse_expression(PREC_NONE);
        if(peek().type != TokenType::SEMICOLON) {
            throw std::runtime_error(
                "Expected \";\" after expression");
        }
        advance(); // consume ; token
        return expr;

    }

    std::vector<std::unique_ptr<ASTNode>> parse_program() {
        std::vector<std::unique_ptr<ASTNode>> statements;

        while (peek().type != TokenType::END_OF_FILE) {
            statements.push_back(parse_statement());
        }

        return statements;
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

        case TokenType::EQUAL:         return "=";
        case TokenType::PLUS_EQUAL:    return "+=";
        case TokenType::MINUS_EQUAL:   return "-=";
        case TokenType::STAR_EQUAL:    return "*=";
        case TokenType::SLASH_EQUAL:   return "/=";
        case TokenType::PERCENT_EQUAL: return "%=";
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
    }else if (auto n = dynamic_cast<ReturnNode*>(node)) {
        std::cout << indent << "Return\n";
        if (n->expr)
            print_ast(n->expr.get(), depth + 1);
    }else if (auto n = dynamic_cast<BlockNode*>(node)) {
        std::cout << indent << "Block" << std::endl;
        for(const auto& stt: n->statements){
            print_ast(stt.get(), depth + 1);
        }
    }
}