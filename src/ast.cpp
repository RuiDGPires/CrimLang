#include "ast.hpp"

namespace crl{
Ast::Ast(){
}

Node::Node(){}
Node::~Node(){}

void Node::addChild(Node *n){
    n->parent = this;
    this->children.push_back(n);
}

Node *Node::getChild(int i){
    return this->children[i];
}

Node *Node::operator[](int i){
    return this->getChild(i);
}

std::string Node::toString(int depth) const{
    std::string ret = std::string(depth, ' ') + std::string((int) this->type, ' ');
    for (Node *n: this->children)
        ret += "\n" + n->toString(depth + 2);
    return ret;
}

void Node::deleteNode(){
    for (Node *child : this->children)
        if (child != NULL){
            child->deleteNode();
            delete child;
        }
}
}
