#include "BenLib.h"

#ifdef __MEM__
size_t d_memUsage = 0;
size_t d_memTransfer = 0;
#endif //__MEM__

void *BenMalloc(size_t size)
{
	BEN_ASSERT(size > 0);
	void *buf = malloc(size);
	BEN_ASSERT(buf != NULL);
	BenMemset(buf, 0, size);
#ifdef __MEM__
#	ifdef __ALLOC__
	BenLog("+%d bytes\n", size);
#	endif //__ALLOC__
	d_memUsage += size;
#endif //__MEM__
	return buf;
}

void BenFree(char **buf, size_t size)
{
	if (size != 0)
	{
		if (buf == NULL)
			return;//BEN_ASSERT(*buf != NULL);
	}
	else
		return;

	free(*buf);
	*buf = NULL;
#ifdef __MEM__
#	ifdef __ALLOC__
	BenLog("-%d bytes\n", size);
#	endif //__ALLOC__
	d_memUsage -= size;
#endif //__MEM__
}

void BenMemset(void *buf, int c, size_t size)
{
	BEN_ASSERT(buf != NULL);
	BEN_ASSERT(size > 0);
	memset(buf, c, size);

}

void *BenRealloc(void *buf, size_t oldSize, size_t newSize)
{
	//BEN_ASSERT(oldSize <= newSize);
	void *newBuf = realloc(buf, newSize);

#ifdef __MEM__
	size_t size = newSize - oldSize;
#	ifdef __ALLOC__
	BenLog("+%d bytes\n", size);
#	endif //__ALLOC__
	d_memUsage += size;
#endif//__MEM__

	return newBuf;
}

void BenMemcpy(void *dest, void *src, size_t size)
{
	BEN_ASSERT(dest != NULL);
	BEN_ASSERT(src != NULL);
	BEN_ASSERT(size > 0);

	memcpy(dest, src, size);

#ifdef __MEM__
#	ifdef __ALLOC__
	BenLog("<=>%d bytes\n", size);
#	endif //__ALLOC__
	d_memTransfer += size;
#endif //__MEM__
}

char *BenStrDup(char *str)
{
	BEN_ASSERT(str != NULL);
	char *buf = _strdup(str);
	BEN_ASSERT(buf != NULL);
#ifdef __MEM__
	size_t size = strlen(str)+1;
#	ifdef __ALLOC__
	BenLog("+%d bytes\n", size);
#	endif //__ALLOC__
	d_memUsage += size;
#endif//__MEM__
	return buf;
}

int BenStrLen(char *str)
{
	if (str == NULL) return -1;
	return strlen(str);
}

void EnterFunc(char *funcName)
{
	BenLog("=====Enter %s=====\n", funcName);
}

void LeaveFunc(char *funcName)
{ 
#ifdef __MEM__
	BenLog("=====Leave %s: %d bytes=====\n", funcName, d_memUsage);
#else
	BenLog("=====Leave %s =======\n", funcName);
#endif //__MEM__
}