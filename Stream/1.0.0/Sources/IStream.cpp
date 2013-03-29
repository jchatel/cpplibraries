#include "stdafx.h"
#include "Stream.h"
#include "StreamMemory.h"
#include "ResourceManager.h"

namespace Stream {
namespace Helper {

		// -------------------------------------------------------------------------------
		IStreamReader	*__fastcall CreateStreamReaderFromMemory(Allocator::IAllocator *allocator, const char *addr, int size)
		{
			return MEM_ALLOC(allocator, StreamReadMemory, "StreamReadMemory")(addr, size);
		}

		// -------------------------------------------------------------------------------
		IStreamReader	*__fastcall CreateStreamReaderFromHardDrive(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, const char *filename)
		{
			// check file exists first
			FILE *file = fopen(filename, "rb");
			if (file == NULL)
				return NULL;
			fclose(file);

			if (allocator == NULL)
			{
				return NULL;
			}

			return MEM_ALLOC(allocator, STREAMREADHD, "STREAMREADHD")(allocator, reporter, filename);
		}

		// -------------------------------------------------------------------------------
		IStreamWriter	*__fastcall CreateStreamWriterFromHardDrive(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, const char *filename)
		{
			if (allocator == NULL)
			{
				return NULL;
			}

			return MEM_ALLOC(allocator, STREAMWRITEHD, "STREAMWRITEHD")(allocator, reporter, filename);
		}
} }


using namespace Stream;

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
IStreamWriter::IStreamWriter()
{
}


// -------------------------------------------------------------------------------
IPAKFILEPARAM::IPAKFILEPARAM()
: IAllocator(NULL), IReporter(NULL)
{

}

// ------------------------------------------------------------------------------------------
IPAKFILE	*IPAKFILE::Create(IPAKFILEPARAM &param, const char *filename, ...)
{
	if (param.IAllocator == NULL)
	{
		REPORT_ERROR(param.IReporter, true, "[Stream] You must provide a memory allocator interface");
		return NULL;
	}

	if (filename == NULL)
	{
		REPORT_ERROR(param.IReporter, true, "[Stream] You can't have a NULL name for IPAKFILE::Create");
		return NULL;
	}

	char buffer[MAX_PATH];
	va_list args;
	va_start(args, filename);
	if (_vsnprintf(buffer, MAX_PATH, filename, args) == -1)
	{
		REPORT_ERROR(param.IReporter, true, "[Stream] Internal Buffer Exceeded at creation");
	}
	va_end(args);

	// Create the IPAKFILE object
	void *memory = param.IAllocator->Allocate(sizeof(PAKFILEPRIVATE), buffer, __FILE__, __LINE__);
	if (memory == NULL)
		return NULL;

	PAKFILEPRIVATE	*fileprivate = new (memory) PAKFILEPRIVATE(param, buffer);

	if (fileprivate->Init() == false)
	{
		// crap, didn't work, delete the object
		fileprivate->Destroy();
		return NULL; // let the init function complain about problems
	}

	return fileprivate;
};


// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
IRESOURCEMANAGERPARAM::IRESOURCEMANAGERPARAM()
: mIAllocator(NULL), mIReporter(NULL)
{
}

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
IRESOURCEMANAGER	*IRESOURCEMANAGER::Create(IRESOURCEMANAGERPARAM &param)
{
	if (param.mIAllocator == NULL)
	{
		REPORT_ERROR(param.mIReporter, true, "[IRESOURCEMANAGER] You must provide a memory allocator interface");
		return NULL;
	}

	void *memory = param.mIAllocator->Allocate(sizeof(RESOURCEMANAGERPRIVATE), "IRESOURCEMANAGERPARAM", __FILE__, __LINE__);
	if (memory == NULL)
		return NULL;

	RESOURCEMANAGERPRIVATE	*resourcemanager = new (memory) RESOURCEMANAGERPRIVATE(param);

	return resourcemanager;
}


