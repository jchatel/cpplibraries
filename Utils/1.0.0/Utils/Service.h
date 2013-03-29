#pragma once

namespace Utils {

namespace Service {

// -------------------------------------------------------------------------------
class ISERVICENOTIFICATION
{
public:

	virtual void	OnServiceStart() = 0;
	virtual void	OnServiceStop() = 0;
};


// -------------------------------------------------------------------------------
class SERVICE
{
protected:

	static const char				*ServiceName;
	static SERVICE					*Instance;
	static bool ReportStatusToSCMManager(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
	static SERVICE_STATUS			ServiceStatus;
	static SERVICE_STATUS_HANDLE	ServiceStatusHandle;

public:

	static	void	WinMain(DWORD dwArgc, LPTSTR *lpszArgv);
	static	void	Control(DWORD dwCtrlCode);

	HANDLE					hStopEvent, hServiceStoppedEvent;
	ISERVICENOTIFICATION	*Notification;

public:

	SERVICE(const char *servicename, ISERVICENOTIFICATION *notification);
	~SERVICE();

	void	Process(bool debug);

	void	InterruptProcess();

protected:

	void	ServiceStart()
	{
		Notification->OnServiceStart();
	};
	void	ServiceStop()
	{
		Notification->OnServiceStop();
	};

// statics
public:

	static	bool	Startup(char *servicename); // run service
	static	bool	Install(const char *servicename, const char *displayname, const char *dependencies = "");
	static	bool	Uninstall(const char *servicename);
};


} } // namespace