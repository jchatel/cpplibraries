#include "stdafx.h"
#include "XML.h"

using namespace XML;

long	XMLSECTIONPRIVATE::GlobalId = 0;

// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
void	XMLKEYPRIVATE::Save(FILE *file)
{
	fputs(KeyName.c_str(), file);
	fputs("=\"", file);
	fputs(Value.c_str(), file);
	fputs("\" ", file);
}

// ---------------------------------------------------------------------------------------------
const char *XMLKEYPRIVATE::GetKeyName() const
{
	return KeyName.c_str();
}

// -------------------------------------------------------------------------------
const char *XMLKEYPRIVATE::GetValue() const
{
	return Value.c_str();
}

// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
XMLSECTIONPRIVATE::XMLSECTIONPRIVATE(XMLPRIVATE *parent)
// : ListOfKey(allocator), ListOfSection(allocator)
{
	Id = GlobalId++;

	ListOfKey = Utils::TLIST<XMLKEYPRIVATE>::Create(parent->Param.mAllocator, parent->Param.mReporter);
	ListOfSection = Utils::TLIST<XMLSECTIONPRIVATE>::Create(parent->Param.mAllocator, parent->Param.mReporter);
}

// ---------------------------------------------------------------------------------------------
XMLSECTIONPRIVATE::~XMLSECTIONPRIVATE()
{
	ListOfSection->DeleteAll();
	ListOfSection->Destroy();

	ListOfKey->DeleteAll();
	ListOfKey->Destroy();
}

// ---------------------------------------------------------------------------------------------
void	XMLSECTIONPRIVATE::AddSection(XMLSECTIONPRIVATE *newsection)
{
	ListOfSection->AddToLast(newsection);
}

// ---------------------------------------------------------------------------------------------
void	XMLSECTIONPRIVATE::AddKey(XMLKEYPRIVATE *key)
{
	ListOfKey->AddToLast(key);
}

// ---------------------------------------------------------------------------------------------
long	XMLSECTIONPRIVATE::GetUnicId()
{
	return Id;
}

// ---------------------------------------------------------------------------------------------
const char	*XMLSECTIONPRIVATE::GetSectionName() const
{
	return SectionName.c_str();
}

/*
// ---------------------------------------------------------------------------------------------
std::vector<XMLSECTION*>	*XMLSECTIONPRIVATE::GetVectorSection()
{
	// build the vector
	VectorSection.clear();

	TLOOP(XMLSECTIONPRIVATE, ListOfSection, node)
	{
		VectorSection.push_back(node->Data);
	}

	return &VectorSection;
}

// ---------------------------------------------------------------------------------------------
std::vector<XMLKEY*>	*XMLSECTIONPRIVATE::GetVectorKey()
{
	// build the vector
	VectorKey.clear();

	TLOOP(XMLKEYPRIVATE, ListOfKey, node)
	{
		VectorKey.push_back(node->Data);
	}

	return &VectorKey;
}
*/

// -------------------------------------------------------------------------------
bool	XMLSECTIONPRIVATE::EnumerateSections(IXMLENUMERATE *enumerate, void *userpointer)
{
	if (enumerate == NULL)
	{
		//REPORT_ERROR(Param.mReporter, true, "[GI::XML] No enumerate object is passed when trying to enumerate");
	}

	TLOOP(XMLSECTIONPRIVATE, ListOfSection, node)
	{
		if (enumerate->XMLEnumerateSections(node->Data, userpointer) == false)
			return false;
	}

	return true;
}

// -------------------------------------------------------------------------------
bool	XMLSECTIONPRIVATE::EnumerateKeys(IXMLENUMERATE *enumerate, void *userpointer)
{
	if (enumerate == NULL)
	{
		//REPORT_ERROR(Param.mReporter, true, "[GI::XML] No enumerate object is passed when trying to enumerate");
	}

	TLOOP(XMLKEYPRIVATE, ListOfKey, node)
	{
		if (enumerate->XMLEnumerateKeys(node->Data, userpointer) == false)
			return false;
	}

	return true;
}

// ---------------------------------------------------------------------------------------------
XMLSECTIONPRIVATE	*XMLSECTIONPRIVATE::GetSectionId(long sectionid)
{
	if (Id == sectionid)
		return this;

	Utils::TLISTNODE<XMLSECTIONPRIVATE> *nodesection = NULL;
	while (nodesection = ListOfSection->GetNextNode(nodesection))
	{
		XMLSECTIONPRIVATE *section = nodesection->Data->GetSectionId(sectionid);
		if (section)
			return section; // found it
	}

	return NULL;
}

// ---------------------------------------------------------------------------------------------
void	XMLSECTIONPRIVATE::Save(FILE *file, long deep)
{
	if (ListOfKey->GetSize() == 0
		&& ListOfSection->GetSize() == 0)
	{
		for (long x = 0; x < deep*4; x++)
			fputs(" ", file);

		fputs("<", file);
		fputs(SectionName.c_str(), file);
		fputs("/>\n", file);
		return;
	}

	long x;
	for (x = 0; x < deep*4; x++)
		fputs(" ", file);

	fputs("<", file);
	fputs(SectionName.c_str(), file);
	fputs(" ", file);

	Utils::TLISTNODE<XMLKEYPRIVATE> *nodekey = NULL;
	while (nodekey = ListOfKey->GetNextNode(nodekey))
	{
		nodekey->Data->Save(file);
	}

	if (ListOfSection->GetSize() == 0)
	{
		fputs("/>\n", file);
		return;
	}

	fputs(">\n", file);

	Utils::TLISTNODE<XMLSECTIONPRIVATE> *nodesection = NULL;
	while (nodesection = ListOfSection->GetNextNode(nodesection))
	{
		nodesection->Data->Save(file, deep+1);
	}

	for (x = 0; x < deep*4; x++)
		fputs(" ", file);
	fputs("</", file);
	fputs(SectionName.c_str(), file);
	fputs(">\n", file);
}

// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
XMLPRIVATE::XMLPRIVATE(XMLCreateParam &param)
{
	Param = param;

	ListOfSection = Utils::TLIST<XMLSECTIONPRIVATE>::Create(param.mAllocator, param.mReporter);

	UserData.This = this;
	Load();
}

// ---------------------------------------------------------------------------------------------
XMLPRIVATE::~XMLPRIVATE()
{
	ListOfSection->DeleteAll();
	ListOfSection->Destroy();
}

// -------------------------------------------------------------------------------
void	XMLPRIVATE::Destroy()
{
	Allocator::IAllocator *allocator = Param.mAllocator;

	this->~XMLPRIVATE();
	allocator->Free(this);
}


// ---------------------------------------------------------------------------------------------
void	XMLPRIVATE::Free()
{
	ListOfSection->DeleteAll();
}

// ---------------------------------------------------------------------------------------------
bool	XMLPRIVATE::Load()
{
	Free();

	if (Param.mStreamReader == NULL)
		return true;

	Param.mStreamReader->Seek(0);

	return Parse(Param.mStreamReader);
}


// ---------------------------------------------------------------------------------------------
bool	XMLPRIVATE::Reload()
{
	if (FileName.size() == 0)
	{
		REPORT_ERROR(Param.mReporter, true, "[Stream] You need to load an XML file first before calling Reload on it");
		return false;
	}

	return Load();
}

// ---------------------------------------------------------------------------------------------
void _cdecl StartElementFunction(void *userData, const char *sectionname, const char **atts)
{ // text is automatically changed back
	XMLUSERDATA *data = (XMLUSERDATA*) userData;

	void *memory = data->This->GetAllocator()->Allocate(sizeof(XMLSECTIONPRIVATE), "GI::XML", __FILE__, __LINE__);
	if (memory == NULL)
		return;
	XMLSECTIONPRIVATE *section = new (memory) XMLSECTIONPRIVATE(data->This);

	section->SectionName = data->This->XMLToString(sectionname);

	if (data->This->VectorBuild.size() == 0)
		data->This->ListOfSection->AddToLast(section);
	else
		data->This->VectorBuild[data->This->VectorBuild.size()-1]->ListOfSection->AddToLast(section);

	data->This->VectorBuild.push_back(section);

	long index = 0;
	while (atts[index] != NULL)
	{
		memory = data->This->GetAllocator()->Allocate(sizeof(XMLKEYPRIVATE), "GI::XML", __FILE__, __LINE__);
		if (memory == NULL)
			return;
		XMLKEYPRIVATE *key = new (memory) XMLKEYPRIVATE;
		key->KeyName = data->This->XMLToString(atts[index]);
		index++;
		key->Value = data->This->XMLToString(atts[index]);
		index++;
		section->ListOfKey->AddToLast(key);
	}

	data->Deep++;
}


// ---------------------------------------------------------------------------------------------
void _cdecl EndElementFunction(void *userData, const char *sectionname)
{
	XMLUSERDATA *data = (XMLUSERDATA*) userData;

	data->Deep--;

	if (data->This->VectorBuild.size() > 0)
		data->This->VectorBuild.pop_back(); // erase last element
	else
	{
		REPORT_ERROR(data->This->Param.mReporter, true, "[GI::XML] XML Badly formatted!!");
	}
}


// ---------------------------------------------------------------------------------------------
void _cdecl CharacterDataHandler(void *userData, const XML_Char *data, int len) // for things in between tags: eg. <tag>something</tag>
{
	XMLUSERDATA *userdata = (XMLUSERDATA*) userData;

	userdata->This->VectorBuild[userdata->This->VectorBuild.size()-1]->AssociatedData.append(data, len);
}


// ---------------------------------------------------------------------------------------------
bool	XMLPRIVATE::Parse(Stream::IStreamReader *streamread)
{
	if (streamread == NULL)
		return NULL;

	UserData.Deep = 0;

	XML_Parser	Parser = XML_ParserCreate(NULL);
	XML_SetUserData(Parser, &UserData);
	XML_SetCharacterDataHandler(Parser, CharacterDataHandler);

	XML_SetElementHandler(Parser, StartElementFunction, EndElementFunction);

	const int BUFFERREAD = 1024;
	char buffer[BUFFERREAD];
	int isfinal = 0;

	do 
	{
		unsigned int len = streamread->Read(buffer, BUFFERREAD);

		isfinal = (len == 0);

		if (!XML_Parse(Parser, buffer, len, isfinal)) 
		{
			REPORT_ERROR(Param.mReporter, true, "[GI::XML] XML parsing error\n(%s at line %d position %d):\n%s", streamread->GetName(), XML_GetCurrentLineNumber(Parser), XML_GetCurrentColumnNumber(Parser), XML_ErrorString(XML_GetErrorCode(Parser)));

			//streamread->Destroy(); // bad, need a proper cleanup
			
			VectorBuild.clear(); // empty

			if (Parser)
				XML_ParserFree(Parser);

			return false;
		}

		if (len == 0)
			break; // done

	} while (true);

	if (Parser)
		XML_ParserFree(Parser);

	Parser = NULL;

	if (VectorBuild.size() > 0)
	{
		REPORT_ERROR(Param.mReporter, true, "[GI::XML] XML not properly closed");
	}

	VectorBuild.clear(); // empty

//	streamread->Destroy(); // bad, need a proper cleanup

	return true;
}



// ---------------------------------------------------------------------------------------------
bool	XMLPRIVATE::Save(const char *filename, ...)
{
	if (filename) // otherwise use the defualt filename
	{
		char buffer[MAX_PATH];
		va_list args;
		va_start(args, filename);
		if (_vsnprintf_s(buffer, MAX_PATH, filename, args) == -1)
		{
			REPORT_ERROR(Param.mReporter, true, "[GI::XML] Internal Buffer Exceeded at Save");
		}
		va_end(args);

		FileName = buffer;
	}

	if (FileName.size() == 0)
	{
		REPORT_ERROR(Param.mReporter, true, "[GI::XML] You need to provide a filename in order to save an XML");
		return false;
	}

	FILE *file = NULL;

	file = fopen(filename, "wt");
	if (file == NULL)
	{
		REPORT_ERROR(Param.mReporter, true, "[GI::XML] XML file could not be open for write: %s", filename);
		return false;
	}

	fputs("<?xml version=\"1.0\"?>\n", file);

	Utils::TLISTNODE<XMLSECTIONPRIVATE> *nodesection = NULL;
	while (nodesection = ListOfSection->GetNextNode(nodesection))
	{
		nodesection->Data->Save(file, 0);
	}

	fclose(file);

	return true;
}

// ---------------------------------------------------------------------------------------------
XMLSECTIONPRIVATE	*XMLPRIVATE::GetSectionId(long sectionid)
{
	if (sectionid < 0)
		return NULL;

	Utils::TLISTNODE<XMLSECTIONPRIVATE> *nodesection = NULL;
	while (nodesection = ListOfSection->GetNextNode(nodesection))
	{
		XMLSECTIONPRIVATE *section = nodesection->Data->GetSectionId(sectionid);
		if (section)
			return section; // found it
	}

	return NULL;
}

// ---------------------------------------------------------------------------------------------
long	XMLPRIVATE::AddSection(const char *sectionname, long parentsectionid)
{
	// Search parent section
	XMLSECTIONPRIVATE *sectionroot = NULL;
	
	if (parentsectionid >= 0)
	{
		sectionroot = GetSectionId(parentsectionid);
		if (sectionroot == NULL)
		{
			REPORT_ERROR(Param.mReporter, true, "[GI::XML] parentsectionid specified does not lead to any existing section.\nDoc will be added to the root instead...\nPlease fix.");
		}
	}

	void *memory = Param.mAllocator->Allocate(sizeof(XMLSECTIONPRIVATE), "GI::XML", __FILE__, __LINE__);
	if (memory == NULL)
		return 0;
	XMLSECTIONPRIVATE *newsection = new (memory) XMLSECTIONPRIVATE(this);

	long id = newsection->Id;
	newsection->SectionName = StringToXML(sectionname);

	if (sectionroot == NULL)
	{
		if (ListOfSection->GetSize() > 0)
		{
			REPORT_ERROR(Param.mReporter, true, "[GI::XML] Only one section is allowed at the root of the document.\nSection added to that root section instead.\nTo fix this error, please create a root section to encapsulate all your sections or specify the section id of another section to add to");
			return id;
		}
		
		ListOfSection->AddToLast(newsection);
		return id;
	}

	sectionroot->AddSection(newsection);

	return id;
}


// ---------------------------------------------------------------------------------------------
void	XMLPRIVATE::AddKey(long parentsectionid, const char *keyname, const char *value)
{
	// Search parent section
	XMLSECTIONPRIVATE *sectionroot = GetSectionId(parentsectionid);

	if (sectionroot == NULL)
	{
		REPORT_ERROR(Param.mReporter, true, "[GI::XML] Invalid parentsectionid.\nCould not found a section with this id.\nKey can't be added");
		return;
	}

	void *memory = Param.mAllocator->Allocate(sizeof(XMLKEYPRIVATE), "GI::XML", __FILE__, __LINE__);
	if (memory == NULL)
		return;
    XMLKEYPRIVATE *newkey = new (memory) XMLKEYPRIVATE;
	newkey->KeyName = StringToXML(keyname);
	newkey->Value = StringToXML(value);
	sectionroot->AddKey(newkey);
}

// ---------------------------------------------------------------------------------------------
const char	*XMLPRIVATE::StringToXML(const char *string)
{
	// < &lt;
	// > &gt;
	// & &amp;
	// " &quot;
	// ' &apos;

	static char buffer[5120];
	const unsigned char *p = (const unsigned char *)string;
	long index = 0;
	while (*p != '\0')
	{
		switch (*p)
		{
			case '<':
				buffer[index++] = '&';
				buffer[index++] = 'l';
				buffer[index++] = 't';
				buffer[index++] = ';';
				break;
			case '>':
				buffer[index++] = '&';
				buffer[index++] = 'g';
				buffer[index++] = 't';
				buffer[index++] = ';';
				break;
			case '&':
				buffer[index++] = '&';
				buffer[index++] = 'a';
				buffer[index++] = 'm';
				buffer[index++] = 'p';
				buffer[index++] = ';';
				break;
			case '\"':
				buffer[index++] = '&';
				buffer[index++] = 'q';
				buffer[index++] = 'u';
				buffer[index++] = 'o';
				buffer[index++] = 't';
				buffer[index++] = ';';
				break;
			case '\'':
				buffer[index++] = '&';
				buffer[index++] = 'a';
				buffer[index++] = 'p';
				buffer[index++] = 'o';
				buffer[index++] = 's';
				buffer[index++] = ';';
				break;
			case '\n':
				buffer[index++] = '&';
				buffer[index++] = '#';
				buffer[index++] = 'x';
				buffer[index++] = 'A';
				buffer[index++] = ';';
				break;
			case '\r':
				buffer[index++] = '&';
				buffer[index++] = '#';
				buffer[index++] = 'x';
				buffer[index++] = 'D';
				buffer[index++] = ';';
				break;
			default:
				{
					if (*p < 128)
					{
						buffer[index++] = *p;
						break;
					}

					// need converting to utf8
					static wchar_t newchar[MAX_PATH];
					int size = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)p, 1, newchar,  MAX_PATH);
					static char newbuffer[MAX_PATH];
					size = WideCharToMultiByte(CP_UTF8, 0, newchar, size, (LPSTR)newbuffer, MAX_PATH, 0, NULL);

					for (long x = 0; x < size; x++)
						buffer[index++] = newbuffer[x];
				}
		}


		p++;
		if (index > 5110)
		{
			REPORT_ERROR(Param.mReporter, true, "[GI::XML] String too long");
		}
	}

	buffer[index] = '\0';

	return buffer;
}

// ---------------------------------------------------------------------------------------------
const char	*XMLPRIVATE::XMLToString(const char *string)
{
	// need converting from utf8

	static char newbuffer[5120];
	
	wchar_t newchar[5120];
	int size = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)string, (int)strlen(string), newchar,  5120);
	size = WideCharToMultiByte(CP_ACP, 0, newchar, size, (LPSTR)newbuffer, 5120, 0, NULL);
	newbuffer[size] = '\0';

	return newbuffer;
}

/*
// ---------------------------------------------------------------------------------------------
void	*XMLPRIVATE::GetListOfSection()
{
	return &ListOfSection;
}
*/

// -------------------------------------------------------------------------------
void	XMLPRIVATE::EnumerateSections(IXMLENUMERATE *enumerate, void *userpointer)
{
	if (enumerate == NULL)
	{
		//REPORT_ERROR(Param.mReporter, true, "[GI::XML] No enumerate object is passed when trying to enumerate");
	}

	TLOOP(XMLSECTIONPRIVATE, ListOfSection, node)
	{
		if (enumerate->XMLEnumerateSections(node->Data, userpointer) == false)
			return;
	}
}

