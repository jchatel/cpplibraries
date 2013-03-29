#pragma once


namespace Stream
{


// -------------------------------------------------------------------------------
class StreamReadMemory : public Stream::IStreamReader
{
public:
	const char		*mMemoryAddress;
	unsigned int	mCursor;
	unsigned int	mMaxSize;

	StreamReadMemory(const char *addr, int size)
		: mMemoryAddress(addr), mCursor(0), mMaxSize(size)
	{
	}

	void			Destroy() {};

	unsigned int	GetCurrentPosition()
	{
		return mCursor;
	};
	unsigned int	GetSize()
	{
		return mMaxSize;
	};
	bool			Seek(unsigned int offset)
	{
		mCursor = offset;
		return true;
	};
	unsigned int	Read(void *destination, unsigned int buffersize)
	{
		unsigned int tocopy = buffersize;

		if (tocopy > mMaxSize - mCursor)
			tocopy = mMaxSize - mCursor;
		memcpy(destination, mMemoryAddress + mCursor, tocopy);

		mCursor += tocopy;						   

		return tocopy;
	};
	unsigned int	ReadLine(void *destination, unsigned int maxsize)
	{
		return 0;
	}
	const char		*GetName()
	{
		return "MEMORY";
	};
};

	

}; // namespace
