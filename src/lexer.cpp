#include "compiler.hpp"

class _lx_Tracker{
	private:
	std::vector<crl::Token> token_list;
	crl::Ast *ast, *aux;
	long unsigned int pointer = 0;
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
		void func_call();
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
	if (t != this->current.type) throw std::string("Unexpected token: " + this->current.str);
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
		n--;
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
#define TYPE_NUMERIC() (3, crl::Token::Type::INT, crl::Token::Type::DEC, crl::Token::Type::CHAR)
#define ACC_AND_ADD(type) (accept(type)) this->add(this->previous())
#define EXP_AND_ADD(type) (expect(type)); this->add(this->previous())
#define ACC_AND_ADD_VAR(types) (accept types) this->add(this->previous())
#define EXP_AND_ADD_VAR(types) expect types; this->add(this->previous())


void _lx_Tracker::func_call(){
	this->enter(crl::Node::Type::CALL);
	this->add(this->previous());
	this->expect(crl::Token::Type::LPAREN);
	this->enter(crl::Node::Type::FUNCARGS);	
	if (this->current.type != crl::Token::Type::RPAREN){
		do{
			this->expression();
		}while(this->accept(crl::Token::Type::COMMA));
		expect(crl::Token::Type::RPAREN);
	}else{
		this->next();
	}
	this->leave();
	this->leave();
}
void _lx_Tracker::factor(){
	this->enter(crl::Node::Type::FACTOR);
	if (accept(crl::Token::Type::IDENT)){
		if (this->current.type == crl::Token::Type::LPAREN){
			this->func_call();
		}else
			this->add(this->previous());
	} 
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
		this->declaration();
	}
	
	this->leave();
}

void _lx_Tracker::block(crl::Token::Type t){
	this->enter(crl::Node::Type::BLOCK);
	while(!accept(t))
		statement();		
	this->leave();
}

void _lx_Tracker::statement(){
	this->enter(crl::Node::Type::STATEMENT);

	if (accept(crl::Token::Type::IF)){
		this->enter(crl::Node::Type::IF);
		this->expect(crl::Token::Type::LPAREN);
		this->expression();	
		this->expect(crl::Token::Type::RPAREN);

		if (accept(crl::Token::Type::LBRACK))
			this->block(crl::Token::Type::RBRACK);
		else{
			this->statement();
		}
		this->leave();
	}else if (accept(crl::Token::Type::ELSE)){
		this->enter(crl::Node::Type::ELSE);
		if (accept(crl::Token::Type::LBRACK))
			this->block(crl::Token::Type::RBRACK);
		else{
			this->statement();
		}
		this->leave();
	}else if (accept(crl::Token::Type::WHILE)){
		this->enter(crl::Node::Type::WHILE);
		this->expect(crl::Token::Type::LPAREN);
		this->expression();	
		this->expect(crl::Token::Type::RPAREN);

		if (accept(crl::Token::Type::LBRACK))
			this->block(crl::Token::Type::RBRACK);
		else
			this->statement();
		this->leave();
	}else if (accept(crl::Token::Type::FOR)){
		this->enter(crl::Node::Type::FOR);
		this->expect(crl::Token::Type::LPAREN);
		this->statement();	
		if (this->previous().type != crl::Token::Type::SEMICLN)
			this->expect(crl::Token::Type::SEMICLN);

		this->expression();
		this->expect(crl::Token::Type::SEMICLN);
		this->expression();
		this->expect(crl::Token::Type::RPAREN);

		if (accept(crl::Token::Type::LBRACK))
			this->block(crl::Token::Type::RBRACK);
		else
			this->statement();
		this->leave();
		this->leave();
	}else if (accept(crl::Token::Type::RETURN)){
		this->enter(crl::Node::Type::RETURN);
		this->expression();
		this->expect(crl::Token::Type::SEMICLN);
		this->leave();
	}else if (accept(crl::Token::Type::IDENT)){
		if (this->current.type == crl::Token::Type::LPAREN){
				this->func_call();
		}else{
			this->enter(crl::Node::Type::ASSIGN);
			this->add(this->previous());
			this->expect(crl::Token::Type::BECOMES);
			if (accept(crl::Token::Type::STRING))
				this->add(this->previous());
			else
				this->expression();
			this->expect(crl::Token::Type::SEMICLN);
			this->leave();
		}
	}else if (accept TYPE_SPEC()){
		this->enter(crl::Node::Type::INIT);
		this->add(this->previous());
		// VAR INIT
		while ACC_AND_ADD(crl::Token::Type::TIMES);

		bool _mutable = accept(crl::Token::Type::MUT);
		if (_mutable)
			this->add(this->previous());

		EXP_AND_ADD(crl::Token::Type::IDENT);

		if (!_mutable){
			EXP_AND_ADD(crl::Token::Type::BECOMES);
			if (accept(crl::Token::Type::STRING))
				this->add(this->previous());
			else
				this->expression();
		}else{
			if (accept(crl::Token::Type::BECOMES)){
				this->add(this->previous());
				if (accept(crl::Token::Type::STRING))
					this->add(this->previous());
				else
					this->expression();
			}
		}
		
		expect(crl::Token::Type::SEMICLN);
		this->leave();
	}else
		throw std::string("Unexpected token: " + this->current.str);
	
	this->leave();
}

void _lx_Tracker::declaration(){
	if (accept(crl::Token::Type::CAS)){
		this->enter(crl::Node::Type::CAS);
		EXP_AND_ADD(crl::Token::Type::IDENT);
		this->expect(crl::Token::Type::LBRACK);
		while(accept(crl::Token::Type::STRING)){
			this->add(this->previous());
			this->expect(crl::Token::Type::SEMICLN);
		}
		this->expect(crl::Token::Type::RBRACK);
	}else{	
		this->enter(crl::Node::Type::DECLARATION);
		EXP_AND_ADD_VAR(TYPE_SPEC());
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
					this->enter(crl::Node::Type::ASSIGN);
					if (accept(crl::Token::Type::STRING))
						this->add(this->previous());
					else
						this->expression();
					this->leave();
				}
			}
			
			expect(crl::Token::Type::SEMICLN);
		}
	}
	this->leave();
}

crl::Ast *crl::generate_ast(std::vector<Token> t){
	_lx_Tracker tracker(t);
	tracker.program();

	return tracker.result(); 
}
