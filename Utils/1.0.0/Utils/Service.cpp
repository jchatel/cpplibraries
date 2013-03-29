#include "stdafx.h"
#include "Service.h"

using namespace Utils::Service;


SERVICE *SERVICE::Instance = NULL;
SERVICE_STATUS			SERVICE::ServiceStatus;
SERVICE_STATUS_HANDLE	SERVICE::ServiceStatusHandle;

const char	*SERVICE::ServiceName = NULL;


// -------------------------------------------------------------------------------
SERVICE::SERVICE(const char *servicename, ISERVICENOTIFICATION *notification)
: Notification(notification)
{
	SERVICE::ServiceName = servicename;

	Instance = this;

	hServiceStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}
 
// -------------------------------------------------------------------------------
SERVICE::~SERVICE()
{
	Instance = NULL;

	if (hStopEvent)
	{
		CloseHandle(hStopEvent);
		hStopEvent = NULL;
	}

	if (hServiceStoppedEvent)
	{
		CloseHandle(hServiceStoppedEvent);
		hServiceStoppedEvent = NULL;
	}
}

// -------------------------------------------------------------------------------
void	SERVICE::InterruptProcess()
{
	SetEvent(hStopEvent);

	WaitForSingleObject(hServiceStoppedEvent, INFINITE); // wait for end message
}


// -------------------------------------------------------------------------------
void	SERVICE::Process(bool debug)
{
	ServiceStart();

	if (debug || ReportStatusToSCMManager(SERVICE_RUNNING, NO_ERROR, 0))
	{
		WaitForSingleObject(hStopEvent, INFINITE); // wait for end message
	}

	ServiceStop();

	SetEvent(hServiceStoppedEvent);
}


// -------------------------------------------------------------------------------
bool SERVICE::ReportStatusToSCMManager(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
//    if (bDebug)
//		return true;

	static DWORD dwCheckPoint = 1;

    if (dwCurrentState == SERVICE_START_PENDING)
        ServiceStatus.dwControlsAccepted = 0;
    else
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    ServiceStatus.dwCurrentState = dwCurrentState;
    ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    ServiceStatus.dwWaitHint = dwWaitHint;

    if ( ( dwCurrentState == SERVICE_RUNNING ) ||
         ( dwCurrentState == SERVICE_STOPPED ) )
        ServiceStatus.dwCheckPoint = 0;
    else
        ServiceStatus.dwCheckPoint = dwCheckPoint++;


    if (SetServiceStatus(ServiceStatusHandle, &ServiceStatus) == FALSE)
	{
		return false;
    }

    return true;
}

// -------------------------------------------------------------------------------
VOID WINAPI ServiceControl(DWORD dwCtrlCode)
{
	SERVICE::Control(dwCtrlCode);
}

// -------------------------------------------------------------------------------
void	SERVICE::Control(DWORD dwCtrlCode)
{
    switch(dwCtrlCode)
    {
        case SERVICE_CONTROL_STOP:
            ReportStatusToSCMManager(SERVICE_STOP_PENDING, NO_ERROR, 0);
			if (Instance)
				Instance->InterruptProcess();
			ReportStatusToSCMManager(SERVICE_STOPPED, NO_ERROR, 0);
            return;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        default:
            break;

    }

    ReportStatusToSCMManager(ServiceStatus.dwCurrentState, NO_ERROR, 0);
}

// -------------------------------------------------------------------------------
VOID WINAPI ServiceWinMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	SERVICE::WinMain(dwArgc, lpszArgv);
}

// -------------------------------------------------------------------------------
void	SERVICE::WinMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	UNREFERENCED_PARAMETER(dwArgc);
	UNREFERENCED_PARAMETER(lpszArgv);

	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwServiceSpecificExitCode = 0;

	ServiceStatusHandle = RegisterServiceCtrlHandler( SERVICE::ServiceName, ServiceControl);
	if (ServiceStatusHandle == NULL)
		return;

	if (ReportStatusToSCMManager(SERVICE_START_PENDING, NO_ERROR, 5000) == false)
	{
		ReportStatusToSCMManager(SERVICE_STOPPED, GetLastError(), 0);
	}

	if (Instance)
	{
		Instance->Process(false);
	}

	ReportStatusToSCMManager(SERVICE_STOPPED, GetLastError(), 0);

	return;
}


// -------------------------------------------------------------------------------
bool	SERVICE::Startup(char *servicename)
{
	SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { servicename, ServiceWinMain},
        { NULL, NULL }
    };

	BOOL ret = StartServiceCtrlDispatcher(dispatchTable);

	return ret == TRUE;
}

// -------------------------------------------------------------------------------
bool	SERVICE::Install(const char *servicename, const char *displayname, const char *dependencies)
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    char path[MAX_PATH];

    if ( GetModuleFileName( NULL, path, MAX_PATH ) == 0 )
    {
        return false;
    }

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );

	if (schSCManager == NULL)
	{
		return false;
	}
    
    schService = CreateService(
        schSCManager,               // SCManager database
        servicename,			    // name of service
        displayname,				// name to display
        SERVICE_ALL_ACCESS,         // desired access
        SERVICE_WIN32_OWN_PROCESS,  // service type
        SERVICE_AUTO_START, // DEMAND_START,       // start type
        SERVICE_ERROR_NORMAL,       // error control type
        path,	                    // service's binary
        NULL,                       // no load ordering group
        NULL,                       // no tag identifier
        dependencies,				// dependencies
        NULL,                       // LocalSystem account
        NULL);                      // no password

    if (schService == NULL)
    {
		CloseServiceHandle(schSCManager);
		return false;
	}

	BOOL ret = StartService(schService, 0, NULL);
	UNREFERENCED_PARAMETER(ret);


	CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

	return true;
}

// -------------------------------------------------------------------------------
bool	SERVICE::Uninstall(const char *servicename)
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );

    if (schSCManager == NULL)
	{
		return false;
	}

    schService = OpenService(schSCManager, servicename, SERVICE_ALL_ACCESS);
    if (schService == NULL)
	{
	    CloseServiceHandle(schSCManager);
		return false;
	}

    // try to stop the service
	SERVICE_STATUS ssStatus;
    if (ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus))
    {
        Sleep( 1000 );

        while (QueryServiceStatus( schService, &ssStatus ))
        {
            if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
            {
                Sleep( 1000 );
            }
            else
                break;
        }

    }

	bool ret = true;

    if (DeleteService(schService))
	{
	}
    else
	{
		ret = false;
		DWORD errcode = GetLastError();
		switch (errcode)
		{
			case ERROR_ACCESS_DENIED:
				ret = false;
				break;
			case ERROR_INVALID_HANDLE:
				ret = false;
				break;
			case ERROR_SERVICE_MARKED_FOR_DELETE:
				ret = false;
				break;
		}
	}


    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

	return ret;
}

