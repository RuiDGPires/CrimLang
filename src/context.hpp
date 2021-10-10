#pragma once
#include <map>
#include <vector>
#include <string>


class Context{
	public:
		class Item{
			public:
				enum Type {NONE, FUNC, CAS, VAR}; // NONE IS FOR ERROR HANDLING

				Type type;
				std::string name;
				std::string subtype;

				bool _mutable = false; // Only used if is variable

				std::vector<std::pair<std::string, bool>> args; // Only used if is function/cas, each argument is a pair of <type, is_mutable>
				Item(){this->type = NONE;};
				Item(Type type, std::string name, std::string sub, std::vector<std::pair<std::string, bool>>args){
					this->type = type; this->name = name; this->subtype = sub; this->args = args;
				};
				~Item(){};

				bool operator ==(Item &other)const{
					return this->type == other.type && this->subtype == other.subtype && this->args == other.args && this->_mutable == other._mutable;
				}
		};
		Context();
		Context(Context *);
		~Context();

		Context *parent;
		Item seek(std::string); // Seek for an item in current and above scopes
		bool has_name_local(std::string); // Checks if current scope has the given name
		bool has_name(std::string);
		bool has(Item &); // Checks if item is defined in the given and above scopes
		void add(Item);

	private:
		std::map<const std::string, Item> table;
};
