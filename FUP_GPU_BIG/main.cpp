#include <iostream>
#include <fstream>
#include <map>
#include <deque>
#include <vector>
#include "FreItem.h"
#include "Item.h"
#include <time.h>
#include <windows.h>
#include <tchar.h>
#include "GPUMiner_Kernel.h"
//#define debug
#define print
#define FIXED_VALUE 8
#define MIN_SUP 1010
#define BOUND2 2000
#define BOUND 1000
#define NUMITEM 999 
using namespace std;
//使用hash_map和deque的组合，来读取原数据库的频繁项情况
#define NUM 3
//读取记录事务的二进制文件
extern "C"
char *readBin(IN char *fileName);
//打印生成的字典树
void printTree(set<Item> *tree,int level);
//生成字典树
FreItem genTree(map<FreItem,int> FreMap,int itemcount,set<Item> *items,FreItem FI);
//删除死节点
int pruneNodes(int level,set<Item> *items,int depth);
//向上求整
int BenCeilInt(int x, int y)
{
	return (x/y + (int)(x%y!=0));
}
void countSupports(int level, int depth, int *parentList, int *itemLen, int numPair, vector<Item*>* itemList, size_t numItem, int parentOffset);
void countSupportsOver2(int level, int depth, int *parentList, int *itemLen, int numPair, vector<Item*>* itemList, size_t numItem, int parentOffset);
int pruneCandidates(int level,set<Item>* items,ofstream &out);
int pruneCandidates(int level, set<Item> *items, int depth, int *itemset,ofstream &out);
ofstream& printTreeInTxt(set<Item> *tree,int level,ofstream &out);
void printSet(const Item& item, int *itemset, int length,ofstream &out);
bool operator < (const FreItem& f1,const FreItem& f2)
{
	if (f1.frekey.size() < f2.frekey.size())
	{
		return true;
	}
	else if (f1.frekey.size()==f2.frekey.size() )
	{
		int i = 0,limit = (int)f1.frekey.size();
		while(i < limit)
		{
			if (f1.frekey.at(i) < f2.frekey.at(i))
			{
				return true;
			}
			else if (f1.frekey.at(i) == f2.frekey.at(i))
			{
				i++;
			}
			else
				return false;
		}
		return false;
	}
	else
		return false;
}
std::map<FreItem,int> merge_maps_new(std::map<FreItem,int> & lhs,  std::map<FreItem,int> & rhs)
{
	typedef std::map<FreItem,int>::iterator input_iterator;
	std::map<FreItem,int> result_intersection;//交集
	for ( input_iterator it1 = lhs.begin(), it2 = rhs.begin(),end1 = lhs.end(), end2 = rhs.end();
		it1 != end1 && it2 != end2; )
	{
		if ( it1->first == it2->first )
		{
			result_intersection.insert(make_pair(it1->first,it1->second + it2->second));
			//result[it1->first] = std::make_pair( it1->second, it2->second );
			it1 = lhs.erase(it1);
			it2 = rhs.erase(it2);
		}
		else
		{
			if ( it1->first < it2->first )
			{
				++it1;
			}	
			else
			{
				++it2;
			}

		}
	}
	return result_intersection;
}
//比较两个map求其交集
template <typename Key, typename Value>
std::map<Key,Value> 
merge_maps( std::map<Key,Value> const & lhs,  std::map<Key,Value> const & rhs)
{
	typedef typename std::map<Key,Value>::const_iterator input_iterator;
	std::map<Key,Value> result_intersection;//交集
	for ( input_iterator it1 = lhs.begin(), it2 = rhs.begin(),end1 = lhs.end(), end2 = rhs.end();
		it1 != end1 && it2 != end2; )
	{
		if ( it1->first == it2->first )
		{
			result_intersection.insert(make_pair(it1->first,it1->second + it2->second));
			++it1; ++it2;
		}
		else
		{
			if ( it1->first < it2->first )
			{
				++it1;
			}	
			else
			{
				++it2;
			}		
		}
	}
	return result_intersection;
}
//求map1与map2的差集，即map1-map12_intersection,map1应该是小文件得出的map
std::map<FreItem,int> difference_maps(std::map<FreItem,int> map1,  std::map<FreItem,int> map_intersection )
{
	cout << "原map1len:" << map1.size() << " ";
	cout << "原map_intersection len:" << map_intersection.size() << " ";
	typedef std::map<FreItem,int>::iterator input_iterator;
	//std::map<FreItem,int> result = map1;
	//map1一定大于map_intersection
	for (input_iterator it1 = map1.begin(),it_inter = map_intersection.begin(),end1 = map1.end(),end_inter =map_intersection.end();it1 != end1 && it_inter != end_inter;)
	{
		if (it1->first == it_inter->first)
		{
			//std::cout << "删除项" <<it1->first.frekey.at(0) << endl;;
			it1 = map1.erase(it1);
			++it_inter;
		}
		else
		{
			if ( it1->first < it_inter->first )
			{
				++it1;
			}	
			else
			{
				++it_inter;
			}
		}			
	}
	cout << "差集len:" << map1.size() << " ";
	return map1;
}
char* readBin(char *fileName)
{
	HANDLE hFile = CreateFile(fileName, GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("文件句柄无法正确得到！\n");
		exit(-1);
	}
	HANDLE hFileMap = CreateFileMapping(hFile, 
		NULL, PAGE_READWRITE, 0, 0, NULL);
	if (hFileMap == NULL)
	{
		printf("无法映射文件！\n");
		exit(-1);
	}
	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);
	DWORD dwGran = SysInfo.dwAllocationGranularity;

	DWORD dwFileSizeHigh;
	__int64 qwFileSize = GetFileSize(hFile, &dwFileSizeHigh);
	qwFileSize |= (((__int64)dwFileSizeHigh) << 32);

	__int64 qwFileOffset = 0;
	__int64 T_newmap = 900 * dwGran;

	DWORD dwBlockBytes = 1000 * dwGran;
	if (qwFileSize - qwFileOffset < dwBlockBytes)
		dwBlockBytes = (DWORD)qwFileSize;

	char *lpbMapAddress = (char *)MapViewOfFile(hFileMap,FILE_MAP_ALL_ACCESS,(DWORD)(qwFileOffset >> 32), 
		(DWORD)(qwFileOffset & 0xFFFFFFFF),dwBlockBytes);
	if (lpbMapAddress == NULL)
	{
		printf("无法映射文件！\n");
		exit(-1);
	}
	CloseHandle(hFile);
	return lpbMapAddress;
}

int main()
{
	int count =0;//记录频繁是几频繁项，并且count是每次加1的，碰到*****即加1
	map<FreItem,int> FreMap;//用map存储频繁项及其对应个数
	map<FreItem,int> FreMaplit;//用map存储频繁项及其对应个数,由相对较小的数据库求得
	ofstream out;//用于输出结果到FreMap_result.txt
	out.open("E:\\design\\FreMap_result.txt");
	char strline[1024];
	char str[32];
	cout << "读取文件形成map中，请等待...." <<endl;
	clock_t start = clock();
	//读入原数据形成的频繁项籍
	char *dataFile = "E:\\GPU_Bitmap_Apriori\\output\\fimi\\m_T10I4D100K_result.txt";
	char *dataFilelit = "E:\\GPU_Bitmap_Apriori\\output\\fimi\\m_T10I4D1K_cpu_new.txt";
	FILE *fp = fopen(dataFile,"r");
	if (fp == NULL)
	{
		printf("大文件打开失败\n");
		system("pause");
		exit(1);
	}
	//扫描大文件
	while(fgets(strline,1024,fp))
	{
		if (strline[0] == '*' )
		{
			count++;
			continue;
		}
		FreItem FI = FreItem();
		const char *loc = strline;
		FI.frekey.push_back(atoi(strline));
		int res = 1;	
		loc = strstr(strline," ");
		while(loc != NULL && res != count)
		{
			FI.frekey.push_back(atoi(loc+1));
			res++;
			loc = strstr(loc+1, " ");
		}
		FreMap.insert(make_pair(FI,atoi(loc+1)));
	}
	cout << "大文件扫描完成，用时：" << (clock()-start)/double(CLOCKS_PER_SEC) <<endl;
	//扫描小文件
	start = clock();
	FILE *fplit = fopen(dataFilelit,"r");
	if(fplit == NULL)
	{
		printf("小文件打开失败\n");
		system("pause");
		exit(1);
	}
	count = 0;
	while (fgets(str,32,fplit))
	{
		if (str[0] == '*' )
		{
			count++;
			continue;
		}
		FreItem FI = FreItem();
		const char *loc = str;
		FI.frekey.push_back(atoi(str));
		int res = 1;	
		loc = strstr(str," ");
		while(loc != NULL && res != count)
		{
			FI.frekey.push_back(atoi(loc+1));
			res++;
			loc = strstr(loc+1, " ");
		}
		FreMaplit.insert(make_pair(FI,atoi(loc+1)));
	}
	cout << "小文件扫描完成，用时：" << (clock()-start)/double(CLOCKS_PER_SEC) <<endl;
	start = clock();
	map<FreItem,int> result = merge_maps_new(FreMap,FreMaplit);
	cout << "(FreMap-FreMaplit)_len:" << FreMap.size() << endl;
	cout << "(FreMaplit-FreMap)_len:" << FreMaplit.size() << endl;
	cout << "(FreMaplit&FreMap)_len" << result.size() <<endl;
	//test
	cout << "求交集,差集所消耗时间："<< (clock()-start)/double(CLOCKS_PER_SEC) <<"s"<<endl;
	cout << "形成字典树" << endl;
	//开始求FreMap中的频繁项在原数据库中的支持度
	//!!!!!!!---------------------------------------------!!!!!!!!!!!!!!!!!!!!!
	char *dataBigfile = "E:\\design\\read_reserve_T10I4D1K.bin";
	//只要满足最终的支持度大于等于101000*1% = 1010即可通过，作为频繁项
	//构建项集树所需时间
	start = clock();
	clock_t startgenF1tree = start;
	int itemcount = 1;//频繁n项集中n的个数
	Item *trie = new Item(-1);//trie 为一个指向item的set集合
	set<Item> *items = trie->makeChildren();
	//1----------------------向items中插入频繁1项集
	map<FreItem,int>::iterator it = FreMap.begin();
	while(it != FreMap.end() && it->first.frekey.size() == itemcount)
	{
		Item item(it->first.frekey.at(0));
		item.Increment(it->second);
		items->insert(item);
		it++;
	}
	cout << "      树生成耗费时："<< (clock()-startgenF1tree)/double(CLOCKS_PER_SEC) <<"s" << endl;
	//将二进制文件放入内存中
	clock_t startcountF1 = clock();
	char * addr = readBin(dataBigfile);
	//printf("test for readBin:%d,%d,%d,%d",addr[0],addr[1],addr[2],addr[3]);//测试文件的读入是否正确
	int numTran;
	memcpy(&numTran,addr,sizeof(int));
	//cout << "numTran:" <<  numTran <<endl;
	int byteBase = BenCeilInt(numTran, (sizeof(byte)*8));
	size_t shortBase = byteBase / 2 + (size_t)(byteBase & 0x1);
	int intBase = shortBase * 2 + sizeof(int) * 2;
	addr = addr + FIXED_VALUE;
	int *test = (int *)(addr+4);
	//计算频繁1项集在dataBigfile中找对应的项的值，先依次遍历第一层items，之后直接定位到文件的指定位置
	set<Item>::iterator tmp;
	for (tmp = items->begin();tmp != items->end();)
	{
		int idlen = (tmp->getId()-1);
		//文件向右偏移8+（idlen*intBase）
		int supNum = ((int *)(addr+idlen*intBase+4))[0];
		int suptmp = tmp->getSupport() +supNum;
		if ( suptmp >= MIN_SUP )
		{
			tmp->setSupport(suptmp);
			tmp++;
		}
		else //删除当前项
		{
			set<Item>::iterator runnertmp = tmp;
			++tmp;
			items->erase(runnertmp);
		}
	}
	cout << "      计算支持度并删选满足项耗时："<< (clock()-startcountF1)/double(CLOCKS_PER_SEC) <<"s" << endl;
	cout << "------频繁1项集验证结束------" << endl;
#ifdef debug
	//打印频繁1项集的支持度,目前的结果是没有一个是满足最小支持度的
	if (items->size() == 0)
	{
		out << "做差集后在FreMap中的频繁1项集在更新后的数据中都不是频繁的" << endl;
	}
	for (tmp = items->begin();tmp != items->end();tmp++)
	{
		out << "  " << tmp->getId() << " " << "(" <<tmp->getSupport() << ")" << endl;
	}
#endif	
#ifdef print
	if (items->size() == 0)
	{
		out << "做差集后在FreMap中的频繁1项集在更新后的数据中都不是频繁的" << endl;
	}
	else
	{
		out << "*****" << endl;
		for (tmp = items->begin();tmp != items->end();tmp++)
		{
			out << tmp->getId() << " "  <<tmp->getSupport() << endl;
		}
	}

#endif
	clock_t startgenF2tree = clock();
	set<Item>::iterator runner;
	itemcount++;
	//2----------------------向items中插入频繁2项集,先找是否2项集中的第一项在items中
	pair<set<Item>::iterator,bool> pr;
	while (it!=FreMap.end() && it->first.frekey.size() == itemcount)
	{	
		set<Item>::iterator r ;
		/*
		if((r = items->find(Item(1))) != items->end())
		{
			if ((r->getChildren()->find(Item(25))) != r->getChildren()->end() )
			{
				cout << "暂停！！！！" << endl;
			}
		}*/
		Item item1(it->first.frekey.at(0));
		if (items->find(item1) == items->end())
		{
			pr = items->insert(item1);
			set<Item> *children = (pr.first)->makeChildren();
			//假设第二项都不相同，插入第二项
			Item item2(it->first.frekey.at(1));
			item2.Increment(it->second);
			children->insert(item2);
			it++;
		}
		else//否则直接插入2项集的第二项
		{
			//set<Item> *children = (pr.first)->getChildren();
			r = items->find(item1);
			set<Item> *children = r->makeChildren();
			Item item2(it->first.frekey.at(1));
			item2.Increment(it->second);
			children->insert(item2);
			it++;
		}
	}
	cout << "      树生成耗费时间："<< (clock()-startgenF2tree)/double(CLOCKS_PER_SEC) <<"s" << endl;
	//要枝剪死节点！！！！！！！！！！！！！！！
	clock_t pruneF2 = clock();
	pruneNodes(itemcount,items,1);
	cout << "      删去死节点耗时："<< (clock()-pruneF2)/double(CLOCKS_PER_SEC) <<"s" << endl;
#ifdef debug
	printTree(items,1);
	cout << "level_2_tree生成所用时间" << (clock()-start)/double(CLOCKS_PER_SEC) <<"s"<<endl;
#endif


	//频繁2项集使用CUDA进行计算，已知items
	//!!!!!!!!!将文件放入GPU内存中
	clock_t startcountF2 = clock();
	unsigned short *m = (unsigned short*)addr;
	initGPUMiner(m,NUMITEM,numTran);
	int *bParentList = new int[BOUND2];
	vector<Item*> bItemList;
	int *bItemLen = new int[BOUND2];
	runner = items->begin();
	int bParentOffset = 0;//这个变量没什么用，为了统一表达
	//很有可能外层循环只循环一次
	while (runner != items->end())
	{
		int bNumPair = 0;
		size_t bNumItem = 0;
		bItemList.clear();
		while(runner != items->end() && runner->getChildren() && bNumItem < BOUND)
		{
			bParentList[bNumPair] = runner->getId()-1;//!!!!!!
			set<Item>* children = runner->getChildren();
			for(set<Item>::iterator cIt = children->begin(); cIt != children->end(); cIt++) 
			{
				Item &item = *cIt;
				bItemList.push_back(&item);//将当前runner的全部子item放入bItemList
			}
			bItemLen[bNumPair] = children->size();
			bNumItem += runner->getChildren()->size();
			bNumPair ++;
			runner ++;
			bParentOffset++;
		}
		countSupports(itemcount,2,bParentList,bItemLen,bNumPair,&bItemList,bNumItem,bParentOffset);
	}
	delete [] bParentList;
	delete [] bItemLen;
	cout << "      计算支持度耗时："<< (clock()-startcountF2)/double(CLOCKS_PER_SEC) <<"s" << endl;
#ifdef debug
	//测试打印支持度	
	cout <<"\n----------------------------" << itemcount << "----------------------------" << endl;
	printTree(items,1);
#endif
	out << "*****" << endl;
	clock_t startpruneCanF2 = clock();
	pruneCandidates(2,items,out);
	cout << "      筛选满足项耗时："<< (clock()-startpruneCanF2)/double(CLOCKS_PER_SEC) <<"s" << endl;
	cout << "------频繁2项集验证结束------" << endl;
#ifdef debug
	cout << "\n对打印枝剪后的结果，频繁项数itemcount=" << itemcount << endl;
	if (items->size() == 0)
	{
		out << "做差集后在FreMap中的频繁2项集在更新后的数据中都不是频繁的" << endl;
	}
	else
	{
		printTreeInTxt(items,1,out);
	}
	printTree(items,1);
#endif
	
	itemcount++;
	//使用循环进行树的生成
	bool running = true;
	while(running)
	{
		//对于itemcount>2时，进行字典树的生成（向items中插入频繁itemcount项集),使用genTree函数
		//注意迭代器it是一直处于叠加状态，所以要把迭代器作为参数传入，迭代器这里在不能作为参数传入，可以先将it指向的值传入在find
		FreItem FI = it->first;
		it = FreMap.find(genTree(FreMap,itemcount,items,FI));
		clock_t pruneStartTime = clock();
		pruneNodes(itemcount,items,1);//删除死节点
		cout << "      删去死节点耗时："<< (clock()-pruneStartTime)/double(CLOCKS_PER_SEC) <<"s" << endl;
		clock_t countStartTime= clock();
		//计算频候选繁n项集的支持度
		int *bParentList = new int[BOUND2];
		vector<Item*> bItemList;
		int *bItemLen = new int[BOUND2];
		runner = items->begin();
		int bParentOffset = 0;
		//如果项很小的话很有可能外层循环只循环一次
		while (runner != items->end())
		{
			int bNumPair = 0;
			int bNumItem = 0;
			bItemList.clear();
			while(runner != items->end() && runner->getChildren() && bNumItem < BOUND)
			{
				bParentList[bNumPair] = runner->getId()-1;//!!!!!!
				set<Item>* children = runner->getChildren();
				for(set<Item>::iterator cIt = children->begin(); cIt != children->end(); cIt++) 
				{
					Item &item = *cIt;
					bItemList.push_back(&item);//将当前runner的全部子item放入bItemList
				}
				bItemLen[bNumPair] = children->size();
				bNumItem += runner->getChildren()->size();
				bNumPair ++;
				runner ++;
				bParentOffset++;
			}
			countSupportsOver2(itemcount,2,bParentList,bItemLen,bNumPair,&bItemList,bNumItem,bParentOffset);
		}
		delete [] bParentList;
		delete [] bItemLen;
		cout << "      计算支持度耗时："<< (clock()-countStartTime)/double(CLOCKS_PER_SEC) <<"s" << endl;
#ifdef debug
		//测试打印支持度	
		cout <<"\n----------------------------" << itemcount << "----------------------------" << endl;
		printTree(items,1);
#endif
		//先打印再枝剪
		out << "*****" << endl;
		clock_t pruneCandidatesStartTime = clock();
		pruneCandidates(itemcount,items,out);
		cout << "      筛选满足项耗时："<< (clock()-pruneCandidatesStartTime)/double(CLOCKS_PER_SEC) <<"s" << endl;
		cout << "------频繁" << itemcount<<"项集验证结束------" << endl;
#ifdef debug
		if (items->size() == 0)
		{
			out << "做差集后在FreMap中的频繁" << itemcount <<"项集在更新后的数据中都不是频繁的" << endl;
		}
		else
		{
			printTreeInTxt(items,1,out);
		}
		cout << "\n对打印枝剪后的结果，频繁项数itemcount=" << itemcount << endl;
		printTree(items,1);
#endif
		map<FreItem,int>::iterator ittmp = it;
		if ((++ittmp) != FreMap.end())
		{
			itemcount++;
			it++;
		}
		else
		{
			running = false;
		}
			

	}
	cout << "支持度计算所需时间："<< (clock()-start)/double(CLOCKS_PER_SEC) <<"s"<<endl;
	out.close();
	
}
FreItem genTree(map<FreItem,int> FreMap,int itemcount,set<Item> *items,FreItem FI)
{
	clock_t start = clock();
	set<Item>::iterator runner;
	pair<set<Item>::iterator,bool> pr;
	map<FreItem,int>::iterator  iter = FreMap.find(FI);
	while(iter != FreMap.end() && iter->first.frekey.size() == itemcount)
	{
		set<Item> *children = items;
		for (int i = 0;i < itemcount;i++)
		{
			if (i == itemcount-1)//最后一项要插入支持度
			{
				Item item(iter->first.frekey.at(i));
				item.Increment(iter->second);
				pr = children->insert(item);
				children = (pr.first)->makeChildren();
			}
			else
			{
				Item item(iter->first.frekey.at(i));	
				if ((runner = children->find(item)) != children->end())
				{
					children = runner->makeChildren();
					continue;
				}
				else
				{
					pr = children->insert(item);
					children = (pr.first)->makeChildren();
				}
			}
		}
		iter++;
	}
	/*
	if (iter == FreMap.end())
	{
		iter--;
	}*/
	iter--;
	FreItem FItmp = iter->first;
	cout << "      树生成耗费时间："<< (clock()-start)/double(CLOCKS_PER_SEC) <<"s"<<endl;
	return FItmp;//返回的是最后扫描的

}

//删除死节点，主要是为了方便进行cuda计算
int pruneNodes(int level,set<Item> *items,int depth)
{
	if(items == 0)
		return 0;
	int nodes = 0;
	if(depth==level) nodes = (int)items->size();
	else 
	{
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
	//返回深度为level的节点个数
	return nodes;

}
ofstream& printTreeInTxt(set<Item> *tree,int level,ofstream &out)
{
	char *blank = "  ";
	if (tree->size())
	{
		for(int i = 0;i < level;i++)
		{
			out << blank;
		}
		for (set<Item>::iterator runner =tree->begin(); runner != tree->end(); runner++)
		{
			out << "\n";
			for(int i = 0;i < level;i++)
			{
				out << blank;
			}
			out << runner->getId() << " ";
			if (runner->getChildren() != NULL)
			{
				printTree(runner->getChildren(),level+1);
			}
			if (runner->getChildren() == NULL)
			{
				out << "(" << runner->getSupport() << ")";
				continue;
			}
			if (runner->getChildren()->size() == 0)//输出支持度
			{
				out << "(" << runner->getSupport() << ")";
			}
		}
		for(int i = 0;i < level;i++)
		{
			out << blank;
		}
	}
	return out;
}
//保证同一层的输出的缩进是在一列
void printTree(set<Item> *tree,int level)
{
	char *blank = "  ";
	if (tree->size())
	{
		for(int i = 0;i < level;i++)
		{
			cout << blank;
		}
		for (set<Item>::iterator runner =tree->begin(); runner != tree->end(); runner++)
		{
			cout << "\n";
			for(int i = 0;i < level;i++)
			{
				cout << blank;
			}
			cout << runner->getId() << " ";
			if (runner->getChildren() != NULL)
			{
				printTree(runner->getChildren(),level+1);
			}
			if (runner->getChildren() == NULL)
			{
				cout << "(" << runner->getSupport() << ")";
				continue;
			}
			if (runner->getChildren()->size() == 0)//输出支持度
			{
				cout << "(" << runner->getSupport() << ")";
			}

		}
		for(int i = 0;i < level;i++)
		{
			cout << blank;
		}
	}
}
void countSupports(int level, int depth, int *parentList, int *itemLen, int numPair, vector<Item*>* itemList, size_t numItem, int parentOffset)
{
	if (itemList == 0)
	{
		return;
	}
	int* itemIdList = new int[BOUND2];
	for(int i=0; i < itemList->size(); i++)
	{
		itemIdList[i] = ((*itemList)[i])->getId()-1;
	}	
	try
	{
		int *supList = new int[numItem];
		//cout << "numItem = " << numItem << endl;
		if (supList == 0)
		{
			cout << "内存分配失败！！！" << endl;
			cout << "numItem = " << numItem << endl;
			//exit(-1);
		}
		gpuBoundVectorAnd(-1,parentList, itemLen, numPair, itemIdList, numItem, depth, true, supList);
		//继续计算支持度
		for(int i=0; i<itemList->size(); i++)
		{
			Item *item = (*itemList)[i];
			item->Increment(supList[i]);
		}
		delete[] supList;	
		//free(supList);
		delete[] itemIdList;
	}
	catch (const std::bad_alloc& e)
	{
		cout << "内存分配出现异常" << endl;
	}
}

void countSupportsOver2(int level, int depth, int *parentList, int *itemLen, int numPair, vector<Item*>* itemList, size_t numItem, int parentOffset)
{
	if (itemList == 0) return;//表示当父项没有子项时，即没有更多候选项了，进行返回

	int* itemIdList = new int[BOUND2];
	for(int i=0; i<itemList->size(); i++)
		itemIdList[i] = ((*itemList)[i])->getId()-1;//将全部的子项的itemId存入itemIdList数组中


	int * bParentList = new int[numPair];
	for( int i = 0; i < numPair; i++ )
			bParentList[i] = i;//初始化bParentList数组，此数组没有实质意义

	if( depth < level)
	{
		if(depth == 2)//字典树遍历的深度为2时，就是有两个item
		{
			gpuBoundVectorAnd( -1, parentList, itemLen, numPair, itemIdList, numItem, depth, false, 0);
		}
		else
		{
			//事实上bParentList这个参数是可以不要的
			gpuBoundVectorAnd( parentOffset, bParentList, itemLen, numPair, itemIdList, numItem, depth, false, 0); 
		}

		int *bItemList = new int[BOUND2];//bParentList
		vector<Item*> bChildList;//bItemList
		int *bChildLen = new int[BOUND2];//bItemLen
		int index = 0;
		int bItemOffset = 0;//bParentOffset

		while(index < numItem)//深度为depth所含的项的总个数
		{
			bItemOffset = index;//这里和之前的bParentOffset其实是一样的

			bChildList.clear();
			int bNumPair = 0;							// number of parent child pairs process together父子对的个数
			size_t bNumChild = 0;//bNumItem

			Item* curItem = (*itemList)[index];//到时候会对index逐次加1，等于说是遍历了一遍深度为depth的项
			while(index < numItem && curItem->getChildren() && bNumChild < BOUND)
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
			countSupportsOver2(level, depth+1, bItemList, bChildLen, bNumPair, &bChildList, bNumChild, bItemOffset);
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

}

int pruneCandidates(int level,set<Item>* items,ofstream &out)
{
	int left;
	int *tmp = new int[level];
	left = pruneCandidates(level,items,1,tmp,out);
	delete [] tmp;
	return left;
}


//prune itemsets whose sup < minsup
int pruneCandidates(int level, set<Item> *items, int depth, int *itemset,ofstream &out)
{
	if(items == 0) return 0;
	int left = 0; 

	for(set<Item>::iterator runner = items->begin(); runner != items->end(); ) {
		itemset[depth-1] = runner->getId();
		if(depth == level) {
			if(runner->getSupport() < MIN_SUP) {
				runner->deleteChildren();
				set<Item>::iterator tmp = runner++; 
				items->erase(tmp);
			}
			else {
#ifdef print
				printSet(*runner, itemset, depth,out);
#endif
				left++;
				runner++;
			}
		}
		else {	
			int now = pruneCandidates(level, runner->getChildren(), depth+1, itemset,out);
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
void printSet(const Item& item, int *itemset, int length,ofstream &out)
{
	set<int> outset;
	for(int j=0; j<length; j++) 
		outset.insert(itemset[j]); 
	for(set<int>::iterator k=outset.begin(); k!=outset.end(); k++)  out << *k << " ";
		out << item.getSupport() <<  endl;
}
