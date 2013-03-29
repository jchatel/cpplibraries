#pragma once


namespace IRC {

class User;
class ChannelManager;

// ------------------------------------------------------------------------------------------
class UserManager
{
public:

private:

	Allocator::IAllocator	*mAllocator;
	Utils::CriticalSection	mCriticalSection;
	std::map<std::string, User *> mUsers;
	ChannelManager *mChannelManager;

public:

	UserManager(Allocator::IAllocator *allocator, ChannelManager *channelmanager);

	User	*GetUser(const char *nickname);
	bool	AddUser(User *user);
	bool	RemoveUser(User *user);

	bool	SendUserMessage(User *from, const char *nickname, const char *text);
};


} // namespace




