#pragma once

namespace Reporter
{

// -------------------------------------------------------------------------------
class EventLogReporter : public IReporter
{
private:

	std::string mCategoryName;

public:

	EventLogReporter(const char *eventcategoryname);
	~EventLogReporter();

public: // IReporter

	void	Report(SEVERITY severity, const char *function, const char *filename, long line, const char *message);
	void	ReportFormatted(SEVERITY severity, const char *function, const char *filename, long line, const char *message, ...);
	void	Destroy() {delete this;};
};

} // namespace
