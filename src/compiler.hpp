#pragma once

#include "exceptions.hpp"
#include "defs.h"
#include "ast.hpp"
#include <vector>
#include <string>



namespace crl {
	typedef struct preprocessor_lib{
		size_t libc;
		char **libv;
	} Pproc_lib;

	std::vector<Token> tokenize(std::string);
	void preprocess(std::vector<Token> &, Pproc_lib);
	Ast *generate_ast(std::vector<Token>);
	void semantic_check(Ast*);
	void generate_cas(Ast*, std::string);
}

