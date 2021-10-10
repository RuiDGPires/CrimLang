#pragma once

#include "token.hpp"
#include <string>

namespace crl{
	class CL_Exception : public std::exception {
		protected:
			std::string name, msg; 
			int line, column;
		public:
			const char *what() noexcept;	

			CL_Exception(int, int);
	};
	
	class SyntaxError : public CL_Exception{
		public:
			SyntaxError(std::string);
	};

	class UnexpectedToken : public CL_Exception {
		public:
			UnexpectedToken(crl::Token);
	};
}
