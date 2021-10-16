#include "compiler.hpp"
#include <iostream>
#include <fstream>

static bool is_number(char c){
	return c >= '0' && c <= '9';
}
	
static bool is_letter(char c){
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

static bool is_symbol(char c){
	return c == '(' || c == ')' || c == '{' || c == '}' || c == ',' || c == ';' \
						|| c == '=' || c == '+' || c == '-' || c == '*' || c == '/' || c == '$' \
						|| c == '#' || c == '<' || c == '>' || c =='&' || c == '%' || c == '|';
}


static bool is_whitespace(char c){
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}


class Tracker{
	private:
		enum PState{NONE, IDENT, NUM, STRING, CHAR, SYMBOL, COMMENT, PREPROC};
		crl::Token current_token;
		u32 line = 1, column = 0;
		PState state = NONE;
		bool str_spec = false; // AUX VARIABLE FOR STRING PARSING
		int preproc_line = 0;	// 0 if undifined, -1 if not, 1 if yes

		void set_pos();
		void set_type(crl::Token::Type);
		void push_char(char c);
		bool parse_symbol(std::string);
		void check_keyword();
		std::vector<crl::Token> result;
	public:
		void take(char);
		void dump();
		void dump_eof();

		std::vector<crl::Token> get_result();
		Tracker(){}
		~Tracker(){}
};

void Tracker::set_pos(){
	this->current_token.line = this->line;
	this->current_token.column = this->column;
}

void Tracker::push_char(char c){
	this->current_token.str.push_back(c);
}

void Tracker::set_type(crl::Token::Type t){
	this->current_token.type = t;
}

bool Tracker::parse_symbol(std::string str){
	if (str.compare("*") == 0)
		this->current_token.type = crl::Token::Type::TIMES;
	else if (str.compare("+") == 0)
		this->current_token.type = crl::Token::Type::PLUS;
	else if (str.compare("++") == 0)
		this->current_token.type = crl::Token::Type::INC;
	else if (str.compare("-") == 0)
		this->current_token.type = crl::Token::Type::MINUS;
	else if (str.compare("--") == 0)
		this->current_token.type = crl::Token::Type::DECR;
	else if (str.compare("/") == 0)
		this->current_token.type = crl::Token::Type::SLASH;
	else if (str.compare(".") == 0)
		this->current_token.type = crl::Token::Type::DOT;
	else if (str.compare("(") == 0)
		this->current_token.type = crl::Token::Type::LPAREN;
	else if (str.compare(")") == 0)
		this->current_token.type = crl::Token::Type::RPAREN;
	else if (str.compare("{") == 0)
		this->current_token.type = crl::Token::Type::LBRACK;
	else if (str.compare("}") == 0)
		this->current_token.type = crl::Token::Type::RBRACK;
	else if (str.compare("$") == 0)
		this->current_token.type = crl::Token::Type::DOLLAR;
	else if (str.compare("&") == 0)
		this->current_token.type = crl::Token::Type::AND;
	else if (str.compare("&&") == 0)
		this->current_token.type = crl::Token::Type::LAND;
	else if (str.compare("|") == 0)
		this->current_token.type = crl::Token::Type::OR;
	else if (str.compare("||") == 0)
		this->current_token.type = crl::Token::Type::LOR;
	else if (str.compare(":") == 0)
		this->current_token.type = crl::Token::Type::CLN;
	else if (str.compare(";") == 0)
		this->current_token.type = crl::Token::Type::SEMICLN;
	else if (str.compare(",") == 0)
		this->current_token.type = crl::Token::Type::COMMA;
	else if (str.compare("=") == 0)
		this->current_token.type = crl::Token::Type::BECOMES;
	else if (str.compare("+=") == 0)
		this->current_token.type = crl::Token::Type::PLEQ;
	else if (str.compare("-=") == 0)
		this->current_token.type = crl::Token::Type::MIEQ;
	else if (str.compare("*=") == 0)
		this->current_token.type = crl::Token::Type::TIEQ;
	else if (str.compare("/=") == 0)
		this->current_token.type = crl::Token::Type::SLEQ;
	else if (str.compare("==") == 0)
		this->current_token.type = crl::Token::Type::EQL;
	else if (str.compare(">=") == 0)
		this->current_token.type = crl::Token::Type::GEQ;
	else if (str.compare("<=") == 0)
		this->current_token.type = crl::Token::Type::LEQ;
	else if (str.compare(">") == 0)
		this->current_token.type = crl::Token::Type::GTR;
	else if (str.compare("<") == 0)
		this->current_token.type = crl::Token::Type::LSS;
	else if (str.compare("!=") == 0)
		this->current_token.type = crl::Token::Type::NEQ;
	else if (str.compare("//") == 0){
		this->current_token.type = crl::Token::Type::NONE;
		this->current_token.str = ""; 
		this->state = PState::COMMENT;
	}
	else return 0;

	return 1;
}

void Tracker::check_keyword(){
	if (this->current_token.str.compare("if") == 0)
		this->current_token.type = crl::Token::Type::IF;
	else if (this->current_token.str.compare("else") == 0)
		this->current_token.type = crl::Token::Type::ELSE;
	else if (this->current_token.str.compare("for") == 0)
		this->current_token.type = crl::Token::Type::FOR;
	else if (this->current_token.str.compare("while") == 0)
		this->current_token.type = crl::Token::Type::WHILE;
	else if (this->current_token.str.compare("mut") == 0)
		this->current_token.type = crl::Token::Type::MUT;
	else if (this->current_token.str.compare("void") == 0)
		this->current_token.type = crl::Token::Type::VOID;
	else if (this->current_token.str.compare("char") == 0)
		this->current_token.type = crl::Token::Type::TCHAR;
	else if (this->current_token.str.compare("string") == 0)
		this->current_token.type = crl::Token::Type::TSTR;
	else if (this->current_token.str.compare("i32") == 0)
		this->current_token.type = crl::Token::Type::I32;
	else if (this->current_token.str.compare("u32") == 0)
		this->current_token.type = crl::Token::Type::U32;
	else if (this->current_token.str.compare("f32") == 0)
		this->current_token.type = crl::Token::Type::F32;
	else if (this->current_token.str.compare("f64") == 0)
		this->current_token.type = crl::Token::Type::F64;
	else if (this->current_token.str.compare("cas") == 0)
		this->current_token.type = crl::Token::Type::CAS;
	else if (this->current_token.str.compare("as") == 0)
		this->current_token.type = crl::Token::Type::AS;
	else if (this->current_token.str.compare("return") == 0)
		this->current_token.type = crl::Token::Type::RETURN;
	else if (this->current_token.str.compare("break") == 0)
		this->current_token.type = crl::Token::Type::BREAK;
	else if (this->current_token.str.compare("continue") == 0)
		this->current_token.type = crl::Token::Type::CONTINUE;
}

void Tracker::dump(){
	this->result.push_back(current_token);
	this->state = PState::NONE;
	current_token = crl::Token();
}

void Tracker::dump_eof(){
	current_token.type = crl::Token::Type::EOF_;
	current_token.str = '\0';
	this->dump();
}

void Tracker::take(char c){
	if (c == '\n'){
		if (this->preproc_line == 1){
			this->current_token.type = crl::Token::Type::PREPROC_END;
			this->current_token.str = "</PP>";
			this->dump();
		}
		this->preproc_line = 0;
		if (state == PState::COMMENT) state = PState::NONE;
		this->line++;
		this->column = 0;
	}

	this->column++;

	eval:
	switch(this->state){
		case PState::NONE:
			if (c == '#'){
				if (this->preproc_line == 0){
					this->preproc_line = 1;			
					this->set_pos();
					this->current_token.type = crl::Token::Type::PREPROC_START;
					this->current_token.str = "<PP>";
					this->dump();
				}else
					throw crl::SyntaxError(line, column, "Unexpected #");
			}else if (is_letter(c)){
				if (!this->preproc_line) this->preproc_line = -1;
				state = PState::IDENT;
				set_type(crl::Token::Type::IDENT);
				this->push_char(c);
				this->set_pos();
			}else if (is_number(c)){
				if (!this->preproc_line) this->preproc_line = -1;
				state = PState::NUM;
				set_type(crl::Token::Type::INT);
				this->push_char(c);
				this->set_pos();
			}else if (is_symbol(c)){
				if (!this->preproc_line) this->preproc_line = -1;
				state = PState::SYMBOL;
				this->set_pos();
				this->push_char(c);
			}else if (c == '"'){
				if (!this->preproc_line) this->preproc_line = -1;
				state = PState::STRING;
				set_type(crl::Token::Type::STRING);
				this->set_pos();
			}else if (c == '\''){
				if (!this->preproc_line) this->preproc_line = -1;
				state = PState::CHAR;
				set_type(crl::Token::Type::CHAR);
				this->set_pos();
			}else if (is_whitespace(c))
				break;
			else throw crl::SyntaxError(line, column, "Unkown character: " + std::string(1, c));
			break;
	
		case PState::IDENT:
			if (is_letter(c) || is_number(c) || c == '_')
				this->push_char(c);
			else {
				this->check_keyword();
				this->dump();
				goto eval;
			}
			break;

		case PState::NUM:
			if (is_number(c))
				this->push_char(c);
			else if (c == '.'){
				if (this->current_token.type == crl::Token::Type::INT){
					this->current_token.type = crl::Token::Type::DEC; 
					this->push_char(c);	
				}else throw crl::SyntaxError(line, column, "Invalid numberic format"); 
			} else {
				this->dump();	
				goto eval;
			}
			break;

		case PState::SYMBOL:
			if (is_symbol(c)){
				if (this->parse_symbol(this->current_token.str + c)){
					if (this->state == PState::COMMENT) break;
					this->push_char(c);
				}else{
					if (this->parse_symbol(this->current_token.str)){
						if (this->state == PState::COMMENT) break;
						this->dump();
						goto eval;
					}else throw crl::SyntaxError(line, column, "Unkown symbol: " + this->current_token.str); 
				}
			}else if (this->parse_symbol(this->current_token.str)){
				if (this->state == PState::COMMENT) break;
				this->dump();
				goto eval;
			}else throw crl::SyntaxError(line, column, "Unkown symbol: " + this->current_token.str); 
			
			break;

		case PState::STRING:
			if (c == '\\'){
				this->push_char(c);
				this->str_spec = !this->str_spec;	 // SWITCH STR SPECIAL
			}else if (c == '"'){
				if (!this->str_spec){
					this->dump();
					break;	
				}else{
					this->current_token.str.pop_back();
					this->push_char(c);
					this->str_spec = false;
				}
			}else {
				this->push_char(c);
				this->str_spec = false;
			}
			break;
		case PState::CHAR:
			{
			size_t size = this->current_token.str.size();
			if (size == 0){
				if (c == '\'') throw crl::SyntaxError(line, column, "Empty char is not allowed"); 
				this->push_char(c);
			}else if (size == 1) {
				if (this->current_token.str[0] == '\\'){
					if (c == '\'') this->current_token.str.pop_back();
					this->push_char(c);
				}else{
					if (c != '\'') throw crl::SyntaxError(line, column, "Invalid char: " + this->current_token.str); 
					this->dump();
				}
			}else if (c != '\'') throw crl::SyntaxError(line, column, "Invalid char: " + this->current_token.str);
			else this->dump();
			}
			break;
		default:
			break;
	}
}

std::vector<crl::Token> Tracker::get_result(){
	return this->result;
}

std::vector<crl::Token> crl::tokenize(std::string filename){
	std::ifstream file;
	file.open(filename);

	Tracker tracker;	
	char c;
	while (file >> std::noskipws >> c){
		tracker.take(c);
	}
	// TODO : Is this needed?
	tracker.take(c);

	tracker.dump_eof();
	file.close();

	return tracker.get_result();
}
