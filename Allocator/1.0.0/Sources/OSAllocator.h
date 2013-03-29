#pragma once

#include "../Allocator/IAllocator.h"

namespace Allocator {

// -------------------------------------------------------------------------------
class	OSALLOCATOR : public IAllocator
{
private:

	volatile LONG mAllocationCount;

public:

	OSALLOCATOR();
	~OSALLOCATOR();

//	void			Destroy() {this->~OSALLOCATOR; free(this);}
	void			Destroy() {delete(this);}
	
	void			*Allocate(unsigned int size, const char *context, const char *filename, long line);
	void			Free(void *memory);

	int				AddRef() {return 0;};
	int				DecRef() {return 0;};

	// Debug helper:
	void			SetMemoryNotification(IMEMORYNOTIFICATION *memnotification) {};

	unsigned int	GetNumberOfAllocation() {return 0;};
	unsigned int	GetFreeMemorySize() {return 0;};
	unsigned int	GetAllocatedSize() {return 0;};
	void			Dump(IMEMORYENUMERATION *notification) {};
};

}

