#pragma once

namespace Reporter
{

// -------------------------------------------------------------------------------
class ConsoleReporter : public IReporter
{
private:

	Utils::CriticalSection		mCriticalSection;
	HANDLE mConsole;

public:

	ConsoleReporter();
	~ConsoleReporter();

public: // IReporter

	void	Report(SEVERITY severity, const char *function, const char *filename, long line, const char *message);
	void	ReportFormatted(SEVERITY severity, const char *function, const char *filename, long line, const char *message, ...);
	void	Destroy() {delete this;};
};

} // namespace
