#pragma once


namespace Network {

	class IOConnection;

	// -------------------------------------------------------------------------------
	struct IOBUFFER
	{
//		enum {MAX_BUFFER_RECEIVE = 64 * 1024};

		IOBUFFER(unsigned int size)
			: mSize(size)
		{
			mBuffer = new char[size];
		}

		IOBUFFER(void *data, unsigned int size)
			: mSize(size)
		{
			mBuffer = new char[size];

			// TODO exception if buffer bigger than size
			memcpy(mBuffer, data, size);
		}

		~IOBUFFER()
		{
			delete [] mBuffer;
			mBuffer = NULL;
		}

		char	*mBuffer; //[MAX_BUFFER_RECEIVE];
		DWORD	mSize;
	};

	// ---------------------------------------------------------------------------------------------
	class IOOVERLAPPED : public OVERLAPPED
	{
	public:

		enum IOOPCODE
		{
			IOINVALID = -1,

			IOACCEPT,
			IOREAD,
			IOWRITE,
			IOCLOSE,
			IOCONNECT,

			IODELETED, // for debug purposes

			IOLAST
		};

	public:

		IOOPCODE			mOpCode;
		IOConnection		*mIOConnection;
		IOBUFFER			*mIOBuffer;
		Allocator::IAllocator *mAllocator;

		unsigned char		mAcceptData[(sizeof(sockaddr_in) + 16) * 2];
		DWORD				mAcceptBytes;
		SOCKET				mSocket;

		IOOVERLAPPED(Allocator::IAllocator *allocator, IOOPCODE opcode, IOConnection *client = NULL);

		void Destroy();

	private:
		~IOOVERLAPPED();
	};


} // namespace