#include "stdafx.h"
#include "CompressionReader.h"

using namespace Compression;



// -------------------------------------------------------------------------------
CompressionReader::CompressionReader(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, Stream::IStreamReader *streamreader, Stream::IStreamWriter *streamwriter)
: mAllocator(allocator), mReporter(reporter), mStreamReader(streamreader), mStreamWriter(streamwriter)
{
}

// -------------------------------------------------------------------------------
void	CompressionReader::Destroy()
{
}

// -------------------------------------------------------------------------------
bool	CompressionReader::Init()
{
	return true;
}

// -------------------------------------------------------------------------------
bool	CompressionReader::Process()
{
	const static int READ_CHUNK_SIZE = 1024;
	const static uLong DECOMPRESSED_DATA_BUFFER_SIZE = 1024 * 5;

	unsigned int totalinput = mStreamReader->GetSize();

	unsigned char inputbuffer[READ_CHUNK_SIZE];
	unsigned char outputbuffer[DECOMPRESSED_DATA_BUFFER_SIZE];

	int err;
	z_stream d_stream; // decompression stream 

//	strcpy((char*)uncompr, "garbage");

	d_stream.zalloc = (alloc_func)0; // TODO using allocator
	d_stream.zfree = (free_func)0; // TODO using allocator
	d_stream.opaque = (voidpf)0;

	d_stream.next_in  = NULL; //(unsigned char *)Data;
	d_stream.avail_in = 0;
	d_stream.next_out = NULL;

	err = inflateInit(&d_stream);
	if (err != Z_OK)
	{
		// error!
		REPORT_ERROR(mReporter, true, "[Compression/Reader] Compressed data seems corrupted, error at inflateInit: %d", err);
		return false;
	}

	while (true)
	{
		if (d_stream.avail_in < READ_CHUNK_SIZE)
		{
			d_stream.avail_in = mStreamReader->Read(inputbuffer, READ_CHUNK_SIZE - d_stream.avail_in);
		}

		if (d_stream.avail_in == 0)
			break;

		d_stream.next_in = inputbuffer;

		do
		{
			d_stream.next_out = outputbuffer;
			d_stream.avail_out = DECOMPRESSED_DATA_BUFFER_SIZE;
			d_stream.total_out = 0;

			err = inflate(&d_stream, Z_NO_FLUSH);

			if (err != Z_STREAM_END && err != Z_OK)
			{
				// error!
				REPORT_ERROR(mReporter, true, "[Compression/Reader] Compressed data seems corrupted, error at inflate: %d", err);
				return false;
			}

			mStreamWriter->Write(outputbuffer, d_stream.total_out);
		}
		while (d_stream.avail_in > 0 && d_stream.total_out > 0);
	}

	err = inflateEnd(&d_stream);
	if (err != Z_OK)
	{
		// error!
		REPORT_ERROR(mReporter, true, "[Compression/Reader] Compressed data seems corrupted, error at inflateEnd: %d", err);
		return false;
	}


	mStreamWriter->Destroy();

	return true;
}
