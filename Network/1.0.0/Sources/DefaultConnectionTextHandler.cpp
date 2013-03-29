#include "stdafx.h"
#include "DefaultConnectionTextHandler.h"

using namespace Network;
using namespace Extension;


// -------------------------------------------------------------------------------
DefaultConnectionTextHandler::DefaultConnectionTextHandler(IIOClientNotification *notification)
: mReceiveSize(0), mNotification(notification)
{
}

// -------------------------------------------------------------------------------
DefaultConnectionTextHandler::~DefaultConnectionTextHandler()
{
}


// -------------------------------------------------------------------------------
bool	DefaultConnectionTextHandler::AddData(IIOConnection *connection, const char *data, unsigned int size)
{
	if (size + mReceiveSize > MAX_BUFFER_RECEIVE)
	{
		return false; // too much data at once, disconnect
	}

	for (unsigned int i = 0; i < size; ++i)
	{
		if (data[i] == '\r') // filter all \r to avoid 2 return to line characters
		{
			continue;
		}

		mReceiveBuffer[mReceiveSize] = data[i];

		++mReceiveSize;
	}

	return ProcessData(connection);
}

// -------------------------------------------------------------------------------
void	DefaultConnectionTextHandler::RemoveData(unsigned int size)
{
	if (mReceiveSize < size)
	{
		//		OUTPUT("Clear too much the buffer");
		size = mReceiveSize;
	}

	char temp_buff[MAX_BUFFER_RECEIVE];

	mReceiveSize -= size;

	char *p = (char *) mReceiveBuffer + size;

	memcpy(temp_buff, p, mReceiveSize);

	memcpy(mReceiveBuffer, temp_buff, mReceiveSize);
}

// -------------------------------------------------------------------------------
bool	DefaultConnectionTextHandler::ProcessData(IIOConnection *connection)
{
	if (mReceiveSize == 0)
		return true;

	while (true)
	{
		mReceiveBuffer[mReceiveSize] = '\0'; // but end of string data

		char *p = strstr(mReceiveBuffer, "\n");
		if (p)
		{ // treat only end of lines
			*p = 0;

			if (mNotification->OnReceiveData(connection, mReceiveBuffer, (unsigned int)(p - mReceiveBuffer + 1)) == false)
				return false;

			RemoveData((unsigned int)(p - mReceiveBuffer + 1));
			continue;
		}

		break;
	}

	return true;
}

// -------------------------------------------------------------------------------
void	DefaultConnectionTextHandler::OnConnecting(IIOConnection *connection)
{
	mNotification->OnConnecting(connection);
}

// -------------------------------------------------------------------------------
void	DefaultConnectionTextHandler::OnConnected(IIOConnection *connection)
{
	mNotification->OnConnected(connection);
}

// -------------------------------------------------------------------------------
IIOClientNotification::eRETURNCODE	DefaultConnectionTextHandler::OnReceiveData(IIOConnection *connection, const char *data, unsigned int size)
{
	if (AddData(connection, data, size) == false)
	{
		return IIOClientNotification::eCLOSE;
	}

	return IIOClientNotification::eOK;
}

// -------------------------------------------------------------------------------
void	DefaultConnectionTextHandler::OnFailToConnect(IIOConnection *connection)
{
	mNotification->OnFailToConnect(connection);
}

// -------------------------------------------------------------------------------
void	DefaultConnectionTextHandler::OnDisconnect(IIOConnection *connection)
{
	mNotification->OnDisconnect(connection);
}


// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
TextLineProtocol::TextLineProtocol(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, Extension::ITextLineProtocolNotification *notitifation)
	: mNotitifation(notitifation), mReceiveSize(0), mAllocator(allocator)
{
}

// -------------------------------------------------------------------------------
void	TextLineProtocol::Destroy()
{
	MEM_UNALLOC_THIS(mAllocator);
}

// -------------------------------------------------------------------------------
bool	TextLineProtocol::AddData(const char *data, unsigned int size)
{
	if (size + mReceiveSize > MAX_BUFFER_RECEIVE)
	{
		return false; // too much data at once, disconnect
	}

	for (unsigned int i = 0; i < size; ++i)
	{
		if (data[i] == '\r') // filter all \r to avoid 2 return to line characters
		{
			continue;
		}

		mReceiveBuffer[mReceiveSize] = data[i];

		++mReceiveSize;
	}

	return ProcessData();
}

// -------------------------------------------------------------------------------
void	TextLineProtocol::RemoveData(unsigned int size)
{
	if (mReceiveSize < size)
	{
		//		OUTPUT("Clear too much the buffer");
		size = mReceiveSize;
	}

	char temp_buff[MAX_BUFFER_RECEIVE];

	mReceiveSize -= size;

	char *p = (char *) mReceiveBuffer + size;

	memcpy(temp_buff, p, mReceiveSize);

	memcpy(mReceiveBuffer, temp_buff, mReceiveSize);
}

// -------------------------------------------------------------------------------
bool	TextLineProtocol::ProcessData()
{
	if (mReceiveSize == 0)
		return true;

	while (true)
	{
		mReceiveBuffer[mReceiveSize] = '\0'; // but end of string data

		char *p = strstr(mReceiveBuffer, "\n");
		if (p)
		{ // treat only end of lines
			*p = 0;

			if (mNotitifation->OnLine(mReceiveBuffer, (unsigned int)(p - mReceiveBuffer + 1)) == false)
				return false;

			RemoveData((unsigned int)(p - mReceiveBuffer + 1));
			continue;
		}

		break;
	}

	return true;
}


