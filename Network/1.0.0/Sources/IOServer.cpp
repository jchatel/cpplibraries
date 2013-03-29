#include "stdafx.h"
#include "IOServer.h"
#include "IOWorker.h"
#include "IOConnection.h"
#include "IOOverlapped.h"

using namespace Network;

// ---------------------------------------------------------------------------------------------
const long KEY_STOP = 0xffffffff;


// ---------------------------------------------------------------------------
DWORD WINAPI ThreadIOServerCleanSocket(LPVOID lpParameter)
{
	IOServer *server = (IOServer *)lpParameter;
	return server->ThreadProcessCleanSockets();
}

// ---------------------------------------------------------------------------
DWORD WINAPI ThreadIOServer(LPVOID lpParameter)
{
	IOServer *server = (IOServer *)lpParameter;
	return server->ThreadProcess();
}


// ---------------------------------------------------------------------------------------------
IOServer::IOServer(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, IIOServerNotification *notificationobject, unsigned short port, PROTOCOL::Enum protocol, const char *debugname)
: mNotification(notificationobject), mPort(port), mAllocator(allocator), mReporter(reporter), mDebugName(debugname), mRef(1), ListenSocket(0), mProtocol(protocol)
{
	CloseServer = NULL;
	mIOCompletionPort = NULL;
	ThreadHandleProcess = NULL;
	NumberOfThreads = 0;
	ThreadHandleAcceptEx = NULL;
	AcceptExThreadMore = NULL;
//	AcceptExThreadDone = NULL;
	ThreadHandleCleanSocket = NULL;
	CleanSocketThreadDone = NULL;

	mClientList = Utils::TLIST<IOConnection>::Create(NULL, NULL);
}

// ---------------------------------------------------------------------------------------------
IOServer::~IOServer()
{
	if (CleanSocketThreadDone)
		WaitForSingleObject(CleanSocketThreadDone, INFINITE);

	for (DWORD x = 0; x < NumberOfThreads; ++x)
	{
		if (ListOfIOWorker[x])
		{
			MEM_UNALLOC(mAllocator, ListOfIOWorker[x]);
		}
	}

	CloseHandle(mIOCompletionPort);

	if (CloseServer)
		CloseHandle(CloseServer);
}

// -------------------------------------------------------------------------------
bool	IOServer::Init()
{
	CloseServer = CreateEvent(NULL, TRUE, FALSE, NULL);
	CleanSocketThreadDone = CreateEvent(NULL, TRUE, FALSE, NULL);
//	AcceptExThreadDone = CreateEvent(NULL, TRUE, FALSE, NULL);
	AcceptExThreadMore = CreateEvent(NULL, TRUE, FALSE, NULL);

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
    DWORD dwNumProcessors = systemInfo.dwNumberOfProcessors;
//	NumberOfThreads = dwNumProcessors * 4;
	NumberOfThreads = eMAX_THREADS;

	for (DWORD x = 0; x < NumberOfThreads; ++x)
	{
		ListOfIOWorker[x] = NULL;
	}

	//----------------------------------------
	// Declare and initialize variables
	WSADATA wsaData;
	lpfnAcceptEx = NULL;
	GUID GuidAcceptEx = WSAID_ACCEPTEX;

	//----------------------------------------
	// Initialize Winsock
	int iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
	if( iResult != NO_ERROR )
	{
		int errcode = WSAGetLastError();
		REPORT_ERROR(mReporter, true, "Network::IOServer(%s): Error using WSAStartup (%d)", mDebugName.c_str(), errcode);
		return false;
	}

	//----------------------------------------
	// Create a handle for the completion port
	mIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (u_long)0, NumberOfThreads);
//	gIocp = mIOCompletionPort;

	//----------------------------------------
	// Create a listening socket

	if (mProtocol == PROTOCOL::TCP)
	{
		ListenSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	}
	else
	{
		ListenSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	}

	if (ListenSocket == INVALID_SOCKET)
	{
		int errcode = WSAGetLastError();
		REPORT_ERROR(mReporter, true, "Network::IOServer(%s): Error creating the listening socket (%d)", mDebugName.c_str(), errcode);
		WSACleanup();
		return false;
	}

	if (mProtocol == PROTOCOL::UDP)
	{
		int opt = 1;
		int ret = setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char *) &opt, 4);
		if (ret != 0)
		{
			int errcode = WSAGetLastError();
			REPORT_ERROR(mReporter, true, "Network::IOServer(%s): setsockopt SO_REUSEADDR failed (%d)", mDebugName.c_str(), errcode);
			WSACleanup();
			return false;
		}
	}

	//----------------------------------------
	// Bind the listening socket to the local IP address
	//
	SOCKADDR_IN addr;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons( mPort );

	if (bind(ListenSocket, (SOCKADDR *) &addr, sizeof(addr)) == SOCKET_ERROR)
	{
		int errcode = WSAGetLastError();
		REPORT_ERROR(mReporter, true, "Network::IOServer(%s): Error binding the listening socket (%d)", mDebugName.c_str(), errcode);
		closesocket(ListenSocket);
		return false;
	}

	//----------------------------------------
	// Start listening on the listening socket
	if (mProtocol == PROTOCOL::TCP)
	{
		if (listen( ListenSocket, 100 ) == SOCKET_ERROR)
		{
			int errcode = WSAGetLastError();
			REPORT_ERROR(mReporter, true, "Network::IOServer(%s): Error listening on socket (%d)", mDebugName.c_str(), errcode);
			return false;
		} 
	}

	//----------------------------------------
	// Associate the listening socket with the completion port
	CreateIoCompletionPort((HANDLE)ListenSocket, mIOCompletionPort, (u_long)0, 0);


	if (mProtocol == PROTOCOL::TCP)
	{
		//----------------------------------------
		// Load the AcceptEx function into memory using WSAIoctl.
		// The WSAIoctl function is an extension of the ioctlsocket()
		// function that can use overlapped I/O. The function's 3rd
		// through 6th parameters are input and output buffers where
		// we pass the pointer to our AcceptEx function. This is used
		// so that we can call the AcceptEx function directly, rather
		// than refer to the Mswsock.lib library.
		DWORD dwBytes;
		WSAIoctl(ListenSocket, 
			SIO_GET_EXTENSION_FUNCTION_POINTER, 
			&GuidAcceptEx, 
			sizeof(GuidAcceptEx),
			&lpfnAcceptEx, 
			sizeof(lpfnAcceptEx), 
			&dwBytes, 
			NULL, 
			NULL);

		// Clean Socket function
		CleanCycle = 1000; // 1000 * 60
		CleanTimeOut = 1000 * 60 * 3; // 3 minutes

		// Thread cleaning socket process
		ThreadHandleCleanSocket = CreateThread(NULL, 0, ThreadIOServerCleanSocket, this, 0, &ThreadCleanSocketId);

		// Thread (waiting) process
		ThreadHandleProcess = CreateThread(NULL, 0, ThreadIOServer, this, 0, &ThreadProcessId);
	}

	// --- Create Threads
	mRef += NumberOfThreads;
	for (DWORD x = 0; x < NumberOfThreads; ++x)
	{
		char buffer[128];
		_snprintf_s(buffer, 128, "NetS/W(%d):%s", x, mDebugName.c_str());
		IOWorker *newworker = MEM_ALLOC(mAllocator, IOWorker, "Network::IOServer/IOWorker")(mReporter, this, buffer);
		ListOfIOWorker[x] = newworker;
		newworker->SpawnThread(mIOCompletionPort);
	}

	// post as many accepts as threads
	for (DWORD x = 0; x < NumberOfThreads; ++x)
	{
		if (mProtocol == PROTOCOL::TCP)
		{
			PostAccept();
		}
		else
		{
			PostRead(NULL);
		}
	}
/*
	else
	{
		// Start off an asynchronous read on the socket.  
		//  
		UCHAR achInBuf [1024];
		DWORD nbytes;
		HANDLE g_hReadEvent = CreateEvent(NULL,TRUE,TRUE,NULL);
		OVERLAPPED Overlapped;
		Overlapped.hEvent = g_hReadEvent;
		Overlapped.Internal = 0;
		Overlapped.InternalHigh = 0;
		Overlapped.Offset = 0;
		Overlapped.OffsetHigh = 0;
		BOOL b = ReadFile ((HANDLE)ListenSocket,&achInBuf, sizeof(achInBuf),&nbytes,&Overlapped);
		if (!b && GetLastError () != ERROR_IO_PENDING)
		{
			return FALSE;
		}
	}
	*/

	return true;
}

// -------------------------------------------------------------------------------
void	IOServer::Destroy()
{
	{
		Utils::Lock lock(mCriticalSection); // need to scope it or it will be released AFTER object is destroyed!!!
		Uninit();
	}

	Release();
}

// -------------------------------------------------------------------------------
void	IOServer::PostAccept()
{
	IOOVERLAPPED *newoverlap = MEM_ALLOC(mAllocator, IOOVERLAPPED, "Network::IOServer/IOOVERLAPPED")(mAllocator, IOOVERLAPPED::IOACCEPT);
	newoverlap->mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (newoverlap->mSocket == INVALID_SOCKET)
	{
		int errcode = WSAGetLastError();
		REPORT_ERROR(mReporter, true, "Network::IOServer(%s): Error creating accept socket (%d)", mDebugName.c_str(), errcode);
		newoverlap->Destroy();
		return;
	}

	BOOL ret = lpfnAcceptEx(ListenSocket, newoverlap->mSocket, newoverlap->mAcceptData, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, NULL, newoverlap);

	if (ret == false)
	{
		int code = WSAGetLastError();
		if (code != ERROR_IO_PENDING)
		{
			REPORT_ERROR(mReporter, true, "Network::IOServer(%s): Error acceptex on the accept socket (%d)", mDebugName.c_str(), code);
			// very bad!
			newoverlap->Destroy();
			return;
		}
	}

	struct linger li = { 0, 0 }; // default: SO_DONTLINGER
	li.l_onoff = 1; // SO_LINGER, timeout = 0 (force close now, don't bother sending remaining data)
	setsockopt(newoverlap->mSocket, SOL_SOCKET, SO_LINGER, (char *) &li, sizeof li );

	//socklen_t optlen;
	//int sendbuff;
	//optlen = sizeof(sendbuff);
	//getsockopt(newoverlap->mSocket, SOL_SOCKET, SO_SNDBUF, (char *) &sendbuff, &optlen);
}


// -------------------------------------------------------------------------------
bool	IOServer::PostRead(IOConnection *client)
{
	if (client && client->mSocket == INVALID_SOCKET)
		return false; // cancel read, socket is closing

	IOOVERLAPPED *newoverlap = MEM_ALLOC(mAllocator, IOOVERLAPPED, "Network::IOServer/IOOVERLAPPED")(mAllocator, IOOVERLAPPED::IOREAD, client);

	newoverlap->mIOBuffer = MEM_ALLOC(mAllocator, IOBUFFER, "Network::IOServer/IOOVERLAPPED")(1024);

	DWORD flags = 0;

    WSABUF WinsockBuffer;
	WinsockBuffer.len = newoverlap->mIOBuffer->mSize;
	WinsockBuffer.buf = newoverlap->mIOBuffer->mBuffer;

	DWORD byterecv;

	SOCKET sock;
	if (client)
	{
		sock = client->GetSocket();
	}
	else
	{
		sock = ListenSocket;
	}

	int ires = WSARecv(sock, &WinsockBuffer, 1, &byterecv, &flags, newoverlap, NULL);
	if (ires == SOCKET_ERROR)
	{
		int lasterror = GetLastError();
		if (lasterror != ERROR_IO_PENDING)
		{
			// problem reading, close the socket

			if (client)
			{
				client->Shutdown();
			}

//			IOOVERLAPPED *closeoverlap = MEM_ALLOC(mAllocator, IOOVERLAPPED(IOOVERLAPPED::IOCLOSE, client);
//			PostQueuedCompletionStatus(mIOCompletionPort, 0, NULL, closeoverlap);

			newoverlap->Destroy();
			return false;
		}
	}

	return true;
}


// -------------------------------------------------------------------------------
bool		IOServer::OnReceiveData(IOOVERLAPPED *overlap)
{
	bool ret = false;

	Utils::Lock lock(mCriticalSection);

	try
	{
		ret = mNotification->OnReceiveData(overlap->mIOConnection, overlap->mIOBuffer->mBuffer, overlap->mIOBuffer->mSize) == IIOServerNotification::eOK;
	}
	catch(...)
	{
		REPORT_ERROR(mReporter, true, "Network::IOServer(%s): Exception inside OnReceiveData", mDebugName.c_str());
		return false;
	}

	return ret;
}


// ---------------------------------------------------------------------------------------------
void	IOServer::Uninit()
{
	if (CloseServer)
		SetEvent(CloseServer);

	{
		Utils::Lock lock(mCriticalSection);

		mClientList->RemoveAll();
		mClientList->Destroy();
		mClientList = NULL;
	}

	BOOL ret;

	for (int i = 0; i < eMAX_THREADS; i++)
	{
		ret = PostQueuedCompletionStatus(mIOCompletionPort, 0, KEY_STOP, 0 );
		if (ret == FALSE)
		{
			REPORT_ERROR(mReporter, true, "Network::IOServer(%s): Error on PostQueuedCompletionStatus at exit", mDebugName.c_str());
		}
	}

	shutdown(ListenSocket, SD_BOTH);
	closesocket(ListenSocket);
}

// -------------------------------------------------------------------------------
IOConnection	*IOServer::CreateNewIOClientFromOverlap(IOOVERLAPPED *overlap, const sockaddr_in &addr)
{
	IOConnection *client = MEM_ALLOC(mAllocator, IOConnection, "Network::IOServer/IOConnection")(mAllocator, mReporter, overlap->mSocket, addr, mDebugName.c_str());

	CreateIoCompletionPort((HANDLE)overlap->mSocket, mIOCompletionPort, (ULONG_PTR)0, 0);

	{
		Utils::Lock lock(mCriticalSection);

		mNotification->OnConnectionOpened(client);

		mClientList->AddToLast(client);
	}

	return client;
}

// -------------------------------------------------------------------------------
void	IOServer::DestroyIOClient(IOConnection *ioclient)
{
	{
		Utils::Lock lock(mCriticalSection);
//		ioclient->Shutdown();
		ioclient->CloseSocket();
		mClientList->Remove(ioclient);
		mNotification->OnConnectionClosed(ioclient);
	}

	ioclient->Release();
}


// ---------------------------------------------------------------------------------------------
DWORD	IOServer::ThreadProcessCleanSockets()
{
	Utils::Thread::SetThreadName("Network::CleanIOSocket");

	// Clean inactive sockets for more than 3 minutes
	DWORD ret;
	while (true)
	{
		ret = WaitForSingleObject(CloseServer, CleanCycle);
		if (ret != WAIT_TIMEOUT)
			break; // event set
		
		DWORD now = GetTickCount();

		{
			Utils::Lock lock(mCriticalSection);

			if (mClientList == NULL)
				break;

			Utils::TLISTNODE<IOConnection> *node = mClientList->GetNextNode(NULL);
			Utils::TLISTNODE<IOConnection> *next_node = NULL;
			while (node)
			{
				next_node = mClientList->GetNextNode(node);

				if (node->Data->mLastAccessed != 0
					&& now - node->Data->mLastAccessed > CleanTimeOut)
				{
					node->Data->Shutdown();
//					IOOVERLAPPED *newoverlap = MEM_ALLOC(mAllocator, IOOVERLAPPED(IOOVERLAPPED::IOCLOSE, node->Data);
//					PostQueuedCompletionStatus(mIOCompletionPort, 0, NULL, newoverlap);
				}

				node = next_node;
			}
		} // critical section
	}

	SetEvent(CleanSocketThreadDone);
	return 0;
}



// ---------------------------------------------------------------------------------------------
DWORD	IOServer::ThreadProcess()
{
	Utils::Thread::SetThreadName("Network::ThreadIOServer");

	WaitForSingleObject(CloseServer, INFINITE);
	return 0;
}


// -------------------------------------------------------------------------------
void	IOServer::OnWorkerAccept(IOOVERLAPPED *overlap)
{
	sockaddr_in Local;
	sockaddr_in remote;

	memcpy(&Local, reinterpret_cast<struct sockaddr_in *>(overlap->mAcceptData + 10), sizeof(sockaddr_in));
	memcpy(&remote, reinterpret_cast<struct sockaddr_in *>(overlap->mAcceptData + 38), sizeof(sockaddr_in));

	IOConnection *newioconnection = CreateNewIOClientFromOverlap(overlap, remote); // addref contained
	if (newioconnection)
	{
		if (PostRead(newioconnection) == false)
		{
			DestroyIOClient(newioconnection);
		}
	}

	PostAccept();
}

// -------------------------------------------------------------------------------
void	IOServer::OnWorkerDisconnect(IOOVERLAPPED *overlap)
{
	DestroyIOClient(overlap->mIOConnection);
}


// -------------------------------------------------------------------------------
void	IOServer::OnWorkerReceiveData(IOOVERLAPPED *overlap)
{
	if (OnReceiveData(overlap) == false)
	{
		// our client did something wrong, so we disconnect it
		DestroyIOClient(overlap->mIOConnection);
	}
	else
	{
		if (PostRead(overlap->mIOConnection) == false)
		{
			// destroy as no overlap event will be called since none are waiting
			DestroyIOClient(overlap->mIOConnection);
		}
	}
}

// -------------------------------------------------------------------------------
void	IOServer::Release()
{
	if (InterlockedDecrement(&mRef) == 0)
	{
		// all workers released and Destroy called
		MEM_UNALLOC_THIS(mAllocator);
	}
}
