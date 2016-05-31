#include "FIMEncode.h"

//--------------------------------------------------
//Forward declarations
//--------------------------------------------------
static void
CountSupport(IN OUT uchar_t *inMatrix, 
					IN size_t numTran, 
					IN size_t numItem);
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
	//Memory Mapping
	BenMM_t *mm = BenStartMMap(fileName);
	size_t matrixSize = mm->fileSize-MATRIX_OFFSET;
	uchar_t *matrix = (uchar_t*)BenMalloc(matrixSize);
	size_t numTran = 0;
	size_t numItem = 0;
	BenMemcpy(&numTran, mm->addr+NUM_TRAN_OFFSET, sizeof(size_t));
	BenMemcpy(&numItem, mm->addr+NUM_ITEM_OFFSET, sizeof(size_t));
	BenMemcpy(matrix, mm->addr+MATRIX_OFFSET, matrixSize);
	BenEndMMap(mm);
	*outNumTran = numTran;
	dprint("file:%s, numTran:%d, numItem:%d, matrixSize:%d bytes\n", 
		fileName, numTran, numItem, matrixSize);
	BenStop(0);
	BenPrintTime("io", 0);

	//count
	BenSetup(0);
	BenStart(0);
	CountSupport(matrix, numTran, numItem);
	BenStop(0);
	//BenPrintTime("count", 0);
	//BenLog("\t\t\tinput item#%d\n", numItem);

	//remove
	BenSetup(0);
	BenStart(0);
	RemoveFail(matrix, outNumItem, numTran, numItem, minSup);
	dprint("#freq: %d\n", *outNumItem);
	BenStop(0);
	//BenPrintTime("remove", 0);

	//sort
	BenSetup(0);
	BenStart(0);
	SortBySup(matrix, numTran, *outNumItem);
	BenStop(0);
	//BenPrintTime("sort", 0);

	//encode
	BenSetup(0);
	BenStart(0);
	Encode(matrix, numTran, *outNumItem, outMatrix, outIdSup, outMatrixSize, outIdSupSize);
	BenStop(0);
	//BenPrintTime("encode", 0);

	BenFree((char**)&matrix, matrixSize);
}

void PairwiseCount(IN uchar_t *matrix, 
				   IN size_t numTran, 
				   IN size_t numItem,
				   IN int minSup, 
				   OUT Item *trie, 
				   OUT size_t *num2FI)
{
	BEN_ASSERT(matrix != NULL);

	size_t byteBase = BenCeilInt(numTran, 8);
	size_t shortBase = byteBase / 2  + (size_t)(byteBase & 0x1);

	int numCount = 0;
	int numAnd = 0;
	int count = 0;

	set<Item> *items = trie->getChildren();
	
	for(set<Item>::iterator runner = items->begin(); runner != items->end(); runner++)
	{
		set<Item> *children = runner->makeChildren();
		int i = runner->getId();

		//uchar_t *item1 = (uchar_t*)(matrix + byteBase * i);
		ushort_t *item1s = (ushort_t*)(matrix + shortBase * 2 * i);
		
		//printf("%d\n", i);
		for (int j = i+1; j < numItem; j++)
		{
			//uchar_t *item2 = (uchar_t*)(matrix + byteBase * j);
			ushort_t *item2s = (ushort_t*)(matrix + shortBase * 2 * j);
			int sup = 0;
			for (int k = 0; k < shortBase; k++)
			{
				ushort_t index = item1s[k] & item2s[k];
				sup += g_bitCountUshort[index];
			}
	/*		//decide whether byteBase is odd
			if (byteBase & 0x1)
			{
				uchar_t index = item1[byteBase-1] & item2[byteBase-1];
				sup += g_bitCountUchar[index];
			}
	*/		
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
					IN size_t numItem)
{
	BEN_ASSERT(inMatrix != NULL);

	int byteBase = BenCeilInt(numTran, (sizeof(byte)*8));
	size_t shortBase = byteBase / 2 + (size_t)(byteBase & 0x1);
	//int intBase = byteBase + 2*sizeof(int);
	int intBase = shortBase * 2 + sizeof(int) * 2;

	for (int i = 0; i < numItem; i++)
	{
		int count = 0;
		int *intBuf = (int*)(inMatrix + intBase*i);
		byte_t *byteBuf = (byte_t*)(intBuf + 2);
		ushort_t *ushortBuf = (ushort_t*)byteBuf;

		for (int k = 0; k < shortBase; k++)
			count += g_bitCountUshort[ushortBuf[k]];
	/*	if (byteBase & 0x1)
			count += g_bitCountUchar[ushortBuf[byteBase-1]];
	*/	
		intBuf[1] = count;
	//	BenLog("count:%d\n", count);
		//dprint("oldId: %d - support: %d\n", intBuf[0], intBuf[1]);
	}
}

void RemoveFail(IN OUT uchar_t *inMatrix, 
		OUT size_t *outNumItem,
		IN size_t numTran, 
		IN size_t numItem,
		IN size_t minSup)
{
	BEN_ASSERT(inMatrix != NULL);

	int byteBase = BenCeilInt(numTran, sizeof(byte)*8);
	size_t shortBase = byteBase / 2 + (size_t)(byteBase & 0x1);
	//int intBase = byteBase + 2*sizeof(int);
	int intBase = shortBase * 2 + sizeof(int) * 2;

	size_t count = 0;
	for (int i = 0; i < numItem; i++)
	{
		int *buf = (int*)(inMatrix + intBase*i);
		if (buf[1] >= minSup)
		{
			uchar_t *cur = (uchar_t*)(inMatrix + intBase*count);
			BenMemcpy(cur, buf, intBase);
			count++;
			//dprint("%d\n", buf[1]);
		}
	}
	*outNumItem = count;
}

int cmp_func(const void *key1, const void *key2)
{
	int *buf1 = (int*)key1;
	int *buf2 = (int*)key2;

	if (buf1[1] < buf2[1]) return 1;
	if (buf1[1] > buf2[1]) return -1;
	return 0;
}

void SortBySup(IN OUT uchar_t *inMatrix, 
		IN size_t numTran, 
		IN size_t numItem)
{
	int byteBase = BenCeilInt(numTran, sizeof(byte)*8);
	size_t shortBase = byteBase / 2 + (size_t)(byteBase & 0x1);
	//int intBase = byteBase + 2*sizeof(int);
	int intBase = shortBase * 2 + sizeof(int) * 2;

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

	int byteBase = BenCeilInt(numTran, sizeof(byte)*8);
	size_t shortBase = byteBase / 2 + (size_t)(byteBase & 0x1);
	//int intBase = byteBase + 2*sizeof(int);
	int intBase = shortBase * 2 + sizeof(int) * 2;
	
	size_t count = 0;
	*outMatrixSize = shortBase * 2 * numItem;
	*outIdSupSize = sizeof(IdSup_t)*numItem;
	*outMatrix = (uchar_t*)BenMalloc(*outMatrixSize);
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
