#pragma once

#include <vector>
#include <string>

namespace crl {
	class Token {
		public:
		enum Type{NONE};

		std::string str;
		Type type;
	};
	std::vector<Token> tokenize(std::string);
}
