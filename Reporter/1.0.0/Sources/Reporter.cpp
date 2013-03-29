#include "stdafx.h"
#include "../Reporter/IReporter.h"
#include "EventLogReporter.h"
#include "ConsoleReporter.h"
#include "FileReporter.h"
#include "EmptyReporter.h"

namespace Reporter {
namespace Helper {

IReporter	*__fastcall CreateEventLoggerReporter(const char *categroyname)
{
	return new EventLogReporter(categroyname);
}

IReporter	*__fastcall CreateConsoleReporter()
{
	return new ConsoleReporter();
}

IReporter	*__fastcall CreateFileReporter(const char *filename)
{
	return new FileReporter(filename);
}

IReporter	*__fastcall CreateEmptyReporter()
{
	return new EmptyReporter();
}


} }



