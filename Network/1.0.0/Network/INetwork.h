#pragma once

#include <winsock2.h>
#include <Ws2tcpip.h.>
#include <Mswsock.h>

namespace Allocator {
	class IAllocator;
}

namespace Reporter {
	class IReporter;
}

namespace Network {

	// -------------------------------------------------------------------------------
	class	IIOConnection
	{
	public:

		virtual ~IIOConnection() {};

		virtual void			SendData(void *data, unsigned int size) = 0;
		virtual void			SendText(const char *text) = 0;
		virtual void			SendTextFormatted(const char *text, ...) = 0;
		virtual volatile LONG	GetIdClient() = 0;

		virtual void			SetUserData(void *data) = 0;
		virtual void			*GetUserData() = 0;

		virtual sockaddr_in		&GetSockAddr() = 0;
		virtual const char		*GetResolvedName() = 0;

		virtual void			CloseSocket() = 0;
	};

	// -------------------------------------------------------------------------------
	class	IIOServerNotification
	{
	public:

		virtual ~IIOServerNotification() {};

		enum	eRETURNCODE
		{
			eOK,
			eCLOSE,
		};

		virtual void		OnConnectionOpened(IIOConnection *ioconnection) = 0;
		virtual eRETURNCODE	OnReceiveData(IIOConnection *iconnection, const char *data, unsigned int size) = 0;
		virtual void		OnConnectionClosed(IIOConnection *ioconnection) = 0;
	};

	// -------------------------------------------------------
	namespace PROTOCOL
	{
		enum Enum
		{
			TCP = 0,
			UDP = 1,
		};
	}

	// -------------------------------------------------------------------------------
	class IIOServer
	{
	public:
		virtual void Destroy() = 0;

	protected:
		virtual ~IIOServer() {};
	};


	// -------------------------------------------------------------------------------
	class IIOClientNotification
	{
	public:

		virtual ~IIOClientNotification() {};

		enum	eRETURNCODE
		{
			eOK,
			eCLOSE,
		};

		virtual void	OnConnecting(IIOConnection *connection) = 0;
		virtual void	OnConnected(IIOConnection *connection) = 0;
		virtual eRETURNCODE	OnReceiveData(IIOConnection *connection, const char *data, unsigned int size) = 0;
		virtual void	OnFailToConnect(IIOConnection *connection) = 0;
		virtual void	OnDisconnect(IIOConnection *connection) = 0;
	};

	// -------------------------------------------------------------------------------
	class IIOClient
	{
	public:
		virtual ~IIOClient() {};
		virtual void Destroy() = 0;
	};

	// -------------------------------------------------------------------------------
	namespace Helper
	{
		IIOServer *__fastcall CreateIOCompletionServer(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, IIOServerNotification *notification, unsigned short port, PROTOCOL::Enum protocol, const char *debugname = "?");
		IIOClient *__fastcall CreateIOCompletionClient(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, IIOClientNotification *notification, const char *address, unsigned short port, PROTOCOL::Enum protocol, const char *debugname = "?");

		IIOClientNotification	*__fastcall CreateDefaultConnectionTextHandler(Allocator::IAllocator *allocator, IIOClientNotification *notification);

		IN_ADDR __fastcall GetIp4FromHost(const char *hostaddress);
	}

	// -------------------------------------------------------------------------------
	namespace Debug
	{
		volatile long __fastcall GetNumberOfWorkerThreads();
		volatile long __fastcall GetNumberOfActiveWorkerThreads();
	}



} // namespace
