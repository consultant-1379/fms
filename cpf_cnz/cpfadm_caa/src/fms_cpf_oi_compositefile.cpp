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

#include "fms_cpf_oi_compositefile.h"
#include "fms_cpf_createfile.h"
#include "fms_cpf_deletefile.h"
#include "fms_cpf_changefileattribute.h"
#include "fms_cpf_datafile.h"
#include "fms_cpf_common.h"
#include "fms_cpf_jtpconnectionhndl.h"

#include "fms_cpf_privateexception.h"
#include "fms_cpf_eventalarmhndl.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"

#include <boost/format.hpp>

#include <sstream>
#include <list>

extern ACS_TRA_Logging CPF_Log;

namespace compositeFileClass{

	const char ImmImplementerName[] = "CPF_OI_CompositeFile";
}

// Range of deleteFileTimer parameter
namespace TimeRange
{
	const short MinValue = -1;
	const int MaxValue = 20160;
}




/*============================================================================
	ROUTINE: FMS_CPF_OI_CompositeFile
 ============================================================================ */
FMS_CPF_OI_CompositeFile::FMS_CPF_OI_CompositeFile(FMS_CPF_CmdHandler* cmdHandler) : FMS_CPF_OI_BaseObject(cmdHandler, cpf_imm::CompositeFileClassName) ,
	acs_apgcc_objectimplementerinterface_V3(compositeFileClass::ImmImplementerName)
{
	fms_cpf_oi_compFileTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_OI_CompositeFile");
}

/*============================================================================
	ROUTINE: create
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CompositeFile::create(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *className, const char* parentname, ACS_APGCC_AttrValues **attr)
{
	TRACE(fms_cpf_oi_compFileTrace, "%s", "Entering in create(...) callback");

	ACS_CC_ReturnType result = ACS_CC_SUCCESS;
	// To avoid warning about unused parameter
	UNUSED(oiHandle);
	TRACE(fms_cpf_oi_compFileTrace, "create(...), insert a composite file under parent=<%s>", parentname);

	compositeFileInfo newCompFileInfo;

	//initiate values
	newCompFileInfo.compression = false;
	newCompFileInfo.recordLength = 0;
	newCompFileInfo.fileName = "";
	newCompFileInfo.completed = false;
	newCompFileInfo.action = Create;
        newCompFileInfo.deleteFileTimer = -1;

	// Get volume and cpName by the parent DN
	if(!getVolumeName(parentname, newCompFileInfo.volumeName) ||
			!getCpName(parentname, newCompFileInfo.cpName)	)
	{
		CPF_Log.Write("FMS_CPF_OI_CompositeFile::create, error on get parent info", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_oi_compFileTrace, "%s", "create(...), error on get parent info");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());
		result = ACS_CC_FAILURE;
		return result;
	}

	// Check the IMM class name
	if( 0 == ACE_OS::strcmp(m_ImmClassName.c_str(), className ) )
	{
		std::string objectName;
		// extract the attributes
		for(size_t idx = 0; (attr[idx] != NULL) && (result != ACS_CC_FAILURE) ; ++idx)
		{
                     		// check if RDN attribute to get fileName
			if( 0 == ACE_OS::strcmp(cpf_imm::CompositeFileKey, attr[idx]->attrName) )
		 	{
				std::string fileRDN = reinterpret_cast<char *>(attr[idx]->attrValues[0]);
				newCompFileInfo.fileDN = fileRDN + parseSymbol::comma + std::string(parentname);

				// get the fileName from RDN
				getLastFieldValue(fileRDN, newCompFileInfo.fileName, false);

				objectName = newCompFileInfo.fileName;

				ACS_APGCC::toUpper(newCompFileInfo.fileName);

				// Validate the value
				if(!checkFileName(newCompFileInfo.fileName) )
				{
					result = ACS_CC_FAILURE;
					// Assemble the formated error message
					boost::format errorMsg(errorText::invalidFileName);
					errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

					errorMsg % cpf_imm::CompositeFileObjectName % objectName;

					setExitCode(static_cast<int>(FMS_CPF_Exception::INVALIDFILE), errorMsg.str() );

					TRACE(fms_cpf_oi_compFileTrace, "create(...), file name not valid:<%s>", newCompFileInfo.fileName.c_str());

				}
			}
			// check if recordLength attribute
                        else if( 0 == ACE_OS::strcmp(cpf_imm::recordLengthAttribute, attr[idx]->attrName) )
                        {
                                newCompFileInfo.recordLength = *reinterpret_cast<unsigned int*>(attr[idx]->attrValues[0]);

                                if( !checkRecordLength(newCompFileInfo.recordLength) )
                                {
                                        result = ACS_CC_FAILURE;

                                        // Assemble the formated error message
                                        boost::format errorMsg(errorText::invalidAttribute);
                                        errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

                                        errorMsg % cpf_imm::recordLengthAttribute % newCompFileInfo.recordLength % cpf_imm::CompositeFileObjectName % objectName;

                                        setExitCode(static_cast<int>(FMS_CPF_Exception::UNREASONABLE), errorMsg.str());

                                        TRACE(fms_cpf_oi_compFileTrace, "create(...), record length<%d> not valid", newCompFileInfo.recordLength);
                                        break;
                                }
                        }
			else if( 0 == ACE_OS::strcmp(cpf_imm::deleteFileTimerAttribute, attr[idx]->attrName) )
                        {
				TRACE(fms_cpf_oi_compFileTrace, "%s", "create(...), create inside deleteFileTImer");
				std::string filename="RELFSW";
				TRACE(fms_cpf_oi_compFileTrace, "create(...), deleteFileTimer filename = <%s>", newCompFileInfo.fileName.c_str());
				if((((filename.compare(newCompFileInfo.fileName.substr(0,6)))==0) || (m_isMultiCP)) && (newCompFileInfo.deleteFileTimer != -1))
				{
					result = ACS_CC_FAILURE;
					TRACE(fms_cpf_oi_compFileTrace, "%s", "create(...), create inside modify deleteFileTImer");
					boost::format errorMsg(errorText::attributeChange);
					errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );
					errorMsg % cpf_imm::deleteFileTimerAttribute % newCompFileInfo.deleteFileTimer % cpf_imm::CompositeFileObjectName % objectName;
					setExitCode(static_cast<int>(FMS_CPF_Exception::ILLOPTION), errorMsg.str());
					TRACE(fms_cpf_oi_compFileTrace, "create(...), deleteFileTimer is <%d> not changable attribute", newCompFileInfo.deleteFileTimer);
					break;	
				}
				newCompFileInfo.deleteFileTimer = *reinterpret_cast<int*>(attr[idx]->attrValues[0]);
                                TRACE(fms_cpf_oi_compFileTrace, "create(...), deleteFileTimer = <%d>", newCompFileInfo.deleteFileTimer);
                        }
		}
               
		// Validate file values
		if( ACS_CC_SUCCESS == result )
		{
			TRACE(fms_cpf_oi_compFileTrace, "%s", "create(...), create a new composite file");
			m_compositeFileOperationTable.insert(pair<ACS_APGCC_CcbId, compositeFileInfo>(ccbId, newCompFileInfo));
		}
	}
	else
	{
		CPF_Log.Write("FMS_CPF_OI_CompositeFile::create, error call on not implemented class", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_oi_compFileTrace, "%s", "create(...), error call on not implemented class");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());
		result = ACS_CC_FAILURE;
	}

	TRACE(fms_cpf_oi_compFileTrace, "%s", "Leaving create(...)");
	return result;
}

/*============================================================================
	ROUTINE: deleted
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CompositeFile::deleted(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName)
{
	TRACE(fms_cpf_oi_compFileTrace, "%s", "Entering deleted(...)");

	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	ACS_CC_ReturnType result = ACS_CC_FAILURE;

	compositeFileInfo delCompFileInfo;

	delCompFileInfo.action = Delete;
	delCompFileInfo.completed = false;

	// Get file name and Cp name
	if(getCompositeFileName(objName, delCompFileInfo.fileName) &&
			getCpName(objName, delCompFileInfo.cpName)	)
	{
		result = ACS_CC_SUCCESS;
		m_compositeFileOperationTable.insert(pair<ACS_APGCC_CcbId, compositeFileInfo>(ccbId, delCompFileInfo));
		TRACE(fms_cpf_oi_compFileTrace,"%s","deleted(...), delete a composite file");
	}
	else
	{
		TRACE(fms_cpf_oi_compFileTrace,"%s","deleted(...), error on get composite file info");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());
	}

	TRACE(fms_cpf_oi_compFileTrace, "%s", "Leaving deleted(...)");
	return result;
}

/*============================================================================
	ROUTINE: modify
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CompositeFile::modify(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName, ACS_APGCC_AttrModification **attrMods)
{
	TRACE(fms_cpf_oi_compFileTrace, "%s", "Entering modify(...)");
	ACS_CC_ReturnType result = ACS_CC_SUCCESS;
        
        compositeFileInfo modCompositeFileInfo;

	// To avoid warning about unused parameter
	UNUSED(oiHandle);
	UNUSED(ccbId);
	UNUSED(attrMods);
       modCompositeFileInfo.action = Modify;
	modCompositeFileInfo.changeMask = 0;
	modCompositeFileInfo.deleteFileTimer = -1;
        modCompositeFileInfo.completed = false;
    
        if(getCompositeFileName(objName, modCompositeFileInfo.fileName) &&
			getCpName(objName, modCompositeFileInfo.cpName)	)
			{
			std::string fileDN(objName);
                	std::string objectName;
                	// get the objectName from RDN
                 	getLastFieldValue(fileDN, objectName, false);
 	
               	  // extract the attributes to modify
              for(size_t idx = 0; attrMods[idx] != NULL ; idx++)
	      {
			ACS_APGCC_AttrValues modAttribute = attrMods[idx]->modAttr;

			// check if recordLength attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::recordLengthAttribute, modAttribute.attrName) )
			{
				// change not allowed
				result = ACS_CC_FAILURE;
				// Assemble the formated error message
                                boost::format errorMsg(errorText::attributeChange);
             			errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );       
                                errorMsg % cpf_imm::recordLengthAttribute % cpf_imm::CompositeFileObjectName % objectName;
	                        setExitCode(errorText::errorValue, errorMsg.str());
	                        TRACE(fms_cpf_oi_compFileTrace, "%s", "modify(...), record length change not allowed");
	                      break;
	                }  
	  
	           //check deletefiletimer attribute
               	   if( 0 == ACE_OS::strcmp(cpf_imm::deleteFileTimerAttribute, modAttribute.attrName) )
	           {
				std::string filename="RELFSW";
				if(((filename.compare(modCompositeFileInfo.fileName.substr(0,6)))==0) || m_isMultiCP)
				{
					result = ACS_CC_FAILURE;
					boost::format errorMsg(errorText::attributeChange);
					errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );
                                	errorMsg % cpf_imm::deleteFileTimerAttribute % cpf_imm::CompositeFileObjectName % objectName;
					setExitCode(static_cast<int>(FMS_CPF_Exception::ILLOPTION), errorMsg.str());
					TRACE(fms_cpf_oi_compFileTrace, "%s", "modify(...), deleteFileTimer change not allowed");
					break;
				}
	                       	modCompositeFileInfo.deleteFileTimer= *reinterpret_cast<int*>(modAttribute.attrValues[0]);
                               TRACE(fms_cpf_oi_compFileTrace,"modify callback deletimervalue change to :<%d>",modCompositeFileInfo.deleteFileTimer);
	                       if( (modCompositeFileInfo.deleteFileTimer > TimeRange::MaxValue) )
				{
					result = ACS_CC_FAILURE;
					// Assemble the formated error message
                                        boost::format errorMsg(errorText::attributeChange);
			                errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );
                                        errorMsg % cpf_imm::deleteFileTimerAttribute % cpf_imm::CompositeFileObjectName % objectName;
	                                setExitCode(errorText::errorValue, errorMsg.str());
	                                TRACE(fms_cpf_oi_compFileTrace, "%s", "modify(...), deletefiletimer parameter not valid ");
		                	break;
				}
					// Set the flag for deletefiletimer change
				modCompositeFileInfo.changeMask |= changeSet::DELETEFILETIMER_CHANGE;  
				TRACE(fms_cpf_oi_compFileTrace, "%s", "modify(...), composite file deletefiletimer change");
				TRACE(fms_cpf_oi_compFileTrace,"modify callback deletimervalue change to :<%d>",modCompositeFileInfo.deleteFileTimer);
				continue;
	           }
			
		}	
		if(ACS_CC_FAILURE != result )
		{
			m_compositeFileOperationTable.insert(pair<ACS_APGCC_CcbId, compositeFileInfo>(ccbId, modCompositeFileInfo));
			TRACE(fms_cpf_oi_compFileTrace, "%s", "modify(...),  modify attributes of a composite file");
		}
           }	
        else
	{
		TRACE(fms_cpf_oi_compFileTrace,"%s","deleted(...), error on get composite file info");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());
	}
     
	TRACE(fms_cpf_oi_compFileTrace, "%s", "Leaving modify(...)");
	return result;
}

/*============================================================================
	ROUTINE: complete
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CompositeFile::complete(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_compFileTrace, "%s", "Entering complete(...)");

	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	ACS_CC_ReturnType result = ACS_CC_SUCCESS;

	FMS_CPF_Exception::errorType cmdExeResult;
	std::string errorDetail;

	// find all operations related to the ccbid
	std::pair<operationTable::iterator, operationTable::iterator> operationRange;
	operationRange = m_compositeFileOperationTable.equal_range(ccbId);

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
				fileInfo.composite = true;
				fileInfo.recordLength = element->second.recordLength;
				fileInfo.fileName = element->second.fileName;
				fileInfo.volumeName = element->second.volumeName;
				fileInfo.fileDN = element->second.fileDN;
                                fileInfo.deleteFileTimer = element->second.deleteFileTimer;
				// will be empty string in SCP
				fileInfo.cpName = element->second.cpName;

				TRACE(fms_cpf_oi_compFileTrace, "%s", "complete(), enqueue create request to the CMD Handler");

				// create the file creation request
				CPF_CreateFile_Request* createFile = new CPF_CreateFile_Request(waitResult, fileInfo);

				m_CmdHandler->enqueue(createFile);

				TRACE(fms_cpf_oi_compFileTrace, "%s","complete(), wait cmd execution");
				waitResult.get(operationResult);

				cmdExeResult = operationResult.errorCode();

				// checks the cmd execution result
				if(cmdExeResult != FMS_CPF_Exception::OK )
				{
					result = ACS_CC_FAILURE;
					errorDetail = operationResult.errorText();
					errorDetail += operationResult.detailInfo();
					TRACE(fms_cpf_oi_compFileTrace, "%s","complete(), error on composite file creation");
				}
				else
				{
					// File has been created
					element->second.completed = true;
					if(!m_isMultiCP)
					{
						FMS_CPF_JTPConnectionHndl::updateDeleteFileTimer(fileInfo.fileName, fileInfo.deleteFileTimer);
						TRACE(fms_cpf_oi_compFileTrace, "%s","complete(), composite file created");
					}
				}
			}
			break;

			case Delete :
			{
				CPF_DeleteFile_Request::deleteFileData fileInfo;

				fileInfo.composite = true;
				fileInfo.fileType = FMS_CPF_Types::ft_REGULAR;
				fileInfo.fileName = element->second.fileName;
				fileInfo.cpName = element->second.cpName;

				TRACE(fms_cpf_oi_compFileTrace, "%s", "complete(), delete request");

				// create the file deletion request
				CPF_DeleteFile_Request* deleteFile = new CPF_DeleteFile_Request(fileInfo);

				// Acquire exclusive file access for delete
				if( deleteFile->acquireFileLock(operationResult) )
				{
					m_deleteOperationList.push_back(deleteFile);
					// File has been locked for deleted
					element->second.completed = true;
					TRACE(fms_cpf_oi_compFileTrace, "%s", "complete(), composite file deleted");
				}
				else
				{
					result = ACS_CC_FAILURE;
					errorDetail = operationResult.errorText();
					errorDetail += operationResult.detailInfo();
					TRACE(fms_cpf_oi_compFileTrace, "%s", "complete(), error on composite file deletion");
				}
			}
			break;

			case Modify :
			{
			        CPF_ChangeFileAttribute_Request::cpFileModData fileInfo;
				fileInfo.fileType = FMS_CPF_Types::ft_REGULAR;
				fileInfo.cpName = element->second.cpName;
				fileInfo.currentFileName = element->second.fileName;
				
				fileInfo.newdeleteFileTimer = element->second.deleteFileTimer;
				fileInfo.changeFlags = element->second.changeMask;
				
				TRACE(fms_cpf_oi_compFileTrace, "%s", "complete(), enqueue change request to the CMD Handler");

				// create the file modify request
				CPF_ChangeFileAttribute_Request* modifyFile = new CPF_ChangeFileAttribute_Request(waitResult, fileInfo);
				TRACE(fms_cpf_oi_compFileTrace,"modify frunction complete  deletimervalue change to :<%d>",fileInfo.newdeleteFileTimer);
				 // The cmd request object will be deleted by the cmd handler
				m_CmdHandler->enqueue(modifyFile);

				TRACE(fms_cpf_oi_compFileTrace, "%s", "complete(), wait request execution");
				waitResult.get(operationResult);

				cmdExeResult = operationResult.errorCode();
				
				// checks the cmd request
				if( cmdExeResult != FMS_CPF_Exception::OK )
				{
					result = ACS_CC_FAILURE;
					errorDetail = operationResult.errorText();
					errorDetail += operationResult.detailInfo();
					TRACE(fms_cpf_oi_compFileTrace, "%s","complete(), error on composite file modify");
				}
				
				else
				{
					// For undo operation
					element->second.deleteFileTimer = fileInfo.newdeleteFileTimer;
					element->second.completed = true;					
                                        TRACE(fms_cpf_oi_compFileTrace,"modify frunction  deletimervalue change to :<%d>",fileInfo.newdeleteFileTimer);
				         // File has been changed
                                      	FMS_CPF_JTPConnectionHndl::updateDeleteFileTimer(fileInfo.currentFileName, fileInfo.newdeleteFileTimer);
					TRACE(fms_cpf_oi_compFileTrace, "%s", "complete(), composite file modified");	
				}
			}
			break;

			default :
				TRACE(fms_cpf_oi_compFileTrace, "%s", "complete(), compositeFile IMM action unknown");
		}
	}

	// set the exit code to the caller
	setExitCode(static_cast<int>(cmdExeResult), errorDetail);

	TRACE(fms_cpf_oi_compFileTrace, "%s", "Leaving complete(...)");
	return result;
}

/*============================================================================
	ROUTINE: abort
 ============================================================================ */
void FMS_CPF_OI_CompositeFile::abort(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_compFileTrace, "%s", "Entering abort(...)");
	// To avoid warning about unused parameter
	UNUSED(oiHandle);
	// find all operations related to the ccbid
	std::pair<operationTable::iterator, operationTable::iterator> operationRange;
	operationRange = m_compositeFileOperationTable.equal_range(ccbId);

	//for each operation found
	operationTable::iterator element;
	for(element = operationRange.first; (element != operationRange.second); ++element)
	{
		ACE_Future<FMS_CPF_PrivateException> waitResult;
		FMS_CPF_PrivateException operationResult;

		// Check if the operation is already done
		if(true == element->second.completed)
		{
			switch(element->second.action)
			{
				case Create :
					{
						CPF_DeleteFile_Request::deleteFileData fileInfo;

						fileInfo.composite = true;
						fileInfo.fileType = FMS_CPF_Types::ft_REGULAR;
						fileInfo.fileName = element->second.fileName;
						fileInfo.cpName = element->second.cpName;

						TRACE(fms_cpf_oi_compFileTrace, "%s", "abort(), undo of composite file creation");

						// create the file deletion object
						CPF_DeleteFile_Request* deleteFileObj = new (std::nothrow) CPF_DeleteFile_Request(fileInfo);

						if(NULL != deleteFileObj)
						{
							// Acquire exclusive file access
							if( deleteFileObj->acquireFileLock(operationResult) )
							{
								// Remove the created file
								deleteFileObj->deleteFile();
							}
							delete deleteFileObj;
						 }
						if(!m_isMultiCP)
						{
						 	FMS_CPF_JTPConnectionHndl::removeDeleteFileTimer(fileInfo.fileName);
						}
					}
					break;

				case Delete :
					{
						TRACE(fms_cpf_oi_compFileTrace, "%s", "abort(), undo of composite file deletion");
						continue;
					}
					break;

				case Modify :
					{
						CPF_ChangeFileAttribute_Request::cpFileModData fileInfo;
                                                fileInfo.fileType = FMS_CPF_Types::ft_REGULAR;
                                                fileInfo.cpName = element->second.cpName;
                                                fileInfo.currentFileName = element->second.fileName;
                                                fileInfo.newdeleteFileTimer = element->second.deleteFileTimer;
                                                fileInfo.changeFlags = element->second.changeMask;
                                                TRACE(fms_cpf_oi_compFileTrace, "%s","abort(), undo composite file");
						// create the file modify request
						CPF_ChangeFileAttribute_Request* modifyFile = new CPF_ChangeFileAttribute_Request(waitResult, fileInfo);
						// The cmd request object will be deleted by the cmd handler
						m_CmdHandler->enqueue(modifyFile);
						waitResult.get(operationResult);
					}
					break;
			}

			// checks the cmd result
			if( operationResult.errorCode() != FMS_CPF_Exception::OK )
			{
				TRACE(fms_cpf_oi_compFileTrace, "abort(), undo failed, error:<%s>", operationResult.errorText());
			}
		}
	}

	// Erase all elements from the table of the operations
	m_compositeFileOperationTable.erase(operationRange.first, operationRange.second);

	// Erase all delete operation
	std::vector<CPF_DeleteFile_Request*>::iterator delOperation;
	for(delOperation = m_deleteOperationList.begin(); delOperation != m_deleteOperationList.end(); ++delOperation)
	{
		delete (*delOperation);
	}

	m_deleteOperationList.clear();

	TRACE(fms_cpf_oi_compFileTrace, "%s", "Leaving abort(...)");
}

/*============================================================================
	ROUTINE: apply
 ============================================================================ */
void FMS_CPF_OI_CompositeFile::apply(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_compFileTrace, "%s", "Entering apply(...)");
	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	std::vector<CPF_DeleteFile_Request*>::iterator delOperation;
	CPF_DeleteFile_Request* deleteCmd;

	// Complete all delete requests, all file are already opened in exclusive mode
	for(delOperation = m_deleteOperationList.begin(); delOperation != m_deleteOperationList.end(); ++delOperation)
	{
		deleteCmd = *delOperation;
		if(!m_isMultiCP)
		{
		std::string filename = deleteCmd->getFileName();
		FMS_CPF_JTPConnectionHndl::removeDeleteFileTimer(filename);
		FMS_CPF_JTPConnectionHndl::removeCompositeFile(filename);
		}
		// Physical file deletion
		deleteCmd->deleteFile();
		delete deleteCmd;
	}
	m_deleteOperationList.clear();

	// find all operations related to the ccbid
	std::pair<operationTable::iterator, operationTable::iterator> operationRange;
	operationRange = m_compositeFileOperationTable.equal_range(ccbId);

	// Erase all elements from the table of the operations
	m_compositeFileOperationTable.erase(operationRange.first, operationRange.second);

	TRACE(fms_cpf_oi_compFileTrace, "%s", "Leaving apply(...)");
}

/*============================================================================
ROUTINE: adminOperationCallback
============================================================================ */
void FMS_CPF_OI_CompositeFile::adminOperationCallback(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_InvocationType invocation,
		const char* p_objName, ACS_APGCC_AdminOperationIdType operationId,
		ACS_APGCC_AdminOperationParamType** paramList)
{
	TRACE(fms_cpf_oi_compFileTrace, "%s", "Entering adminOperationCallback(...)");

	// To avoid warning about unused parameter
	UNUSED(p_objName);
	UNUSED(operationId);
	UNUSED(paramList);
	// No actions are defined in CompositeFile class
	adminOperationResult(oiHandle, invocation, actionResult::NOOPERATION);
	TRACE(fms_cpf_oi_compFileTrace, "%s", "Leaving adminOperationCallback(...)");
}

/*============================================================================
	ROUTINE: updateRuntime
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CompositeFile::updateRuntime(const char* p_objName, const char** p_attrName)
{
	TRACE(fms_cpf_oi_compFileTrace, "%s", "Entering updateRuntime(...)");
	ACS_CC_ReturnType result = ACS_CC_SUCCESS;
	UNUSED(p_attrName);

	CPF_DataFile_Request::dataFile infoFile;

	// get infinite main file name, cp name and subfile name
	if( getCompositeFileName(p_objName, infoFile.mainFileName) &&
		getCpName(p_objName, infoFile.cpName) )
	{
		ACE_Future<FMS_CPF_PrivateException> waitResult;
		FMS_CPF_PrivateException operationResult;
		FMS_CPF_Exception::errorType cmdExeResult;

		// create the file modify request
		CPF_DataFile_Request* dataFile = new CPF_DataFile_Request(waitResult, infoFile, false, true);

		m_CmdHandler->enqueue(dataFile);

		TRACE(fms_cpf_oi_compFileTrace, "%s", "updateRuntime(), wait data file cmd execution");
		waitResult.get(operationResult);

		cmdExeResult = operationResult.errorCode();

		// checks the cmd execution result
		if(FMS_CPF_Exception::OK != cmdExeResult )
		{
			result = ACS_CC_FAILURE;
			FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
			setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT),exErr.errorText());
			TRACE(fms_cpf_oi_compFileTrace, "%s","updateRuntime(), error on composite file update");
		}
		else
		{
			// Set the numOfReaders on File
			ACS_CC_ImmParameter paramNumReaders;
			paramNumReaders.attrName = cpf_imm::numReadersAttribute;
			paramNumReaders.attrType = ATTR_UINT32T;
			paramNumReaders.attrValuesNum = 1;
			void* numReaders[1] = { reinterpret_cast<void*>(&(infoFile.numOfReaders)) };
			paramNumReaders.attrValues = numReaders;

			TRACE(fms_cpf_oi_compFileTrace, "updateRuntime(...), update numOfReaders=<%d>", infoFile.numOfReaders );
			modifyRuntimeObj(p_objName, &paramNumReaders);

			// Set the numOfWriters on File
			ACS_CC_ImmParameter paramNumWriters;
			paramNumWriters.attrName = cpf_imm::numWritersAttribute;
			paramNumWriters.attrType = ATTR_UINT32T;
			paramNumWriters.attrValuesNum = 1;
			void* numWriters[1] = { reinterpret_cast<void*>(&(infoFile.numOfWriters)) };
			paramNumWriters.attrValues = numWriters;

			TRACE(fms_cpf_oi_compFileTrace, "updateRuntime(...), update numOfWriters=<%d>", infoFile.numOfWriters );
			modifyRuntimeObj(p_objName, &paramNumWriters);

			// Set the exclusiveAccess on File
			ACS_CC_ImmParameter paramExcAccess;
			paramExcAccess.attrName = cpf_imm::exclusiveAccessAttribute;
			paramExcAccess.attrType = ATTR_INT32T;
			paramExcAccess.attrValuesNum = 1;
			void* excAccess[1] = { reinterpret_cast<void*>(&(infoFile.exclusiveAccess)) };
			paramExcAccess.attrValues = excAccess;

			TRACE(fms_cpf_oi_compFileTrace, "updateRuntime(...), update exclusiveAccess=<%d>", infoFile.exclusiveAccess);
			modifyRuntimeObj(p_objName, &paramExcAccess);

			setExitCode(SUCCESS);
		}
	}
	else
	{
		TRACE(fms_cpf_oi_compFileTrace, "%s", "updateRuntime(...), error to get composite file name");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT),exErr.errorText());

	}
	TRACE(fms_cpf_oi_compFileTrace, "%s", "Leaving updateRuntime(...)");
	return result;
}

/*============================================================================
	ROUTINE: ~FMS_CPF_OI_CompositeFile
 ============================================================================ */
FMS_CPF_OI_CompositeFile::~FMS_CPF_OI_CompositeFile()
{
	m_compositeFileOperationTable.clear();

	if(NULL != fms_cpf_oi_compFileTrace)
		delete fms_cpf_oi_compFileTrace;
}
