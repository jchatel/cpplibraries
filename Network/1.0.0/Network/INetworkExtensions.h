#pragma once

namespace Allocator {
	class IAllocator;
}

namespace Reporter {
	class IReporter;
}

namespace Network {
namespace Extension {


	// -------------------------------------------------------------------------------
	class ITextLineProtocolNotification
	{
	public:
		virtual ~ITextLineProtocolNotification() {};

		virtual bool OnLine(char *line, unsigned int size) = 0;
	};

	// -------------------------------------------------------------------------------
	class ITextLineProtocol
	{
	public:
		virtual ~ITextLineProtocol() {};

		virtual bool AddData(const char *data, unsigned int size) = 0;

		virtual void Destroy() = 0;

		virtual void Reset() = 0;
	};

	namespace Helper {

		ITextLineProtocol * __fastcall CreateTextLineProtocol(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, ITextLineProtocolNotification *notification);

	}


} } // namespace
