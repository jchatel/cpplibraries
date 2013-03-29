#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <Windows.h>

#include "Allocator/IAllocator.h"
#include "Reporter/IReporter.h"
#include "Stream/IStream.h"

#include "../../../../../3rdParty/Zlib/1.2.3/zlib.h"


#include "../Compression/ICompression.h"
