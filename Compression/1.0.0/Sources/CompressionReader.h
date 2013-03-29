#pragma once


namespace Compression {




// -------------------------------------------------------------------------------
class CompressionReader : public ICompressionReader
{
private:

	Allocator::IAllocator		*mAllocator;
	Reporter::IReporter			*mReporter;
	Stream::IStreamReader		*mStreamReader;
	Stream::IStreamWriter		*mStreamWriter;

public:

	CompressionReader(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, Stream::IStreamReader *streamreader, Stream::IStreamWriter *streamwriter);

	bool	Init();

public: // ICompressionReader

	void	Destroy();
	bool	Process();
};













} // namespace







