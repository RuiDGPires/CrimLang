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
		if (argc != 2) throw "Learn how to use this shit dude";
		print_token_vec(crl::tokenize(argv[1]));
	}catch(const char*str){
		std::cout << str << std::endl;
	}
	return 0;
}
