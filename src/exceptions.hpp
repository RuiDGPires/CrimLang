#pragma once

#include "token.hpp"
#include <string>

namespace crl{
	class CL_Exception : public std::exception {
		protected:
			std::string name, msg; 
			int line, column;
		public:
			std::string what() noexcept;	

			CL_Exception(int, int);
	};
	
	class SyntaxError : public CL_Exception{
		public:
			SyntaxError(int, int, std::string);
	};

	class UnexpectedToken : public CL_Exception {
		public:
			UnexpectedToken(crl::Token&);
	};

	class AssertionError : public CL_Exception {
		public:
			AssertionError(int, int, std::string);
	};
}
