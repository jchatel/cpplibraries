#pragma once

namespace Network {

class IOWorker;
class IOCONNECTION;
class IOOVERLAPPED;

// -------------------------------------------------------------------------------
class IOConnection : public IIOConnection
{
public:

	static volatile	LONG	NumberOfConnection;
	volatile		LONG	mRefCount;
	SOCKET					mSocket;
	DWORD					mLastAccessed;
	static volatile	LONG	GlobalId;
	volatile		LONG	mIdClient;
	Utils::CriticalSection	mCriticalSection;
	void					*mUserData;
	sockaddr_in				mSocketAddress;

	Allocator::IAllocator	*mAllocator;
	Reporter::IReporter		*mReporter;

	std::string				mDebugName;

	std::string				mResolvedHost;

#ifdef LOG_BINARY
	FILE					*mBinaryFile;
#endif

public:

	IOConnection(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, SOCKET socket, const sockaddr_in &addr, const char *debugname);
	~IOConnection();

	int	Acquire();
	int	Release();

	bool	Shutdown();
	void	CloseSocket();

	inline void	Touch() {mLastAccessed = GetTickCount();};

	virtual volatile LONG	GetIdClient() {return mIdClient;};

	SOCKET	GetSocket() {return mSocket;}
	void	SendData(void *data, unsigned int size);
	void	SendText(const char *text);
	void	SendTextFormatted(const char *text, ...);

	void	SetUserData(void *data)
	{
		mUserData = data;
	}

	void	*GetUserData()
	{
		return mUserData;
	}

	sockaddr_in	&GetSockAddr()
	{
		return mSocketAddress;
	}

	const char *GetResolvedName();
};


} // namespace
