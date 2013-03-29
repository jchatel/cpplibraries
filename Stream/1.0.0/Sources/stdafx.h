#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <New.h>

#include <vector>

#include "Allocator/IAllocator.h"
#include "Reporter/IReporter.h"
#include "Utils/TList.h"
#include "Utils/CriticalSection.h"

#include "../Stream/IStream.h"
