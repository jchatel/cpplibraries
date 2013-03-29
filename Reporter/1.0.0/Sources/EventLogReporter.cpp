#include "StdAfx.h"
#include "EventLogReporter.h"


using namespace Reporter;

// -------------------------------------------------------------------------------
EventLogReporter::EventLogReporter(const char *eventcategoryname)
: mCategoryName(eventcategoryname)
{
}

// -------------------------------------------------------------------------------
EventLogReporter::~EventLogReporter()
{
}
 
// -------------------------------------------------------------------------------
void	EventLogReporter::ReportFormatted(SEVERITY severity, const char *function, const char *filename, long line, const char *message, ...)
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
void	EventLogReporter::Report(SEVERITY severity, const char *function, const char *filename, long line, const char *message)
{
	LPCSTR  lpszStrings[1];

	HANDLE  hEventSource = RegisterEventSource(NULL, mCategoryName.c_str());

	WORD eventtype = EVENTLOG_ERROR_TYPE;

	switch (severity)
	{
		case IReporter::eLOG:
			eventtype = EVENTLOG_SUCCESS;
			break;
		case IReporter::eWARNING:
			eventtype = EVENTLOG_INFORMATION_TYPE;
			break;
		case IReporter::eERROR:
			eventtype = EVENTLOG_ERROR_TYPE;
			break;
		case IReporter::eFATAL:
			eventtype = EVENTLOG_ERROR_TYPE;
			break;
	}

#ifdef _DEBUG
	OutputDebugString(message);
	OutputDebugString("\n");
#endif

	lpszStrings[0] = message;

	ReportEvent(hEventSource, // handle of event source
		eventtype,  // event type
		0,                    // event category
		0,                    // event ID
		NULL,                 // current user's SID
		1,                    // strings in lpszStrings
		0,                    // no bytes of raw data
		lpszStrings,          // array of error strings
		NULL);                // no raw data

	if (hEventSource)
		DeregisterEventSource(hEventSource);
}


