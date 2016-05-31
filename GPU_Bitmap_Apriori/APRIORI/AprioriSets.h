#pragma once

#include <set>
#include <vector>
#include <fstream>
#include <hash_map>
#include <time.h>
#include "Item.h"
#include "../Kernel/GPUMiner_Kernel.h"
#include "../L1L2/FIMEncode.h"


using namespace std;


class Element
{
public:
	Element(int iold, int inew=0) : oldid(iold), id(inew){}

	int oldid; // id in fn 原本项的id
	int id;    // new id ordered by desc support 在排序后赋予item新的id,从0开始
	bool operator< (const Element  &e) const {return oldid < e.oldid;}
};

class AprioriSets
{
public:
	AprioriSets(void);
	~AprioriSets(void);

	void setData(char *fn);
	int setOutputSets(char *fn);
	void setBound(int b) {bound = b; BOUND = bound*2; }
	
	void setMinSup(int ms){minSup = ms;}

	// print debug information
	void setVerbose(){verbose=true;}	

	int generateSets();

	clock_t getPrintTime() { return printTime;}

protected:
	int generateCandidates(int level);
	int generateCandidatesFor2(int level,set<Item> *items,size_t numItem);
	int generateCandidates(int level, set<Item> *items, int depth, int *current);
	bool checkSubsets(int sl, int *iset, set<Item> *items, int spos, int depth);

	//remove "dead-end" nodes from trie
	int pruneNodes(int level);
	int pruneNodes(int level, set<Item> *items, int depth);

	void countSupports(int level);
	void countSupports(int level, int depth, int *parentList, int *itemLen, int numPair, vector<Item*>* itemList, size_t numItem, int parentOffset);
	void countSupportsFor2(int level, int depth, int *parentList, int *itemLen, int numPair, vector<Item*>* itemList, size_t numItem, int parentOffset);
	
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
	Item *trie; //pointer to a set of items

	size_t numFItem; // freq item#
	size_t numTran;  // input transaction#
	size_t num2FI;   // freq 2-itemse#
	size_t numKernelCall;
	
	IdSup_t *oidSup;    //oidSup[new item id in fn] = {new item id in mining, its support}
	size_t oidSupSize;
	set<Element> *oidNid; //<old item id in fn, new item id in mining>

	int bound; // number of intersection each GPU kernel call can perform每一个GPU内核可以执行的交运算的数量
	int BOUND; // BOUND = bound*2

	clock_t printTime;
};
