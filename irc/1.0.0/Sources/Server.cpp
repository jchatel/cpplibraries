#include "stdafx.h"
#include "Server.h"
#include "ClientConnection.h"
#include "Channel.h"
#include "UserManager.h"
#include "User.h"

namespace IRC {



// ------------------------------------------------------------------------------------------
ServerInfo::ServerInfo()
{
	GetSystemTime(&mDateCreation);
}


// ------------------------------------------------------------------------------------------
ServerPrivate::ServerPrivate(Helper::SERVERPARAM &parameters)
	:  mAllocator(parameters.mAllocator), mReporter(parameters.mReporter), mNotification(parameters.mNotification)
{
	mChannelManager = MEM_ALLOC(mAllocator, ChannelManager, "ChannelManager")(parameters.mAllocator);
	mUserManager = MEM_ALLOC(mAllocator, UserManager, "UserManager")(parameters.mAllocator, mChannelManager);

	mServerInfo.mServerName = parameters.mServerName;
	mServerInfo.mServerVersion = parameters.mServerVersion;
	mNetwork = Network::Helper::CreateIOCompletionServer(mAllocator, mReporter, this, parameters.mServerPort, Network::PROTOCOL::TCP, "IRCServer");
}

// ------------------------------------------------------------------------------------------
ServerPrivate::~ServerPrivate()
{
}

// ------------------------------------------------------------------------------------------
void	ServerPrivate::Destroy()
{
	MEM_UNALLOC_THIS(mAllocator);
}

// ------------------------------------------------------------------------------------------
void	ServerPrivate::Process()
{
}

// ------------------------------------------------------------------------------------------
void	ServerPrivate::OnConnectionOpened(Network::IIOConnection *ioconnection)
{
	ClientConnection *client = MEM_ALLOC(mAllocator, ClientConnection, "ClientConnection")(mAllocator, mReporter, ioconnection, mServerInfo, mChannelManager, mUserManager);
	ioconnection->SetUserData(client);
}

// ------------------------------------------------------------------------------------------
Network::IIOServerNotification::eRETURNCODE		ServerPrivate::OnReceiveData(Network::IIOConnection *iconnection, const char *data, unsigned int size)
{
	ClientConnection *client = (ClientConnection *)iconnection->GetUserData();
	if (client == NULL)
	{
		return Network::IIOServerNotification::eCLOSE;
	}

	client->AddData(data, size);

	return Network::IIOServerNotification::eOK;
}

// ------------------------------------------------------------------------------------------
void	ServerPrivate::OnConnectionClosed(Network::IIOConnection *ioconnection)
{
	ClientConnection *client = (ClientConnection *)ioconnection->GetUserData();
	if (client)
	{
		MEM_UNALLOC(mAllocator, client);
		ioconnection->SetUserData(NULL);
	}
}



} // namespace