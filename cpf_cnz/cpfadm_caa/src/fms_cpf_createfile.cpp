/*
 * * @file fms_cpf_createfile.cpp
 *	@brief
 *	Class method implementation for CPF_CreateFile_Request.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_createfile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-07-04
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
 *	| 1.0.0  | 2011-07-04 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_createfile.h"
#include "fms_cpf_common.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filereference.h"
#include "fms_cpf_tqchecker.h"

#include "fms_cpf_fileid.h"
#include "fms_cpf_attribute.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	ROUTINE: CPF_CreateFile_Request
 ============================================================================ */
CPF_CreateFile_Request::CPF_CreateFile_Request(ACE_Future<FMS_CPF_PrivateException>& result,const cpFileData& fileInfo)
:m_result(result), m_FileInfo(fileInfo)
{
	cpf_CreateFileTrace = new (std::nothrow) ACS_TRA_trace("CPF_CreateFile_Request");
}

/*============================================================================
	ROUTINE: call
 ============================================================================ */
int CPF_CreateFile_Request::call()
{
	TRACE(cpf_CreateFileTrace, "%s", "Entering call()");

	FMS_CPF_PrivateException resultOK(FMS_CPF_Exception::OK);

	// Check file type
	if( FMS_CPF_Types::ft_REGULAR == m_FileInfo.fileType )
	{
           //HY46076
           unsigned long activeSubfile = 0;
           unsigned long lastSentSubfile = 0;
		//Check the file type
		if( m_FileInfo.subFileName.empty() )
		{
			//Composite or Simple File creation
			// Each object could throw an exception in case of error
			try{
				//create the attribute object
				FMS_CPF_Attribute attribute(FMS_CPF_Types::createFileAttr(FMS_CPF_Types::ft_REGULAR, m_FileInfo.recordLength,m_FileInfo.composite,m_FileInfo.maxSize,m_FileInfo.maxTime,m_FileInfo.releaseCondition,activeSubfile,lastSentSubfile,m_FileInfo.tqMode,m_FileInfo.transferQueue,m_FileInfo.tqMode,m_FileInfo.deleteFileTimer) );
              
                                          FMS_CPF_FileId fileid(m_FileInfo.fileName);                             
				TRACE(cpf_CreateFileTrace, "%s", "call(), creating a regular file");
				FileReference fRef = DirectoryStructureMgr::instance()->create( fileid, m_FileInfo.volumeName,attribute, FMS_CPF_Types::NONE_,m_FileInfo.fileDN,m_FileInfo.cpName.c_str() );

				//Close the create file
				DirectoryStructureMgr::instance()->closeExceptionLess(fRef, m_FileInfo.cpName.c_str());

				m_result.set(resultOK);
				TRACE(cpf_CreateFileTrace, "%s", "call(), regular file created");
			}
			catch(FMS_CPF_PrivateException& ex )
			{
				TRACE(cpf_CreateFileTrace, "%s", "call(), exception on regular file creation");
				char errorMsg[1024]={'\0'};
				snprintf(errorMsg, 1023,"CPF_CreateFile_Request::call(), creation of the regular file=<%s> failed,  error=<%s>", m_FileInfo.fileName.c_str(), ex.errorText() );
				CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
				m_result.set(ex);
			}
		}
		else
		{
			// SubFile of a composite file creation
			std::string fileName = m_FileInfo.fileName + SubFileSep + m_FileInfo.subFileName;
			// Each object could throw an exception in case of error
			try{

				FMS_CPF_FileId fileid(fileName);

				TRACE(cpf_CreateFileTrace, "call(), creating a subfile=<%s>", fileName.c_str() );
				FileReference fRef = DirectoryStructureMgr::instance()->create(fileid ,FMS_CPF_Types::NONE_, m_FileInfo.cpName.c_str() );

				//Close the create file
				DirectoryStructureMgr::instance()->closeExceptionLess(fRef, m_FileInfo.cpName.c_str());

				m_result.set(resultOK);

				TRACE(cpf_CreateFileTrace, "%s", "call(), composite sub file created");
			}
			catch(FMS_CPF_PrivateException& ex)
			{
				TRACE(cpf_CreateFileTrace, "%s", "call(), exception on composite sub file creation");
				char errorMsg[1024]={'\0'};
				snprintf(errorMsg, 1023,"CPF_CreateFile_Request::call(), creation of the subfile=<%s> failed,  error=<%s>", fileName.c_str(), ex.errorText() );
				CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);

				m_result.set(ex);
			}

		}
	}
	else if(FMS_CPF_Types::ft_INFINITE == m_FileInfo.fileType)
	{
		// Infinite File creation
		unsigned long activeSubfile = 0;
		unsigned long lastSentSubfile = 0;

		// Each object could throw an exception in case of error
		try
		{
			if(m_FileInfo.tqMode == FMS_CPF_Types::tm_FILE )
			{
				if( !TQChecker::instance()->createTQFolder(m_FileInfo.transferQueue) )
				{
					TRACE(cpf_CreateFileTrace, "%s", "call(), folder TQ creation fail");
					throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, "TQ folder creation failed");
				}
			}

			FMS_CPF_Types::transferMode initTQMode = m_FileInfo.tqMode;
			//create the attribute object
			FMS_CPF_Attribute InfFileAttributes(FMS_CPF_Types::createFileAttr(FMS_CPF_Types::ft_INFINITE, m_FileInfo.recordLength, m_FileInfo.composite, m_FileInfo.maxSize, m_FileInfo.maxTime,m_FileInfo.releaseCondition, activeSubfile, lastSentSubfile, m_FileInfo.tqMode, m_FileInfo.transferQueue, initTQMode) );
			FMS_CPF_FileId fileid(m_FileInfo.fileName);

			TRACE(cpf_CreateFileTrace, "%s", "call(), creating a infinite file");
			FileReference fRef = DirectoryStructureMgr::instance()->create(fileid, m_FileInfo.volumeName,
																		   InfFileAttributes,
																		   FMS_CPF_Types::NONE_,
																		   m_FileInfo.fileDN,
																		   m_FileInfo.cpName.c_str()
																		   );

			//Close the create file
			DirectoryStructureMgr::instance()->closeExceptionLess(fRef, m_FileInfo.cpName.c_str());

			m_result.set(resultOK);
			TRACE(cpf_CreateFileTrace, "%s", "call(), infinite file created");
		}
		catch(FMS_CPF_PrivateException& ex )
		{
			TRACE(cpf_CreateFileTrace, "%s", "call(), exception on infinite file creation");
			char errorMsg[1024]={'\0'};
			snprintf(errorMsg, 1023,"CPF_CreateFile_Request::call(), creation of the infinite file=<%s> failed,  error=<%s>", m_FileInfo.fileName.c_str(), ex.errorText() );
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);

			m_result.set(ex);
		}
	}
	TRACE(cpf_CreateFileTrace, "%s", "Leaving call()");
	return 0;
}

/*============================================================================
	ROUTINE: ~CPF_CreateFile_Request
 ============================================================================ */
CPF_CreateFile_Request::~CPF_CreateFile_Request()
{
	if(NULL != cpf_CreateFileTrace)
			delete cpf_CreateFileTrace;
}
