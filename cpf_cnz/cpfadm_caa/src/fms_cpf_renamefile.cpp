/*
 * * @file fms_cpf_renamefile.cpp
 *	@brief
 *	Class method implementation for CPF_RenameFile_Request.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_renamefile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-09-27
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
 *	| 1.0.0  | 2012-09-27 | qvincon      | File created.                       |
  *	+========+============+==============+=====================================+
  *	| 1.1.0  | 2014-04-28 | qvincon      | Infinite subfiles removed from IMM  |
  *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_renamefile.h"
#include "fms_cpf_common.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filereference.h"
#include "fms_cpf_filedescriptor.h"

#include "fms_cpf_file.h"
#include "fms_cpf_exception.h"

#include "acs_apgcc_omhandler.h"
#include "ACS_TRA_trace.h"
#include "ACS_APGCC_Util.H"
#include "ACS_TRA_Logging.h"

#define BOOST_FILESYSTEM_VERSION 3

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

#include <sstream>

extern ACS_TRA_Logging CPF_Log;

namespace {
	const std::string hiddenTag("@DEL");
}

/*============================================================================
	ROUTINE: CPF_RenameFile_Request
 ============================================================================ */
CPF_RenameFile_Request::CPF_RenameFile_Request(const renameFileData& renameInfo)
: m_RenameInfo(renameInfo),
  m_fileHidden(false),
  m_fileSwapped(false),
  m_ExitCode(0),
  m_ExitMessage(),
  m_ComCliExitMessage(),
  m_trace(new (std::nothrow) ACS_TRA_trace("CPF_RenameFile_Request"))
{

}

/*============================================================================
	ROUTINE: executeRenameFile
 ============================================================================ */
bool CPF_RenameFile_Request::executeRenameFile()
{
	TRACE(m_trace, "%s", "Entering in executeRenameFile()");
	bool result = false;

	FMS_CPF_FileId srcFileId(m_RenameInfo.currentFile);
	FMS_CPF_FileId newFileId(m_RenameInfo.newFile);

	FileReference srcFileReference;

	try
	{
		// Open logical src file
		srcFileReference = DirectoryStructureMgr::instance()->open(srcFileId, FMS_CPF_Types::XR_XW_, m_RenameInfo.cpName.c_str());

		// check if the new file name already exists
		bool fileExists = DirectoryStructureMgr::instance()->exists(newFileId, m_RenameInfo.cpName.c_str());

		if(fileExists)
		{
			TRACE(m_trace, "executeRenameFile(), new file name:<%s> already exists", m_RenameInfo.newFile.c_str());
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILEEXISTS, m_RenameInfo.newFile );
		}

		// Retrieve the file DN
		DirectoryStructureMgr::instance()->getFileDN(m_RenameInfo.currentFile, m_RenameInfo.cpName, m_RenameInfo.fileDN);

		// Retrieve the physical file path
		m_RenameInfo.filePath = srcFileReference->getPath();


		// Retrieve the file base attributes
		FMS_CPF_Types::fileType ftype;
		bool compositeFile;
		unsigned int recordLength;
		int deleteFileTimer;

		FMS_CPF_Attribute srcFileAttribute = srcFileReference->getAttribute();
		srcFileAttribute.extractAttributes( ftype, compositeFile, recordLength, deleteFileTimer);

		// Get the volume name
		m_RenameInfo.volumeName = srcFileReference->getVolume();

		// Close the file reference to can operate on it
		DirectoryStructureMgr::instance()->closeExceptionLess(srcFileReference, m_RenameInfo.cpName.c_str());

		if(srcFileId.subfileAndGeneration().empty())
		{
			// Rename of regular or infinite file

			if(!newFileId.subfileAndGeneration().empty())
			{
				TRACE(m_trace, "%s", "executeRenameFile(), rename main/simple file to subfile not allowed");
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::NOTBASEFILE, m_RenameInfo.newFile);
			}

			std::list<std::string> subFilesList;
			// Check file type
			if(compositeFile)
			{
				// Get all subfile under current file name
				TRACE(m_trace, "%s", "executeRenameFile(), composite file rename");
				makeSubFilesList(subFilesList);
			}

			// Hidden the current file, so it will be possible delete the
			// Imm object without delete the physical file
			hiddenFile();

			if( FMS_CPF_Types::ft_REGULAR == ftype )
			{
				TRACE(m_trace, "%s", "executeRenameFile(), regular file rename");
				// Update the IMM objects
				renameRegularFile(compositeFile, recordLength, deleteFileTimer, subFilesList);
			}
			else
			{
				TRACE(m_trace, "%s", "executeRenameFile(), infinite file rename");
				FMS_CPF_Types::fileAttributes attributes;
				attributes.ftype = FMS_CPF_Types::ft_INFINITE;
				attributes.infinite.rlength = recordLength;
				std::string transferQueue;

				srcFileAttribute.extractExtAttributes( attributes.infinite.maxsize,
													   attributes.infinite.maxtime,
													   attributes.infinite.release,
													   attributes.infinite.active,
													   attributes.infinite.lastReportedSubfile,
													   attributes.infinite.mode,
													   transferQueue,
													   attributes.infinite.inittransfermode
													  );

				if( FMS_CPF_Types::tm_NONE != attributes.infinite.mode )
				{
					throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILEISPROT, errorText::FileAttachedToTQ);
				}
				// Set empty field
				ACE_OS::strncpy(attributes.infinite.transferQueue, transferQueue.c_str(), FMS_CPF_TQMAXLENGTH);

				// Update the IMM objects
				renameInfiniteFile(attributes);

				// Update the internal infinite file state
				updateInfiniteFileState(attributes, subFilesList);
			}
		}
		else
		{
			// SubFile rename

			// check if an infinite subfile
			if( FMS_CPF_Types::ft_INFINITE == ftype )
			{
				TRACE(m_trace, "%s", "executeRenameFile(), infinite subfile rename not allowed");
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::TYPEERROR);
			}

			// Check if destination file is a subfile
			if( newFileId.subfileAndGeneration().empty())
			{
				TRACE(m_trace, "%s", "executeRenameFile(), rename subfile to main/simple file not allowed");
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::NOTSUBFILE, m_RenameInfo.newFile);
			}

			// Check that main file is same
			if ( srcFileId.file().compare(newFileId.file()) != 0 )
			{
				TRACE(m_trace, "%s", "executeRenameFile(), change the main file of subfile not allowed");
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::NOTSAMEFILE, newFileId.file() );
			}

			std::string oldSubFileName(srcFileId.subfileAndGeneration());
			std::string newSubFileName(newFileId.subfileAndGeneration());

			// Hidden the current file, so it will be possible delete the
			// Imm object without delete the physical file
			hiddenFile();

			// Update the IMM objects
			renameSubFile(oldSubFileName,newSubFileName);
		}
		TRACE(m_trace, "%s", "executeRenameFile(), file rename success");
		result = true;
	}
	catch(const FMS_CPF_PrivateException& ex)
	{
		m_ExitCode = static_cast<int>(ex.errorCode());
		m_ExitMessage = ex.errorText();
		m_ExitMessage.append(ex.detailInfo());

	    m_ComCliExitMessage = m_ExitMessage;

		char errorMsg[256] = {0};
		snprintf(errorMsg, 255, "%s(), file:<%s> rename failed, error:<%s>", __func__ ,m_RenameInfo.currentFile.c_str(), m_ExitMessage.c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", errorMsg);

		DirectoryStructureMgr::instance()->closeExceptionLess(srcFileReference, m_RenameInfo.cpName.c_str());
		undoRename();
	}
	TRACE(m_trace, "%s", "Leaving executeRenameFile()");
	return result;
}

/*============================================================================
	ROUTINE: renameRegularFile
============================================================================ */
void CPF_RenameFile_Request::renameRegularFile(bool composite, unsigned int recordLength, int deleteFileTimer, const std::list<std::string>& subFilesList)
	throw(FMS_CPF_PrivateException)
{
	TRACE(m_trace, "%s", "Entering in renameRegularFile()");
	OmHandler objManager;

	// Init OM resource
	if(objManager.Init() != ACS_CC_FAILURE)
	{
		std::string omOperation;
		// prepare an transaction id
		std::string transActionName(m_RenameInfo.currentFile);

		try
		{
			ACS_CC_ReturnType omResult;

			// Get the parent DN
			std::string parentDN;
			getParentDN(m_RenameInfo.fileDN, parentDN);

			TRACE(m_trace, "renameRegularFile(), parent object DN:<%s>", parentDN.c_str());

			std::vector<ACS_CC_ValuesDefinitionType> objAttrList;

			ACS_CC_ValuesDefinitionType attributeRDN;
			attributeRDN.attrType = ATTR_STRINGT;
			attributeRDN.attrValuesNum = 1;
			std::string classObjectName;
			char tmpRDN[128] = {0};

			// check the object type
			if(composite)
			{
				attributeRDN.attrName = cpf_imm::CompositeFileKey;
				ACE_OS::snprintf(tmpRDN, 127, "%s=%s", cpf_imm::CompositeFileKey, m_RenameInfo.newFile.c_str());
				classObjectName = cpf_imm::CompositeFileClassName;
			}
			else
			{
				attributeRDN.attrName = cpf_imm::SimpleFileKey;
				ACE_OS::snprintf(tmpRDN, 127, "%s=%s", cpf_imm::SimpleFileKey, m_RenameInfo.newFile.c_str());
				classObjectName = cpf_imm::SimpleFileClassName;
			}

			void* tmpValueRDN[1] = { reinterpret_cast<void*>(tmpRDN) };
			attributeRDN.attrValues = tmpValueRDN;

			objAttrList.push_back(attributeRDN);

			ACS_CC_ValuesDefinitionType attributeRecordLength;
			attributeRecordLength.attrName = cpf_imm::recordLengthAttribute;
			attributeRecordLength.attrType = ATTR_UINT32T;
			attributeRecordLength.attrValuesNum = 1;
			void* tmpValueRecLen[1] = { reinterpret_cast<void*>(&recordLength) };
			attributeRecordLength.attrValues = tmpValueRecLen;

			objAttrList.push_back(attributeRecordLength);

			ACS_CC_ValuesDefinitionType deleteFileTimerAttribute;
			void* tmpDeleteFileTimer[1];
			if(composite)
			{
        			deleteFileTimerAttribute.attrName = cpf_imm::deleteFileTimerAttribute;
				deleteFileTimerAttribute.attrType = ATTR_INT32T;
				deleteFileTimerAttribute.attrValuesNum = 1;
				tmpDeleteFileTimer[0] = reinterpret_cast<void*>(&deleteFileTimer); //HY85874
				deleteFileTimerAttribute.attrValues = tmpDeleteFileTimer; //HY85874
        			objAttrList.push_back(deleteFileTimerAttribute);
			}

			TRACE(m_trace, "renameRegularFile(), create file object RDN:<%s>", tmpRDN);

			// Create the new file object
			omResult = objManager.createObject(classObjectName.c_str(), parentDN.c_str(), objAttrList, transActionName);

			if(ACS_CC_FAILURE == omResult)
			{
				omOperation = "createObject()";
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
			}

			// prepare the new DN of the created object
			char newFileDN[1024] = {0};
			ACE_OS::snprintf(newFileDN, 1023, "%s,%s", tmpRDN, parentDN.c_str() );

			// check subfile creation
			if(composite)
			{
				TRACE(m_trace, "renameRegularFile(), create subfile object under DN:<%s>", newFileDN);

				ACS_CC_ValuesDefinitionType attributeSubFileRDN;
				attributeSubFileRDN.attrName = cpf_imm::CompositeSubFileKey;
				attributeSubFileRDN.attrType = ATTR_STRINGT;
				attributeSubFileRDN.attrValuesNum = 1;

				std::list<std::string>::const_iterator subFileToCreate;
				// create all subfiles
				for(subFileToCreate = subFilesList.begin(); subFileToCreate != subFilesList.end(); ++subFileToCreate)
				{
					objAttrList.clear();
					// Fill the RDN Attribute
					char tmpSubFileRDN[64] = {0};

					ACE_OS::sprintf(tmpSubFileRDN, "%s=%s", cpf_imm::CompositeSubFileKey, (*subFileToCreate).c_str());
					void* valueRDN[1] = {reinterpret_cast<void*>(tmpSubFileRDN)};
					attributeSubFileRDN.attrValues = valueRDN;

					//Add the attributes to vector
					objAttrList.push_back(attributeSubFileRDN);

					TRACE(m_trace, "renameRegularFile(), create subfile object RDN:<%s>", tmpSubFileRDN);

					// create the composite subfile object into IMM
					omResult = objManager.createObject(cpf_imm::CompositeSubFileClassName, newFileDN, objAttrList, transActionName);

					if(ACS_CC_FAILURE == omResult)
					{
						omOperation = "createObject(subFile)";
						throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
					}

					// Store the DN of the created object
					char subFileDN[1024] = {0};
					ACE_OS::snprintf(subFileDN, 1023, "%s,%s", tmpSubFileRDN, newFileDN );
					m_CreatedFileDNs.push_back(std::string(subFileDN));
				}
			}

			// Store the DN of create object
			m_CreatedFileDNs.push_back(std::string(newFileDN));
			TRACE(m_trace, "renameRegularFile(), apply transaction:<%s>", transActionName.c_str());

			// Commit the transaction request
			omResult = objManager.applyRequest(transActionName);

			if(ACS_CC_FAILURE == omResult)
			{
				m_CreatedFileDNs.clear();
				omOperation = "applyRequest()";
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
			}

			// Swap the physical files
			swapFile();

			//delete the old object and its child
			std::vector<std::string>::const_iterator subFileDN;
			std::vector<std::string> subFileDnList;

			objManager.getChildren( m_RenameInfo.fileDN.c_str(), ACS_APGCC_SUBLEVEL, &subFileDnList);

			// remove child
			for(subFileDN = subFileDnList.begin(); subFileDN != subFileDnList.end(); ++subFileDN)
			{
				if(objManager.deleteObject((*subFileDN).c_str()) == ACS_CC_FAILURE)
				{
					omOperation = "deleteObject(subfile)";
					throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
				}
				m_DeleteFileDNs.push_back((*subFileDN));
			}

			// remove the main object
			if(objManager.deleteObject(m_RenameInfo.fileDN.c_str()) == ACS_CC_FAILURE)
			{
				omOperation = "deleteObject(file)";
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
			}

			// Deallocate OM resource
			objManager.Finalize();
		}
		catch(FMS_CPF_PrivateException& ex)
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "CPF_RenameFile_Request::renameRegularFile(%s), error:<%d> on <%s>, CP:<%s>", m_RenameInfo.currentFile.c_str(), objManager.getInternalLastError(), omOperation.c_str(), m_RenameInfo.cpName.c_str() );
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(m_trace, "%s", errMsg);

			// Deallocate OM resource
			objManager.Finalize();
			// re-throw same exception
			throw;
		}
	}
	else
	{
		//Error Handling
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "CPF_RenameFile_Request::renameRegularFile(%s), error:<%d> on OmHandler object init(), Cp:<%s>", m_RenameInfo.currentFile.c_str(), objManager.getInternalLastError(), m_RenameInfo.cpName.c_str());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", errMsg);
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
	}
	TRACE(m_trace, "%s", "Leaving renameRegularFile()");
}

/*============================================================================
	ROUTINE: renameInfiniteFile
============================================================================ */
void CPF_RenameFile_Request::renameInfiniteFile(FMS_CPF_Types::fileAttributes& attribute)
	throw(FMS_CPF_PrivateException)
{
	TRACE(m_trace, "%s", "Entering in renameInfiniteFile()");
	OmHandler objManager;

	// Init OM resource
	if(objManager.Init() != ACS_CC_FAILURE)
	{
		std::string omOperation;
		// create a new infinite object and delete the old one
		try
		{
			ACS_CC_ReturnType omResult;
			std::string parentDN;

			getParentDN(m_RenameInfo.fileDN, parentDN);

			TRACE(m_trace, "renameInfiniteFile(), parent object DN:<%s>", parentDN.c_str());

			std::vector<ACS_CC_ValuesDefinitionType> objAttrList;

			// Set the RDN attribute
			ACS_CC_ValuesDefinitionType attributeRDN;
			attributeRDN.attrType = ATTR_STRINGT;
			attributeRDN.attrValuesNum = 1;
			char tmpRDN[128] = {0};
			attributeRDN.attrName = cpf_imm::InfiniteFileKey;
			ACE_OS::snprintf(tmpRDN, 127, "%s=%s", cpf_imm::InfiniteFileKey, m_RenameInfo.newFile.c_str());
			void* tmpValueRDN[1] = { reinterpret_cast<void*>(tmpRDN) };
			attributeRDN.attrValues = tmpValueRDN;

			objAttrList.push_back(attributeRDN);

			// Set the attribute record length
			ACS_CC_ValuesDefinitionType attributeRecordLength;
			attributeRecordLength.attrName = cpf_imm::recordLengthAttribute;
			attributeRecordLength.attrType = ATTR_UINT32T;
			attributeRecordLength.attrValuesNum = 1;
			void* tmpValueRecLen[1] = { reinterpret_cast<void*>(&attribute.infinite.rlength) };
			attributeRecordLength.attrValues = tmpValueRecLen;

			objAttrList.push_back(attributeRecordLength);

			// Set the optional attribute maxSize
			ACS_CC_ValuesDefinitionType maxSizeAttribute;
			void* tmpMaxSize[1];

			if(0 != attribute.infinite.maxsize )
			{
				TRACE(m_trace, "renameInfiniteFile(), set maxSize <%d>", attribute.infinite.maxsize );
				maxSizeAttribute.attrName = cpf_imm::maxSizeAttribute;
				maxSizeAttribute.attrType = ATTR_UINT32T;
				maxSizeAttribute.attrValuesNum = 1;
				tmpMaxSize[0] =  reinterpret_cast<void*>(&attribute.infinite.maxsize);
				maxSizeAttribute.attrValues = tmpMaxSize;
				objAttrList.push_back(maxSizeAttribute);
			}

			// Set the optional attribute maxTime
			ACS_CC_ValuesDefinitionType maxTimeAttribute;
			void* tmpMaxTime[1];

			if(0 != attribute.infinite.maxtime )
			{
				unsigned int maxTimeMinutes = static_cast<unsigned int>(attribute.infinite.maxtime /60);
				TRACE(m_trace, "renameInfiniteFile(), set maxTime <%d>", maxTimeMinutes );
				maxTimeAttribute.attrName = cpf_imm::maxTimeAttribute;
				maxTimeAttribute.attrType = ATTR_UINT32T;
				maxTimeAttribute.attrValuesNum = 1;
				tmpMaxTime[0] =  reinterpret_cast<void*>(&maxTimeMinutes);
				maxTimeAttribute.attrValues = tmpMaxTime;
				objAttrList.push_back(maxTimeAttribute);
			}

			TRACE(m_trace, "renameInfiniteFile(), set release cond. to <%s>", (attribute.infinite.release ? "ON" :"OFF") );

			// Set the attribute release condition
			int releaseConditionValue = (attribute.infinite.release ? 1 : 0);
			ACS_CC_ValuesDefinitionType relCondAttribute;

			relCondAttribute.attrName = cpf_imm::releaseCondAttribute;
			relCondAttribute.attrType = ATTR_INT32T;
			relCondAttribute.attrValuesNum = 1;
			void* tmpRelCond[1] = {reinterpret_cast<void*>(&releaseConditionValue)};
			relCondAttribute.attrValues = tmpRelCond;
			objAttrList.push_back(relCondAttribute);

			// Create the new file object
			omResult = objManager.createObject(cpf_imm::InfiniteFileClassName, parentDN.c_str(), objAttrList);

			if(ACS_CC_FAILURE == omResult)
			{
				omOperation = "createObject()";
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
			}

			// store DN of the created object
			char newFileDN[1024] = {0};
			ACE_OS::snprintf(newFileDN, 1023, "%s,%s", tmpRDN, parentDN.c_str() );
			m_CreatedFileDNs.push_back(std::string(newFileDN));

			// Swap the physical files
			swapFile();

			TRACE(m_trace, "renameInfiniteFile(), delete the old object:<%s>", m_RenameInfo.fileDN.c_str() );

			omResult = objManager.deleteObject(m_RenameInfo.fileDN.c_str(), ACS_APGCC_SUBLEVEL);

			if(ACS_CC_FAILURE == omResult)
			{
				omOperation = "deleteObject()";
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
			}

			// Deallocate OM resource
			objManager.Finalize();
		}
		catch(FMS_CPF_PrivateException& ex)
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "CPF_RenameFile_Request::renameInfiniteFile(%s), error:<%d> on <%s>, CP:<%s>", m_RenameInfo.currentFile.c_str(), objManager.getInternalLastError(), omOperation.c_str(), m_RenameInfo.cpName.c_str() );
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(m_trace, "%s", errMsg);
			// Deallocate OM resource
			objManager.Finalize();
			// re-throw same exception
			throw;
		}
	}
	else
	{
		//Error Handling
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "CPF_RenameFile_Request::renameInfiniteFile(%s), error:<%d> on OmHandler object init(), Cp:<%s>", m_RenameInfo.currentFile.c_str(), objManager.getInternalLastError(), m_RenameInfo.cpName.c_str());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", errMsg);
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
	}
	TRACE(m_trace, "%s", "Leaving renameInfiniteFile()");
}

/*============================================================================
	ROUTINE: updateInfiniteFileState
============================================================================ */
void CPF_RenameFile_Request::renameSubFile(const std::string& oldSubFileName, const std::string& newSubFileName )
		throw(FMS_CPF_PrivateException)
{
	TRACE(m_trace, "%s", "Entering in renameSubFile()");
	OmHandler objManager;

	// Init OM resource
	if(objManager.Init() != ACS_CC_FAILURE)
	{
		TRACE(m_trace, "renameSubFile(), parent object DN:<%s>", m_RenameInfo.fileDN.c_str());

		std::string omOperation;
		ACS_CC_ReturnType omResult;
		std::vector<ACS_CC_ValuesDefinitionType> objAttrList;

		// Set the RDN attribute
		ACS_CC_ValuesDefinitionType attributeRDN;
		attributeRDN.attrType = ATTR_STRINGT;
		attributeRDN.attrValuesNum = 1;
		char tmpRDN[128] = {0};
		attributeRDN.attrName = cpf_imm::CompositeSubFileKey;
		ACE_OS::snprintf(tmpRDN, 127, "%s=%s", cpf_imm::CompositeSubFileKey, newSubFileName.c_str());
		void* tmpValueRDN[1] = { reinterpret_cast<void*>(tmpRDN) };
		attributeRDN.attrValues = tmpValueRDN;

		objAttrList.push_back(attributeRDN);

		// Create the new file object
		omResult = objManager.createObject(cpf_imm::CompositeSubFileClassName, m_RenameInfo.fileDN.c_str(), objAttrList);

		try
		{

			if(ACS_CC_FAILURE == omResult)
			{
				omOperation = "createObject()";
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
			}

			char newFileDN[1024] = {0};
			ACE_OS::snprintf(newFileDN, 1023, "%s,%s", tmpRDN, m_RenameInfo.fileDN.c_str() );

			// store DN of the created object
			m_CreatedFileDNs.push_back(std::string(newFileDN));

			m_RenameInfo.newFile = newSubFileName;

			// Swap the physical files
			swapFile();

			std::vector<std::string>::const_iterator subFileDN;
			std::vector<std::string> subFileDnList;

			objManager.getChildren( m_RenameInfo.fileDN.c_str(), ACS_APGCC_SUBLEVEL, &subFileDnList);

			size_t equalPos;
			size_t commaPos;
			TRACE(m_trace, "renameSubFile(), search subfile:<%s> DN", oldSubFileName.c_str() );

			for(subFileDN = subFileDnList.begin(); subFileDN != subFileDnList.end(); ++subFileDN)
			{
				// Get the file name from DN
				// Split the field in RDN and Value
				equalPos = (*subFileDN).find_first_of(parseSymbol::equal);
				commaPos = (*subFileDN).find_first_of(parseSymbol::comma);

				// Check if some error happens
				if( (std::string::npos != equalPos) && (std::string::npos != commaPos) )
				{
					std::string fileName = (*subFileDN).substr(equalPos + 1, (commaPos - equalPos - 1) );

					// make the name in upper case
					ACS_APGCC::toUpper(fileName);
					// Compare the file name
					if(oldSubFileName.compare(fileName) == 0)
					{

						TRACE(m_trace, "renameSubFile(), DN found, delete the old object:<%s>", (*subFileDN).c_str() );

						omResult = objManager.deleteObject((*subFileDN).c_str());
						if(ACS_CC_FAILURE == omResult)
						{
							omOperation = "deleteObject(subfile)";
							throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
						}
						break;
					}
				}
			}

			// Deallocate OM resource
			objManager.Finalize();
		}
		catch(FMS_CPF_PrivateException& ex)
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "CPF_RenameFile_Request::renameSubFile(%s), error:<%d> on <%s>, CP:<%s>", m_RenameInfo.currentFile.c_str(), objManager.getInternalLastError(), omOperation.c_str(), m_RenameInfo.cpName.c_str() );
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(m_trace, "%s", errMsg);

			// Deallocate OM resource
			objManager.Finalize();
			// re-throw same exception
			throw;
		}
	}
	else
	{
		//Error Handling
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "CPF_RenameFile_Request::renameSubFile(%s), error:<%d> on OmHandler object init(), Cp:<%s>", m_RenameInfo.currentFile.c_str(), objManager.getInternalLastError(), m_RenameInfo.cpName.c_str());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", errMsg);
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
	}
	TRACE(m_trace, "%s", "Leaving renameSubFile()");
}

/*============================================================================
	ROUTINE: updateInfiniteFileState
============================================================================ */
void CPF_RenameFile_Request::updateInfiniteFileState(FMS_CPF_Types::fileAttributes& attribute, const std::list<std::string>& subFilesList)
{
	TRACE(m_trace, "%s", "Entering in updateInfiniteFileState()");
	if(1U != attribute.infinite.active)
	{
		FileReference newFileReference;

		try
		{
			TRACE(m_trace, "updateInfiniteFileState(), active:<%d>, last sent:<%d>",attribute.infinite.active, attribute.infinite.lastReportedSubfile );
			FMS_CPF_FileId newFileId(m_RenameInfo.newFile);
			// Open logical src file
			newFileReference = DirectoryStructureMgr::instance()->open(newFileId, FMS_CPF_Types::XR_XW_, m_RenameInfo.cpName.c_str());

			FMS_CPF_Attribute oldFileState(attribute);

			// Change file attributes
			newFileReference->setAttribute(oldFileState);

			DirectoryStructureMgr::instance()->closeExceptionLess(newFileReference,m_RenameInfo.cpName.c_str());//HV47401
		}
		catch(const FMS_CPF_PrivateException& ex)
		{
			DirectoryStructureMgr::instance()->closeExceptionLess(newFileReference,m_RenameInfo.cpName.c_str());//HV47401
			char errorMsg[1024] = {0};
			snprintf(errorMsg, 1023, "updateInfiniteFileState(), file:<%s> open file failed, error:<%s>", m_RenameInfo.currentFile.c_str(), ex.errorText() );
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		}

	}
	TRACE(m_trace, "%s", "Leaving updateInfiniteFileState()");
}

/*============================================================================
	ROUTINE: makeSubFilesList
============================================================================ */
void CPF_RenameFile_Request::makeSubFilesList(std::list<std::string>& subFilesToCreate)
{
	TRACE(m_trace, "%s", "Entering in makeSubFilesList()");

	std::list<FMS_CPF_FileId>::iterator subFileIterator;
	std::list<FMS_CPF_FileId> subFilesList;

	FMS_CPF_FileId srcFileId(m_RenameInfo.currentFile);

	DirectoryStructureMgr::instance()->getListFileId(srcFileId, subFilesList, m_RenameInfo.cpName.c_str());

	TRACE(m_trace, "makeSubFilesList(), num=<%zd> subFiles", subFilesList.size());

	for(subFileIterator = subFilesList.begin(); subFileIterator != subFilesList.end(); ++subFileIterator)
	{
		std::string subFileName((*subFileIterator).subfileAndGeneration());
		// Insert subfile in the list of subfiles to create
		subFilesToCreate.push_back(subFileName);
		TRACE(m_trace, "makeSubFilesList(), subFile=<%s> must be created", subFileName.c_str() );
	}

	TRACE(m_trace, "%s", "Leaving makeSubFilesList()");
}

/*============================================================================
	ROUTINE: undoRename
 ============================================================================ */
void CPF_RenameFile_Request::undoRename()
{
	TRACE(m_trace, "%s", "Entering in undoRename()");

	if(!m_DeleteFileDNs.empty())
	{
		// Re-create delete subfile objects
		OmHandler objManager;
		// Init OM resource
		if(objManager.Init() != ACS_CC_FAILURE)
		{
			// get parent DN
			std::string parentDN;
			std::list<std::string>::const_iterator createFileDn;
			createFileDn = m_DeleteFileDNs.begin();

			getParentDN(*createFileDn, parentDN);
			size_t parentDNLength = parentDN.length();

			TRACE(m_trace, "undoRename(), create subfiles under object:<%s>", parentDN.c_str());

			std::vector<ACS_CC_ValuesDefinitionType> objAttrList;

			ACS_CC_ValuesDefinitionType attributeSubFileRDN;
			attributeSubFileRDN.attrName = cpf_imm::CompositeSubFileKey;
			attributeSubFileRDN.attrType = ATTR_STRINGT;
			attributeSubFileRDN.attrValuesNum = 1;
			// recreate all delete objects

			for(; createFileDn != m_DeleteFileDNs.end(); ++createFileDn)
			{
				objAttrList.clear();
				char tmpSubFileRDN[64] = {0};
				std::string subFileRDN = (*createFileDn).substr(0, (*createFileDn).length() - parentDNLength -1);
				ACE_OS::sprintf(tmpSubFileRDN, "%s", subFileRDN.c_str());
				void* valueRDN[1] = {reinterpret_cast<void*>(tmpSubFileRDN)};
				attributeSubFileRDN.attrValues = valueRDN;

				//Add the attributes to vector
				objAttrList.push_back(attributeSubFileRDN);

				TRACE(m_trace, "undoRename(), create subfile object RDN:<%s>", tmpSubFileRDN);
				// create the composite subfile object into IMM
				objManager.createObject(cpf_imm::CompositeSubFileClassName, parentDN.c_str(), objAttrList);
			}

			// deallocate resource
			objManager.Finalize();
		}
	}

	// re-swap physical files
	undoSwapFile();

	if(!m_CreatedFileDNs.empty())
	{
		// delete created object
		OmHandler objManager;
		// Init OM resource
		TRACE(m_trace, "%s", "undoRename(), delete created objects");
		if(objManager.Init() != ACS_CC_FAILURE)
		{
			std::list<std::string>::const_iterator deleteFileDn;
			for(deleteFileDn = m_CreatedFileDNs.begin(); deleteFileDn != m_CreatedFileDNs.end(); ++deleteFileDn)
			{
				objManager.deleteObject((*deleteFileDn).c_str());
			}
		}
		// deallocate resource
		objManager.Finalize();
	}

	// re-show original files
	undoHiddenFile();

	TRACE(m_trace, "%s", "Leaving undoRename()");
}

/*============================================================================
	ROUTINE: hiddenFile
 ============================================================================ */
void CPF_RenameFile_Request::hiddenFile() throw(FMS_CPF_PrivateException)
{
	TRACE(m_trace, "%s", "Entering in hiddenFile()");
	boost::system::error_code hiddenResult;

	std::string newFileName(m_RenameInfo.filePath);
	newFileName += hiddenTag;

	// rename the file to hidden it
	boost::filesystem::rename(m_RenameInfo.filePath, newFileName, hiddenResult );

	if(SUCCESS != hiddenResult.value() )
	{
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "CPF_RenameFile_Request::hiddenFile(), error:<%d> on rename:<%s>, CP:<%s>", hiddenResult.value(), m_RenameInfo.filePath.c_str(), m_RenameInfo.cpName.c_str());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", errMsg);
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
	}
	m_fileHidden = true;

	TRACE(m_trace, "%s", "Leaving hiddenFile()");
}

/*============================================================================
	ROUTINE: undoHiddenFile
 ============================================================================ */
void CPF_RenameFile_Request::undoHiddenFile()
{
	TRACE(m_trace, "%s", "Entering in undoHiddenFile()");

	if(m_fileHidden)
	{
		boost::system::error_code hiddenResult;

		std::string hiddenFileName(m_RenameInfo.filePath);
		hiddenFileName += hiddenTag;

		// rename the file to hidden it
		boost::filesystem::rename(hiddenFileName, m_RenameInfo.filePath, hiddenResult );

		if(SUCCESS != hiddenResult.value() )
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "CPF_RenameFile_Request::undoHiddenFile(), error:<%d> on rename:<%s>, CP:<%s>", hiddenResult.value(), hiddenFileName.c_str(), m_RenameInfo.cpName.c_str());
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(m_trace, "%s", errMsg);
		}
		m_fileHidden = false;
	}
	TRACE(m_trace, "%s", "Leaving undoHiddenFile()");
}

/*============================================================================
	ROUTINE: swapFile
 ============================================================================ */
void CPF_RenameFile_Request::swapFile() throw(FMS_CPF_PrivateException)
{
	TRACE(m_trace, "%s", "Entering in swapFile()");

	boost::system::error_code swapResult;
	size_t slashPos = m_RenameInfo.filePath.find_last_of(DirDelim);

	// Check if the tag is present
	if( std::string::npos != slashPos )
	{
		std::string path = m_RenameInfo.filePath.substr(0, slashPos + 1);
		m_RenameInfo.newFilePath = path + m_RenameInfo.newFile;

		TRACE(m_trace, "swapFile(), <%s> to <%s> ", m_RenameInfo.newFilePath.c_str(), m_RenameInfo.filePath.c_str() );

		boost::filesystem::rename( m_RenameInfo.newFilePath, m_RenameInfo.filePath, swapResult );

		if(SUCCESS != swapResult.value() )
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "CPF_RenameFile_Request::swapFile(), error:<%d> on rename: <%s> to <%s>", swapResult.value(), m_RenameInfo.newFilePath.c_str(), m_RenameInfo.filePath.c_str());
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(m_trace, "%s", errMsg);
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
		}

		std::string hiddenFileName(m_RenameInfo.filePath);
		hiddenFileName += hiddenTag;

		TRACE(m_trace, "swapFile(), <%s> to <%s>", hiddenFileName.c_str(), m_RenameInfo.newFilePath.c_str());
		// rename the hidden file to new file
		boost::filesystem::rename(hiddenFileName, m_RenameInfo.newFilePath, swapResult );

		if(SUCCESS != swapResult.value() )
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "CPF_RenameFile_Request::swapFile(), error:<%d> on rename: <%s> to <%s>", swapResult.value(), hiddenFileName.c_str(), m_RenameInfo.newFilePath.c_str());
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(m_trace, "%s", errMsg);
			// undo first rename
			boost::filesystem::rename(m_RenameInfo.filePath, m_RenameInfo.newFilePath, swapResult );
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
		}
	}
	// set swapped flag for undo purpose
	m_fileSwapped = true;

	TRACE(m_trace, "%s", "Leaving swapFile()");
}

/*============================================================================
	ROUTINE: undoSwapFile
 ============================================================================ */
void CPF_RenameFile_Request::undoSwapFile() throw(FMS_CPF_PrivateException)
{
	TRACE(m_trace, "%s", "Entering in undoSwapFile()");
	if(m_fileSwapped)
	{
		boost::system::error_code swapResult;

		std::string hiddenFileName(m_RenameInfo.filePath);
		hiddenFileName += hiddenTag;

		TRACE(m_trace, "undoSwapFile(), <%s> to <%s>", m_RenameInfo.newFilePath.c_str(), hiddenFileName.c_str());

		// rename new file to the hidden file
		boost::filesystem::rename(m_RenameInfo.newFilePath, hiddenFileName, swapResult );

		if(SUCCESS != swapResult.value() )
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "CPF_RenameFile_Request::undoSwapFile(), error:<%d> on rename: <%s> to <%s>", swapResult.value(), m_RenameInfo.newFilePath.c_str(), hiddenFileName.c_str());
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(m_trace, "%s", errMsg);
		}

		TRACE(m_trace, "undoSwapFile(), <%s> to <%s>", m_RenameInfo.filePath.c_str(),  m_RenameInfo.newFilePath.c_str());
		// rename current old to new one
		boost::filesystem::rename( m_RenameInfo.filePath, m_RenameInfo.newFilePath, swapResult );

		if(SUCCESS != swapResult.value() )
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "CPF_RenameFile_Request::undoSwapFile(), error:<%d> on rename: <%s> to <%s>", swapResult.value(), m_RenameInfo.filePath.c_str(), m_RenameInfo.newFilePath.c_str());
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(m_trace, "%s", errMsg);
		}

		m_fileSwapped = false;
	}
	TRACE(m_trace, "%s", "Leaving undoSwapFile()");
}

/*============================================================================
	ROUTINE: getParentDN
 ============================================================================ */
void CPF_RenameFile_Request::getParentDN(const std::string& fileDN, std::string& parentDN) throw(FMS_CPF_PrivateException)
{
	TRACE(m_trace, "%s", "Entering in getParentDN()");
	size_t commaPos = fileDN.find_first_of(parseSymbol::comma);
	// Check if the tag is present
	if( std::string::npos != commaPos )
	{
		// get the parent DN
		parentDN = fileDN.substr(commaPos +1);
	}
	else
	{
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "CPF_RenameFile_Request::getParentDN(), error on get parent DN from the file DN:<%s>, CP:<%s>", fileDN.c_str(), m_RenameInfo.cpName.c_str());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", errMsg);
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
	}
	TRACE(m_trace, "%s", "Leaving getParentDN()");
}

/*============================================================================
	ROUTINE: ~CPF_RenameFile_Request
 ============================================================================ */
CPF_RenameFile_Request::~CPF_RenameFile_Request()
{
	if(NULL != m_trace)
		delete m_trace;
}
