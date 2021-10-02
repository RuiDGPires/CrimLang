#pragma once

#include "token.hpp"
#include <string>
#include <vector>

namespace crl{
	class Node{
		private:
			std::vector<Node *>  children;
		public:
			enum Type{NONE, PROGRAM, DECLARATION, INIT, BLOCK, STATEMENT, ASSIGN, FUNC, IF, ELSE, FOR, WHILE, CALL, FUNCARGS,EXPRESSION, TERM, FACTOR, LEAF, TYPE, RETURN};

			Type type;

			Node *parent;

			Node();
			Node(Type, Node *);
			virtual ~Node();
			virtual void add_child(Node *);
			virtual Node *get_child(int);
			virtual Node *operator[](int);
			virtual std::string to_string() const;
			virtual std::string to_string(int depth) const;
	};
	
	class Leaf : public Node {
		public:
			Token token;
			Leaf(Token);
			~Leaf();
			std::string to_string(int) const;
	};


	typedef Node Ast;
}

