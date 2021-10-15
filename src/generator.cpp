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
			ASSERT(false, "Unable to alloc register");
		}
		void free(int i){
			regs[i] = false;
		}
		std::string reg_name(int i){
			std::string ret = "R";	
			ret += std::string(1, '1' + i);
			return ret;
		}

		RegTracker(){}
		RegTracker(u32 num){this->regs = std::vector<bool>(num, false);}
		~RegTracker(){}
};

class LabelTracker{
	private:
		u32 count = 0;
	public:
		LabelTracker(){}
		~LabelTracker(){}
		u32 create(){
			return count++;
		}
		std::string name(u32 i){
			return std::string("L") + std::to_string(i); 
		}
};

class SymbTracker{
	private:
		std::map<std::string, u32> global_table;
		std::map<std::string, std::list<std::pair<u32, bool>>> local_table; // bool is if is pointer or not
		std::list<std::string> local_names;
		u32 static_mem_pointer = 0;
	public:
		u32 sp = 0;
		u32 frame_offset = 0;

		void push(std::string name, std::stringstream &stream){
			stream << "PSH " << name << "\n";	
			sp++;
		}
		void pop(std::string name, std::stringstream &stream){
			stream << "POP " << name << "\n";	
			sp--;
		}

		u32 global_create(std::string name, u32 size){
			this->global_table.insert(std::pair(name, static_mem_pointer));	 // Add to the global table
			u32 res = this->static_mem_pointer;
			this->static_mem_pointer += size;
			return res;
		}
		void arg_create(std::string name, u32 size, bool is_pointer){
			if (this->local_table.count(name) == 0) this->local_table.insert(std::pair(name, std::list<std::pair<u32, bool>>()));
			frame_offset += size;
			this->local_table.at(name).push_front(std::pair(frame_offset, is_pointer));
			local_names.push_front(name);
		}

		void local_create(std::string name, u32 size){
			if (this->local_table.count(name) == 0) this->local_table.insert(std::pair(name, std::list<std::pair<u32, bool>>()));
			this->local_table.at(name).push_front(std::pair(frame_offset + ++sp, false));
			local_names.push_front(name);
		}

		bool has_local(std::string name){
			if (local_table.count(name) > 0) return local_table.at(name).size() > 0;
			else return false;
		}
		
		std::pair<u32, bool> get_local(std::string name){
			return local_table.at(name).front();
		}
		u32 get_global(std::string name){
			return global_table.at(name);
		}

		size_t check(){
			return local_names.size();
		}

		void clean(u32 local_size){
			size_t size = local_names.size();
			for (; size > local_size; size--){
				std::string name = local_names.front();
				local_names.pop_front();

				local_table.at(name).pop_front();
			}
		}

		SymbTracker(){}
		~SymbTracker(){}
};

class _gc_Tracker{
	private:
		std::ofstream *file;
		std::stringstream stream_defines, stream_global_vars, stream_funcs;

		std::string current_func_name;
		RegTracker reg_tracker = RegTracker(9);
		LabelTracker label_tracker;
		SymbTracker symb_tracker;

		void declaration_cas(crl::Node *);
		void declaration_var(crl::Node *);
		void declaration_func(crl::Node *);
		void func_call(crl::Node *, std::stringstream &);
		void init(crl::Node *);
		void block(crl::Node *);
		void return_(crl::Node *);
		void assign(crl::Node *);
		void if_(crl::Node *);

		u32 expression_reg(crl::Node *, std::stringstream &);
		void expression(crl::Node *, std::stringstream &);
		u32 term(crl::Node *, std::stringstream &);
		u32 factor(crl::Node *, std::stringstream &);

		void program(crl::Node *);
		void parse_node(crl::Node*, std::stringstream &);

	public:
		_gc_Tracker(std::ofstream *f){ this->file = f;}
		~_gc_Tracker(){}

		void parse_node(crl::Node*);
};

u32 _gc_Tracker::factor(crl::Node *node, std::stringstream &stream){
	u32 reg;

	ASSERT(node->type == crl::Node::Type::FACTOR, "Factor needs to be factor (Duh)");

	if (node->get_child(0)->type == crl::Node::Type::LEAF){
		reg = this->reg_tracker.alloc();
		std::string reg_name = reg_tracker.reg_name(reg);

		crl::Token t = get_token(node->get_child(0));
		switch (t.type){
			case crl::Token::Type::IDENT:
				if (this->symb_tracker.has_local(t.str)){
					std::pair<u32, bool> p = symb_tracker.get_local(t.str);
					stream <<
						"LOAD " << reg_name << " m[RF][-" << p.first << "]\n";
					if (p.second)
						stream <<
							"LOAD " << reg_name << " m[" << reg_name << "]\n";

				}else{
					stream <<
						"MVI " << reg_name << " " << symb_tracker.get_global(t.str) << "\n" <<
						"LOAD "<< reg_name << " m[" << reg_name << "]\n";
				}
				break;
			case crl::Token::Type::INT:
				stream <<
					"MVI " << reg_name << " " << t.str << "\n";
				break;
			case crl::Token::Type::CHAR:
				stream <<
					"MVI " << reg_name << " '" << t.str << "'\n'";
				break;
			default:
				ASSERT(false, "Factor not implemented");
				break;
		}
	}else if (node->get_child(0)->type == crl::Node::Type::EXPRESSION)
		reg =  this->expression_reg(node->get_child(0), stream);
	else if (node->get_child(0)->type == crl::Node::Type::CALL){
		this->func_call(node->get_child(0), stream);
		reg = this->reg_tracker.alloc();	
		stream << "POP " << reg_tracker.reg_name(reg) << "\n"; // TODO Size larger than 1
	}else ASSERT(false, "Not implemented");
	
	return reg;
}

u32 _gc_Tracker::term(crl::Node *node, std::stringstream &stream){
	ASSERT(node->type == crl::Node::Type::TERM, "Term has to be term (duuuh)");
	size_t size = node->children.size();
	ASSERT(node->get_child(0)->type == crl::Node::Type::FACTOR, "Child needs to be factor");
	u32 reg1 = this->factor(node->get_child(0), stream);
	std::string name1 = reg_tracker.reg_name(reg1);
	u32 reg2;

	for (size_t i = 1; i < size; i += 2){
		// TODO : division
		reg2 = this->factor(node->get_child(i+1), stream);
		if (get_token(node->get_child(i)).type == crl::Token::Type::TIMES)
			stream << "MUL " << name1 << " " << reg_tracker.reg_name(reg2) << "\n";
		else
			stream << "DIV " << name1 << " " << reg_tracker.reg_name(reg2) << "\n";
		reg_tracker.free(reg2);
	}
	
	return reg1;
}

u32 _gc_Tracker::expression_reg(crl::Node *node, std::stringstream &stream){
	size_t size = node->children.size();
	size_t i = 0;
	bool negate = false;

	if (node->get_child(0)->type == crl::Node::Type::LEAF){
		crl::Token t = get_token(node->get_child(0));
		if (t.type == crl::Token::Type::MINUS){
			negate = true;
			i++;
		}else if (t.type == crl::Token::Type::PLUS) i++;
	}
	u32 reg1 = this->term(node->get_child(i++), stream);
	std::string reg_name = reg_tracker.reg_name(reg1);

	if (negate){
		u32 aux = reg_tracker.alloc();
		stream <<
			"MVI " << reg_tracker.reg_name(aux) << "0\n" <<
			"SUB " << reg_tracker.reg_name(aux) << " " << reg_name << "\n";
		reg_tracker.free(reg1);
		std::string name1 = reg_tracker.reg_name(aux);
		reg1 = aux;
	}

	u32 reg2;
	for (; i < size; i += 2){
		// TODO : division
		reg2 = this->term(node->get_child(i+1), stream);
		if (get_token(node->get_child(i)).type == crl::Token::Type::PLUS)
			stream << "ADD " << reg_name << " " << reg_tracker.reg_name(reg2) << "\n";
		else
			stream << "SUB " << reg_name << " " << reg_tracker.reg_name(reg2) << "\n";
		reg_tracker.free(reg2);
	}
	return reg1; 
}

void _gc_Tracker::expression(crl::Node *node, std::stringstream &stream){
	u32 reg = this->expression_reg(node, stream);
	stream << "PSH " << reg_tracker.reg_name(reg) << "\n";
	reg_tracker.free(reg);
}

void _gc_Tracker::declaration_var(crl::Node *node){
	ASSERT(node->get_child(0)->type == crl::Node::Type::LEAF, "Var declaration must start with type");
	ASSERT(node->get_child(1)->type == crl::Node::Type::LEAF, "Var declaration must have a name as second token");
	std::string type = get_token(node->get_child(0)).str;
	std::string name = get_token(node->get_child(1)).str;


	u32 size = size_of(type);

	this->stream_defines << "%define " << name << " " << symb_tracker.global_create(name, size) << "\n";

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
}

void _gc_Tracker::declaration_cas(crl::Node *node){
	ASSERT(node->get_child(0)->type == crl::Node::Type::LEAF, "Func declaration must have a type as first token");
	ASSERT(node->get_child(1)->type == crl::Node::Type::LEAF, "Func declaration must have a name as second token");
	std::string type = get_token(node->get_child(0)).str;
	std::string name = get_token(node->get_child(1)).str;
	this->current_func_name = name;
	u32 size = size_of(type);

	stream_funcs << name << ":\n"; 
	
	// RESULT SPACE ALLOCATION MUST BE DONE BY CALLER
	
	symb_tracker.frame_offset = size - 1;
	symb_tracker.sp = 0;

	this->stream_funcs << 
		";Function " << name << "\n" <<
		"MOV R7 RF\n" << // TMP STORE RF
		"MOV RF RS\n";

	symb_tracker.push("R7", stream_funcs);
	symb_tracker.push("RE", stream_funcs);


	// Arguents will be stored on the stack like this:
	// foo(a, b, c) -> [a b c]

	// TREAT ARGMENTS
	size_t args_size = 0;
	for(u32 i = 0; i < node->argc; i++){
		crl::Node *arg = node->get_child(2 + i);
		std::string type = get_token(arg->get_child(0)).str;
		std::string name = get_token(arg->get_child(1)).str;

		bool is_mut = arg->annotation == "mut";
		u32 arg_size = is_mut? 1: size_of(type);
		args_size += arg_size;
		
		symb_tracker.arg_create(name, arg_size, is_mut);
	}

	u32 rollback = symb_tracker.frame_offset;	
	this->stream_funcs <<
		"MVI R1 " << rollback << "\n" <<
		"ADD RF R1\n"; // Rollback frame

	crl::Node *block = node->get_child(2+node->argc);
	size_t block_size = block->children.size();
	for (size_t i = 0; i < block_size; i++){
		ASSERT(block->get_child(i)->type == crl::Node::Type::LEAF, "ALL CHILDREN OF CAS DECLARATION MUST BE LEAFS");
		stream_funcs << get_token(block->get_child(i)).str << "\n";
	}

	this->stream_funcs << 
		"; End of function\n" <<
		name << "-end:\n" <<
		"MOV RS RF\n" <<
		"MVI R1 " << args_size + 2 << "\n" <<
		"SUB RS R1\n";
		
	symb_tracker.pop("RE", stream_funcs);
	symb_tracker.pop("RF", stream_funcs);

	this->stream_funcs <<
		"MVI R2 2\n"
		"SUB R1 R2\n" <<
		"ADD RS R1\n" <<
		"RET\n\n";
		// The rest is the result (on stack)
}

void _gc_Tracker::declaration_func(crl::Node *node){
	ASSERT(node->get_child(0)->type == crl::Node::Type::LEAF, "Func declaration must have a type as first token");
	ASSERT(node->get_child(1)->type == crl::Node::Type::LEAF, "Func declaration must have a name as second token");
	std::string type = get_token(node->get_child(0)).str;
	std::string name = get_token(node->get_child(1)).str;
	this->current_func_name = name;
	u32 size = size_of(type);

	stream_funcs << name << ":\n"; 
	
	// RESULT SPACE ALLOCATION MUST BE DONE BY CALLER
	
	symb_tracker.frame_offset = size - 1;
	symb_tracker.sp = 0;

	this->stream_funcs << 
		";Function " << name << "\n" <<
		"MOV R7 RF\n" << // TMP STORE RF
		"MOV RF RS\n";

	symb_tracker.push("R7", stream_funcs);
	symb_tracker.push("RE", stream_funcs);


	// Arguents will be stored on the stack like this:
	// foo(a, b, c) -> [a b c]

	// TREAT ARGMENTS
	size_t s = symb_tracker.check();
	size_t args_size = 0;
	for(u32 i = 0; i < node->argc; i++){
		crl::Node *arg = node->get_child(2 + i);
		std::string type = get_token(arg->get_child(0)).str;
		std::string name = get_token(arg->get_child(1)).str;

		bool is_mut = arg->annotation == "mut";
		u32 arg_size = is_mut? 1: size_of(type);
		args_size += arg_size;
		
		symb_tracker.arg_create(name, arg_size, is_mut);
	}

	u32 rollback = symb_tracker.frame_offset;	
	this->stream_funcs <<
		"MVI R1 " << rollback << "\n" <<
		"ADD RF R1\n"; // Rollback frame

	crl::Node *block = node->get_child(2+node->argc);
	ASSERT(block->type == crl::Node::Type::BLOCK, "Funcs must have a block");
	

	this->block(block);
	symb_tracker.clean(s);

	this->stream_funcs << 
		"; End of function\n" <<
		name << "-end:\n" <<
		"MOV RS RF\n" <<
		"MVI R1 " << args_size + 1 + size << "\n" <<
		"SUB RS R1\n";
		
	symb_tracker.pop("RE", stream_funcs);
	symb_tracker.pop("RF", stream_funcs);

	this->stream_funcs <<
		"MVI R2 2\n"
		"SUB R1 R2\n" <<
		"ADD RS R1\n" <<
		"RET\n\n";
		// The rest is the result (on stack)
}

void _gc_Tracker::return_(crl::Node *node){
	if (node->children.size() > 0){
		this->expression(node->get_child(0), stream_funcs);
		stream_funcs << 
			"POP R1\n" <<
			"STORE m[RF] R1\n";
	}

	stream_funcs << 
		"JMP " << current_func_name << "-end\n";
}

void _gc_Tracker::func_call(crl::Node *node, std::stringstream &stream){
	ASSERT(node->get_child(0)->type == crl::Node::Type::LEAF, "Func declaration must have a type as first token");
	std::string name = get_token(node->get_child(0)).str;

	u32 ret_size = size_of(node->annotation);

	stream << 
		"MVI R1 " << ret_size << "\n" <<
		"SUB RS R1\n"; // ALLOCATE MEMORY FOR RETURN VALUE

	for (u32 i = 1; i <= node->argc; i ++){
		crl::Node *arg = node->get_child(i);
		bool is_mut = arg->annotation == "ref";

		if (is_mut){
			std::string arg_name = get_token(arg->get_child(0)).str;
			if (symb_tracker.has_local(arg_name)){
				stream << "MVI R1 " << symb_tracker.get_local(arg_name).first << "\n" <<
					"MOV R2 RF\n" << 
					"SUB R2 R1\n" <<
					"PSH R2\n";
			}else{
				stream << "MVI R1 " << symb_tracker.get_global(arg_name) << "\n" <<
					"PSH R1\n";
			}
		}else this->expression(arg->get_child(0), stream_funcs);
	}

	stream << "JMP " << name << "\n";
}

void _gc_Tracker::block(crl::Node *node){
	size_t s = symb_tracker.check();

	size_t size = node->children.size();
	for (size_t i = 0; i < size; i++)
		parse_node(node->get_child(i), stream_funcs);

	symb_tracker.clean(s);
}

void _gc_Tracker::init(crl::Node *node){
	ASSERT(node->get_child(0)->type == crl::Node::Type::LEAF, "Var declaration must start with type");
	ASSERT(node->get_child(1)->type == crl::Node::Type::LEAF, "Var declaration must have a name as second token");
	std::string type = get_token(node->get_child(0)).str;
	std::string name = get_token(node->get_child(1)).str;
	
	u32 size = size_of(type);

	symb_tracker.local_create(name, size);

	if (node->children.size() == 3){
		ASSERT(node->get_child(2)->type == crl::Node::Type::ASSIGN, "Third child must be assign");
		this->expression(node->get_child(2)->get_child(0), this->stream_funcs);
		// The result of the expression will be stored on the stack, that is our variable
	}else
		this->stream_funcs << 
			"; Advance the stack (variable is unitialized)" << 
			"SUB RS " << size << "\n";// PUSH ZEROS
	
	this->stream_funcs << "\n";
}

void _gc_Tracker::assign(crl::Node *node){
	crl::Token t = get_token(node->get_child(0));
	
	u32 reg1 = reg_tracker.alloc();
	std::string name1 = reg_tracker.reg_name(reg1);
	u32 reg2 = this->expression_reg(node->get_child(1), stream_funcs);
	std::string name2 = reg_tracker.reg_name(reg2);
	

	if (symb_tracker.has_local(t.str)){
		std::pair<u32, bool> p = symb_tracker.get_local(t.str);

		if (p.second)
			stream_funcs <<
				"LOAD " << name1 << " m[RF][-" << p.first  << "]\n" <<
				"STORE m[" << name1 << "] " << name2 << "\n";
		else
			stream_funcs <<
				"STORE m[RF][-" << p.first << "] " << name2 << "\n";
	}else{
		stream_funcs << "MVI " << name1 << " " << t.str << "\n" <<
			"STORE m[" << name1 << "] " << name2 << "\n";
	}
	
	reg_tracker.free(reg1);
	reg_tracker.free(reg2);
}


void _gc_Tracker::if_(crl::Node *node){
	u32 reg = this->expression_reg(node->get_child(0), stream_funcs);

	std::string	done_label = label_tracker.name(label_tracker.create());

	std::string false_label;

	bool has_else = node->children.size() == 3;
	if (has_else)
		false_label	= label_tracker.name(label_tracker.create());
	else
		false_label = done_label;
	
	stream_funcs <<
		"CMP " << reg_tracker.reg_name(reg) << " R0\n" <<
		"JMP.Z " << false_label << "\n";
		
	reg_tracker.free(reg);
	this->parse_node(node->get_child(1)->get_child(0));

	stream_funcs << "JMP " << done_label << "\n";

	if (has_else){
		stream_funcs <<
			false_label << ":\n";
		this->parse_node(node->get_child(2)->get_child(0));
	}
	stream_funcs <<
		done_label << ":\n";
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
	*(this->file) << "PSH R0\n" << "JMP main\n";
	*(this->file) << "END\n";
	*(this->file) << 
		";--END-OF-HEADER-\n";

	*(this->file) << stream_funcs.str();
}

void _gc_Tracker::parse_node(crl::Node * node){
	this->parse_node(node, this->stream_funcs);
}

void _gc_Tracker::parse_node(crl::Node * node, std::stringstream &stream){
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
			this->func_call(node, stream);
			break;
		case crl::Node::Type::EXPRESSION:
			this->expression(node, stream);
			break;
		case crl::Node::Type::TERM:
			this->term(node, stream);
			break;
		case crl::Node::Type::FACTOR:
			this->factor(node, stream);
			break;
		case crl::Node::Type::RETURN:
			this->return_(node);
			break;
		case crl::Node::Type::ASSIGN:
			this->assign(node);
			break;
		case crl::Node::Type::IF:
			this->if_(node);
			break;
		case crl::Node::Type::BLOCK:
			this->block(node);
			break;
		default:
			std::cout << node->type << std::endl;
			ASSERT(false, "Not implemented");
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
