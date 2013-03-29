#pragma once

namespace Utils
{

	// -------------------------------------------------------------------------------
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // must be 0x1000

		LPCSTR szName; // pointer to name (in user addr space)

		DWORD dwThreadID; // thread ID (-1=caller thread)

		DWORD dwFlags; // reserved for future use, must be zero

	} THREADNAME_INFO;

	// -------------------------------------------------------------------------------
	class Thread
	{
	public:
		static void SetThreadName(LPCSTR szThreadName, DWORD dwThreadID = -1)
		{
			THREADNAME_INFO info;
			{
				info.dwType = 0x1000;
				info.szName = szThreadName;
				info.dwThreadID = dwThreadID;
				info.dwFlags = 0;
			}
			__try
			{
				RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info );
			}
			__except (EXCEPTION_CONTINUE_EXECUTION)
			{
			}
		}
	};
}

