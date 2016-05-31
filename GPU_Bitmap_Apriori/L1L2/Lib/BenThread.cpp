#include "BenLib.h"

#ifdef __UNIX__
BenThread_t BenNewThread(void *(*start_routine)(void*),  void *arg)
{
	BenThread_t threadId = -1;
	int rc = -1;

	rc = pthread_create(&threadId, NULL, start_routine, (BenVoid *)arg);

	if (rc != 0) return -1;

	return threadId;
}

void BenExitThread(void *status)
{
	pthread_exit(status);
}

void BenWaitForSingle(BenThread_t id)
{
	pthread_join(id, NULL);
}

void BenWaitForMul(BenThread_t* ids, int num)
{
	int i;
	for (i = 0; i < num; i++)
	{
		pthread_join(ids[i], NULL);
	}		
}
#else //__UNIX__
BenThread_t BenNewThread(void *(*start_routine)(void*),  void *arg)
{
	BenThread_t thread = CreateThread(NULL, 0, 
		(LPTHREAD_START_ROUTINE)start_routine, arg, 0, NULL);
	return thread;
}

void BenExitThread(void *status)
{
	ExitThread(0);
}

void BenWaitForSingle(BenThread_t id)
{
	WaitForSingleObject(id, INFINITE);
}

void BenWaitForMul(BenThread_t* ids, int num)
{
	WaitForMultipleObjects(num, ids, TRUE, INFINITE);
}
#endif //__WIN32__