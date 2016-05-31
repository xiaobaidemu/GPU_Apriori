#include "FIMEncode.h"
#define reserve
//--------------------------------------------------
//Forward declarations
//--------------------------------------------------
static void
CountSupport(IN OUT uchar_t *inMatrix, 
					IN size_t numTran, 
					IN size_t numItem,IN char *addr);
static void
RemoveFail(IN OUT uchar_t *inMatrix, 
		OUT size_t *outNumItem,
		IN size_t numTran, 
		IN size_t numItem,
		IN size_t minSup);
static void
SortBySup(IN OUT uchar_t *inMatrix, 
		IN size_t numTran, 
		IN size_t numItem);
static void
Encode(IN uchar_t *inMatrix, 
		IN size_t numTran, 
		IN size_t numItem,
		OUT uchar_t **outMatrix,
		OUT IdSup_t **outIdSup,
		OUT size_t *outMatrixSize,
		OUT size_t *outIdSupSize);
//---------------------------------------------------
//External definition
//---------------------------------------------------
void EncodeBin(IN char *fileName,
			   IN int minSup,
			   OUT uchar_t **outMatrix,
			   OUT IdSup_t **outIdSup,
			   OUT size_t *outNumTran,
			   OUT size_t *outNumItem,
			   OUT size_t *outMatrixSize,
			   OUT size_t *outIdSupSize)
{
	BenSetup(0);
	BenStart(0);
	/*
	*  Memory Mapping内存映射数据文件
	*  返回BenMM_t结构体，其中包括了映射后文件的映射开始地址，文件大小，以及文件映射内核
	*/
	BenMM_t *mm = BenStartMMap(fileName);
	/*
	BenMM_t *mm = (BenMM_t*)BenMalloc(sizeof(BenMM_t));
	FILE *f = fopen(fileName,"wb+");
	if (f == NULL)
	{
		printf("文件无法打开");
		
	}
	else{
		printf("文件打开成功");
	}
	fseek(f,0,SEEK_END);
	mm->fileSize = ftell(f);
	printf("wenjian daxiao =%d",ftell(f));

	mm->addr = f->_base;
	*/
	size_t matrixSize = mm->fileSize-MATRIX_OFFSET;//MATRIX_OFFSET=(sizeof(size_t)*2)
	//printf("\n!!!!mm->fileSize=%d",mm->fileSize);
	//printf("\n!!!!filesize = %d",matrixSize);

	uchar_t *matrix = (uchar_t*)BenMalloc(matrixSize);//为矩阵分配空间
	size_t numTran = 0;
	size_t numItem = 0;
	BenMemcpy(&numTran, mm->addr+NUM_TRAN_OFFSET, sizeof(size_t));//从文件的映射开始地址开始拷贝4个字节到numTran的地址中
	BenMemcpy(&numItem, mm->addr+NUM_ITEM_OFFSET, sizeof(size_t));//在从文件映射开始地址后四个字节开始拷贝4个字节到numItem的地址中
	BenMemcpy(matrix, mm->addr+MATRIX_OFFSET, matrixSize);//从文件映射开始地址后八个字节开始拷贝matrixSize个字节
	
	//printf("\n!!!!test1:matrix[i]=%d,%d,%d,%d,%d,%d",matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);//matrix目前没有问题
	//BenEndMMap(mm);//释放相应资源
	*outNumTran = numTran;//出口参数
	dprint("file:%s, numTran:%d, numItem:%d, matrixSize:%d bytes\n", 
		fileName, numTran, numItem, matrixSize);
	//printf("\nfile:%s, numTran:%d, numItem:%d, matrixSize:%d bytes\n", 
		//fileName, *outNumTran, numItem, matrixSize);
	BenStop(0);
	BenPrintTime("io", 0);

	//count
	BenSetup(0);
	BenStart(0);
	//使用CountSupport计算单项作为候选项时的支持度
	CountSupport(matrix, numTran, numItem,mm->addr);
	//printf("\n!!!!test:numItem=%d",numItem);
	//printf("\n!!!!test2:matrix[i]=%d,%d,%d,%d,%d,%d",matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
	BenStop(0);
	//BenPrintTime("count", 0);
	BenEndMMap(mm);//释放相应资源
	//BenLog("\t\t\tinput item#%d\n", numItem);

	//remove
	BenSetup(0);
	BenStart(0);
	//问题出在RemoveFail函数中
	RemoveFail(matrix, outNumItem, numTran, numItem, minSup);
	//printf("\n!!!!test3:matrix[i]=%d,%d,%d,%d,%d,%d",matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
	//printf("\n!!!!test:*outNumItem=%d",*outNumItem);
	dprint("#freq: %d\n", *outNumItem);
	BenStop(0);
	//BenPrintTime("remove", 0);


	//sort
	BenSetup(0);
	BenStart(0);
	SortBySup(matrix, numTran, *outNumItem);
	//printf("\n!!!!test4:matrix[i]=%d,%d,%d,%d,%d,%d",matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
	//printf("\n!!!!test:*outNumItem=%d",*outNumItem);
	BenStop(0);
	//BenPrintTime("sort", 0);

	//encode
	BenSetup(0);
	BenStart(0);
	Encode(matrix, numTran, *outNumItem, outMatrix, outIdSup, outMatrixSize, outIdSupSize);
	//printf("\n!!!!test5:matrix[i]=%d,%d,%d,%d,%d,%d",matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
	BenStop(0);
	//BenPrintTime("encode", 0);
	//释放matrix这个指针所指向的空间
	BenFree((char**)&matrix, matrixSize);
}
/*
 * 计算level=2时的支持度
 */
void PairwiseCount(IN uchar_t *matrix, 
				   IN size_t numTran, 
				   IN size_t numItem,
				   IN int minSup, 
				   OUT Item *trie, 
				   OUT size_t *num2FI)
{
	BEN_ASSERT(matrix != NULL);
	
	size_t byteBase = BenCeilInt(numTran, 8);
	//(size_t)(byteBase & 0x1)的目的主要就是使shortBase变成2的倍数，便于以后的数据操作
	size_t shortBase = byteBase / 2  + (size_t)(byteBase & 0x1);

	int numCount = 0;
	int numAnd = 0;
	int count = 0;

	set<Item> *items = trie->getChildren();//返回trie的子节点set集，也就是频繁项集为1的项
	
	for(set<Item>::iterator runner = items->begin(); runner != items->end(); runner++)
	{
		set<Item> *children = runner->makeChildren();//为每一个trie的子集都创建一个set集
		int i = runner->getId();

		//uchar_t *item1 = (uchar_t*)(matrix + byteBase * i);
		ushort_t *item1s = (ushort_t*)(matrix + shortBase * 2 * i);
		
		//printf("%d\n", i);
		/*
		 * 当前freItem要依次与后续FreItem做交运算
		 */
		for (int j = i+1; j < numItem; j++)
		{
			//uchar_t *item2 = (uchar_t*)(matrix + byteBase * j);
			ushort_t *item2s = (ushort_t*)(matrix + shortBase * 2 * j);
			int sup = 0;
			for (int k = 0; k < shortBase; k++)
			{
				//两个字节和两个字节之间进行交运算，来构建候选项，并计算其支持度
				ushort_t index = item1s[k] & item2s[k];
				sup += g_bitCountUshort[index];
			}
			//枝剪掉不满足小支持度的2-Item，并将满足条件的item的插入当前项的子项中
			if (sup >= minSup)
			{
				Item item(j);
				item.Increment(sup);
				children->insert(item);
				count ++;
			}
			numAnd++;
			numCount++;
			//BenLog("sup:%d\n", sup); 
		}

	}

	*num2FI = count;

	//BenLog("\t\tcount#: %d\n", numCount);
	//BenLog("\t\tand#%: d\n", numAnd);

}

//---------------------------------------------------
//Internal definitions
//---------------------------------------------------
static void
CountSupport(IN OUT uchar_t *inMatrix, 
					IN size_t numTran, 
					IN size_t numItem,
					IN char * addr)
{
	BEN_ASSERT(inMatrix != NULL);
	/*
	* BenCeilInt 取整函数，numTran/8*byte的内存大小，之后向上取整，这样写的目的就是计算全部的事务可以用多少个字节来表示
	* 因为要用01矩阵，这样做的目的就是计算一个单项在全部事务中的包含情况，用01表示所占的字节
	*/
	int byteBase = BenCeilInt(numTran, (sizeof(byte)*8));
	//(size_t)(byteBase & 0x1)的目的主要就是使shortBase变成2的倍数，便于以后的数据操作
	size_t shortBase = byteBase / 2 + (size_t)(byteBase & 0x1);//short型包含两个字节，级把byteBase转换成short型
	//int intBase = byteBase + 2*sizeof(int);
	int intBase = shortBase * 2 + sizeof(int) * 2;//多的这八个字节就是4字节的itemId和四个字节的FF FF FF FF
	/*
	 *	遍历全部的Item项，计算单个项的支持度存放至intBuf[1]中，即inMatrix
	 */
	for (int i = 0; i < numItem; i++)
	{
		int count = 0;
		// 每次遍历偏移intBase*i个字节，并将其转变为指向int型数组的指针intBuf
		int *intBuf = (int*)(inMatrix + intBase*i);
		//intBuf+2表示跳过两个int长度的字节（即8个字节），之后再将其变为指向byte类型的指针，这时byteBuf真正指向的就是事务的01情况
		byte_t *byteBuf = (byte_t*)(intBuf + 2);
		ushort_t *ushortBuf = (ushort_t*)byteBuf;//ushort_t 为无符号短整型，两个字节表示，即以后偏移量会以2字节为一单位进行偏移
		/*
		 * 将byteBase长的数据分为shortBase个
		 * g_bitCountUshort这个数组包含了2^16次方个短整型数组的每个数对应二进制所包含1的个数
		 * 但这里其实只用包含2^4次方个短整型数即可呀？
		 * 另外这一部分如果也将其用CUDA并行方式完成也可能会有很好的性能提升
		 */
		for (int k = 0; k < shortBase; k++)
			count += g_bitCountUshort[ushortBuf[k]];
		
	/*	if (byteBase & 0x1)
			count += g_bitCountUchar[ushortBuf[byteBase-1]];
	*/	
		intBuf[1] = count;//intBuf数组中intBuf[0]:存放itemId;intBuf[1]:存放itemSup支持度，其实intBuf就是占据了FF FF FF FF的位置
		//*(addr+8+intBase*i+4) =  (char)count;
#ifdef reserve
		int *addrbuf = (int*)(addr+8+intBase*i);
		addrbuf[1] = count;
#endif
	//	BenLog("count:%d\n", count);
		//dprint("oldId: %d - support: %d\n", intBuf[0], intBuf[1]);
	}

}
/*
* 直接去掉level==1时，支持度不满足要求的项，先验定理
* 出口参数outNumItem，返回满足支持度的单项数
* 注意inMatrix指向的是uchar_t型数组，或者指向就是byte型数组
*/
void RemoveFail(IN OUT uchar_t *inMatrix, 
		OUT size_t *outNumItem,
		IN size_t numTran, 
		IN size_t numItem,
		IN size_t minSup)
{
	BEN_ASSERT(inMatrix != NULL);
	//printf("\n!!!!Remove函数:numItem=%d,numTran=%d",numItem,numTran);
	//printf("\n!!!!Remove函数test:inMatrix[i]=%d,%d,%d,%d,%d,%d",inMatrix[0],inMatrix[1],inMatrix[2],inMatrix[3],inMatrix[4],inMatrix[5]);
	int byteBase = BenCeilInt(numTran, sizeof(byte)*8);
	size_t shortBase = byteBase / 2 + (size_t)(byteBase & 0x1);
	//int intBase = byteBase + 2*sizeof(int);
	int intBase = shortBase * 2 + sizeof(int) * 2;

	size_t count = 0;//记录满足最小支持度的单项候选项的个数
	int test=0;
	for (int i = 0; i < numItem; i++)
	{

		int *buf = (int*)(inMatrix + intBase*i);
		if (i<=10)
		{
			//printf("\n!!!!Remove函数,i=%d:buf[1]=%d",i,buf[1]);
		}
		
		if (buf[1] >= minSup)//buf的第二个四字节整型存储的是其支持度，其占据的原本FF FF FF FF的位置
		{
			//如果满足最小支持度，则重新厚茧inMatrix,即将满足要求的全部inMatrix向前覆盖不满足的信息
			uchar_t *cur = (uchar_t*)(inMatrix + intBase*count);
			BenMemcpy(cur, buf, intBase);//
			count++;
			//dprint("%d\n", buf[1]);
		}
		test++;
	}
	*outNumItem = count;
	//printf("\n!!!Remove函数:for_times=%d",test);
	
}
/*
* 从大到小排序
*/
int cmp_func(const void *key1, const void *key2)
{
	int *buf1 = (int*)key1;
	int *buf2 = (int*)key2;

	if (buf1[1] < buf2[1]) return 1;
	if (buf1[1] > buf2[1]) return -1;
	return 0;
}
/*
 * 此函数字面意思就是按照支持度的从大到小进行排序
 * numItem
 */
void SortBySup(IN OUT uchar_t *inMatrix, 
		IN size_t numTran, 
		IN size_t numItem)
{
	int byteBase = BenCeilInt(numTran, sizeof(byte)*8);
	size_t shortBase = byteBase / 2 + (size_t)(byteBase & 0x1);
	//int intBase = byteBase + 2*sizeof(int);
	int intBase = shortBase * 2 + sizeof(int) * 2;
	/*
	 * qsort函数，快排，intBase表示一个元素占用空间的大小，
	 * cmp_func,指向函数的指针，用于确定排序的顺序
	 * 从大到小排序
	 */
	qsort(inMatrix, numItem, intBase, cmp_func);
}

void Encode(IN uchar_t *inMatrix, 
		IN size_t numTran, 
		IN size_t numItem,
		OUT uchar_t **outMatrix,
		OUT IdSup_t **outIdSup,
		OUT size_t *outMatrixSize,
		OUT size_t *outIdSupSize)
{
	BEN_ASSERT(inMatrix != NULL);
	//printf("\n!!!!test:inMatrix[i]=%d,%d,%d,%d,%d,%d",inMatrix[0],inMatrix[1],inMatrix[2],inMatrix[3],inMatrix[4],inMatrix[5]);
	//printf("\n!!!!test:numItem=%d",numItem);
	int byteBase = BenCeilInt(numTran, sizeof(byte)*8);
	size_t shortBase = byteBase / 2 + (size_t)(byteBase & 0x1);
	//int intBase = byteBase + 2*sizeof(int);
	int intBase = shortBase * 2 + sizeof(int) * 2;
	
	size_t count = 0;
	//目前矩阵的大小，在经过枝剪步骤之后
	*outMatrixSize = shortBase * 2 * numItem;
	//全部item组成的信息（IdSup_t）所占的字节数
	*outIdSupSize = sizeof(IdSup_t)*numItem;
	//为出口参数outMatrix分配空间
	*outMatrix = (uchar_t*)BenMalloc(*outMatrixSize);
	//为出口参数outIdSup分配空间
	*outIdSup = (IdSup_t*)BenMalloc(*outIdSupSize);
	for (int i = 0; i < numItem; i++)
	{
		int *buf = (int*)(inMatrix + intBase*i);
		BenMemcpy(*outMatrix+shortBase * 2 * i, (void*)(buf+2), shortBase * 2);
		BenMemcpy(*outIdSup+i, (void*)(buf), sizeof(IdSup_t));
	}
}


void AndPairs(IN ushort_t *item1s,			  
			  IN uchar_t *matrix, 
			  IN int itemId2,
			  IN size_t shortBase,   // number of shorts
			  OUT ushort_t **result)
{
	BEN_ASSERT(item1s != NULL);
	BEN_ASSERT(matrix != NULL);

	uchar_t *item2 = (uchar_t*)(matrix + shortBase * 2 * itemId2);
	ushort_t *item2s = (ushort_t*)(matrix + shortBase * 2 * itemId2);

	*result = (ushort_t*)BenMalloc(shortBase * sizeof(short));

	for (int k = 0; k < shortBase; k++)
	{
		(*result)[k] = item1s[k] & item2s[k];
	}

	//decide whether byteBase is odd
	//if (byteBase & 0x1)
	//{
	//	*oddLastResult = item1OddLast & item2[byteBase-1];
	//}
}

void AndResultBitCount(IN ushort_t *andResult,			  
					   IN size_t shortBase, 
					   OUT int *sup)
{
	BEN_ASSERT(andResult != NULL);

	int count = 0;
	for (int k = 0; k < shortBase; k++)
	{
		ushort_t index = andResult[k];
		count += g_bitCountUshort[index];
	}
	////decide whether byteBase is odd
	//if (byteBase & 0x1)
	//{
	//	count += g_bitCountUchar[oddLastAndResult];
	//}

	*sup = count;
}
