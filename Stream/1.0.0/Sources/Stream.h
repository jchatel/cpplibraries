#pragma once


namespace Stream
{

// -------------------------------------------------------------------------------
class STREAMREADHD: public IStreamReader
{
protected:

	std::string		mFilename;
	Allocator::IAllocator	*mAllocator;
	Reporter::IReporter		*mReporter;
	FILE			*mFile;

public:

	STREAMREADHD(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, const char *filename);
	~STREAMREADHD();

	unsigned int	GetSize();
	unsigned int	GetCurrentPosition();
	bool			Seek(unsigned int offset);
	unsigned int	Read(void *destination, unsigned int size);
	unsigned int	ReadLine(void *destination, unsigned int maxsize);
	const char		*GetName();

	void	Destroy();
};

// -------------------------------------------------------------------------------
class STREAMWRITEHD: public IStreamWriter
{
protected:

	Allocator::IAllocator			*mAllocator;
	Reporter::IReporter			*mReporter;
	FILE			*mFile;

public:

	STREAMWRITEHD(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, const char *filename);
	~STREAMWRITEHD();

	unsigned int	Write(const void *source, unsigned int size);

	void	Destroy();
};

class PAKFILEPRIVATE;
// -------------------------------------------------------------------------------
struct CHUNK
{
	PAKFILEPRIVATE	*mParent;
	unsigned int	mHashId;
	char			mName[MAX_PATH];
	unsigned int	mVersion;
	unsigned int	mSize;
	unsigned int	mOffset;
};

// -------------------------------------------------------------------------------
class STREAMREADPAK: public IStreamReader
{
private:

	unsigned int	mRead;
	CHUNK			*mChunk;

public:
	STREAMREADPAK(CHUNK *chunk);

	unsigned int	GetSize();
	unsigned int	GetCurrentPosition();
	bool			Seek(unsigned int offset);
	unsigned int	Read(void *destination, unsigned int size);
	unsigned int	ReadLine(void *destination, unsigned int maxsize);
	const char		*GetName() {return mChunk->mName;};

	void	Destroy();
};


// -------------------------------------------------------------------------------
class STREAMWRITEPAK : public IStreamWriter
{
private:
	unsigned int	mWrote;
	CHUNK			*mChunk;

public:
	STREAMWRITEPAK(CHUNK *chunk);

	unsigned int	Write(const void *source, unsigned int size);

	void	Destroy();
};



// -------------------------------------------------------------------------------
struct PAKHEADER
{
	char			mHeader[4];
	unsigned int	mVersion;
	unsigned int	mNumberOfResources;
	unsigned int	mDictionaryOffset;
};

// -------------------------------------------------------------------------------
class PAKFILEPRIVATE : public IPAKFILE
{
public:
	Allocator::IAllocator		*IAllocator;
	Reporter::IReporter		*IReporter;

	enum	{PAKVERSION = 1,};

	std::string		mFilename;
	HANDLE			mFile;

	PAKHEADER		mHeader;

	std::vector<CHUNK>	mVectorChunks;

	static unsigned int	GetHashFromString(const char *string);

	int			GetResourceIndex(const char *resourcename);
	int			GetResourceIndex(unsigned int hashid);

	enum eWRITESTATE
	{
		eDONE,
		eINPROGRESS,
	};

	bool		SetWritting(eWRITESTATE state);

	bool		WriteCleanHeader();
	bool		WriteUpdate();
	bool		WriteResource(const char *resourcename, void *data, unsigned int size, CHUNK *chunk);
	bool		ReadHeader();
	bool		ReadDictionary();
	CHUNK		*GetChunk(const char *resourcename, unsigned int size); // a 0 size means we don't know the size

public:
	
	PAKFILEPRIVATE(IPAKFILEPARAM &param, const char *filename);
	~PAKFILEPRIVATE();

	bool			Init();

	void			Destroy();

	IStreamReader	*GetResource(const char *resourcename);

	bool			AddResource(const char *resourcename, void *data, unsigned int size);

	IStreamWriter	*GetAddResourceStream(const char *resourcename);

	void			EnumerateResources(IPAKENUMERTION *enumerate);
};


}; // namespace
