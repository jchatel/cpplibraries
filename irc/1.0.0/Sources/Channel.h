#pragma once

namespace IRC {


class User;


// ------------------------------------------------------------------------------------------
class IChannelEnumeration
{
public:

	virtual ~IChannelEnumeration() {};

	virtual bool OnUser(const char *nickname) = 0;
};


// ------------------------------------------------------------------------------------------
class Channel
{
private:

	std::set<User *>		mUsers;

public:

	std::string				mName;
	std::string				mTopic;

	Utils::CriticalSection	mCriticalSection;

public:

	Channel(const char *name);
	~Channel();

	bool OnLine(char *line, unsigned int size);
	const char *GetName();
	int	GetTotalUsers() {return 1;}; // TODO
	const char *GetTopic();

	bool	AddUser(User *user);
	bool	RemoveUser(User *user);

	void	AddLine(User *from, const char *text);

	void	EnumerateUsers(IChannelEnumeration *enumeration);
};


} // namespace




