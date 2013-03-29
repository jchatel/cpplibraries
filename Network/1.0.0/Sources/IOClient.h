#pragma once

#include "IOWorker.h"

namespace Network {

class IOOVERLAPPED;
class IOConnection;

// ---------------------------------------------------------------------------------------------
class IOClient : public IIOClient, public IWorkerNotification
{
private:

	HANDLE	mIOCompletionPort;
	DWORD	NumberOfThreads;
	bool	mThreadCreated;

	IIOClientNotification	*mNotification;
	IOConnection			*mConnection;

	LPFN_CONNECTEX			lpfnConnectEx;

	Allocator::IAllocator	*mAllocator;
	Reporter::IReporter		*mReporter;

	std::string				mDebugName;
	HANDLE					mAllThreadsDone;

	PROTOCOL::Enum			mProtocol;

private:

	// Thread (wait) process
	DWORD	ThreadProcessId;

	enum
	{
		eMAX_THREADS = 1
	};

	IOWorker	*ListOfIOWorker[eMAX_THREADS];

	volatile LONG mRef;

	Utils::CriticalSection	mCriticalSection;

	void	Uninit();

public:

	std::string		mAddress;
	unsigned short	mPort;

public:

	IOClient(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, IIOClientNotification *notificationobject, PROTOCOL::Enum protocol, const char *debugname);
	~IOClient();

	bool	Init();

	bool	PostRead(IOConnection *client);
	bool	OnReceiveData(IOOVERLAPPED *overlap);
	void	OnConnected(IOConnection *ioclient);
	void	OnFailToConnect(IOConnection *ioclient);

public: // IIOClient

	bool	Connect(const char *address, unsigned short port);
	void	Destroy();

public: // IWorkerNotification

	void	OnWorkerAccept(IOOVERLAPPED *overlap) {}; // useless for clients, TODO log error
	void	OnWorkerConnect(IOOVERLAPPED *overlap);
	void	OnWorkerDisconnect(IOOVERLAPPED *overlap);
	void	OnWorkerReceiveData(IOOVERLAPPED *overlap);

	void	Release();
};

} // namespace
