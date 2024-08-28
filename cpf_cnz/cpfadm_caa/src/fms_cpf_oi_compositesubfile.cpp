/*
 * * @file fms_cpf_oi_compositefile.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_OI_CompositeFile.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_oi_compositefile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-30
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
 *	| 1.0.0  | 2011-06-30 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_oi_compositesubfile.h"
#include "fms_cpf_createfile.h"
#include "fms_cpf_deletefile.h"
#include "fms_cpf_changefileattribute.h"
#include "fms_cpf_datafile.h"
#include "fms_cpf_common.h"

#include "fms_cpf_privateexception.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"
#include "acs_apgcc_omhandler.h"

#include <boost/format.hpp>

extern ACS_TRA_Logging CPF_Log;

namespace compositeSubFileClass{
    const char ImmImplementerName[] = "CPF_OI_CompositeSubFile";
}

/*============================================================================
	ROUTINE: FMS_CPF_OI_CompositeFile
 ============================================================================ */
FMS_CPF_OI_CompositeSubFile::FMS_CPF_OI_CompositeSubFile(FMS_CPF_CmdHandler* cmdHandler) : FMS_CPF_OI_BaseObject(cmdHandler, cpf_imm::CompositeSubFileClassName) ,
		acs_apgcc_objectimplementerinterface_V3(compositeSubFileClass::ImmImplementerName)
{
	fms_cpf_oi_compSubFileTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_OI_CompositeSubFile");
}

/*============================================================================
	ROUTINE: create
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CompositeSubFile::create(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *className, const char* parentname, ACS_APGCC_AttrValues **attr)
{
	TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Entering in create(...) callback");

	ACS_CC_ReturnType result = ACS_CC_SUCCESS;
	// To avoid warning about unused parameter
	UNUSED(oiHandle);
	TRACE(fms_cpf_oi_compSubFileTrace, "create(...), insert a composite subfile under parent=<%s>", parentname);

	compositeSubFileInfo newCompSubFileInfo;
	//initiate values
	newCompSubFileInfo.completed = false;
	newCompSubFileInfo.compression = false;
	newCompSubFileInfo.action = Create;

	// Get composite file name and cpName by the parent DN
	if(!getCpName(parentname, newCompSubFileInfo.cpName) ||
			!getCompositeFileName(parentname, newCompSubFileInfo.mainFileName) )
	{
		CPF_Log.Write("FMS_CPF_OI_CompositeSubFile::create, error on get parent info", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_oi_compSubFileTrace, "%s", "create(...), error on get parent info");
		result = ACS_CC_FAILURE;
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT),exErr.errorText());

		TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Leaving create(...)");
		return result;
	}

	// Check the IMM class name
	if( 0 == ACE_OS::strcmp(m_ImmClassName.c_str(), className ) )
	{
		// extract the attributes
		for(size_t idx = 0; attr[idx] != NULL; ++idx)
		{
			// check if RDN attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::CompositeSubFileKey, attr[idx]->attrName) )
			{
				std::string fileRDN = reinterpret_cast<char *>(attr[idx]->attrValues[0]);
				// get the fileName from RDN
				getLastFieldValue(fileRDN, newCompSubFileInfo.subFileName, false);

				std::string objectName(newCompSubFileInfo.subFileName);

				ACS_APGCC::toUpper(newCompSubFileInfo.subFileName);

				// Validate the value
				if(!checkSubFileName(newCompSubFileInfo.subFileName) )
				{
					result = ACS_CC_FAILURE;
					// Assemble the formated error message
					boost::format errorMsg(errorText::invalidFileName);
					errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

					errorMsg % cpf_imm::CompositeSubFileObjectName % objectName;

					setExitCode(static_cast<int>(FMS_CPF_Exception::INVALIDFILE), errorMsg.str() );

					TRACE(fms_cpf_oi_compSubFileTrace, "create(...), file name not valid:<%s>", objectName.c_str());
					break;
				}
				continue;
			}

		}
		// Validate file values
		if(  ACS_CC_SUCCESS == result )
		{
			TRACE(fms_cpf_oi_compSubFileTrace,"%s","create(...), create a new composite sub file");
			m_compositeSubFileOperationTable.insert(pair<ACS_APGCC_CcbId, compositeSubFileInfo>(ccbId, newCompSubFileInfo));
		}
	}
	else
	{
		CPF_Log.Write("FMS_CPF_OI_CompositeSubFile::create, error call on not implemented class", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_oi_compSubFileTrace, "%s", "create(...), error call on not implemented class");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT),exErr.errorText());

		result = ACS_CC_FAILURE;
	}

	TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Leaving create(...)");
	return result;
}
/*============================================================================
	ROUTINE: deleted
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CompositeSubFile::deleted(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName)
{
	TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Entering deleted(...)");
	ACS_CC_ReturnType result = ACS_CC_FAILURE;

	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	compositeSubFileInfo delSubFileInfo;

	delSubFileInfo.action = Delete;
	delSubFileInfo.completed = false;

	if( getCompositeFileName(objName, delSubFileInfo.mainFileName) &&
		getSubCompositeFileName(objName, delSubFileInfo.subFileName) &&
		getCpName(objName, delSubFileInfo.cpName) )
	{
		result = ACS_CC_SUCCESS;
		m_compositeSubFileOperationTable.insert(pair<ACS_APGCC_CcbId, compositeSubFileInfo>(ccbId, delSubFileInfo));
		TRACE(fms_cpf_oi_compSubFileTrace,"%s","deleted(...), delete a composite subfile");
	}
	else
	{
		TRACE(fms_cpf_oi_compSubFileTrace,"%s","deleted(...), error on get composite subfile info");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());
	}
	TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Leaving deleted(...)");
	return result;
}

/*============================================================================
	ROUTINE: modify
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CompositeSubFile::modify(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName, ACS_APGCC_AttrModification **attrMods)
{
	TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Entering modify(...)");

	ACS_CC_ReturnType result = ACS_CC_FAILURE;
	// To avoid warning about unused parameter
	UNUSED(oiHandle);
	UNUSED(ccbId);
	UNUSED(objName);
	UNUSED(attrMods);

	FMS_CPF_Exception exErr(FMS_CPF_Exception::PARAMERROR);
	setExitCode(static_cast<int>(FMS_CPF_Exception::PARAMERROR), exErr.errorText());


	TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Leaving modify(...)");
	return result;
}

/*============================================================================
	ROUTINE: complete
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CompositeSubFile::complete(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Entering complete(...)");

	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	ACS_CC_ReturnType result = ACS_CC_SUCCESS;

	FMS_CPF_Exception::errorType cmdExeResult;
	std::string errorDetail;

	// find all operations related to the ccbid
	std::pair<operationTable::iterator, operationTable::iterator> operationRange;
	operationRange = m_compositeSubFileOperationTable.equal_range(ccbId);

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

				// set all sub file composite attribute
				fileInfo.fileType = FMS_CPF_Types::ft_REGULAR;
				fileInfo.composite = false;
				fileInfo.fileName = element->second.mainFileName;
				fileInfo.subFileName = element->second.subFileName;

				// will be empty string in SCP
				fileInfo.cpName = element->second.cpName;

				TRACE(fms_cpf_oi_compSubFileTrace, "%s", "complete(), enqueue create request to the CMD Handler");

				// create the file creation request
				CPF_CreateFile_Request* createFile = new CPF_CreateFile_Request(waitResult, fileInfo);

				// enqueue create request
				m_CmdHandler->enqueue(createFile);

				TRACE(fms_cpf_oi_compSubFileTrace, "%s", "complete(), wait request execution");

				// wait that create request is handled
				waitResult.get(operationResult);

				cmdExeResult = operationResult.errorCode();

				// checks the cmd execution result
				if(cmdExeResult == FMS_CPF_Exception::OK )
				{
					// File has been created
					element->second.completed = true;
					TRACE(fms_cpf_oi_compSubFileTrace, "%s", "complete(), composite subfile created");
				}
				else
				{
					// Error on subfile creation
					result = ACS_CC_FAILURE;
					errorDetail = operationResult.errorText();
					errorDetail += operationResult.detailInfo();
					TRACE(fms_cpf_oi_compSubFileTrace, "%s", "complete(), error on composite subfile creation");
				}
			}
			break;

			case Delete:
			{
				CPF_DeleteFile_Request::deleteFileData fileInfo;

				std::string completeSubFileName = element->second.mainFileName;
				completeSubFileName += (SubFileSep + element->second.subFileName);
				fileInfo.fileName = completeSubFileName;
				fileInfo.composite = false;
				fileInfo.fileType = FMS_CPF_Types::ft_REGULAR;
				fileInfo.cpName = element->second.cpName;

				TRACE(fms_cpf_oi_compSubFileTrace, "%s", "complete(), delete request");

				// create the file creation request
				CPF_DeleteFile_Request* deleteSubFile = new CPF_DeleteFile_Request(fileInfo);

				// Acquire exclusive file access for delete
				if( deleteSubFile->acquireFileLock(operationResult) )
				{
					m_deleteOperationList.push_back(deleteSubFile);
					// File has been locked for deleted
					element->second.completed = true;
					TRACE(fms_cpf_oi_compSubFileTrace, "%s", "complete(), composite subfile deleted");
				}
				else
				{
					result = ACS_CC_FAILURE;
					errorDetail = operationResult.errorText();
					errorDetail += operationResult.detailInfo();
					TRACE(fms_cpf_oi_compSubFileTrace, "%s", "complete(), error on composite subfile deletion");
				}
			}
			break;

			case Modify:
			{
				TRACE(fms_cpf_oi_compSubFileTrace, "%s", "complete(), composite subfile change is not possible");
			}
			break;

			default:
				TRACE(fms_cpf_oi_compSubFileTrace, "%s", "complete(), compositeSubFile IMM action unknown");
		}
	}


	// set the exit code to the caller
	setExitCode(static_cast<int>(cmdExeResult), errorDetail);

	TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Leaving complete(...)");
	return result;
}


/*============================================================================
	ROUTINE: abort
 ============================================================================ */
void FMS_CPF_OI_CompositeSubFile::abort(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Entering abort(...)");
	// To avoid warning about unused parameter
	UNUSED(oiHandle);
	// find all operations related to the ccbid
	std::pair<operationTable::iterator, operationTable::iterator> operationRange;
	operationRange = m_compositeSubFileOperationTable.equal_range(ccbId);

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
						std::string completeSubFileName = element->second.mainFileName;
						completeSubFileName += (SubFileSep + element->second.subFileName);
						fileInfo.fileName = completeSubFileName;
						fileInfo.composite = false;
						fileInfo.fileType = FMS_CPF_Types::ft_REGULAR;
						fileInfo.cpName = element->second.cpName;

						TRACE(fms_cpf_oi_compSubFileTrace, "%s", "abort(), undo of composite file creation");

						// create the file deletion object
						CPF_DeleteFile_Request* deleteSubFileObj = new (std::nothrow) CPF_DeleteFile_Request(fileInfo);

						if( NULL != deleteSubFileObj)
						{
							// Acquire exclusive file access
							if( deleteSubFileObj->acquireFileLock(operationResult) )
							{
								// Remove the created file
								deleteSubFileObj->deleteFile();
							}

							delete deleteSubFileObj;
						}
					}
					break;

				case Delete :
					{
						TRACE(fms_cpf_oi_compSubFileTrace, "%s", "abort(), undo of composite subfile deletion");
						continue;
					}
					break;

				case Modify :
					{
						TRACE(fms_cpf_oi_compSubFileTrace, "%s", "abort(), composite subfile change");
					}
					break;
			}

			// checks the cmd result
			if( operationResult.errorCode() != FMS_CPF_Exception::OK )
			{
				TRACE(fms_cpf_oi_compSubFileTrace, "abort(), undo failed, error:<%s>", operationResult.errorText());
			}
		}
	}

	// Erase all elements from the table of the operations
	m_compositeSubFileOperationTable.erase(operationRange.first, operationRange.second);

	// Erase all delete operation
	std::vector<CPF_DeleteFile_Request*>::iterator delOperation;
	for(delOperation = m_deleteOperationList.begin(); delOperation != m_deleteOperationList.end(); ++delOperation)
	{
		delete (*delOperation);
	}

	m_deleteOperationList.clear();

	TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Leaving abort(...)");
}


/*============================================================================
	ROUTINE: apply
 ============================================================================ */
void FMS_CPF_OI_CompositeSubFile::apply(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Entering apply(...)");
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
	operationRange = m_compositeSubFileOperationTable.equal_range(ccbId);

	// Erase all elements from the table of the operations
	m_compositeSubFileOperationTable.erase(operationRange.first, operationRange.second);

	TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Leaving apply(...)");
}

/*============================================================================
ROUTINE: adminOperationCallback
============================================================================ */
void FMS_CPF_OI_CompositeSubFile::adminOperationCallback(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_InvocationType invocation,
		const char* p_objName, ACS_APGCC_AdminOperationIdType operationId,
		ACS_APGCC_AdminOperationParamType** paramList)
{
	TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Entering adminOperationCallback(...)");
	// To avoid warning about unused parameter
	UNUSED(p_objName);
	UNUSED(operationId);
	UNUSED(paramList);
	// No actions are defined in CompositeSubFile class
	adminOperationResult(oiHandle, invocation, actionResult::NOOPERATION);

	TRACE(fms_cpf_oi_compSubFileTrace, "%s", "Leaving adminOperationCallback(...)");
}

/*============================================================================
	ROUTINE: updateRuntime
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CompositeSubFile::updateRuntime(const char* p_objName, const char** p_attrName)
{
	TRACE(fms_cpf_oi_compSubFileTrace,"%s","Entering in updateRuntime()");

	// To avoid warning about unused parameter
	UNUSED(p_attrName);

	ACS_CC_ReturnType result = ACS_CC_SUCCESS;
	CPF_DataFile_Request::dataFile infoFile;

	// get composite main file name, cp name and subfile name
	if( getCompositeFileName(p_objName, infoFile.mainFileName) &&
				getSubCompositeFileName(p_objName, infoFile.subFileName) &&
				getCpName(p_objName, infoFile.cpName) )
	{
		ACE_Future<FMS_CPF_PrivateException> waitResult;
		FMS_CPF_PrivateException operationResult;
		FMS_CPF_Exception::errorType cmdExeResult;

		// create the file modify request
		CPF_DataFile_Request* dataFile = new CPF_DataFile_Request(waitResult, infoFile);

		m_CmdHandler->enqueue(dataFile);

		TRACE(fms_cpf_oi_compSubFileTrace, "%s","updateRuntime(), wait datafile cmd execution");
		waitResult.get(operationResult);

		cmdExeResult = operationResult.errorCode();

		// checks the cmd execution result
		if(FMS_CPF_Exception::OK != cmdExeResult )
		{
			result = ACS_CC_FAILURE;
			std::string errMsg(operationResult.errorText());
			errMsg +=  operationResult.detailInfo();
			setExitCode(static_cast<int>(cmdExeResult), errMsg);

			TRACE(fms_cpf_oi_compSubFileTrace, "%s","updateRuntime(), error on composite subfile update");
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

			TRACE(fms_cpf_oi_compSubFileTrace, "updateRuntime(), update size=<%d>",infoFile.fileSize );
			modifyRuntimeObj(p_objName, &paramSize);

			// Set the numOfReaders on File
			ACS_CC_ImmParameter paramNumReaders;
			paramNumReaders.attrName = cpf_imm::numReadersAttribute;
			paramNumReaders.attrType = ATTR_UINT32T;
			paramNumReaders.attrValuesNum = 1;
			void* numReaders[1] = { reinterpret_cast<void*>(&(infoFile.numOfReaders)) };
			paramNumReaders.attrValues = numReaders;

			TRACE(fms_cpf_oi_compSubFileTrace, "updateRuntime(), update numOfReaders=<%d>", infoFile.numOfReaders );
			modifyRuntimeObj(p_objName, &paramNumReaders);

			// Set the numOfWriters on File
			ACS_CC_ImmParameter paramNumWriters;
			paramNumWriters.attrName = cpf_imm::numWritersAttribute;
			paramNumWriters.attrType = ATTR_UINT32T;
			paramNumWriters.attrValuesNum = 1;
			void* numWriters[1] = { reinterpret_cast<void*>(&(infoFile.numOfWriters)) };
			paramNumWriters.attrValues = numWriters;

			TRACE(fms_cpf_oi_compSubFileTrace, "updateRuntime(), update numOfWriters=<%d>", infoFile.numOfWriters );
			modifyRuntimeObj(p_objName, &paramNumWriters);

			// Set the exclusiveAccess on File
			ACS_CC_ImmParameter paramExcAccess;
			paramExcAccess.attrName = cpf_imm::exclusiveAccessAttribute;
			paramExcAccess.attrType = ATTR_INT32T;
			paramExcAccess.attrValuesNum = 1;
			void* excAccess[1] = { reinterpret_cast<void*>(&(infoFile.exclusiveAccess)) };
			paramExcAccess.attrValues = excAccess;

			TRACE(fms_cpf_oi_compSubFileTrace, "updateRuntime(), update exclusiveAccess=<%d>", infoFile.exclusiveAccess);
			modifyRuntimeObj(p_objName, &paramExcAccess);

			setExitCode(SUCCESS);
		}
	}
	else
	{
		result = ACS_CC_FAILURE;
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());

		TRACE(fms_cpf_oi_compSubFileTrace,"%s","modify(...), error on get composite subfile info");
	}

	TRACE(fms_cpf_oi_compSubFileTrace,"%s","updateRuntime(), Leaving updateRuntime");

	return result;
}

/*============================================================================
	ROUTINE: ~FMS_CPF_OI_CompositeSubFile
 ============================================================================ */
FMS_CPF_OI_CompositeSubFile::~FMS_CPF_OI_CompositeSubFile()
{
	m_compositeSubFileOperationTable.clear();

	if(NULL != fms_cpf_oi_compSubFileTrace)
		delete fms_cpf_oi_compSubFileTrace;
}
