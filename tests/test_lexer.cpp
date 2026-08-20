#include "test_framework.hpp"
#include "../lexer.hpp"
#include <vector>

// Helper: tokenize source and strip the trailing END_OF_FILE token
static std::vector<Token> lex(const std::string& src) {
    Lexer L(src);
    auto toks = L.tokenize();
    if (!toks.empty() && toks.back().type == TokenType::END_OF_FILE) {
        toks.pop_back();
    }
    return toks;
}

static std::vector<TokenType> types(const std::vector<Token>& toks) {
    std::vector<TokenType> out;
    for (auto& t : toks) out.push_back(t.type);
    return out;
}

static bool sameTypes(const std::vector<Token>& toks, std::vector<TokenType> expected) {
    auto got = types(toks);
    return got == expected;
}

// ---------------- Punctuation / single-char tokens ----------------

TEST(lex_single_char_punctuation) {
    auto toks = lex(";{}()[],.:?");
    CHECK(sameTypes(toks, {
        TokenType::SEMICOLON, TokenType::LEFT_BRACE, TokenType::RIGHT_BRACE,
        TokenType::LEFT_PAREN, TokenType::RIGHT_PAREN, TokenType::LEFT_BRACKET,
        TokenType::RIGHT_BRACKET, TokenType::COMMA, TokenType::DOT,
        TokenType::COLON, TokenType::QUESTION
    }));
}

TEST(lex_whitespace_is_skipped) {
    auto toks = lex("  ;\t;\r;");
    CHECK(sameTypes(toks, {TokenType::SEMICOLON, TokenType::SEMICOLON, TokenType::SEMICOLON}));
}

TEST(lex_newline_increments_line_number) {
    auto toks = lex(";\n;\n;");
    CHECK_EQ(toks.size(), 3u);
    CHECK_EQ(toks[0].line, 1);
    CHECK_EQ(toks[1].line, 2);
    CHECK_EQ(toks[2].line, 3);
}

// ---------------- Keywords ----------------

TEST(lex_keywords) {
    auto toks = lex("return int void if else while for char");
    CHECK(sameTypes(toks, {
        TokenType::RETURN, TokenType::INT, TokenType::VOID, TokenType::IF,
        TokenType::ELSE, TokenType::WHILE, TokenType::FOR, TokenType::CHAR
    }));
    // Keyword tokens should not carry a lexeme
    for (auto& t : toks) CHECK(!t.lexeme.has_value());
}

TEST(lex_float_keyword_maps_to_float_literal_type) {
    // Documents current (quirky) lexer behavior: the bare word "float" is
    // tokenized as TokenType::FLOAT_LITERAL rather than a distinct FLOAT type.
    auto toks = lex("float");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::FLOAT_LITERAL);
    CHECK(!toks[0].lexeme.has_value());
}

// ---------------- Identifiers ----------------

TEST(lex_identifier_simple) {
    auto toks = lex("myVar");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::IDENTIFIER);
    CHECK(toks[0].lexeme.has_value());
    CHECK_EQ(*toks[0].lexeme, std::string("myVar"));
}

TEST(lex_identifier_with_underscore_and_digits) {
    auto toks = lex("_foo_bar123");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::IDENTIFIER);
    CHECK_EQ(*toks[0].lexeme, std::string("_foo_bar123"));
}

TEST(lex_multiple_identifiers_separated_by_space) {
    auto toks = lex("foo bar baz");
    CHECK_EQ(toks.size(), 3u);
    CHECK_EQ(*toks[0].lexeme, std::string("foo"));
    CHECK_EQ(*toks[1].lexeme, std::string("bar"));
    CHECK_EQ(*toks[2].lexeme, std::string("baz"));
}

TEST(lex_identifier_keyword_boundary_not_a_prefix_match) {
    // "returning" must not be mistaken for the "return" keyword just
    // because it starts with it - the whole word has to match.
    auto toks = lex("returning");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::IDENTIFIER);
    CHECK_EQ(*toks[0].lexeme, std::string("returning"));
}

TEST(lex_keywords_are_case_sensitive) {
    // "Return" (capital R) is not the same as the "return" keyword.
    auto toks = lex("Return");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::IDENTIFIER);
    CHECK_EQ(*toks[0].lexeme, std::string("Return"));
}

// ---------------- Numeric literals ----------------

TEST(lex_int_literal) {
    auto toks = lex("12345");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::INT_LITERAL);
    CHECK_EQ(*toks[0].lexeme, std::string("12345"));
}

TEST(lex_float_literal) {
    auto toks = lex("3.14");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::FLOAT_LITERAL);
    CHECK_EQ(*toks[0].lexeme, std::string("3.14"));
}

TEST(lex_float_literal_second_dot_stops_number) {
    // "1.2.3" reads "1.2" as a float, then "." then "3" as int.
    auto toks = lex("1.2.3");
    CHECK_EQ(toks.size(), 3u);
    CHECK(toks[0].type == TokenType::FLOAT_LITERAL);
    CHECK_EQ(*toks[0].lexeme, std::string("1.2"));
    CHECK(toks[1].type == TokenType::DOT);
    CHECK(toks[2].type == TokenType::INT_LITERAL);
    CHECK_EQ(*toks[2].lexeme, std::string("3"));
}

TEST(lex_leading_dot_is_not_part_of_a_number) {
    // A number only starts via readNumber() when the *first* character is
    // a digit (dispatched from isdigit(c) in tokenize()). A leading "."
    // is instead consumed by the standalone DOT case in the switch, so
    // ".5" becomes DOT, INT_LITERAL("5") rather than a single float 0.5.
    auto toks = lex(".5");
    CHECK_EQ(toks.size(), 2u);
    CHECK(toks[0].type == TokenType::DOT);
    CHECK(toks[1].type == TokenType::INT_LITERAL);
    CHECK_EQ(*toks[1].lexeme, std::string("5"));
}

TEST(lex_trailing_dot_is_included_in_float) {
    // "3." is read as a complete float literal with a trailing dot and no
    // fractional digits.
    auto toks = lex("3.");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::FLOAT_LITERAL);
    CHECK_EQ(*toks[0].lexeme, std::string("3."));
}

TEST(lex_number_immediately_followed_by_letters_splits_into_two_tokens) {
    // readNumber() stops at the first non-digit/non-dot character, so
    // "123abc" becomes an INT_LITERAL followed by a separate IDENTIFIER
    // rather than one error token.
    auto toks = lex("123abc");
    CHECK_EQ(toks.size(), 2u);
    CHECK(toks[0].type == TokenType::INT_LITERAL);
    CHECK_EQ(*toks[0].lexeme, std::string("123"));
    CHECK(toks[1].type == TokenType::IDENTIFIER);
    CHECK_EQ(*toks[1].lexeme, std::string("abc"));
}

TEST(lex_leading_zeros_preserved_in_int_literal) {
    auto toks = lex("007");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::INT_LITERAL);
    CHECK_EQ(*toks[0].lexeme, std::string("007"));
}

// ---------------- String / char literals ----------------

TEST(lex_string_literal) {
    // NOTE: readString()'s substr math is off by one, so the lexeme keeps
    // the trailing closing quote instead of stripping both delimiters.
    // This test pins down that actual (buggy) behavior.
    auto toks = lex("\"hello world\"");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::STRING_LITERAL);
    CHECK_EQ(*toks[0].lexeme, std::string("hello world\""));
}

TEST(lex_empty_string_literal) {
    // Same off-by-one: an "empty" string literal still carries the
    // trailing quote character in its lexeme.
    auto toks = lex("\"\"");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::STRING_LITERAL);
    CHECK_EQ(*toks[0].lexeme, std::string("\""));
}

TEST(lex_unterminated_string_is_error) {
    auto toks = lex("\"never closed");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::ERROR);
}

TEST(lex_string_with_digits_and_punctuation_stays_one_token) {
    // Confirms the whole quoted body is captured as a single
    // STRING_LITERAL rather than being tokenized as its contents.
    auto toks = lex("\"1+2=3;\"");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::STRING_LITERAL);
    // (includes the trailing-quote quirk noted above)
    CHECK_EQ(*toks[0].lexeme, std::string("1+2=3;\""));
}

TEST(lex_string_spanning_multiple_lines_tracks_line_number) {
    // readString() increments `line` internally on embedded newlines, so
    // a multi-line string literal advances the line counter, and the
    // embedded "\n" is preserved verbatim in the lexeme (no escaping).
    auto toks = lex("\"line1\nline2\"");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::STRING_LITERAL);
    CHECK_EQ(*toks[0].lexeme, std::string("line1\nline2\""));
    CHECK_EQ(toks[0].line, 2);
}

TEST(lex_string_has_no_escape_sequence_support) {
    // There is no backslash handling in readString(), so a backslash is
    // just an ordinary character and does NOT escape the quote that
    // follows it. `"abc\"def"` therefore ends the string right after the
    // backslash (keeping it + the quote per the trailing-quote quirk),
    // leaving "def" and the final quote to be re-lexed on their own.
    std::string src = "\"abc\\\"def\"";
    auto toks = lex(src);
    CHECK_EQ(toks.size(), 3u);
    CHECK(toks[0].type == TokenType::STRING_LITERAL);
    CHECK_EQ(*toks[0].lexeme, std::string("abc\\\""));
    CHECK(toks[1].type == TokenType::IDENTIFIER);
    CHECK_EQ(*toks[1].lexeme, std::string("def"));
    CHECK(toks[2].type == TokenType::ERROR);
}

TEST(lex_char_literal) {
    auto toks = lex("'a'");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::CHAR_LITERAL);
    CHECK_EQ(*toks[0].lexeme, std::string("a"));
}

TEST(lex_char_literal_digit) {
    auto toks = lex("'5'");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::CHAR_LITERAL);
    CHECK_EQ(*toks[0].lexeme, std::string("5"));
}

TEST(lex_char_literal_has_no_escape_sequence_support) {
    // readString()'s char branch consumes exactly one raw character and
    // then expects the delimiter. It has no backslash handling, so an
    // escape sequence like '\n' fails: the backslash is consumed as "the
    // character", the following 'n' isn't the closing quote -> ERROR
    // (without consuming 'n'). That stray 'n' is then re-lexed as an
    // IDENTIFIER, and the dangling closing quote errors out on its own too.
    std::string src = "'\\n'";
    auto toks = lex(src);
    CHECK_EQ(toks.size(), 3u);
    CHECK(toks[0].type == TokenType::ERROR);
    CHECK(toks[1].type == TokenType::IDENTIFIER);
    CHECK_EQ(*toks[1].lexeme, std::string("n"));
    CHECK(toks[2].type == TokenType::ERROR);
}

TEST(lex_unterminated_char_is_error) {
    // On mismatch, readString() reports ERROR but does not consume the
    // offending character, so the tokenizer picks it back up on the next
    // loop iteration (here as a stray identifier "b").
    auto toks = lex("'ab");
    CHECK_EQ(toks.size(), 2u);
    CHECK(toks[0].type == TokenType::ERROR);
    CHECK(toks[1].type == TokenType::IDENTIFIER);
    CHECK_EQ(*toks[1].lexeme, std::string("b"));
}

// ---------------- Operators ----------------

TEST(lex_arithmetic_operators) {
    auto toks = lex("+ - * / %");
    CHECK(sameTypes(toks, {
        TokenType::PLUS, TokenType::MINUS, TokenType::STAR,
        TokenType::SLASH, TokenType::PERCENT
    }));
}

TEST(lex_compound_assignment_operators) {
    auto toks = lex("+= -= *= /= %=");
    CHECK(sameTypes(toks, {
        TokenType::PLUS_EQUAL, TokenType::MINUS_EQUAL, TokenType::STAR_EQUAL,
        TokenType::SLASH_EQUAL, TokenType::PERCENT_EQUAL
    }));
}

TEST(lex_comparison_operators) {
    auto toks = lex("== != < <= > >=");
    CHECK(sameTypes(toks, {
        TokenType::EQUAL_EQUAL, TokenType::NOT_EQUAL, TokenType::LESS,
        TokenType::LESS_EQUAL, TokenType::GREATER, TokenType::GREATER_EQUAL
    }));
}

TEST(lex_logic_operators) {
    auto toks = lex("&& || !");
    CHECK(sameTypes(toks, {TokenType::AND_AND, TokenType::OR_OR, TokenType::NOT}));
}

TEST(lex_single_equal_vs_double_equal) {
    auto toks = lex("= ==");
    CHECK(sameTypes(toks, {TokenType::EQUAL, TokenType::EQUAL_EQUAL}));
}

TEST(lex_single_ampersand_is_error) {
    // The lexer only supports && ; a lone '&' should error out.
    auto toks = lex("&");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::ERROR);
}

TEST(lex_single_pipe_is_error) {
    // Symmetric to '&': the lexer only supports || ; a lone '|' errors.
    auto toks = lex("|");
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::ERROR);
}

TEST(lex_standalone_less_and_greater) {
    auto toks = lex("< >");
    CHECK(sameTypes(toks, {TokenType::LESS, TokenType::GREATER}));
}

TEST(lex_standalone_not_vs_not_equal) {
    auto toks = lex("! !=");
    CHECK(sameTypes(toks, {TokenType::NOT, TokenType::NOT_EQUAL}));
}

TEST(lex_operator_tokens_carry_no_lexeme) {
    auto toks = lex("+ - == && !");
    for (auto& t : toks) CHECK(!t.lexeme.has_value());
}

TEST(lex_no_comment_support_slashes_are_division_operators) {
    // This lexer has no // or /* */ comment handling (per its own header
    // comment). "//comment" is therefore lexed as two SLASH tokens
    // followed by an ordinary identifier, not skipped.
    auto toks = lex("//comment");
    CHECK(sameTypes(toks, {TokenType::SLASH, TokenType::SLASH, TokenType::IDENTIFIER}));
    CHECK_EQ(*toks[2].lexeme, std::string("comment"));
}

// ---------------- Integration: small snippet ----------------

TEST(lex_full_statement_snippet) {
    auto toks = lex("int x = 42;");
    CHECK(sameTypes(toks, {
        TokenType::INT, TokenType::IDENTIFIER, TokenType::EQUAL,
        TokenType::INT_LITERAL, TokenType::SEMICOLON
    }));
    CHECK_EQ(*toks[1].lexeme, std::string("x"));
    CHECK_EQ(*toks[3].lexeme, std::string("42"));
}

TEST(lex_if_else_condition_snippet) {
    auto toks = lex("if (a >= b) { return a; } else { return b; }");
    CHECK(sameTypes(toks, {
        TokenType::IF, TokenType::LEFT_PAREN, TokenType::IDENTIFIER,
        TokenType::GREATER_EQUAL, TokenType::IDENTIFIER, TokenType::RIGHT_PAREN,
        TokenType::LEFT_BRACE, TokenType::RETURN, TokenType::IDENTIFIER,
        TokenType::SEMICOLON, TokenType::RIGHT_BRACE, TokenType::ELSE,
        TokenType::LEFT_BRACE, TokenType::RETURN, TokenType::IDENTIFIER,
        TokenType::SEMICOLON, TokenType::RIGHT_BRACE
    }));
}

TEST(lex_crlf_line_endings) {
    // '\r' is silently skipped and only '\n' advances the line counter,
    // so Windows-style CRLF endings still track lines correctly.
    auto toks = lex(";\r\n;\r\n;");
    CHECK_EQ(toks.size(), 3u);
    CHECK_EQ(toks[0].line, 1);
    CHECK_EQ(toks[1].line, 2);
    CHECK_EQ(toks[2].line, 3);
}

TEST(lex_chained_comparison_and_assignment_snippet) {
    auto toks = lex("a >= b <= c");
    CHECK(sameTypes(toks, {
        TokenType::IDENTIFIER, TokenType::GREATER_EQUAL, TokenType::IDENTIFIER,
        TokenType::LESS_EQUAL, TokenType::IDENTIFIER
    }));
}

TEST(lex_punctuation_tokens_carry_no_lexeme) {
    auto toks = lex(";{}()[],.:?");
    for (auto& t : toks) CHECK(!t.lexeme.has_value());
}

TEST(lex_number_then_operator_no_space) {
    // "3.+2" -> FLOAT_LITERAL("3."), PLUS, INT_LITERAL("2"): confirms
    // readNumber() stops cleanly the moment it sees a non-digit/non-dot,
    // even with no whitespace separating tokens.
    auto toks = lex("3.+2");
    CHECK_EQ(toks.size(), 3u);
    CHECK(toks[0].type == TokenType::FLOAT_LITERAL);
    CHECK_EQ(*toks[0].lexeme, std::string("3."));
    CHECK(toks[1].type == TokenType::PLUS);
    CHECK(toks[2].type == TokenType::INT_LITERAL);
    CHECK_EQ(*toks[2].lexeme, std::string("2"));
}

TEST(lex_appends_end_of_file_token) {
    Lexer L("x");
    auto toks = L.tokenize();
    CHECK_EQ(toks.size(), 2u);
    CHECK(toks.back().type == TokenType::END_OF_FILE);
}

TEST(lex_empty_source_yields_only_eof) {
    Lexer L("");
    auto toks = L.tokenize();
    CHECK_EQ(toks.size(), 1u);
    CHECK(toks[0].type == TokenType::END_OF_FILE);
}