#include "compiler.hpp"

class Tracker{
	private:
	std::vector<crl::Token> token_list;
	crl::Ast ast;
	int pointer = 0;

	public:
		Tracker(){}
		Tracker(std::vector<crl::Token> &tl){
			this->token_list = tl; 
			this->current = tl[0];
			this->ast_enter(crl::Node::Type::PROGRAM);
		}
		~Tracker(){}

		crl::Token current;
		void next();
		crl::Token &peek();
		bool accept(crl::Token::Type);
		void expect(crl::Token::Type);

		void ast_enter(crl::Node::Type);
		void ast_add(crl::Token);
		void ast_exit();
};


void Tracker::next(){
	this->current = this->token_list[++this->pointer];
}

crl::Token &Tracker::peek(){
	return this->token_list[this->pointer + 1];
}

bool Tracker::accept(crl::Token::Type t){
	bool res = t == this->current.type;
	if (res)
		this->next();
	return res;
}

void Tracker::expect(crl::Token::Type t){
	// TODO : Improve error system
	if (t != this->current.type) throw "Unexpected token";
	next();
}

void Tracker::ast_enter(crl::Node::Type t){
	crl::Node * node = new crl::Node(t, this->ast);
	this->ast->add_child(node);
	this->ast = node;
}

void Tracker::ast_exit(){
	this->ast = this->ast->parent;
}

void Tracker::ast_add(crl::Token t){
	this->ast->add_child(new crl::Leaf(t));
}

crl::Ast crl::generate_ast(std::vector<Token>){

	return Ast();
}
