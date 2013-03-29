#include "stdafx.h"
#include "XML.h"

using namespace XML;


// -------------------------------------------------------------------------------
XMLCreateParam::XMLCreateParam()
: mAllocator(NULL), mReporter(NULL), mStreamReader(NULL)
{
}
	
// ------------------------------------------------------------------------------------------
XMLKEY::XMLKEY()
{
}

// ------------------------------------------------------------------------------------------
XMLKEY::~XMLKEY()
{
}

// ------------------------------------------------------------------------------------------
XMLSECTION::XMLSECTION()
{
}

// ------------------------------------------------------------------------------------------
XMLSECTION::~XMLSECTION()
{
}

// ---------------------------------------------------------------------------------------------
IXML	*__fastcall IXML::Create(XMLCreateParam &param)
{
	if (param.mReporter == NULL)
	{
		OutputDebugString("[Stream] No IREPORT provided, can't create GI::XML object\n");
		return NULL;
	}
	
	if (param.mAllocator == NULL)
	{
		REPORT_ERROR(param.mReporter, true, "[Stream] No IMEMORY provided, can't create GI::XML object");
		return NULL;
	}

	void *memory = param.mAllocator->Allocate(sizeof(XMLPRIVATE), "IXML::Create", __FILE__, __LINE__);

	return new (memory) XMLPRIVATE(param);
}


