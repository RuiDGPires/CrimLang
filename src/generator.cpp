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
	if (type == "void")
		return 0;
	else if (type == "i32" || type == "u32" || type == "f32" || type == "char")
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
		std::map<std::string, std::list<std::pair<u32, bool>>> local_table; // bool is if is pointer or not

		std::list<std::string> local_names;
		u32 static_mem_pointer = 0;
		u32 frame_offset = 0;
		RegTracker regs = RegTracker(9);

		void declaration_cas(crl::Node *);
		void declaration_var(crl::Node *);
		void declaration_func(crl::Node *);
		void func_call(crl::Node *, std::stringstream &);
		void init(crl::Node *);
		void block(crl::Node *);

		void expression(crl::Node *, std::stringstream &);

		void program(crl::Node *);

	public:
		_gc_Tracker(std::ofstream *f){ this->file = f;}
		~_gc_Tracker(){}

		void parse_node(crl::Node*);
};

void _gc_Tracker::expression(crl::Node *node, std::stringstream &stream){
	// TODO
	stream << "MVI R1 2\nPSH R1\n";
}

void _gc_Tracker::declaration_var(crl::Node *node){
	ASSERT(node->get_child(0)->type == crl::Node::Type::LEAF, "Var declaration must start with type");
	ASSERT(node->get_child(1)->type == crl::Node::Type::LEAF, "Var declaration must have a name as second token");
	std::string type = get_token(node->get_child(0)).str;
	std::string name = get_token(node->get_child(1)).str;


	u32 size = size_of(type);

	this->stream_defines << "%define " << name << " " << static_mem_pointer << "\n";
	this->global_table.insert(std::pair(name, static_mem_pointer));	 // Add to the global table

	if (node->children.size() == 3){
		ASSERT(node->get_child(2)->type == crl::Node::Type::ASSIGN, "Third child must be assign");
		this->expression(node->get_child(2)->get_child(0), this->stream_global_vars);
		this->stream_global_vars << 
			"; Global " << name << "\n" << 
			"MVI R1 " << name << "\n" <<
			"; Pop the result from the expression\n";
		for (u32 i = 0; i < size; i++)
				this->stream_global_vars <<
				"POP R2\n" <<
				"STORE m[R1][" << size - i - 1<< "] R2\n";
		this->stream_global_vars << "\n";
	}
	static_mem_pointer += size; 
}

void _gc_Tracker::declaration_cas(crl::Node *node){
	ASSERT(node->get_child(0)->type == crl::Node::Type::LEAF, "Func declaration must have a type as first token");
	ASSERT(node->get_child(1)->type == crl::Node::Type::LEAF, "Func declaration must have a name as second token");
	std::string type = get_token(node->get_child(0)).str;
	std::string name = get_token(node->get_child(1)).str;
	u32 size = size_of(type);

	stream_funcs << name << ":\n"; 
	
	// RESULT SPACE ALLOCATION MUST BE DONE BY CALLER
	u32 frame_rollback = size;

	this->stream_funcs << 
		";Function " << name << "\n" <<
		"MOV R7 RF\n" << // TMP STORE RF
		"MOV RF RS\n" <<
		"PSH R7\n" <<
		"PSH R9\n" <<
		"PSH R8\n" <<
		"PSH RE\n"; // PUSH RF

	// Arguents will be stored on the stack like this:
	// foo(a, b, c) -> [a b c]

	// TREAT ARGMENTS
	for(u32 i = 0; i < node->argc; i++){
		crl::Node *arg = node->get_child(2 + i);
		std::string type = get_token(arg->get_child(0)).str;
		std::string name = get_token(arg->get_child(1)).str;

		bool is_mut = arg->annotation == "mut";
		u32 arg_size = is_mut? 1: size_of(type);
		
		local_names.push_front(name);
		local_table.insert(std::pair(name, std::list<std::pair<u32, bool>>()));
		local_table.at(name).push_front(std::pair(arg_size + frame_rollback, is_mut));

		frame_rollback += arg_size;
	}

	
	this->stream_funcs <<
		"MVI R1 " << frame_rollback << "\n" <<
		"ADD RF R1\n"; // Rollback frame

	crl::Node *block = node->get_child(2+node->argc);
	size_t block_size = block->children.size();
	for (size_t i = 0; i < block_size; i++){
		ASSERT(block->get_child(i)->type == crl::Node::Type::LEAF, "ALL CHILDREN OF CAS DECLARATION MUST BE LEAFS");
		stream_funcs << get_token(block->get_child(i)).str << "\n";
	}
	
	this->frame_offset = 0;
	this->block(block);

	this->stream_funcs << 
		"; End of function\n" <<
		name << "-end:\n" <<
		"POP RE\n" <<
		"POP R8\n" <<
		"POP R9\n" <<
		"POP RF\n" <<
		"MVI R1 " << frame_rollback - size << "\n" <<
		"ADD RS R1\n" <<
		"RET\n\n";
		// The rest is the result (on stack)
}

void _gc_Tracker::declaration_func(crl::Node *node){
	ASSERT(node->get_child(0)->type == crl::Node::Type::LEAF, "Func declaration must have a type as first token");
	ASSERT(node->get_child(1)->type == crl::Node::Type::LEAF, "Func declaration must have a name as second token");
	std::string type = get_token(node->get_child(0)).str;
	std::string name = get_token(node->get_child(1)).str;
	u32 size = size_of(type);

	stream_funcs << name << ":\n"; 
	
	// RESULT SPACE ALLOCATION MUST BE DONE BY CALLER
	u32 frame_rollback = size;

	this->stream_funcs << 
		";Function " << name << "\n" <<
		"MOV R7 RF\n" << // TMP STORE RF
		"MOV RF RS\n" <<
		"PSH R7\n" <<
		"PSH R9\n" <<
		"PSH R8\n" <<
		"PSH RE\n"; // PUSH RF

	// Arguents will be stored on the stack like this:
	// foo(a, b, c) -> [a b c]

	// TREAT ARGMENTS
	for(u32 i = 0; i < node->argc; i++){
		crl::Node *arg = node->get_child(2 + i);
		std::string type = get_token(arg->get_child(0)).str;
		std::string name = get_token(arg->get_child(1)).str;

		bool is_mut = arg->annotation == "mut";
		u32 arg_size = is_mut? 1: size_of(type);
		
		local_names.push_front(name);
		local_table.insert(std::pair(name, std::list<std::pair<u32, bool>>()));
		local_table.at(name).push_front(std::pair(arg_size + frame_rollback, is_mut));

		frame_rollback += arg_size;
	}

	
	this->stream_funcs <<
		"MVI R1 " << frame_rollback << "\n" <<
		"ADD RF R1\n"; // Rollback frame

	crl::Node *block = node->get_child(2+node->argc);
	ASSERT(block->type == crl::Node::Type::BLOCK, "Funcs must have a block");
	
	this->frame_offset = 0;
	this->block(block);

	this->stream_funcs << 
		"; End of function\n" <<
		name << "-end:\n" <<
		"POP RE\n" <<
		"POP R8\n" <<
		"POP R9\n" <<
		"POP RF\n" <<
		"MVI R1 " << frame_rollback - size << "\n" <<
		"ADD RS R1\n" <<
		"RET\n\n";
		// The rest is the result (on stack)
}

void _gc_Tracker::func_call(crl::Node *node, std::stringstream &stream){
	ASSERT(node->get_child(0)->type == crl::Node::Type::LEAF, "Func declaration must have a type as first token");
	std::string name = get_token(node->get_child(0)).str;

	u32 ret_size = size_of(node->annotation);

	stream << 
		"MVI R1 " << ret_size << "\n" <<
		"ADD RS R1\n"; // ALLOCATE MEMORY FOR RETURN VALUE

	for (u32 i = 1; i <= node->argc; i ++){
		crl::Node *arg = node->get_child(i);
		bool is_mut = arg->annotation == "ref";

		if (is_mut){
			std::string arg_name = get_token(arg->get_child(0)).str;
			if (local_table.count(arg_name) > 0){
				ASSERT(local_table.at(arg_name).front().second, "Variable needs to be mutable to be passed by reference");
				stream << "MVI R1 " << local_table.at(arg_name).front().first << "\n" <<
					"MOV R2 RF\n" << 
					"SUB R2 R1\n" <<
					"PSH R2\n";
			}else{
				stream << "MVI R1 " << global_table.at(arg_name) << "\n" <<
					"PSH R1\n";
			}
		}else this->expression(arg->get_child(0), stream_funcs);
	}

	stream << "JMP " << name << "\n";
}

void _gc_Tracker::block(crl::Node *node){
	size_t size = node->children.size();
	for (size_t i = 0; i < size; i++)
		parse_node(node->get_child(i));
}

void _gc_Tracker::init(crl::Node *node){
	ASSERT(node->get_child(0)->type == crl::Node::Type::LEAF, "Var declaration must start with type");
	ASSERT(node->get_child(1)->type == crl::Node::Type::LEAF, "Var declaration must have a name as second token");
	std::string type = get_token(node->get_child(0)).str;
	std::string name = get_token(node->get_child(1)).str;
	
	u32 size = size_of(type);


	bool is_mut = node->annotation == "mut";
	
	if (this->local_table.count(name) == 0) this->local_table.insert(std::pair(name, std::list<std::pair<u32, bool>>()));
	this->local_table.at(name).push_front(std::pair(frame_offset, is_mut));
	this->local_names.push_front(name);

	if (node->children.size() == 3){
		ASSERT(node->get_child(2)->type == crl::Node::Type::ASSIGN, "Third child must be assign");
		this->expression(node->get_child(2)->get_child(0), this->stream_global_vars);
		// The result of the expression will be stored on the stack, that is our variable
	}else
		this->stream_funcs << 
			"; Advance the stack (variable is unitialized)" << 
			"SUB RS " << size << "\n";// PUSH ZEROS
	
	this->stream_funcs << "\n";
	
	frame_offset += size;
}

void _gc_Tracker::program(crl::Node *node){
	*(this->file) << 
		";-----HEADER-----\n"<< 
		"_start:\n";


	for (crl::Node *child : node->children)
		this->parse_node(child);

	*(this->file) << stream_defines.str();
	*(this->file) << 
		"\n;---GLOBAL-INITS---\n";

	*(this->file) << stream_global_vars.str();
	*(this->file) << "JMP main\n";
	*(this->file) << "END\n";
	*(this->file) << 
		";--END-OF-HEADER-\n";

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
		case crl::Node::Type::CALL:
			this->func_call(node, stream_funcs);
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
