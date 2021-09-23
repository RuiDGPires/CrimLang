#include "compiler.hpp"
#include "defs.h"
#include <iostream>
#include <fstream>

bool isNumber(char c){
	return c >= '0' && c <= '9';
}
	
bool isLetter(char c){
	return (c >= 'a' && c <= 'z') || (c >= 'a' && c <= 'Z');
}

bool isSymbol(char c){
	return c == '(' || c == ')' || c == '{' || c == '}' || c == ';' \
						|| c == '=' || c == '+' || c == '-' || c == '*' || c == '/' || c == '$' \
						|| c == '#' || c == '"' || c == '\'' || c == '<' || c == '>';
}


enum ParseState{NONE, IDENT, NUM, QUOTE, SYMBOL, COMMENT};

std::vector<crl::Token> crl::tokenize(std::string filename){
	std::vector<Token> res;

	std::ifstream file;
	file.open(filename);

	char c;
	u32 line = 0;
	ParseState state = ParseState::NONE;

	while (file.get(c)){
		switch (state){
			case ParseState::NONE:

				break;
			case ParseState::IDENT:

				break;
			case ParseState::NUM:

				break;
			case ParseState::SYMBOL:

				break;
			default:
				
				break;
		}
	}

	file.close();
	return res;
}
