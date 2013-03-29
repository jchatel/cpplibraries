#pragma once

#include "Server.h"
#include "User.h"
#include "ChannelManager.h"

namespace IRC {


class Channel;
class UserManager;


// ------------------------------------------------------------------------------------------
class ClientConnection : public Network::Extension::ITextLineProtocolNotification, ChannelManager::IChannelNotification, IUserNotification
{
public:

	Allocator::IAllocator	*mAllocator;
	Reporter::IReporter		*mReporter;
	Network::Extension::ITextLineProtocol	*mTextProtocol;
	Network::IIOConnection					*mConnection;
	ServerInfo	&mServerInfo;
	ChannelManager	*mChannelManager;
	UserManager		*mUserManager;
	User			*mUser;

	std::string		mNick;
	std::string		mUserName;
	std::string		mHost;
	std::string		mPassword;
	std::string		mRealName;
	std::string		mTag;

//	IBot			*mBot;

public:

	ClientConnection(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, Network::IIOConnection *connection, ServerInfo &serverinfo, ChannelManager *channelmanager, UserManager *usermanager);
	~ClientConnection();

	bool AddData(const char *data, unsigned int size);

public: // Network::Extension::ITextLineProtocolNotification

	bool OnLine(char *line, unsigned int size);

public: // ChannelManager::IChannelNotification

	bool OnNewChannel(Channel *channel);

public: // IUserNotification

	void	OnPrivateMessage(const char *fromtag, const char *line);
	void	OnChatLine(Channel *channel, const char *tag, const char *line);
	void	OnEnumerateUsers(Channel *channel);
	void	OnUserJoin(Channel *channel, const char *tag);
	void	OnUserPart(Channel *channel, const char *tag);
};


} // namespace




