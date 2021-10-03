#include "compiler.hpp"
#include <iostream>
#include <fstream>

static void assert(bool v, std::string s){
	if (!v) throw "Assertion error: " + s;
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
			assert(node->get_child(0)->type == crl::Node::Type::LEAF, "FIRST TOKEN OF CAS DECLARATION MUST BE LEAF (identifier)");
			*(this->file) << ((crl::Leaf *)node->get_child(0))->token.str << ":\n"; 
			
			size_t size = node->children.size();
			for (size_t i = 1; i < size; i++){
				assert(node->get_child(i)->type == crl::Node::Type::LEAF, "ALL CHILDREN OF CAS DECLARATION MUST BE LEAFS");
				*(this->file) << ((crl::Leaf *)node->children[i])->token.str << "\n";
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

	assert(ast->type == Node::Type::NONE, "Invalid type");
	assert(ast->children.size() == 1, "Invalid size");

	ast = ast->get_child(0); // Program

	_gc_Tracker tracker(&file);


	for (Node *child : ast->children)
		tracker.parse_node(child);


	file.close();
}
}
