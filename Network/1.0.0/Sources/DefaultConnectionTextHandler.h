#pragma once

namespace Network {

// -------------------------------------------------------------------------------
class DefaultConnectionTextHandler : public IIOClientNotification
{
private:

	enum {MAX_BUFFER_RECEIVE = 10 * 1024,};

	char		mReceiveBuffer[MAX_BUFFER_RECEIVE];
	DWORD		mReceiveSize;

	void	RemoveData(unsigned int size);
	bool	AddData(IIOConnection *connection, const char *data, unsigned int size);
	bool	ProcessData(IIOConnection *connection);

	IIOClientNotification	*mNotification;

public:

	DefaultConnectionTextHandler(IIOClientNotification *notification);
	~DefaultConnectionTextHandler();

public: // IIOClientNotification

	void	OnConnecting(IIOConnection *connection);
	void	OnConnected(IIOConnection *connection);
	IIOClientNotification::eRETURNCODE	OnReceiveData(IIOConnection *connection, const char *data, unsigned int size);
	void	OnFailToConnect(IIOConnection *connection);
	void	OnDisconnect(IIOConnection *connection);
};

// -------------------------------------------------------------------------------
class TextLineProtocol : public Extension::ITextLineProtocol
{
private:

	Allocator::IAllocator	*mAllocator;

	enum {MAX_BUFFER_RECEIVE = 10 * 1024,};

	char		mReceiveBuffer[MAX_BUFFER_RECEIVE];
	DWORD		mReceiveSize;

	void	RemoveData(unsigned int size);
	bool	ProcessData();

	Extension::ITextLineProtocolNotification *mNotitifation;

public:

	TextLineProtocol(Allocator::IAllocator *allocator, Reporter::IReporter *reporter, Extension::ITextLineProtocolNotification *notitifation);

public: // Extension::ITextLineProtocol

	bool AddData(const char *data, unsigned int size);
	void Destroy();
	void Reset() {mReceiveSize = 0;};
};

} // namespace
