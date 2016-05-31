#include "BenLib.h"

bool BenCheckArg(int argc, char **argvs, char *arg)
{
	int i;
	for (i = 0; i < argc; i++)
		if (strcmp(argvs[i], arg) == 0) return true;	
	return false;
}

bool BenGetArgInt(int argc, char **argvs, char *argv, int *val)
{
	int i;
	for (i = 0; i < argc-1; i++)
		if (strcmp(argvs[i], argv) == 0) 
		{ 
			*val = atoi(argvs[i+1]);
			return true;
		}
	return false;
}

bool BenGetArgFloat(int argc, char **argvs, char *argv, float *val)
{
	int i;
	for (i = 0; i < argc-1; i++)
		if (strcmp(argvs[i], argv) == 0) 
		{
			*val = atof(argvs[i+1]);
			return true;
		}
	return false;
}

bool BenGetArgStr(int argc, char **argvs, char *argv, char **val)
{
	int i;
	for (i = 0; i < argc-1; i++)
		if (strcmp(argvs[i], argv) == 0) 
		{
			*val = argvs[i+1];
			return true;
		}
	return false;
}

bool BenGetArgStrs(int argc, char **argvs, char *argv, char **val, int *len)
{
	int i, j;
	*len = 0;
	for (i = 0; i < argc-1; i++)
		if (strcmp(argvs[i], argv) == 0) 
		{
			int k = 0;
			for (j = i+1; j < argc; j++)
			{
				val[k] = argvs[j];
				(*len)++;
				k++;
			}
			return true;
		}
	return false;
}

void BenPrintUsage(char *fmt, ...)
{
	va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

	exit(0);
}