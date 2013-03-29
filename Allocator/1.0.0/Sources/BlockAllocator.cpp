#include "stdafx.h"
#include "BlockAllocator.h"


using namespace Allocator;
using namespace Reporter;


// -------------------------------------------------------------------------------
BlockAllocator::BlockAllocator(unsigned int blocksize, unsigned int blocksperpage, const char *context, const char *filename, long line)
: mBlockSize(blocksize), mBlocksPerPage(blocksperpage)
{
}

// -------------------------------------------------------------------------------
BlockAllocator::~BlockAllocator()
{
}


// -------------------------------------------------------------------------------
void	*BlockAllocator::Allocate(const char *context, const char *filename, long line)
{
//	return malloc(mBlockSize + sizeof(BlockInfo));
	return malloc(mBlockSize);
}

// -------------------------------------------------------------------------------
void	BlockAllocator::Free(void *pointer)
{
	free(pointer);
}



