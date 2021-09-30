#include "token.hpp"

crl::Token::Token(){
	this->type = crl::Token::Type::NONE;
	this->str = "";
	this->line = 0;
	this->column = 0;
}

crl::Token::~Token(){}

#include <sstream>
std::string crl::Token::to_string() const {
	std::ostringstream out;
	out	<< *this;
	return out.str();
}

std::ostream& operator << ( std::ostream& outs, const crl::Token &token){
	return outs << "[" << token.type << "]" << " (" << token.line << ", " << token.column << "): " << token.str;
}
