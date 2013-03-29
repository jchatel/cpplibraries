#include "stdafx.h"
#include "ClientConnection.h"
#include "Channel.h"
#include "UserManager.h"
#include "User.h"

namespace IRC {


// ------------------------------------------------------------------------------------------
ClientConnection::ClientConnection(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, Network::IIOConnection *connection, ServerInfo &serverinfo, ChannelManager *channelmanager, UserManager *usermanager)
	: mAllocator(allocator), mConnection(connection), mServerInfo(serverinfo), mChannelManager(channelmanager), mUserManager(usermanager), mUser(NULL)
	, mReporter(reporter)
{
	mTextProtocol = Network::Extension::Helper::CreateTextLineProtocol(allocator, mReporter, this);

	const sockaddr_in sock = connection->GetSockAddr();

	char buffer[128];
	char service[128];

	getnameinfo((SOCKADDR *)&sock, sizeof(sock), buffer, 128, service, 128, 0);
	mHost = buffer;

	REPORT_LOG(mReporter, true, "NEW CONNECTION FROM %s", mConnection->GetResolvedName());
}

// ------------------------------------------------------------------------------------------
ClientConnection::~ClientConnection()
{
	if (mUser)
	{
		REPORT_LOG(mReporter, true, "USER %s DISCONNECTED", mUser->GetNick());

		mUserManager->RemoveUser(mUser);
		MEM_UNALLOC(mAllocator, mUser);
	}
	else
	{
		REPORT_LOG(mReporter, true, "CONNECTION %s DISCONNECTED", mConnection->GetResolvedName());
	}

	mTextProtocol->Destroy();
}

// ------------------------------------------------------------------------------------------
bool	ClientConnection::AddData(const char *data, unsigned int size)
{
	return mTextProtocol->AddData(data, size);
}

// ------------------------------------------------------------------------------------------
bool	ClientConnection::OnLine(char *line, unsigned int size)
{
	mReporter->ReportFormatted(Reporter::IReporter::eLOG, __FUNCTION__, __FILE__, __LINE__, "(%s) %s", mNick.c_str(), line);

	//CAP LS
	//PASS abc123
	//NICK Kamron
	//USER JECH 0 * :...
	if (_strnicmp(line, "CAP ", 4) == 0)
	{
		return true; // ignore
	}

	if (_strnicmp(line, "PASS ", 5) == 0)
	{
		// if (already logged in, ti's an illegal command)
		//{
		//mConnection->SendText(:Unauthorized command (already registered)\n");
		// return true;
		// }

		mPassword = line+5;

		if (mPassword.length() == 0)
		{
			mConnection->SendTextFormatted(":%s 461 PASS :Not enough parameters\n", mServerInfo.mServerName.c_str());

			return true;
		}

		return true;
	}

	if (_strnicmp(line, "NICK ", 5) == 0)
	{
		mNick = line+5;

		if (mPassword.length() == 0)
		{
			// FORCE to have a password

			mConnection->SendTextFormatted(":%s 461 PASS :Not enough parameters\n", mServerInfo.mServerName.c_str());

			return false;
		}

		// TODO check validity of nickname (length/chars)

		const char *nick = mNick.c_str();

		if (mUserManager->GetUser(nick) != NULL)
		{ // someone already exists with that nick
			mConnection->SendTextFormatted(":%s 433 %s :Nickname is already in use\n", mServerInfo.mServerName.c_str(), nick);
			return true;
		}

		mUser = MEM_ALLOC(mAllocator, User, "User")(nick, this);
		if (mUserManager->AddUser(mUser) == false)
		{
			MEM_UNALLOC(mAllocator, mUser);
			mConnection->SendTextFormatted(":%s 433 %s :Nickname is already in use\n", mServerInfo.mServerName.c_str(), nick);
			return true;
		}

		return true;
	}

	if (mUser == NULL)
	{
		return true; // ignore command until a nickname is given
	}

	if (_strnicmp(line, "USER ", 5) == 0)
	{
		line += 5;

		// Capture User
		// Parameters: <user> <mode> <unused> <realname>
		char* context = NULL;

		char *username = strtok_s(line, " ", &context);

		mUserName = username;

		char *mode = strtok_s(NULL, " ", &context);

		char *unused = strtok_s(NULL, " ", &context);

		char *realname = strtok_s(NULL, " ", &context);

		// Check if can connect and link with other data (e.g. clearing firm account)

		// Respond welcome
		const char *servername = mServerInfo.mServerName.c_str();

		const char *nick = mNick.c_str();

		mTag = mNick;
		mTag += "!";
		mTag += mUserName;
		mTag += "@";
		mTag += mHost;

		mUser->SetTag(mTag.c_str());

		if (realname)
			mRealName = realname;
		else
			mRealName = mNick;

		//REPORT_LOG(mReporter, true, "NICKNAME %s", nick);

		mConnection->SendTextFormatted(":%s 001 %s :Welcome to the Internet Relay Network %s!%s@%s\n", servername, nick, nick, mUserName.c_str(), mHost.c_str());
		mConnection->SendTextFormatted(":%s 002 %s :Your host is %s, running version %s\n", servername, nick, servername, mServerInfo.mServerVersion.c_str());

		mConnection->SendTextFormatted(":%s 003 %s :This server was created %.4d-%.2d-%.2d at %.2d:%.2d:%.2d\n", servername, nick
											, mServerInfo.mDateCreation.wYear, mServerInfo.mDateCreation.wMonth, mServerInfo.mDateCreation.wDay
											, mServerInfo.mDateCreation.wHour, mServerInfo.mDateCreation.wMinute, mServerInfo.mDateCreation.wSecond);
		// <available user modes> aoOirw
		// <available channel modes> abeiIklmnoOpqrRstv
		mConnection->SendTextFormatted(":%s 004 %s :%s %s aoOirw abeiIklmnoOpqrRstv\n", servername, nick, servername, mServerInfo.mServerVersion.c_str());

		mConnection->SendTextFormatted(":%s 375 %s :- %s\n", servername, nick, servername);
		mConnection->SendTextFormatted(":%s 372 %s :-  _   _   _   _   ____   ____   ____    _   _____   _   _   \n", servername, nick);
		mConnection->SendTextFormatted(":%s 372 %s :- | |_| | | |_| | |    | | ___| |    |  | | |  _  | |  \\| |  \n", servername, nick);
		mConnection->SendTextFormatted(":%s 372 %s :- |  _  |  \\   /  | ___| | _|_  |  _ \\  | | | |_| | | |\\  |  \n", servername, nick);
		mConnection->SendTextFormatted(":%s 372 %s :- |_| |_|   |_|   |_|    |____| |_| \\_| |_| |_____| |_| \\_|  \n", servername, nick);
		mConnection->SendTextFormatted(":%s 372 %s :-                                                            \n", servername, nick);
		mConnection->SendTextFormatted(":%s 372 %s :- --- Welcome to HYPERION ---\n", servername, nick);
			
		mConnection->SendTextFormatted(":%s 376 %s :End of MOTD command\n", servername, nick);

		//if (mBot)
		//{
		//	mBot->OnNewUserConnected(mNick.c_str());
		//}

		return true;
	}

	// check user exist at this stage, or don't agree on executing other commands

	if (_strnicmp(line, "MODE ", 5) == 0)
	{
		line += 5;

		// don't care :)
		mConnection->SendTextFormatted(":%s MODE %s :%s\n", mNick.c_str(), mNick.c_str(), line);

		return true;
	}


	if (_strnicmp(line, "JOIN ", 5) == 0)
	{
		line += 5;

		// may be joining multiple chan at once
		char* context = NULL;

		char *channelname = strtok_s(line, ",", &context);

		if (channelname != NULL && strlen(channelname) == 1 && channelname[0] == '0')
		{
			// leave all channels
			mUser->LeaveAllChannels();

			return true;
		}

		while (channelname != NULL)
		{
			// Add user to this channel

			Channel *chan = mChannelManager->GetOrCreateChannel(channelname);

			if (chan == NULL)
			{
				// TODO respond error creating channel (illegal name or such)
				return false; // close connection
			}

//			REPORT_LOG(mReporter, true, "%s JOINS %s", mUser->GetNick(), chan->GetName());

			chan->AddUser(mUser); 

			// channel name

			//mConnection->SendTextFormatted(":%s!%s@%s JOIN :%s\n", mNick.c_str(), mUser.c_str(), mHost.c_str(), chan->GetName());

			channelname = strtok_s(NULL, ",", &context);
		}

		return true;
	}

	if (_strnicmp(line, "PRIVMSG ", 8) == 0)
	{
		line += 8;

		char* context = NULL;

		char *name = strtok_s(line, " ", &context);
		++context; // remove the :

		if (name[0] == '#')
		{
			Channel *chan = mChannelManager->GetOrCreateChannel(name);

			if (chan == NULL)
			{
				return false;
			}

			chan->AddLine(mUser, context);
			return true;
		}

		// send to someone private
		mUserManager->SendUserMessage(mUser, name, context);

		return true;
	}

	if (_strnicmp(line, "PART ", 5) == 0)
	{
		line += 5;

		// may be joining multiple chan at once
		char* context = NULL;

		char *channelname = strtok_s(line, ",", &context);

		while (channelname != NULL)
		{
			// Add user to this channel

			Channel *chan = mChannelManager->GetOrCreateChannel(channelname);

			if (chan == NULL)
			{
				// TODO respond error creating channel (illegal name or such)
				return false; // close connection
			}

//			REPORT_LOG(mReporter, true, "%s LEAVES %s", mUser->GetNick(), chan->GetName());

			chan->RemoveUser(mUser); 

			// channel name

//			mConnection->SendTextFormatted(":%s!%s@%s PART :%s\n", mNick.c_str(), mUser.c_str(), mHost.c_str(), chan->GetName());

			channelname = strtok_s(NULL, ",", &context);
		}

		return true;
	}

	if (_strnicmp(line, "LIST", 4) == 0)
	{
		mConnection->SendTextFormatted(":%s 321 %s Channel :Users Name\n", mServerInfo.mServerName.c_str(), mNick.c_str());

		// Enumerate channels here
		mChannelManager->EnumerateChannels(this);

//		mConnection->SendTextFormatted(":%s 322 %s #bete 170 :de7,1 §®º°8,1 4,0 WeLComE tO #BeTe 7,1°º®§ 4,0 Galau Radio Streaming @ 13http://70.38.31.198:8989/listen.pls13 Group https://www.facebook.com/groups/RadioGalauers/. Request? Posting di wall/chatbox. rules: remote off for sc berlaku buat Op \n", mServerInfo.mServerName.c_str(), mNick.c_str());

		mConnection->SendTextFormatted(":%s 323 %s :End of /LIST\n", mServerInfo.mServerName.c_str(), mNick.c_str());

		return true;
	}

	if (_strnicmp(line, "PING ", 5) == 0)
	{
		mConnection->SendTextFormatted(":%s PONG %s %s\n", mServerInfo.mServerName.c_str(), mServerInfo.mServerName.c_str(), line+5);
		return true;
	}

	if (_strnicmp(line, "WHOIS ", 6) == 0)
	{
		// TODO list of users

		User *user = mUserManager->GetUser(line + 6);
		if (user)
		{
			mConnection->SendTextFormatted(":%s 311 %s %s %s * :%s\n", mServerInfo.mServerName.c_str(), user->GetNick(), user->GetUserName(), user->GetHost(), user->GetRealName());
		}
		return true;
	}

	return true;  // ignore
}


// ------------------------------------------------------------------------------------------
void	ClientConnection::OnPrivateMessage(const char *fromtag, const char *line)
{
	mConnection->SendTextFormatted(":%s PRIVMSG %s :%s\n", fromtag, mUser->GetNick(), line);
}

// ------------------------------------------------------------------------------------------
void	ClientConnection::OnChatLine(Channel *channel, const char *tag, const char *line)
{
	const char *name = channel->GetName();

	mConnection->SendTextFormatted(":%s PRIVMSG %s :%s\n", tag, name, line);
}

// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
class EnumerateChannelUsers : public IChannelEnumeration
{
public:

	Network::IIOConnection *mConnection;

	std::string	mUsers;
	std::string mServerName;
	std::string mNickName;
	std::string mChannelName;

	EnumerateChannelUsers(Network::IIOConnection *connection, const char *servername, const char *nickname, const char *channelname)
		: mConnection(connection), mServerName(servername), mNickName(nickname), mChannelName(channelname)
	{
	}

	~EnumerateChannelUsers()
	{
		if (mUsers.length() > 0)
		{
			Flush();
		}

		mConnection->SendTextFormatted(":%s 366 %s %s :End of /NAMES list.\n", mServerName.c_str(), mNickName.c_str(), mChannelName.c_str());
	}

	bool OnUser(const char *nickname)
	{
		// check if line too long

		if (mUsers.length() > 400)
		{
			Flush();
		}

		if (mUsers.length() > 0)
		{
			mUsers += " ";
		}

		mUsers += nickname;
		return true;
	}

	void Flush()
	{
		mConnection->SendTextFormatted(":%s 353 %s = %s :%s\n", mServerName.c_str(), mNickName.c_str(), mChannelName.c_str(), mUsers.c_str());
		mUsers.clear();
	}
};

// ------------------------------------------------------------------------------------------
void	ClientConnection::OnUserJoin(Channel *channel, const char *tag)
{
	const char *name = channel->GetName();

	mConnection->SendTextFormatted(":%s JOIN :%s\n", tag, name);
}

// ------------------------------------------------------------------------------------------
void	ClientConnection::OnEnumerateUsers(Channel *channel)
{
	EnumerateChannelUsers enumerate(this->mConnection, mServerInfo.mServerName.c_str(), mUser->GetNick(), channel->GetName());
	channel->EnumerateUsers(&enumerate);
}

// ------------------------------------------------------------------------------------------
void	ClientConnection::OnUserPart(Channel *channel, const char *tag)
{
	const char *name = channel->GetName();

	mConnection->SendTextFormatted(":%s PART :%s\n", tag, name);
}

// ------------------------------------------------------------------------------------------
bool	ClientConnection::OnNewChannel(Channel *channel)
{
	mConnection->SendTextFormatted(":%s 322 %s %s %d :%s\n"
		, mServerInfo.mServerName.c_str(), mNick.c_str(), channel->GetName(), channel->GetTotalUsers(), channel->GetTopic());

	return true;
}





} // namespace