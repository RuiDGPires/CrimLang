#include "compiler.hpp"

class _lx_Tracker{
	private:
	std::vector<crl::Token> token_list;
	crl::Ast *ast, *aux;
	int pointer = 0;
	bool eof = false;

	public:
		_lx_Tracker(){}
		_lx_Tracker(std::vector<crl::Token> &tl){
			this->token_list = tl; 
			this->aux = new crl::Node();
			this->ast = aux;
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

		void enter(crl::Node::Type);
		void add(crl::Token);
		void leave();
		crl::Ast *result();


		void factor();
		void term();
		void expression();
		void program();
		void declaration();
		void statement();
		void block(crl::Token::Type);
};


void _lx_Tracker::next(){
	// TODO
	if (this->pointer < this->token_list.size() -1)
		this->current = this->token_list[++this->pointer];
}

crl::Token &_lx_Tracker::peek(){
	return this->token_list[this->pointer + 1];
}

crl::Token &_lx_Tracker::previous(){
	return this->token_list[this->pointer - 1];
}

bool _lx_Tracker::accept(crl::Token::Type t){
	bool res = t == this->current.type;
	if (res)
		this->next();
	return res;
}

#include <stdarg.h>
bool _lx_Tracker::accept(int n, ...){
	va_list args;
	va_start(args, n);
	bool res = false;
	while(n > 0){
		int t = va_arg(args, int);
		if ((int) this->current.type == t){
			res = true;
			break;
		}
		n--;
	}
	va_end(args);
	if (res)
		this->next();
	return res;
}

void _lx_Tracker::expect(crl::Token::Type t){
	// TODO : Improve error system
	if (t != this->current.type) throw "Unexpected token: " + this->current.str;
	this->next();
}

void _lx_Tracker::expect(int n, ...){
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
	if (!res) throw "Unexpected token: " + this->current.str;
	this->next();
}

void _lx_Tracker::enter(crl::Node::Type t){
	crl::Node * node = new crl::Node(t, this->ast);
	this->ast->add_child(node);
	this->ast = node;
}

void _lx_Tracker::leave(){
	this->ast = this->ast->parent;
}

void _lx_Tracker::add(crl::Token t){
	this->ast->add_child(new crl::Leaf(t));
}

crl::Ast *_lx_Tracker::result(){
	return this->aux;
}

#define TYPE_SPEC() (7, crl::Token::Type::VOID, crl::Token::Type::CHAR, crl::Token::Type::U32, crl::Token::Type::I32, crl::Token::Type::CHAR, crl::Token::Type::F32, crl::Token::Type::F64)
#define TYPE_NUMERIC() (2, crl::Token::Type::INT, crl::Token::Type::DEC)
#define ACC_AND_ADD(type) (accept(type)) this->add(this->previous())
#define EXP_AND_ADD(type) (expect(type)); this->add(this->previous())
#define ACC_AND_ADD_VAR(types) (accept types) this->add(this->previous())
#define EXP_AND_ADD_VAR(types) expect types; this->add(this->previous())

void _lx_Tracker::factor(){
	this->enter(crl::Node::Type::FACTOR);
	if ACC_AND_ADD(crl::Token::Type::IDENT);
	else if ACC_AND_ADD_VAR(TYPE_NUMERIC());
	else if (accept(crl::Token::Type::LPAREN)){
		this->expression();
		this->expect(crl::Token::Type::RPAREN);
	}else{
		throw std::string("Syntax Error");
	}
	this->leave();
}



void _lx_Tracker::term(){
	this->enter(crl::Node::Type::TERM);
	this->factor();
	while(this->accept(2, crl::Token::Type::TIMES, crl::Token::Type::SLASH)){
		this->add(this->previous());
		this->factor();
	}
	this->leave();
}

void _lx_Tracker::expression(){
	this->enter(crl::Node::Type::EXPRESSION);
	if ACC_AND_ADD_VAR((2, crl::Token::Type::PLUS, crl::Token::Type::MINUS));
	this->term();
	while(accept(2, crl::Token::Type::PLUS, crl::Token::Type::MINUS)){
		this->add(this->previous());
		this->term();
	}
	this->leave();
}

void _lx_Tracker::program(){
	this->enter(crl::Node::Type::PROGRAM);
	while(!accept(crl::Token::Type::EOF_)){
		if (accept TYPE_SPEC()) 
			this->declaration();
	}
	
	this->leave();
}

void _lx_Tracker::block(crl::Token::Type t){
	while(!accept(t))
		statement();		
}

void _lx_Tracker::statement(){
	EXP_AND_ADD_VAR(TYPE_NUMERIC());
}

void _lx_Tracker::declaration(){
	this->enter(crl::Node::Type::DECLARATION);
	this->add(this->previous()); // TYPE SPEC
	// Pointer?
	while ACC_AND_ADD(crl::Token::Type::TIMES);

	bool _mutable = accept(crl::Token::Type::MUT);
	if (_mutable)
		this->add(this->previous());
	EXP_AND_ADD(crl::Token::Type::IDENT);	
	// Function declaration
	if (accept(crl::Token::Type::LPAREN)){
		this->enter(crl::Node::Type::FUNC);
		this->enter(crl::Node::Type::FUNCARGS);
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
		this->leave();
		expect(crl::Token::Type::RPAREN);
		expect(crl::Token::Type::LBRACK);
	
		this->block(crl::Token::Type::RBRACK);
		this->leave();
	}else{
		if (!_mutable){
			EXP_AND_ADD(crl::Token::Type::BECOMES);
			this->expression();
		}else{
			if (accept(crl::Token::Type::BECOMES)){
				this->add(this->previous());
				this->expression();
			}
		}
		
		expect(crl::Token::Type::SEMICLN);
	}
	this->leave();
}

crl::Ast *crl::generate_ast(std::vector<Token> t){
	_lx_Tracker tracker(t);
	tracker.program();

	return tracker.result(); 
}
