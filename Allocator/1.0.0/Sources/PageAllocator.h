#pragma once

#include "../Allocator/IAllocator.h"

namespace Allocator {


enum MEMORYCODE
{
	MEM_INVALID		= 0xBA,
	MEM_FREEBLOCK	= 0xFB,
	MEM_ALLOCATED	= 0xA0,
};


// -------------------------------------------------------------------------------
class	ALLOCDEF
{
public:
	unsigned int Size;

	#ifdef _DEBUG

		enum
		{
			eMAXCONTEXTSIZE = 128,
			eMAX_PATH = 256,
		};

		int		Line;
		char	Context[eMAXCONTEXTSIZE];
		char	Filename[eMAX_PATH];
		int		mAllocNumber;
	#endif

public:

	ALLOCDEF(unsigned int size);
};

// -------------------------------------------------------------------------------
class	ALLOCHEADER
{
public:
	
	unsigned char	Code; // Freed, allocated

	ALLOCDEF	AllocDef;
	
	ALLOCHEADER	*Prev;
	ALLOCHEADER	*Next;

public:

	ALLOCHEADER(unsigned int sizeinbytes, ALLOCHEADER *previous, ALLOCHEADER *next);
};

// -------------------------------------------------------------------------------
class	PAGEALLOC
{
public:
	int		NbAllocation;

	void	*Memory;
	unsigned int	TotalSize;

	PAGEALLOC *Next;

	ALLOCHEADER	*FreeList;

	MEMORYPARAM	Param;

	bool		IsExtraPage;

protected:

	void	PrimeAllocate();
	void	MergeIntoPreviousBlock(ALLOCHEADER *newfreeblock, ALLOCHEADER *previousblockfree);
	void	MergeIntoNextBlock(ALLOCHEADER *mergeinto, ALLOCHEADER *nextblockfree);

public:

	PAGEALLOC(MEMORYPARAM &memparam, PAGEALLOC *previouspage, bool extrapage = false);
	~PAGEALLOC();

	inline bool IsValid() {return Memory != NULL;}

	ALLOCHEADER *CanAllocate(unsigned int totalsize) const;
	void		*Allocate(ALLOCDEF &allocdef);

	inline bool	IsInThisPage(void *address) const;
	void		Free(void *memory);

	void		ReportLeak();
	void		Dump(IMEMORYENUMERATION *notification, unsigned int pagenumber);
	void		SetMemoryNotification(IMEMORYNOTIFICATION *memnotification);

	unsigned int	GetNumberOfAllocation();
	unsigned int	GetFreeMemorySize();
	unsigned int	GetAllocatedSize();
};



#define LOCKMEM() MEMORYMT __lock(this)

// -------------------------------------------------------------------------------
class	PageAllocator : public IAllocator
{
protected:

	class	MEMORYMT
	{
	public:

		MEMORYMT(PageAllocator *mem)
			: Mem(mem)
		{
			Mem->Lock();
		}

		~MEMORYMT()
		{
			Mem->Unlock();
		}

		PageAllocator *Mem;
	};

protected:

	volatile LONG		mNumAlloc;

	int					RefCount;
	CRITICAL_SECTION	CriticalSection;
	MEMORYPARAM			Param;

public:

	PAGEALLOC	FirstPage;

public:

	PageAllocator(MEMORYPARAM &memparam);
	~PageAllocator();
	void	Destroy();

	void	*Allocate(unsigned int size, const char *context, const char *filename, long line);
	void	Free(void *memory);
	void	Dump(IMEMORYENUMERATION *notification);

	void	SetMemoryNotification(IMEMORYNOTIFICATION *memnotification);

	unsigned int	GetNumberOfAllocation();
	unsigned int	GetFreeMemorySize();
	unsigned int	GetAllocatedSize();

	bool	Lock();
	void	Unlock();

	int		AddRef();
	int		DecRef();
};

}

