#include "ast.hpp"

namespace crl{

Node::Node(){
	this->type = Node::Type::NONE;
	this->parent = NULL;
}

Node::Node(Type type, Node *parent){
	this->type = type;
	this->parent = parent;
}

Node::~Node(){
	size_t size = this->children.size();
	for (size_t i = 0; i < size; i++)
		if (this->children[i] != NULL)
			delete this->children[i];
}

void Node::add_child(Node *n){
	n->parent = this;
	this->children.push_back(n);
}

Node *Node::get_child(int i){
	return this->children[i];
}

Node *Node::operator[](int i){
	return this->get_child(i);
}

std::string Node::to_string() const {
	return this->to_string(0);
}

std::string Node::to_string(int depth) const{
	std::string aux = "";
	if (this->annotation != "")
		aux = "(" + this->annotation + ") ";
	std::string ret = std::string(depth, ' ') + aux + std::to_string((int) this->type);

	for (Node *n: this->children)
		ret += "\n" + n->to_string(depth + 2);
	return ret;
}

Leaf::Leaf(Token t){
	this->type = Node::Type::LEAF;
	this->token = t;
}

Leaf::~Leaf(){}

std::string Leaf::to_string(int depth) const{
	return std::string(depth, ' ') + this->token.to_string();
}
}
