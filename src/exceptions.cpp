#include "exceptions.hpp"
#include <sstream>

namespace crl{
	CL_Exception::CL_Exception(int line, int column){
		this->line = line;
		this->column = column;
		this->name = "CrimLang Exception";
		this->msg = "";
	}

	const char *CL_Exception::what() noexcept{
		std::ostringstream o;

		o << this->name << " (" << this->line << ", " << this->column << "): " << this->msg;
		return o.str().c_str();
	}


	UnexpectedToken::UnexpectedToken(crl::Token t) : CL_Exception(t.line, t.column){
		this->name = "Unexpected Token";
		this->msg = t.str;
	}

	SyntaxError::SyntaxError(int l, int c, std::string s) : CL_Exception(l, c){
		this->name = "Syntax Error";
		this->msg = s;
	}
}
