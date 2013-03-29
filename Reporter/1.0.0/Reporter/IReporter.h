#pragma once


namespace Reporter {

// -------------------------------------------------------------------------------
class IReporter
{
public:

	enum SEVERITY
	{
		eLOG,
		eWARNING,
		eERROR,
		eFATAL,
	};

protected:

	IReporter() {}; // use Create from Helper layer
	virtual ~IReporter() {}; // use destroy

public:

	virtual void	Report(SEVERITY severity, const char *function, const char *filename, long line, const char *message) = 0;
	virtual void	ReportFormatted(SEVERITY severity, const char *function, const char *filename, long line, const char *message, ...) = 0;
	virtual	void	Destroy() = 0;
};

// -------------------------------------------------------------------------------
namespace Helper
{
	IReporter	*__fastcall CreateEventLoggerReporter(const char *categroyname);
	IReporter	*__fastcall CreateConsoleReporter();
	IReporter	*__fastcall CreateFileReporter(const char *filename);
	IReporter	*__fastcall CreateEmptyReporter();
}

// -------------------------------------------------------------------------------
#define REPORT_LOG(reporter, condition, message, ...)		\
if ((condition) && reporter)								\
{															\
	reporter->ReportFormatted(Reporter::IReporter::eLOG, __FUNCTION__, __FILE__, __LINE__, message, __VA_ARGS__);		\
}

// -------------------------------------------------------------------------------
#define REPORT_WARNING(reporter, condition, message, ...)	\
if ((condition) && reporter)							\
{															\
	reporter->ReportFormatted(Reporter::IReporter::eWARNING, __FUNCTION__, __FILE__, __LINE__, message, __VA_ARGS__);	\
}

// -------------------------------------------------------------------------------
#define REPORT_ERROR(reporter, condition, message, ...)		\
if ((condition) && reporter)							\
{															\
	reporter->ReportFormatted(Reporter::IReporter::eERROR, __FUNCTION__, __FILE__, __LINE__, message, __VA_ARGS__);	\
}

// -------------------------------------------------------------------------------
#define REPORT_FATAL(reporter, condition, message, ...)		\
	if ((condition) && reporter)							\
{															\
	reporter->ReportFormatted(Reporter::IReporter::eFATAL, __FUNCTION__, __FILE__, __LINE__, message, __VA_ARGS__);	\
}

} // namespace
