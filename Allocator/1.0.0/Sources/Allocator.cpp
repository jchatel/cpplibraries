#include "stdafx.h"
#include "../Allocator/IAllocator.h"
#include "OSAllocator.h"
#include "PageAllocator.h"
#include "BlockAllocator.h"

namespace Allocator {


// -------------------------------------------------------------------------------
MEMORYPARAM::MEMORYPARAM()
: BaseMemoryInBytes(50 * 1024 * 1204), MemoryPerPage(10 * 1024 * 1024), IReport(NULL), IMonitor(NULL)
//, mTypeAllocator(ePAGEALLOCATOR)
{

}

// -------------------------------------------------------------------------------
namespace Helper { 

// -------------------------------------------------------------------------------
IAllocator	*__fastcall CreatePageAllocator(MEMORYPARAM &memparam)
{
	return new PageAllocator(memparam);
}

// -------------------------------------------------------------------------------
IAllocator	*__fastcall CreateOSAllocator()
{
	return new OSALLOCATOR();
}

} // namespace

} // namespace