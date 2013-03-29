#pragma once


namespace XML {

typedef void (*XMLSTARTELEMENT) (void *userData, const char *section, const char **atts);
typedef void (*XMLENDELEMENT) (void *userData, const char *section);

// ---------------------------------------------------------------------------------------------
class XMLKEYPRIVATE : public XMLKEY
{
public:

	std::string		KeyName;
	std::string		Value;

public:

	void	Save(FILE *file);
	const char *GetKeyName() const;
	const char *GetValue() const;
};

class XMLPRIVATE;

// ---------------------------------------------------------------------------------------------
class XMLSECTIONPRIVATE : public XMLSECTION
{
public:

	static	long				GlobalId;

	long						Id;
	std::string					SectionName;
	Utils::TLIST<XMLKEYPRIVATE>		*ListOfKey;
	Utils::TLIST<XMLSECTIONPRIVATE>	*ListOfSection;
	std::string					AssociatedData;

protected:

	std::vector<XMLSECTION*>	VectorSection; // rebuild with each request (I know it's not ideal at all!);
	std::vector<XMLKEY*>		VectorKey; // rebuild with each request (I know it's not ideal at all!);


public:

	XMLSECTIONPRIVATE(XMLPRIVATE *parent);
	~XMLSECTIONPRIVATE();


	void					Save(FILE *file, long deep);
	void					AddSection(XMLSECTIONPRIVATE *newsection);
	void					AddKey(XMLKEYPRIVATE *key);
	XMLSECTIONPRIVATE		*GetSectionId(long sectionid);

	const char				*GetAssociatedData() const {return AssociatedData.c_str();}

	long					GetUnicId();
	const char				*GetSectionName() const;

	bool	EnumerateSections(IXMLENUMERATE *enumerate, void *userpointer);
	bool	EnumerateKeys(IXMLENUMERATE *enumerate, void *userpointer);
};

// ---------------------------------------------------------------------------------------------
struct XMLUSERDATA
{
	XMLPRIVATE		*This;
	long			Deep;
};

// ---------------------------------------------------------------------------------------------
class XMLPRIVATE : public IXML
{
public:

	std::string					FileName;
	XMLUSERDATA					UserData;
	Utils::TLIST<XMLSECTIONPRIVATE>	*ListOfSection;

	std::vector<XMLSECTIONPRIVATE*>		VectorBuild; // used to build the stack of sections

	XMLCreateParam	Param;

protected:

	std::vector<XMLSECTION*> VectorSection; // rebuild with each request (I know it's not ideal at all!);

	bool				Parse(Stream::IStreamReader *streamread);
	XMLSECTIONPRIVATE	*GetSectionId(long sectionid);

public:

	XMLPRIVATE(XMLCreateParam &param);
	~XMLPRIVATE();

	void	Destroy();

	bool	Load();
	bool	Reload();
	void	Free();
	bool	Save(const char *filename, ...);
	long	AddSection(const char *sectionname, long parentsectionid);
	void	AddKey(long parentsectionid, const char *keyname, const char *value);

	const char	*StringToXML(const char *string);
	const char	*XMLToString(const char *xmlstring);

	inline Allocator::IAllocator	*GetAllocator() {return Param.mAllocator;}

	void	EnumerateSections(IXMLENUMERATE *enumerate, void *userpointer);
};



} // namespace



