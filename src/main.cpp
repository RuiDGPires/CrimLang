#include "compiler.hpp"
#include <iostream>

static __attribute((unused)) void print_token_vec(std::vector<crl::Token> vec){
	size_t size = vec.size();

	for (size_t i = 0; i < size; i++){
		std::cout << vec[i] << std::endl;
	}
}

#define PROGRAM_ENTRY_
int main(int argc, char *argv[]){
	try{
		std::vector<crl::Token> vec = crl::tokenize(argv[1]);
		crl::preprocess(vec);
		print_token_vec(vec);
		crl::Ast *ast = crl::generate_ast(vec);
		//std::cout << ast->to_string() << std::endl;
		crl::semantic_check(ast);
		generate_cas(ast, argv[2]);
		delete ast;
	}catch(std::string &e){
		std::cout << e << std::endl;
	}catch(crl::CL_Exception &e){
		std::cout << e.what() << std::endl;
	}
	return 0;
}
