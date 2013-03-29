#include "stdafx.h"
#include "OSAllocator.h"


using namespace Allocator;
using namespace Reporter;


// -------------------------------------------------------------------------------
OSALLOCATOR::OSALLOCATOR()
: mAllocationCount(0)
{
}

// -------------------------------------------------------------------------------
OSALLOCATOR::~OSALLOCATOR()
{
#ifdef _DEBUG
	if (mAllocationCount != 0)
	{
//		DebugBreak(); // leak!
		char buffer[256];
		OutputDebugString("\n\n----------------------------\n\n");
		_snprintf_s(buffer, 256, "----------- %d LEAK -----------", mAllocationCount);
		OutputDebugString("\n\n----------------------------\n\n");
	}
#endif
}

// -------------------------------------------------------------------------------
void	*OSALLOCATOR::Allocate(unsigned int size, const char *context, const char *filename, long line)
{
	void *addr = malloc(size);

#ifdef _DEBUG
	InterlockedIncrement(&mAllocationCount);

//	char buffer[1024];
//	sprintf_s(buffer, "%s(%d) : [OSAllocator] ALLOC of %d bytes for %s @ 0x%x to 0x%x\n", filename, line, size, context, addr, (char *)addr+size);
//	OutputDebugString(buffer);

#endif

	return addr;
}

// -------------------------------------------------------------------------------
void	OSALLOCATOR::Free(void *memory)
{
#ifdef _DEBUG
	InterlockedDecrement(&mAllocationCount);
#endif
	free(memory);
}





