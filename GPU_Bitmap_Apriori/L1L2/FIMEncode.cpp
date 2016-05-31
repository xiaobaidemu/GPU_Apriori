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
	*  Memory Mapping�ڴ�ӳ�������ļ�
	*  ����BenMM_t�ṹ�壬���а�����ӳ����ļ���ӳ�俪ʼ��ַ���ļ���С���Լ��ļ�ӳ���ں�
	*/
	BenMM_t *mm = BenStartMMap(fileName);
	/*
	BenMM_t *mm = (BenMM_t*)BenMalloc(sizeof(BenMM_t));
	FILE *f = fopen(fileName,"wb+");
	if (f == NULL)
	{
		printf("�ļ��޷���");
		
	}
	else{
		printf("�ļ��򿪳ɹ�");
	}
	fseek(f,0,SEEK_END);
	mm->fileSize = ftell(f);
	printf("wenjian daxiao =%d",ftell(f));

	mm->addr = f->_base;
	*/
	size_t matrixSize = mm->fileSize-MATRIX_OFFSET;//MATRIX_OFFSET=(sizeof(size_t)*2)
	//printf("\n!!!!mm->fileSize=%d",mm->fileSize);
	//printf("\n!!!!filesize = %d",matrixSize);

	uchar_t *matrix = (uchar_t*)BenMalloc(matrixSize);//Ϊ�������ռ�
	size_t numTran = 0;
	size_t numItem = 0;
	BenMemcpy(&numTran, mm->addr+NUM_TRAN_OFFSET, sizeof(size_t));//���ļ���ӳ�俪ʼ��ַ��ʼ����4���ֽڵ�numTran�ĵ�ַ��
	BenMemcpy(&numItem, mm->addr+NUM_ITEM_OFFSET, sizeof(size_t));//�ڴ��ļ�ӳ�俪ʼ��ַ���ĸ��ֽڿ�ʼ����4���ֽڵ�numItem�ĵ�ַ��
	BenMemcpy(matrix, mm->addr+MATRIX_OFFSET, matrixSize);//���ļ�ӳ�俪ʼ��ַ��˸��ֽڿ�ʼ����matrixSize���ֽ�
	
	//printf("\n!!!!test1:matrix[i]=%d,%d,%d,%d,%d,%d",matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);//matrixĿǰû������
	//BenEndMMap(mm);//�ͷ���Ӧ��Դ
	*outNumTran = numTran;//���ڲ���
	dprint("file:%s, numTran:%d, numItem:%d, matrixSize:%d bytes\n", 
		fileName, numTran, numItem, matrixSize);
	//printf("\nfile:%s, numTran:%d, numItem:%d, matrixSize:%d bytes\n", 
		//fileName, *outNumTran, numItem, matrixSize);
	BenStop(0);
	BenPrintTime("io", 0);

	//count
	BenSetup(0);
	BenStart(0);
	//ʹ��CountSupport���㵥����Ϊ��ѡ��ʱ��֧�ֶ�
	CountSupport(matrix, numTran, numItem,mm->addr);
	//printf("\n!!!!test:numItem=%d",numItem);
	//printf("\n!!!!test2:matrix[i]=%d,%d,%d,%d,%d,%d",matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
	BenStop(0);
	//BenPrintTime("count", 0);
	BenEndMMap(mm);//�ͷ���Ӧ��Դ
	//BenLog("\t\t\tinput item#%d\n", numItem);

	//remove
	BenSetup(0);
	BenStart(0);
	//�������RemoveFail������
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
	//�ͷ�matrix���ָ����ָ��Ŀռ�
	BenFree((char**)&matrix, matrixSize);
}
/*
 * ����level=2ʱ��֧�ֶ�
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
	//(size_t)(byteBase & 0x1)��Ŀ����Ҫ����ʹshortBase���2�ı����������Ժ�����ݲ���
	size_t shortBase = byteBase / 2  + (size_t)(byteBase & 0x1);

	int numCount = 0;
	int numAnd = 0;
	int count = 0;

	set<Item> *items = trie->getChildren();//����trie���ӽڵ�set����Ҳ����Ƶ���Ϊ1����
	
	for(set<Item>::iterator runner = items->begin(); runner != items->end(); runner++)
	{
		set<Item> *children = runner->makeChildren();//Ϊÿһ��trie���Ӽ�������һ��set��
		int i = runner->getId();

		//uchar_t *item1 = (uchar_t*)(matrix + byteBase * i);
		ushort_t *item1s = (ushort_t*)(matrix + shortBase * 2 * i);
		
		//printf("%d\n", i);
		/*
		 * ��ǰfreItemҪ���������FreItem��������
		 */
		for (int j = i+1; j < numItem; j++)
		{
			//uchar_t *item2 = (uchar_t*)(matrix + byteBase * j);
			ushort_t *item2s = (ushort_t*)(matrix + shortBase * 2 * j);
			int sup = 0;
			for (int k = 0; k < shortBase; k++)
			{
				//�����ֽں������ֽ�֮����н����㣬��������ѡ���������֧�ֶ�
				ushort_t index = item1s[k] & item2s[k];
				sup += g_bitCountUshort[index];
			}
			//֦����������С֧�ֶȵ�2-Item����������������item�Ĳ��뵱ǰ���������
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
	* BenCeilInt ȡ��������numTran/8*byte���ڴ��С��֮������ȡ��������д��Ŀ�ľ��Ǽ���ȫ������������ö��ٸ��ֽ�����ʾ
	* ��ΪҪ��01������������Ŀ�ľ��Ǽ���һ��������ȫ�������еİ����������01��ʾ��ռ���ֽ�
	*/
	int byteBase = BenCeilInt(numTran, (sizeof(byte)*8));
	//(size_t)(byteBase & 0x1)��Ŀ����Ҫ����ʹshortBase���2�ı����������Ժ�����ݲ���
	size_t shortBase = byteBase / 2 + (size_t)(byteBase & 0x1);//short�Ͱ��������ֽڣ�����byteBaseת����short��
	//int intBase = byteBase + 2*sizeof(int);
	int intBase = shortBase * 2 + sizeof(int) * 2;//�����˸��ֽھ���4�ֽڵ�itemId���ĸ��ֽڵ�FF FF FF FF
	/*
	 *	����ȫ����Item����㵥�����֧�ֶȴ����intBuf[1]�У���inMatrix
	 */
	for (int i = 0; i < numItem; i++)
	{
		int count = 0;
		// ÿ�α���ƫ��intBase*i���ֽڣ�������ת��Ϊָ��int�������ָ��intBuf
		int *intBuf = (int*)(inMatrix + intBase*i);
		//intBuf+2��ʾ��������int���ȵ��ֽڣ���8���ֽڣ���֮���ٽ����Ϊָ��byte���͵�ָ�룬��ʱbyteBuf����ָ��ľ��������01���
		byte_t *byteBuf = (byte_t*)(intBuf + 2);
		ushort_t *ushortBuf = (ushort_t*)byteBuf;//ushort_t Ϊ�޷��Ŷ����ͣ������ֽڱ�ʾ�����Ժ�ƫ��������2�ֽ�Ϊһ��λ����ƫ��
		/*
		 * ��byteBase�������ݷ�ΪshortBase��
		 * g_bitCountUshort������������2^16�η��������������ÿ������Ӧ������������1�ĸ���
		 * ��������ʵֻ�ð���2^4�η���������������ѽ��
		 * ������һ�������Ҳ������CUDA���з�ʽ���Ҳ���ܻ��кܺõ���������
		 */
		for (int k = 0; k < shortBase; k++)
			count += g_bitCountUshort[ushortBuf[k]];
		
	/*	if (byteBase & 0x1)
			count += g_bitCountUchar[ushortBuf[byteBase-1]];
	*/	
		intBuf[1] = count;//intBuf������intBuf[0]:���itemId;intBuf[1]:���itemSup֧�ֶȣ���ʵintBuf����ռ����FF FF FF FF��λ��
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
* ֱ��ȥ��level==1ʱ��֧�ֶȲ�����Ҫ�������鶨��
* ���ڲ���outNumItem����������֧�ֶȵĵ�����
* ע��inMatrixָ�����uchar_t�����飬����ָ�����byte������
*/
void RemoveFail(IN OUT uchar_t *inMatrix, 
		OUT size_t *outNumItem,
		IN size_t numTran, 
		IN size_t numItem,
		IN size_t minSup)
{
	BEN_ASSERT(inMatrix != NULL);
	//printf("\n!!!!Remove����:numItem=%d,numTran=%d",numItem,numTran);
	//printf("\n!!!!Remove����test:inMatrix[i]=%d,%d,%d,%d,%d,%d",inMatrix[0],inMatrix[1],inMatrix[2],inMatrix[3],inMatrix[4],inMatrix[5]);
	int byteBase = BenCeilInt(numTran, sizeof(byte)*8);
	size_t shortBase = byteBase / 2 + (size_t)(byteBase & 0x1);
	//int intBase = byteBase + 2*sizeof(int);
	int intBase = shortBase * 2 + sizeof(int) * 2;

	size_t count = 0;//��¼������С֧�ֶȵĵ����ѡ��ĸ���
	int test=0;
	for (int i = 0; i < numItem; i++)
	{

		int *buf = (int*)(inMatrix + intBase*i);
		if (i<=10)
		{
			//printf("\n!!!!Remove����,i=%d:buf[1]=%d",i,buf[1]);
		}
		
		if (buf[1] >= minSup)//buf�ĵڶ������ֽ����ʹ洢������֧�ֶȣ���ռ�ݵ�ԭ��FF FF FF FF��λ��
		{
			//���������С֧�ֶȣ������º��inMatrix,��������Ҫ���ȫ��inMatrix��ǰ���ǲ��������Ϣ
			uchar_t *cur = (uchar_t*)(inMatrix + intBase*count);
			BenMemcpy(cur, buf, intBase);//
			count++;
			//dprint("%d\n", buf[1]);
		}
		test++;
	}
	*outNumItem = count;
	//printf("\n!!!Remove����:for_times=%d",test);
	
}
/*
* �Ӵ�С����
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
 * �˺���������˼���ǰ���֧�ֶȵĴӴ�С��������
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
	 * qsort���������ţ�intBase��ʾһ��Ԫ��ռ�ÿռ�Ĵ�С��
	 * cmp_func,ָ������ָ�룬����ȷ�������˳��
	 * �Ӵ�С����
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
	//Ŀǰ����Ĵ�С���ھ���֦������֮��
	*outMatrixSize = shortBase * 2 * numItem;
	//ȫ��item��ɵ���Ϣ��IdSup_t����ռ���ֽ���
	*outIdSupSize = sizeof(IdSup_t)*numItem;
	//Ϊ���ڲ���outMatrix����ռ�
	*outMatrix = (uchar_t*)BenMalloc(*outMatrixSize);
	//Ϊ���ڲ���outIdSup����ռ�
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
