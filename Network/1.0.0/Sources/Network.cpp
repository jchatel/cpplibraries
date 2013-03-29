#include "stdafx.h"
#include "../Network/INetwork.h"
#include "IOServer.h"
#include "IOClient.h"
#include "IOWorker.h"
#include "DefaultConnectionTextHandler.h"


namespace Network {

// -------------------------------------------------------------------------------
namespace Helper {

// -------------------------------------------------------------------------------
IIOServer *__fastcall CreateIOCompletionServer(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, IIOServerNotification *notification, unsigned short port, PROTOCOL::Enum protocol, const char *debugname)
{
	IOServer *server = MEM_ALLOC(allocator, IOServer, "Network::IOServer")(allocator, reporter, notification, port, protocol, debugname);

	if (server->Init() == false)
	{
		server->Destroy();
//		MEM_UNALLOC(allocator, server);
		server = NULL;
	}

	return server;
}

// -------------------------------------------------------------------------------
IIOClient *__fastcall CreateIOCompletionClient(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, IIOClientNotification *notification, const char *address, unsigned short port, PROTOCOL::Enum protocol, const char *debugname)
{
	IOClient *client = MEM_ALLOC(allocator, IOClient, "Network::IOClient")(allocator, reporter, notification, protocol, debugname);

	if (client->Init() == false || client->Connect(address, port) == false)
	{
		MEM_UNALLOC(allocator, client);
		client = NULL;
	}

	return client;
}


// -------------------------------------------------------------------------------
IN_ADDR __fastcall GetIp4FromHost(const char *hostaddress)
{
	IN_ADDR addr;
	memset(&addr, 0, sizeof(IN_ADDR));

	hostent *Hosten = gethostbyname(hostaddress); // server by name

	if (Hosten == NULL)
	{
//		int errcode = WSAGetLastError();
//		REPORT_ERROR(mReporter, true, "Network::IOClient(%s): Cannot resolve address %s (errcode %d)", mDebugName.c_str(), address, errcode);

		return addr;
	}

	memcpy(&(addr),Hosten->h_addr,Hosten->h_length); 
	return addr;
}

// -------------------------------------------------------------------------------
IIOClientNotification	*__fastcall CreateDefaultConnectionTextHandler(Allocator::IAllocator *allocator, IIOClientNotification *notification)
{
	IIOClientNotification *proxynotification = MEM_ALLOC(allocator, DefaultConnectionTextHandler, "Network::DefaultConnectionTextHandler")(notification);

	return proxynotification;
}


} // namespace


// -------------------------------------------------------------------------------
namespace Debug {

// -------------------------------------------------------------------------------
volatile long __fastcall GetNumberOfActiveWorkerThreads()
{
	return IOWorker::NumberOfActiveThreads;
}

// -------------------------------------------------------------------------------
volatile long __fastcall GetNumberOfWorkerThreads()
{
	return IOWorker::NumberOfThreads;
}

} // namespace


}



