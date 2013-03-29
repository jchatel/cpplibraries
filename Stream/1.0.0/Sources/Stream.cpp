#include "stdafx.h"
#include "Stream.h"

using namespace Stream;

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
STREAMREADHD::STREAMREADHD(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, const char *filename)
: mAllocator(allocator), mReporter(reporter), mFilename(filename)
{
	mFile = fopen(filename, "rb");
}

// -------------------------------------------------------------------------------
STREAMREADHD::~STREAMREADHD()
{
	if (mFile)
	{
		fclose(mFile);
		mFile = NULL;
	}
}

// -------------------------------------------------------------------------------
unsigned int	STREAMREADHD::GetSize()
{
	unsigned int oldpos = GetCurrentPosition();

	fseek(mFile, 0, SEEK_END);

	unsigned int endpos = GetCurrentPosition();

	Seek(oldpos);

	return endpos;
};

// -------------------------------------------------------------------------------
unsigned int	STREAMREADHD::GetCurrentPosition()
{
	if (mFile == NULL)
		return 0;

	return ftell(mFile);
}

// -------------------------------------------------------------------------------
bool	STREAMREADHD::Seek(unsigned int offset)
{
	if (mFile == NULL)
		return false;

	return fseek(mFile, offset, SEEK_SET) == 0; // success
}


// -------------------------------------------------------------------------------
unsigned int	STREAMREADHD::Read(void *destination, unsigned int size)
{
	if (mFile == NULL)
		return 0;

	return (unsigned int)fread(destination, 1, size, mFile);
};

// -------------------------------------------------------------------------------
unsigned int	STREAMREADHD::ReadLine(void *destination, unsigned int maxsize)
{
	if (mFile == NULL)
		return 0;

	long startposition = ftell(mFile);

	fgets((char *)destination, maxsize, mFile);

	long endposition = ftell(mFile);

	return endposition - startposition;
};

// -------------------------------------------------------------------------------
const char		*STREAMREADHD::GetName()
{
	return mFilename.c_str();
};

// -------------------------------------------------------------------------------
void	STREAMREADHD::Destroy()
{
	this->~STREAMREADHD();
	mAllocator->Free(this);
}

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
STREAMWRITEHD::STREAMWRITEHD(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, const char *filename)
: mAllocator(allocator), mReporter(reporter)
{
	mFile = fopen(filename, "wb");
}

// -------------------------------------------------------------------------------
STREAMWRITEHD::~STREAMWRITEHD()
{
	if (mFile)
	{
		fclose(mFile);
		mFile = NULL;
	}
}

// -------------------------------------------------------------------------------
void	STREAMWRITEHD::Destroy()
{
	this->~STREAMWRITEHD();
	mAllocator->Free(this);
}

// -------------------------------------------------------------------------------
unsigned int	STREAMWRITEHD::Write(const void *source, unsigned int size)
{
	if (mFile == NULL)
		return 0;

	return (unsigned int)fwrite(source, 1, size, mFile);
};




// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
STREAMREADPAK::STREAMREADPAK(CHUNK *chunk)
	: mChunk(chunk), mRead(0)
{
};

// -------------------------------------------------------------------------------
void	STREAMREADPAK::Destroy()
{
	this->~STREAMREADPAK();
	mChunk->mParent->IAllocator->Free(this);
};

// -------------------------------------------------------------------------------
unsigned int	STREAMREADPAK::GetSize()
{
	return mChunk->mSize;
}

// -------------------------------------------------------------------------------
unsigned int	STREAMREADPAK::GetCurrentPosition()
{
	return mRead; // mChunk->mOffset;
}

// -------------------------------------------------------------------------------
bool	STREAMREADPAK::Seek(unsigned int offset)
{
	if (offset > mChunk->mSize)
	{
		mRead = mChunk->mSize;
		return false;
	}

	mRead = offset;
	return true;
}

// -------------------------------------------------------------------------------
unsigned int	STREAMREADPAK::Read(void *destination, unsigned int size)
{
	DWORD byteread = 0;
	OVERLAPPED seek;
	memset(&seek, 0, sizeof(OVERLAPPED));

	seek.Offset = mChunk->mOffset + mRead;

	if (mRead + size > mChunk->mSize)
	{
		size = mChunk->mSize - mRead;
		if (size == 0)
		{
			// accepted behaviour to read beyond the size, but clamp it so we return 0
//			GIREPORT_ERROR(mChunk->mParent->IReporter, true , "[GI::FILE] Trying to read beyong (%u) STREAM size (%u) in PAK: %s", mRead + size, mChunk->mSize, mChunk->mParent->mFilename.c_str());
			return 0;
		}
	}

	BOOL ret = ReadFile(mChunk->mParent->mFile, destination, size, &byteread, &seek);

	if (ret == FALSE)
	{
		REPORT_ERROR(mChunk->mParent->IReporter, true, "[GI::FILE] Problem (%u) reading stream PAK: %s", GetLastError(), mChunk->mParent->mFilename.c_str());
		return 0;
	}

	mRead += byteread;

	return byteread;
};

// -------------------------------------------------------------------------------
unsigned int	STREAMREADPAK::ReadLine(void *destination, unsigned int maxsize)
{
	unsigned int size = 0;
	unsigned int read;
	bool firstendofline = false;
	unsigned char *p = (unsigned char *)destination;

	while (read = Read(p, 1) && size < maxsize)
	{
		++size;

		if (*p == '\n'
			|| *p == '\r')
		{
			if (firstendofline)
				break;

			((unsigned char *)destination)[size - 1] = '\0';
			firstendofline = true;
		}

		++p;
	}

	return size;
};


// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
STREAMWRITEPAK::STREAMWRITEPAK(CHUNK *chunk)
: mChunk(chunk), mWrote(0)
{
//	REPORT_ERROR(mChunk->mParent->IReporter, mChunk->mParent->
}


// -------------------------------------------------------------------------------
unsigned int	STREAMWRITEPAK::Write(const void *source, unsigned int size)
{
	// TODO: check size on Hard Drive to see if we can add resources somehow  GetDiskFreeSpace 

	DWORD written = 0;
	DWORD byteread = 0;
	OVERLAPPED seek;
	memset(&seek, 0, sizeof(OVERLAPPED));

	seek.Offset = mChunk->mOffset + mWrote;

	BOOL ret = WriteFile(mChunk->mParent->mFile, source, size, &written, &seek);

	REPORT_ERROR(mChunk->mParent->IReporter, ret == FALSE, "[GI::FILE] Problem (%u) writing stream PAK: %s", GetLastError(), mChunk->mParent->mFilename.c_str());

	mWrote += written;
	mChunk->mParent->mHeader.mDictionaryOffset += written;
	mChunk->mSize += written;

	return written;
}


// -------------------------------------------------------------------------------
void	STREAMWRITEPAK::Destroy()
{
	mChunk->mParent->WriteUpdate();

	this->~STREAMWRITEPAK();
	mChunk->mParent->IAllocator->Free(this);
};



	
	
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
PAKFILEPRIVATE::PAKFILEPRIVATE(IPAKFILEPARAM &param, const char *filename)
  : IAllocator(param.IAllocator), IReporter(param.IReporter), mFile(NULL), mFilename(filename)
{
};

// -------------------------------------------------------------------------------
bool	PAKFILEPRIVATE::Init()
{
	mFile = CreateFile(mFilename.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (mFile == INVALID_HANDLE_VALUE)
	{
		// file did not exist, creates it

		mFile = CreateFile(mFilename.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (mFile == INVALID_HANDLE_VALUE)
		{
			REPORT_ERROR(IReporter, true, "[GI::FILE] Could not open PAK file: %s", mFilename.c_str());
			mFile = NULL;
			return false;
		}
		
		if (WriteCleanHeader() == false)
			return false;
	}

	// File opened and ready
	if (ReadHeader() == false)
		return false;

	return true;
}

// -------------------------------------------------------------------------------
PAKFILEPRIVATE::~PAKFILEPRIVATE()
{
	if (mFile)
	{
		CloseHandle(mFile);
		mFile = NULL;
	}
}

// -------------------------------------------------------------------------------
bool	PAKFILEPRIVATE::WriteCleanHeader()
{
	DWORD written;

	PAKHEADER header;
	memcpy(header.mHeader, "PAK0", 4);
	header.mVersion = PAKVERSION;
	header.mNumberOfResources = 0;
	header.mDictionaryOffset = sizeof(PAKHEADER); // no data to skip since there are no resources

	BOOL ret = WriteFile(mFile, &header, sizeof(PAKHEADER), &written, NULL);

	REPORT_ERROR(IReporter, ret == false, "[GI::FILE] Error writting (%u) PAK clean header: %s", GetLastError(), mFilename.c_str());

	return ret == TRUE;
}

// -------------------------------------------------------------------------------
bool	PAKFILEPRIVATE::WriteUpdate()
{
	DWORD written;
	BOOL ret;
	OVERLAPPED seek;

	// Write dictionary first

	for (unsigned int x = 0; x < mHeader.mNumberOfResources; ++x)
	{
		memset(&seek, 0, sizeof(OVERLAPPED));
		seek.Offset = mHeader.mDictionaryOffset + x * sizeof(CHUNK);

		ret = WriteFile(mFile, &mVectorChunks[x], sizeof(CHUNK), &written, &seek);

		if (ret == TRUE)
			continue;

		REPORT_ERROR(IReporter, true, "[GI::FILE] Error writing dictionnay (%u) PAK: %s", GetLastError(), mFilename.c_str());
		return false;
	}


	// Header

	memcpy(mHeader.mHeader, "PAK0", 4);

	seek.Offset = 0;

	ret = WriteFile(mFile, &mHeader, sizeof(PAKHEADER), &written, &seek);

	if (ret == FALSE)
	{
		REPORT_ERROR(IReporter, true, "[GI::FILE] Error writing updated header (%u) PAK: %s", GetLastError(), mFilename.c_str());
		return false;
	}

	return true;
}

// -------------------------------------------------------------------------------
bool	PAKFILEPRIVATE::SetWritting(eWRITESTATE state)
{
	DWORD written;

	OVERLAPPED seek;
	memset(&seek, 0, sizeof(OVERLAPPED));
	seek.Offset = 0;

	switch (state)
	{
		case eDONE:			mHeader.mHeader[3] = '0'; break; // writting

		case eINPROGRESS:	mHeader.mHeader[3] = '1'; break; // writting

		default:
			mHeader.mHeader[3] = '2'; // unknown state
	}

	BOOL ret = WriteFile(mFile, &mHeader.mHeader, 4, &written, &seek);

	REPORT_ERROR(IReporter, ret == false, "[GI::FILE] Error writting (%u) PAK state: %s", GetLastError(), mFilename.c_str());

	return ret == TRUE;
}

// -------------------------------------------------------------------------------
bool	PAKFILEPRIVATE::ReadHeader()
{
	// first, read the header and make sure all is correct

	// Read header first
	DWORD byteread = 0;
	OVERLAPPED seek;
	memset(&seek, 0, sizeof(OVERLAPPED));
	seek.Offset = 0;
	BOOL ret = ReadFile(mFile, &mHeader, sizeof(PAKHEADER), &byteread, &seek);

	if (ret == FALSE)
	{
		REPORT_ERROR(IReporter, true, "[GI::FILE] Could not read (%u) PAK header: %s", GetLastError(), mFilename.c_str());
		return false;
	}

	// check header integrity
	if (byteread < sizeof(PAKHEADER))
	{
		REPORT_ERROR(IReporter, true, "[GI::FILE] PAK not conventionnal: %s", mFilename.c_str());
		return false;
	}

	if (memcmp(mHeader.mHeader, "PAK", 3) != 0)
	{
		REPORT_ERROR(IReporter, true, "[GI::FILE] PAK header corrupted: %s", mFilename.c_str());
		return false;
	}

	if (mHeader.mHeader[3] != '0')
	{
		REPORT_ERROR(IReporter, true, "[GI::FILE] PAK corrupted (wasn't closed properly while adding resources, please rebuild): %s", mFilename.c_str());
		return false;
	}
	
	if (mHeader.mVersion > PAKVERSION)
	{
		REPORT_ERROR(IReporter, true, "[GI::FILE] PAK format not forward compatible, this PAK version is newer than the source code to read it: %s", mFilename.c_str());
		return false;
	}

	// Header ok, Read dictionnay

	return ReadDictionary();
}

// -------------------------------------------------------------------------------
bool	PAKFILEPRIVATE::ReadDictionary()
{
	mVectorChunks.clear();

	if (mHeader.mNumberOfResources == 0)
	{
		REPORT_WARNING(IReporter, mHeader.mDictionaryOffset != sizeof(PAKHEADER), "[GI::FILE] PAK offset size suspicious, probably a difference in version between code and PAK file: %s", mFilename.c_str());
		return true; // empty dictionary
	}

	// Read chunks
	CHUNK newchunk;

	DWORD byteread = 0;								    
	OVERLAPPED seek;
	BOOL ret;

	for (unsigned int x = 0; x < mHeader.mNumberOfResources; ++x)
	{
		memset(&seek, 0, sizeof(OVERLAPPED));
		seek.Offset = mHeader.mDictionaryOffset + x * sizeof(CHUNK);
		ret = ReadFile(mFile, &newchunk, sizeof(CHUNK), &byteread, &seek);

		if (ret == FALSE)
		{
			REPORT_ERROR(IReporter, true, "[GI::FILE] Error (%u) reading PAK: %s", GetLastError(), mFilename.c_str());
			return false;
		}

		if (byteread != sizeof(CHUNK))
		{
			REPORT_ERROR(IReporter, true, "[GI::FILE] Error (%u) reading PAK, incorrect chunk size: %s", GetLastError(), mFilename.c_str());
			return false;
		}

		newchunk.mParent = this;
		mVectorChunks.push_back(newchunk);
	}

	return true;
}

// -------------------------------------------------------------------------------
unsigned int	PAKFILEPRIVATE::GetHashFromString(const char *string)
{
	unsigned int hash = 2166136261;

	char *pchar = (char *)string;
	char lower;
	while (*pchar != NULL)
	{
		// lowercase only
		lower = *pchar;
		if (lower >= 'A' && lower <= 'Z')
			lower = lower - 'A' + 'a';

		hash = hash ^ lower;
		hash *= 16777619;
		pchar++;
	}

	return hash;
}


// -------------------------------------------------------------------------------
int		PAKFILEPRIVATE::GetResourceIndex(unsigned int hashid)
{
	for (unsigned int x = 0; x < mHeader.mNumberOfResources; ++x)
	{
		if (mVectorChunks[x].mHashId == hashid)
		{
			return x;
		}
	}

	return -1;
}

// -------------------------------------------------------------------------------
int		PAKFILEPRIVATE::GetResourceIndex(const char *resourcename)
{
	unsigned int hashid = GetHashFromString(resourcename);
	return GetResourceIndex(hashid);
}

// -------------------------------------------------------------------------------
IStreamReader		*PAKFILEPRIVATE::GetResource(const char *resourcename)
{
	int index = GetResourceIndex(resourcename);

	if (index < 0)
	{
		// could not be found in the resource pak
		return NULL;
	}

	mVectorChunks[index].mParent = this; // update parent pointer
	STREAMREADPAK *stream = MEM_ALLOC(IAllocator, STREAMREADPAK, resourcename)(&mVectorChunks[index]);
	return stream;
}

// -------------------------------------------------------------------------------
void	PAKFILEPRIVATE::EnumerateResources(IPAKENUMERTION *enumerate)
{
	size_t max = mVectorChunks.size();

	RESOURCEINFO info;

	for (size_t x = 0; x < max; ++x)
	{
		CHUNK *chunk = &mVectorChunks[x];
		
		info.mVersion = chunk->mVersion;
		info.mSize = chunk->mSize;
		info.mName = chunk->mName;
		info.mOffset = chunk->mOffset;
		
		enumerate->EnumerateResource(info);
	}
}


// -------------------------------------------------------------------------------
bool	PAKFILEPRIVATE::WriteResource(const char *resourcename, void *data, unsigned int size, CHUNK *chunk)
{
	// write the data in the file
	DWORD written;

	OVERLAPPED seek;
	memset(&seek, 0, sizeof(OVERLAPPED));
	seek.Offset = chunk->mOffset;

	BOOL ret = WriteFile(mFile, data, size, &written, &seek);

	if (ret == false)
	{
		REPORT_ERROR(IReporter, true, "[GI::FILE] Error (%u) writting resource (%s) in PAK: %s", GetLastError(), resourcename, mFilename.c_str());
		return false;
	}

	if (size != written)
	{
		REPORT_ERROR(IReporter, true, "[GI::FILE] Resource (%s) could not be completely written in PAK: %s", resourcename, mFilename.c_str());
		return false;
	}

	return true;
}


// -------------------------------------------------------------------------------
CHUNK	*PAKFILEPRIVATE::GetChunk(const char *resourcename, unsigned int size)
{
	unsigned int hashid = GetHashFromString(resourcename);

	// Create new chunk & fill information
	CHUNK newchunk;
	// fefault new chunk
	strncpy(newchunk.mName, resourcename, MAX_PATH);
	newchunk.mHashId = hashid;
	newchunk.mVersion = 1;

	CHUNK *chunk = &newchunk;

	// check if already in this pakfile to increase version or splatter
	int index = GetResourceIndex(hashid);

	if (index < 0) 
	{ // not found
		chunk->mOffset = mHeader.mDictionaryOffset; // add at the end (on top of dictionary

		// Increase current offset
		mHeader.mDictionaryOffset += size;
		mHeader.mNumberOfResources++; // only if we add instead of replace

		// Add pak to our dictionary a copy of chunk
		mVectorChunks.push_back(*chunk);

		// point to last one in vector (sticky in memory, we don't return a variable on the stack, that would be bad)
		chunk = &mVectorChunks[mVectorChunks.size() - 1];
	}
	else
	{
		chunk = &mVectorChunks[index];

		chunk->mVersion++; // inc version

		if (size == 0 || chunk->mSize < size) // if size is 0 that will force creating another chunk of data in the stream
		 {
			 // existing chunk too small for us, require a new Offset
			chunk->mOffset = mHeader.mDictionaryOffset; // update Offset
			mHeader.mDictionaryOffset += size; // increase the resource data size
		 }
		// if new version is smaller, splatter previous one
		// Offset is not updated since we splatter previous information
	}

	chunk->mSize = size; // update with new size regardless of the situation

	return chunk;
}

// -------------------------------------------------------------------------------
bool	PAKFILEPRIVATE::AddResource(const char *resourcename, void *data, unsigned int size)
{
	// only supports current version
	if (mHeader.mVersion != PAKVERSION)
	{
		REPORT_ERROR(IReporter, true, "[GI::FILE] You can't add resources to a PAK that is of an older version, please recreate this PAK from scratch");
		return false;
	}

	// TODO: check size on Hard Drive to see if we can add resources

	CHUNK *chunk = GetChunk(resourcename, size);

	if (SetWritting(eINPROGRESS) == false) // for file integrity in case of crash while updating
		return false;

	if (WriteResource(resourcename, data, size, chunk) == false)
		return false;

	// rewrite the header back
	if (WriteUpdate() == false) // will set automatically the writting state to done
		return false;

	return true;
}

// -------------------------------------------------------------------------------
IStreamWriter	*PAKFILEPRIVATE::GetAddResourceStream(const char *resourcename)
{
	if (SetWritting(eINPROGRESS) == false) // for file integrity in case of crash while updating
		return NULL;

	CHUNK *chunk = GetChunk(resourcename, 0);
	chunk->mSize = 0;
	chunk->mParent = this;

	STREAMWRITEPAK *stream = MEM_ALLOC(IAllocator, STREAMWRITEPAK, "STREAMWRITEPAK")(chunk);

	return stream;	
}


// -------------------------------------------------------------------------------
void	PAKFILEPRIVATE::Destroy()
{
	Allocator::IAllocator *alloc = IAllocator;

	this->~PAKFILEPRIVATE();
	alloc->Free(this);
}


