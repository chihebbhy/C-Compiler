#include <iostream>
#include "lexer.hpp"
#include "parser.hpp"

int main(int argc, char* argv[]){
	if( argc != 2 ){
		std::cerr << "Unexpected number of arguments." << std::endl;
		return -1;
	}else if( argc == 1){
		std::string fileName;
		std::cout << "Please provide a file to analyze." << std::endl;
		std::cin >> fileName;
		argv[1] = const_cast<char*>(fileName.c_str());
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
	printTokens(Tokens);

	// Separator
	std::cout << std::endl << "------------------------------" << std::endl;



	// Parser
	Parser parser(Tokens);
	auto ast = parser.parse_program();
	for (const auto& what : ast) {
		print_ast(what.get());
		std::cout << "---- next ----" << std::endl;
	}


	return 0;
}
