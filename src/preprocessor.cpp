#include "compiler.hpp"
#include <unistd.h>

#ifdef DEBUG
#define ASSERT(v, s) if (!(v)) throw crl::AssertionError(-1, -1, s)
#else
#define ASSERT(...)
#endif

bool file_exists(std::string fileName){
	return !access(fileName.c_str(), R_OK);
}

static void include(std::vector<crl::Token> &current, crl::Pproc_lib lib, std::string other, size_t &begin, size_t &i){
	std::vector<crl::Token> other_file = crl::tokenize(other);
	preprocess(other_file, lib);

	size_t other_size = other_file.size();
	current.erase(current.begin() + begin, current.begin() + begin + 4);
	current.insert(current.begin() + begin, other_file.begin(), other_file.end() - 1);
	i += other_size-2;
}

namespace crl {
	void preprocess(std::vector<Token> &vec, Pproc_lib libs){
		size_t size = vec.size();	
		bool preproc_line = false;
		size_t begin = 0;

		for (size_t i = 0; i < size; i++){
			switch(vec[i].type){
				case Token::Type::PREPROC_START:
					preproc_line = true;
					begin = i;
				break;
				case Token::Type::PREPROC_END:
					preproc_line = false;
					vec.erase(vec.begin() + i--);
				break;
				case Token::Type::IDENT:
					if (!preproc_line) break;
					if (vec[i].str == "include"){
						ASSERT(i < size - 3, "Incomplete preprocessor directive");
						ASSERT(vec[i+2].type == Token::Type::PREPROC_END, "Expected end of preprocessor directive");
						if (vec[i+1].type == Token::Type::STRING){
							ASSERT(file_exists(vec[i+1].str), "File " + vec[i+1].str + " cannot be read");
							include(vec, libs, vec[i+1].str, begin, i);
						}else if (vec[i+1].type == Token::Type::GLOBAL_FOLDER){
							std::string other_full_path;

							for (size_t j = 0; j < libs.libc; j++)
								if (file_exists(std::string(libs.libv[j]) + "/" + vec[i+1].str)){
									other_full_path = std::string(libs.libv[j]) + "/" + vec[i+1].str;
									break;
								}
							ASSERT(other_full_path != "", "File " + vec[i+1].str + " cannot be read");

							include(vec, libs, other_full_path, begin, i);
						}else ASSERT(false, "unkown preprocessor directive");
					}
					break;
				default:
					break;
			}	
		}
	}
}
