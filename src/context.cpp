#include "context.hpp"

Context::Context(){
	this->parent = NULL;
}	
Context::Context(Context *parent){
	this->parent = parent;
}	
Context::~Context(){}	

Context::Item Context::seek(std::string name){
	if (!this->table.count(name))
		if (parent != NULL)
			return parent->seek(name);
		else 
			return Context::Item();
	else
		return this->table.at(name);
}

bool Context::has_name_local(std::string name){
	return this->table.count(name) != 0;
}

bool Context::has_name(std::string name){
	Context::Item a = seek(name);
	return a.type != Context::Item::Type::NONE;
}

bool Context::has(Context::Item &item){
	Context::Item other = seek(item.name);
	return other == item;
}

void Context::add(Context::Item item){
	this->table.insert(std::pair<std::string, Context::Item>(item.name, item));
}
