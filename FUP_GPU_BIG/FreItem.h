#ifndef FREITEM_H
#define FREITEM_H
#include <deque>
#include <iostream>
using namespace std;
class FreItem
{
public:
	deque<int> frekey;
	//int frenum;
	//int frearry[] = new int[frenum];
	FreItem();
	FreItem(deque<int> fk);
	bool operator == (const FreItem &FI)const;
	//bool operator <= (const FreItem &FI)const;
	~FreItem();

};

#endif