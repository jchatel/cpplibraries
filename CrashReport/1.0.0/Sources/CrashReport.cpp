#include "stdafx.h"
#include "../CrashReport/CrashReport.h"
#include "Dbghelp.h"
#include "time.h"

#pragma comment(lib, "Dbghelp.lib")

namespace CrashReport
{
	std::string CrashDumpFilePrefix = "CrashDump";

// avoiding C Runtime
// -------------------------------------------------------------------------------
char *findlast(LPCTSTR string, int ch)
{
	char *start = (char *)string;

	while (*string++) // find end of string
		;
	// search towards front
	while (--string != start && *string != (char) ch)
		;

	if (*string == (char) ch) // char found ?
		return (char *)string;

	return NULL;
}

// -------------------------------------------------------------------------------
char * GetFilePart(LPCTSTR source)
{
	char *result = findlast(source, '\\');

	if (result)
		result++;
	else
		result = (char *)source;

	return result;
}

// -------------------------------------------------------------------------------
static void __cdecl DumpMiniDump(HANDLE hFile, PEXCEPTION_POINTERS excpInfo)
{
	if (excpInfo == NULL) 
	{
		// Generate exception to get proper context in dump
		__try 
		{
			//OutputDebugString("Raising Exception\r\n");
			RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
		} 
		__except(DumpMiniDump(hFile, GetExceptionInformation()), EXCEPTION_CONTINUE_EXECUTION) 
		{
		}
	} 
	else
	{
		// OutputDebugString(_T("writing minidump\r\n"));
		MINIDUMP_EXCEPTION_INFORMATION eInfo;
		eInfo.ThreadId = GetCurrentThreadId();
		eInfo.ExceptionPointers = excpInfo;
		eInfo.ClientPointers = FALSE;

		// note:  MiniDumpWithIndirectlyReferencedMemory does not work on Win98
		MINIDUMP_TYPE minidumptype = (MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpFilterMemory);

		::MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			hFile,
			minidumptype,
			excpInfo ? &eInfo : NULL,
			NULL,
			NULL);
	}
}

// -------------------------------------------------------------------------------
int __cdecl CrashReport::Create(PEXCEPTION_POINTERS pExceptPtrs)
{
	static bool bFirstTime = true;
	if (!bFirstTime)	// Going recursive! That must mean this routine crashed!
		return EXCEPTION_CONTINUE_SEARCH;
	bFirstTime = false;

	char modulename[MAX_PATH+2];
	modulename[0] = '\0';
	if (GetModuleFileName(0, modulename, sizeof(modulename)) <= 0)
	{
		strcpy_s(modulename, "UnknownModule");
	}

	char *filenamepart = GetFilePart(modulename);
	*filenamepart = '\0';

	// Replace the filename with our minidump file name
	time_t t;
	time(&t);
	struct tm lt;
	localtime_s(&lt, &t);
	int year = lt.tm_year + 1900;
	if (year < 1970)
		year = 2000 + lt.tm_year;

	char buffer[MAX_PATH+1];
	_snprintf_s(buffer, MAX_PATH+1, "%s%s - CrashDump - %.4d-%.2d-%.2d - %.2d-%.2d-%.2d.dmp"
		, modulename, CrashDumpFilePrefix.c_str(), year, lt.tm_mon +1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
	buffer[MAX_PATH] = '\0';

	// Create the file
	HANDLE hMiniDumpFile = CreateFile(buffer, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);

	// Write the minidump to the file
	if (hMiniDumpFile != INVALID_HANDLE_VALUE)
	{
		DumpMiniDump(hMiniDumpFile, pExceptPtrs);

		// Close file
		CloseHandle(hMiniDumpFile);
	}

	// return the magic value which tells Win32 that this handler didn't
	// actually handle the exception - so that things will proceed as per
	// normal.
	return EXCEPTION_CONTINUE_SEARCH;
}


// -------------------------------------------------------------------------------
LONG _stdcall FatalException(_EXCEPTION_POINTERS *ep)
{
	CrashReport::Create(ep);

	return EXCEPTION_CONTINUE_SEARCH;
}


// -------------------------------------------------------------------------------
void __fastcall CrashReport::Initialize(const char *fileprefix)
{
	CrashDumpFilePrefix = fileprefix;

	::SetUnhandledExceptionFilter(FatalException);
}


} // namespace
