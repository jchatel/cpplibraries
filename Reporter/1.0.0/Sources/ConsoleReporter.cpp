#include "StdAfx.h"
#include "ConsoleReporter.h"


using namespace Reporter;

// -------------------------------------------------------------------------------
ConsoleReporter::ConsoleReporter()
{
	 mConsole = GetStdHandle(STD_OUTPUT_HANDLE);
}

// -------------------------------------------------------------------------------
ConsoleReporter::~ConsoleReporter()
{
	CloseHandle(mConsole);
}

// -------------------------------------------------------------------------------
void	ConsoleReporter::ReportFormatted(SEVERITY severity, const char *function, const char *filename, long line, const char *message, ...)
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
void	ConsoleReporter::Report(SEVERITY severity, const char *function, const char *filename, long line, const char *message)
{
	Utils::Lock lock(mCriticalSection);

	switch (severity)
	{
		case IReporter::eLOG:
			SetConsoleTextAttribute(mConsole, 7 + 0*16);
			printf("LOG:\t");
			break;
		case IReporter::eWARNING:
			SetConsoleTextAttribute(mConsole, 14 + 0*16);
			printf("WAR:\t");
			break;
		case IReporter::eERROR:
			SetConsoleTextAttribute(mConsole,  15 + 12*16);
			printf("ERR:\t");
			break;
		case IReporter::eFATAL:
			SetConsoleTextAttribute(mConsole,  15 + 12*16);
			printf("FAT:\t");
			break;
	}

#ifdef _DEBUG
	//OutputDebugString(buffer);
	//OutputDebugString("\n");
#endif

	std::cout << message; // printf takes %s argument, so we use count instead especially not to have any trasnformations
	printf("\n");
}


