#include "stdafx.h"
#include "UserManager.h"
#include "User.h"


using namespace IRC;



// ------------------------------------------------------------------------------------------
UserManager::UserManager(Allocator::IAllocator *allocator, ChannelManager *channelmanager)
	: mAllocator(allocator), mChannelManager(channelmanager)
{
}


// ------------------------------------------------------------------------------------------
User	*UserManager::GetUser(const char *nickname)
{
	Utils::Lock lock(mCriticalSection);

	std::map<std::string, User *>::iterator iter = mUsers.find(nickname);
	if (iter == mUsers.end())
	{
		return NULL;
	}

	return iter->second;
}


// ------------------------------------------------------------------------------------------
bool	UserManager::AddUser(User *user)
{
	Utils::Lock lock(mCriticalSection);

	const char *nickname = user->GetNick();

	std::map<std::string, User *>::iterator iter = mUsers.find(nickname);
	if (iter == mUsers.end())
	{
//		User *user = MEM_ALLOC(mAllocator, User, "User")(nickname);
		mUsers.insert(std::pair<std::string, User *>(nickname, user));
		return true;
	}

	return false; // already exists
}

// ------------------------------------------------------------------------------------------
bool	UserManager::RemoveUser(User *user)
{
	Utils::Lock lock(mCriticalSection);

	std::map<std::string, User *>::iterator iter = mUsers.find(user->GetNick());
	if (iter == mUsers.end())
	{
		return false; // not found
	}

	mUsers.erase(iter);

	user->LeaveAllChannels();

	return true;
}


// ------------------------------------------------------------------------------------------
bool	UserManager::SendUserMessage(User *from, const char *nickname, const char *line)
{
	Utils::Lock lock(mCriticalSection);

	std::map<std::string, User *>::iterator iter = mUsers.find(nickname);
	if (iter == mUsers.end())
	{
		return false; // not found
	}

	User *target = iter->second;

	target->OnPrivateMessage(from->GetTag(), line);

	return true;
}

