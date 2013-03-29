#include "stdafx.h"
#include "IOClient.h"
#include "IOWorker.h"
#include "IOConnection.h"
#include "IOOverlapped.h"

using namespace Network;

// ---------------------------------------------------------------------------------------------
const long KEY_STOP = 0xffffffff;

// ---------------------------------------------------------------------------------------------
IOClient::IOClient(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, IIOClientNotification *notificationobject, PROTOCOL::Enum protocol, const char *debugname)
: mNotification(notificationobject), mPort(0), lpfnConnectEx(NULL), mAllocator(allocator), mReporter(reporter), mDebugName(debugname), mRef(1), mConnection(NULL)
, mThreadCreated(false), mProtocol(protocol)
{
	mAllThreadsDone = CreateEvent(NULL, true, false, NULL);
	mIOCompletionPort = NULL;
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	DWORD dwNumProcessors = systemInfo.dwNumberOfProcessors;
	//	NumberOfThreads = dwNumProcessors * 4;
	NumberOfThreads = eMAX_THREADS;

	for (DWORD x = 0; x < NumberOfThreads; ++x)
	{
		char buffer[128];
		_snprintf_s(buffer, 128, "NetC/W(%d):%s", x, mDebugName.c_str());
		ListOfIOWorker[x] = MEM_ALLOC(mAllocator, IOWorker, "Network::IOClient/IOWorker")(mReporter, this, buffer);
	}
}

// ---------------------------------------------------------------------------------------------
IOClient::~IOClient()
{
	if (mIOCompletionPort)
		CloseHandle(mIOCompletionPort);
	mIOCompletionPort = NULL;

	for (DWORD x = 0; x < NumberOfThreads; ++x)
	{
		MEM_UNALLOC(mAllocator, ListOfIOWorker[x]);
	}
	NumberOfThreads = 0;
	CloseHandle(mAllThreadsDone);
}

// -------------------------------------------------------------------------------
bool	IOClient::Init()
{
	//----------------------------------------
	// Declare and initialize variables
	WSADATA wsaData;

	//----------------------------------------
	// Initialize Winsock
	int iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
	if( iResult != NO_ERROR )
	{
		int errcode = WSAGetLastError();
		REPORT_ERROR(mReporter, true, "Network::IOClient(%s): Error using WSAStartup (%d)", mDebugName.c_str(), errcode);
		return false;
	}

	//----------------------------------------
	// Create a handle for the completion port
	mIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (u_long)0, NumberOfThreads);

	mRef += NumberOfThreads;
	for (DWORD x = 0; x < NumberOfThreads; ++x)
	{
		ListOfIOWorker[x]->SpawnThread(mIOCompletionPort);
	}

	mThreadCreated = true;

	return true;
}


// -------------------------------------------------------------------------------
bool	IOClient::Connect(const char *address, unsigned short port)
{
	mPort = port;
	mAddress = address;

	GUID GuidConnectEx = WSAID_CONNECTEX;

	SOCKET sock;

	if (mProtocol == PROTOCOL::TCP)
	{
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	}
	else
	{
		sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	}

	if (sock == INVALID_SOCKET)
	{
		int errcode = WSAGetLastError();
		REPORT_ERROR(mReporter, true, "Network::IOClient(%s): Error creating the connect socket (%d)", mDebugName.c_str(), errcode);
		return false;
	}

	if (mProtocol == PROTOCOL::UDP)
	{
		int opt = 1;
		int ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &opt, 4);
		if (ret != 0)
		{
			int errcode = WSAGetLastError();
			REPORT_ERROR(mReporter, true, "Network::IOServer(%s): setsockopt SO_REUSEADDR failed (%d)", mDebugName.c_str(), errcode);
			WSACleanup();
			return false;
		}
	}


	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons( 0 );

	if (bind(sock, (SOCKADDR *) &addr, sizeof(addr)) == SOCKET_ERROR)
	{
		int errcode = WSAGetLastError();
		REPORT_ERROR(mReporter, true, "Network::IOClient(%s): Error binding the connect socket (%d)", mDebugName.c_str(), errcode);
		closesocket(sock);
		return false;
	}

	mConnection = MEM_ALLOC(mAllocator, IOConnection, "Network::IOClient/IOConnection")(mAllocator, mReporter, sock, addr, mDebugName.c_str());

	IOOVERLAPPED *newoverlap = MEM_ALLOC(mAllocator, IOOVERLAPPED, "Network::IOClient/IOOVERLAPPED")(mAllocator, IOOVERLAPPED::IOCONNECT, mConnection);
	newoverlap->mSocket = sock;

	// Associate the socket with the completion port
	CreateIoCompletionPort((HANDLE)newoverlap->mSocket, mIOCompletionPort, (u_long)0, 0);

	DWORD dwBytes;
	WSAIoctl(newoverlap->mSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidConnectEx, sizeof(GuidConnectEx), &lpfnConnectEx, sizeof(lpfnConnectEx), &dwBytes, NULL, NULL);

	// Try ConnectEx
	SOCKADDR_IN			sockaddr_in;
	BOOL				bRet;

	sockaddr_in.sin_port = htons(port);

	if (address[0] >= '0' && address[0] <= '9')
	{ // xxx.xxx.xxx.xxx style

		unsigned int addr;
		addr = inet_addr(address); // Convert nnn.nnn address to a usable one

		if (addr==INADDR_NONE)
		{
			int errcode = WSAGetLastError();
			REPORT_ERROR(mReporter, true, "Network::IOClient(%s): Invalid IP address %s (errcode %d)", mDebugName.c_str(), address, errcode);
			mConnection->CloseSocket();
			mConnection->Release();
			mConnection = NULL;
			return false;
		}

		sockaddr_in.sin_family = AF_INET;
		sockaddr_in.sin_addr.s_addr = addr;
	}
	else
	{
		hostent *Hosten = gethostbyname(address); // server by name

		if (Hosten == NULL)
		{
			int errcode = WSAGetLastError();
			REPORT_ERROR(mReporter, true, "Network::IOClient(%s): Cannot resolve address %s (errcode %d)", mDebugName.c_str(), address, errcode);
			mConnection->CloseSocket();
			mConnection->Release();
			mConnection = NULL;
			return false;
		}

		memcpy(&(sockaddr_in.sin_addr),Hosten->h_addr,Hosten->h_length); 
		sockaddr_in.sin_family = Hosten->h_addrtype;
	}



/*
typedef BOOL (PASCAL FAR * LPFN_CONNECTEX) (
	IN SOCKET s,
	IN const struct sockaddr FAR *name,
	IN int namelen,
	IN PVOID lpSendBuffer OPTIONAL,
	IN DWORD dwSendDataLength,
	OUT LPDWORD lpdwBytesSent,
	IN LPOVERLAPPED lpOverlapped
	);
*/

	mNotification->OnConnecting(newoverlap->mIOConnection);

	if (mProtocol == PROTOCOL::TCP)
	{
		dwBytes = 0;
		bRet = lpfnConnectEx(newoverlap->mSocket,
							 (LPSOCKADDR)&sockaddr_in,
							 sizeof(sockaddr_in),
							 NULL,
							 0,
							 NULL,
							 newoverlap);

		int retcode = 0;
		if (bRet == FALSE)
		{
			int errcode = WSAGetLastError();
			if (errcode != WSA_IO_PENDING)
			{
				REPORT_ERROR(mReporter, true, "Network::IOClient(%s): Error connectex on the connect socket (%d)", mDebugName.c_str(), errcode);

				mNotification->OnFailToConnect(newoverlap->mIOConnection);

				newoverlap->Destroy();

				mConnection->CloseSocket();
				mConnection->Release();
				mConnection = NULL;

				return false;
			}
		}
	}
	else
	{
		bRet = WSAConnect(mConnection->mSocket, (LPSOCKADDR)&sockaddr_in, sizeof(sockaddr_in), NULL, NULL, NULL, NULL);
		if (bRet != 0)
		{
			int errcode = WSAGetLastError();
			if (errcode != WSA_IO_PENDING)
			{
				REPORT_ERROR(mReporter, true, "Network::IOClient(%s): Error connect on the connect socket (%d)", mDebugName.c_str(), errcode);

				mNotification->OnFailToConnect(newoverlap->mIOConnection);

				newoverlap->Destroy();

				mConnection->CloseSocket();
				mConnection->Release();
				mConnection = NULL;

				return false;
			}
		}

		memcpy(&mConnection->mSocketAddress, &sockaddr_in, sizeof(sockaddr_in));
	}


	return true;
}


// -------------------------------------------------------------------------------
bool	IOClient::PostRead(IOConnection *client)
{
	if (client->mSocket == INVALID_SOCKET)
		return false; // cancel read, socket is closing

	IOOVERLAPPED *newoverlap = MEM_ALLOC(mAllocator, IOOVERLAPPED, "Network::IOClient/IOOVERLAPPED-PostRead")(mAllocator, IOOVERLAPPED::IOREAD, client);
	newoverlap->mIOBuffer = MEM_ALLOC(mAllocator, IOBUFFER, "Network::IOClient/IOBUFFER-PostRead")(1024);

	DWORD flags = 0;

    WSABUF WinsockBuffer;
	WinsockBuffer.len = newoverlap->mIOBuffer->mSize;
	WinsockBuffer.buf = newoverlap->mIOBuffer->mBuffer;

	DWORD byterecv;
	int ires = WSARecv(client->GetSocket(), &WinsockBuffer, 1, &byterecv, &flags, newoverlap, NULL);
	if (ires == SOCKET_ERROR)
	{
		int errcode = GetLastError();
		if (errcode != ERROR_IO_PENDING)
		{
			// problem reading, close the socket
			REPORT_ERROR(mReporter, true, "Network::IOClient(%s): Error PostRead (%d)", mDebugName.c_str(), errcode);

			client->Shutdown();

//			IOOVERLAPPED *closeoverlap = MEM_ALLOC IOOVERLAPPED(IOOVERLAPPED::IOCLOSE, client);
//			PostQueuedCompletionStatus(mIOCompletionPort, 0, NULL, closeoverlap);

			newoverlap->Destroy();
			return false;
		}
	}

	return true;
}


// -------------------------------------------------------------------------------
bool		IOClient::OnReceiveData(IOOVERLAPPED *overlap)
{
	bool ret = false;
	Utils::Lock lock(mCriticalSection);

	try
	{
		ret = mNotification->OnReceiveData(overlap->mIOConnection, overlap->mIOBuffer->mBuffer, overlap->mIOBuffer->mSize) == IIOClientNotification::eOK;
	}
	catch(...)
	{
		REPORT_ERROR(mReporter, true, "Network::IOClient(%s): Exception inside OnReceiveData", mDebugName.c_str());
		return false;
	}
	return ret;
}


// -------------------------------------------------------------------------------
void	IOClient::Uninit()
{
	// critical section should be locked by function above 

	if (mConnection)
	{
		mConnection->CloseSocket();
		mConnection->Release();
		mConnection = NULL;
		BOOL ret;

		// Release the worker threads...

		for (DWORD i = 0; i < NumberOfThreads; i++)
		{
			ret = PostQueuedCompletionStatus(mIOCompletionPort, 0, KEY_STOP, 0 );
			if (ret == FALSE)
			{
				// error
				DWORD errcode = GetLastError();
				REPORT_ERROR(mReporter, true, "Network::IOClient(%s): PostQueuedCompletionStatus error %d", mDebugName.c_str(), errcode);
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------
void	IOClient::Destroy()
{
	{
		Utils::Lock lock(mCriticalSection); // need to scope it or it will be released AFTER object is destroyed!!!
		Uninit();
	}

	if (mThreadCreated) // meaning more than one thread is activated, there is no race condition as if this counter 
	{
		WaitForSingleObject(mAllThreadsDone, INFINITE);
	}

	Release();
}

// -------------------------------------------------------------------------------
void	IOClient::OnConnected(IOConnection *ioclient)
{
	Utils::Lock lock(mCriticalSection);

	try
	{
		mNotification->OnConnected(ioclient);
	}
	catch(...)
	{
		REPORT_ERROR(mReporter, true, "Network::IOClient(%s): Exception inside OnConnected", mDebugName.c_str());
	}
}

// -------------------------------------------------------------------------------
void	IOClient::OnFailToConnect(IOConnection *ioclient)
{
	Utils::Lock lock(mCriticalSection);

	try
	{
		mNotification->OnFailToConnect(ioclient);
	}
	catch(...)
	{
		REPORT_ERROR(mReporter, true, "Network::IOClient(%s): Exception inside OnFailToConnect", mDebugName.c_str());
	}
}


// -------------------------------------------------------------------------------
void	IOClient::OnWorkerConnect(IOOVERLAPPED *overlap)
{
	// connection happened
	if (PostRead(overlap->mIOConnection))
	{
		OnConnected(overlap->mIOConnection);
	}
	else
	{
		OnFailToConnect(overlap->mIOConnection);
	}
}

// -------------------------------------------------------------------------------
void	IOClient::OnWorkerDisconnect(IOOVERLAPPED *overlap)
{
	Utils::Lock lock(mCriticalSection);

	mNotification->OnDisconnect(overlap->mIOConnection);

	Uninit();
}

// -------------------------------------------------------------------------------
void	IOClient::OnWorkerReceiveData(IOOVERLAPPED *overlap)
{
	if (OnReceiveData(overlap) == false)
	{
		// our client did something wrong, so we disconnect it
		OnWorkerDisconnect(overlap);
	}
	else
	{
		if (PostRead(overlap->mIOConnection) == false)
		{
			// destroy as no overlap event will be called since none are waiting
			OnWorkerDisconnect(overlap);
		}
	}
}

// -------------------------------------------------------------------------------
void	IOClient::Release()
{
	LONG ret = InterlockedDecrement(&mRef);

	if (ret == 0)
	{
		// all workers released and Destroy called
		MEM_UNALLOC_THIS(mAllocator);
	}

	if (ret == 1)
	{
		SetEvent(mAllThreadsDone); // just own thread remaining
	}
}
