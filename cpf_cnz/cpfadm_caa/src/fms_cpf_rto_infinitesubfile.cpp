/*
 * * @file fms_cpf_rto_infinitesubfile.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_RTO_InfiniteSubFile.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_rto_infinitesubfile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-10-15
 *	@version 1.0.0
 *
 *	COPYRIGHT Ericsson AB, 2010
 *	All rights reserved.
 *
 *	The information in this document is the property of Ericsson.
 *	Except as specifically authorized in writing by Ericsson, the receiver of
 *	this document shall keep the information contained herein confidential and
 *	shall protect the same in whole or in part from disclosure and dissemination
 *	to third parties. Disclosure and disseminations to the receivers employees
 *	shall only be made on a strict need to know basis.
 *
 *	REVISION INFO
 *	+========+============+==============+=====================================+
 *	| REV    | DATE       | AUTHOR       | DESCRIPTION                         |
 *	+========+============+==============+=====================================+
 *	| 1.0.0  | 2011-10-15 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_rto_infinitesubfile.h"
#include "fms_cpf_infinitesubfiles_manager.h"
#include "fms_cpf_cmdhandler.h"
#include "fms_cpf_datafile.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filereference.h"
#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_message.h"
#include "fms_cpf_common.h"


#include "fms_cpf_privateexception.h"
#include "fms_cpf_fileid.h"

#include "acs_apgcc_omhandler.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"

#include <vector>

extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	ROUTINE: FMS_CPF_RTO_InfiniteSubFile
 ============================================================================ */
FMS_CPF_RTO_InfiniteSubFile::FMS_CPF_RTO_InfiniteSubFile(FMS_CPF_CmdHandler* cmdHandler):
FMS_CPF_OI_BaseObject(cmdHandler, cpf_imm::InfiniteSubFileClassName),
m_implementerName(""),
m_objManager(NULL)
{
	// Define trace
	fms_cpf_RTOTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_RTO_InfiniteSubFile");
}

/*============================================================================
	ROUTINE: ~FMS_CPF_RTO_InfiniteSubFile
 ============================================================================ */
FMS_CPF_RTO_InfiniteSubFile::~FMS_CPF_RTO_InfiniteSubFile()
{
	if(NULL != m_objManager)
	{
		// Deallocate OM resource
		m_objManager->Finalize();
		delete m_objManager;
	}

	if(NULL != fms_cpf_RTOTrace)
		delete fms_cpf_RTOTrace;
}

/*============================================================================
	ROUTINE: registerTOImm
 ============================================================================ */
bool FMS_CPF_RTO_InfiniteSubFile::registerToImm(bool systemType, const std::string& implName)
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering in registerToImm");
	bool result = false;
	m_IsMultiCP = systemType;
	m_implementerName = implName;

	// Allocate OM resource
	m_objManager = new (std::nothrow) OmHandler();

	if(NULL == m_objManager)
	{
		CPF_Log.Write("FMS_CPF_RTO_InfiniteSubFile::registerToImm, error on OmHandler allocation", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_RTOTrace, "%s", "Leaving registerToImm(), error on OmHandler allocation");
	}
	else
	{
		// Init RT owner
		if(m_objManager->Init() != ACS_CC_FAILURE)
		{
			// Init OM resource
			if(init(m_implementerName) != ACS_CC_FAILURE)
			{
				result = true;
				TRACE(fms_cpf_RTOTrace, "registerToImm(), RT owner<%s> registered", m_implementerName.c_str() );
			}
			else
			{
				// Deallocate OM resource
				m_objManager->Finalize();
				delete m_objManager;
				m_objManager = NULL;
				CPF_Log.Write("FMS_CPF_RTO_InfiniteSubFile::registerToImm, error on RT owner init", LOG_LEVEL_ERROR);
				TRACE(fms_cpf_RTOTrace, "%s", "Leaving registerToImm(), error on RT owner init");
			}
		}
		else
		{
			// Deallocate OM resource
			delete m_objManager;
			m_objManager = NULL;
			CPF_Log.Write("FMS_CPF_RTO_InfiniteSubFile::registerToImm, error on OmHandler init", LOG_LEVEL_ERROR);
			TRACE(fms_cpf_RTOTrace, "%s", "Leaving registerToImm(), error on OmHandler init");
		}
	}

	TRACE(fms_cpf_RTOTrace, "%s", "Leaving registerToImm");
	return result;
}

/*============================================================================
	ROUTINE: updateInitialState
 ============================================================================ */
void FMS_CPF_RTO_InfiniteSubFile::updateInitialState(const std::string& cpName)
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering updateInitialState()");
	ACS_CC_ReturnType result;
	std::vector<mapKeyType>::const_iterator element;
	std::vector<mapKeyType> fileList;

	std::set<std::string> immISF;
	std::set<std::string> datadiskISF;
	std::set<std::string>::const_iterator immElement;
	std::set<std::string>::iterator datadiskElement;

	// get the list of pair <infinite file name , DN>
	DirectoryStructureMgr::instance()->getListOfInfiniteFile(cpName, fileList);

	bool isRestartAfterRestore = DirectoryStructureMgr::instance()->isAfterRestore();

	for(element = fileList.begin(); element != fileList.end(); ++element)
	{
		// Infinite file DN
		std::string fileDN(element->second);
		TRACE(fms_cpf_RTOTrace, "updateInitialState(), infinite file:<%s>, DN:<%s>", element->first.c_str(), fileDN.c_str());

		getListOfISF(fileDN, immISF);
		DirectoryStructureMgr::instance()->getListOfInfiniteSubFile(element->first, cpName, datadiskISF);

		if(isRestartAfterRestore && datadiskISF.begin() != datadiskISF.end())
		{
			updateAttribute(cpName, element->first, (*datadiskISF.begin()), (*datadiskISF.rbegin()) );
		}

		for(immElement = immISF.begin(); immElement != immISF.end(); ++immElement)
		{
			datadiskElement = datadiskISF.find((*immElement));
			if(datadiskISF.end() != datadiskElement)
			{
				// ISF on data disk and in IMM
				datadiskISF.erase(datadiskElement);
			}
			else
			{
				// ISF only into IMM
				// remove it
				char subFileDN[1024] = {0};
				ACE_OS::snprintf(subFileDN, 1023, "%s=%s,%s", cpf_imm::InfiniteSubFileKey, (*immElement).c_str(), fileDN.c_str() );
				// Remove the runtime object
				result = deleteRuntimeObj(subFileDN);
				if(ACS_CC_SUCCESS == result)
				{
					TRACE(fms_cpf_RTOTrace, "updateInitialState(), subfile<%s> object deleted in IMM", subFileDN );
				}
				else
				{
					char errorMsg[512]={'\0'};
					ACE_OS::snprintf(errorMsg, 511, "updateInitialState(), error:<%d> on subfile object deletion, infinitefile=<%s>, DN:<%s>", getInternalLastError(),  element->first.c_str(), subFileDN );
					CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
					TRACE(fms_cpf_RTOTrace, "%s", errorMsg);

					 // The message will be deleted from receiver
					CPF_DeleteISF_Msg* removeISFMsg = new (std::nothrow) CPF_DeleteISF_Msg(subFileDN);

					if(NULL != removeISFMsg)
					{
						// Insert the creation request in the ISF manager
						int putResult = InfiteSubFileHndl::instance()->putq(removeISFMsg);

						if(FAILURE == putResult)
						{
							removeISFMsg->release();
							CPF_Log.Write("FMS_CPF_RTO_InfiniteSubFile::updateCallback(), failed to put remove ISF message", LOG_LEVEL_ERROR);
							// re-try
							deleteRuntimeObj(subFileDN);
						}
					}
				}
			}
		}

		std::vector<ACS_CC_ValuesDefinitionType> objAttrList;

		ACS_CC_ValuesDefinitionType attributeRDN;
		/*Fill the rdn Attribute */
		attributeRDN.attrName = cpf_imm::InfiniteSubFileKey;
		attributeRDN.attrType = ATTR_STRINGT;
		attributeRDN.attrValuesNum = 1;

		// Now it contains the objects to create
		for(datadiskElement = datadiskISF.begin(); datadiskElement != datadiskISF.end(); ++datadiskElement)
		{
			char tmpRDNValue[128] = {0};
			ACE_OS::snprintf(tmpRDNValue, 127, "%s=%s", cpf_imm::InfiniteSubFileKey, (*datadiskElement).c_str() );
			void* valueRDN[1] = {reinterpret_cast<void*>(tmpRDNValue)};
			attributeRDN.attrValues = valueRDN;

			//Add the attributes to vector
			objAttrList.push_back(attributeRDN);

			ACS_CC_ReturnType result = createRuntimeObj(cpf_imm::InfiniteSubFileClassName, fileDN.c_str(), objAttrList);

			if(ACS_CC_SUCCESS == result)
			{
				TRACE(fms_cpf_RTOTrace, "updateInitialState, subfile<%s> object created in IMM", (*datadiskElement).c_str() );
			}
			else
			{
				char errorMsg[512]={'\0'};
				ACE_OS::snprintf(errorMsg, 511, "createISF(), error:<%d> on subfile<%s> object creation, infinitefile=<%s>, DN:<%s>", getInternalLastError(), (*datadiskElement).c_str(),  element->first.c_str(), fileDN.c_str() );
				CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
				TRACE(fms_cpf_RTOTrace, "%s", errorMsg);
				//re-try
				createRuntimeObj(cpf_imm::InfiniteSubFileClassName, fileDN.c_str(), objAttrList);
			}

			objAttrList.clear();
		}

		// Clear the list for the next file
		immISF.clear();
		datadiskISF.clear();
	}

	TRACE(fms_cpf_RTOTrace, "%s", "Leaving updateInitialState");
}

/*============================================================================
	ROUTINE: updateInitialState
 ============================================================================ */
void FMS_CPF_RTO_InfiniteSubFile::updateAttribute(const std::string& cpName, const std::string& fileName, const std::string& firstSubFile, const std::string& lastSubFile)
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering updateAttribute()");
	FMS_CPF_FileId fileid(fileName);
	FileReference reference;

	try
	{
		TRACE(fms_cpf_RTOTrace, "updateAttribute(), file:<%s> of Cp:<%s>, first ISF:<%s> and last ISF:<%s>", fileName.c_str(),
																											 cpName.c_str(),
																											 firstSubFile.c_str(),
																											 lastSubFile.c_str());
		// Open logical file
		reference = DirectoryStructureMgr::instance()->open(fileid, FMS_CPF_Types::NONE_, cpName.c_str());

		FMS_CPF_Attribute fileAttribute = reference->getAttribute();

		FMS_CPF_Types::fileType fType;
		unsigned long recordLength=0;
		unsigned long maxSize=0;
		unsigned long maxTime=0;
		unsigned long activeSubFile = 0;
		unsigned long lastSent = 0;
		bool composite;
		bool releaseCondition=false;
		std::string transferQueue;
		FMS_CPF_Types::transferMode transferMode;
		FMS_CPF_Types::transferMode initTransferMode;

		FMS_CPF_Types::extractFileAttr(fileAttribute, fType, recordLength, composite, maxSize,
									   maxTime, releaseCondition ,activeSubFile, lastSent, transferMode,
									   transferQueue, initTransferMode
									  );

		TRACE(fms_cpf_RTOTrace, "updateAttribute(), stored report:<%lu> and active<%lu>", lastSent, activeSubFile);

		// get the first subfile of the infinite file
		std::istringstream stringToInt(firstSubFile);
		stringToInt >> lastSent;

		TRACE(fms_cpf_RTOTrace, "updateAttribute(), current last report <%d>", lastSent);

		// Set last subfile sent to the first subfile less one,
		// so the next one to send will be just the current first one
		--lastSent;

		// get the last subfile, it is the current active subfile
		std::istringstream stringToInt2(lastSubFile);
		stringToInt2 >> activeSubFile;

		TRACE(fms_cpf_RTOTrace, "updateAttribute(), current active <%d>", activeSubFile);

		// Assemble the file attribute with old and new parameters value
		FMS_CPF_Types::fileAttributes newFileAttributes =
				FMS_CPF_Types::createFileAttr(fType, recordLength, composite, maxSize, maxTime,
											  releaseCondition, activeSubFile, lastSent, transferMode,
											  transferQueue, initTransferMode
											 );

		TRACE(fms_cpf_RTOTrace, "updateAttribute(), update last report to <%lu> and active to <%lu>", lastSent, activeSubFile);
		FMS_CPF_Attribute newFileAttribute(newFileAttributes);

		// Change file attributes
		reference->setAttribute(newFileAttribute);

	}
	catch(FMS_CPF_PrivateException& ex)
	{
		char errorMsg[128] = {0};
		snprintf(errorMsg, 127, "FMS_CPF_RTO_InfiniteSubFile::updateAttribute(), attribute file=<%s> failed on Cp:<%s>, error=<%s>", fileName.c_str(), cpName.c_str(), ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_RTOTrace, "%s", errorMsg);
	}

	DirectoryStructureMgr::instance()->closeExceptionLess(reference, cpName.c_str());

	TRACE(fms_cpf_RTOTrace, "%s", "Leaving updateAttribute");
}

/*============================================================================
	ROUTINE: updateCallback
 ============================================================================ */
void FMS_CPF_RTO_InfiniteSubFile::getListOfISF(const std::string& fileDN, std::set<std::string>& subFileList)
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering in getListOfISF");

	std::vector<std::string> subFileDNList;
	// get all file of this volume
	if(m_objManager->getChildren( fileDN.c_str(), ACS_APGCC_SUBLEVEL, &subFileDNList) == ACS_CC_SUCCESS )
	{
		std::vector<std::string>::const_iterator subFileDN;

		// insert each file in the list
		for(subFileDN = subFileDNList.begin(); subFileDN != subFileDNList.end(); ++subFileDN)
		{
			std::string subFileName;
			if(getSubInfiniteFileName((*subFileDN), subFileName) )
			{
				subFileList.insert(subFileName);
				// Todo only for test scope to remove i!
				TRACE(fms_cpf_RTOTrace, "getListOfISF(), found ISF:<%s>", subFileName.c_str() );
			}
		}
	}

	TRACE(fms_cpf_RTOTrace, "%s", "Leaving getListOfISF");
}

/*============================================================================
	ROUTINE: createISF
 ============================================================================ */
int FMS_CPF_RTO_InfiniteSubFile::createISF(const unsigned int& subFileValue, const std::string& fileName, const std::string& volume, const std::string& cpName)
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering in createISF");
    std::string infiniteFileDN;
    operationResult createResult = Retry;

    if(getParentDN(fileName, volume, cpName, infiniteFileDN))
    {
    	//The list of attributes
    	std::vector<ACS_CC_ValuesDefinitionType> objAttrList;
    	ACS_CC_ValuesDefinitionType attributeRDN;

    	char tmpRDNValue[64] = {0};
    	ACE_OS::snprintf(tmpRDNValue, 63, "%s=%.10i", cpf_imm::InfiniteSubFileKey, subFileValue);

    	//Fill the rdn Attribute
		attributeRDN.attrName = cpf_imm::InfiniteSubFileKey;
		attributeRDN.attrType = ATTR_STRINGT;
		attributeRDN.attrValuesNum = 1;

		void* valueRDN[1] = {reinterpret_cast<void*>(tmpRDNValue)};
		attributeRDN.attrValues = valueRDN;

		//Add the attributes to vector
		objAttrList.push_back(attributeRDN);

		ACS_CC_ReturnType result;
		result = createRuntimeObj(cpf_imm::InfiniteSubFileClassName, infiniteFileDN.c_str(), objAttrList);

		if(ACS_CC_SUCCESS == result)
		{
			createResult = OK;
			TRACE(fms_cpf_RTOTrace, "createISF, subfile<%s> object created in IMM", tmpRDNValue );
		}
		else
		{
			createResult = Retry;
			int immError = getInternalLastError();

			// Object already present
			if(IMM_ErrorCode::EXIST == immError)
			{
				createResult = OK;
			}
			else if(IMM_ErrorCode::BAD_HANDLE == immError)
			{
				// Imm handle error
				createResult = ResetAndRetry;
			}

			char errorMsg[512]={'\0'};
			ACE_OS::snprintf(errorMsg, 511, "createISF(), error:<%d> on subfile<%s> object creation, infinitefile=<%s>, DN:<%s>", immError, tmpRDNValue, fileName.c_str(), infiniteFileDN.c_str() );
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_RTOTrace, "%s", errorMsg);
		}
    }
    else
    {
    	char errorMsg[512]={'\0'};
    	ACE_OS::snprintf(errorMsg, 511, "createISF, error on get DN of infinite file=<%s>", fileName.c_str() );
    	CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
    	TRACE(fms_cpf_RTOTrace, "%s", errorMsg);
    }

    TRACE(fms_cpf_RTOTrace, "%s", "Leaving createISF");
    return createResult;
}

/*============================================================================
	ROUTINE: deleteISF
 ============================================================================ */
int FMS_CPF_RTO_InfiniteSubFile::deleteISF(const unsigned int& subFileValue, const std::string& fileName, const std::string& volume, const std::string& cpName)
{
    TRACE(fms_cpf_RTOTrace, "%s", "Entering in deleteISF()");
    std::string infiniteFileDN;
    int deleteResult = Retry;
    // Get parent DN
    if(getParentDN(fileName, volume, cpName, infiniteFileDN))
    {
    	// assemble the full DN
       	char strSubFile[12]={'\0'};
    	ACE_OS::snprintf(strSubFile, 11, "%.10i", subFileValue);
    	std::string subFileDN = cpf_imm::InfiniteSubFileKey;
    	subFileDN += parseSymbol::equal;
    	subFileDN += strSubFile;
    	subFileDN += parseSymbol::comma;
    	subFileDN += infiniteFileDN;

    	// Remove the runtime object
    	ACS_CC_ReturnType result = deleteRuntimeObj(subFileDN.c_str());

    	if(ACS_CC_SUCCESS == result)
        {
    	    deleteResult = OK;
            TRACE(fms_cpf_RTOTrace, "deleteISF(), subfile<%s> object deleted in IMM", subFileDN.c_str() );
        }
        else
        {
            deleteResult = Retry;
            int immError = getInternalLastError();

            if(IMM_ErrorCode::BAD_HANDLE == immError)
            {
                    // Imm handle error
                    deleteResult = ResetAndRetry;
            }

            char errorMsg[512]={'\0'};

            ACE_OS::snprintf(errorMsg, 511, "deleteISF(), error:<%d> on subfile<%s> object deletion, infinitefile=<%s>, DN:<%s>", immError, strSubFile, fileName.c_str(), infiniteFileDN.c_str() );
            CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
            TRACE(fms_cpf_RTOTrace, "%s", errorMsg);
        }
    }
    else
    {
        char errorMsg[512]={'\0'};
        ACE_OS::snprintf(errorMsg, 511, "deleteISF(), error on get DN of infinite file=<%s>", fileName.c_str() );
        CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
        TRACE(fms_cpf_RTOTrace, "%s", errorMsg);
    }

    TRACE(fms_cpf_RTOTrace, "%s", "Leaving deleteISF()");
    return deleteResult;
}

/*============================================================================
        ROUTINE: deleteISF
 ============================================================================ */
int FMS_CPF_RTO_InfiniteSubFile::deleteISF(const std::string& subFileDN)
{
    TRACE(fms_cpf_RTOTrace, "Entering in deleteISF(), DN:<%s>", subFileDN.c_str());
    int deleteResult = Retry;

    // Remove the runtime object
    ACS_CC_ReturnType result = deleteRuntimeObj(subFileDN.c_str());

    if(ACS_CC_SUCCESS == result)
    {
        deleteResult = OK;
        TRACE(fms_cpf_RTOTrace, "deleteISF(), subfile<%s> object deleted in IMM", subFileDN.c_str() );
    }
    else
    {
        deleteResult = Retry;
        int immError = getInternalLastError();

        if(IMM_ErrorCode::BAD_HANDLE == immError)
        {
            // Imm handle error
            deleteResult = ResetAndRetry;
        }
        else if(IMM_ErrorCode::NOT_EXIST == immError)
        {
        	// Already deleted
        	deleteResult = OK;
        }

        char errorMsg[512]={'\0'};

        ACE_OS::snprintf(errorMsg, 511, "deleteISF(), error:<%d> on subfile DN<%s> object deletion", immError, subFileDN.c_str() );
        CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
        TRACE(fms_cpf_RTOTrace, "%s", errorMsg);
    }

    TRACE(fms_cpf_RTOTrace, "%s", "Leaving deleteISF()");
    return deleteResult;
}

/*============================================================================
	ROUTINE: assembleRootDN
 ============================================================================ */
void FMS_CPF_RTO_InfiniteSubFile::assembleRootDN(const std::string& volume, const std::string& cpName, std::string& rootDN)
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering in assembleRootDN");

	rootDN = cpf_imm::VolumeKey;
	rootDN += parseSymbol::equal;
	rootDN += volume;

	if(m_IsMultiCP)
	{
		rootDN += parseSymbol::underLine + cpName;
	}
	TRACE(fms_cpf_RTOTrace, "assembleRootDN, partial rootDn=<%s>", rootDN.c_str());

	rootDN += parseSymbol::comma;
	rootDN += cpf_imm::parentRoot;


	TRACE(fms_cpf_RTOTrace, "%s", "Leaving assembleRootDN");
}

/*============================================================================
	ROUTINE: getParentDN
 ============================================================================ */
bool FMS_CPF_RTO_InfiniteSubFile::getInfiniteFileDN(const std::string& fileName, const std::string& rootDN, std::string& fileDN)
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering in getInfiniteFileDN");
	bool result = false;

	std::vector<std::string> listOfDNs;
	std::vector<std::string>::const_iterator file;

	// get all files DN under the volume
	if(m_objManager->getChildren(rootDN.c_str(), ACS_APGCC_SUBLEVEL, &listOfDNs) != ACS_CC_FAILURE)
	{
		// Search the specific file name
		for(file =listOfDNs.begin(); file != listOfDNs.end(); ++file)
		{
			// Get the file name from DN
			// Split the field in RDN and Value
			size_t equalPos = (*file).find_first_of(parseSymbol::equal);
			size_t commaPos = (*file).find_first_of(parseSymbol::comma);

			// Check if some error happens
			if( (std::string::npos != equalPos) && (std::string::npos != commaPos) )
			{
				// Attribute found and get it
				std::string attributeValue = (*file).substr(equalPos + 1, (commaPos - equalPos - 1) );
				// make the name in upper case
				ACS_APGCC::toUpper(attributeValue);

				// check the filename
				if(fileName.compare(attributeValue) == 0)
				{
					// file is found
					fileDN = (*file);
					result = true;
					TRACE(fms_cpf_RTOTrace, "getInfiniteFileDN(), file=<%s> as DN=<%s>", fileName.c_str(), fileDN.c_str());
					// exit from the for loop
					break;
				}
			}
		}
	}
	else
	{
		TRACE(fms_cpf_RTOTrace, "getInfiniteFileDN(), error to getChildren under root DN=<%s>",  rootDN.c_str());
	}

	TRACE(fms_cpf_RTOTrace, "%s", "Leaving getInfiniteFileDN");
	return result;
}

/*============================================================================
	ROUTINE: getParentDN
 ============================================================================ */
bool FMS_CPF_RTO_InfiniteSubFile::getParentDN(const std::string& fileName, const std::string& volume, const std::string& cpName, std::string& parentDN)
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering in getParentDN");
	bool result = false;
	std::string rootObjectDN;
	// Try to get the DN from internal map
	result = getMapEntry(fileName, cpName, parentDN);

	// Check if it is found
	if(!result)
	{
		TRACE(fms_cpf_RTOTrace, "%s", "getParentDN, file not present in the map");
		// Entry not found in the map
		//assembleRootDN(volume, cpName, rootObjectDN);
		//result = getInfiniteFileDN(fileName, rootObjectDN, parentDN);
		result = DirectoryStructureMgr::instance()->getFileDN(fileName, cpName, parentDN);
		if(result)
		{
			// update the map with the new value
			TRACE(fms_cpf_RTOTrace, "%s", "getParentDN, insert a new file DN in the map");
			// Insert the new file DN in the map
			insertMapEntry(fileName, cpName, parentDN);
		}
	}

	TRACE(fms_cpf_RTOTrace, "%s", "Leaving getParentDN");
	return result;
}

/*============================================================================
	ROUTINE: updateCallback
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_RTO_InfiniteSubFile::updateCallback(const char* p_objName, const char** p_attrNames)
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering updateCallback(...)");
	ACS_CC_ReturnType result = ACS_CC_SUCCESS;
	// To avoid warning about unused parameter
	UNUSED(p_attrNames);

	CPF_DataFile_Request::dataFile infoFile;

	// get infinite main file name, cp name and subfile name
	if( getInfiniteFileName(p_objName, infoFile.mainFileName) &&
					getSubInfiniteFileName(p_objName, infoFile.subFileName) &&
					getCpName(p_objName, infoFile.cpName) )
	{
		ACE_Future<FMS_CPF_PrivateException> waitResult;
		FMS_CPF_PrivateException operationResult;
		FMS_CPF_Exception::errorType cmdExeResult;

		TRACE(fms_cpf_RTOTrace, "updateRuntime(), cpName:<%s>", infoFile.cpName.c_str());
		// create the file modify request
		CPF_DataFile_Request* dataFile = new CPF_DataFile_Request(waitResult, infoFile);

		m_CmdHandler->enqueue(dataFile);

		TRACE(fms_cpf_RTOTrace, "%s", "updateCallback(), wait data file cmd execution");
		waitResult.get(operationResult);

		cmdExeResult = operationResult.errorCode();

		// checks the cmd execution result
		if(FMS_CPF_Exception::OK != cmdExeResult )
		{
                    infoFile.exclusiveAccess = 0;
                    infoFile.fileSize = 0;
                    infoFile.numOfReaders = 0;
                    infoFile.numOfWriters = 0;

                    TRACE(fms_cpf_RTOTrace, "%s", "updateCallback(), error on infinite subfile data get");

                    if(FMS_CPF_Exception::FILENOTFOUND == cmdExeResult)
                    {
                        // The message will be deleted from receiver
                        CPF_DeleteISF_Msg* removeISFMsg = new (std::nothrow) CPF_DeleteISF_Msg(p_objName);

                        if(NULL != removeISFMsg)
                        {
                            // Insert the creation request in the ISF manager
                            int putResult = InfiteSubFileHndl::instance()->putq(removeISFMsg);

                            if(FAILURE == putResult)
                            {
                                removeISFMsg->release();
                                CPF_Log.Write("FMS_CPF_RTO_InfiniteSubFile::updateCallback(), failed to put remove ISF message", LOG_LEVEL_ERROR);
                            }
                        }
                        else
                        {
                            CPF_Log.Write("FMS_CPF_RTO_InfiniteSubFile::updateCallback(), failed allocation of remove ISF message", LOG_LEVEL_ERROR);
                        }

                    }
		}

		// Set the File Size
		ACS_CC_ImmParameter paramSize;
		paramSize.attrName = cpf_imm::sizeAttribute;
		paramSize.attrType = ATTR_UINT32T;
		paramSize.attrValuesNum = 1;
		void* size[1] = { reinterpret_cast<void*>(&(infoFile.fileSize)) };
		paramSize.attrValues = size;

		TRACE(fms_cpf_RTOTrace,"updateRuntime(), update size=%d",infoFile.fileSize );
		modifyRuntimeObj(p_objName, &paramSize);

		// Set the numOfReaders on File
		ACS_CC_ImmParameter paramNumReaders;
		paramNumReaders.attrName = cpf_imm::numReadersAttribute;
		paramNumReaders.attrType = ATTR_UINT32T;
		paramNumReaders.attrValuesNum = 1;
		void* numReaders[1] = { reinterpret_cast<void*>(&(infoFile.numOfReaders)) };
		paramNumReaders.attrValues = numReaders;

		TRACE(fms_cpf_RTOTrace,"updateRuntime(), update numOfReaders=%d", infoFile.numOfReaders );
		modifyRuntimeObj(p_objName, &paramNumReaders);

		// Set the numOfWriters on File
		ACS_CC_ImmParameter paramNumWriters;
		paramNumWriters.attrName = cpf_imm::numWritersAttribute;
		paramNumWriters.attrType = ATTR_UINT32T;
		paramNumWriters.attrValuesNum = 1;
		void* numWriters[1] = { reinterpret_cast<void*>(&(infoFile.numOfWriters)) };
		paramNumWriters.attrValues = numWriters;

		TRACE(fms_cpf_RTOTrace,"updateRuntime(), update numOfWriters=%d", infoFile.numOfWriters );
		modifyRuntimeObj(p_objName, &paramNumWriters);

		// Set the exclusiveAccess on File
		ACS_CC_ImmParameter paramExcAccess;
		paramExcAccess.attrName = cpf_imm::exclusiveAccessAttribute;
		paramExcAccess.attrType = ATTR_INT32T;
		paramExcAccess.attrValuesNum = 1;
		void* excAccess[1] = { reinterpret_cast<void*>(&(infoFile.exclusiveAccess)) };
		paramExcAccess.attrValues = excAccess;

		TRACE(fms_cpf_RTOTrace, "updateRuntime(), update exclusiveAccess=%d", infoFile.exclusiveAccess);
		modifyRuntimeObj(p_objName, &paramExcAccess);

	}

	TRACE(fms_cpf_RTOTrace, "%s", "Leaving updateCallback(...)");
	return result;
}

/*============================================================================
	ROUTINE: getMapEntry
 ============================================================================ */
bool FMS_CPF_RTO_InfiniteSubFile::getMapEntry(const std::string& fileName, const std::string& cpName, std::string& fileDN)
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering getMapEntry(...)");
	bool result = false;
	mapKeyType mapKey(fileName, cpName);
	std::map<mapKeyType, std::string>::const_iterator element;
	{
		// Lock the map usage
		ACE_GUARD_RETURN(ACE_Thread_Mutex, objLock, m_mutex, false);

		element = m_FileDNMap.find(mapKey);

		if(element != m_FileDNMap.end())
		{
			fileDN = (*element).second;
			result = true;
		}
	}
	TRACE(fms_cpf_RTOTrace, "%s", "Leaving getMapEntry(...)");
	return result;
}

/*============================================================================
	ROUTINE: clearMap
 ============================================================================ */
void FMS_CPF_RTO_InfiniteSubFile::clearMap()
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering clearMap(...)");
	{
		// Lock the map usage
		ACE_GUARD(ACE_Thread_Mutex, objLock, m_mutex);
		m_FileDNMap.clear();
	}
	TRACE(fms_cpf_RTOTrace, "%s", "Leaving clearMap(...)");
}

/*============================================================================
	ROUTINE: insertMapEntry
 ============================================================================ */
void FMS_CPF_RTO_InfiniteSubFile::insertMapEntry(const std::string& fileName, const std::string& cpName, const std::string& fileDN)
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering insertMapEntry(...)");
	{
		// Lock the map usage
		ACE_GUARD(ACE_Thread_Mutex, objLock, m_mutex);
		mapKeyType mapKey(fileName, cpName);
		m_FileDNMap.insert(std::make_pair(mapKey, fileDN));
		TRACE(fms_cpf_RTOTrace, "insertMapEntry(...), map size=%zd", m_FileDNMap.size());
	}
	TRACE(fms_cpf_RTOTrace, "%s", "Leaving insertMapEntry(...)");
}

/*============================================================================
	ROUTINE: updateMapEntry
 ============================================================================ */
void FMS_CPF_RTO_InfiniteSubFile::updateMapEntry(const std::string& fileName, const std::string& cpName, const std::string& newFileName, const std::string& fileDN)
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering updateMapEntry(...)");
	{
		// Lock the map usage
		ACE_GUARD(ACE_Thread_Mutex, objLock, m_mutex);
		mapKeyType mapKey(fileName, cpName);
		m_FileDNMap.erase(mapKey);
		mapKeyType newMapKey(newFileName, cpName);
		m_FileDNMap.insert(std::make_pair(newMapKey, fileDN));
	}
	TRACE(fms_cpf_RTOTrace, "%s", "Leaving updateMapEntry(...)");
}

/*============================================================================
	ROUTINE: updateMapEntry
 ============================================================================ */
void FMS_CPF_RTO_InfiniteSubFile::removeMapEntry(const std::string& fileName, const std::string& cpName)
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering removeMapEntry(...)");
	{
		// Lock the map usage
		ACE_GUARD(ACE_Thread_Mutex, lockObj, m_mutex);
		mapKeyType mapKey(fileName, cpName);
		m_FileDNMap.erase(mapKey);
	}
	TRACE(fms_cpf_RTOTrace, "%s", "Leaving removeMapEntry(...)");
}
/*============================================================================
	ROUTINE: adminOperationCallback
 ============================================================================ */
void FMS_CPF_RTO_InfiniteSubFile::adminOperationCallback(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_InvocationType invocation, const char* p_objName, ACS_APGCC_AdminOperationIdType operationId,  ACS_APGCC_AdminOperationParamType**paramList)
{
	TRACE(fms_cpf_RTOTrace, "%s", "Entering adminOperationCallback(...)");
	// To avoid warning about unused parameter
	int FAILED = 21; // SA_AIS_ERR_FAILED_OPERATION
	UNUSED(p_objName);
	UNUSED(operationId);
	UNUSED(paramList);
	// No actions are defined in infiniteSubFile class
	adminOperationResult(oiHandle, invocation, FAILED);
	TRACE(fms_cpf_RTOTrace, "%s", "Leaving adminOperationCallback(...)");
}
