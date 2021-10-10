#include "compiler.hpp"

class _ps_Tracker{
	private:
	std::vector<crl::Token> token_list;
	crl::Ast *ast, *aux;
	long unsigned int pointer = 0;
	bool eof = false;

	public:
		_ps_Tracker(){}
		_ps_Tracker(std::vector<crl::Token> &tl){
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
		void annotate(std::string);
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


void _ps_Tracker::next(){
	// TODO
	if (this->pointer < this->token_list.size() -1)
		this->current = this->token_list[++this->pointer];
}

crl::Token &_ps_Tracker::peek(){
	return this->token_list[this->pointer + 1];
}

crl::Token &_ps_Tracker::previous(){
	return this->token_list[this->pointer - 1];
}

bool _ps_Tracker::accept(crl::Token::Type t){
	bool res = t == this->current.type;
	if (res)
		this->next();
	return res;
}

#include <stdarg.h>
bool _ps_Tracker::accept(int n, ...){
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

void _ps_Tracker::expect(crl::Token::Type t){
	if (t != this->current.type) throw crl::UnexpectedToken(this->current);
	this->next();
}

void _ps_Tracker::expect(int n, ...){
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
	if (!res) throw crl::UnexpectedToken(this->current); 
	this->next();
}

void _ps_Tracker::enter(crl::Node::Type t){
	crl::Node * node = new crl::Node(t, this->ast);
	this->ast->add_child(node);
	this->ast = node;
}

void _ps_Tracker::leave(){
	this->ast = this->ast->parent;
}

void _ps_Tracker::annotate(std::string s){
	this->ast->annotation = s;
}

void _ps_Tracker::add(crl::Token t){
	this->ast->add_child(new crl::Leaf(t));
}

crl::Ast *_ps_Tracker::result(){
	return this->aux;
}

#define TYPE_SPEC() (7, crl::Token::Type::VOID, crl::Token::Type::TCHAR, crl::Token::Type::TSTR, crl::Token::Type::U32, crl::Token::Type::I32, crl::Token::Type::F32, crl::Token::Type::F64)
#define TYPE_NUMERIC() (3, crl::Token::Type::INT, crl::Token::Type::DEC, crl::Token::Type::CHAR)
#define ACC_AND_ADD(type) (accept(type)) this->add(this->previous())
#define EXP_AND_ADD(type) (expect(type)); this->add(this->previous())
#define ACC_AND_ADD_VAR(types) (accept types) this->add(this->previous())
#define EXP_AND_ADD_VAR(types) expect types; this->add(this->previous())


void _ps_Tracker::func_call(){
	this->enter(crl::Node::Type::CALL);
	this->add(this->previous());
	this->expect(crl::Token::Type::LPAREN);
	if (this->current.type != crl::Token::Type::RPAREN){
		do{
			this->enter(crl::Node::Type::ARG);
			if (this->accept(crl::Token::Type::AND)){
				this->annotate("ref");
				this->expect(crl::Token::IDENT);
				this->add(this->previous());
			} else
				this->expression();
			this->leave();
		}while(this->accept(crl::Token::Type::COMMA));
		expect(crl::Token::Type::RPAREN);
	}else{
		this->next();
	}
	this->leave();
}
void _ps_Tracker::factor(){
	this->enter(crl::Node::Type::FACTOR);
	if (accept(crl::Token::Type::IDENT)){
		if (this->current.type == crl::Token::Type::LPAREN){
			this->func_call();
		}else
			this->add(this->previous());
	} 
	else if ACC_AND_ADD(crl::Token::Type::STRING);
	else if ACC_AND_ADD_VAR(TYPE_NUMERIC());
	else if (accept(crl::Token::Type::LPAREN)){
		this->expression();
		this->expect(crl::Token::Type::RPAREN);
	}else{
		throw crl::UnexpectedToken(this->current);
	}
	this->leave();
}

void _ps_Tracker::term(){
	this->enter(crl::Node::Type::TERM);
	this->factor();
	while(this->accept(2, crl::Token::Type::TIMES, crl::Token::Type::SLASH)){
		this->add(this->previous());
		this->factor();
	}
	this->leave();
}

void _ps_Tracker::expression(){
	this->enter(crl::Node::Type::EXPRESSION);
	if ACC_AND_ADD_VAR((2, crl::Token::Type::PLUS, crl::Token::Type::MINUS));
	this->term();
	while(accept(2, crl::Token::Type::PLUS, crl::Token::Type::MINUS)){
		this->add(this->previous());
		this->term();
	}
	this->leave();
}

void _ps_Tracker::program(){
	this->enter(crl::Node::Type::PROGRAM);
	while(!accept(crl::Token::Type::EOF_)){
		this->declaration();
	}
	
	this->leave();
}

void _ps_Tracker::block(crl::Token::Type t){
	this->enter(crl::Node::Type::BLOCK);
	while(!accept(t))
		statement();		
	this->leave();
}

void _ps_Tracker::statement(){
	if (accept(crl::Token::Type::SEMICLN))
		this->enter(crl::Node::Type::NONE);
	else if (accept(crl::Token::Type::IF)){
		this->enter(crl::Node::Type::IF);

		this->expect(crl::Token::Type::LPAREN);
		this->expression();	
		this->expect(crl::Token::Type::RPAREN);

		this->enter(crl::Node::Type::THEN);
		if (accept(crl::Token::Type::LBRACK))
			this->block(crl::Token::Type::RBRACK);
		else
			this->statement();
		
		this->leave(); // LEAVE THEN

		if (accept(crl::Token::Type::ELSE)){
			this->enter(crl::Node::Type::ELSE);
			if (accept(crl::Token::Type::LBRACK))
				this->block(crl::Token::Type::RBRACK);
			else
				this->statement();
			this->leave();
		}
	}else if (accept(crl::Token::Type::WHILE)){
		this->enter(crl::Node::Type::WHILE);
		this->expect(crl::Token::Type::LPAREN);
		this->expression();	
		this->expect(crl::Token::Type::RPAREN);

		if (accept(crl::Token::Type::LBRACK))
			this->block(crl::Token::Type::RBRACK);
		else
			this->statement();
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
	}else if (accept(crl::Token::Type::RETURN)){
		this->enter(crl::Node::Type::RETURN);
		if (!accept(crl::Token::Type::SEMICLN)){
			this->expression();
			this->expect(crl::Token::Type::SEMICLN);
		}
	}else if (accept(crl::Token::Type::IDENT)){
		if (this->current.type == crl::Token::Type::LPAREN){
				this->func_call();
				this->expect(crl::Token::Type::SEMICLN);
		}else{
			this->enter(crl::Node::Type::ASSIGN);
			this->add(this->previous());
			this->expect(crl::Token::Type::BECOMES);
			this->expression();
			this->expect(crl::Token::Type::SEMICLN);
		}
	}else if (accept TYPE_SPEC()){
		this->enter(crl::Node::Type::INIT);
		this->add(this->previous());
		// VAR INIT

		bool _mutable = accept(crl::Token::Type::MUT);
		if (_mutable)
			this->annotate("mut");

		EXP_AND_ADD(crl::Token::Type::IDENT);

		if (!_mutable){
			this->enter(crl::Node::Type::ASSIGN);
			expect(crl::Token::Type::BECOMES);
			this->expression();
			this->leave();
		}else{
			if (accept(crl::Token::Type::BECOMES)){
				this->enter(crl::Node::Type::ASSIGN);
				this->expression();
				this->leave();
			}
		}
		
		expect(crl::Token::Type::SEMICLN);
	}else
		throw crl::UnexpectedToken(this->current);
	
	this->leave();
}

void _ps_Tracker::declaration(){
	// aux will be used to stor tokens until it's know if this is a declaration of a function or a variable
	std::vector<crl::Token> aux;
	bool is_cas = false;
	bool is_void = false;

	if (accept(crl::Token::Type::VOID))
		is_void = true;
	else		
		expect TYPE_SPEC();
	
	aux.push_back(this->previous());

	if (accept(crl::Token::Type::CAS)) 
		is_cas=true;


	bool _mutable = accept(crl::Token::Type::MUT);
	
	expect(crl::Token::Type::IDENT);
	aux.push_back(this->previous());

	size_t size = aux.size();
	if (_mutable)
		goto var_;
	// Function declaration
	if (accept(crl::Token::Type::LPAREN)){
		if (!is_cas)
			this->enter(crl::Node::Type::FUNC_DECLARATION);
		else
			this->enter(crl::Node::Type::CAS);

		// Dump stored tokens
		for (size_t i = 0; i < size; i++)
			this->add(aux[i]);

		if (accept TYPE_SPEC()){
			this->enter(crl::Node::Type::ARG);
			this->add(this->previous());

			if (accept(crl::Token::Type::MUT))
				this->annotate("mut");

			EXP_AND_ADD(crl::Token::Type::IDENT);

			this->leave();
			while(accept(crl::Token::Type::COMMA)){
				this->enter(crl::Node::Type::ARG);
				EXP_AND_ADD_VAR(TYPE_SPEC());

				if (accept(crl::Token::Type::MUT))
					this->annotate("mut");

				EXP_AND_ADD(crl::Token::Type::IDENT);
				this->leave();
			}
		}
		expect(crl::Token::Type::RPAREN);
		expect(crl::Token::Type::LBRACK);
		if (!is_cas)	
			this->block(crl::Token::Type::RBRACK);
		else{
			this->enter(crl::Node::Type::BLOCK);
			while(accept(crl::Token::Type::STRING)){
				this->add(this->previous());
				this->expect(crl::Token::Type::SEMICLN);
			}
			this->expect(crl::Token::Type::RBRACK);
			this->leave();
		}
	}else{
var_:
		if (is_void) throw crl::SyntaxError(this->current.line, this->current.column, "Variables can't have void type");
		this->enter(crl::Node::Type::VAR_DECLARATION);
		//Dump stored tokens
		for (size_t i = 0; i < size; i++)
			this->add(aux[i]);

		if (!_mutable){
			this->expect(crl::Token::Type::BECOMES);
			this->enter(crl::Node::Type::ASSIGN);
			this->expression();
			this->leave();
		}else{
			this->annotate("mut");
			if (accept(crl::Token::Type::BECOMES)){
				this->enter(crl::Node::Type::ASSIGN);
				this->expression();
				this->leave();
			}
		}
		
		expect(crl::Token::Type::SEMICLN);
	}
	
	this->leave();
}

crl::Ast *crl::generate_ast(std::vector<Token> t){
	_ps_Tracker tracker(t);
	tracker.program();

	return tracker.result(); 
}
