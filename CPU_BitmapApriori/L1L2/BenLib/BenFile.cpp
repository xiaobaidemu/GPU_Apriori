#include "BenLib.h"

double io_time = 0.0;
void BenWriteFile(char *fileName, void *buf, size_t size)
{
	BenSetup(6);
	BenStart(6);
#ifdef __UNIX__
#else //__UNIX__
	HANDLE hFile; 
	hFile = CreateFileA(fileName,               // file to open
                       GENERIC_WRITE,          
                       FILE_SHARE_WRITE,      
                       NULL,                  // default security
                       CREATE_ALWAYS,         // existing file only
                       FILE_ATTRIBUTE_NORMAL, // normal file
                       NULL);                 // no attr. template
	BEN_ASSERT(hFile != INVALID_HANDLE_VALUE );

	DWORD dwResult = 0; 
	BEN_ASSERT(WriteFile (hFile, buf, size, &dwResult, NULL));
	//BenLog("should write %d bytes, actuall %d bytes\n", size, dwResult);
	CloseHandle(hFile);
#endif //__WIN32__
	BenStop(6);
	io_time += BenGetElapsedTime(6);
}

void BenAppendFile(char *fileName, void *buf, size_t size)
{
	BenSetup(6);
	BenStart(6);
#ifdef __UNIX__
#else //__UNIX__
	//constrian 4GB file size.
	//Todo: use SetFilePointer later
#define FILE_WRITE_TO_END_OF_FILE       0xffffffff

	HANDLE hFile; 
	hFile = CreateFileA(fileName,               // file to open
                       GENERIC_WRITE,          
                       FILE_SHARE_WRITE,      
                       NULL,                  // default security
                       OPEN_ALWAYS,         // existing file only
                       FILE_ATTRIBUTE_NORMAL, // normal file
                       NULL);                 // no attr. template
	BEN_ASSERT(hFile != INVALID_HANDLE_VALUE );

	DWORD dwResult = 0; 
	OVERLAPPED o;
	BenMemset(&o, 0, sizeof(o));
	o.Offset = FILE_WRITE_TO_END_OF_FILE; 
	o.OffsetHigh = -1;
	BEN_ASSERT(WriteFile (hFile, buf, size, &dwResult, &o));
	//BenLog("should write %d bytes, actuall %d bytes\n", size, dwResult);
	CloseHandle(hFile);
#endif //__WIN32__
	BenStop(6);
	io_time += BenGetElapsedTime(6);
}

char *BenReadFile(char *fileName, size_t offset, size_t size)
{
	BenSetup(6);
	BenStart(6);
	char *buf = (char*)BenMalloc(size);
#ifdef __UNIX__
#else //__UNIX__
	HANDLE hFile; 
	hFile = CreateFileA(fileName,               // file to open
                       GENERIC_READ,          
                       FILE_SHARE_READ,      
                       NULL,                  // default security
                       OPEN_EXISTING,         // existing file only
                       FILE_ATTRIBUTE_NORMAL, // normal file
                       NULL);                 // no attr. template
	BEN_ASSERT(hFile != INVALID_HANDLE_VALUE );

	DWORD dwResultSize = 0; 
	OVERLAPPED o;
	BenMemset(&o, 0, sizeof(o));
	o.Offset = offset;
	BEN_ASSERT(ReadFile (hFile, buf, size, &dwResultSize, &o));
	//BenLog("should read %d bytes, actually %d bytes\n", size, dwResultSize);
	CloseHandle(hFile);
#endif //__WIN32__
	return buf;
	BenStop(6);
	io_time += BenGetElapsedTime(6);
}

File_t BenOpen(char *fileName, char rwa)
{
	return 0;
}

size_t BenRead(File_t handle, char *buf, size_t fileOffset, size_t size)
{
	return -1;
}

size_t BenWrite(File_t handle, char *buf, size_t size)
{
	return -1;
}

BenMM_t *BenStartMMap(char *fileName)
{
	BenMM_t *mm = (BenMM_t*)BenMalloc(sizeof(BenMM_t));
#ifndef __UNIX__
	HANDLE hFile = CreateFile(fileName, 
		GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
	BEN_ASSERT(hFile != INVALID_HANDLE_VALUE);

	HANDLE hFileMap = CreateFileMapping(hFile, 
		NULL, PAGE_READWRITE, 0, 0, NULL);
	BEN_ASSERT(hFileMap != NULL);

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

	char *lpbMapAddress = (char *)MapViewOfFile(hFileMap,FILE_MAP_ALL_ACCESS,
	  (DWORD)(qwFileOffset >> 32), (DWORD)(qwFileOffset & 0xFFFFFFFF),dwBlockBytes);
	BEN_ASSERT(lpbMapAddress != NULL);

	CloseHandle(hFile); 

	mm->addr = lpbMapAddress;
	mm->hMap = hFileMap;
	mm->fileSize = qwFileSize;
#endif //__UNIX__

	return mm;
}
 
void BenEndMMap(BenMM_t *mm)
{
#ifndef __UNIX__
	UnmapViewOfFile(mm->addr);
	CloseHandle(mm->hMap);
#endif __UNIX__
	BenFree((char**)&mm, sizeof(BenMM_t));
}

BenDir_t *BenEnterDir(char *dirname)
{
	BEN_ASSERT(dirname != NULL);
	BenDir_t *dir = (BenDir_t*)BenMalloc(sizeof(BenDir_t));
	
#ifndef __UNIX__
	dir->hDir = FindFirstFile(dirname, &dir->wfd); 
	BEN_ASSERT(dir->hDir != INVALID_HANDLE_VALUE);
#endif //__UNIX__

	return dir;
}

char BenFindDir(BenDir_t *dir, BenFileInDir_t *file)
{
	BEN_ASSERT(dir != NULL);
	BEN_ASSERT(file != NULL);

#ifndef __UNIX__
	if (dir->wfd.cFileName[0] != '.')
	{
		if (!FindNextFile(dir->hDir, &dir->wfd))
			return 0;
	} 

	while (dir->wfd.cFileName[0] == '.')
	{
		if (!FindNextFile(dir->hDir, &dir->wfd))
			return 0;
	}

	file->fileId++;
	file->filename = dir->wfd.cFileName;
	file->fileSize = dir->wfd.nFileSizeLow;
#endif //__UNIX__
	return 1;
}

void BenLeaveDir(BenDir_t *dir)
{
	BEN_ASSERT(dir != NULL);
#ifndef __UNIX__
	CloseHandle(dir->hDir);
#endif //__UNIX__
	BenFree((char**)&dir, sizeof(BenDir_t));
}

size_t BenGetFileSize(char *fileName)
{
	HANDLE hFile = CreateFile(fileName, 
		GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
	 DWORD dwFileSizeHigh;
	 __int64 qwFileSize = GetFileSize(hFile, &dwFileSizeHigh);
	 qwFileSize |= (((__int64)dwFileSizeHigh) << 32);
	 return (size_t)qwFileSize;
}