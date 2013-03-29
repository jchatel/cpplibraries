#pragma once


namespace Utils
{

// -------------------------------------------------------------------------------
template <typename T>
class FastQueue
{
private:

	std::vector<T> mVector[2];

	std::vector<T> *mInput;
	std::vector<T> *mOutput;

	unsigned int mCurrentOutputIndex;

	CriticalSection	mInputSection;
	CriticalSection	mOutputSection;

	HANDLE			mWakeUpEvent;

	volatile LONGLONG	mStatsCurrentInputs;
	volatile LONGLONG	mStatsCurrentOutputs;
	volatile LONGLONG	mStatsTotalProcessed;

public:
	
	// -------------------------------------------------------------------------------
	FastQueue()
	: mCurrentOutputIndex(0), mStatsCurrentInputs(0), mStatsCurrentOutputs(0), mStatsTotalProcessed(0)
	{
		mVector[0].reserve(1000);
		mVector[1].reserve(1000);

		mInput = &mVector[0];
		mOutput = &mVector[1];

		mWakeUpEvent = CreateEvent(NULL, false, false, NULL);
	}

	// -------------------------------------------------------------------------------
	~FastQueue()
	{
		CloseHandle(mWakeUpEvent);
	}

	// -------------------------------------------------------------------------------
	void Clear()
	{
		Lock lock(mOutputSection);
		Lock lock2(mInputSection);

		mInput->resize(0); // empty but don't deallocate memory
		mOutput->resize(0); // empty but don't deallocate memory
		mCurrentOutputIndex = 0;
	}

	// -------------------------------------------------------------------------------
	unsigned int Push(const T &item)
	{
		Lock lock(mInputSection);

		mInput->push_back(item);

		if (mInput->size() == 1)
			SetEvent(mWakeUpEvent);

		size_t size = mInput->size();

		mStatsCurrentInputs = size;

		return size;
	}

	// -------------------------------------------------------------------------------
	void	RetrieveStats(LONGLONG &input, LONGLONG &output, LONGLONG &totalprocessed)
	{
		input = mStatsCurrentInputs;
		output = mStatsCurrentOutputs;
		totalprocessed = mStatsTotalProcessed;
	}

	// -------------------------------------------------------------------------------
	T	Pop()
	{
		Lock lock(mOutputSection);

		if (mCurrentOutputIndex == mOutput->size())
		{
			// swap queues
			{
				mOutput->resize(0); // empty but don't deallocate memory
				Lock lock(mInputSection);
				std::vector<T> *temp = mOutput;
				mOutput = mInput;
				mInput = temp;

				mStatsCurrentInputs = 0;
				mStatsCurrentOutputs = mOutput->size();
			}

			mCurrentOutputIndex = 0;

			if (mOutput->size() == 0)
			{
				WaitForSingleObject(mWakeUpEvent, INFINITE);
				return Pop();
			}

		}

		--mStatsCurrentOutputs;
		++mStatsTotalProcessed;

		return (*mOutput)[mCurrentOutputIndex++];
	}
};


} // namespace
