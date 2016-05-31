/*
 *System-independent library.
 *Written by Wenbin FANG (wenbin@cse.ust.hk)
 *
 *COMPILE MACROS:
 *1, __UNIX__ or __WIN32__
 *2, __TIMER__
 *3, __DEBUG__
 *4, __CUDA__
 */

#include "BenLib.h"

CPerfCounter a[5];
#ifdef __TIMER__
void BenSetup(int i)
{
    a[i]._clocks = 0;
    a[i]._start = 0;
#ifdef __UNIX__
    a[i]._freq = 1000;
#else
    QueryPerformanceFrequency((LARGE_INTEGER *)&(a[i]._freq));
#endif
    BenReset(i);
}

void BenStart(int i)
{
#ifdef __UNIX__
    struct timeval s;
    gettimeofday(&s, 0);
    a[i]._start = (i64)s.tv_sec * 1000 + (i64)s.tv_usec / 1000;
#else //__UNIX__
    QueryPerformanceCounter((LARGE_INTEGER *)&(a[i]._start));
#endif //__WIN32__
}

void BenStop(int i)
{
    i64 n = 0;

#ifdef __UNIX__
    struct timeval s;
    gettimeofday(&s, 0);
    n = (i64)s.tv_sec * 1000 + (i64)s.tv_usec / 1000;
#else //__UNIX__
    QueryPerformanceCounter((LARGE_INTEGER *)&n);
#endif //__WIN32__
    n -= a[i]._start;
    a[i]._start = 0;
    a[i]._clocks += n;
}

void BenReset(int i)
{
    a[i]._clocks = 0;
}

double BenGetElapsedTime(int i)
{
    return (double)a[i]._clocks / (double)a[i]._freq;
}

void BenPrintTime(char *msg, int i)
{
	if (msg != NULL)
		printf("%s: %lf s\n", msg, BenGetElapsedTime(i));
	else
		printf("%lf s\n", BenGetElapsedTime(i)); 
}
#else
void BenSetup(int){}
void BenStart(int){}
void BenStop(int){}
void BenReset(int){}
double BenGetElapsedTime(int){ return 0.0; }
void BenPrintTime(char *, int){}
#endif //__TIME__

void BenStartTimer(BenTimeVal_t *start_tv)
{
#ifdef __UNIX__
   gettimeofday(start_tv, NULL);
#else
	QueryPerformanceCounter((LARGE_INTEGER *)start_tv);
#endif
}

double BenEndTimer(BenTimeVal_t *start_tv)
{
#ifdef __UNIX__
   BenTimeVal_t end_tv;

   gettimeofday(&end_tv, NULL);

   time_t sec = end_tv.tv_sec - start_tv->tv_sec;
   time_t ms = end_tv.tv_usec - start_tv->tv_usec;

   time_t diff = sec * 1000000 + ms;

   return (double)((double)diff/1000.0);
#else
	BenTimeVal_t end_tv;
	BenTimeVal_t ticksPerSecond;
	
	QueryPerformanceFrequency(&ticksPerSecond); 
	QueryPerformanceCounter((LARGE_INTEGER *)&end_tv);

	return (double)(end_tv.QuadPart-start_tv->QuadPart)/(double)ticksPerSecond.QuadPart;
#endif
}