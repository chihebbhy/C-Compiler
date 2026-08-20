#include "test_framework.hpp"
#include "../lexer.hpp"
#include "../parser.hpp"
#include <string>
#include <stdexcept>

// Serializes an AST into a fully-parenthesized prefix-ish string so tests
// can assert on exact tree shape rather than relying on printed side effects.
static std::string ser(ASTNode* node) {
    if (!node) return "null";
    if (auto n = dynamic_cast<LiteralNode*>(node)) {
        return n->value;
    }
    if (auto n = dynamic_cast<UnaryNode*>(node)) {
        return "(" + opToString(n->op) + " " + ser(n->expr.get()) + ")";
    }
    if (auto n = dynamic_cast<BinaryNode*>(node)) {
        return "(" + ser(n->left.get()) + " " + opToString(n->op) + " " + ser(n->right.get()) + ")";
    }
    return "?";
}

static std::string parseToString(const std::string& src) {
    Lexer L(src);
    auto toks = L.tokenize();
    Parser P(toks);
    auto ast = P.parse_statement();
    return ser(ast.get());
}

// ---------------- Literals ----------------

TEST(parse_int_literal) {
    CHECK_EQ(parseToString("42"), std::string("42"));
}

TEST(parse_float_literal) {
    CHECK_EQ(parseToString("3.14"), std::string("3.14"));
}

TEST(parse_string_literal) {
    // Inherits the lexer's off-by-one quirk: the literal's value keeps the
    // trailing closing quote.
    CHECK_EQ(parseToString("\"hi\""), std::string("hi\""));
}

TEST(parse_char_literal) {
    CHECK_EQ(parseToString("'a'"), std::string("a"));
}

// ---------------- Unary expressions ----------------

TEST(parse_unary_minus) {
    CHECK_EQ(parseToString("-1"), std::string("(- 1)"));
}

TEST(parse_unary_not) {
    CHECK_EQ(parseToString("!1"), std::string("(! 1)"));
}

TEST(parse_unary_plus) {
    CHECK_EQ(parseToString("+1"), std::string("(+ 1)"));
}

TEST(parse_double_unary_minus) {
    CHECK_EQ(parseToString("--1"), std::string("(- (- 1))"));
}

// ---------------- Binary expressions: grouping / associativity ----------------
// NOTE: these assertions reflect the parser's ACTUAL current behavior.
// Because parse_expression always recurses with (op_precedence + 1) for the
// right-hand side, and the outer loop keeps running at the caller's
// min_precedence, operators end up combining strictly left-to-right
// regardless of their nominal precedence tier. These tests pin down that
// real behavior so regressions are caught if it changes.

TEST(parse_left_to_right_same_precedence) {
    CHECK_EQ(parseToString("1+2+3"), std::string("((1 + 2) + 3)"));
    CHECK_EQ(parseToString("1-2-3"), std::string("((1 - 2) - 3)"));
}

TEST(parse_mixed_term_and_factor_is_left_to_right_not_math_precedence) {
    // A "real" math precedence would give (1 + (2 * 3)); this parser gives
    // ((1 + 2) * 3) instead - documenting the current implementation.
    CHECK_EQ(parseToString("1+2*3"), std::string("((1 + 2) * 3)"));
    CHECK_EQ(parseToString("1*2+3"), std::string("((1 * 2) + 3)"));
}

TEST(parse_factor_chain) {
    CHECK_EQ(parseToString("2*3%4"), std::string("((2 * 3) % 4)"));
}

TEST(parse_comparison_and_logic_chain) {
    CHECK_EQ(parseToString("1<2&&3>4"), std::string("((1 < 2) && (3 > 4))"));
}

TEST(parse_equality_chain) {
    CHECK_EQ(parseToString("1==2!=0"), std::string("((1 == 2) != 0)"));
}

TEST(parse_chained_comparisons_left_to_right) {
    CHECK_EQ(parseToString("1<2==3<4"), std::string("(((1 < 2) == 3) < 4)"));
}

// ---------------- Parentheses ----------------

TEST(parse_parenthesized_expression) {
    CHECK_EQ(parseToString("(1+2)"), std::string("(1 + 2)"));
}

TEST(parse_parens_do_not_change_result_here) {
    // Because of the left-to-right quirk above, explicit parens around the
    // first two operands produce the same tree as the unparenthesized form.
    CHECK_EQ(parseToString("(1+2)*3"), std::string("((1 + 2) * 3)"));
}

TEST(parse_nested_parens) {
    CHECK_EQ(parseToString("((1))"), std::string("1"));
}

TEST(parse_unclosed_paren_throws) {
    bool threw = false;
    try {
        parseToString("(1+2");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
}

// ---------------- Error handling ----------------

TEST(parse_unexpected_token_throws) {
    bool threw = false;
    try {
        parseToString(";");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
}

TEST(parse_empty_input_throws) {
    bool threw = false;
    try {
        parseToString("");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
}

// ---------------- opToString helper ----------------

TEST(op_to_string_known_operators) {
    CHECK_EQ(opToString(TokenType::PLUS), std::string("+"));
    CHECK_EQ(opToString(TokenType::MINUS), std::string("-"));
    CHECK_EQ(opToString(TokenType::STAR), std::string("*"));
    CHECK_EQ(opToString(TokenType::AND_AND), std::string("&&"));
    CHECK_EQ(opToString(TokenType::OR_OR), std::string("||"));
    CHECK_EQ(opToString(TokenType::NOT), std::string("!"));
}

TEST(op_to_string_unknown_operator) {
    CHECK_EQ(opToString(TokenType::SEMICOLON), std::string("unknown"));
}
