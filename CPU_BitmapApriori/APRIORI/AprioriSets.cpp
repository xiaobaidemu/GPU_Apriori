#include <algorithm>
#include <iostream>

#include "AprioriSets.h"

using namespace std;

AprioriSets::AprioriSets()
{
	minSup = 0;
	dataFn = NULL;
	
	verbose = true;

	matrix = NULL;
	matrixSize = 0;
	shortBase = 0;

	trie = new Item(-1);  // item id of root: -1

	numFItem = 0;
	numTran = 0;
	num2FI = 0;

	oidSup = NULL;    //oidSup[new item id in fn] = {new item id in mining, its support}
	oidSupSize = 0;
	oidNid = NULL; //<odl item id in fn, new item id in mining>

	printTime = 0;
}

AprioriSets::~AprioriSets()
{
	if(trie) {
		trie->deleteChildren();
		delete trie;
	}

	BenFree((char**)(&matrix), matrixSize);
	BenFree((char**)(&oidSup), oidSupSize);

	if(oidNid) delete oidNid;
}

//input transaction set file
//binary file
//transaction set in the vertical matrix format
void AprioriSets::setData(char *fn)
{
	dataFn = fn;
}

int AprioriSets::setOutputSets(char *fn)
{
	setsout.open(fn);
	if(!setsout.is_open()) {
		cerr << "error: could not open " << fn << endl;
		return -1;
	}
	return 0;
}

//pass=level: the length of frequent itemset
int AprioriSets::generateSets()
{
  int total=0, pass=0;
  bool running = true;

  while(running) {
    clock_t start;
    int generated=0, nodes=0, left=0; //tnr: transaction number; pruned: item# after prune

    pass++;
	cout << "----------pass: " << pass << " ----------" << endl;

    if(pass>2) 
	{
      start = clock();
      generated = generateCandidates(pass);
      nodes = pruneNodes(pass);

  	  if(verbose) 
		cout << "candidate#: " << generated << endl
			 << "\t[candidate generation time: " << (clock()-start)/double(CLOCKS_PER_SEC) << "s] " << endl;	
    }

    start = clock();
    countSupports(pass);
    if(verbose) 
	{
		if(pass == 1)
		{
			cout << "pattern#: " << numFItem << "   " << "transaction#: " << numTran << endl;
			left = numFItem;
		}
		else if(pass == 2)
		{
			cout << "pattern#: " << num2FI << endl;
			left = num2FI;
		}

		cout << "\t[count support time: " << (clock()-start)/double(CLOCKS_PER_SEC) << "s] " << "   " << endl;
	}

    //if(pass==1 && setsout.is_open()) printSet(*trie,0,0); 

    if(pass>2) 
	{	
		start = clock();
		left = pruneCandidates(pass);
		if(verbose) 
		{
			cout << "pattern#: " << left << endl
				 << "\t[pattern identification time: " << (clock()-start)/double(CLOCKS_PER_SEC) << "s]" << endl;
		}
	}

    total += left;
    if(left <= pass) running = false;
  }

  cout << endl;

  return total;
}

//count the supports of k-candidates, k is level
void AprioriSets::countSupports(int level)
{
	// count all single items
	if(level==1) {

		// count support, remove infreq items, sort items by count, encode items, construct matrix
		EncodeBin(dataFn, minSup, &matrix, &oidSup, &numTran, &numFItem, &matrixSize, &oidSupSize);
		
		trie->Increment(numTran);
		set<Item> *items = trie->makeChildren();

		//  store mapping of old and new item id, insert freq items into trie
		oidNid = new set<Element>;
		for(size_t i = 0; i < numFItem; i++)
		{
			Item item(i);
			item.Increment(oidSup[i].sup);
			items->insert(item);

			oidNid->insert( Element( oidSup[i].id, i ) );
		}

		//write patterns into file
		clock_t start = clock();
		printSet(*trie,0,0); 
		for(set<Item>::iterator runner = items->begin(); runner != items->end(); runner++) 
		{			
			int itemset = runner->getId();
			printSet(*runner, &itemset, level);	
		}
		printTime += clock() - start;
	}
	else if(level==2) {

		//  count support of 2-candidates (all class pairs), insert freq 2-itemsets into trie
		PairwiseCount(matrix, numTran, numFItem, minSup, trie, &num2FI);

		//write patterns into file
		clock_t start = clock();
		set<Item> *items = trie->getChildren();
		int * itemset = new int[level];
		for(set<Item>::iterator runner1 = items->begin(); runner1 != items->end(); runner1++) 
		{			
			itemset[level-2] = runner1->getId();
			set<Item> *children = runner1->getChildren();
			for(set<Item>::iterator runner2 = children->begin(); runner2 != children->end(); runner2++) 
			{
				itemset[level-1] = runner2->getId();
				if (runner1 == items->begin() && runner2 == children->begin())
				{
					printf("	!!!!!!itemset[0] = %d;itemset[0] = %d",itemset[0],itemset[1]);
				}
				printSet(*runner2, itemset, level);				
			}

		}
		delete [] itemset;
		printTime += clock() - start;

	}
	else // level >= 3
	{
		set<Item> *items = trie->getChildren();

		int byteBase = BenCeilInt(numTran, sizeof(byte)*8);
		shortBase = byteBase / 2 + (size_t)(byteBase & 0x1);

		for(set<Item>::iterator runner = items->begin(); runner != items->end(); runner++) 
		{
			int itemId1 = runner->getId();
			//if(level == 3)
				//printf("	!!!!!!!itemId1 = %d!!!!!!\n",itemId1);
			ushort_t *item1s = (ushort_t*)(matrix + shortBase * 2 * itemId1);

			countSupports(level, runner->getChildren(), 2, item1s);
				
		}
	}
}

void AprioriSets::countSupports(int level, set<Item> * items, int depth, ushort_t *ancestorAndResult)
{
	if(items == 0) return ;

	for(set<Item>::iterator runner = items->begin(); runner != items->end(); runner++) {
		int itemId = runner->getId();

		ushort_t *result = NULL;

		AndPairs(ancestorAndResult, matrix, itemId, shortBase, &result);

		if(depth == level){
			int sup = 0;
			AndResultBitCount(result, shortBase, &sup);
			runner->Increment(sup);		
		}
		else countSupports(level, runner->getChildren(), depth+1, result);

		if(result)
			delete [] result;
	}
}


//after prune, trie remains the left itemsets
int AprioriSets::pruneCandidates(int level)
{
	int left;
	int *tmp = new int[level];
	left = pruneCandidates(level,trie->getChildren(),1,tmp);
	delete [] tmp;

	return left;
}


//prune itemsets whose sup < minsup
int AprioriSets::pruneCandidates(int level, set<Item> *items, int depth, int *itemset)
{
	if(items == 0) return 0;
	int left = 0; //itemset # that are not pruned, i.e., remained as frequent

	for(set<Item>::iterator runner = items->begin(); runner != items->end(); ) {
		itemset[depth-1] = runner->getId();

	    if(depth == level) {
			if(runner->getSupport() < minSup) {
				runner->deleteChildren();
				set<Item>::iterator tmp = runner++; //i.e., tmp=runner, runner++
				items->erase(tmp);
			}
			else 
			{
				clock_t start = clock();
				if(setsout.is_open()) printSet(*runner, itemset, depth);
				printTime += clock() - start;

				left++;
				runner++;
			}
		}
		else {	
			int now = pruneCandidates(level, runner->getChildren(), depth+1, itemset);
			if(now) {
				left += now;
				runner++;
			}
			else {
				runner->deleteChildren();
				set<Item>::iterator tmp = runner++;
				items->erase(tmp);
			}
		}
	}

	return left;
}



//print frequent itemsets into a file
void AprioriSets::printSet(const Item& item, int *itemset, int length)
{
	set<int> outset;

	for(int j=0; j<length; j++) 
		if(oidSup) outset.insert(oidSup[itemset[j]].id); 
		else outset.insert(itemset[j]); 
	for(set<int>::iterator k=outset.begin(); k!=outset.end(); k++)  setsout << *k << " ";
	setsout << "(" << item.getSupport() << ")" << endl;
}

//when level > 2, generate candidates
//DFS traversal of trie
int AprioriSets::generateCandidates(int level)
{
	int *tmp = new int[level];
	int generated = generateCandidates(level, trie->getChildren(), 1, tmp);
	delete [] tmp;

	return generated;
}

int AprioriSets::generateCandidates(int level, set<Item> *items, int depth, int *current)
{
	if(items == 0) return 0;

	int generated = 0;
	set<Item>::reverse_iterator runner;

	if(depth == level-1) {
		for(runner = items->rbegin(); runner != items->rend(); runner++) {
			current[depth-1] = runner->getId();
			set<Item> *children = runner->makeChildren();

			for(set<Item>::reverse_iterator it = items->rbegin(); it != runner; it++) {
				current[depth] = it->getId();
				if(level<=2 || checkSubsets(level,current, trie->getChildren(), 0, 1)) {
					children->insert(Item(it->getId()));
					generated++;
				}
			}
		}
	}
	else {
		for(runner = items->rbegin(); runner != items->rend(); runner++) {
			current[depth-1] = runner->getId();
			generated += generateCandidates(level, runner->getChildren(), depth+1, current);
		}
	}
	return generated;
}

//check whether subsets of a itemset is on trie
bool AprioriSets::checkSubsets(int sl, int *iset, set<Item> *items, int spos, int depth)
{
	if(items==0) return 0;

	bool ok = true;
	set<Item>::iterator runner;
	int loper = spos;
	spos = depth+1;

	while(ok && (--spos >= loper)) {
		runner = items->find(Item(iset[spos]));
		if(runner != items->end()) {
			if(depth<sl-1) ok = checkSubsets(sl, iset, runner->getChildren(), spos+1, depth+1);
		}
		else ok=false;
	}

	return ok;
}

//prune "dead end" nodes
int AprioriSets::pruneNodes(int level)
{
	return pruneNodes(level,trie->getChildren(),1);
}

int AprioriSets::pruneNodes(int level, set<Item> *items, int depth)
{
	if(items == 0) return 0;

	int nodes = 0;

	if(depth==level) nodes = (int)items->size();
	else {
		for(set<Item>::iterator runner = items->begin(); runner != items->end(); ) {
			int now = pruneNodes(level, runner->getChildren(), depth+1);
			if(now) {
				nodes += now;
				nodes++;
				runner++;
			}
			else {
				runner->deleteChildren();
				set<Item>::iterator tmp = runner++;
				items->erase(tmp);
			}
		}
	}

	return nodes;
}

int AprioriSets::countL2Nodes()
{
	set<Item> *items = trie->getChildren();
	int count = 0;
	for(set<Item>::iterator runner = items->begin(); runner != items->end(); )
		count += (int) runner->getChildren()->size();
	return count;
}
