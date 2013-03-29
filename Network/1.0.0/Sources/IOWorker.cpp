#include "stdafx.h"
#include "IOWorker.h"
#include "IOConnection.h"
#include "IOOverlapped.h"

using namespace Network;

// ---------------------------------------------------------------------------------------------
volatile long	IOWorker::NumberOfActiveThreads = 0;
volatile long	IOWorker::NumberOfThreads = 0;

const long KEY_STOP = 0xffffffff;
const int KELO = 10;

// -------------------------------------------------------------------------------
DWORD WINAPI ThreadIOWorker(LPVOID lpParameter)
{
	IOWorker *worker = (IOWorker *)lpParameter;
	return worker->ThreadProcessWorker();
}


// -------------------------------------------------------------------------------
IOWorker::IOWorker(Reporter::IReporter *reporter, IWorkerNotification *notification, const char *workername)
: mIocp(0), mNotification(notification), mReporter(reporter), ThreadWorker(NULL), mWorkerName(workername)
{
	InterlockedIncrement(&NumberOfThreads);
}

// ---------------------------------------------------------------------------------------------
IOWorker::~IOWorker()
{
	InterlockedDecrement(&NumberOfThreads);

	if (ThreadWorker)
		CloseHandle(ThreadWorker);
	ThreadWorker = NULL;
}

// -------------------------------------------------------------------------------
void	IOWorker::SpawnThread(HANDLE iocp)
{
	mIocp = iocp;
	ThreadWorker = CreateThread(NULL, 0, ThreadIOWorker, this, 0, &ThreadWorkerId);
}

// ---------------------------------------------------------------------------------------------
DWORD	IOWorker::ThreadProcessWorker()
{
	Utils::Thread::SetThreadName(mWorkerName.c_str());

	DWORD		numberofbytes;
	BOOL		result;
	int			ires;
	IOOVERLAPPED	*overlap;
	ULONG_PTR	mylong;

	InterlockedIncrement(&NumberOfActiveThreads);

	// attach to IOCP and start looping
	while (true)
	{
		InterlockedDecrement(&NumberOfActiveThreads);

		result = GetQueuedCompletionStatus(mIocp, &numberofbytes, &mylong, (OVERLAPPED **)&overlap, INFINITE);

		if (mylong == KEY_STOP)
			break; // exit

		if (result == FALSE)
		{
			ires = GetLastError();

			if (overlap == NULL)
			{
				// Really bad, we don't even have the socket to close it, we may want to close all the sockets and reopen if this
				// problem persists
				InterlockedIncrement(&NumberOfActiveThreads);
				Sleep(10);
				continue;
			}

			if (ires != ERROR_SUCCESS)
			{
				// The user may have dropped the connection
				/*
				char errornum[20];
				OUTPUT("[DIFFERENT ERROR] : ");
				sprintf(errornum, "%d", ires);
				OUTPUT(errornum);
				OUTPUT("\n");
				*/
			}

			/*
			// Close socket, something is going wrong on it
			CloseSocket(socketuser);
			MEM_UNALLOC(mAllocator, overlap;
			continue;
			*/
		}

		if (overlap == NULL)
		{
//			LOG("OVERLAP == NULL \n");
			break;
		}

		InterlockedIncrement(&NumberOfActiveThreads);

		// -------------------------
		// CREATE CLIENT CONNECTION
		// -------------------------

		if (overlap->mOpCode == IOOVERLAPPED::IOACCEPT)
		{
			mNotification->OnWorkerAccept(overlap);
		}

		if (overlap->mOpCode == IOOVERLAPPED::IOCONNECT)
		{
			mNotification->OnWorkerConnect(overlap);
		}

		// -------------------------
		// DESTROY CLIENT CONNECTION
		// -------------------------

		if (overlap->mOpCode == IOOVERLAPPED::IOCLOSE)
		{
		}

		// -------------------------------
		// WRITE TO CLIENT CONNECTION DONE
		// -------------------------------
														  
		if (overlap->mOpCode == IOOVERLAPPED::IOWRITE)
		{
		}

		// -------------------------------
		// CLIENT CONNECTION HAS SOME DATA
		// -------------------------------

		if (overlap->mOpCode == IOOVERLAPPED::IOREAD)
		{
			if (numberofbytes == 0)
			{
				// clean disconnection
				mNotification->OnWorkerDisconnect(overlap);
			}
			else
			{
				overlap->mIOBuffer->mSize = numberofbytes;
				mNotification->OnWorkerReceiveData(overlap);
			}
		}

		overlap->mOpCode = IOOVERLAPPED::IODELETED;
		overlap->Destroy();
	}

	mNotification->Release();

	return 0;
}



