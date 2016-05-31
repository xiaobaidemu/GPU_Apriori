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
//ʹ��hash_map��deque����ϣ�����ȡԭ���ݿ��Ƶ�������
#define NUM 3
//��ȡ��¼����Ķ������ļ�
extern "C"
char *readBin(IN char *fileName);
//��ӡ���ɵ��ֵ���
void printTree(set<Item> *tree,int level);
//�����ֵ���
FreItem genTree(map<FreItem,int> FreMap,int itemcount,set<Item> *items,FreItem FI);
//ɾ�����ڵ�
int pruneNodes(int level,set<Item> *items,int depth);
//��������
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
	std::map<FreItem,int> result_intersection;//����
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
//�Ƚ�����map���佻��
template <typename Key, typename Value>
std::map<Key,Value> 
merge_maps( std::map<Key,Value> const & lhs,  std::map<Key,Value> const & rhs)
{
	typedef typename std::map<Key,Value>::const_iterator input_iterator;
	std::map<Key,Value> result_intersection;//����
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
//��map1��map2�Ĳ����map1-map12_intersection,map1Ӧ����С�ļ��ó���map
std::map<FreItem,int> difference_maps(std::map<FreItem,int> map1,  std::map<FreItem,int> map_intersection )
{
	cout << "ԭmap1len:" << map1.size() << " ";
	cout << "ԭmap_intersection len:" << map_intersection.size() << " ";
	typedef std::map<FreItem,int>::iterator input_iterator;
	//std::map<FreItem,int> result = map1;
	//map1һ������map_intersection
	for (input_iterator it1 = map1.begin(),it_inter = map_intersection.begin(),end1 = map1.end(),end_inter =map_intersection.end();it1 != end1 && it_inter != end_inter;)
	{
		if (it1->first == it_inter->first)
		{
			//std::cout << "ɾ����" <<it1->first.frekey.at(0) << endl;;
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
	cout << "�len:" << map1.size() << " ";
	return map1;
}
char* readBin(char *fileName)
{
	HANDLE hFile = CreateFile(fileName, GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("�ļ�����޷���ȷ�õ���\n");
		exit(-1);
	}
	HANDLE hFileMap = CreateFileMapping(hFile, 
		NULL, PAGE_READWRITE, 0, 0, NULL);
	if (hFileMap == NULL)
	{
		printf("�޷�ӳ���ļ���\n");
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
		printf("�޷�ӳ���ļ���\n");
		exit(-1);
	}
	CloseHandle(hFile);
	return lpbMapAddress;
}

int main()
{
	int count =0;//��¼Ƶ���Ǽ�Ƶ�������count��ÿ�μ�1�ģ�����*****����1
	map<FreItem,int> FreMap;//��map�洢Ƶ������Ӧ����
	map<FreItem,int> FreMaplit;//��map�洢Ƶ������Ӧ����,����Խ�С�����ݿ����
	ofstream out;//������������FreMap_result.txt
	out.open("E:\\design\\FreMap_result.txt");
	char strline[1024];
	char str[32];
	cout << "��ȡ�ļ��γ�map�У���ȴ�...." <<endl;
	clock_t start = clock();
	//����ԭ�����γɵ�Ƶ���
	char *dataFile = "E:\\GPU_Bitmap_Apriori\\output\\fimi\\m_T10I4D100K_result.txt";
	char *dataFilelit = "E:\\GPU_Bitmap_Apriori\\output\\fimi\\m_T10I4D1K_cpu_new.txt";
	FILE *fp = fopen(dataFile,"r");
	if (fp == NULL)
	{
		printf("���ļ���ʧ��\n");
		system("pause");
		exit(1);
	}
	//ɨ����ļ�
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
	cout << "���ļ�ɨ����ɣ���ʱ��" << (clock()-start)/double(CLOCKS_PER_SEC) <<endl;
	//ɨ��С�ļ�
	start = clock();
	FILE *fplit = fopen(dataFilelit,"r");
	if(fplit == NULL)
	{
		printf("С�ļ���ʧ��\n");
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
	cout << "С�ļ�ɨ����ɣ���ʱ��" << (clock()-start)/double(CLOCKS_PER_SEC) <<endl;
	start = clock();
	map<FreItem,int> result = merge_maps_new(FreMap,FreMaplit);
	cout << "(FreMap-FreMaplit)_len:" << FreMap.size() << endl;
	cout << "(FreMaplit-FreMap)_len:" << FreMaplit.size() << endl;
	cout << "(FreMaplit&FreMap)_len" << result.size() <<endl;
	//test
	cout << "�󽻼�,�������ʱ�䣺"<< (clock()-start)/double(CLOCKS_PER_SEC) <<"s"<<endl;
	cout << "�γ��ֵ���" << endl;
	//��ʼ��FreMap�е�Ƶ������ԭ���ݿ��е�֧�ֶ�
	//!!!!!!!---------------------------------------------!!!!!!!!!!!!!!!!!!!!!
	char *dataBigfile = "E:\\design\\read_reserve_T10I4D1K.bin";
	//ֻҪ�������յ�֧�ֶȴ��ڵ���101000*1% = 1010����ͨ������ΪƵ����
	//�����������ʱ��
	start = clock();
	clock_t startgenF1tree = start;
	int itemcount = 1;//Ƶ��n���n�ĸ���
	Item *trie = new Item(-1);//trie Ϊһ��ָ��item��set����
	set<Item> *items = trie->makeChildren();
	//1----------------------��items�в���Ƶ��1�
	map<FreItem,int>::iterator it = FreMap.begin();
	while(it != FreMap.end() && it->first.frekey.size() == itemcount)
	{
		Item item(it->first.frekey.at(0));
		item.Increment(it->second);
		items->insert(item);
		it++;
	}
	cout << "      �����ɺķ�ʱ��"<< (clock()-startgenF1tree)/double(CLOCKS_PER_SEC) <<"s" << endl;
	//���������ļ������ڴ���
	clock_t startcountF1 = clock();
	char * addr = readBin(dataBigfile);
	//printf("test for readBin:%d,%d,%d,%d",addr[0],addr[1],addr[2],addr[3]);//�����ļ��Ķ����Ƿ���ȷ
	int numTran;
	memcpy(&numTran,addr,sizeof(int));
	//cout << "numTran:" <<  numTran <<endl;
	int byteBase = BenCeilInt(numTran, (sizeof(byte)*8));
	size_t shortBase = byteBase / 2 + (size_t)(byteBase & 0x1);
	int intBase = shortBase * 2 + sizeof(int) * 2;
	addr = addr + FIXED_VALUE;
	int *test = (int *)(addr+4);
	//����Ƶ��1���dataBigfile���Ҷ�Ӧ�����ֵ�������α�����һ��items��֮��ֱ�Ӷ�λ���ļ���ָ��λ��
	set<Item>::iterator tmp;
	for (tmp = items->begin();tmp != items->end();)
	{
		int idlen = (tmp->getId()-1);
		//�ļ�����ƫ��8+��idlen*intBase��
		int supNum = ((int *)(addr+idlen*intBase+4))[0];
		int suptmp = tmp->getSupport() +supNum;
		if ( suptmp >= MIN_SUP )
		{
			tmp->setSupport(suptmp);
			tmp++;
		}
		else //ɾ����ǰ��
		{
			set<Item>::iterator runnertmp = tmp;
			++tmp;
			items->erase(runnertmp);
		}
	}
	cout << "      ����֧�ֶȲ�ɾѡ�������ʱ��"<< (clock()-startcountF1)/double(CLOCKS_PER_SEC) <<"s" << endl;
	cout << "------Ƶ��1���֤����------" << endl;
#ifdef debug
	//��ӡƵ��1���֧�ֶ�,Ŀǰ�Ľ����û��һ����������С֧�ֶȵ�
	if (items->size() == 0)
	{
		out << "�������FreMap�е�Ƶ��1��ڸ��º�������ж�����Ƶ����" << endl;
	}
	for (tmp = items->begin();tmp != items->end();tmp++)
	{
		out << "  " << tmp->getId() << " " << "(" <<tmp->getSupport() << ")" << endl;
	}
#endif	
#ifdef print
	if (items->size() == 0)
	{
		out << "�������FreMap�е�Ƶ��1��ڸ��º�������ж�����Ƶ����" << endl;
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
	//2----------------------��items�в���Ƶ��2�,�����Ƿ�2��еĵ�һ����items��
	pair<set<Item>::iterator,bool> pr;
	while (it!=FreMap.end() && it->first.frekey.size() == itemcount)
	{	
		set<Item>::iterator r ;
		/*
		if((r = items->find(Item(1))) != items->end())
		{
			if ((r->getChildren()->find(Item(25))) != r->getChildren()->end() )
			{
				cout << "��ͣ��������" << endl;
			}
		}*/
		Item item1(it->first.frekey.at(0));
		if (items->find(item1) == items->end())
		{
			pr = items->insert(item1);
			set<Item> *children = (pr.first)->makeChildren();
			//����ڶ������ͬ������ڶ���
			Item item2(it->first.frekey.at(1));
			item2.Increment(it->second);
			children->insert(item2);
			it++;
		}
		else//����ֱ�Ӳ���2��ĵڶ���
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
	cout << "      �����ɺķ�ʱ�䣺"<< (clock()-startgenF2tree)/double(CLOCKS_PER_SEC) <<"s" << endl;
	//Ҫ֦�����ڵ㣡����������������������������
	clock_t pruneF2 = clock();
	pruneNodes(itemcount,items,1);
	cout << "      ɾȥ���ڵ��ʱ��"<< (clock()-pruneF2)/double(CLOCKS_PER_SEC) <<"s" << endl;
#ifdef debug
	printTree(items,1);
	cout << "level_2_tree��������ʱ��" << (clock()-start)/double(CLOCKS_PER_SEC) <<"s"<<endl;
#endif


	//Ƶ��2�ʹ��CUDA���м��㣬��֪items
	//!!!!!!!!!���ļ�����GPU�ڴ���
	clock_t startcountF2 = clock();
	unsigned short *m = (unsigned short*)addr;
	initGPUMiner(m,NUMITEM,numTran);
	int *bParentList = new int[BOUND2];
	vector<Item*> bItemList;
	int *bItemLen = new int[BOUND2];
	runner = items->begin();
	int bParentOffset = 0;//�������ûʲô�ã�Ϊ��ͳһ���
	//���п������ѭ��ֻѭ��һ��
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
				bItemList.push_back(&item);//����ǰrunner��ȫ����item����bItemList
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
	cout << "      ����֧�ֶȺ�ʱ��"<< (clock()-startcountF2)/double(CLOCKS_PER_SEC) <<"s" << endl;
#ifdef debug
	//���Դ�ӡ֧�ֶ�	
	cout <<"\n----------------------------" << itemcount << "----------------------------" << endl;
	printTree(items,1);
#endif
	out << "*****" << endl;
	clock_t startpruneCanF2 = clock();
	pruneCandidates(2,items,out);
	cout << "      ɸѡ�������ʱ��"<< (clock()-startpruneCanF2)/double(CLOCKS_PER_SEC) <<"s" << endl;
	cout << "------Ƶ��2���֤����------" << endl;
#ifdef debug
	cout << "\n�Դ�ӡ֦����Ľ����Ƶ������itemcount=" << itemcount << endl;
	if (items->size() == 0)
	{
		out << "�������FreMap�е�Ƶ��2��ڸ��º�������ж�����Ƶ����" << endl;
	}
	else
	{
		printTreeInTxt(items,1,out);
	}
	printTree(items,1);
#endif
	
	itemcount++;
	//ʹ��ѭ��������������
	bool running = true;
	while(running)
	{
		//����itemcount>2ʱ�������ֵ��������ɣ���items�в���Ƶ��itemcount�),ʹ��genTree����
		//ע�������it��һֱ���ڵ���״̬������Ҫ�ѵ�������Ϊ�������룬�����������ڲ�����Ϊ�������룬�����Ƚ�itָ���ֵ������find
		FreItem FI = it->first;
		it = FreMap.find(genTree(FreMap,itemcount,items,FI));
		clock_t pruneStartTime = clock();
		pruneNodes(itemcount,items,1);//ɾ�����ڵ�
		cout << "      ɾȥ���ڵ��ʱ��"<< (clock()-pruneStartTime)/double(CLOCKS_PER_SEC) <<"s" << endl;
		clock_t countStartTime= clock();
		//����Ƶ��ѡ��n���֧�ֶ�
		int *bParentList = new int[BOUND2];
		vector<Item*> bItemList;
		int *bItemLen = new int[BOUND2];
		runner = items->begin();
		int bParentOffset = 0;
		//������С�Ļ����п������ѭ��ֻѭ��һ��
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
					bItemList.push_back(&item);//����ǰrunner��ȫ����item����bItemList
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
		cout << "      ����֧�ֶȺ�ʱ��"<< (clock()-countStartTime)/double(CLOCKS_PER_SEC) <<"s" << endl;
#ifdef debug
		//���Դ�ӡ֧�ֶ�	
		cout <<"\n----------------------------" << itemcount << "----------------------------" << endl;
		printTree(items,1);
#endif
		//�ȴ�ӡ��֦��
		out << "*****" << endl;
		clock_t pruneCandidatesStartTime = clock();
		pruneCandidates(itemcount,items,out);
		cout << "      ɸѡ�������ʱ��"<< (clock()-pruneCandidatesStartTime)/double(CLOCKS_PER_SEC) <<"s" << endl;
		cout << "------Ƶ��" << itemcount<<"���֤����------" << endl;
#ifdef debug
		if (items->size() == 0)
		{
			out << "�������FreMap�е�Ƶ��" << itemcount <<"��ڸ��º�������ж�����Ƶ����" << endl;
		}
		else
		{
			printTreeInTxt(items,1,out);
		}
		cout << "\n�Դ�ӡ֦����Ľ����Ƶ������itemcount=" << itemcount << endl;
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
	cout << "֧�ֶȼ�������ʱ�䣺"<< (clock()-start)/double(CLOCKS_PER_SEC) <<"s"<<endl;
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
			if (i == itemcount-1)//���һ��Ҫ����֧�ֶ�
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
	cout << "      �����ɺķ�ʱ�䣺"<< (clock()-start)/double(CLOCKS_PER_SEC) <<"s"<<endl;
	return FItmp;//���ص������ɨ���

}

//ɾ�����ڵ㣬��Ҫ��Ϊ�˷������cuda����
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
				set<Item>::iterator tmp = runner++;//ע��������ȸ�ֵ���Լӣ�����tmp���ǵ�ǰ��runner
				items->erase(tmp);
			}
		}
	}
	//�������Ϊlevel�Ľڵ����
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
			if (runner->getChildren()->size() == 0)//���֧�ֶ�
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
//��֤ͬһ����������������һ��
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
			if (runner->getChildren()->size() == 0)//���֧�ֶ�
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
			cout << "�ڴ����ʧ�ܣ�����" << endl;
			cout << "numItem = " << numItem << endl;
			//exit(-1);
		}
		gpuBoundVectorAnd(-1,parentList, itemLen, numPair, itemIdList, numItem, depth, true, supList);
		//��������֧�ֶ�
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
		cout << "�ڴ��������쳣" << endl;
	}
}

void countSupportsOver2(int level, int depth, int *parentList, int *itemLen, int numPair, vector<Item*>* itemList, size_t numItem, int parentOffset)
{
	if (itemList == 0) return;//��ʾ������û������ʱ����û�и����ѡ���ˣ����з���

	int* itemIdList = new int[BOUND2];
	for(int i=0; i<itemList->size(); i++)
		itemIdList[i] = ((*itemList)[i])->getId()-1;//��ȫ���������itemId����itemIdList������


	int * bParentList = new int[numPair];
	for( int i = 0; i < numPair; i++ )
			bParentList[i] = i;//��ʼ��bParentList���飬������û��ʵ������

	if( depth < level)
	{
		if(depth == 2)//�ֵ������������Ϊ2ʱ������������item
		{
			gpuBoundVectorAnd( -1, parentList, itemLen, numPair, itemIdList, numItem, depth, false, 0);
		}
		else
		{
			//��ʵ��bParentList��������ǿ��Բ�Ҫ��
			gpuBoundVectorAnd( parentOffset, bParentList, itemLen, numPair, itemIdList, numItem, depth, false, 0); 
		}

		int *bItemList = new int[BOUND2];//bParentList
		vector<Item*> bChildList;//bItemList
		int *bChildLen = new int[BOUND2];//bItemLen
		int index = 0;
		int bItemOffset = 0;//bParentOffset

		while(index < numItem)//���Ϊdepth����������ܸ���
		{
			bItemOffset = index;//�����֮ǰ��bParentOffset��ʵ��һ����

			bChildList.clear();
			int bNumPair = 0;							// number of parent child pairs process together���ӶԵĸ���
			size_t bNumChild = 0;//bNumItem

			Item* curItem = (*itemList)[index];//��ʱ����index��μ�1������˵�Ǳ�����һ�����Ϊdepth����
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
				//now==0��ֱ��ִ��֦���ڵ�Ĺ���
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
