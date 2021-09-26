#pragma once
#include "defs.h"
#include <string>

namespace crl {
	class Token {
		public:
		enum Type{NONE, IDENT,
		//Keywords
		IF, ELSE, MUT, CHAR, I32, U32, FLOAT, DOUBLE, RETURN, FOR, WHILE,
		//Others
		INT, DEC, TIMES, POW, PLUS, MINUS, SLASH, COMMENTSYM, DOT, LPAREN, RPAREN, LBRACK, RBRACK, DOLLAR, CLN, SEMICLN, COMMA, BECOMES, PLEQ, MIEQ, TIEQ, SLEQ, EQL, GEQ, LEQ, GTR, LSS, NEQ
		};

		std::string str;
		Type type;

		u32 line, column;
		Token();
		~Token();
	};
}

#include <iostream>
std::ostream& operator << ( std::ostream& outs, const crl::Token &token);
