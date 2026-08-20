#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <cctype>
#include <string_view>
#pragma once


/*
token types:
	->, classes, structs, enums, typedefs, unions, bitfields, preprocessor directives, comments, and whitespace are not included in this lexer.
	and backslash weird cases like in strings.
*/
enum class TokenType{

	ERROR, // to be replaced by an error class

	RETURN, INT, VOID,

	IF, ELSE, WHILE, FOR, CHAR, 
	// literal
	IDENTIFIER, INT_LITERAL,FLOAT_LITERAL,
	CHAR_LITERAL,STRING_LITERAL,

	// operators 
	PLUS,           // + 
	MINUS,          // - 
	STAR,           // * 
	SLASH,          // / 
	PERCENT,        // % 

	// Assignment
	EQUAL,          // = 
	PLUS_EQUAL,     // +=
	MINUS_EQUAL,    // -=
	STAR_EQUAL,     // *=
	SLASH_EQUAL,    // /=
	PERCENT_EQUAL,  // %=

	// Comparison
	EQUAL_EQUAL,    // ==
	NOT_EQUAL,      // !=
	LESS,           // <
	LESS_EQUAL,     // <=
	GREATER,        // >
	GREATER_EQUAL,  // >=

	// Logic
	AND_AND,        // &&
	OR_OR,          // ||
	NOT,            // ! 	
	// 2 arguments 				
	LEFT_BRACKET, RIGHT_BRACKET, // [  ] 
	LEFT_PAREN, RIGHT_PAREN, //     (  )
	LEFT_BRACE, RIGHT_BRACE, //     {  }
	COMMA,          // ,				

	// Struct/member access
	DOT,            // .

	COLON,          // :				
	QUESTION,       // ?				

	SEMICOLON,	//						
	END_OF_FILE // 						
};

class Token{
	public:
		TokenType type;
		std::optional<std::string> lexeme;
		int line;
};

class Lexer{
	public:
		std::size_t start = 0;
		std::size_t current = 0;
		std::string source;
		int line = 1;
	std::unordered_map<std::string, TokenType> keywords;

	void initializeKeywords(){
		keywords["return"] = TokenType::RETURN;
		keywords["int"]    = TokenType::INT;
		keywords["float"]  = TokenType::FLOAT_LITERAL;
		keywords["void"]   = TokenType::VOID;
		keywords["if"]   = TokenType::IF;
		keywords["else"]   = TokenType::ELSE;
		keywords["while"]   = TokenType::WHILE;
		keywords["for"]   = TokenType::FOR;
		keywords["char"]   = TokenType::CHAR;

	}

	explicit Lexer(std::string sourceSTR) : source(std::move(sourceSTR)){
		initializeKeywords();
	}

	std::vector<Token> tokenize();

	bool isAtEnd(){
		return current >= source.length();
	}

	// reads current letter and moves to the next
	char advance(){
		if(isAtEnd()) return '\0';
		return source[current++];
	}
	// reads current letter
	char peek(){
		if(isAtEnd()) return '\0';
		return source[current];
	}

	// read whole number [0..9]
	Token readNumber(){
		bool hasDot = false;
		while(isdigit(peek()) || peek() == '.'){
			if( peek() == '.'){ 
				if (hasDot){
					break; }
				hasDot = true;
			}
			
			advance();
			
		}
		std::string s = source.substr(start, current - start);
		if(hasDot){
			return {TokenType::FLOAT_LITERAL, s, line};
		}else{
			return {TokenType::INT_LITERAL, s, line};
		}
	}

	Token readAlpha(){
		while(isalnum(peek()) || peek() == '_'){
			advance();
		}
		std::string s = source.substr(start, current - start);
		// check for keywords
		auto key = keywords.find(s);

		if( key != keywords.end()){ // if key is found in the keywords map
			return {key->second, std::nullopt, line};
		}
		// var or name or something
		return {TokenType::IDENTIFIER, s, line};

	}
	Token readString(char delimiter){
		if ( delimiter == '"'){
			while(peek() != delimiter && !isAtEnd()){
				if(peek() == '\n') line++;
				advance();
			}
			if( isAtEnd()){
				std::cerr << "Error: Unterminated string literal at line " << line << std::endl;
				return {TokenType::ERROR, std::nullopt, line};
			}
			advance();
			std::string s = source.substr(start + 1, current - start - 1);
			return {TokenType::STRING_LITERAL, s, line};
		}
		char c = advance();
		if (peek() != delimiter){
			std::cerr << "Error: Unterminated character literal at line " << line << std::endl;
			return {TokenType::ERROR, std::nullopt, line};
		}
		advance();
		return {TokenType::CHAR_LITERAL, std::string {c}, line};

	}
	Token readLogic(char c){
		// && or || 
		switch(c){
			case '&':{
				if(peek() == '&'){
					advance();
					return {TokenType::AND_AND, std::nullopt, line};
				}
				std::cerr << "Error: Invalid token at line " << line << std::endl;
				return {TokenType::ERROR, std::nullopt, line};
			}
			case '|':{
				if(peek() == '|'){
					advance();
					return {TokenType::OR_OR, std::nullopt, line};
				}
				std::cerr << "Error: Invalid token at line " << line << std::endl;
				return {TokenType::ERROR, std::nullopt, line};
			}

		}
		std::cerr << "Error: Invalid token at line " << line << std::endl;
		return {TokenType::ERROR, std::nullopt, line};
	}	
	Token readOperation(char c){
		//  +  -  *  /  %  
		// +=  -= *= /= %=  
		// =  !   <  >
		// == !=  <= >= 
		switch(c){
			case '+':{ // + +=
				if( peek() == '='){
					advance();
					return {TokenType::PLUS_EQUAL, std::nullopt, line};
				}
				return {TokenType::PLUS, std::nullopt, line};
			}
			case '-':{ // - -=
				if( peek() == '='){
					advance();
					return {TokenType::MINUS_EQUAL, std::nullopt, line};
				}
				return {TokenType::MINUS, std::nullopt, line};
			}
			case '*':{ // * *=
				if( peek() == '='){
					advance();
					return {TokenType::STAR_EQUAL, std::nullopt, line};
				}
				return {TokenType::STAR, std::nullopt, line};
			}
			case '/':{ // / /= 
				if( peek() == '='){
					advance();
					return {TokenType::SLASH_EQUAL, std::nullopt, line};
				}
				return {TokenType::SLASH, std::nullopt, line};
			}
			case '%':{ // % %=
				if( peek() == '='){
					advance();
					return {TokenType::PERCENT_EQUAL, std::nullopt, line};
				}
				return {TokenType::PERCENT, std::nullopt, line};
			}
			case '=':{ // = ==
				if( peek() == '='){
					advance();
					return {TokenType::EQUAL_EQUAL, std::nullopt, line};
				}
				return {TokenType::EQUAL, std::nullopt, line};
			}
			case '<':{ // < <=
				if( peek() == '='){
					advance();
					return {TokenType::LESS_EQUAL, std::nullopt, line};
				}
				return {TokenType::LESS, std::nullopt, line};
			}
			case '>':{ // > >=
				if( peek() == '='){
					advance();
					return {TokenType::GREATER_EQUAL, std::nullopt, line};
				}
				return {TokenType::GREATER, std::nullopt, line};
			}
			case '!':{ // ! !=
				if( peek() == '='){
					advance();
					return {TokenType::NOT_EQUAL, std::nullopt, line};
				}
				return {TokenType::NOT, std::nullopt, line};
			}
		}
		std::cerr << "Error: Invalid token at line " << line << std::endl;
		return {TokenType::ERROR, std::nullopt, line};
	}
};


inline std::vector<Token> Lexer::tokenize(){

	std::vector<Token> Tokens;

	while(!isAtEnd()){
		start = current;
		char c = advance();

		switch(c){
		case '\n' : line++;
			break;
		case ' '  :
		case '\t' :
		case '\r' :
			break;

		case ';'  : Tokens.push_back({TokenType::SEMICOLON, std:: nullopt, line}); break;
		case '{'  : Tokens.push_back({TokenType::LEFT_BRACE, std:: nullopt, line}); break;
		case '}'  : Tokens.push_back({TokenType::RIGHT_BRACE, std:: nullopt, line}); break;
		case '('  : Tokens.push_back({TokenType::LEFT_PAREN, std:: nullopt, line}); break;
		case ')'  : Tokens.push_back({TokenType::RIGHT_PAREN, std:: nullopt, line}); break;
		case '['  : Tokens.push_back({TokenType::LEFT_BRACKET, std:: nullopt, line}); break;
		case ']'  : Tokens.push_back({TokenType::RIGHT_BRACKET, std:: nullopt, line}); break;
		case ','  : Tokens.push_back({TokenType::COMMA, std:: nullopt, line}); break;
		case '.'  : Tokens.push_back({TokenType::DOT, std:: nullopt, line}); break;
		case ':'  : Tokens.push_back({TokenType::COLON, std:: nullopt, line}); break;
		case '?'  : Tokens.push_back({TokenType::QUESTION, std:: nullopt, line}); break;

		default:
			if (isdigit(c)){ // read float int literals
				Tokens.push_back(readNumber());
			}else if(isalpha(c) || c == '_'){ // read name and keywords
				Tokens.push_back(readAlpha());
			}else if(c == '"' || c == '\''){ // read string and char
				Tokens.push_back(readString(c));
			}else if(c == '&' || c == '|' ){
				Tokens.push_back(readLogic(c));
			}
			else if(c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '!' || c == '=' || c == '<' || c == '>'){
				Tokens.push_back(readOperation(c));
			}
			else{
				std::cerr << "Unable to create a token for this" << std::endl;
				Tokens.push_back({TokenType::ERROR, std::nullopt, line});
			}
			break;
		}
	}
	Tokens.push_back({TokenType::END_OF_FILE, std::nullopt, line});
	return Tokens;
}


inline std::string_view tokenTypeToString(TokenType type) {
	switch (type) {
		case TokenType::RETURN:        return "RETURN";
		case TokenType::INT:           return "INT";
		case TokenType::VOID:          return "VOID";
		case TokenType::IDENTIFIER:    return "IDENTIFIER";
		case TokenType::INT_LITERAL:   return "INT_LITERAL";
		case TokenType::LEFT_BRACKET:  return "LEFT_BRACKET";
		case TokenType::RIGHT_BRACKET: return "RIGHT_BRACKET";
		case TokenType::LEFT_PAREN:    return "LEFT_PAREN";
		case TokenType::RIGHT_PAREN:   return "RIGHT_PAREN";
		case TokenType::LEFT_BRACE:    return "LEFT_BRACE";
		case TokenType::RIGHT_BRACE:   return "RIGHT_BRACE";
		case TokenType::SEMICOLON:     return "SEMICOLON";
		case TokenType::COMMA:         return "COMMA";
		case TokenType::COLON:         return "COLON";
		case TokenType::QUESTION:      return "QUESTION";
		case TokenType::DOT:           return "DOT";
		case TokenType::IF:            return "IF";
		case TokenType::ELSE:          return "ELSE";
		case TokenType::WHILE:         return "WHILE";
		case TokenType::FOR:           return "FOR";
		case TokenType::CHAR:          return "CHAR";
		case TokenType::FLOAT_LITERAL:   return "FLOAT_LITERAL";
		case TokenType::CHAR_LITERAL:    return "CHAR_LITERAL";
		case TokenType::STRING_LITERAL:  return "STRING_LITERAL";
		case TokenType::PLUS:          return "PLUS";
		case TokenType::MINUS:         return "MINUS";
		case TokenType::STAR:          return "STAR";
		case TokenType::SLASH:         return "SLASH";
		case TokenType::PERCENT:       return "PERCENT";
		case TokenType::EQUAL:         return "EQUAL";
		case TokenType::PLUS_EQUAL:    return "PLUS_EQUAL";
		case TokenType::MINUS_EQUAL:   return "MINUS_EQUAL";
		case TokenType::STAR_EQUAL:    return "STAR_EQUAL";
		case TokenType::SLASH_EQUAL:   return "SLASH_EQUAL";
		case TokenType::PERCENT_EQUAL: return "PERCENT_EQUAL";
		case TokenType::EQUAL_EQUAL:   return "EQUAL_EQUAL";
		case TokenType::NOT_EQUAL:     return "NOT_EQUAL";
		case TokenType::LESS:          return "LESS";
		case TokenType::LESS_EQUAL:    return "LESS_EQUAL";
		case TokenType::GREATER:       return "GREATER";
		case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
		case TokenType::AND_AND:       return "AND_AND";
		case TokenType::OR_OR:         return "OR_OR";
		case TokenType::NOT:           return "NOT";
		case TokenType::END_OF_FILE:   return "END_OF_FILE";
		default:                       return "UNKNOWN";
	}
}

inline void printTokens(const std::vector<Token>& tokens) {
	std::cout << "--- TOKEN ANALYSIS RESULTS ---" << std::endl;
		// Print out the details of each token
	for (const auto& token : tokens) {
		std::cout << "[Line " << token.line << "] " << tokenTypeToString(token.type);

			// Only print the lexeme value if std::optional contains something
			if (token.lexeme.has_value()) {
				std::cout << " -> Value: \"" << token.lexeme.value() << "\"";
			}
			std::cout << std::endl;
		}
		std::cout << "------------------------------" << std::endl;
		std::cout << "Total tokens processed: " << tokens.size() << std::endl;
}