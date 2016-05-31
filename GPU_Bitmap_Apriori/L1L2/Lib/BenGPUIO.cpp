#include "BenLib.h"

#ifdef __GPUIO__
int defaultCmpfunc(DB *db, const DBT *key1, const DBT *key2)
{
	int *key11 = (int*)key1->data;
	int *key22 = (int*)key2->data;

	if (*key11 > *key22) return 1;
	if (*key11 < *key22) return -1;
	return 0;
}

GPUIO_t *BenOpenGPUIO(char *fileName, bt_cmp_func_t cmpFunc)
{
	GPUIO_t *gpuIo = (GPUIO_t*)BenMalloc(sizeof(GPUIO_t));
	
	int ret = db_create(&(gpuIo->dbp), NULL, 0);
	if (ret != 0)
	{
		printf("Error:cannot access db file %s\n", fileName);
		exit(-1);
	}

	if (cmpFunc == NULL)
		gpuIo->dbp->set_bt_compare(gpuIo->dbp, defaultCmpfunc);
	else
		gpuIo->dbp->set_bt_compare(gpuIo->dbp, cmpFunc);

	ret = gpuIo->dbp->open(gpuIo->dbp, NULL, fileName, 
		NULL, DB_BTREE, DB_CREATE, 0);

	return gpuIo;
}
 
int BenReadBulk(GPUIO_t *gpuIo, Bulk_t *bulk)
{
	BEN_ASSERT(gpuIo != NULL);
	BEN_ASSERT(bulk != NULL);

	DBT key;
	DBT val;
	BenMemset(&key, 0, sizeof(DBT));
	BenMemset(&val, 0, sizeof(DBT));

	key.data = bulk->key;
	key.size = bulk->keySize;

	gpuIo->dbp->get(gpuIo->dbp, NULL, &key, &val, 0);
	bulk->cpuData = val.data;
	bulk->dataSize = val.size;
	if (bulk->readMode & GPU_MEM)
	{
		bulk->gpuData = D_MALLOC(val.size);
		D_MEMCPY_H2D(bulk->gpuData, bulk->cpuData, bulk->dataSize);
	}
	else
		bulk->gpuData = NULL;

	return 0;
}

int BenWriteBulk(GPUIO_t *gpuIo, Bulk_t *bulk)
{
	BEN_ASSERT(gpuIo != NULL);
	BEN_ASSERT(bulk != NULL);

	DBT key;
	DBT val;
	BenMemset(&key, 0, sizeof(DBT));
	BenMemset(&val, 0, sizeof(DBT));

	key.data = bulk->key;
	key.size = bulk->keySize;

	if (bulk->writeMode == CPU_MEM)
	{
		val.data = bulk->cpuData;
		val.size = bulk->dataSize;
	}
	else if (bulk->writeMode == GPU_MEM)
	{
		val.data = BenMalloc(bulk->dataSize);
		val.size = bulk->dataSize;
		D_MEMCPY_D2H(val.data, bulk->gpuData, bulk->dataSize);
	}
	else
	{
		printf("Error:writeMode setup error! either GPU_MEM or CPU_MEM is allowed!\n");
		exit(-1);
	}

	int ret = gpuIo->dbp->put(gpuIo->dbp, NULL, &key, &val, 0);
	if (ret != 0)
	{
		gpuIo->dbp->err(gpuIo->dbp, ret, "fail");
		exit(-1);
	}
	gpuIo->dbp->sync(gpuIo->dbp, 0);
	if (bulk->writeMode == GPU_MEM)
		BenFree((char**)&val.data, bulk->dataSize);	
	return 0;
}

void BenCloseGPUIO(GPUIO_t *gpuIo)
{
	BEN_ASSERT(gpuIo != NULL);
	
	if (gpuIo->dbp != NULL)
		gpuIo->dbp->close(gpuIo->dbp, 0);

	BenFree((char**)&gpuIo, sizeof(GPUIO_t));
}

void BenResetBulk(GPUIO_t *gpuIo, Bulk_t *bulk)
{
	if (bulk->readMode & GPU_MEM)
		D_FREE(bulk->gpuData, bulk->dataSize);
}
#endif //GPUIO