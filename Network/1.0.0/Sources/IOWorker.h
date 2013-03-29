#pragma once


namespace Network {

class IOOVERLAPPED;

// -------------------------------------------------------------------------------
class IWorkerNotification
{
public:

	virtual ~IWorkerNotification() {};

	virtual void	OnWorkerAccept(IOOVERLAPPED *overlap) = 0;
	virtual void	OnWorkerConnect(IOOVERLAPPED *overlap) = 0;
	virtual void	OnWorkerDisconnect(IOOVERLAPPED *overlap) = 0;
	virtual void	OnWorkerReceiveData(IOOVERLAPPED *overlap) = 0;
	
	virtual void	Release() = 0;
};


// ---------------------------------------------------------------------------------------------
class IOWorker
{
public:

	static volatile long	NumberOfActiveThreads;
	static volatile long	NumberOfThreads;

	IWorkerNotification		*mNotification;
	HANDLE					mIocp;

	DWORD			ThreadWorkerId;
	HANDLE			ThreadWorker;

	Reporter::IReporter		*mReporter;

	std::string				mWorkerName;

public:

	IOWorker(Reporter::IReporter *reporter, IWorkerNotification *notification, const char *workername);
	~IOWorker();

	void		SpawnThread(HANDLE iocp);

	DWORD		ThreadProcessWorker();
};

} // namespace