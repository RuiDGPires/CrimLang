#pragma once

#include "token.hpp"
#include <string>
#include <vector>

namespace crl{
	class Node{
		private:
			std::vector<Node*>  children;
		public:
			enum Type{PROGRAM, INIT, BLOCK, STATEMENT, ASSIGN, FUNC, IF, ELSE, CALL, FUNCARGS,EXPRESSION, TERM, FACTOR, LEAF, TYPE, };

			Type type;
			crl::Token token;

			Node *parent;

			Node();
			virtual ~Node();
			virtual void deleteNode();
			virtual void addChild(Node *);
			virtual Node *getChild(int);
			virtual Node *operator[](int);
			virtual std::string toString(int depth) const;
	};

	class Ast{
		private:
		public:
			Ast();
			~Ast();
			std::string toString(int) const;
	};
}


