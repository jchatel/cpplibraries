#pragma once

namespace IRC {


class Channel;


// ------------------------------------------------------------------------------------------
class IUserNotification
{
public:

	virtual ~IUserNotification() {};

	virtual void	OnPrivateMessage(const char *fromtag, const char *line) = 0;
	virtual void	OnChatLine(Channel *channel, const char *tag, const char *line) = 0;
	virtual void	OnUserJoin(Channel *channel, const char *tag) = 0;
	virtual void	OnUserPart(Channel *channel, const char *tag) = 0;
	virtual void	OnEnumerateUsers(Channel *channel) = 0;
};


// ------------------------------------------------------------------------------------------
class User
{
public:

	Utils::CriticalSection	mCriticalSection;

	std::string			mName;
	std::string			mTag;
	std::string		mUserName;
	std::string		mHost;
	std::string		mRealName;

	IUserNotification	*mNotification;

	std::list<Channel*>	mChannels;

public:

	User(const char *name, IUserNotification *notification);
	~User();

	const char *GetNick() { return mName.c_str();};

	void SetTag(const char *tag) {mTag = tag;};
	const char *GetTag() {return mTag.c_str();};

	void	OnPrivateMessage(const char *fromtag, const char *line);

	void	AddedToChannel(Channel *channel);
	void	RemovedFromChannel(Channel *channel);
	void	LeaveAllChannels();

	const char *GetUserName() {return mUserName.c_str();};
	const char *GetHost() {return mHost.c_str();};
	const char *GetRealName() {return mRealName.c_str();};

	void OnChatLine(Channel *channel, const char *tag, const char *line);
	void OnUserJoin(Channel *channel, const char *tag);
	void OnUserPart(Channel *channel, const char *tag);
};


} // namespace




