#pragma once

namespace Reporter
{

// -------------------------------------------------------------------------------
class EmptyReporter : public IReporter
{
public:

	EmptyReporter() {};
	~EmptyReporter() {};

public: // IReporter

	void	Report(SEVERITY severity, const char *function, const char *filename, long line, const char *message) {};
	void	ReportFormatted(SEVERITY severity, const char *function, const char *filename, long line, const char *message, ...) {};
	void	Destroy() {delete this;};
};

} // namespace
