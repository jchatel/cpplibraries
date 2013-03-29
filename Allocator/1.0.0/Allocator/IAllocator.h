#pragma once

// ------------------------------------------------------------------------------------------
#include <New.h>

namespace Reporter {
	class IReporter;
}

namespace Allocator {

// -------------------------------------------------------------------------------
struct MEMORYINFOBLOCK // For Debug and dumping
{
	enum
	{
		eMAXCONTEXTSIZE = 128,
		eMAX_PATH = 256,
	};

	int		mPageNumber; // What page is the memory on (0 means main allocation page)
	unsigned int	mSize; // Size of the allocated block of memory
	unsigned int	mOffset; // What address in the allocator can you find the data
	char	mFilename[eMAX_PATH]; // In Debug Build only: File where the allocation was requested
	char	mContext[eMAXCONTEXTSIZE]; // In Debug Build only: Send back the user text given as a context of the allocation
	int		mLine; // In Debug Build only: Line where the allocation was requestion
	int		mAllocNumber; // In Debug Build only: allocation number (to help debugging memory leaks)
};



// -------------------------------------------------------------------------------
class IMEMORYENUMERATION
{
public:

	virtual ~IMEMORYENUMERATION() {};

	virtual bool	EnumMemoryBlock(const MEMORYINFOBLOCK &memblock) { (memblock); return true;};
};



// -------------------------------------------------------------------------------
class IMEMORYNOTIFICATION
{
public:

	virtual ~IMEMORYNOTIFICATION() {};

	virtual void	OnAllocate(const MEMORYINFOBLOCK &memblock) = 0;
	virtual void	OnFree(const MEMORYINFOBLOCK &memblock) = 0;
};

// -------------------------------------------------------------------------------
class MEMORYPARAM
{
public:
	MEMORYPARAM();

	unsigned int		BaseMemoryInBytes; // Set by default to 50 meg
	unsigned int		MemoryPerPage; // Set by default to 10 meg

	/*
	enum eALLOCATORTYPE
	{
		ePAGEALLOCATOR, // default
		eBLOCKALLOCATOR, // allocation of fixed size object, allow for more efficient usage of memory management
		eMALLOCFREE, // simple call to malloc and free, no facilities except allocate and free function
	};

	eALLOCATORTYPE		mTypeAllocator;
	*/

	Reporter::IReporter	*IReport; // for the IAllocator to be able to report errors, it is acceptable to give NULL
								  // although you will have not have reports even in case or fatal errors

	IMEMORYNOTIFICATION	*IMonitor; // an interface if you want to monitor allocation and desallocation at runtime (optional)
};

// -------------------------------------------------------------------------------
class IAllocator
{
protected:
	IAllocator() {}; // use Create
	virtual ~IAllocator() {}; // use Destroy

public:
	virtual void			Destroy() = 0; // automatically DecRef
	
	virtual void			*Allocate(unsigned int size, const char *context, const char *filename, long line) = 0;
	virtual void			Free(void *memory) = 0;

	virtual int				AddRef() = 0;
	virtual int				DecRef() = 0;

	// Debug helper:
	virtual void			SetMemoryNotification(IMEMORYNOTIFICATION *memnotification) = 0;

	virtual unsigned int	GetNumberOfAllocation() = 0;
	virtual unsigned int	GetFreeMemorySize() = 0;
	virtual unsigned int	GetAllocatedSize() = 0;
	virtual void			Dump(IMEMORYENUMERATION *notification) {(notification);};
};


namespace Helper {

	IAllocator		*__fastcall CreatePageAllocator(MEMORYPARAM &memparam); // automatically AddRef
	IAllocator		*__fastcall CreateOSAllocator(); // use malloc and free to allocate and deallocate and doens't provide nohing else (no memory leaking system or 
}

// Helper
template <typename T>
inline void CallDestructor(T* _pPointer)
{
	_pPointer->~T();
}

#define MEM_ALLOC(allocator, type, context) new (allocator->Allocate(sizeof(type), context, __FILE__, __LINE__)) type
#define MEM_UNALLOC(allocator, pointer) {if (pointer) {Allocator::CallDestructor(pointer); allocator->Free(pointer); pointer = NULL;}}
#define MEM_UNALLOC_THIS(allocator) {Allocator::CallDestructor(this); allocator->Free(this);}

}; // namespace