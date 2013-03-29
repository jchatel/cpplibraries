#pragma once


namespace Stream
{

// ------------------------------------------------------------------------------------------
class PATHMANAGERPRIVATE
{
public:

	std::vector<std::string>	VectorPath;
	Reporter::IReporter			*IReporter;
	Utils::CriticalSection		mCriticalSection;

public:

	// Fonctions
	PATHMANAGERPRIVATE(Reporter::IReporter *reporter);
	~PATHMANAGERPRIVATE();

	void	AddPath(const char *relatifpath);

	bool	GetFullname(const char *filename, char *outputbuffer, size_t buffersize);
};

// ------------------------------------------------------------------------------------------
class PAKMANAGERPRIVATE
{
public:

	std::vector<IPAKFILE*>	VectorFile;
	Allocator::IAllocator	*mMemory;
	Reporter::IReporter		*mReporter;
	Utils::CriticalSection	mCriticalSection;

public:

	// Fonctions
	PAKMANAGERPRIVATE(Allocator::IAllocator *mem, Reporter::IReporter *report);
	~PAKMANAGERPRIVATE();

	void	CleanAll();

	void	AddPakFile(const char *filename);

	IStreamReader	*GetStreamLoad(PATHMANAGERPRIVATE *pathmanager, const char *resourcename); // TODO: make it threadsafe!
};


// -------------------------------------------------------------------------------
class RESOURCEMANAGERPRIVATE : public IRESOURCEMANAGER
{
private:

	PATHMANAGERPRIVATE		mPathManager;
	PAKMANAGERPRIVATE		mPakManager;
	IRESOURCEMANAGERPARAM	mParam;
	Utils::CriticalSection	mCriticalSection;

public:

	RESOURCEMANAGERPRIVATE(IRESOURCEMANAGERPARAM &param);
	~RESOURCEMANAGERPRIVATE();

	void	Destroy();

	void	AddPakFile(const char *pakfilename);
	void	AddMediaPath(const char *relativepath);

	IStreamReader	*GetResource(const char *resourcename);
};


}; // namespace
