#pragma once

namespace IRC {

class Channel;

// ------------------------------------------------------------------------------------------
class ChannelManager
{
public:

	class IChannelNotification
	{
	public:
		virtual ~IChannelNotification() {};

		virtual bool OnNewChannel(Channel *channel) = 0;
	};

private:

	Allocator::IAllocator	*mAllocator;
	Utils::CriticalSection	mCriticalSection;
	std::map<std::string, Channel *> mChannels;

public:

	ChannelManager(Allocator::IAllocator *allocator);

	Channel *GetOrCreateChannel(char *channelname);
	void	EnumerateChannels(IChannelNotification *notification);

	void	LeaveAllChannels();
};



} // namespace




