#ifndef __FIMENCODE_H__
#define __FIMENCODE_H__

#include "BenLib/BenLib.h"
#include "../APRIORI/Item.h"


//numTran | numItem | matrix
#define NUM_TRAN_OFFSET		0
#define NUM_ITEM_OFFSET		sizeof(size_t)
#define MATRIX_OFFSET		(sizeof(size_t)*2)

typedef struct
{
	int id;
	int sup;
} IdSup_t;

typedef struct
{
	size_t item1;
	size_t item2;
	size_t count;
} PairwiseCount_t;

extern "C"
{

void EncodeBin(IN char *fileName,
			   IN int minSup,
			   OUT unsigned char **outMatrix,
			   OUT IdSup_t **outIdSup,
			   OUT size_t *outNumTran,
			   OUT size_t *outNumItem,
			   OUT size_t *outMatrixSize,
			   OUT size_t *outIdSupSize);

void PairwiseCount(IN uchar_t *matrix, 
				   IN size_t numTran, 
				   IN size_t numItem,
				   IN int minSup, 
				   OUT Item *trie, 
				   OUT size_t *num2FI);

void AndPairs(IN ushort_t *item1s,			  
			  IN uchar_t *matrix, 
			  IN int itemId2,
			  IN size_t shortBase,   // number of shorts
			  OUT ushort_t **result);

void AndResultBitCount(IN ushort_t *andResult,
					   IN size_t shortBase, 
					   OUT int *sup);


}
#endif //__FIMENCODE_H__

