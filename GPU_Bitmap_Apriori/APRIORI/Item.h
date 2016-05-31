
#ifndef ITEM_H
#define ITEM_H

#include <set>
using namespace std;

class Item
{
public:

	Item(int i) : id(i), support(0), children(0) {}
	Item(const Item &i) : id(i.id), support(i.support), children(i.children) {}
	~Item(){}

	int getId() const {return id;}

	int Increment(int inc = 1) const {return support+=inc;}

	set<Item> *makeChildren() const;
	int deleteChildren() const;

	int getSupport() const {return support;}
	set<Item> *getChildren() const {return children;}

	bool operator<(const Item &i) const{return id < i.id;}

private:

	const int id;

	mutable int support;
	mutable set<Item> *children;
};

#endif