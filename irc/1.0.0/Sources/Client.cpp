#include "stdafx.h"
#include "Client.h"


namespace IRC {

// ---------------------------------------------------------------------------
DWORD WINAPI ThreadReconnect(LPVOID lpParameter)
{
	Sleep(5000); // client may be deleted in 5 sec...

	ClientPrivate *client = (ClientPrivate *)lpParameter;
	client->ReconnectFromAnotherThread();
	return 0;
}

// ------------------------------------------------------------------------------------------
ChannelPrivate::ChannelPrivate(ClientPrivate *parent, const char *name)
	: Parent(parent), Name(name)
{
}

// ------------------------------------------------------------------------------------------
void	ChannelPrivate::Send(const char *message)
{
	Parent->mConnection->SendTextFormatted("PRIVMSG %s :%s\n", Name.c_str(), message);
}

// -------------------------------------------------------------------------------
const char *ChannelPrivate::GetName()
{
	return Name.c_str();
}

// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
ClientPrivate::ClientPrivate(Helper::CLIENTPARAM &parameters)
	: Server(parameters.ServerAddress), Port(parameters.ServerPort), State(STATE_INVALID)
	, InternalState(COMMSTATE_INVALID), mAllocator(parameters.mAllocator), mReporter(parameters.mReporter)
	, mNotification(parameters.Notification), mNickname(parameters.Nickname), mPassword(parameters.mPassword)
	, mUser(parameters.User), mRealName(parameters.RealName), mShutdown(false)
{
	mOn = false;

	mReconnectionDone = CreateEvent(NULL, true, true, NULL);

	DisconnectedTime = 0;
	TimeWaitToReconnect = 0;

	TimeWaitBase = 3000;
	TimeWaitIncrement = 3000; // add 3 seconds

	mTextProtocol = Network::Extension::Helper::CreateTextLineProtocol(mAllocator, mReporter, this);
	mNetwork = Network::Helper::CreateIOCompletionClient(mAllocator, mReporter, this, Server.c_str(), Port, Network::PROTOCOL::TCP, "IRC/NetworkClient");
}

// ------------------------------------------------------------------------------------------
ClientPrivate::~ClientPrivate()
{
	Network::IIOClient *client = NULL; // to prevent dead lock on disconnect notification coming back to this thread

	{
		Utils::Lock lock(mCriticalSection);

		if (mConnection)
		{
			mConnection->SendTextFormatted("Quit %s\n", mExitMessage.c_str());
		}

		client = mNetwork;

		mNetwork = NULL;

		mNotification = NULL;

		std::map<std::string, ChannelPrivate*>::iterator iter = mChannels.begin();
		while (iter != mChannels.end())
		{
			ChannelPrivate *channel = iter->second;
			MEM_UNALLOC(mAllocator, channel);
			++iter;
		}
	}

	client->Destroy();

	mTextProtocol->Destroy();
	mTextProtocol = NULL;
}

// -------------------------------------------------------------------------------
void	ClientPrivate::Destroy()
{
	MEM_UNALLOC_THIS(mAllocator);
}


// ------------------------------------------------------------------------------------------
IChannel	*ClientPrivate::Join(const char *channelname)
{
	ChannelPrivate *channel = MEM_ALLOC(mAllocator, ChannelPrivate, "CHANNELPRIVATE")(this, channelname);

	std::map<std::string, ChannelPrivate*>::iterator iter = mChannels.find(channelname);
	if (iter == mChannels.end())
	{
		mChannels.insert(std::pair<std::string, ChannelPrivate*>::pair(channelname, channel));
	}

	if (mConnection)
	{
		mConnection->SendTextFormatted("JOIN %s\n", channelname);
	}

	return channel;
}

// -------------------------------------------------------------------------------
void	ClientPrivate::SendPrivateMessage(const char *nickname, const char *message)
{
	mConnection->SendTextFormatted("PRIVMSG %s :%s\n", nickname, message);
}

// ------------------------------------------------------------------------------------------
void	ClientPrivate::SendPrivateMessageFormatted(const char *nickname, const char *message, ...)
{
	char	buffer[4096];

	va_list args;
	va_start(args, message);
	if (_vsnprintf_s(buffer, 4096, message, args) == -1)
	{
		REPORT_ERROR(mReporter, true, "IRC::ClientPrivate: Text too big: can't use SendPrivateMessageFormatted with string bigger than 4k");
	}
	va_end(args);
	buffer[4095] = '\0';

	mConnection->SendTextFormatted("PRIVMSG %s :%s\n", nickname, buffer);
}

// ------------------------------------------------------------------------------------------
IClient::ENUM_IRC_STATE	ClientPrivate::GetState()
{
	return State;
}

// ------------------------------------------------------------------------------------------
Network::IIOClientNotification::eRETURNCODE	ClientPrivate::OnReceiveData(Network::IIOConnection *connection, const char *data, unsigned int size)
{
	if (mTextProtocol->AddData(data, size))
	{
		return Network::IIOClientNotification::eOK;
	}

	return Network::IIOClientNotification::eCLOSE;
}

// ------------------------------------------------------------------------------------------
bool	ClientPrivate::OnLine(char *line, unsigned int size)
{
	char* context = NULL;
	char *prefix = strtok_s(line, " ", &context);
	char *replycode = strtok_s(NULL, " ", &context);

	int code = atoi(replycode);

	if (_stricmp(replycode, "JOIN") == 0)
	{
		char *channelname = strtok_s(NULL, " ", &context);

		_strlwr_s(channelname, strlen(channelname) + 1);

		++channelname;

		char *nickname = strtok_s(prefix + 1, "!", &context);

		std::map<std::string, ChannelPrivate*>::iterator iter = mChannels.find(channelname);
		if (iter == mChannels.end())
		{
			return false; // something went wrong
		}

		if (_stricmp(nickname, mNickname.c_str()) == 0)
		{
			// we joined a channel
			mNotification->OnJoin(iter->second);
		}
	//	else
		{
			// someone else joined one channel, update the users in that channel
			// TODO
		}

		return true;
	}

	if (_stricmp(replycode, "PRIVMSG") == 0)
	{
		prefix++;

		// private message, check who is the target
		char *target = strtok_s(NULL, " ", &context);

		char *message = context + 1;

		if (_stricmp(target, this->mNickname.c_str()) == 0)
		{
			context = NULL;
			char *fromnick = strtok_s(prefix, "!", &context);

			mNotification->OnPrivateMessage(fromnick, message);
		}

		return true;
	}
		
	if (code == 376)
	{ // logged
		mNotification->OnConnect();
		return true;
	}


	return true;
}

// -------------------------------------------------------------------------------
void	ClientPrivate::OnConnected(Network::IIOConnection *connection)
{
	mTextProtocol->Reset();

	mConnection = connection;

	mConnection->SendTextFormatted("PASS %s\n", mPassword.c_str());
	mConnection->SendTextFormatted("NICK %s\n", mNickname.c_str());
	mConnection->SendTextFormatted("USER %s\n", mUser.c_str());

	mOn = true;
}

// -------------------------------------------------------------------------------
void	ClientPrivate::OnConnecting(Network::IIOConnection *connection)
{
	mOn = true;
}

// ------------------------------------------------------------------------------------------
void	ClientPrivate::OnFailToConnect(Network::IIOConnection *connection)
{
	if (mNotification)
	{
		mNotification->OnConnectionFailed();
	}
}

// ------------------------------------------------------------------------------------------
void	ClientPrivate::OnDisconnect(Network::IIOConnection *connection)
{
	Utils::Lock lock(mCriticalSection);

	mConnection = NULL;
	State = STATE_INVALID;
	mOn = false;

	if (mNotification && mNotification->OnDisconnect() == false)
	{
		// destroy in another thread not to dead lock
		CreateThread(NULL, 0, ThreadReconnect, this, 0, NULL);
	}
}

// --------------------------------------------------------------------------------------------------
void	ClientPrivate::ReconnectFromAnotherThread()
{
	Utils::Lock lock(mCriticalSection);

	if (mNetwork != NULL)
	{
		mNetwork->Destroy();
	}

	mNetwork = Network::Helper::CreateIOCompletionClient(mAllocator, mReporter, this, Server.c_str(), Port, Network::PROTOCOL::TCP, "IRC/NetworkClient-Reconnect");
}

// --------------------------------------------------------------------------------------------------
void	ClientPrivate::Reconnect()
{
	mConnection = NULL;

	// destroy in another thread not to dead lock
	CreateThread(NULL, 0, ThreadReconnect, this, 0, NULL);
}


// ------------------------------------------------------------------------------------------
void	ClientPrivate::Process()
{
	/*
	if (IrcServer->IsConnected() == false
		&& IrcServer->IsConnecting() == false)
	{
		if (DisconnectedTime == 0)
			DisconnectedTime = timeGetTime();

		if (timeGetTime() - DisconnectedTime > TimeWaitToReconnect)
		{
			IrcServer->Reconnect();
			InternalState = COMMSTATE_INVALID;
			DisconnectedTime = 0;
			TimeWaitToReconnect += TimeWaitIncrement; // to prevent full throttle reconnect (reconnecting too fast for the servers taste)
		}
	}
	*/
}




} // namespace