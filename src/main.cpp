#include "compiler.hpp"
#include <iostream>

static void print_token_vec(std::vector<crl::Token> vec){
	size_t size = vec.size();

	for (size_t i = 0; i < size; i++){
		std::cout << vec[i] << std::endl;
	}
}

#define PROGRAM_ENTRY_
int main(int argc, char *argv[]){
	try{
		//print_token_vec(crl::tokenize(argv[1]));
		crl::Ast *ast = crl::generate_ast(crl::tokenize(argv[1]));
		std::cout << ast->to_string();
		delete ast;
	}catch(std::string &e){
		std::cout << e << std::endl;
	}
	return 0;
}
