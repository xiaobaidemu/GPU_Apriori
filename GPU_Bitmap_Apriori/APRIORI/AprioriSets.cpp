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
		nodes = pruneNodes(pass);//此处是枝剪以后不会再用的节点，并不是枝剪候选项
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
      nodes = pruneNodes(pass);//此处是枝剪以后不会再用的节点，并不是枝剪候选项

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

		// count support, remove infreq items, sort items by count,对item进行排序，encode items, construct matrix
		//此函数的作用就是对计算单项候选项的支持度，并对其进行枝剪和支持读排序，重构后的矩阵为单项支持度都满足最小支持度,这的&matrix是指matrix的地址，不是引用谢谢
		EncodeBin(dataFn, minSup, &matrix, &oidSup, &numTran, &numFItem, &matrixSize, &oidSupSize);
		//trie指向一组Item集合，此时trie应该是root
		trie->Increment(numTran);
		set<Item> *items = trie->makeChildren();//注意：children =items

		//  store mapping of old and new item id, insert freq items into trie
		// ！！！！！注意items是赋予了项新的id后的树，而oidSu数组中包含的是原本项id和该项对应的支持度的值
		//！！！！！并将这种原id和新id的对应关系插入到以Element为元素的数组中
		oidNid = new set<Element>;
		for(size_t i = 0; i < numFItem; i++)
		{
			Item item(i);
			item.Increment(oidSup[i].sup);
			items->insert(item);

			oidNid->insert( Element( oidSup[i].id, i ) );
		}

		//write patterns into file，将频繁模式写入文件中
		printSet(*trie,0,0); 
		for(set<Item>::iterator runner = items->begin(); runner != items->end(); runner++) 
		{			
			int itemset = runner->getId();
			printSet(*runner, &itemset, level);	
		}
	}
	
	else if(level==2) {
		//从level==2开始就应该开始使用GPU进行运算了，感觉numFItem其实就是长度为1的频繁项集的个数（不是感觉是一定）
		//  count support of 2-candidates (all class pairs), insert freq 2-itemsets into trie
		PairwiseCount(matrix, numTran, numFItem, minSup, trie, &num2FI);//此时已经计算玩了2-freItem,其中trie和num2FI 为出口参数

		//write patterns into file
		set<Item> *items = trie->getChildren();
		int * itemset = new int[level];//应该是用来记录项集中每个项的编号，这里是Fre2Item，用一个数组itemset来记录频繁项集
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

		unsigned short* m = (unsigned short*)matrix;//unsigned short是两个字节
		initGPUMiner( m, numFItem, numTran );//当前的m为所有长度为1的频繁项所构成的集合		

	}
	/*
	else if (level == 2)
	{
		unsigned short* m = (unsigned short*)matrix;//unsigned short是两个字节
		initGPUMiner( m, numFItem, numTran );//当前的m为所有长度为1的频繁项所构成的集合	
		numKernelCall = 0;
		set<Item> *items = trie->getChildren();
		int *bParentList = new int[BOUND];//bound=1000;BOUND=2000，在main函数中设置的
		vector<Item*> bItemList;
		int *bItemLen = new int[BOUND];

		set<Item>::iterator runner = items->begin();
		int bParentOffset = 0;//bparentOffset对应的就是每次内循环结束时，父项的位置，见打印txt
		//int j=0;
		while(runner!= items->end())
		{
			int bNumPair = 0;							// number of parent child pairs process together,先假设level = 3
			size_t bNumItem = 0;

			bItemList.clear();//清空这一步很重要
			//int i = 0;
			while(runner!= items->end() && runner->getChildren() && bNumItem < bound)
			{
				bParentList[bNumPair] = runner->getId(); //bParentList[bNumPair]应该表示：深度为1的第bNumPair项中的项的id
				set<Item>* children = runner->getChildren();
				for(set<Item>::iterator cIt = children->begin(); cIt != children->end(); cIt++) 
				{
					Item &item = *cIt;
					bItemList.push_back(&item);//将当前runner的全部子item放入bItemList
				}
				bItemLen[bNumPair] = children->size();//bItemLen[bNumPair]表示：深度为1的第bNumPair项中含有子项的个数（每个父项对应子项的长度）
				bNumItem += runner->getChildren()->size();//bNumItem表示：全部深度为1的项中含有的子项的总数（认为是子项的总长度）
				bNumPair ++;//bNumPair是记录父项的个数的
				runner ++;//在此处对runner进行自加
				bParentOffset ++;//
			}
			countSupportsFor2(level, 2, bParentList, bItemLen, bNumPair, &bItemList, bNumItem, bParentOffset);
		}
		//printf("\n 外循环次数 = %d",j);
		delete [] bParentList;
		delete [] bItemLen;

	}*/
	else // level >= 3
	{
		numKernelCall = 0;

		set<Item> *items = trie->getChildren();
		//printf("\nitemsSize = %d",items->size());

		int *bParentList = new int[BOUND];//bound=1000;BOUND=2000，在main函数中设置的
		//printf("\tbound = %d",bound);
		vector<Item*> bItemList;
		int *bItemLen = new int[BOUND];

		set<Item>::iterator runner = items->begin();
		int bParentOffset = 0;//bparentOffset对应的就是每次内循环结束时，父项的位置，见打印txt
		//int j=0;
		while(runner!= items->end())
		{
			int bNumPair = 0;							// number of parent child pairs process together,先假设level = 3
			size_t bNumItem = 0;
 
			bItemList.clear();//清空这一步很重要
			//int i = 0;
			while(runner!= items->end() && runner->getChildren() && bNumItem < bound)
			{
				bParentList[bNumPair] = runner->getId(); //bParentList[bNumPair]应该表示：深度为1的第bNumPair项中的项的id

				set<Item>* children = runner->getChildren();
				for(set<Item>::iterator cIt = children->begin(); cIt != children->end(); cIt++) 
				{
					Item &item = *cIt;
					bItemList.push_back(&item);//将当前runner的全部子item放入bItemList
				}


				bItemLen[bNumPair] = children->size();//bItemLen[bNumPair]表示：深度为1的第bNumPair项中含有子项的个数（每个父项对应子项的长度）

				bNumItem += runner->getChildren()->size();//bNumItem表示：全部深度为1的项中含有的子项的总数（认为是子项的总长度）
				bNumPair ++;//bNumPair是记录父项的个数的

				runner ++;//在此处对runner进行自加
				bParentOffset ++;//
				//i++;
			}
			//printf("\n 内循环次数 = %d ;bNumItem = %d ;bNumPair = %d;bParentOffset = %d ",i,bNumItem,bNumPair,bParentOffset);
			//下面函数主要传递的参数有：（bParentList,bItemLen,这两个数组），vector bItemList的指针，bParentOffset感觉应该就是指父项(深度为1的项)的个数吧
			//每次应该只能计算bound以内的数目
			countSupports(level, 2, bParentList, bItemLen, bNumPair, &bItemList, bNumItem, bParentOffset);
			//bParentList 和 bItemList 应该是相对应的，前者记录的是父节点所包含item的id,后者记录的是全部父节点所包含全部子项的item信息
			//j++;
		}

		//printf("\n 外循环次数 = %d",j);
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
		//继续计算支持度
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
		cout << "内存分配出现异常" << endl;
	}
	numKernelCall ++;
}
void AprioriSets::countSupports(int level, int depth, int *parentList, int *itemLen, int numPair, vector<Item*>* itemList, size_t numItem, int parentOffset)
{
	if (itemList == 0) return;//表示当父项没有子项时，即没有更多候选项了，进行返回

	int* itemIdList = new int[BOUND];
	for(int i=0; i<itemList->size(); i++)
		itemIdList[i] = ((*itemList)[i])->getId();//将全部的子项的itemId存入itemIdList数组中


	int * bParentList = new int[numPair];
	for( int i = 0; i < numPair; i++ )
			bParentList[i] = i;//初始化bParentList数组，此数组没有实质意义

	if( depth < level)
	{
		if(depth == 2)//字典树遍历的深度为2时，就是有两个item
		{
			/*原型函数void gpuBoundVectorAnd( const int midRltBeginPos, const int *parentList, const int *itemLenList, const int pairNum,
			 *		const int *itemIdList, const int itemIdListLen, const int midRltStoreLevel, 
			 *		const bool countSup, int *supList)
			 *      And&交运算左边：int midRltBeginPos, const int *parentList，-1表示parentList->matrix
			 *                右边：itemIdList: matrix，int *itemLenList, int pairNum
			 */
			gpuBoundVectorAnd( -1, parentList, itemLen, numPair, itemIdList, numItem, depth, false, 0);
		}
		else
		{
			//事实上bParentList这个参数是可以不要的
			gpuBoundVectorAnd( parentOffset, bParentList, itemLen, numPair, itemIdList, numItem, depth, false, 0); 
		}

		int *bItemList = new int[BOUND];//bParentList
		vector<Item*> bChildList;//bItemList
		int *bChildLen = new int[BOUND];//bItemLen
		int index = 0;
		int bItemOffset = 0;//bParentOffset

		while(index < numItem)//深度为depth所含的项的总个数
		{
			bItemOffset = index;//这里和之前的bParentOffset其实是一样的

			bChildList.clear();
			int bNumPair = 0;							// number of parent child pairs process together父子对的个数
			size_t bNumChild = 0;//bNumItem

			Item* curItem = (*itemList)[index];//到时候会对index逐次加1，等于说是遍历了一遍深度为depth的项
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
		int *supList = new int[numItem];//这里的numItem 事实上是参数bNumChild,将支持度值记录在supList数组中
		//parentOffset即上面的bItemOffset,注意bParentList不是上面的bItemList
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
		//printf("\n枝剪了节点之后没有枝剪候选项之前：trie->getChildren() = %d\n",trie->getChildren()->size());
	left = pruneCandidates(level,trie->getChildren(),1,tmp);
	delete [] tmp;

	return left;
}


//prune itemsets whose sup < minsup
int AprioriSets::pruneCandidates(int level, set<Item> *items, int depth, int *itemset)
{
	if(items == 0) return 0;
	int left = 0; //itemset # that are not pruned, i.e., remained as frequent left记录枝剪后频繁项的数目

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
				//now==0则直接执行枝剪节点的过程
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
	int count = 0;//记录产生多少2候选项
	for(set<Item>::iterator runner = items->begin(); runner != items->end(); runner++)
	{
		set<Item> *children = runner->makeChildren();//为每一个trie的子集都创建一个set集
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
		//printf("\n没有枝剪节点前：trie->getChildren() = %d\n",trie->getChildren()->size());
	int generated = generateCandidates(level, trie->getChildren(), 1, tmp);
	delete [] tmp;

	return generated;
}
/*
 * int depth 表示当前items所处在字典树的深度，例如上面指的是处于深度为1的地方
 */
int AprioriSets::generateCandidates(int level, set<Item> *items, int depth, int *current)
{
	if(items == 0) return 0;
	int generated = 0;
	set<Item>::reverse_iterator runner;

	if(depth == level-1) {//判断是否到倒数第二个层级
		for(runner = items->rbegin(); runner != items->rend(); runner++) {
			current[depth-1] = runner->getId();
			//之后的步骤就是生成最后一个item,如level==3,则以下步骤就是生成第三层的节点
			set<Item> *children = runner->makeChildren();
			//和有兄弟一起生成候选项it != runner，表示不能有相同的项在候选项中
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
			generated += generateCandidates(level, runner->getChildren(), depth+1, current);//递归生成候选项，深度+1
		}
	}
	return generated;
}

//check whether subsets of a itemset is on trie
/*
 * 从items(即trie)开始检测iset,看是否iset的子集存在在trie中的前level-1层
 * checkSubsets(level,current, trie->getChildren(), 0, 1)
 */
bool AprioriSets::checkSubsets(int sl, int *iset, set<Item> *items, int spos, int depth)
{
	if(items==0) return 0;

	bool ok = true;
	set<Item>::iterator runner;
	int loper = spos;
	spos = depth+1;
	//使用递归的方法来进行子集查询
	/*
	 * depth 在这里的用途应该就是保证是验证的第k-1项，所以会有判断depth<level-1
	 */
	while(ok && (--spos >= loper)) {
		runner = items->find(Item(iset[spos]));//find找到包含iset[spos]的这一项，并返回对对应的iterator
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
 *枝剪掉depth==level时，depth-1的子节点仍为空的节点 ，因为无法之后产生候选项将不再使用这些节点，
 * 因为无法使用没有子节点的节点产生level+1的候选项，另外候选项的checkSubset之后也不会用到这些节点
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
				set<Item>::iterator tmp = runner++;//注意这个是先赋值后自加，所以tmp就是当前的runner
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
