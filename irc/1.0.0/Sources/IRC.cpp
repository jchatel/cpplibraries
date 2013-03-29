#include "stdafx.h"
#include "Client.h"
#include "Server.h"


namespace IRC {

// -------------------------------------------------------------------------------
namespace Helper {

	// -------------------------------------------------------------------------------
	CLIENTPARAM::CLIENTPARAM()
		: mAllocator(NULL), mReporter(NULL), Notification(NULL), ServerPort(6666)
	{
	}

	// ------------------------------------------------------------------------------------------
	IClient	*__fastcall CreateClient(CLIENTPARAM &parameters)
	{
		if (parameters.mReporter == NULL)
		{
			OutputDebugString("[IIRC] IIRCClient::Create() function requires a valid IREPORT object");
			return NULL;
		}

		if (parameters.mAllocator == NULL)
		{
			REPORT_ERROR(parameters.mReporter, true, "[IIRC] Can't allocate memory for IIRCClient");
			return NULL;
		}

		return MEM_ALLOC(parameters.mAllocator, ClientPrivate, "IRCClientPrivate")(parameters);
	}

	// -------------------------------------------------------------------------------
	SERVERPARAM::SERVERPARAM()
		: mAllocator(NULL), mReporter(NULL), mServerPort(6666)
	{
	}

	// ------------------------------------------------------------------------------------------
	IServer	*__fastcall CreateServer(SERVERPARAM &parameters)
	{
		if (parameters.mReporter == NULL)
		{
			OutputDebugString("[IIRC] IIRCServer::Create() function requires a valid IREPORT object");
			return NULL;
		}

		if (parameters.mAllocator == NULL)
		{
			REPORT_ERROR(parameters.mReporter, true, "[IIRC] Can't allocate memory for IRCServer");
			return NULL;
		}

		return MEM_ALLOC(parameters.mAllocator, ServerPrivate, "IRCServerPrivate")(parameters);
	}

}




} // namespace