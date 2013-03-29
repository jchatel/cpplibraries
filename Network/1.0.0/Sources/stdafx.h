#pragma once

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>
#include <Mswsock.h>
#include <Windows.h>

#include <string>

#include "../Network/INetwork.h"
#include "../Network/INetworkExtensions.h"

#include "../../../Allocator/1.0.0/Allocator/IAllocator.h"
#include "../../../Reporter/1.0.0/Reporter/IReporter.h"

#include "../../../Utils/1.0.0/Utils/ThreadName.h"
#include "../../../Utils/1.0.0/Utils/TList.h"
#include "../../../Utils/1.0.0/Utils/CriticalSection.h"

//#ifdef _DEBUG
//	#define LOG_BINARY
//#endif


//#include "../Reporter/IReporter.h"
// #include "Allocator/IAllocator.h"