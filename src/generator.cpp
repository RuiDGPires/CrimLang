#include "compiler.hpp"
#include <iostream>
#include <fstream>

#ifdef DEBUG
#define ASSERT(v, s) if (!(v)) throw crl::AssertionError(-1, -1, s)
#else
#define ASSERT(...)
#endif

static crl::Token get_token(crl::Node *node){
	return ((crl::Leaf *)node)->token;
}

class _gc_Tracker{
	std::ofstream *file;
	public:
		_gc_Tracker(std::ofstream *f){ this->file = f;}
		~_gc_Tracker(){}

		void parse_node(crl::Node*);
};

void _gc_Tracker::parse_node(crl::Node * node){
	switch (node->type){
		case crl::Node::Type::CAS:
			{
			ASSERT(node->get_child(1)->type == crl::Node::Type::LEAF, "Cas declaration must have a name as second token");
			*(this->file) << get_token(node->get_child(1)).str << ":\n"; 
			ASSERT(node->get_child(2)->type == crl::Node::Type::BLOCK, "Cas declaration must have a block");
			
			crl::Node *block = node->get_child(2);
			size_t size = block->children.size();
			for (size_t i = 0; i < size; i++){
				ASSERT(block->get_child(i)->type == crl::Node::Type::LEAF, "ALL CHILDREN OF CAS DECLARATION MUST BE LEAFS");
				*(this->file) << get_token(block->get_child(i)).str << "\n";
			}
			}	
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


	for (Node *child : ast->children)
		tracker.parse_node(child);


	file.close();
}
}
