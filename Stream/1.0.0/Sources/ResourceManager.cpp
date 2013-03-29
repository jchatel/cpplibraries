#include "stdafx.h"
#include "ResourceManager.h"

using namespace Stream;


// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
bool	FileExistsOnHardDrive(const char *filename)
{
	WIN32_FIND_DATA findfiledata;
	HANDLE hFind;

	hFind = FindFirstFile(filename, &findfiledata);
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		return false;
	}

	FindClose(hFind);
	return true;
}

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
PATHMANAGERPRIVATE::PATHMANAGERPRIVATE(Reporter::IReporter *reporter)
: IReporter(reporter)
{
}

// ------------------------------------------------------------------------------------------
PATHMANAGERPRIVATE::~PATHMANAGERPRIVATE()
{
	VectorPath.clear();
}


// ---------------------------------------------------------------------------------------------
void	PATHMANAGERPRIVATE::AddPath(const char *relatifpath)
{
	Utils::Lock a(mCriticalSection);

	if (relatifpath == NULL)
		return;

	long length = (long)strlen(relatifpath);

	if (length == 0)
		return;
	
	if (length >= MAX_PATH)
	{
		REPORT_ERROR(IReporter, true, "[GI::IRESOURCEMANAGER] Media path too long");
		return;
	}

	char buffer[MAX_PATH];

	char lastchar = relatifpath[length - 1];
	_snprintf_s(buffer, MAX_PATH, "%s", relatifpath);

	if (lastchar == '//' || lastchar == '/')
	{
		// cut the / at the end
		buffer[length - 1] = '\0';
	}

	VectorPath.push_back(buffer);
}

// ---------------------------------------------------------------------------------------------
bool	PATHMANAGERPRIVATE::GetFullname(const char *filename, char *outputbuffer, size_t buffersize)
{
	Utils::Lock a(mCriticalSection);

	// check open one after the other

//	FILE *exist = fopen(filename, "r");
	if (FileExistsOnHardDrive(filename))
	{
//		fclose(exist);

		strncpy(outputbuffer, filename, buffersize);
		return true; // already pointing to data, no need to check media paths
	}

	for (unsigned long x = 0; x < VectorPath.size(); x++)
	{
		_snprintf(outputbuffer, buffersize, "%s/%s", VectorPath[x].c_str(), filename);
//		exist = fopen(outputbuffer, "r");
		if (FileExistsOnHardDrive(outputbuffer))
		{
//			fclose(exist);
			return true; // already pointing to data, no need to check media paths
		}
	}

	outputbuffer[0] = '\0'; // clear the buffer

//	REPORT_ERRORF(IReporter, true, ("[GI::IRESOURCEMANAGER] Can't find \"%s\" in any of the media path specified", filename));

	return false; // can't find it...
}

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
PAKMANAGERPRIVATE::PAKMANAGERPRIVATE(Allocator::IAllocator *mem, Reporter::IReporter *report)
: mMemory(mem), mReporter(report)
{
}

// ------------------------------------------------------------------------------------------
PAKMANAGERPRIVATE::~PAKMANAGERPRIVATE()
{
	CleanAll();
}

// -------------------------------------------------------------------------------
void	PAKMANAGERPRIVATE::CleanAll()
{
	Utils::Lock a(mCriticalSection);

	for (unsigned int x = 0; x < VectorFile.size(); ++x)
	{
		VectorFile[x]->Destroy();
	}

	VectorFile.clear();
}

// ---------------------------------------------------------------------------------------------
void	PAKMANAGERPRIVATE::AddPakFile(const char *filename)
{
	Utils::Lock a(mCriticalSection);

	if (filename == NULL)
		return;

	IPAKFILEPARAM param;
	param.IAllocator = mMemory;
	param.IReporter = mReporter;

	IPAKFILE *filepak = IPAKFILE::Create(param, filename);

	if (filepak == NULL)
	{
		return;
	}

	VectorFile.push_back(filepak);
}

// ---------------------------------------------------------------------------------------------
IStreamReader		*PAKMANAGERPRIVATE::GetStreamLoad(PATHMANAGERPRIVATE *pathmanager, const char *resourcename)
{
	Utils::Lock a(mCriticalSection);

	IStreamReader *stream = NULL;

	size_t maxfiles = VectorFile.size();
	for (size_t x = 0; x < maxfiles; ++x)
	{
		stream = VectorFile[x]->GetResource(resourcename);
		if (stream)
		{
			return stream;
		}
	}

	if (pathmanager)
	{
		char buffer[MAX_PATH];
		// ok ok, not in pak as IS, need to iterate with media paths too
		size_t max = pathmanager->VectorPath.size();
		for (size_t x = 0; x < max; ++x)
		{
			_snprintf(buffer, MAX_PATH, "%s/%s", pathmanager->VectorPath[x].c_str(), resourcename);
			_strlwr(buffer);

			for (size_t f = 0; f < maxfiles; ++f)
			{
				stream = VectorFile[f]->GetResource(buffer);
				if (stream)
				{
					return stream;
				}
			}
		}
	}

	return NULL;
}


// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
RESOURCEMANAGERPRIVATE::RESOURCEMANAGERPRIVATE(IRESOURCEMANAGERPARAM &param)
: mPathManager(param.mIReporter), mPakManager(param.mIAllocator, param.mIReporter), mParam(param)
{
}

// -------------------------------------------------------------------------------
RESOURCEMANAGERPRIVATE::~RESOURCEMANAGERPRIVATE()
{
	mPakManager.CleanAll();
}

// -------------------------------------------------------------------------------
void	RESOURCEMANAGERPRIVATE::Destroy()
{
	Allocator::IAllocator *alloc = mParam.mIAllocator;

	this->~RESOURCEMANAGERPRIVATE();
	alloc->Free(this);
}

// -------------------------------------------------------------------------------
void	RESOURCEMANAGERPRIVATE::AddPakFile(const char *pakfilename)
{
	Utils::Lock a(mCriticalSection);

	mPakManager.AddPakFile(pakfilename);
}

// -------------------------------------------------------------------------------
void	RESOURCEMANAGERPRIVATE::AddMediaPath(const char *relativepath)
{
	Utils::Lock a(mCriticalSection);

	mPathManager.AddPath(relativepath);
}

// -------------------------------------------------------------------------------
IStreamReader	*RESOURCEMANAGERPRIVATE::GetResource(const char *resourcename)
{
	// Policy is check on HD first, then in pak (allow you to overload pak with files)
	Utils::Lock a(mCriticalSection);

	IStreamReader *stream = NULL;

	char bufferfullname[MAX_PATH];

	if (mPathManager.GetFullname(resourcename, bufferfullname, MAX_PATH))
	{
		stream = Helper::CreateStreamReaderFromHardDrive(mParam.mIAllocator, mParam.mIReporter, bufferfullname);
	}

	if (stream == NULL)
	{
		// try in Pak
//		stream = mPakManager.GetStreamLoad(resourcename);
		// try in pak with various media paths
		stream = mPakManager.GetStreamLoad(&mPathManager, resourcename);
	}

	return stream;
}

