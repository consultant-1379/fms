/*
 * * @file fms_cpf_changefileattribute.cpp
 *	@brief
 *	Class method implementation for CPF_ChangeFileAttribute_Request.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_changefileattribute.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-08-27
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
 *	| 1.0.0  | 2011-08-27 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_changefileattribute.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filereference.h"
#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_tqchecker.h"
#include "fms_cpf_common.h"

#include "fms_cpf_fileid.h"
#include "fms_cpf_attribute.h"
#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;


/*============================================================================
	ROUTINE: CPF_ChangeFileAttribute_Request
 ============================================================================ */
CPF_ChangeFileAttribute_Request::CPF_ChangeFileAttribute_Request(ACE_Future<FMS_CPF_PrivateException>& result, cpFileModData& fileInfo)
:m_result(result), m_FileInfo(fileInfo)
{
	cpf_ChangeAttrTrace = new (std::nothrow) ACS_TRA_trace("CPF_ChangeFileAttribute_Request");

}

/*============================================================================
	ROUTINE: call
 ============================================================================ */
int CPF_ChangeFileAttribute_Request::call()
{
	TRACE(cpf_ChangeAttrTrace, "%s", "Entering in call()");
	FMS_CPF_FileId fileid( m_FileInfo.currentFileName );
	FileReference reference;
	FMS_CPF_PrivateException resultOK(FMS_CPF_Exception::OK);

	try
	{
		// Open logical file
		reference = DirectoryStructureMgr::instance()->open(fileid, FMS_CPF_Types::NONE_, m_FileInfo.cpName.c_str());
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		TRACE(cpf_ChangeAttrTrace, "call(), catched an exception on file=<%s> open", m_FileInfo.currentFileName.c_str());
		char errorMsg[512];
		snprintf(errorMsg, 511, "CPF_ChangeFileAttribute_Request::call(), Change attribute mask<%d> of file=<%s> open file failed, error=<%s>", m_FileInfo.changeFlags, m_FileInfo.currentFileName.c_str(), ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		m_result.set(ex);
		TRACE(cpf_ChangeAttrTrace, "%s", "Leaving call()");
		return 0;
	}

	try
	{

		TRACE(cpf_ChangeAttrTrace, "%s", "call(), infinite file attribute change");
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
                 //HY46076
                long int deleteFileTimer=-1;

		FMS_CPF_Types::extractFileAttr(fileAttribute, fType, recordLength, composite, maxSize,
									   maxTime, releaseCondition ,activeSubFile, lastSent, transferMode,
									   transferQueue, initTransferMode,deleteFileTimer
									  );

		// check for a max time change
		if( (m_FileInfo.changeFlags & changeSet::MAXTIME_CHANGE))
		{
			TRACE(cpf_ChangeAttrTrace, "call(), change max time from <%d> to <%d>", maxTime, m_FileInfo.newMaxTime);
			unsigned long tmpValue = maxTime;
			maxTime = m_FileInfo.newMaxTime;
			// for undo operation
			m_FileInfo.newMaxTime = tmpValue;
		}

		// check for a max size change
		if( (m_FileInfo.changeFlags & changeSet::MAXSIZE_CHANGE))
		{
			TRACE(cpf_ChangeAttrTrace, "call(), change max size from <%d> to <%d>", maxSize, m_FileInfo.newMaxSize);
			unsigned long tmpValue = maxSize;
			maxSize = m_FileInfo.newMaxSize;
			// for undo operation
			m_FileInfo.newMaxSize = tmpValue;
		}

		// check for a release condition change
		if( (m_FileInfo.changeFlags & changeSet::RELCOND_CHANGE))
		{
			bool tmpValue = releaseCondition;
			releaseCondition = m_FileInfo.newReleaseCondition;
			// for undo operation
			m_FileInfo.newReleaseCondition = tmpValue;
			TRACE(cpf_ChangeAttrTrace, "call(), set release condition to <%s>", (releaseCondition ? "TRUE":"FALSE") );
		}

		// check for a TQ change
		if( (m_FileInfo.changeFlags & changeSet::FILETQ_CHANGE))
		{
			TRACE(cpf_ChangeAttrTrace, "call(), change TQ from <%s> to <%s>, init mode:<%d>, new mode:<%d>", transferQueue.c_str(),
																	m_FileInfo.newTransferQueue.c_str(), initTransferMode, m_FileInfo.newTQMode );

			if( (FMS_CPF_Types::tm_NONE != m_FileInfo.newTQMode) &&
					(initTransferMode != m_FileInfo.newTQMode) &&
					(FMS_CPF_Types::tm_NONE != initTransferMode) )
			{
				TRACE(cpf_ChangeAttrTrace, "%s", "call(), change TQ type not allowed");
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INVTRANSFERMODE);
			}

			// for undo operation save current TQ name and mode
			m_FileInfo.oldTransferQueue = transferQueue;
			m_FileInfo.oldTQMode = transferMode;

			transferQueue = m_FileInfo.newTransferQueue;
			transferMode = m_FileInfo.newTQMode;

			if( FMS_CPF_Types::tm_NONE == initTransferMode)
					initTransferMode = transferMode;

			if( (FMS_CPF_Types::tm_FILE == transferMode) && !transferQueue.empty() )
			{
				// Create TQ folder if not exist
				if( !TQChecker::instance()->createTQFolder(transferQueue) )
				{
					TRACE(cpf_ChangeAttrTrace, "%s", "call(), folder TQ creation fail");
					throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, "TQ folder creation failed");
				}
			}

			// Clear TQ DN info
			reference->clearTransferQueueDN();
		}
                             //HY46076
               if( (m_FileInfo.changeFlags & changeSet::DELETEFILETIMER_CHANGE))
               {
                       TRACE(cpf_ChangeAttrTrace, "call(), change DELETEFILETIMER from <%d> to <%d>", deleteFileTimer,m_FileInfo.newdeleteFileTimer);
                       long int tmpValue = deleteFileTimer;
                       deleteFileTimer = m_FileInfo.newdeleteFileTimer;
                       // for undo operation
                      // m_FileInfo.newdeleteFileTimer = tmpValue;
               } //HY46076 end
            


		// Assemble the file attribute with old and new parameters value
		FMS_CPF_Types::fileAttributes newFileAttributes =
				FMS_CPF_Types::createFileAttr(fType, recordLength, composite, maxSize, maxTime,
											  releaseCondition, activeSubFile, lastSent, transferMode,
											  transferQueue, initTransferMode,deleteFileTimer
											 );

		FMS_CPF_Attribute newFileAttribute(newFileAttributes);
		TRACE(cpf_ChangeAttrTrace, "%s", "call(), infinite file set new attribute");
		// Change file attributes
		reference->setAttribute(newFileAttribute);

		// set operation result to OK
		m_result.set(resultOK);
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		TRACE(cpf_ChangeAttrTrace, "call(), exception on file=<%s> attribute changes", m_FileInfo.currentFileName.c_str());
		char errorMsg[1024];
		snprintf(errorMsg, 1023,"CPF_ChangeFileAttribute_Request::call(), Change attribute on file:<%s> failed, error:<%s>", m_FileInfo.currentFileName.c_str(), ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		m_result.set(ex);
	}

	DirectoryStructureMgr::instance()->closeExceptionLess(reference, m_FileInfo.cpName.c_str());

	TRACE(cpf_ChangeAttrTrace, "%s", "Leaving call()");
	return 0;
}

/*============================================================================
	ROUTINE: ~CPF_ChangeFileAttribute_Request
 ============================================================================ */
CPF_ChangeFileAttribute_Request::~CPF_ChangeFileAttribute_Request()
{
	if(NULL != cpf_ChangeAttrTrace)
				delete cpf_ChangeAttrTrace;
}
