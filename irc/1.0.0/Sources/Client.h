#pragma once

namespace IRC {

class ClientPrivate;

// ------------------------------------------------------------------------------------------
class ChannelPrivate : public IChannel
{
public:

	ClientPrivate	*Parent;
	std::string		Name;

public:

	ChannelPrivate(ClientPrivate *parent, const char *name);

	void	Send(const char *message);

	const char *GetName();
};



// ------------------------------------------------------------------------------------------
class ClientPrivate : public IClient, public Network::IIOClientNotification, public Network::Extension::ITextLineProtocolNotification
{
public:

	enum COMMSTATE
	{
		COMMSTATE_INVALID = -1,
		COMMSTATE_NICK,
		COMMSTATE_LOOP,
		COMMSTATE_REGISTERED,
	};

	COMMSTATE		InternalState;

	std::string		Server;
	unsigned short	Port;
	ENUM_IRC_STATE	State;

	Network::IIOClient		*mNetwork;
	Network::IIOConnection	*mConnection;
	Network::Extension::ITextLineProtocol	*mTextProtocol;

	Allocator::IAllocator	*mAllocator;
	Reporter::IReporter		*mReporter;
	IClientNotification	*mNotification;

	std::string		mNickname;
	std::string		mPassword;
	std::string		mRealName;
	std::string		mUser;
	std::string		mTag;

	std::string		mExitMessage;

	bool			mOn;

	std::map<std::string, ChannelPrivate*>	mChannels;

protected:

	DWORD	DisconnectedTime;
	DWORD	TimeWaitToReconnect;
	DWORD	TimeWaitBase;
	DWORD	TimeWaitIncrement;

	Utils::CriticalSection	mCriticalSection;

	HANDLE mReconnectionDone;
	bool mShutdown;

public:

	ClientPrivate(Helper::CLIENTPARAM &parameters);
	~ClientPrivate();

	void			Destroy();

	void			Process();

	IChannel				*Join(const char *channelname);
	IClient::ENUM_IRC_STATE	GetState();
	void					SendPrivateMessage(const char *nickname, const char *message);
	void					SendPrivateMessageFormatted(const char *nickname, const char *message, ...);
	void					SetExitMessage(const char *message) {mExitMessage = message;};

	void	ReconnectFromAnotherThread();
	void	Reconnect();

public:	// IIOClientNotification

	void	OnConnecting(Network::IIOConnection *connection);
	void	OnConnected(Network::IIOConnection *connection);
	Network::IIOClientNotification::eRETURNCODE	OnReceiveData(Network::IIOConnection *connection, const char *data, unsigned int size);
	void	OnFailToConnect(Network::IIOConnection *connection);
	void	OnDisconnect(Network::IIOConnection *connection);

public: // Network::Extension::ITextLineProtocolNotification

	bool	OnLine(char *line, unsigned int size);
};


} // namespace




