#pragma once

namespace Reporter { class IReporter; }
namespace Allocator { class IAllocator; }

#include <string>

namespace IRC {

	class IChannel;

	// ------------------------------------------------------------------------------------------
	class IClientNotification
	{
	public:
		virtual ~IClientNotification() {};

		virtual void	OnConnect() = 0;
		virtual void	OnJoin(IChannel *channel) {};
		virtual void	OnLeave(IChannel *channel) {};
		virtual	void	OnMessage(IChannel *channel, const char *nickname, const char *message) {};
		virtual void	OnPrivateMessage(const char *fromnickname, const char *message) {};
		virtual	void	OnNotification(const char *notification) {};
		virtual void	OnConnectionFailed() = 0;
		virtual bool	OnDisconnect() {return true;}; // true means accept disconnect, false means try to reconnect
		virtual void	OnChangeNickname(const char *previousnickname, const char *newnickname) {}; // this will be removed once the IRC will manipulate user alone
		virtual void	OnUserLeave(IChannel *channel, const char *nickname) {};

		virtual void	OnServerResponse(const char *line) {}; // for catching every message that the server is sending e.g. for logging/debugging
	};

	// ------------------------------------------------------------------------------------------
	class IServerNotification
	{
	public:
		virtual ~IServerNotification() {};
	};

	// ------------------------------------------------------------------------------------------
	class IChannel
	{
	public:

		virtual ~IChannel() {};

		virtual void		Send(const char *message) = 0;
		virtual const char *GetName() = 0;
	};

	// ------------------------------------------------------------------------------------------
	class IClient
	{
	public:

		enum ENUM_IRC_STATE
		{
			STATE_INVALID = -1,
			STATE_NOT_CONNECTED,
			STATE_CONNECTED,
			STATE_REGISTERED,
		};


	protected:
		IClient() {}; // use Create
		virtual ~IClient() {}; // use Destroy

	public:

		virtual void			Destroy() = 0;

		virtual void			Reconnect() = 0;

		virtual void			Process() = 0; // must be called regularly for updates
		virtual IChannel		*Join(const char *channelname) = 0; // no need to add # in front, e.g. GIHANNEL *chan = myirc->Join("Kamron");

		virtual void			SendPrivateMessage(const char *nickname, const char *message) = 0;
		virtual void			SendPrivateMessageFormatted(const char *nickname, const char *message, ...) = 0;

		virtual void			SetExitMessage(const char *message) = 0;

		virtual ENUM_IRC_STATE	GetState() = 0;
	};

	// ------------------------------------------------------------------------------------------
	class IServer
	{
	public:

	protected:
		IServer() {}; // use Create
		virtual ~IServer() {}; // use Destroy

	public:

		virtual void		Destroy() = 0;

	};

	// -------------------------------------------------------------------------------
	namespace Helper
	{
		// -------------------------------------------------------------------------------
		struct CLIENTPARAM
		{
			CLIENTPARAM();

			const char		*ServerAddress; // e.g. "Oslo2.NO.EU.undernet.org"
			unsigned short	ServerPort; // e.g. 6666
			const char		*Nickname;
			const char		*User;
			const char		*RealName;
			const char		*mPassword;

			Allocator::IAllocator	*mAllocator;
			Reporter::IReporter		*mReporter;
			IClientNotification		*Notification;
		};

		IClient	* __fastcall CreateClient(CLIENTPARAM &parameters);

		// -------------------------------------------------------------------------------
		class SERVERPARAM
		{
		public:
			SERVERPARAM();

			unsigned short	mServerPort; // e.g. 6666

			Allocator::IAllocator	*mAllocator;
			Reporter::IReporter		*mReporter;
			IServerNotification		*mNotification;
			const char				*mServerName; // Maximum 63 chars ( letter / digit ) *( letter / digit / "-" ) *( letter / digit ); as specified in RFC 1123 [HNAME]
			const char				*mServerVersion; // your way to tag your version
		};

		IServer	* __fastcall CreateServer(SERVERPARAM &parameters);
	}



}; // namespace