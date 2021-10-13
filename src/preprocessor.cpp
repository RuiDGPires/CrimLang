#include "compiler.hpp"

#ifdef DEBUG
#define ASSERT(v, s) if (!(v)) throw crl::AssertionError(-1, -1, s)
#else
#define ASSERT(...)
#endif

namespace crl {
	void preprocess(std::vector<Token> &vec){
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
						ASSERT(vec[i+1].type == Token::Type::STRING, "Expected a string as filename");
						ASSERT(vec[i+2].type == Token::Type::PREPROC_END, "Expected end of preprocessor directive");

						std::vector<Token> other_file = tokenize(vec[i+1].str);
						size_t other_size = other_file.size();
						vec.erase(vec.begin() + begin, vec.begin() + begin + 4);
						vec.insert(vec.begin() + begin, other_file.begin(), other_file.end() - 1);
						i += other_size-2;
					}
					break;
				default:
					break;
			}	
				
		}
	}
}
