#include "fms_cpf_omcmdhandler.h"
#include "fms_cpf_exception.h"
#include "fms_cpf_api_trace.h"
#include "ACS_TRA_trace.h"

#include <set>
namespace Imm_Error
{
	const int NOT_EXIST = -12;
	const int FAILED_OPERATION = -21;
}

FMS_CPF_omCmdHandler::FMS_CPF_omCmdHandler()
{
    numAttr = 0;
    attributes = 0;
    className = 0;
    parentName = 0;
    curNumAttr = 0;
    m_initialized = false;
    initImmOpResult();
    fmsCpfOmHandlerTrace = new ACS_TRA_trace("FMS_CPF_OmCmdHandler");

}

FMS_CPF_omCmdHandler::FMS_CPF_omCmdHandler(int nAttr, const char *pName, const char *cName)
{
    numAttr = nAttr;
    attributes = new (std::nothrow) ACS_CC_ValuesDefinitionType[numAttr];
    className = cName;
    parentName = pName;
    curNumAttr = 0;
    m_initialized = false;
    initImmOpResult();
    fmsCpfOmHandlerTrace = new ACS_TRA_trace("FMS_CPF_OmCmdHandler");
}

// Constructor to modify an attribute of the object in the IMM structure
FMS_CPF_omCmdHandler::FMS_CPF_omCmdHandler(const char *DN, char *attrName, char *attrValue, ACS_CC_AttrValueType attrType)
{

    ObjectDN = DN;
    attributes = NULL;
    paramToChange.attrName = attrName;
	paramToChange.attrType = attrType;
	paramToChange.attrValuesNum = 1;
	paramToChange.attrValues = new void*[paramToChange.attrValuesNum];
	paramToChange.attrValues[0] = reinterpret_cast<void*>(attrValue);
    m_initialized = false;
    initImmOpResult();
    fmsCpfOmHandlerTrace = new ACS_TRA_trace("FMS_CPF_OmCmdHandler");
}

FMS_CPF_omCmdHandler::~FMS_CPF_omCmdHandler()
{
    if(attributes)
      delete[] attributes;

    if (ACS_CC_SUCCESS != finalize())
        TRACE(fmsCpfOmHandlerTrace,"~FMS_CPF_omCmdHandler IMM finalize failed, error code = %d, error msg = %s", getLastImmErrorCode(), getLastImmErrorCode());

	if(NULL != fmsCpfOmHandlerTrace)
			delete fmsCpfOmHandlerTrace;
}

void
FMS_CPF_omCmdHandler::addNewAttrBase(char *aName, ACS_CC_AttrValueType attrType)
{ 
      attributes[curNumAttr].attrName = aName;
      attributes[curNumAttr].attrType = attrType;
      attributes[curNumAttr].attrValuesNum = 1;
      attributes[curNumAttr].attrValues = new void*[1];
}

void
FMS_CPF_omCmdHandler::addNewAttr(char *aName, ACS_CC_AttrValueType attrType, const char *attrVal)
{ 
    char *value = const_cast<char*>(attrVal);
    if(curNumAttr <= numAttr-1)
    {
      addNewAttrBase(aName, attrType);
      attributes[curNumAttr].attrValues[0] = reinterpret_cast<void*>(value);
      AttrList.push_back(attributes[curNumAttr]);
      curNumAttr++;
    }
}

void
FMS_CPF_omCmdHandler::addNewAttr(char *aName, ACS_CC_AttrValueType attrType, int *valueI32)
{ 
    if(curNumAttr <= numAttr-1)
    {
      addNewAttrBase(aName, attrType);
      attributes[curNumAttr].attrValues[0] = reinterpret_cast<void*>(valueI32);
      AttrList.push_back(attributes[curNumAttr]);
      curNumAttr++;
    }
}

void
FMS_CPF_omCmdHandler::addNewAttr(char *aName, ACS_CC_AttrValueType attrType, unsigned int *valueUnsInt32)
{ 
    if(curNumAttr <= numAttr-1)
    {
      addNewAttrBase(aName, attrType);
      attributes[curNumAttr].attrValues[0] = reinterpret_cast<void*>(valueUnsInt32);
      AttrList.push_back(attributes[curNumAttr]);
      curNumAttr++;
    }
}

int
FMS_CPF_omCmdHandler::writeObj(void)
{
    bool bFail = false;
    string pn(parentName);

	if (!isInitialized()||(omHandler.createObject(className, parentName, AttrList) == ACS_CC_FAILURE))
	{
		   bFail = true;
	}

    if(bFail)
    {
    	setImmOpResult();
    }

    return getLastImmErrorCode();
}

int FMS_CPF_omCmdHandler::modifyObj(void)
{
    bool bFail = false;
    initImmOpResult();

	if (!isInitialized()||(omHandler.modifyAttribute(ObjectDN, &paramToChange) == ACS_CC_FAILURE))
	{
		bFail = true;
		TRACE(fmsCpfOmHandlerTrace, "modifyObj: ObjectDN=%s", ObjectDN);
	}

    if(bFail)
    {
    	setImmOpResult();
    }

    return getLastImmErrorCode();
}


int
FMS_CPF_omCmdHandler::searchDN(char *rootName, std::string txtToFind, std::string &strResult)
{
    std::vector<std::string>::iterator p;
    std::vector<std::string> myList2;
    std::string strRet = "";
    size_t position = 0;
    initImmOpResult();
    bool bFail = false;

    if (!isInitialized()||(omHandler.getClassInstances(rootName, myList2) == ACS_CC_FAILURE))
	{
	   bFail = true;
	}
    else {
	   p = myList2.begin();
	   while(p != myList2.end())
	   {
		  strRet = *p;
		  if ((position = strRet.find(txtToFind)) == string::npos)
		  {
			strRet = "";
		  }
		  else
		  {
			break;
		  }
		  p++;
	   }
	}

    strResult = strRet;

    if(bFail)
    {
    	setImmOpResult();
    }

    return getLastImmErrorCode();

}

int
FMS_CPF_omCmdHandler::loadClassInst(const char* className, std::vector<std::string>&classDnList)
{
    bool bFail = false;
    initImmOpResult();

    if (!isInitialized()||(omHandler.getClassInstances(className, classDnList) == ACS_CC_FAILURE))
	{
	   bFail = true;
	}

    if(bFail)
    {
    	setImmOpResult();
    }

    return getLastImmErrorCode();

}

int
FMS_CPF_omCmdHandler::loadChildInst(const char* className, ACS_APGCC_ScopeT subLevel, std::vector<std::string> *classDnList)
{
    bool bFail = false;
    initImmOpResult();

    if (!isInitialized()||(omHandler.getChildren(className, subLevel, classDnList) == ACS_CC_FAILURE))
    {
       bFail = true;
    }

    if(bFail)
    {
    	setImmOpResult();
    }

    return getLastImmErrorCode();

}

int FMS_CPF_omCmdHandler::getAttrVal(std::string objName, std::string aName, ACS_CC_ImmParameter *paramToFind)
{
    bool bFail = false;
    initImmOpResult();

    size_t strLength = aName.length();
    char tmpValue[strLength + 1];
    ACE_OS::strncpy(tmpValue, aName.c_str(), strLength);
    tmpValue[strLength] = 0;

    paramToFind->attrName = tmpValue;

    if(!isInitialized() || (omHandler.getAttribute(objName.c_str(), paramToFind) == ACS_CC_FAILURE))
	{
    	bFail = true;
	}
    else if(paramToFind->attrValuesNum == 0)
	{
		bFail = true;
	}

    if(bFail) setImmOpResult();

    return getLastImmErrorCode();
}

int
FMS_CPF_omCmdHandler::getAttrListVal(std::string p_objectName, std::vector<ACS_APGCC_ImmAttribute *> p_attributeList)
{
    initImmOpResult();
    bool bFail = false;
    if (!isInitialized()||(omHandler.getAttribute(p_objectName.c_str(), p_attributeList) == ACS_CC_FAILURE))
	{
    	bFail = true;
	}

    if(bFail)
    {
    	setImmOpResult();
    }

    return getLastImmErrorCode();

}

int FMS_CPF_omCmdHandler::deleteObject(const char* p_ObjName)
{
	initImmOpResult();
    bool bFail = false;
	if (!isInitialized()||( omHandler.deleteObject(p_ObjName) == ACS_CC_FAILURE))
	{
		//Delete Object error
		bFail = true;
	}
    if(bFail)
    {
    	setImmOpResult();
    }

    return getLastImmErrorCode();
}

int FMS_CPF_omCmdHandler::deleteObject(const char* p_ObjName, ACS_APGCC_ScopeT p_scope)
{
	initImmOpResult();
	bool bFail = true;
	if (!isInitialized()||( omHandler.deleteObject(p_ObjName, p_scope) == ACS_CC_FAILURE))
	{
		//Delete Object error
		bFail = true;
	}
    if(bFail)
    {
    	setImmOpResult();
    }
    return getLastImmErrorCode();
}

std::string
FMS_CPF_omCmdHandler::getLastImmError(void)
{
    return immMsg;
}

int
FMS_CPF_omCmdHandler::getLastImmErrorCode(void)
{
    return immErrCode;
}

bool FMS_CPF_omCmdHandler::isInitialized()
{
	if (!m_initialized)
	{
		if( omHandler.Init(REGISTERED_OI) == ACS_CC_SUCCESS )
		{
			m_initialized = true;
		}
		else
		  TRACE(fmsCpfOmHandlerTrace, "%s","IMM initiation ERROR NOT OK! ");
	}
	return m_initialized;
}

int FMS_CPF_omCmdHandler::finalize()
{
	int exitCode = ACS_CC_SUCCESS;
	if (m_initialized){

	  if( omHandler.Finalize() != ACS_CC_SUCCESS )
	  {
		// OM handler Finalization failure
		immErrCode = omHandler.getInternalLastError();
		immMsg = omHandler.getInternalLastErrorText();
		exitCode = immErrCode;
	  }
	  else
		  m_initialized = false;
	}
	return exitCode;
}

void FMS_CPF_omCmdHandler::initImmOpResult()
{
	immErrCode = 0;
	immMsg = "";
}

void FMS_CPF_omCmdHandler::setImmOpResult()
{
	immErrCode = omHandler.getInternalLastError();

	TRACE(fmsCpfOmHandlerTrace, "setImmOpResult(), error code:<%d>", immErrCode);

	if( Imm_Error::FAILED_OPERATION == immErrCode)
	{
		//check for CPF custom error
		std::string errorMsg;
		omHandler.getExitCode(immErrCode, errorMsg);

		immErrCode = FMS_CPF_Exception::GENERAL_FAULT;
		immMsg = errorMsg;
		TRACE(fmsCpfOmHandlerTrace, "setImmOpResult(), error message:<%s>", immMsg.c_str());
		if(!errorMsg.empty())
		{
			// try to map the printout to exit code
			std::set<int> errorCodes;

			errorCodes.insert(FMS_CPF_Exception::INTERNALERROR);
			errorCodes.insert(FMS_CPF_Exception::PHYSICALERROR);
			errorCodes.insert(FMS_CPF_Exception::ACCESSERROR);
			errorCodes.insert(FMS_CPF_Exception::FILENOTFOUND);
			errorCodes.insert(FMS_CPF_Exception::FILEEXISTS);
			errorCodes.insert(FMS_CPF_Exception::INVALIDFILE);
			errorCodes.insert(FMS_CPF_Exception::INVALIDREF);
			errorCodes.insert(FMS_CPF_Exception::VOLUMENOTFOUND);
			errorCodes.insert(FMS_CPF_Exception::FILEISOPEN);

			std::set<int>::const_iterator errorCode;

			for(errorCode= errorCodes.begin(); errorCode != errorCodes.end(); ++errorCode)
			{
				FMS_CPF_Exception text(static_cast<FMS_CPF_Exception::errorType>(*errorCode));
				if(std::string::npos != errorMsg.find(text.errorText()) )
				{
					immErrCode = (*errorCode);
					break;
				}
			}
		}
	}

	TRACE(fmsCpfOmHandlerTrace, "setImmOpResult(), error code:<%d>", immErrCode);
}
