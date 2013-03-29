#pragma once

namespace Reporter {
	class IReporter;
}

namespace Allocator {
	class IAllocator;
}

namespace Stream {
	class IStream;
}

namespace Compression {

// -------------------------------------------------------------------------------
class ICompressionReader
{
protected:
	ICompressionReader() {}; // use Create
	virtual ~ICompressionReader() {}; // use Destroy

public:

	virtual void	Destroy() = 0;

	virtual bool	Process() = 0;
};

// -------------------------------------------------------------------------------
namespace Helper
{
	ICompressionReader	*__fastcall CreateCompressionReader(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, Stream::IStreamReader *streamreader, Stream::IStreamWriter *streamwriter);
}

// -------------------------------------------------------------------------------
class ICompressionWriter
{
};




} // namespace





