#pragma once

#include <set>
#include <vector>
#include <fstream>
#include <hash_map>
#include <time.h>
#include "Item.h"
#include "../L1L2/FIMEncode.h"


using namespace std;

class Element
{
public:
	Element(int iold, int inew=0) : oldid(iold), id(inew){}

	int oldid; // id in fn
	int id;    // new id ordered by desc support
	bool operator< (const Element  &e) const {return oldid < e.oldid;}
};

class AprioriSets
{
public:
	AprioriSets(void);
	~AprioriSets(void);

	void setData(char *fn);
	int setOutputSets(char *fn);
	
	void setMinSup(int ms){minSup=ms;}

	// print debug information
	void setVerbose(){verbose=true;}	

	int generateSets();

	clock_t getPrintTime() { return printTime;}

protected:
	int generateCandidates(int level);
	int generateCandidates(int level, set<Item> *items, int depth, int *current);
	bool checkSubsets(int sl, int *iset, set<Item> *items, int spos, int depth);

	//remove "dead-end" nodes from trie
	int pruneNodes(int level);
	int pruneNodes(int level, set<Item> *items, int depth);

	void countSupports(int level);
	void countSupports(int level, set<Item> * items, int depth, ushort_t *ancestorAndResult);	
	
	int pruneCandidates(int level);
	int pruneCandidates(int level, set<Item> *items, int depth, int *itemset);

	void printSet(const Item& item, int *itemset, int length);

	int countL2Nodes();

private:

	int minSup;
	char *dataFn;
	ofstream setsout;
	
	bool verbose;

	unsigned char *matrix;
	size_t matrixSize;
	size_t shortBase; // number of shorts in the matrix
	
	Item *trie; //pointer to a set of items

	size_t numFItem; // freq item#
	size_t numTran;  // input transaction#
	size_t num2FI;   // freq 2-itemse#
	
	IdSup_t *oidSup;    //oidSup[new item id in fn] = {new item id in mining, its support}
	size_t oidSupSize;
	set<Element> *oidNid; //<old item id in fn, new item id in mining>

	clock_t printTime;
};
