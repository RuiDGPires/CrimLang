#include "compiler.hpp"
#include "context.hpp"

#ifdef DEBUG
#define ASSERT(v, s) if (!(v)) throw crl::AssertionError(-1, -1, s)
#else
#define ASSERT(...)
#endif

static crl::Token get_token(crl::Node *node){
	return ((crl::Leaf *)node)->token;
}

class _sc_Tracker{
	private:
		crl::Ast *ast;
		Context *context = new Context();	
		void enter();
		void leave();
		void add(Context::Item);
		
		bool func_returns = false; // Used to check if funcions return at on all possible paths
		bool can_break = false;
	public:
		_sc_Tracker(){this->ast = NULL;}
		_sc_Tracker(crl::Ast *ast){this->ast = ast;}
		~_sc_Tracker(){
			if (this->context != NULL) delete this->context;
		};

		void check();

		void program(crl::Node *);
		void cas(crl::Node *);
		void func_declaration(crl::Node *);
		void var_declaration(crl::Node *);
		void unary(crl::Node *);

		void statement(crl::Node *, std::string);
		void block(crl::Node *, std::string);
		void func_call(crl::Node *, std::string);
		void expression(crl::Node *, std::string);

};

void _sc_Tracker::enter(){
	this->context = new Context(this->context);
}

void _sc_Tracker::leave(){
	Context *aux = this->context;
	this->context = this->context->parent;
	delete aux;
}

void _sc_Tracker::add(Context::Item item){
	ASSERT(!this->context->has_name_local(item.name), "Multiple declarations"); 
	this->context->add(item);
}

void _sc_Tracker::cas(crl::Node *node){
	Context::Item item;
	item.type = Context::Item::Type::CAS;
	item.subtype = ((crl::Leaf *)node->get_child(0))->token.str;
	item.name = ((crl::Leaf *)node->get_child(1))->token.str;

	i32 i;
	for (i = 2; node->get_child(i)->type == crl::Node::Type::ARG; i++){
		crl::Node *arg = node->get_child(i);
		std::string var_type = ((crl::Leaf *) arg->get_child(0))->token.str;

		item.args.push_back(std::pair<std::string, bool>(var_type, arg->annotation == "mut"));
	}

	node->argc = i - 2;
	this->add(item);
}

void _sc_Tracker::func_declaration(crl::Node *node){
	this->func_returns = false;
	Context::Item item;
	item.type = Context::Item::Type::FUNC;
	item.subtype = ((crl::Leaf *)node->get_child(0))->token.str;
	item.name = ((crl::Leaf *)node->get_child(1))->token.str;
	ASSERT(item.name != "_start", "_start is a reserved name");

	// Parse once to add to the args to the declaration
	int i;
	for (i = 2; node->get_child(i)->type == crl::Node::Type::ARG; i++){
		crl::Node *arg = node->get_child(i);
		std::string var_type = ((crl::Leaf *) arg->get_child(0))->token.str;

		item.args.push_back(std::pair<std::string, bool>(var_type, arg->annotation == "mut"));
	}

	node->argc = i - 2; // Store the number of arguments

	this->add(item);

	this->enter();
	// Add args to the function context
	i = 2;
	for (; node->get_child(i)->type == crl::Node::Type::ARG; i++){
		crl::Node *arg = node->get_child(i);
		Context::Item arg_item;
		arg_item.type = Context::Item::Type::VAR;
		arg_item.subtype = ((crl::Leaf *) arg->get_child(0))->token.str;
		arg_item.name = ((crl::Leaf *) arg->get_child(1))->token.str;
		arg_item._mutable = arg->annotation == "mut";
		this->add(arg_item);
	}
	this->block(node->get_child(i), item.subtype);
	
	ASSERT(func_returns, "Functions must return on all possible paths");
	this->leave();
}

void _sc_Tracker::var_declaration(crl::Node *node){
	Context::Item item;
	item.type = Context::Item::Type::VAR;
	item.subtype = ((crl::Leaf *) node->get_child(0))->token.str;
	item.name = ((crl::Leaf *) node->get_child(1))->token.str;

	if (node->annotation == "mut")
		item._mutable = true;


	if (node->children.size() == 3){
		ASSERT(node->get_child(2)->type == crl::Node::Type::ASSIGN, "Must be assign if children size is 3");
		ASSERT(node->get_child(2)->get_child(0)->type == crl::Node::Type::EXPRESSION, "Must assign an expression");
		this->expression(node->get_child(2)->get_child(0), item.subtype);
	}
	this->add(item);
}

void _sc_Tracker::statement(crl::Node *node, std::string annotation){
	switch(node->type){
		case crl::Node::Type::NONE:
			break;
		case crl::Node::Type::CALL:
			node->annotation = "void";
			this->func_call(node, "void");
			break;
		case crl::Node::Type::BREAK:
			ASSERT(this->can_break, "Invalid break/continue");
			break;
		case crl::Node::Type::RETURN:
			this->func_returns = true;
			if (annotation != "void"){
				ASSERT(node->children.size() == 1, "Non-void functions must return a value");
				ASSERT(node->get_child(0)->type == crl::Node::Type::EXPRESSION, "Can only return expressions");
				this->expression(node->get_child(0), annotation);
			}else {
				ASSERT(node->children.size() == 0, "Void functions must return no value");
				node->annotation = "void";
			}break;
		case crl::Node::Type::IF:
			{
			bool aux = this->func_returns;
			this->func_returns = false;

			this->expression(node->get_child(0), "u32");
			ASSERT(node->get_child(1)->type == crl::Node::Type::THEN, "If statements MUST have a THEN child");

			if (node->get_child(1)->get_child(0)->type == crl::Node::Type::BLOCK)
				this->block(node->get_child(1)->get_child(0), annotation);
			else
				this->statement(node->get_child(1)->get_child(0), annotation);
			
			bool then_returns = this->func_returns; // Check if then statement returns
			this->func_returns = false;

			ASSERT(node->children.size() < 4, "If statement size is too large");
			if (node->children.size() == 3){
				ASSERT(node->get_child(2)->type == crl::Node::Type::ELSE, "If statement has something that isn't an ELSE");
				if (node->get_child(2)->get_child(0)->type == crl::Node::Type::BLOCK)
					this->block(node->get_child(2)->get_child(0), annotation);
				else
					this->statement(node->get_child(2)->get_child(0), annotation);
			}else this->func_returns = false;

			bool else_returns = this->func_returns;
		
			this->func_returns = aux | (then_returns && else_returns);	 // CHECK IF FUNCTION RETURNS
			}
			break;
		case crl::Node::Type::WHILE:
			this->can_break = true;
			this->expression(node->get_child(0), "u32");
			if (node->get_child(1)->get_child(0)->type == crl::Node::Type::BLOCK)
				this->block(node->get_child(1)->get_child(0), annotation);
			else
				this->statement(node->get_child(1)->get_child(0), annotation);

			this->can_break = false;
			break;
		case crl::Node::Type::FOR:

			break;
		case crl::Node::Type::ASSIGN:
			{
				Context::Item item = this->context->seek(get_token(node->get_child(0)).str);
				ASSERT(item.type == Context::Item::Type::VAR, "Can only assign values to variables");
				ASSERT(item._mutable, "Can only assign mutable variables");
				if (node->get_child(1)->type == crl::Node::EXPRESSION)
					this->expression(node->get_child(1), item.subtype);
			}
			break;
		case crl::Node::Type::INIT:
			this->var_declaration(node);
			break;
		case crl::Node::Type::INC:
		case crl::Node::Type::DEC:
			this->unary(node);
			break;
		default:
			throw crl::AssertionError(-1, -1, "Unkown node type");
	}
}

void _sc_Tracker::block(crl::Node *node, std::string annotation){
	this->enter();
	size_t size = node->children.size();
	for (size_t i = 0; i < size; i++){
		this->statement(node->get_child(i), annotation);
	}
	this->leave();
}


void _sc_Tracker::func_call(crl::Node *node, std::string annotation){
	size_t size = node->children.size();
	std::string func_name = get_token(node->get_child(0)).str;
	Context::Item item = this->context->seek(func_name);

	ASSERT(item.type == Context::Item::Type::FUNC || item.type == Context::Item::Type::CAS, "'" + func_name + "' is not a function");

	ASSERT(item.subtype == annotation, "Return type does not match expression type");
	
	ASSERT(item.args.size() == size - 1, "Functions do not match");

	node->argc = size - 1;

	for (size_t i = 1; i < size; i++){
		crl::Node *arg = node->get_child(i);
		ASSERT(arg->type == crl::Node::Type::ARG, "Argument isn't of type ARG");
		if (item.args[i-1].second){
			ASSERT(arg->annotation == "ref", "Mutable function argments must be passed as references");
			Context::Item aux = this->context->seek(get_token(arg->get_child(0)).str);
			ASSERT(aux.type == Context::Item::Type::VAR, "No variable with this name");
			ASSERT(aux._mutable, "Variable must be mutable to be passed by reference");
			ASSERT(aux.subtype == item.args[i-1].first, "Passed variable type does not match function argument type");
		}else{
			this->expression(arg->get_child(0), item.args[i-1].first);
			arg->annotation = item.args[i-1].first;
		}
	}
}

#define EXPR_OP(token) (token.type == crl::Token::Type::PLUS || token.type == crl::Token::Type::MINUS || token.type == crl::Token::Type::TIMES || token.type == crl::Token::Type::SLASH)

bool is_castable(std::string s1, std::string s2){
	return true;
}

void _sc_Tracker::expression(crl::Node *node, std::string annotation){
	node->annotation = annotation;
	size_t size = node->children.size();

	for (size_t i = 0; i < size; i++){
		switch (node->get_child(i)->type){
		case crl::Node::Type::CALL:
			node->get_child(i)->annotation = annotation;
			this->func_call(node->get_child(i), annotation);
			break;
			
		case crl::Node::Type::DEC:
		case crl::Node::Type::INC:
			if (((crl::Leaf *) node->get_child(i)->get_child(0))->token.type == crl::Token::Type::IDENT){
				Context::Item item = this->context->seek(((crl::Leaf *) node->get_child(i)->get_child(0))->token.str);
				ASSERT(item.type == Context::Item::Type::VAR && item.subtype == annotation, "Identifier has not been defined: " + item.name);
				ASSERT(item._mutable, "Variable must be mutable");
			}else ASSERT(false, "Unkown type");
			break;
		case crl::Node::Type::CAST:
			if (((crl::Leaf *) node->get_child(i)->get_child(0))->token.type == crl::Token::Type::IDENT){
				Context::Item item = this->context->seek(((crl::Leaf *) node->get_child(i)->get_child(0))->token.str);
				ASSERT(item.type == Context::Item::Type::VAR, "Identifier has not been defined: " + item.name);
				ASSERT(get_token(node->get_child(i)->get_child(1)).str == annotation, "Cast type must correspond to expression type");
				ASSERT(is_castable(item.subtype, annotation), item.subtype + " isn't castable to " + annotation);
			}
			break;
		case crl::Node::Type::LEAF:
			if (((crl::Leaf *) node->get_child(i))->token.type == crl::Token::Type::IDENT){
				Context::Item item = this->context->seek(((crl::Leaf *) node->get_child(i))->token.str);
				ASSERT(item.type == Context::Item::Type::VAR && item.subtype == annotation, "Identifier has not been defined: " + item.name);
			}else if (((crl::Leaf *) node->get_child(i))->token.type == crl::Token::Type::INT){
				ASSERT(annotation == "i32" || annotation == "u32" || annotation == "char" || annotation == "i64" || annotation == "u64", "Must have integer or unsigned type");
			}else if (((crl::Leaf *) node->get_child(i))->token.type == crl::Token::Type::DEC){
				ASSERT(annotation == "f32" || annotation == "f64", "Must have float type");
			}else if (((crl::Leaf *) node->get_child(i))->token.type == crl::Token::Type::STRING){
				ASSERT(annotation == "string", "Must have string type");
			
			}else if (EXPR_OP(((crl::Leaf *) node->get_child(i))->token));
			else ASSERT(false, "Unkown type");
			break;
		default:
			this->expression(node->get_child(i), annotation);
			break;
		}
	}
}

void _sc_Tracker::unary(crl::Node *node){
	crl::Node *ident = node->get_child(0);
	ASSERT(ident->type == crl::Node::Type::LEAF, "Node must be leaf");
	
	crl::Token t = get_token(ident);

	Context::Item item = this->context->seek(t.str);
	ASSERT(item.type == Context::Item::Type::VAR, "Identifier has not been defined: " + item.name);
	ASSERT(item._mutable, "Variable must be mutable");
	ASSERT(item.subtype == "i32" || item.subtype == "u32" || item.subtype == "char" || item.subtype == "i64" || item.subtype == "u64" || item.subtype == "f32" || item.subtype == "f64", "Must have numeric type");
}

void _sc_Tracker::program(crl::Node *node){
	size_t size = node->children.size();

	for (size_t i = 0; i < size; i++){
		crl::Node *child = node->get_child(i);
		ASSERT(child->type == crl::Node::Type::VAR_DECLARATION || child->type == crl::Node::Type::FUNC_DECLARATION || child->type == crl::Node::Type::CAS, "Program must only have declarations or cas instructions");
		if (child->type == crl::Node::Type::FUNC_DECLARATION)
			this->func_declaration(child);
		else if (child->type == crl::Node::Type::VAR_DECLARATION)
			this->var_declaration(child);
		else if (child->type == crl::Node::Type::CAS)
			this->cas(child);
		else
			ASSERT(false, "???");
	}	
}
void _sc_Tracker::check(){
	this->ast = this->ast->get_child(0); // Enter the program	
	this->program(this->ast);
	Context::Item item = this->context->seek("main");
	ASSERT(item.type == Context::Item::Type::FUNC, "Main function is not defined");
	ASSERT(item.subtype == "u32", "Main function is not defined");
}



void crl::semantic_check(crl::Ast *ast){
	_sc_Tracker tracker(ast);
	tracker.check();
}
