#pragma once

#include "defs.h"
#include <vector>
#include <string>

namespace crl {
	class Token {
		public:
		enum Type{NONE, IDENT, INT, DEC, TIMES, POW, PLUS, MINUS, SLASH, COMMENTSYM, DOT, LPAREN, RPAREN, LBRACK, RBRACK, DOLLAR, CLN, SEMICLN, COMMA, BECOMES, PLEQ, MIEQ, TIEQ, SLEQ, EQL, GEQ, LEQ, GTR, LSS, NEQ};
		std::string str;
		Type type;

		u32 line, column;
		Token();
		~Token();
	};
	std::vector<Token> tokenize(std::string);
}

#include <iostream>
std::ostream& operator << ( std::ostream& outs, const crl::Token &token);
