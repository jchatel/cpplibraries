#pragma once

#include "IOWorker.h"

namespace Network {

class IOWorker;
class IOCONNECTION;
class IOOVERLAPPED;
class IOConnection;


// ---------------------------------------------------------------------------------------------
class IOServer : public IIOServer, public IWorkerNotification
{
public:

	HANDLE	mIOCompletionPort;
	HANDLE	CloseServer;
	SOCKET	ListenSocket;
	DWORD	NumberOfThreads;

	PROTOCOL::Enum mProtocol;

private:

	IIOServerNotification	*mNotification;

	Allocator::IAllocator	*mAllocator;
	Reporter::IReporter		*mReporter;

	std::string				mDebugName;
	volatile LONG mRef;

public:

	// Thread (wait) process
	DWORD	ThreadProcessId;
	HANDLE	ThreadHandleProcess;

	enum
	{
		eMAX_THREADS = 4
	};

	IOWorker	*ListOfIOWorker[eMAX_THREADS];

	Utils::CriticalSection	mCriticalSection;

public:

	// AcceptEx thread process

	LPFN_ACCEPTEX	lpfnAcceptEx;

	DWORD		ThreadAcceptExId;
	HANDLE		ThreadHandleAcceptEx;
	HANDLE		AcceptExThreadMore;
//	HANDLE		AcceptExThreadDone;

public:

	// Clean stalled sockets thread
	DWORD	ThreadCleanSocketId;
	HANDLE	ThreadHandleCleanSocket;
	long	CleanCycle; // Time before running another cleaning process, in msec
	DWORD	CleanTimeOut; // Time Idle sockets are considered staled in msec
	HANDLE	CleanSocketThreadDone;

	Utils::TLIST<IOConnection>	*mClientList;

	unsigned short	mPort;

public:

	IOServer(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, IIOServerNotification *notificationobject, unsigned short port, PROTOCOL::Enum protocol, const char *debugname);
	~IOServer();
	DWORD	ThreadProcess();
	DWORD	ThreadProcessCleanSockets();

	void	PostAccept();
	bool	PostRead(IOConnection *client);
	bool	OnReceiveData(IOOVERLAPPED *overlap);

	IOConnection	*CreateNewIOClientFromOverlap(IOOVERLAPPED *overlap, const sockaddr_in &addr);
	void				DestroyIOClient(IOConnection *ioclient);

public: // IIOServer

	bool	Init();
	void	Uninit();
	void	Destroy();

public: // IWorkerNotification

	void	OnWorkerAccept(IOOVERLAPPED *overlap);
	void	OnWorkerConnect(IOOVERLAPPED *overlap) {}; // useless for servers, TODO log error
	void	OnWorkerDisconnect(IOOVERLAPPED *overlap);
	void	OnWorkerReceiveData(IOOVERLAPPED *overlap);

	void	Release();
};

} // namespace
