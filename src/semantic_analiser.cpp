#include "compiler.hpp"
#include "context.hpp"

static void assert(bool v, std::string s){
	if (!v) throw "Assertion error: " + s;
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
		void block(crl::Node *);
		void expression(crl::Node *);
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
	if (this->context->has_name(item.name)) throw std::string("Multiple declarations"); // TODO
	else (this->context->add(item));
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
	this->block(node->get_child(i));
	this->leave();
}

void _sc_Tracker::var_declaration(crl::Node *node){
	Context::Item item;
	item.type = Context::Item::Type::VAR;
	item.subtype = ((crl::Leaf *) node->get_child(0))->token.str;

	if (node->annotation == "mut")
		item._mutable = true;


	if (node->get_child(1)->type == crl::Node::Type::ASSIGN)
		this->expression(node->get_child(1)->get_child(0), item.subtype);

	this->add(item);
}

void _sc_Tracker::block(crl::Node *node){

}

void _sc_Tracker::expression(crl::Node *node, std::string annotation){
	
}

void _sc_Tracker::program(crl::Node *node){
	size_t size = node->children.size();

	for (size_t i = 0; i < size; i++){
		crl::Node *child = node->get_child(i);
		assert(child->type == crl::Node::Type::VAR_DECLARATION || child->type == crl::Node::Type::FUNC_DECLARATION || child->type == crl::Node::Type::CAS, "Program must only have declarations or cas instructions");
		if (child->type == crl::Node::Type::FUNC_DECLARATION)
			this->func_declaration(child);
		else if (child->type == crl::Node::Type::VAR_DECLARATION)
			this->var_declaration(child);
		else if (child->type == crl::Node::Type::CAS)
			this->cas(child);
		else
			throw "???";
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
