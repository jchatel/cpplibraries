#pragma once

namespace IRC {

class UserManager;
class ChannelManager;

// ------------------------------------------------------------------------------------------
struct ServerInfo
{
	ServerInfo();

	std::string		mServerName;
	std::string		mServerVersion;
	SYSTEMTIME		mDateCreation;
};

// ------------------------------------------------------------------------------------------
class ServerPrivate : public IServer, public Network::IIOServerNotification
{
public:

	Allocator::IAllocator	*mAllocator;
	Reporter::IReporter		*mReporter;
	IServerNotification		*mNotification;

	Network::IIOServer		*mNetwork;

	ChannelManager			*mChannelManager;
	UserManager				*mUserManager;

	ServerInfo				mServerInfo;

public:

	ServerPrivate(Helper::SERVERPARAM &parameters);
	~ServerPrivate();

	void			Destroy();

	void			Process();

public:	// IIOServerNotification

	Network::IIOServerNotification::eRETURNCODE	OnReceiveData(Network::IIOConnection *iconnection, const char *data, unsigned int size);
	void		OnConnectionOpened(Network::IIOConnection *ioconnection);
	void		OnConnectionClosed(Network::IIOConnection *ioconnection);
};


} // namespace




