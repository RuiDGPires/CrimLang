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
	if (argc < 3) {std::cout << "Invalid number of command line arguments\n"; return 0;}

	crl::Pproc_lib p;
	p.libc = argc - 3;
	if (argc > 3)
		p.libv = &argv[3];
	else
		p.libv = NULL;
	
	
	try{
		std::vector<crl::Token> vec = crl::tokenize(argv[1]);
		crl::preprocess(vec, p);
		//print_token_vec(vec);
		crl::Ast *ast = crl::generate_ast(vec);
		//std::cout << ast->to_string() << std::endl;
		crl::semantic_check(ast);
		generate_cas(ast, argv[2]);
		delete ast;
	}catch(std::string &e){
		std::cout << e << std::endl;
		return 1;
	}catch(crl::CL_Exception &e){
		std::cout << e.what() << std::endl;
		return 1;
	}
	return 0;
}
