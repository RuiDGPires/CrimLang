#include "compiler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <map>

//----------------------------------------------

// Map each symbol to a linked list of pairs with a value and if that value is mutable or not (pointer or not)

/* Then plan...
 * Global variables are stored in static data location
 *  
 * Memory:
 *	[STATIC DATA| HEAP |->   <-|STACK]
 *
 */

//----------------------------------------------
//

#ifdef DEBUG
#define ASSERT(v, s) if (!(v)) throw crl::AssertionError(-1, -1, s)
#else
#define ASSERT(...)
#endif

static crl::Token get_token(crl::Node *node){
	return ((crl::Leaf *)node)->token;
}

static u32 size_of(std::string type){
	if (type == "i32" || type == "u32" || type == "f32" || type == "char")
		return 1;
	else if (type == "i64" || type == "u64" || type == "f64")
		return 2;	
	else ASSERT(false, "Unkown type");
}

class RegTracker{
	private:
		std::vector<bool> regs;
	public:
		int alloc(){
			size_t size = regs.size();
			for (size_t i = 0; i < size; i++) if (!regs[i]){
				regs[i] = true; return i;
			}
			return -1;
		}
		void free(int i){
			regs[i] = false;
		}
		std::string reg_name(int i){
			std::string ret = "R";	
			ret += std::string(1, '0' + i + 1);
			return ret;
		}

		RegTracker(){}
		RegTracker(u32 num){this->regs = std::vector<bool>(num, false);}
		~RegTracker(){}
};

class _gc_Tracker{
	private:
		std::ofstream *file;
		std::stringstream stream_defines, stream_global_vars, stream_funcs;

		std::map<std::string, u32> global_table;
		std::map<std::string, std::list<u32>> local_table;
		u32 static_mem_pointer = 0;
		RegTracker regs = RegTracker(9);

		void declaration_cas(crl::Node *);
		void declaration_var(crl::Node *);
		void declaration_func(crl::Node *);
		void init(crl::Node *);

		void expression(crl::Node *, std::stringstream &);

		void program(crl::Node *);

	public:
		_gc_Tracker(std::ofstream *f){ this->file = f;}
		~_gc_Tracker(){}

		void parse_node(crl::Node*);
};

void _gc_Tracker::expression(crl::Node *node, std::stringstream &stream){
	stream << "MVI R1 2\nPSH R1\n";
}

void _gc_Tracker::declaration_cas(crl::Node *node){
	ASSERT(node->get_child(1)->type == crl::Node::Type::LEAF, "Cas declaration must have a name as second token");
	stream_funcs << get_token(node->get_child(1)).str << ":\n"; 
	ASSERT(node->get_child(2)->type == crl::Node::Type::BLOCK, "Cas declaration must have a block");
	
	crl::Node *block = node->get_child(2);
	size_t size = block->children.size();
	for (size_t i = 0; i < size; i++){
		ASSERT(block->get_child(i)->type == crl::Node::Type::LEAF, "ALL CHILDREN OF CAS DECLARATION MUST BE LEAFS");
		stream_funcs << get_token(block->get_child(i)).str << "\n";
	}
}

void _gc_Tracker::declaration_var(crl::Node *node){
	ASSERT(node->get_child(0)->type == crl::Node::Type::LEAF, "Var declaration must start with type");
	ASSERT(node->get_child(1)->type == crl::Node::Type::LEAF, "Var declaration must have a name as second token");
	std::string type = get_token(node->get_child(0)).str;
	std::string name = get_token(node->get_child(1)).str;

	std::list<u32> l;
	l.push_front(static_mem_pointer);

	u32 size = size_of(type);

	this->stream_defines << "%define " << name << " " << static_mem_pointer << "\n";

	if (node->children.size() == 3){
		this->expression(node->get_child(2), this->stream_global_vars);
		this->stream_global_vars << 
			"; Global " << name << "\n" << 
			"MVI R1 " << name << "\n" <<
			"; Pop the result from the expression\n";
		for (u32 i = 0; i < size; i++)
			this->stream_global_vars <<
				"POP R2\n" <<
				"STORE m[R1][" << size - i - 1<< "] R2\n";
	}
	static_mem_pointer += size; 
}

void _gc_Tracker::declaration_func(crl::Node *node){
	ASSERT(node->get_child(1)->type == crl::Node::Type::LEAF, "Cas declaration must have a name as second token");
	stream_funcs << get_token(node->get_child(1)).str << ":\n"; 
	ASSERT(node->get_child(2)->type == crl::Node::Type::BLOCK, "Cas declaration must have a block");
	crl::Node *block = node->get_child(2);
	
}

void _gc_Tracker::init(crl::Node *node){

}

void _gc_Tracker::program(crl::Node *node){
	*(this->file) << "_start:\n";

	for (crl::Node *child : node->children)
		this->parse_node(child);

	*(this->file) << stream_defines.str();
	*(this->file) << stream_global_vars.str();
	*(this->file) << "JMP main\n";
	*(this->file) << "END\n";
	*(this->file) << stream_funcs.str();
}

void _gc_Tracker::parse_node(crl::Node * node){
	switch (node->type){
		case crl::Node::Type::CAS:
			this->declaration_cas(node);
			break;
		case crl::Node::Type::VAR_DECLARATION:
			this->declaration_var(node);
			break;
		case crl::Node::Type::FUNC_DECLARATION:
			this->declaration_func(node);
			break;
		case crl::Node::Type::INIT:
			this->init(node);
			break;
		case crl::Node::Type::PROGRAM:
			this->program(node);
			break;
		default:
			break;
	}
}

namespace crl{
void generate_cas(Ast *ast, std::string filename){
	std::ofstream file;
	file.open(filename);

	ASSERT(ast->type == Node::Type::NONE, "Invalid type");
	ASSERT(ast->children.size() == 1, "Invalid size");

	ast = ast->get_child(0); // Program

	_gc_Tracker tracker(&file);


	tracker.parse_node(ast);

	file.close();
}
}
