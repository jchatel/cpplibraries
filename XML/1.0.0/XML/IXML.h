#pragma once

namespace Reporter {
	class IReporter;
}

namespace Allocator {
	class IAllocator;
}

namespace Stream {
	class IRESOURCEMANAGER;
	class IStreamReader;
}


namespace XML {

class IXMLENUMERATE;

// ---------------------------------------------------------------------------------------------
class XMLKEY
{
protected:
	XMLKEY();

public:
	virtual ~XMLKEY();

	virtual const char *GetKeyName() const = 0;
	virtual const char *GetValue() const = 0;
};

// ---------------------------------------------------------------------------------------------
class XMLSECTION
{
protected:
	XMLSECTION();

public:
	virtual ~XMLSECTION();

	virtual long				GetUnicId() = 0;
	virtual const char			*GetSectionName() const = 0;

	virtual const char			*GetAssociatedData() const = 0;

	virtual bool	EnumerateSections(IXMLENUMERATE *enumerate, void *userpointer = NULL) = 0;
	virtual bool	EnumerateKeys(IXMLENUMERATE *enumerate, void *userpointer = NULL) = 0;
};


// -------------------------------------------------------------------------------
class XMLCreateParam
{
public:

	XMLCreateParam();

	Allocator::IAllocator	*mAllocator;
	Reporter::IReporter		*mReporter;
	Stream::IStreamReader	*mStreamReader;
};

// -------------------------------------------------------------------------------------------------
class IXML
{
protected:

	IXML() {}; // (in a later SDK, you may have to go through the XML MANAGER to create an XML object)
	virtual ~IXML() {}; // use Destroy

public:

	static IXML		*__fastcall Create(XMLCreateParam &param);
	virtual void	Destroy() = 0;

	virtual bool	Load() = 0;
	virtual bool	Reload() = 0;
	virtual void	Free() = 0;

	// leave NULL to save on the same name it was loaded with,
	// warning, internal name of the file will change, so if you call reload it will reload the one with the new name
	virtual bool	Save(const char *filename = NULL, ...) = 0; 

	// --- Document manipulation
	virtual long	AddSection(const char *sectionname, long insidesectionid = -1) = 0; // return the Id of that new section
	virtual void	AddKey(long parentsectionid, const char *keyname, const char *value) = 0;

	virtual void	EnumerateSections(IXMLENUMERATE *enumerate, void *userpointer = NULL) = 0;
};

// -------------------------------------------------------------------------------
class IXMLENUMERATE
{
public:
	virtual ~IXMLENUMERATE() {};

	virtual bool	XMLEnumerateSections(XMLSECTION *section, void *userpointer = NULL) {(section);(userpointer);return false;}
	virtual bool	XMLEnumerateKeys(XMLKEY *key, void *userpointer = NULL) {(key);(userpointer);return false;}
};


// -------------------------------------------------------------------------------
namespace Helper
{
	// TODO xml string conversion
}


} // namespace