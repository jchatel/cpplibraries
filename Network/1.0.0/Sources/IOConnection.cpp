#include "stdafx.h"
#include "IOConnection.h"
#include "IOOverlapped.h"

using namespace Network;

#pragma warning(disable:4996)

// -------------------------------------------------------------------------------
volatile LONG	IOConnection::NumberOfConnection = 0;
volatile LONG	IOConnection::GlobalId = 0;

// -------------------------------------------------------------------------------
IOConnection::IOConnection(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, SOCKET socket, const sockaddr_in &addr, const char *debugname)
: mRefCount(1), mSocket(socket), mAllocator(allocator), mReporter(reporter), mDebugName(debugname), mUserData(NULL)
{
#ifdef LOG_BINARY
	static int inc = -1;
	++inc;
	mBinaryFile = fopen(debugname, "wb");
	if (mBinaryFile)
		fclose(mBinaryFile);
#endif

	memcpy(&mSocketAddress, &addr, sizeof(sockaddr_in));

	Touch();
	InterlockedIncrement(&IOConnection::NumberOfConnection);
	mIdClient = InterlockedIncrement(&IOConnection::GlobalId);
}

// -------------------------------------------------------------------------------
IOConnection::~IOConnection()
{
	InterlockedDecrement(&IOConnection::NumberOfConnection);
}

// -------------------------------------------------------------------------------
int	IOConnection::Acquire()
{
	Touch();
	return InterlockedIncrement(&mRefCount);
};


// -------------------------------------------------------------------------------
int	IOConnection::Release()
{
	int val = InterlockedDecrement(&mRefCount);
	if (val > 0)
		return val;

	if (val < 0)
	{
		REPORT_ERROR(mReporter, true, "Network::IOConnection(%s): Connection already deleted!", mDebugName.c_str());
		return 0;
	}

	val = -1; // to help debugging

	MEM_UNALLOC_THIS(mAllocator);

	return 0;
}

// -------------------------------------------------------------------------------
bool	IOConnection::Shutdown()
{
	Utils::Lock lock(mCriticalSection);
	// no one has a handle on it, release

	if (mSocket == INVALID_SOCKET)
	{
		REPORT_ERROR(mReporter, true, "Network::IOConnection(%s): Invalid socket at shutdown", mDebugName.c_str());
		return false;
	}

	shutdown(mSocket, SD_BOTH);

	return true;
}

// -------------------------------------------------------------------------------
void	IOConnection::CloseSocket()
{
	Utils::Lock lock(mCriticalSection);
	// no one has a handle on it, release

	if (mSocket == INVALID_SOCKET)
	{
		REPORT_ERROR(mReporter, true, "Network::IOConnection(%s): Invalid socket at close", mDebugName.c_str());
		return;
	}

	closesocket(mSocket);

	mSocket = INVALID_SOCKET;
}

// -------------------------------------------------------------------------------
const char *IOConnection::GetResolvedName()
{
	if (mResolvedHost.empty())
	{
		char buffer[256];
		char service[256];

		getnameinfo((SOCKADDR *)&mSocketAddress, sizeof(mSocketAddress), buffer, 128, service, 128, 0);

		mResolvedHost = buffer;
	}

	return mResolvedHost.c_str();
}

// -------------------------------------------------------------------------------
void	IOConnection::SendText(const char *text)
{
	SendData((void *)text, (unsigned int)strlen(text));
}

// -------------------------------------------------------------------------------
void	IOConnection::SendTextFormatted(const char *text, ...)
{
	char	buffer[4096];

	va_list args;
	va_start(args, text);
	if (_vsnprintf_s(buffer, 4096, text, args) == -1)
	{
		REPORT_ERROR(mReporter, true, "Network::IOConnection(%s): Text too big: can't use SendTextFormatted with string bigger than 4k", mDebugName.c_str());
	}
	va_end(args);
	buffer[4095] = '\0';

	SendData(buffer, (unsigned int)strlen(buffer));
}

// -------------------------------------------------------------------------------
void	IOConnection::SendData(void *data, unsigned int size)
{
	if (size == 0)
		return;

	Utils::Lock lock(mCriticalSection);

#ifdef LOG_BINARY
	mBinaryFile = fopen(mDebugName.c_str(), "ab");
	if (mBinaryFile)
	{
		fwrite(data, size, 1, mBinaryFile);
		fclose(mBinaryFile);
	}
#endif

	if (mSocket == INVALID_SOCKET)
	{
		REPORT_ERROR(mReporter, true, "Network::IOConnection(%s): Invalid socket at SendData", mDebugName.c_str());
		return; // cancel send, socket is closing
	}

	IOOVERLAPPED *newoverlap = MEM_ALLOC(mAllocator, IOOVERLAPPED, "Network::IOConnection/IOOVERLAPPED")(mAllocator, IOOVERLAPPED::IOWRITE, this);

	newoverlap->mIOBuffer = MEM_ALLOC(mAllocator, IOBUFFER, "Network::IOConnection/IOBUFFER")(data, size);

	DWORD flags = 0;

    WSABUF WinsockBuffer;
	WinsockBuffer.buf = newoverlap->mIOBuffer->mBuffer;
	WinsockBuffer.len = newoverlap->mIOBuffer->mSize;

//	LPWSAOVERLAPPED_COMPLETION_ROUTINE
	DWORD bytesent;
	int ret = 0;

	ret = WSASend(mSocket, &WinsockBuffer, 1, &bytesent, flags, newoverlap, NULL);
/*
	WSAMSG msg;
	msg.name = (SOCKADDR *)&mSocketAddress;
	msg.namelen = sizeof(sockaddr_in);
	msg.lpBuffers = &WinsockBuffer;
	msg.dwBufferCount = 1;

	/*
	WSACMSGHDR *cmsgp;
	struct sctp_sndrcvinfo *srcv;

	unsigned char msgbuf[(WSA_CMSG_SPACE(sizeof(struct sctp_sndrcvinfo)))];
	memset(&msgbuf, 0, sizeof(msgbuf));
	cmsgp = (WSACMSGHDR *)msgbuf;
	cmsgp->cmsg_level = IPPROTO_SCTP;
	cmsgp->cmsg_type = SCTP_SNDRCV;
	cmsgp->cmsg_len = sizeof(msgbuf);

	msg.Control.buf = msgbuf;
	msg.Control.len = sizeof(msgbuf);
	

	msg.Control.len = 0;

	msg.dwFlags = 0;

	ret = WSASendMsg(mSocket, &msg, flags, &bytesent, newoverlap, NULL);
*/

	if (ret == SOCKET_ERROR)
	{
		int code = WSAGetLastError();
		if (code != ERROR_IO_PENDING)
		{
			REPORT_LOG(mReporter, true, "Network::IOConnection(%s): Shutdown initiated following error on Send (%d)", mDebugName.c_str(), code);

			Shutdown();

//			IOOVERLAPPED *closeoverlap = MEM_ALLOC(mAllocator, IOOVERLAPPED(IOOVERLAPPED::IOCLOSE, this);
//			PostQueuedCompletionStatus(gIocp, 0, NULL, closeoverlap);

			newoverlap->Destroy();
		}
	}
}




