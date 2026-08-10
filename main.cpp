#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <cctype>
#include <string_view>

enum class TokenType{

	RETURN, INT, VOID,

	// literal
	IDENTIFIER, INT_LITERAL,

	// 2 arguments
	LEFT_BRACKET, RIGHT_BRACKET, // [  ]
	LEFT_PAREN, RIGHT_PAREN, //     (  )
	LEFT_BRACE, RIGHT_BRACE, //     {  }


	SEMICOLON
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
		keywords["void"]   = TokenType::VOID;
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
		while(isdigit(peek())){
			advance();
		}
		std::string s = source.substr(start, current - start);
		return {TokenType::INT_LITERAL, s, line};
	}

	Token readName(){
		while(isalpha(peek()) || peek() == '_'){
			advance();
		}
		std::string s = source.substr(start, current - start);
		// check for keywords
		auto key = keywords.find(s);
		if( key != keywords.end()){
			return {key->second, std::nullopt, line};
		}
		// var or name or something
		return {TokenType::IDENTIFIER, s, line};

	}
};


std::vector<Token> Lexer::tokenize(){

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
		default:
			if (isdigit(c)){
				Tokens.push_back(readNumber());
			}else if(isalpha(c) || c == '_'){
				Tokens.push_back(readName());
			}else{
				std::cerr << "Unable to create a token for this" << std::endl;
				return Tokens;
			}
			break;
		}
	}
	return Tokens;
}


std::string_view tokenTypeToString(TokenType type) {
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
        default:                       return "UNKNOWN";
    }
}


int main(int argc, char* argv[]){
	if( argc != 2 ){
		std::cerr << "Unexpected number of arguments." << std::endl;
		return -1;
	}
	std::ifstream file(argv[1]);
	if( !file.is_open()){
		std::cerr << "File not valid." << std::endl;
		return -1;
	}

	// Read the whole file
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string contents = buffer.str();

	// Tokens
	std::vector<Token> Tokens;

	// Lexer
	Lexer L(contents);
	Tokens = L.tokenize();
	std::cout << "These are your tokens for this program: " << std::endl;

	    // Print out the details of each token
	std::cout << "--- TOKEN ANALYSIS RESULTS ---" << std::endl;
	for (const auto& token : Tokens) {
		std::cout << "[Line " << token.line << "] " << tokenTypeToString(token.type);

        	// Only print the lexeme value if std::optional contains something
        	if (token.lexeme.has_value()) {
            	std::cout << " -> Value: \"" << token.lexeme.value() << "\"";
        	}
        	std::cout << std::endl;
    	}
    	std::cout << "------------------------------" << std::endl;
    	std::cout << "Total tokens processed: " << Tokens.size() << std::endl;



	return 0;
}

