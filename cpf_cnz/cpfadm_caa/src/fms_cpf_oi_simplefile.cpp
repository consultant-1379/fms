/*
 * * @file fms_cpf_oi_simplefile.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_OI_SimpleFile.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_oi_simplefile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-02-13
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
 *	| 1.0.0  | 2012-02-13 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_oi_simplefile.h"
#include "fms_cpf_createfile.h"
#include "fms_cpf_deletefile.h"
#include "fms_cpf_changefileattribute.h"
#include "fms_cpf_datafile.h"
#include "fms_cpf_common.h"

#include "fms_cpf_privateexception.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"

#include <boost/format.hpp>

#include <sstream>
#include <list>

extern ACS_TRA_Logging CPF_Log;

namespace simpleFileClass{
    const char ImmImplementerName[] = "CPF_OI_SimpleFile";
}

/*============================================================================
	ROUTINE: FMS_CPF_OI_SimpleFile
 ============================================================================ */
FMS_CPF_OI_SimpleFile::FMS_CPF_OI_SimpleFile(FMS_CPF_CmdHandler* cmdHandler) :
	FMS_CPF_OI_BaseObject(cmdHandler, cpf_imm::SimpleFileClassName),
	acs_apgcc_objectimplementerinterface_V3(simpleFileClass::ImmImplementerName)
{
	fms_cpf_oi_simpleFileTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_OI_SimpleFile");
}


/*============================================================================
	ROUTINE: create
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_SimpleFile::create(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *className, const char* parentname, ACS_APGCC_AttrValues **attr)
{
	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Entering in create(...) callback");

	ACS_CC_ReturnType result = ACS_CC_SUCCESS;

	// To avoid warning about unused parameter
	UNUSED(oiHandle);
	TRACE(fms_cpf_oi_simpleFileTrace, "create(...), insert a simple file under parent=<%s>", parentname);

	simpleFileInfo newSimpleFileInfo;

	//initiate values
	newSimpleFileInfo.compression = false;
	newSimpleFileInfo.recordLength = 0;
	newSimpleFileInfo.fileName = "";
	newSimpleFileInfo.completed = false;
	newSimpleFileInfo.action = Create;

	// Get volume and cpName by the parent DN
	if(!getVolumeName(parentname, newSimpleFileInfo.volumeName) ||
			!getCpName(parentname, newSimpleFileInfo.cpName) )
	{
		CPF_Log.Write("FMS_CPF_OI_SimpleFile::create, error on get parent info", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_oi_simpleFileTrace, "%s", "create(...), error on get parent info");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());

		result = ACS_CC_FAILURE;
		return result;
	}

	// Check the IMM class name
	if( 0 == ACE_OS::strcmp(m_ImmClassName.c_str(), className ) )
	{
		std::string objectName;
		// extract first the fileName
		for(size_t idx = 0; attr[idx] != NULL ; ++idx)
		{
			// check if RDN attribute to get fileName
			if( 0 == ACE_OS::strcmp(cpf_imm::SimpleFileKey, attr[idx]->attrName) )
			{
				std::string fileRDN = reinterpret_cast<char *>(attr[idx]->attrValues[0]);

				newSimpleFileInfo.fileDN = fileRDN + parseSymbol::comma + std::string(parentname);

				// get the fileName from RDN
				getLastFieldValue(fileRDN, newSimpleFileInfo.fileName, false);

				objectName = newSimpleFileInfo.fileName;

				ACS_APGCC::toUpper(newSimpleFileInfo.fileName);
				// Validate the value
				if(!checkFileName(newSimpleFileInfo.fileName) )
				{
					result = ACS_CC_FAILURE;

					// Assemble the formated error message
					boost::format errorMsg(errorText::invalidFileName);
					errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

					errorMsg % cpf_imm::SimpleFileObjectName % objectName;

					setExitCode(static_cast<int>(FMS_CPF_Exception::INVALIDFILE), errorMsg.str() );

					TRACE(fms_cpf_oi_simpleFileTrace, "create(...), file name not valid:<%s>", newSimpleFileInfo.fileName.c_str());
				}

				break;
			}
		}

		// extract the attributes
		for(size_t idx = 0; attr[idx] != NULL && (result != ACS_CC_FAILURE); ++idx)
		{
			// check if recordLength attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::recordLengthAttribute, attr[idx]->attrName) )
			{
				newSimpleFileInfo.recordLength = *reinterpret_cast<unsigned int*>(attr[idx]->attrValues[0]);

				if( !checkRecordLength(newSimpleFileInfo.recordLength) )
				{
					result = ACS_CC_FAILURE;

				  	// Assemble the formated error message
					boost::format errorMsg(errorText::invalidAttribute);
					errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

					errorMsg % cpf_imm::recordLengthAttribute % newSimpleFileInfo.recordLength % cpf_imm::SimpleFileObjectName % objectName;

					setExitCode(static_cast<int>(FMS_CPF_Exception::UNREASONABLE), errorMsg.str());

					TRACE(fms_cpf_oi_simpleFileTrace, "create(...), record length not valid:<%d>", newSimpleFileInfo.recordLength );
					break;
				}

				continue;
			}
		}

		// Validate file values
		if( ACS_CC_SUCCESS == result )
		{
			TRACE(fms_cpf_oi_simpleFileTrace, "%s", "create(...), create a new composite file");
			m_simpleFileOperationTable.insert(pair<ACS_APGCC_CcbId, simpleFileInfo>(ccbId, newSimpleFileInfo));
		}
	}
	else
	{
		CPF_Log.Write("FMS_CPF_OI_SimpleFile::create, error call on not implemented class", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_oi_simpleFileTrace, "%s", "create(...), error call on not implemented class");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());

		result = ACS_CC_FAILURE;
	}

	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Leaving create(...)");
	return result;
}

/*============================================================================
	ROUTINE: deleted
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_SimpleFile::deleted(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName)
{
	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Entering deleted(...)");

	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	ACS_CC_ReturnType result = ACS_CC_FAILURE;
	simpleFileInfo delSimpleFileInfo;

	delSimpleFileInfo.action = Delete;
	delSimpleFileInfo.completed = false;

	if(getSimpleFileName(objName, delSimpleFileInfo.fileName) &&
			getCpName(objName, delSimpleFileInfo.cpName)	)
	{
		result = ACS_CC_SUCCESS;
		m_simpleFileOperationTable.insert(pair<ACS_APGCC_CcbId, simpleFileInfo>(ccbId, delSimpleFileInfo));
		TRACE(fms_cpf_oi_simpleFileTrace,"%s","deleted(...), delete a simple file");
	}
	else
	{
		TRACE(fms_cpf_oi_simpleFileTrace, "%s", "deleted(...), error on get simple file info");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());
	}

	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Leaving deleted(...)");
	return result;
}

/*============================================================================
	ROUTINE: modify
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_SimpleFile::modify(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName, ACS_APGCC_AttrModification **attrMods)
{
	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Entering modify(...)");
	ACS_CC_ReturnType result = ACS_CC_FAILURE;

	// To avoid warning about unused parameter
	UNUSED(oiHandle);
	UNUSED(ccbId);
	UNUSED(attrMods);

	std::string fileDN(objName);
	std::string objectName;
	// get the objectName from RDN
	getLastFieldValue(fileDN, objectName, false);

	// Assemble the formated error message
	boost::format errorMsg(errorText::attributeChange);
	errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

	errorMsg % cpf_imm::recordLengthAttribute % cpf_imm::SimpleFileObjectName % objectName;

	setExitCode(errorText::errorValue, errorMsg.str());

	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Leaving modify(...)");
	return result;
}

/*============================================================================
	ROUTINE: complete
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_SimpleFile::complete(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Entering complete(...)");

	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	ACS_CC_ReturnType result = ACS_CC_SUCCESS;

	FMS_CPF_Exception::errorType cmdExeResult;
	std::string errorDetail;

	// find all operations related to the ccbid
	std::pair<operationTable::iterator, operationTable::iterator> operationRange;
	operationRange = m_simpleFileOperationTable.equal_range(ccbId);

	//for each operation found
	operationTable::iterator element;
	for(element = operationRange.first; (element != operationRange.second) && (result == ACS_CC_SUCCESS); ++element)
	{
		ACE_Future<FMS_CPF_PrivateException> waitResult;
		FMS_CPF_PrivateException operationResult;

		switch(element->second.action)
		{
			case Create :
			{
				CPF_CreateFile_Request::cpFileData fileInfo;

				// set all file composite attribute
				fileInfo.fileType = FMS_CPF_Types::ft_REGULAR;
				fileInfo.composite = false;
				fileInfo.recordLength = element->second.recordLength;
				fileInfo.fileName = element->second.fileName;
				fileInfo.volumeName = element->second.volumeName;
				fileInfo.fileDN = element->second.fileDN;

				// will be empty string in SCP
				fileInfo.cpName = element->second.cpName;

				TRACE(fms_cpf_oi_simpleFileTrace, "%s", "complete(), enqueue create request to the CMD Handler");

				// create the file creation request
				CPF_CreateFile_Request* createFile = new CPF_CreateFile_Request(waitResult, fileInfo);

				m_CmdHandler->enqueue(createFile);

				TRACE(fms_cpf_oi_simpleFileTrace, "%s","complete(), wait request execution");
				waitResult.get(operationResult);

				cmdExeResult = operationResult.errorCode();

				// checks the cmd execution result
				if(cmdExeResult != FMS_CPF_Exception::OK )
				{
					result = ACS_CC_FAILURE;
					errorDetail = operationResult.errorText();
					errorDetail += operationResult.detailInfo();
					TRACE(fms_cpf_oi_simpleFileTrace, "%s", "complete(), error on simple file creation");
				}
				else
				{
					// File has been created
					element->second.completed = true;
					TRACE(fms_cpf_oi_simpleFileTrace, "%s", "complete(), simple file created");
				}
			}
			break;

			case Delete :
			{
				CPF_DeleteFile_Request::deleteFileData fileInfo;

				fileInfo.composite = false;
				fileInfo.fileType = FMS_CPF_Types::ft_REGULAR;
				fileInfo.fileName = element->second.fileName;
				fileInfo.cpName = element->second.cpName;

				TRACE(fms_cpf_oi_simpleFileTrace, "%s","complete(), delete request");

				// create the file deletion request
				CPF_DeleteFile_Request* deleteFile = new CPF_DeleteFile_Request(fileInfo);

				// Acquire exclusive file access for delete
				if( deleteFile->acquireFileLock(operationResult) )
				{
					m_deleteOperationList.push_back(deleteFile);
					// File has been locked for deleted
					element->second.completed = true;
					TRACE(fms_cpf_oi_simpleFileTrace, "%s", "complete(), simple file deleted");
				}
				else
				{
					result = ACS_CC_FAILURE;
					errorDetail = operationResult.errorText();
					errorDetail += operationResult.detailInfo();
					TRACE(fms_cpf_oi_simpleFileTrace, "%s", "complete(), error on simple file deletion");
				}
			}
			break;

			case Modify :
			{
				result = ACS_CC_FAILURE;
				TRACE(fms_cpf_oi_simpleFileTrace, "%s", "complete(), simpleFile change is not possible");
			}
			break;

			default :
				TRACE(fms_cpf_oi_simpleFileTrace, "%s", "complete(), simpleFile IMM action unknown");
		}
	}

	// set the exit code to the caller
	setExitCode(static_cast<int>(cmdExeResult), errorDetail);

	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Leaving complete(...)");
	return result;
}

/*============================================================================
	ROUTINE: abort
 ============================================================================ */
void FMS_CPF_OI_SimpleFile::abort(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Entering abort(...)");
	// To avoid warning about unused parameter
	UNUSED(oiHandle);
	// find all operations related to the ccbid
	std::pair<operationTable::iterator, operationTable::iterator> operationRange;
	operationRange = m_simpleFileOperationTable.equal_range(ccbId);

	//for each operation found
	operationTable::iterator element;
	for(element = operationRange.first; (element != operationRange.second); ++element)
	{
		ACE_Future<FMS_CPF_PrivateException> waitResult;
		FMS_CPF_PrivateException operationResult;

		// Check if the operation is already done
		if(element->second.completed)
		{
			switch(element->second.action)
			{
				case Create :
					{
						CPF_DeleteFile_Request::deleteFileData fileInfo;

						fileInfo.composite = false;
						fileInfo.fileType = FMS_CPF_Types::ft_REGULAR;
						fileInfo.fileName = element->second.fileName;
						fileInfo.cpName = element->second.cpName;

						TRACE(fms_cpf_oi_simpleFileTrace, "%s", "abort(), undo simple file creation");

						// create the file deletion object
						CPF_DeleteFile_Request* deleteFileObj = new (std::nothrow) CPF_DeleteFile_Request(fileInfo);

						if(NULL != deleteFileObj)
						{
							// Acquire exclusive file access for delete
							if( deleteFileObj->acquireFileLock(operationResult) )
							{
								// Remove the created file
								deleteFileObj->deleteFile();
							}
							delete deleteFileObj;
						}
					}
					break;

				case Delete :
					{
						TRACE(fms_cpf_oi_simpleFileTrace, "%s", "abort(), undo of simple file deletion");
					}
					break;

				case Modify :
					{
						TRACE(fms_cpf_oi_simpleFileTrace, "%s", "abort(), undo of simple file deletion");
					}
					break;
			}

			// checks the cmd result
			if( operationResult.errorCode() != FMS_CPF_Exception::OK )
			{
				TRACE(fms_cpf_oi_simpleFileTrace, "abort(), undo failed, error:<%s>", operationResult.errorText());
			}
		}
	}

	// Erase all elements from the table of the operations
	m_simpleFileOperationTable.erase(operationRange.first, operationRange.second);

	// Erase all delete operation
	std::vector<CPF_DeleteFile_Request*>::iterator delOperation;
	for(delOperation = m_deleteOperationList.begin(); delOperation != m_deleteOperationList.end(); ++delOperation)
	{
		delete (*delOperation);
	}

	m_deleteOperationList.clear();

	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Leaving abort(...)");
}

/*============================================================================
	ROUTINE: apply
 ============================================================================ */
void FMS_CPF_OI_SimpleFile::apply(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Entering apply(...)");
	// To avoid warning about unused parameter
	UNUSED(oiHandle);
	std::vector<CPF_DeleteFile_Request*>::iterator delOperation;
	CPF_DeleteFile_Request* deleteCmd;

	// Complete all delete requests, all file are already opened in exclusive mode
	for(delOperation = m_deleteOperationList.begin(); delOperation != m_deleteOperationList.end(); ++delOperation)
	{
		deleteCmd = *delOperation;
		// Physical file deletion
		deleteCmd->deleteFile();
		delete deleteCmd;
	}
	m_deleteOperationList.clear();

	// find all operations related to the ccbid
	std::pair<operationTable::iterator, operationTable::iterator> operationRange;
	operationRange = m_simpleFileOperationTable.equal_range(ccbId);

	// Erase all elements from the table of the operations
	m_simpleFileOperationTable.erase(operationRange.first, operationRange.second);

	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Leaving apply(...)");
}


/*============================================================================
ROUTINE: adminOperationCallback
============================================================================ */
void FMS_CPF_OI_SimpleFile::adminOperationCallback(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_InvocationType invocation,
		const char* p_objName, ACS_APGCC_AdminOperationIdType operationId,
		ACS_APGCC_AdminOperationParamType** paramList)
{
	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Entering adminOperationCallback(...)");

	// To avoid warning about unused parameter
	UNUSED(p_objName);
	UNUSED(operationId);
	UNUSED(paramList);
	// No actions are defined in CompositeFile class
	adminOperationResult(oiHandle, invocation, actionResult::NOOPERATION);

	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Leaving adminOperationCallback(...)");
}

/*============================================================================
	ROUTINE: updateRuntime
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_SimpleFile::updateRuntime(const char* p_objName, const char** p_attrName)
{
	TRACE(fms_cpf_oi_simpleFileTrace,"%s","Entering in updateRuntime()");

	// To avoid warning about unused parameter
	UNUSED(p_attrName);

	ACS_CC_ReturnType result = ACS_CC_SUCCESS;
	CPF_DataFile_Request::dataFile infoFile;

	// get simple file name, cp name
	if( getSimpleFileName(p_objName, infoFile.mainFileName) &&
			getCpName(p_objName, infoFile.cpName) )
	{
		ACE_Future<FMS_CPF_PrivateException> waitResult;
		FMS_CPF_PrivateException operationResult;
		FMS_CPF_Exception::errorType cmdExeResult;

		// create the file modify request
		CPF_DataFile_Request* dataFile = new CPF_DataFile_Request(waitResult, infoFile);

		m_CmdHandler->enqueue(dataFile);

		TRACE(fms_cpf_oi_simpleFileTrace, "%s", "updateRuntime(), wait datafile cmd execution");
		waitResult.get(operationResult);

		cmdExeResult = operationResult.errorCode();

		// checks the cmd execution result
		if(FMS_CPF_Exception::OK != cmdExeResult )
		{
			result = ACS_CC_FAILURE;
			std::string errMsg(operationResult.errorText());
			errMsg += operationResult.detailInfo();
			setExitCode(static_cast<int>(cmdExeResult), errMsg);
			TRACE(fms_cpf_oi_simpleFileTrace, "%s","updateRuntime(), error on simple file update");
		}
		else
		{
			// Set the File Size
			ACS_CC_ImmParameter paramSize;
			paramSize.attrName = cpf_imm::sizeAttribute;
			paramSize.attrType = ATTR_UINT32T;
			paramSize.attrValuesNum = 1;
			void* size[1] = { reinterpret_cast<void*>(&(infoFile.fileSize)) };
			paramSize.attrValues = size;

			TRACE(fms_cpf_oi_simpleFileTrace,"updateRuntime(), update size=%d",infoFile.fileSize );
			modifyRuntimeObj(p_objName, &paramSize);

			// Set the numOfReaders on File
			ACS_CC_ImmParameter paramNumReaders;
			paramNumReaders.attrName = cpf_imm::numReadersAttribute;
			paramNumReaders.attrType = ATTR_UINT32T;
			paramNumReaders.attrValuesNum = 1;
			void* numReaders[1] = { reinterpret_cast<void*>(&(infoFile.numOfReaders)) };
			paramNumReaders.attrValues = numReaders;

			TRACE(fms_cpf_oi_simpleFileTrace,"updateRuntime(), update numOfReaders=%d", infoFile.numOfReaders );
			modifyRuntimeObj(p_objName, &paramNumReaders);

			// Set the numOfWriters on File
			ACS_CC_ImmParameter paramNumWriters;
			paramNumWriters.attrName = cpf_imm::numWritersAttribute;
			paramNumWriters.attrType = ATTR_UINT32T;
			paramNumWriters.attrValuesNum = 1;
			void* numWriters[1] = { reinterpret_cast<void*>(&(infoFile.numOfWriters)) };
			paramNumWriters.attrValues = numWriters;

			TRACE(fms_cpf_oi_simpleFileTrace,"updateRuntime(), update numOfWriters=%d", infoFile.numOfWriters );
			modifyRuntimeObj(p_objName, &paramNumWriters);

			// Set the exclusiveAccess on File
			ACS_CC_ImmParameter paramExcAccess;
			paramExcAccess.attrName = cpf_imm::exclusiveAccessAttribute;
			paramExcAccess.attrType = ATTR_INT32T;
			paramExcAccess.attrValuesNum = 1;
			void* excAccess[1] = { reinterpret_cast<void*>(&(infoFile.exclusiveAccess)) };
			paramExcAccess.attrValues = excAccess;

			TRACE(fms_cpf_oi_simpleFileTrace, "updateRuntime(), update exclusiveAccess=<%d>", infoFile.exclusiveAccess);
			modifyRuntimeObj(p_objName, &paramExcAccess);

			setExitCode(SUCCESS);
		}
	}
	else
	{
		result = ACS_CC_FAILURE;
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());

		TRACE(fms_cpf_oi_simpleFileTrace, "%s", "updateRuntime(...), error on get simple file info");
	}

	TRACE(fms_cpf_oi_simpleFileTrace, "%s", "Leaving updateRuntime");

	return result;
}

/*============================================================================
	ROUTINE: ~FMS_CPF_OI_SimpleFile
 ============================================================================ */
FMS_CPF_OI_SimpleFile::~FMS_CPF_OI_SimpleFile()
{
	m_simpleFileOperationTable.clear();

	if(NULL != fms_cpf_oi_simpleFileTrace)
		delete fms_cpf_oi_simpleFileTrace;
}
