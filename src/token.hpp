#pragma once
#include "defs.h"
#include <string>

namespace crl {
	class Token {
		public:
		enum Type{NONE, IDENT,
		//Keywords
		IF, ELSE, MUT, VOID, TCHAR, TSTR, I32, U32, F32, F64, RETURN, FOR, WHILE,
		//Others
		INT, DEC, TIMES, PLUS, MINUS, SLASH, CHAR, STRING, DOT, AND, LPAREN, RPAREN, LBRACK, RBRACK, DOLLAR, CLN, SEMICLN, COMMA, BECOMES, PLEQ, MIEQ, TIEQ, SLEQ, EQL, GEQ, LEQ, GTR, LSS, NEQ, CAS,
	  EOF_};

		std::string str;
		Type type;

		u32 line, column;
		Token();
		~Token();

		std::string to_string() const;
	};
}

#include <iostream>
std::ostream& operator << ( std::ostream& outs, const crl::Token &token);
