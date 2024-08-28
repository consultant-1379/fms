/*
 * * @file fms_cpf_deletefile.cpp
 *	@brief
 *	Class method implementation for CPF_DeleteFile_Request.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_delete.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-08-24
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
 *	| 1.0.0  | 2011-08-24 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_deletefile.h"
#include "fms_cpf_common.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_filereference.h"

#include "fms_cpf_fileid.h"
#include "fms_cpf_attribute.h"
#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;


/*============================================================================
	ROUTINE: CPF_CreateFile_Request
 ============================================================================ */
CPF_DeleteFile_Request::CPF_DeleteFile_Request(const deleteFileData& fileInfo)
: m_FileInfo(fileInfo),
  m_FileReference(0),
  m_RecursiveDelete(false)
{
	cpf_DeleteFileTrace = new (std::nothrow) ACS_TRA_trace("CPF_DeleteFile_Request");
}

/*============================================================================
	ROUTINE: call
 ============================================================================ */
bool CPF_DeleteFile_Request::acquireFileLock(FMS_CPF_PrivateException& lockException)
{
	TRACE(cpf_DeleteFileTrace, "%s", "Entering in acquireFileLock()");
	bool result = true;
	try
	{
		FMS_CPF_FileId fileid( m_FileInfo.fileName );
		if(!fileid.subfileAndGeneration().empty())
		{
			// subfile deletion, check for a recursive deletion
			m_RecursiveDelete = DirectoryStructureMgr::instance()->isFileLockedForDelete(m_FileInfo.fileName, m_FileInfo.cpName);
			TRACE(cpf_DeleteFileTrace, "acquireFileLock(), <%s> file delete", ( m_RecursiveDelete ? "recursive" : "single"));
		}

		if(!m_RecursiveDelete)
		{
			// Open logical file in exclusive access
			m_FileReference = DirectoryStructureMgr::instance()->open(fileid, FMS_CPF_Types::DELETE_, m_FileInfo.cpName.c_str());

			if(m_FileInfo.fileType == FMS_CPF_Types::ft_INFINITE)
			{
				// Check if defined to transfer queue
				FMS_CPF_Attribute fileAttribute = m_FileReference->getAttribute();
				FMS_CPF_Types::transferMode TQMode;
				// Get the current transfer mode
				fileAttribute.getTQmode(TQMode);

				if( FMS_CPF_Types::tm_NONE != TQMode )
				{
					DirectoryStructureMgr::instance()->closeExceptionLess(m_FileReference, m_FileInfo.cpName.c_str() );
					m_FileReference = FileReference::NOREFERENCE;
					throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILEISPROT, errorText::FileAttachedToTQ);
				}
			}
		}

		FMS_CPF_PrivateException resultOK(FMS_CPF_Exception::OK);
		lockException = resultOK;
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		char errorMsg[1024] = {0};
		snprintf(errorMsg, 1023, "acquireFileLock() before delete, open file:<%s> failed, error:<%s>", m_FileInfo.fileName.c_str(), ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(cpf_DeleteFileTrace, "%s", errorMsg);
		lockException = ex;
		result = false;
	}

	TRACE(cpf_DeleteFileTrace, "%s", "Leaving acquireFileLock()");
	return result;
}

/*============================================================================
	ROUTINE: call
 ============================================================================ */
bool CPF_DeleteFile_Request::deleteFile()
{
	TRACE(cpf_DeleteFileTrace, "%s", "Entering deleteFile()");
	bool result = false;

	// check for recursive file deletion
	if(m_RecursiveDelete)
	{
		result = true;
		TRACE(cpf_DeleteFileTrace, "%s", "Leaving deleteFile(), recursive deletion");
		return result;
	}

	// check if the file is already opened
	if(m_FileReference == FileReference::NOREFERENCE)
	{
		TRACE(cpf_DeleteFileTrace, "%s", "Leaving deleteFile(), file not locked");
		return result;
	}

	FMS_CPF_FileId fileid( m_FileInfo.fileName );
	bool fileDeleted = true;

	try
	{
		//checks the file type
		if(m_FileInfo.fileType == FMS_CPF_Types::ft_REGULAR)
		{
			// File composite, simple or subfile
			TRACE(cpf_DeleteFileTrace, "%s", "deleteFile(), remove regular file");
			DirectoryStructureMgr::instance()->remove(m_FileReference, m_FileInfo.cpName.c_str());
			TRACE(cpf_DeleteFileTrace, "%s", "deleteFile(), regular file removed");
		}
		else //FMS_CPF_Types::ft_INFINITE
		{
			TRACE(cpf_DeleteFileTrace, "%s", "deleteFile(), remove infinite file");
			// Check if defined to transfer queue
			DirectoryStructureMgr::instance()->removeBlockSender(m_FileReference, m_FileInfo.cpName.c_str() );

			// Infinite file remove
			int removeResult;
			// Physical path of infinite file
			std::string infiniteFilePath = m_FileReference->getPath();

			std::list<FMS_CPF_FileId> subFilesList;
			std::list<FMS_CPF_FileId>::iterator subFileElement;
			// Get the list of infinite subfiles
			DirectoryStructureMgr::instance()->getListFileId(fileid, subFilesList, m_FileInfo.cpName.c_str());

			TRACE(cpf_DeleteFileTrace, "deleteFile(), infinite file with <%zd> subfiles", subFilesList.size());

			// Remove all subfiles
			for(subFileElement = subFilesList.begin(); subFileElement != subFilesList.end(); ++subFileElement)
			{
				if(!(*subFileElement).isNull())
				{
					// Assemble the subfile physical path
					std::string subFilePath = infiniteFilePath + DirDelim;
					subFilePath += (*subFileElement).subfileAndGeneration();
					// Remove the file
					removeResult =  ::remove(subFilePath.c_str());

					if(SUCCESS != removeResult)
					{
						// Error on file remove log it
						char errorText[256]={0};
						std::string errorDetail(strerror_r(errno, errorText, 255));
						char errorMsg[1024]={'\0'};
						snprintf(errorMsg, 1023,"Delete file=<%s> remove file failed, error=<%s>", subFilePath.c_str(), errorDetail.c_str() );
						CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
						TRACE(cpf_DeleteFileTrace, "%s", "deleteFile(), error to remove infinite subfile");
					}
				}
			}
			// Remove the main file
			DirectoryStructureMgr::instance()->remove(m_FileReference, m_FileInfo.cpName.c_str());
			result = true;
			TRACE(cpf_DeleteFileTrace, "%s", "deleteFile(), infinite file removed");
		}

	}
	catch(FMS_CPF_PrivateException& ex)
	{
		char errorMsg[256]={'\0'};
		snprintf(errorMsg, 255, "deleteFile(), file:<%s> remove failed, error:<%s>", m_FileInfo.fileName.c_str(), ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(cpf_DeleteFileTrace, "%s", errorMsg);
		fileDeleted = false;
	}

	// If the file is not delete we need to close it
	if(!fileDeleted)
	{
		// Close logical file
		DirectoryStructureMgr::instance()->closeExceptionLess(m_FileReference, m_FileInfo.cpName.c_str() );
	}

	m_FileReference = FileReference::NOREFERENCE;

	TRACE(cpf_DeleteFileTrace, "%s", "Leaving deleteFile()");
	return result;
}
/*============================================================================
	ROUTINE: ~CPF_CreateFile_Request
 ============================================================================ */
CPF_DeleteFile_Request::~CPF_DeleteFile_Request()
{
	if(m_FileReference != FileReference::NOREFERENCE)
		DirectoryStructureMgr::instance()->closeExceptionLess(m_FileReference, m_FileInfo.cpName.c_str() );

	if(NULL != cpf_DeleteFileTrace)
		delete cpf_DeleteFileTrace;

}

//TRHY46076
const std::string& CPF_DeleteFile_Request::getFileName()
{
      return m_FileInfo.fileName;
}
