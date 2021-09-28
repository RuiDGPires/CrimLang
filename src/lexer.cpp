#include "compiler.hpp"

class Tracker{
	private:
	std::vector<crl::Token> token_list;
	crl::Ast *ast, aux;
	int pointer = 0;
	bool eof = false;

	public:
		Tracker(){}
		Tracker(std::vector<crl::Token> &tl){
			this->token_list = tl; 
			this->ast = &aux;
			this->current = tl[0];
		}

		crl::Token current;
		void next();
		crl::Token &peek();
		crl::Token &previous();
		bool accept(crl::Token::Type);
		bool accept(int, ...);
		void expect(crl::Token::Type);
		void expect(int, ...);

		void ast_enter(crl::Node::Type);
		void ast_add(crl::Token);
		void ast_exit();
		crl::Ast result();


		void program();
		void declaration();
		void statement();
		void block(crl::Token::Type);
};


void Tracker::next(){
	// TODO
	if (this->eof) throw "End of file reached";
	this->current = this->token_list[++this->pointer];
	if (this->pointer == this->token_list.size() - 1) this->eof = true;
}

crl::Token &Tracker::peek(){
	return this->token_list[this->pointer + 1];
}

crl::Token &Tracker::previous(){
	return this->token_list[this->pointer - 1];
}

bool Tracker::accept(crl::Token::Type t){
	bool res = t == this->current.type;
	if (res)
		this->next();
	return res;
}

#include <stdarg.h>
bool Tracker::accept(int n, ...){
	va_list args;
	va_start(args, n);
	bool res = false;
	while(n > 0){
		int t = va_arg(args, int);
		if ((int) this->current.type == t){
			res = true;
			break;
		}
	}
	va_end(args);
	if (res)
		this->next();
	return res;
}

void Tracker::expect(crl::Token::Type t){
	// TODO : Improve error system
	if (t != this->current.type) throw "Unexpected token";
	this->next();
}

void Tracker::expect(int n, ...){
	va_list args;
	va_start(args, n);
	bool res = false;
	while(n > 0){
		int t = va_arg(args, int);
		if ((int) this->current.type == t){
			res = true;
			break;
		}
	}
	va_end(args);
	if (!res) throw "Unexpected Token";
	this->next();
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

crl::Ast Tracker::result(){
	return this->aux;
}

#define TYPE_SPEC() (5, crl::Token::Type::CHAR, crl::Token::Type::I32, crl::Token::Type::CHAR, crl::Token::Type::F32, crl::Token::Type::F64)
#define ACC_AND_ADD(type) (accept(type)) this->ast_add(this->previous())
#define EXP_AND_ADD(type) (expect(type)); this->ast_add(this->previous())
#define ACC_AND_ADD_VAR(types) (accept types this->ast_add(this->previous())
#define EXP_AND_ADD_VAR(types) expect types; this->ast_add(this->previous())

void Tracker::program(){
	this->ast_enter(crl::Node::Type::PROGRAM);
	if (accept TYPE_SPEC()) 
		this->declaration();
	
	this->ast_exit();
}

void Tracker::block(crl::Token::Type t){
	while(!accept(t))
		statement();		
}

void Tracker::statement(){
	
}

void Tracker::declaration(){
	this->ast_enter(crl::Node::Type::DECLARATION);
	this->ast_add(this->previous());
	// Pointer?
	while ACC_AND_ADD(crl::Token::Type::TIMES);
	
	// Function declaration
	if (accept(crl::Token::Type::LPAREN)){
		if (accept TYPE_SPEC()){
			while ACC_AND_ADD(crl::Token::Type::TIMES);

			EXP_AND_ADD(crl::Token::Type::IDENT);

			while(accept(crl::Token::Type::COMMA)){
				EXP_AND_ADD_VAR(TYPE_SPEC());
				while ACC_AND_ADD(crl::Token::Type::TIMES);
				if ACC_AND_ADD(crl::Token::Type::MUT);
				EXP_AND_ADD(crl::Token::Type::IDENT);
			}
		}
		expect(crl::Token::Type::RPAREN);
		expect(crl::Token::Type::LBRACK);

		this->block(crl::Token::Type::RBRACK);

	}else{
		bool _mutable = accept(crl::Token::Type::MUT);
		if (_mutable){
			this->ast_add(this->previous());
			EXP_AND_ADD(crl::Token::Type::BECOMES);
			this->statement();
		}
		EXP_AND_ADD(crl::Token::Type::IDENT);
		
		expect(crl::Token::Type::SEMICLN);
	}
	this->ast_exit();
}

crl::Ast crl::generate_ast(std::vector<Token> t){
	Tracker tracker(t);
	tracker.program();

	return tracker.result(); 
}
