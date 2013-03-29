#include "StdAfx.h"
#include "FileReporter.h"


using namespace Reporter;

// -------------------------------------------------------------------------------
FileReporter::FileReporter(const char *filename)
: mFilename(filename), mFile(NULL), mPreviousDay(99)
{
	mFile = CreateFile(filename, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

// -------------------------------------------------------------------------------
FileReporter::~FileReporter()
{
	if (mFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(mFile);
	}
	mFile = 0;
}

// -------------------------------------------------------------------------------
void	FileReporter::ReportFormatted(SEVERITY severity, const char *function, const char *filename, long line, const char *message, ...)
{
	char	buffer[4096];

	va_list args;
	va_start(args, message);
	if (_vsnprintf_s(buffer, 4096, message, args) == -1)
	{
	}
	va_end(args);

	buffer[4095] = '\0';

	Report(severity, function, filename, line, buffer);
}
 
// -------------------------------------------------------------------------------
void	FileReporter::Report(SEVERITY severity, const char *function, const char *filename, long line, const char *message)
{
	if (mFile == NULL)
		return;

	Utils::Lock lock(mCriticalSection);

	char	buffer[4096];
	DWORD bytewritten = 0;

	SYSTEMTIME systime;
	GetSystemTime(&systime);
	
	DWORD threadid = GetCurrentThreadId();

	if (mPreviousDay != systime.wDay)
	{
		// New day, insert a log line

		sprintf_s(buffer, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d:%.3d \t%.4X\tLOG:\t---------------------------- NEW DAY %.2d/%.2d ----------------------------\r\n", systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds, threadid, systime.wDay, systime.wMonth);
		WriteFile(mFile, buffer, (DWORD)strlen(buffer), &bytewritten, NULL);

		mPreviousDay = systime.wDay;
	}

	sprintf_s(buffer, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d:%.3d \t%.4X\t", systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds, threadid);
	WriteFile(mFile, buffer, (DWORD)strlen(buffer), &bytewritten, NULL);

	switch (severity)
	{
	case IReporter::eLOG:
		WriteFile(mFile, "LOG:\t", 5, &bytewritten, NULL);
		break;
	case IReporter::eWARNING:
		WriteFile(mFile, "WAR:\t", 5, &bytewritten, NULL);
		break;
	case IReporter::eERROR:
		WriteFile(mFile, "ERR:\t", 5, &bytewritten, NULL);
		break;
	case IReporter::eFATAL:
		WriteFile(mFile, "FAT:\t", 5, &bytewritten, NULL);
		break;
	}

	WriteFile(mFile, message, (DWORD)strlen(message), &bytewritten, NULL);
	WriteFile(mFile, "\r\n", 2, &bytewritten, NULL);

//	fwrite(buffer, strlen(buffer), 1, mFile);
//	fwrite("\r\n", 2, 1, mFile);

	//	fflush(mFile);
}


