#include "compiler.hpp"
#include "context.hpp"

#ifdef DEBUG
#define ASSERT(v, s) if (!v) throw crl::AssertionError(-1, -1, s)
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


	for (int i = 2; node->get_child(i)->type == crl::Node::Type::ARG; i++){
		crl::Node *arg = node->get_child(i);
		std::string var_type = ((crl::Leaf *) arg->get_child(0))->token.str;

		item.args.push_back(std::pair<std::string, bool>(var_type, arg->annotation == "mut"));
	}

	this->add(item);
}

void _sc_Tracker::func_declaration(crl::Node *node){
	Context::Item item;
	item.type = Context::Item::Type::FUNC;
	item.subtype = ((crl::Leaf *)node->get_child(0))->token.str;
	item.name = ((crl::Leaf *)node->get_child(1))->token.str;

	// Parse once to add to the args to the declaration
	for (int i = 2; node->get_child(i)->type == crl::Node::Type::ARG; i++){
		crl::Node *arg = node->get_child(i);
		std::string var_type = ((crl::Leaf *) arg->get_child(0))->token.str;

		item.args.push_back(std::pair<std::string, bool>(var_type, arg->annotation == "mut"));
	}

	this->add(item);

	this->enter();
	// Add args to the function context
	int i = 2;
	for (; node->get_child(i)->type == crl::Node::Type::ARG; i++){
		crl::Node *arg = node->get_child(i);
		Context::Item arg_item;
		arg_item.type = Context::Item::Type::VAR;
		arg_item.subtype = ((crl::Leaf *) arg->get_child(0))->token.str;
		arg_item.name = ((crl::Leaf *) node->get_child(1))->token.str;
		this->add(arg_item);
	}
	this->block(node->get_child(i), item.subtype);
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
		if (node->get_child(2)->get_child(0)->type == crl::Node::Type::EXPRESSION)
			this->expression(node->get_child(2)->get_child(0), item.subtype);
	}
	this->add(item);
}

void _sc_Tracker::statement(crl::Node *node, std::string annotation){
	switch(node->type){
		case crl::Node::Type::NONE:
			break;
		case crl::Node::Type::CALL:
			this->func_call(node, "void");
			break;
		case crl::Node::Type::RETURN:
			this->expression(node->get_child(0), annotation);
			break;
		case crl::Node::Type::IF:
			this->expression(node->get_child(0), "i32");
			ASSERT(node->get_child(1)->type == crl::Node::Type::THEN, "If statements MUST have a THEN child");
			if (node->get_child(1)->get_child(0)->type == crl::Node::Type::BLOCK)
				this->block(node->get_child(1)->get_child(0), annotation);
			else
				this->statement(node->get_child(1)->get_child(0), annotation);
			
			ASSERT(node->children.size() < 4);
			if (node->children.size() == 3){
				ASSERT(node->get_child(2)->type == crl::Node::Type::ELSE, "If statement has something that isn't an ELSE");
				if (node->get_child(2)->get_child(0)->type == crl::Node::Type::BLOCK)
					this->block(node->get_child(2)->get_child(0), annotation);
				else
					this->statement(node->get_child(2)->get_child(0), annotation);
			}

			break;
		case crl::Node::Type::WHILE:
			this->expression(node->get_child(0), "i32");
			if (node->get_child(1)->get_child(0)->type == crl::Node::Type::BLOCK)
				this->block(node->get_child(1)->get_child(0), annotation);
			else
				this->statement(node->get_child(1)->get_child(0), annotation);
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
		default:
			throw crl::AssertionError(-1, -1, "Unkown node type");
	}
}

void _sc_Tracker::block(crl::Node *node, std::string annotation){
	size_t size = node->children.size();
	for (size_t i = 0; i < size; i++){
		this->statement(node->get_child(i), annotation);
	}
}


void _sc_Tracker::func_call(crl::Node *node, std::string annotation){
	size_t size = node->children.size();
	Context::Item item = this->context->seek((get_token(node->get_child(0)).str));

	ASSERT(item.type != Context::Item::Type::FUNC && item.type != Context::Item::Type::CAS, "Invalid call");

	ASSERT(item.subtype == annotation, "Return type does not match expression type");
	
	ASSERT(item.args.size() == size - 1, "Functions do not match");

	for (size_t i = 1; i < size; i++){
		crl::Node *arg = node->get_child(i);
		ASSERT(arg->type != crl::Node::Type::ARG, "Argument isn't of type ARG");
		if (item.args[i-1].second){
			ASSERT(arg->annotation == "ref", "Mutable function argments must be passed as references");
			Context::Item aux = this->context->seek(get_token(arg->get_child(0)).str);
			ASSERT(aux.type == Context::Item::Type::VAR, "No variable with this name");
			ASSERT(aux._mutable, "Variable must be mutable to be passed by reference");
			ASSERT(aux.subtype == item.args[i-1].first, "Passed variable type does not match function argument type");
		}else
			this->expression(arg->get_child(0), annotation);
	}

}

void _sc_Tracker::expression(crl::Node *node, std::string annotation){
	size_t size = node->children.size();

	for (size_t i = 0; i < size; i++){
		switch (node->get_child(i)->type){
		case crl::Node::Type::CALL:
			this->func_call(node->get_child(i), annotation);
			break;
		case crl::Node::Type::LEAF:
			if (((crl::Leaf *) node->get_child(i))->token.type == crl::Token::Type::IDENT){
				Context::Item item = this->context->seek(((crl::Leaf *) node->get_child(i))->token.str);
				ASSERT(item.type == Context::Item::Type::VAR && item.subtype == annotation, "Identifier has not been defined");
			}
			break;
		default:
			this->expression(node->get_child(i), annotation);
			break;
		}
	}
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
}



void crl::semantic_check(crl::Ast *ast){
	_sc_Tracker tracker(ast);
	tracker.check();
}
