#include "stdafx.h"
#include "PageAllocator.h"
#include <malloc.h>
#include <stdio.h>


// TODO: 

// Check for invalid pages
// Unallocate extra pages
// Resist to memory corruptions
// Possibility to have no items in the freelist of a page
// Support Realloc

using namespace Reporter;
using namespace Allocator;

//#define DEBUGMEM

#ifdef DEBUGMEM
	unsigned char DEBUGx = 0x77;
#endif

// -------------------------------------------------------------------------------
ALLOCDEF::ALLOCDEF(unsigned int size)
: Size(size)
{
#ifdef DEBUGMEM
	memset(this, DEBUGx, sizeof(ALLOCDEF));
	Size = size;
	#ifdef _DEBUG
		strcpy_s(Filename, "FREEBLOCK");
		strcpy_s(Context, "NO CONTEXT");
	#endif
#endif
}



// -------------------------------------------------------------------------------
ALLOCHEADER::ALLOCHEADER(unsigned int sizeinbytes, ALLOCHEADER *previous, ALLOCHEADER *next)
: Code(MEM_FREEBLOCK), AllocDef(sizeinbytes), Next(next), Prev(previous)
{
	if (previous)
		previous->Next = this;

	if (Next)
		Next->Prev = this;


#ifdef DEBUGMEM
	// debug
	char *pos = (char *)this;
	pos += sizeof(ALLOCHEADER);
	memset(pos, DEBUGx, sizeinbytes - sizeof(ALLOCHEADER));
#endif
}

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
PAGEALLOC::PAGEALLOC(MEMORYPARAM &memparam, PAGEALLOC *previouspage, bool extrapage)
: NbAllocation(0), Memory(NULL), TotalSize(0), FreeList(NULL), Next(NULL), Param(memparam), IsExtraPage(extrapage)
{
	PrimeAllocate();
}

// -------------------------------------------------------------------------------
PAGEALLOC::~PAGEALLOC()
{
	// delete the Next pointer
	delete Next;
	Next = NULL;

	if (Memory)
	{
		free(Memory);
	}
	Memory = NULL;
}


// -------------------------------------------------------------------------------
void	PAGEALLOC::PrimeAllocate()
{
	if (Memory)
	{
		REPORT_ERROR(Param.IReport, true, "[PageAllocator] Memory already allocated, bug in PrimeAllocation");
		return;
	}

	if (IsExtraPage == false)
		TotalSize = Param.BaseMemoryInBytes;
	else
		TotalSize = Param.MemoryPerPage;

	if (TotalSize < 1024)
	{
		REPORT_ERROR(Param.IReport, true, "[PageAllocator] Minimum allocation for base and page memory is 1024 bytes, page size re-adjusted");
		TotalSize = 1024;
	}

	static unsigned int SizeOfAllocHeader2 = sizeof(ALLOCHEADER) * 2;

	TotalSize += SizeOfAllocHeader2;

	Memory = malloc(TotalSize);

	if (Memory == NULL)
	{
		REPORT_ERROR(Param.IReport, true, "[PageAllocator] Could not allocate the memory required, this is a fatal error, you may want to decrease the amount of memory you ask for");
		return;
	}

#ifdef DEBUGMEM
	memset(Memory, DEBUGx, TotalSize);
#endif

	FreeList = new (Memory) ALLOCHEADER(TotalSize, NULL, NULL);
}


// -------------------------------------------------------------------------------
ALLOCHEADER *PAGEALLOC::CanAllocate(unsigned int totalsize) const
{
	ALLOCHEADER *header = FreeList;
	while (header)
	{
		if (header->AllocDef.Size > totalsize)
			return header;

		header = header->Next;
	}

	return NULL;
}


// -------------------------------------------------------------------------------
unsigned int	PAGEALLOC::GetNumberOfAllocation()
{
	if (Next == NULL)
		return NbAllocation;

	return NbAllocation + Next->GetNumberOfAllocation();
}

// -------------------------------------------------------------------------------
unsigned int	PAGEALLOC::GetFreeMemorySize()
{
	unsigned int freesize = 0;

	ALLOCHEADER *header = FreeList;
	while (header)
	{
		freesize += header->AllocDef.Size;
		header = header->Next;
	}

	if (Next == NULL)
		return freesize;

	return freesize + Next->GetFreeMemorySize();
}

// -------------------------------------------------------------------------------
unsigned int	PAGEALLOC::GetAllocatedSize()
{
	unsigned int freesize = 0;

	ALLOCHEADER *header = FreeList;
	while (header)
	{
		freesize += header->AllocDef.Size;
		header = header->Next;
	}

	if (Next == NULL)
		return TotalSize - freesize;

	return (TotalSize - freesize) + Next->GetAllocatedSize();
}

// -------------------------------------------------------------------------------
void	*PAGEALLOC::Allocate(ALLOCDEF &allocdef)
{
	static unsigned int SizeOfAllocHeader = sizeof(ALLOCHEADER);

	ALLOCHEADER *header = CanAllocate(allocdef.Size + SizeOfAllocHeader * 2);

	if (header == NULL)
	{
		if (Next == NULL)
		{
			// No one could allocate enough memory, could be due to fragmentation
			REPORT_WARNING(Param.IReport, true, "[PageAllocator] Another page was allocated, you may want to increase you base page allocation (fragmentation of memory may be the issue)");

			// Allocate another page
			// but first check if new page can support the requested size of not

			if (Param.MemoryPerPage < allocdef.Size)
			{
				REPORT_ERROR(Param.IReport, true, "[PageAllocator] Can't allocate as the size requested is bigger than the memory per page specified");
				return NULL; // can't allocate
			}

			Next = new PAGEALLOC(Param, this, true);

			if (Next == NULL)
				return NULL;

			return Next->Allocate(allocdef);
		}

		return Next->Allocate(allocdef);
	}

	unsigned int totalsize = allocdef.Size + SizeOfAllocHeader;

#ifdef _DEBUG
	strncpy_s(header->AllocDef.Filename, allocdef.Filename, ALLOCDEF::eMAX_PATH);
	strncpy_s(header->AllocDef.Context, allocdef.Context, ALLOCDEF::eMAXCONTEXTSIZE);
	header->AllocDef.Line = allocdef.Line;
	header->AllocDef.mAllocNumber = allocdef.mAllocNumber;
#endif

	// this page can allocate the requested memory, so please do
	char *freemem = (char *)header;
	ALLOCHEADER *newstillfree = new (freemem + totalsize) ALLOCHEADER(header->AllocDef.Size - totalsize, header->Prev, header->Next);

	if (header->Prev == NULL)
		FreeList = newstillfree;

	header->AllocDef.Size = totalsize;
	header->Code = MEM_ALLOCATED;

	if (Param.IMonitor)
	{
		MEMORYINFOBLOCK memblock;

		#ifdef _DEBUG
			strncpy_s(memblock.mFilename, header->AllocDef.Filename, MEMORYINFOBLOCK::eMAX_PATH);
			strncpy_s(memblock.mContext, header->AllocDef.Context, MEMORYINFOBLOCK::eMAXCONTEXTSIZE);
			memblock.mLine = header->AllocDef.Line;
			memblock.mAllocNumber = header->AllocDef.mAllocNumber;
		#endif

		memblock.mPageNumber = -1; // TODO

		char *start = (char *)Memory;
		char *end = (char *)header;
		memblock.mOffset = (unsigned int)(end - start);
		memblock.mSize = header->AllocDef.Size;

		Param.IMonitor->OnAllocate(memblock);
	}


	++NbAllocation;

	return freemem + SizeOfAllocHeader; // return a pointer on the usuable block (after the header)
}

// -------------------------------------------------------------------------------
bool	PAGEALLOC::IsInThisPage(void *address) const
{
	static unsigned int SizeOfAllocHeader = sizeof(ALLOCHEADER);

	if (Memory == NULL)
		return false;

	if ((char *)address - SizeOfAllocHeader < Memory)
		return false;

	if (address > (char *)Memory + TotalSize)
		return false;

	return true;
}


// -------------------------------------------------------------------------------
void	PAGEALLOC::MergeIntoPreviousBlock(ALLOCHEADER *newfreeblock, ALLOCHEADER *previousblockfree)
{
	newfreeblock->Code = MEM_INVALID;
												
	previousblockfree->AllocDef.Size += newfreeblock->AllocDef.Size;

#ifdef DEBUGMEM
	// debug
	char *p = (char *)previousblockfree;
	p += sizeof(ALLOCHEADER);
	memset(p, DEBUGx, previousblockfree->AllocDef.Size - sizeof(ALLOCHEADER));
	#ifdef _DEBUG
		strcpy_s(previousblockfree->AllocDef.Filename, "FREEBLOCK");
		strcpy_s(previousblockfree->AllocDef.Context, "CONTEXT FREE");
	#endif
#endif
}

// -------------------------------------------------------------------------------
// a block (new one, not from free list) is added to the next adjacent one from the free list
// will also check if the previous one was not adjacent to the block that request merging
void	PAGEALLOC::MergeIntoNextBlock(ALLOCHEADER *mergeinto, ALLOCHEADER *nextblockfree)
{
	// check that the block we want to merge is not inside 2 freeblock, if that's the case, merge the 3 together!
	if (nextblockfree->Prev)
	{
		REPORT_ERROR(Param.IReport, nextblockfree->Prev->Code != MEM_FREEBLOCK, "[PageAllocator] Memory corruption detected");

		char *previous = (char *)nextblockfree->Prev;
		previous += nextblockfree->Prev->AllocDef.Size;
		if (previous == (char *)mergeinto)
		{
			ALLOCHEADER *first = nextblockfree->Prev;

			// bingo! merge all the 3 blocks
			first->Next = nextblockfree->Next;
			if (first->Next)
				first->Next->Prev = first;
			first->AllocDef.Size += mergeinto->AllocDef.Size;
			first->AllocDef.Size += nextblockfree->AllocDef.Size;

			#ifdef DEBUGMEM
				// debug
				char *p = (char *)first;
				p += sizeof(ALLOCHEADER);
				memset(p, DEBUGx, mergeinto->AllocDef.Size - sizeof(ALLOCHEADER));
				#ifdef _DEBUG
					strcpy_s(mergeinto->AllocDef.Filename, "FREEBLOCK");
					strcpy_s(mergeinto->AllocDef.Context, "CONTEXT FREE");
				#endif
			#endif

			return;
		}

		// bad luck, not the case
	}

	mergeinto->Next = nextblockfree->Next;
	if (mergeinto->Next)
		mergeinto->Next->Prev = mergeinto;

	mergeinto->Prev = nextblockfree->Prev;

	nextblockfree->Code = MEM_INVALID;

	mergeinto->Code = MEM_FREEBLOCK;
	mergeinto->AllocDef.Size += nextblockfree->AllocDef.Size;

#ifdef DEBUGMEM
	// debug
	char *p = (char *)mergeinto;
	p += sizeof(ALLOCHEADER);
	memset(p, DEBUGx, mergeinto->AllocDef.Size - sizeof(ALLOCHEADER));
	#ifdef _DEBUG
		strcpy_s(mergeinto->AllocDef.Filename, "FREEBLOCK");
		strcpy_s(mergeinto->AllocDef.Context, "CONTEXT FREE");
	#endif
#endif

	if (mergeinto->Prev == NULL)
		FreeList = mergeinto;
	else
		mergeinto->Prev->Next = mergeinto;
}


// -------------------------------------------------------------------------------
void	PAGEALLOC::Free(void *memory)
{
	if (IsInThisPage(memory) == false)
	{
		if (Next == NULL)
		{
			REPORT_ERROR(Param.IReport, true, "[PageAllocator] Could not free the requested address, the given pointer is invalid");
			return;
		}

		Next->Free(memory);
		return;
	}

	// The address is in this page
	static unsigned int SizeOfAllocHeader = sizeof(ALLOCHEADER);

	--NbAllocation;
	REPORT_ERROR(Param.IReport, NbAllocation < 0, "[PageAllocator] More unallocation than allocation, something is not right!");

	// get header
	ALLOCHEADER *header = (ALLOCHEADER *) ((char *)memory - SizeOfAllocHeader);

	// TODO: check if header is corrupted
	REPORT_ERROR(Param.IReport, header->Code != MEM_ALLOCATED, "[PageAllocator] Trying to free a block that wasn't set as being allocated, may be due to memory corruption or did you deleted this pointer already?");

	header->Code = MEM_FREEBLOCK;

#ifdef DEBUGMEM
	#ifdef _DEBUG
		strcpy_s(header->AllocDef.Filename, "FREEBLOCK");
//		strcpy_s(mergeinto->AllocDef.Context, "CONTEXT FREE");
	#endif
#endif

	if (Param.IMonitor)
	{
		MEMORYINFOBLOCK memblock;

		#ifdef _DEBUG
			strncpy_s(memblock.mFilename, header->AllocDef.Filename, MEMORYINFOBLOCK::eMAX_PATH);
			strncpy_s(memblock.mContext, header->AllocDef.Context, MEMORYINFOBLOCK::eMAXCONTEXTSIZE);
			memblock.mLine = header->AllocDef.Line;
			memblock.mAllocNumber = header->AllocDef.mAllocNumber;
		#endif

		memblock.mPageNumber = -1; // TODO

		char *start = (char *)Memory;
		char *end = (char *)header;
		memblock.mOffset = (unsigned int)(end - start);
		memblock.mSize = header->AllocDef.Size;

		Param.IMonitor->OnFree(memblock);
	}

	// common case, the next block is free (happens when you allocate a buffer to do manipulation then just release it)
	char *memheader = (char *)header;
	ALLOCHEADER *nextheader = (ALLOCHEADER *)(memheader + header->AllocDef.Size);
	// make sure we don't go further than our memory size
	if ((char *)nextheader < (char *)Memory + TotalSize)
	{
		if (nextheader->Code == MEM_FREEBLOCK)
		{
			// cool that's the case
			MergeIntoNextBlock(header, nextheader);
			return;
		}
	}

	// TODO: search the next free block to get a hold on the freelist pointers

	// search in freelist to sort block and eventually merge free block of memory
	ALLOCHEADER *freenode = FreeList;
	ALLOCHEADER *lastnode = NULL;
	while (freenode)
	{
		// sort by address
		if (freenode >= header)
		{
			break;
		}

		lastnode = freenode;
		freenode = freenode->Next;
	}

	// this is where our block should be added (check previous block for merge)
	REPORT_ERROR(Param.IReport, freenode == header || lastnode == header, "[PageAllocator] Memory corruption, the freed block was already in freelist");

	if (lastnode)
	{
		// check if last block and this one are adjacent for merge

		if ((char *)lastnode + lastnode->AllocDef.Size == (char *)header)
		{
			// the 2 memory can be merged
			MergeIntoPreviousBlock(header, lastnode);
			return;
		}
	}

	// create our header
	ALLOCHEADER *initialisedheader = new (header) ALLOCHEADER(header->AllocDef.Size, lastnode, freenode);
	if (initialisedheader->Prev == NULL)
		FreeList = initialisedheader;
}


// -------------------------------------------------------------------------------
void	PAGEALLOC::ReportLeak()
{
	if (NbAllocation == 0)
	{
		REPORT_ERROR(Param.IReport, TotalSize != FreeList->AllocDef.Size, "[PageAllocator] Free memory mentioned is not same as memory initialy allocated, error in the IAllocator management of the memory");

		if (Next == NULL)
			return;

		return Next->ReportLeak();
	}

	REPORT_ERROR(Param.IReport, true, "[PageAllocator] %d leak(s) detected in this page allocation", NbAllocation);

	// Leak detected...

	// check where the the first unallocated block

		OutputDebugString("\n-----------------------------------------------\n");
		char buffer[256];
		_snprintf_s(buffer, 256, "REPORT::IAllocator LEAK DETECTED (%d bytes in %d allocation(s)), DUMP OF ALLOCATED PAGE \n", GetAllocatedSize(), GetNumberOfAllocation());
		OutputDebugString(buffer);
#ifndef _DEBUG
			OutputDebugString("- You need to be in debug mode to know what file the allocation is coming from - \n");
#endif
		OutputDebugString("-----------------------------------------------\n\n");


	ALLOCHEADER *header = (ALLOCHEADER *)Memory; // first block
	char *p = (char *)Memory;
	while (true)
	{
		switch (header->Code)
		{
			case MEM_ALLOCATED:
			{
				static char buffer[4096];
				#ifdef _DEBUG
				_snprintf_s(buffer, 4096, "%s(%d) : [%s] PageAllocator LEAK (%d bytes) - Allocation number %i\n", header->AllocDef.Filename, header->AllocDef.Line, header->AllocDef.Context, header->AllocDef.Size, header->AllocDef.mAllocNumber);
					OutputDebugString(buffer);
				#else
					_snprintf_s(buffer, 4096, "PageAllocator LEAK (%d bytes)\n", header->AllocDef.Size);
					OutputDebugString(buffer);
				#endif
			}
			break;

			case MEM_FREEBLOCK:
				break;

			default:
			{
				REPORT_ERROR(Param.IReport, true, "[PageAllocator] Memory is corrupted, exiting report leak\n");
				goto end;
			}
			break;
		}

		// move to next block
		p += header->AllocDef.Size;

		if (p >= (char *)Memory + TotalSize)
			break;

		header = (ALLOCHEADER *)p;
	}

end:

		OutputDebugString("\n------------------------------------------\n");
		OutputDebugString("------------------------------------------\n");
		OutputDebugString("------------------------------------------\n\n");
}

// -------------------------------------------------------------------------------
void	PAGEALLOC::SetMemoryNotification(IMEMORYNOTIFICATION *memnotification)
{
	Param.IMonitor = memnotification;
}


// -------------------------------------------------------------------------------
void	PAGEALLOC::Dump(IMEMORYENUMERATION *notification, unsigned int pagenumber)
{
	MEMORYINFOBLOCK memblock;
	#ifndef _DEBUG
		strcpy_s(memblock.mFilename, "USE DEBUG TO GET THIS INFORMATION");
		strcpy_s(memblock.mContext, "USE DEBUG TO GET THIS INFORMATION");
		memblock.mLine = 0;
		memblock.mAllocNumber = 0;
	#endif
	memblock.mPageNumber = pagenumber;

	ALLOCHEADER *header = (ALLOCHEADER *)Memory; // first block
	char *p = (char *)Memory;
	bool previouslyfree = false;
	while (true)
	{
		switch (header->Code)
		{
			case MEM_ALLOCATED: // normal
			{
				previouslyfree = false;
			}
			break;

			case MEM_FREEBLOCK: // normal unless the frevious block was also free, in this case we have fragmented free blocks and we want to avoid this!
			{
				REPORT_WARNING(Param.IReport, previouslyfree, "[PageAllocator] Framgmented memory blocks, likely to be a bug in library, should not affect your game though\n");
				previouslyfree = true;
			}
			break;

			default:
			{
				REPORT_ERROR(Param.IReport, true, "[PageAllocator] Memory is corrupted, exiting dump\n");
				return;
			}
			break;
		}

		if (header->Code == MEM_ALLOCATED)
		{
			#ifdef _DEBUG
				strncpy_s(memblock.mFilename, header->AllocDef.Filename, MEMORYINFOBLOCK::eMAX_PATH);
				strncpy_s(memblock.mContext, header->AllocDef.Context, MEMORYINFOBLOCK::eMAXCONTEXTSIZE);
				memblock.mLine = header->AllocDef.Line;
				memblock.mAllocNumber = header->AllocDef.mAllocNumber;
			#endif

			memblock.mPageNumber = -1; // TODO

			char *start = (char *)Memory;
			char *end = (char *)header;
			memblock.mOffset = (unsigned int)(end - start);

			memblock.mSize = header->AllocDef.Size;

			if (notification->EnumMemoryBlock(memblock) == false)
				return; // stop enumeration
		}

		// move to next block
		p += header->AllocDef.Size;

		if (p >= (char *)Memory + TotalSize)
			break;

		header = (ALLOCHEADER *)p;
	}

	if (Next)
		Next->Dump(notification, pagenumber + 1);
}



// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
PageAllocator::PageAllocator(MEMORYPARAM &memparam)
: RefCount(1), FirstPage(memparam, NULL), mNumAlloc(0)
{
	Param = memparam;

	/*
	if (Param.IReport)
	{
		Param.IReport->AddRef(); // acquiert report interface
	}
	*/

	InitializeCriticalSection(&CriticalSection);
}

// -------------------------------------------------------------------------------
PageAllocator::~PageAllocator()
{
	FirstPage.ReportLeak();

	/*
	if (Param.IReport)
	{
		Param.IReport->DecRef(); // release report interface
	}
	*/

	DeleteCriticalSection(&CriticalSection);
}

// -------------------------------------------------------------------------------
void	PageAllocator::Destroy()
{
	if (RefCount > 1)
	{
		REPORT_ERROR(Param.IReport, true, "[PageAllocator] Still some handles on IAllocator (%d), check decref is called enough time on IAllocator, forcing memory leak checks", RefCount);

		FirstPage.ReportLeak(); // still do the report on leaks
	}

	DecRef();
};

// -------------------------------------------------------------------------------
void	*PageAllocator::Allocate(unsigned int size, const char *context, const char *filename, long line)
{
	LOCKMEM();

#ifdef DEBUGMEM
	DEBUGx = rand() % 256;
#endif

	// return malloc(size);

	ALLOCDEF allocdef(size);

#ifdef _DEBUG
	strncpy_s(allocdef.Filename, filename, ALLOCDEF::eMAX_PATH);
	if (context == NULL)
		strncpy_s(allocdef.Context, "NULL", ALLOCDEF::eMAXCONTEXTSIZE);
	else
		strncpy_s(allocdef.Context, context, ALLOCDEF::eMAXCONTEXTSIZE);
	allocdef.Line = line;
	allocdef.mAllocNumber = InterlockedIncrement(&mNumAlloc);
#endif

	char *memory = (char *)FirstPage.Allocate(allocdef);

	// OutputDebugString(GIQuickSprintf("%s(%d) : GIMemory ALLOC of %d bytes for %s\n", filename, line, size, context));
	// OutputDebugString(GIQuickSprintf("TOTAL Allocation(%d) Size(%d)\n", NbAllocation, TotalMemory));

#ifdef DEBUGMEM
//	Dump(NULL);
#endif

	return memory;
}

// -------------------------------------------------------------------------------
void	PageAllocator::Free(void *memory)
{
	LOCKMEM();

#ifdef DEBUGMEM
	DEBUGx = rand() % 256;
#endif

	// free(memory);
	// return;

	if (memory == NULL)
		return;

	FirstPage.Free(memory);

#ifdef DEBUGMEM
//	Dump(NULL);
#endif
}

// -------------------------------------------------------------------------------
unsigned int	PageAllocator::GetNumberOfAllocation()
{
	return FirstPage.GetNumberOfAllocation();
}

// -------------------------------------------------------------------------------
unsigned int	PageAllocator::GetFreeMemorySize()
{
	return FirstPage.GetFreeMemorySize();
}

// -------------------------------------------------------------------------------
unsigned int	PageAllocator::GetAllocatedSize()
{
	return FirstPage.GetAllocatedSize();
}

// -------------------------------------------------------------------------------
void	PageAllocator::SetMemoryNotification(IMEMORYNOTIFICATION *memnotification)
{
	FirstPage.SetMemoryNotification(memnotification);
}

// -------------------------------------------------------------------------------
void	PageAllocator::Dump(IMEMORYENUMERATION *notification)
{
	FirstPage.Dump(notification, 0);
}

// -------------------------------------------------------------------------------
int	PageAllocator::AddRef()
{
	RefCount++;

	// sanity check
	if (RefCount > 1000000)
	{
		REPORT_WARNING(Param.IReport, true, "[PageAllocator] AddRef > 1000000, are you sure AddRef is not called without DecRef being called?\n");
	}

	return RefCount;
}


// -------------------------------------------------------------------------------
int	PageAllocator::DecRef()
{
	if (RefCount < 0)
	{
		REPORT_ERROR(Param.IReport, true, "[PageAllocator] DecRef, but already deleted, did you call RefCount more times than AddRef?\n");
		return RefCount;
	}

	RefCount--;

	if (RefCount == 0)
	{
		delete this; // no one claim to be using it anymore, please release it.
		return 0;
	}

	return RefCount;
}



// -------------------------------------------------------------------------------
bool	PageAllocator::Lock()
{
	EnterCriticalSection(&CriticalSection);

	/*
	DWORD hr = WaitForSingleObject(Mutex, INFINITE);
	if (hr != WAIT_OBJECT_0)
	{
		REPORT_ERROR(Param.IReport, true, "[PageAllocator] Mutex failed to lock");
		return false;
	}
	*/

	return true;
}

// -------------------------------------------------------------------------------
void	PageAllocator::Unlock()
{
	LeaveCriticalSection(&CriticalSection);
}

