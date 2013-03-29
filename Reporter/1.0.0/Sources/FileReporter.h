#pragma once

namespace Reporter
{

// -------------------------------------------------------------------------------
class FileReporter : public IReporter
{
private:

	std::string					mFilename;
	Utils::CriticalSection		mCriticalSection;
	HANDLE						mFile;
	WORD						mPreviousDay;

public:

	FileReporter(const char *filename);
	~FileReporter();

public: // IReporter

	void	Report(SEVERITY severity, const char *function, const char *filename, long line, const char *message);
	void	ReportFormatted(SEVERITY severity, const char *function, const char *filename, long line, const char *message, ...);
	void	Destroy() {delete this;};
};

} // namespace
