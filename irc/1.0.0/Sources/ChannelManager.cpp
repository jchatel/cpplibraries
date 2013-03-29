#include "stdafx.h"
#include "ChannelManager.h"
#include "Channel.h"

using namespace IRC;

// ------------------------------------------------------------------------------------------
Channel		*ChannelManager::GetOrCreateChannel(char *channelname)
{
	Utils::Lock lock(mCriticalSection);

	size_t len = strlen(channelname) + 1;

	_strlwr_s(channelname, len);

	std::map<std::string, Channel *>::iterator iter = mChannels.find(channelname);
	if (iter == mChannels.end())
	{
		Channel *channel = MEM_ALLOC(mAllocator, Channel, "Channel")(channelname);
		mChannels.insert(std::pair<std::string, Channel *>(channelname, channel));
		return channel;
	}

	return iter->second;
}

// ------------------------------------------------------------------------------------------
ChannelManager::ChannelManager(Allocator::IAllocator *allocator)
	: mAllocator(allocator)
{
}

// ------------------------------------------------------------------------------------------
void	ChannelManager::EnumerateChannels(IChannelNotification *notification)
{
	Utils::Lock lock(mCriticalSection);

	std::map<std::string, Channel *>::iterator iter = mChannels.begin();
	while (iter != mChannels.end())
	{
		notification->OnNewChannel(iter->second);
		++iter;
	}
}

