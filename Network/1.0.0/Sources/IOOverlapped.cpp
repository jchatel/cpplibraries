#include "stdafx.h"
#include "IOConnection.h"
#include "IOOverlapped.h"

using namespace Network;

// -------------------------------------------------------------------------------
IOOVERLAPPED::IOOVERLAPPED(Allocator::IAllocator *allocator, IOOPCODE opcode, IOConnection *client)
: mOpCode(opcode), mIOConnection(client), mIOBuffer(NULL), mSocket(0), mAllocator(allocator)
{
	hEvent = NULL;
	Pointer = 0;
	InternalHigh = 0;
	Internal = 0;

	if (mIOConnection)
	{
		int i = mIOConnection->Acquire();
		if (i == 1 && opcode != IOACCEPT)
		{
//			OUTPUT("Error refcount");
		}
	}
}

// -------------------------------------------------------------------------------
IOOVERLAPPED::~IOOVERLAPPED()
{
	if (mIOConnection)
	{
		mIOConnection->Release();
		mIOConnection = NULL;
	}
	if (mIOBuffer)
	{
		MEM_UNALLOC(mAllocator, mIOBuffer);
		mIOBuffer = NULL;
	}
}

// -------------------------------------------------------------------------------
void IOOVERLAPPED::Destroy()
{
	this->~IOOVERLAPPED();
	mAllocator->Free(this);
}



