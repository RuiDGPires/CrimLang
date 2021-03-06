#pragma once

#include "token.hpp"
#include <string>
#include <vector>

namespace crl{
	class Node{
		private:
		public:
			enum Type{NONE, PROGRAM, VAR_DECLARATION, FUNC_DECLARATION, INIT, BLOCK, ASSIGN, IF, THEN, ELSE, FOR, WHILE, CALL, EXPRESSION, TERM, FACTOR, LEAF, CAS, ARG, INC, DEC, CAST, RETURN, BREAK, COMPARISON, VECTOR};

			Type type;

			std::vector<Node *>  children;
			Node *parent;
			
			std::string annotation;
			u32 argc; // Used if function

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

