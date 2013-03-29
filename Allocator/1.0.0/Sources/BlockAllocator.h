#pragma once


namespace Allocator {

// -------------------------------------------------------------------------------
class BlockAllocator
{
private:

	struct BlockInfo
	{
		char			mFile[MAX_PATH];
		unsigned int	mLine;
		char			mContext[255];
	};

	unsigned int		mBlockSize;
	unsigned int		mBlocksPerPage;

public:

	BlockAllocator(unsigned int blocksize, unsigned int blocksperpage, const char *context, const char *filename, long line);
	~BlockAllocator();

	void	*Allocate(const char *context, const char *filename, long line);
	void	Free(void *pointer);

	unsigned int GetSize() {return mBlockSize * mBlocksPerPage;};
};


} // namespace
