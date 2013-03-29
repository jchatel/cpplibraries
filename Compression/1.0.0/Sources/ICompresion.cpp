#include "stdafx.h"
#include "../Compression/ICompression.h"
#include "CompressionReader.h"


// -------------------------------------------------------------------------------
namespace Compression
{
namespace Helper
{

	// -------------------------------------------------------------------------------
ICompressionReader	*__fastcall CreateCompressionReader(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, Stream::IStreamReader *streamreader, Stream::IStreamWriter *streamwriter)
{
	CompressionReader *object = MEM_ALLOC(allocator, CompressionReader, "Compression::Reader")(allocator, reporter, streamreader, streamwriter);

	if (object->Init() == false)
	{
		MEM_UNALLOC(allocator, object);
		object = NULL;
	}

	return object;
}


}
}