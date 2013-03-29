#include "stdafx.h"
#include "DefaultConnectionTextHandler.h"


namespace Network {
namespace Extension {
namespace Helper {

ITextLineProtocol * __fastcall CreateTextLineProtocol(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, ITextLineProtocolNotification *notification)
{
	TextLineProtocol *item = MEM_ALLOC(allocator, TextLineProtocol, "Network::TextLineProtocol")(allocator, reporter, notification);

	return item;
}



} } } // namespace
