#include "stdafx.h"
#include "Channel.h"
#include "User.h"

namespace IRC {


// ------------------------------------------------------------------------------------------
Channel::Channel(const char *name)
	: mName(name)
{
}

// ------------------------------------------------------------------------------------------
Channel::~Channel()
{
}

// ------------------------------------------------------------------------------------------
bool	Channel::OnLine(char *line, unsigned int size)
{
	return true;
}

// ------------------------------------------------------------------------------------------
const char *Channel::GetName()
{
	return mName.c_str();
}

// ------------------------------------------------------------------------------------------
const char *Channel::GetTopic()
{
	return mTopic.c_str();
}

// ------------------------------------------------------------------------------------------
void	Channel::AddLine(User *from, const char *text)
{
	Utils::Lock lock(mCriticalSection);

	const char *tag = from->GetTag();

	std::set<User *>::iterator iter = mUsers.begin();
	while (iter != mUsers.end())
	{
		if (from != (*iter))
		{
			(*iter)->OnChatLine(this, tag, text);
		}

		++iter;
	}
}

// ------------------------------------------------------------------------------------------
bool	Channel::AddUser(User *user)
{
	Utils::Lock lock(mCriticalSection);

	mUsers.insert(user);

	const char *tag = user->GetTag();

	std::set<User *>::iterator iter = mUsers.begin();
	while (iter != mUsers.end())
	{
		(*iter)->OnUserJoin(this, tag);

		++iter;
	}

	user->AddedToChannel(this);

	return true; // TODO check not already in
}

// ------------------------------------------------------------------------------------------
bool	Channel::RemoveUser(User *user)
{
	Utils::Lock lock(mCriticalSection);

	const char *tag = user->GetTag();

	std::set<User *>::iterator iter = mUsers.begin();
	while (iter != mUsers.end())
	{
		(*iter)->OnUserPart(this, tag); // TODO quit message

		++iter;
	}

	mUsers.erase(user);

	user->RemovedFromChannel(this);

	return true; // TODO check was in
}


// ------------------------------------------------------------------------------------------
void	Channel::EnumerateUsers(IChannelEnumeration *enumeration)
{
	Utils::Lock lock(mCriticalSection);

	std::set<User *>::iterator iter = mUsers.begin();
	while (iter != mUsers.end())
	{
		enumeration->OnUser((*iter)->GetNick());

		++iter;
	}
}


} // namespace