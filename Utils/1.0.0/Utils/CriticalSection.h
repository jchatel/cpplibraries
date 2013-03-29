#pragma once

namespace Utils
{

// -------------------------------------------------------------------------------
class CriticalSection
{
private:

	CRITICAL_SECTION	mCriticalSection;

public:
	CriticalSection()
	{
		InitializeCriticalSection(&mCriticalSection);
	}

	~CriticalSection()
	{
		DeleteCriticalSection(&mCriticalSection);
	}

	inline void Lock()
	{
		EnterCriticalSection(&mCriticalSection);
	}

	inline void Unlock()
	{
		LeaveCriticalSection(&mCriticalSection);
	}
};


// -------------------------------------------------------------------------------
class Lock
{
private:
	CriticalSection	*mCriticalSection;

public:

	inline Lock(CriticalSection &criticalsection)
		: mCriticalSection(&criticalsection)
	{
		mCriticalSection->Lock();
	}

	inline ~Lock()
	{
		mCriticalSection->Unlock();
	}
};


}

