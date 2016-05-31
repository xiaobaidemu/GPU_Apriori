#include <time.h>
#include <algorithm>
#include <iostream>

#include "AprioriSets.h"
#include "../Kernel/GPUMiner_Kernel.h"

using namespace std;

AprioriSets::AprioriSets()
{
	minSup = 0;
	dataFn = NULL;
	
	verbose = true;

	matrix = NULL;
	matrixSize = 0;
	trie = new Item(-1);  // item id of root: -1

	numFItem = 0;
	numTran = 0;
	num2FI = 0;
	numKernelCall = 0;

	oidSup = NULL;    //oidSup[new item id in fn] = {new item id in mining, its support}
	oidSupSize = 0;
	oidNid = NULL; //<odl item id in fn, new item id in mining>

	bound = 10;
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

  clock_t genTime = 0;
  while(running) {
    clock_t start;
    int generated=0, nodes=0, left=0; //tnr: transaction number; pruned: item# after prune

    pass++;
	cout << "----------pass: " << pass << " ----------" << endl;
	setsout << "*****" <<endl;
	clock_t genTimer = 0;
	/*
	if (pass == 2)
	{
		start = clock();
		generated = generateCandidatesFor2(pass,trie->getChildren(),numFItem);
		nodes = pruneNodes(pass);//�˴���֦���Ժ󲻻����õĽڵ㣬������֦����ѡ��
		//cout << "NodesLeft=" << nodes <<endl;

		genTimer = (clock()-start);
		if(verbose) 
			cout << "candidate#: " << generated << endl
			<< "\t[candidate generation time: " << (clock()-start)/double(CLOCKS_PER_SEC) << "s] " << endl;	

	}
	*/
    if(pass>2) 
	{
      start = clock();
      generated = generateCandidates(pass);
      nodes = pruneNodes(pass);//�˴���֦���Ժ󲻻����õĽڵ㣬������֦����ѡ��

	  genTimer = (clock()-start);
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
		cout << "\t\tkernel call#: " << numKernelCall << endl;
		cout << "\tmemory copy time: " << getCopyTime() << "s" << endl;
		cout << "\tsum on the cpu time: " << getCountTime() << "s" << endl;
		cout << "\tkernel time: " << getKernelTime() << "s" << endl;
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
		genTime += genTimer;
	}

    total += left;
    if(left <= pass) running = false;
  }

  cout << endl;
  
  cout << "generation time: " << genTime/double(CLOCKS_PER_SEC) << "s" << endl;

  return total;
}


//count the supports of k-candidates, k is level
void AprioriSets::countSupports(int level)
{
	// count all single items
	if(level==1) {

		// count support, remove infreq items, sort items by count,��item��������encode items, construct matrix
		//�˺��������þ��ǶԼ��㵥���ѡ���֧�ֶȣ����������֦����֧�ֶ������ع���ľ���Ϊ����֧�ֶȶ�������С֧�ֶ�,���&matrix��ָmatrix�ĵ�ַ����������лл
		EncodeBin(dataFn, minSup, &matrix, &oidSup, &numTran, &numFItem, &matrixSize, &oidSupSize);
		//trieָ��һ��Item���ϣ���ʱtrieӦ����root
		trie->Increment(numTran);
		set<Item> *items = trie->makeChildren();//ע�⣺children =items

		//  store mapping of old and new item id, insert freq items into trie
		// ����������ע��items�Ǹ��������µ�id���������oidSu�����а�������ԭ����id�͸����Ӧ��֧�ֶȵ�ֵ
		//������������������ԭid����id�Ķ�Ӧ��ϵ���뵽��ElementΪԪ�ص�������
		oidNid = new set<Element>;
		for(size_t i = 0; i < numFItem; i++)
		{
			Item item(i);
			item.Increment(oidSup[i].sup);
			items->insert(item);

			oidNid->insert( Element( oidSup[i].id, i ) );
		}

		//write patterns into file����Ƶ��ģʽд���ļ���
		printSet(*trie,0,0); 
		for(set<Item>::iterator runner = items->begin(); runner != items->end(); runner++) 
		{			
			int itemset = runner->getId();
			printSet(*runner, &itemset, level);	
		}
	}
	
	else if(level==2) {
		//��level==2��ʼ��Ӧ�ÿ�ʼʹ��GPU���������ˣ��о�numFItem��ʵ���ǳ���Ϊ1��Ƶ����ĸ��������Ǹо���һ����
		//  count support of 2-candidates (all class pairs), insert freq 2-itemsets into trie
		PairwiseCount(matrix, numTran, numFItem, minSup, trie, &num2FI);//��ʱ�Ѿ���������2-freItem,����trie��num2FI Ϊ���ڲ���

		//write patterns into file
		set<Item> *items = trie->getChildren();
		int * itemset = new int[level];//Ӧ����������¼���ÿ����ı�ţ�������Fre2Item����һ������itemset����¼Ƶ���
		for(set<Item>::iterator runner1 = items->begin(); runner1 != items->end(); runner1++) 
		{			
			itemset[level-2] = runner1->getId();
			set<Item> *children = runner1->getChildren();
			for(set<Item>::iterator runner2 = children->begin(); runner2 != children->end(); runner2++) 
			{
				itemset[level-1] = runner2->getId();
				printSet(*runner2, itemset, level);				
			}
		}
		delete [] itemset;

		unsigned short* m = (unsigned short*)matrix;//unsigned short�������ֽ�
		initGPUMiner( m, numFItem, numTran );//��ǰ��mΪ���г���Ϊ1��Ƶ���������ɵļ���		

	}
	/*
	else if (level == 2)
	{
		unsigned short* m = (unsigned short*)matrix;//unsigned short�������ֽ�
		initGPUMiner( m, numFItem, numTran );//��ǰ��mΪ���г���Ϊ1��Ƶ���������ɵļ���	
		numKernelCall = 0;
		set<Item> *items = trie->getChildren();
		int *bParentList = new int[BOUND];//bound=1000;BOUND=2000����main���������õ�
		vector<Item*> bItemList;
		int *bItemLen = new int[BOUND];

		set<Item>::iterator runner = items->begin();
		int bParentOffset = 0;//bparentOffset��Ӧ�ľ���ÿ����ѭ������ʱ�������λ�ã�����ӡtxt
		//int j=0;
		while(runner!= items->end())
		{
			int bNumPair = 0;							// number of parent child pairs process together,�ȼ���level = 3
			size_t bNumItem = 0;

			bItemList.clear();//�����һ������Ҫ
			//int i = 0;
			while(runner!= items->end() && runner->getChildren() && bNumItem < bound)
			{
				bParentList[bNumPair] = runner->getId(); //bParentList[bNumPair]Ӧ�ñ�ʾ�����Ϊ1�ĵ�bNumPair���е����id
				set<Item>* children = runner->getChildren();
				for(set<Item>::iterator cIt = children->begin(); cIt != children->end(); cIt++) 
				{
					Item &item = *cIt;
					bItemList.push_back(&item);//����ǰrunner��ȫ����item����bItemList
				}
				bItemLen[bNumPair] = children->size();//bItemLen[bNumPair]��ʾ�����Ϊ1�ĵ�bNumPair���к�������ĸ�����ÿ�������Ӧ����ĳ��ȣ�
				bNumItem += runner->getChildren()->size();//bNumItem��ʾ��ȫ�����Ϊ1�����к��е��������������Ϊ��������ܳ��ȣ�
				bNumPair ++;//bNumPair�Ǽ�¼����ĸ�����
				runner ++;//�ڴ˴���runner�����Լ�
				bParentOffset ++;//
			}
			countSupportsFor2(level, 2, bParentList, bItemLen, bNumPair, &bItemList, bNumItem, bParentOffset);
		}
		//printf("\n ��ѭ������ = %d",j);
		delete [] bParentList;
		delete [] bItemLen;

	}*/
	else // level >= 3
	{
		numKernelCall = 0;

		set<Item> *items = trie->getChildren();
		//printf("\nitemsSize = %d",items->size());

		int *bParentList = new int[BOUND];//bound=1000;BOUND=2000����main���������õ�
		//printf("\tbound = %d",bound);
		vector<Item*> bItemList;
		int *bItemLen = new int[BOUND];

		set<Item>::iterator runner = items->begin();
		int bParentOffset = 0;//bparentOffset��Ӧ�ľ���ÿ����ѭ������ʱ�������λ�ã�����ӡtxt
		//int j=0;
		while(runner!= items->end())
		{
			int bNumPair = 0;							// number of parent child pairs process together,�ȼ���level = 3
			size_t bNumItem = 0;
 
			bItemList.clear();//�����һ������Ҫ
			//int i = 0;
			while(runner!= items->end() && runner->getChildren() && bNumItem < bound)
			{
				bParentList[bNumPair] = runner->getId(); //bParentList[bNumPair]Ӧ�ñ�ʾ�����Ϊ1�ĵ�bNumPair���е����id

				set<Item>* children = runner->getChildren();
				for(set<Item>::iterator cIt = children->begin(); cIt != children->end(); cIt++) 
				{
					Item &item = *cIt;
					bItemList.push_back(&item);//����ǰrunner��ȫ����item����bItemList
				}


				bItemLen[bNumPair] = children->size();//bItemLen[bNumPair]��ʾ�����Ϊ1�ĵ�bNumPair���к�������ĸ�����ÿ�������Ӧ����ĳ��ȣ�

				bNumItem += runner->getChildren()->size();//bNumItem��ʾ��ȫ�����Ϊ1�����к��е��������������Ϊ��������ܳ��ȣ�
				bNumPair ++;//bNumPair�Ǽ�¼����ĸ�����

				runner ++;//�ڴ˴���runner�����Լ�
				bParentOffset ++;//
				//i++;
			}
			//printf("\n ��ѭ������ = %d ;bNumItem = %d ;bNumPair = %d;bParentOffset = %d ",i,bNumItem,bNumPair,bParentOffset);
			//���溯����Ҫ���ݵĲ����У���bParentList,bItemLen,���������飩��vector bItemList��ָ�룬bParentOffset�о�Ӧ�þ���ָ����(���Ϊ1����)�ĸ�����
			//ÿ��Ӧ��ֻ�ܼ���bound���ڵ���Ŀ
			countSupports(level, 2, bParentList, bItemLen, bNumPair, &bItemList, bNumItem, bParentOffset);
			//bParentList �� bItemList Ӧ�������Ӧ�ģ�ǰ�߼�¼���Ǹ��ڵ�������item��id,���߼�¼����ȫ�����ڵ�������ȫ�������item��Ϣ
			//j++;
		}

		//printf("\n ��ѭ������ = %d",j);
		delete [] bParentList;
		delete [] bItemLen;
	}
}

void AprioriSets::countSupportsFor2(int level, int depth, int *parentList, int *itemLen, int numPair, vector<Item*>* itemList, size_t numItem, int parentOffset)
{
	if (itemList == 0)
	{
		return;
	}
	int* itemIdList = new int[BOUND];
	for(int i=0; i < itemList->size(); i++)
	{
		itemIdList[i] = ((*itemList)[i])->getId();
	}	
	try
	{
		int *supList = new int[numItem];
		gpuBoundVectorAnd(-1,parentList, itemLen, numPair, itemIdList, numItem, depth, true, supList);
		//��������֧�ֶ�
		for(int i=0; i<itemList->size(); i++)
		{
			Item *item = (*itemList)[i];
			item->Increment(supList[i]);
		}
		/*
		for(int i = 0;i < 3;i++)
		{
			cout << supList[i] << " ";
		}*/
		//cout << endl ;
		delete[] supList;	
		//free(supList);
		delete[] itemIdList;
	}
	catch (const std::bad_alloc& e)
	{
		cout << "�ڴ��������쳣" << endl;
	}
	numKernelCall ++;
}
void AprioriSets::countSupports(int level, int depth, int *parentList, int *itemLen, int numPair, vector<Item*>* itemList, size_t numItem, int parentOffset)
{
	if (itemList == 0) return;//��ʾ������û������ʱ����û�и����ѡ���ˣ����з���

	int* itemIdList = new int[BOUND];
	for(int i=0; i<itemList->size(); i++)
		itemIdList[i] = ((*itemList)[i])->getId();//��ȫ���������itemId����itemIdList������


	int * bParentList = new int[numPair];
	for( int i = 0; i < numPair; i++ )
			bParentList[i] = i;//��ʼ��bParentList���飬������û��ʵ������

	if( depth < level)
	{
		if(depth == 2)//�ֵ������������Ϊ2ʱ������������item
		{
			/*ԭ�ͺ���void gpuBoundVectorAnd( const int midRltBeginPos, const int *parentList, const int *itemLenList, const int pairNum,
			 *		const int *itemIdList, const int itemIdListLen, const int midRltStoreLevel, 
			 *		const bool countSup, int *supList)
			 *      And&��������ߣ�int midRltBeginPos, const int *parentList��-1��ʾparentList->matrix
			 *                �ұߣ�itemIdList: matrix��int *itemLenList, int pairNum
			 */
			gpuBoundVectorAnd( -1, parentList, itemLen, numPair, itemIdList, numItem, depth, false, 0);
		}
		else
		{
			//��ʵ��bParentList��������ǿ��Բ�Ҫ��
			gpuBoundVectorAnd( parentOffset, bParentList, itemLen, numPair, itemIdList, numItem, depth, false, 0); 
		}

		int *bItemList = new int[BOUND];//bParentList
		vector<Item*> bChildList;//bItemList
		int *bChildLen = new int[BOUND];//bItemLen
		int index = 0;
		int bItemOffset = 0;//bParentOffset

		while(index < numItem)//���Ϊdepth����������ܸ���
		{
			bItemOffset = index;//�����֮ǰ��bParentOffset��ʵ��һ����

			bChildList.clear();
			int bNumPair = 0;							// number of parent child pairs process together���ӶԵĸ���
			size_t bNumChild = 0;//bNumItem

			Item* curItem = (*itemList)[index];//��ʱ����index��μ�1������˵�Ǳ�����һ�����Ϊdepth����
			while(index < numItem && curItem->getChildren() && bNumChild < bound)
			{
				bItemList[bNumPair] = curItem->getId();
                
				set<Item>* children = curItem->getChildren();
				for(set<Item>::iterator cIt = children->begin(); cIt != children->end(); cIt++) 
				{
					Item &item = *cIt;
					bChildList.push_back(&item);
				}

				bChildLen[bNumPair] = children->size();

				bNumChild += children->size();
				bNumPair ++;				

				index ++;
				if(index < numItem)
					curItem = (*itemList)[index];

			}
			//printf("  bItemOffset = %d  ",bItemOffset);
			countSupports(level, depth+1, bItemList, bChildLen, bNumPair, &bChildList, bNumChild, bItemOffset);
			//countSupports(level, 2, bParentList, bItemLen, bNumPair, &bItemList, bNumItem, bParentOffset);
		}

		delete [] bItemList;
		delete [] bChildLen;
	}
	else
	{
		int *supList = new int[numItem];//�����numItem ��ʵ���ǲ���bNumChild,��֧�ֶ�ֵ��¼��supList������
		//parentOffset�������bItemOffset,ע��bParentList���������bItemList
		gpuBoundVectorAnd(parentOffset, bParentList, itemLen, numPair, itemIdList, numItem, depth, true, supList);

		for(int i=0; i<itemList->size(); i++)
		{
			Item *item = (*itemList)[i];
			item->Increment(supList[i]);
		}

		delete [] supList;	
	}
	numKernelCall ++;
}

//after prune, trie remains the left itemsets
int AprioriSets::pruneCandidates(int level)
{
	int left;
	int *tmp = new int[level];
	//if(level == 3)
		//printf("\n֦���˽ڵ�֮��û��֦����ѡ��֮ǰ��trie->getChildren() = %d\n",trie->getChildren()->size());
	left = pruneCandidates(level,trie->getChildren(),1,tmp);
	delete [] tmp;

	return left;
}


//prune itemsets whose sup < minsup
int AprioriSets::pruneCandidates(int level, set<Item> *items, int depth, int *itemset)
{
	if(items == 0) return 0;
	int left = 0; //itemset # that are not pruned, i.e., remained as frequent left��¼֦����Ƶ�������Ŀ

	for(set<Item>::iterator runner = items->begin(); runner != items->end(); ) {
		itemset[depth-1] = runner->getId();

	    if(depth == level) {
			if(runner->getSupport() < minSup) {
				runner->deleteChildren();
				set<Item>::iterator tmp = runner++; //i.e., tmp=runner, runner++
				items->erase(tmp);
			}
			else {
				if(setsout.is_open()) printSet(*runner, itemset, depth);
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
				//now==0��ֱ��ִ��֦���ڵ�Ĺ���
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
	//setsout << "(" << item.getSupport() << ")" << endl;
	setsout << item.getSupport() <<  endl;
}

int AprioriSets::generateCandidatesFor2(int level,set<Item> *items,size_t numItem)
{
	int count = 0;//��¼��������2��ѡ��
	for(set<Item>::iterator runner = items->begin(); runner != items->end(); runner++)
	{
		set<Item> *children = runner->makeChildren();//Ϊÿһ��trie���Ӽ�������һ��set��
		int i = runner->getId();
		for (int j = i+1; j < numItem; j++)
		{
			Item item(j);
			children->insert(item);
			count ++;
		}
	}
	return count;

}
//when level > 2, generate candidates
//DFS traversal of trie
int AprioriSets::generateCandidates(int level)
{
	int *tmp = new int[level];
	//if(level==3)
		//printf("\nû��֦���ڵ�ǰ��trie->getChildren() = %d\n",trie->getChildren()->size());
	int generated = generateCandidates(level, trie->getChildren(), 1, tmp);
	delete [] tmp;

	return generated;
}
/*
 * int depth ��ʾ��ǰitems�������ֵ�������ȣ���������ָ���Ǵ������Ϊ1�ĵط�
 */
int AprioriSets::generateCandidates(int level, set<Item> *items, int depth, int *current)
{
	if(items == 0) return 0;
	int generated = 0;
	set<Item>::reverse_iterator runner;

	if(depth == level-1) {//�ж��Ƿ񵽵����ڶ����㼶
		for(runner = items->rbegin(); runner != items->rend(); runner++) {
			current[depth-1] = runner->getId();
			//֮��Ĳ�������������һ��item,��level==3,�����²���������ɵ�����Ľڵ�
			set<Item> *children = runner->makeChildren();
			//�����ֵ�һ�����ɺ�ѡ��it != runner����ʾ��������ͬ�����ں�ѡ����
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
			generated += generateCandidates(level, runner->getChildren(), depth+1, current);//�ݹ����ɺ�ѡ����+1
		}
	}
	return generated;
}

//check whether subsets of a itemset is on trie
/*
 * ��items(��trie)��ʼ���iset,���Ƿ�iset���Ӽ�������trie�е�ǰlevel-1��
 * checkSubsets(level,current, trie->getChildren(), 0, 1)
 */
bool AprioriSets::checkSubsets(int sl, int *iset, set<Item> *items, int spos, int depth)
{
	if(items==0) return 0;

	bool ok = true;
	set<Item>::iterator runner;
	int loper = spos;
	spos = depth+1;
	//ʹ�õݹ�ķ����������Ӽ���ѯ
	/*
	 * depth ���������;Ӧ�þ��Ǳ�֤����֤�ĵ�k-1����Ի����ж�depth<level-1
	 */
	while(ok && (--spos >= loper)) {
		runner = items->find(Item(iset[spos]));//find�ҵ�����iset[spos]����һ������ضԶ�Ӧ��iterator
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
/*
 *֦����depth==levelʱ��depth-1���ӽڵ���Ϊ�յĽڵ� ����Ϊ�޷�֮�������ѡ�����ʹ����Щ�ڵ㣬
 * ��Ϊ�޷�ʹ��û���ӽڵ�Ľڵ����level+1�ĺ�ѡ������ѡ���checkSubset֮��Ҳ�����õ���Щ�ڵ�
 */
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
				set<Item>::iterator tmp = runner++;//ע��������ȸ�ֵ���Լӣ�����tmp���ǵ�ǰ��runner
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
