#include "stdafx.h"
#include "User.h"
#include "Channel.h"

using namespace IRC;


// ------------------------------------------------------------------------------------------
User::User(const char *name, IUserNotification *notification)
	: mName(name), mNotification(notification)
{
}

// ------------------------------------------------------------------------------------------
User::~User()
{
}

// ------------------------------------------------------------------------------------------
void	User::OnPrivateMessage(const char *fromtag, const char *line)
{
	Utils::Lock lock(mCriticalSection);

	mNotification->OnPrivateMessage(fromtag, line);
}


// ------------------------------------------------------------------------------------------
void	User::LeaveAllChannels()
{
	Utils::Lock lock(mCriticalSection);

	std::list<Channel*>::iterator iter = mChannels.begin();

	while (iter != mChannels.end())
	{
		std::list<Channel*>::iterator current_iter = iter;
		iter++;

		(*current_iter)->RemoveUser(this); // this will trigger a remove from the list
	}

	mChannels.clear();
}

// ------------------------------------------------------------------------------------------
void	User::OnChatLine(Channel *channel, const char *tag, const char *line)
{
	Utils::Lock lock(mCriticalSection);

	mNotification->OnChatLine(channel, tag, line);
}

// ------------------------------------------------------------------------------------------
void	User::OnUserJoin(Channel *channel, const char *tag)
{
	Utils::Lock lock(mCriticalSection);

	mNotification->OnUserJoin(channel, tag);
}


// ------------------------------------------------------------------------------------------
void	User::AddedToChannel(Channel *channel)
{
	Utils::Lock lock(mCriticalSection);

	mChannels.push_back(channel);

	mNotification->OnEnumerateUsers(channel);
}

// ------------------------------------------------------------------------------------------
void	User::OnUserPart(Channel *channel, const char *tag)
{
	Utils::Lock lock(mCriticalSection);

	mNotification->OnUserPart(channel, tag);
}

// ------------------------------------------------------------------------------------------
void	User::RemovedFromChannel(Channel *channel)
{
	Utils::Lock lock(mCriticalSection);

	std::list<Channel*>::iterator iter = mChannels.begin();
	while (iter != mChannels.end())
	{
		if ((*iter) == channel)
		{
			mChannels.erase(iter);
			break;
		}

		iter++;
	}
}

