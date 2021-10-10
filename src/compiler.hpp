#pragma once

#include "exceptions.hpp"
#include "defs.h"
#include "ast.hpp"
#include <vector>
#include <string>

namespace crl {
	std::vector<Token> tokenize(std::string);
	Ast *generate_ast(std::vector<Token>);
	void semantic_check(Ast*);
	void generate_cas(Ast*, std::string);
}

