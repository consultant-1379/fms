/*
 * * @file fms_cpf_datafile.cpp
 *	@brief
 *	Class method implementation for CPF_DataFile_Request.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_datafile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-09
 *	@version 1.0.0
 *
 *	COPYRIGHT Ericsson AB, 2010
 *	All rights reserved.
 *
 *	The information in this document is the property of Ericsson.
 *	Except as specifically authorized in writing by Ericsson, the receiver of
 *	this document shall keep the information contained herein confidential and
 *	shall protect the same in whole or in part from disclosure and dissemination
 *	to third parties. Disclosure and dissemination to the receivers employees
 *	shall only be made on a strict need to know basis.
 *
 *	REVISION INFO
 *	+========+============+==============+=====================================+
 *	| REV    | DATE       | AUTHOR       | DESCRIPTION                         |
 *	+========+============+==============+=====================================+
 *	| 1.0.0  | 2011-11-09 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_datafile.h"
#include "fms_cpf_common.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_filereference.h"

#include "fms_cpf_fileid.h"
#include "fms_cpf_types.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;
/*============================================================================
	ROUTINE: CPF_DataFile_Request
 ============================================================================ */
CPF_DataFile_Request::CPF_DataFile_Request(ACE_Future<FMS_CPF_PrivateException>& result, dataFile& fileInfo, bool isMainFileInfo, bool skipSize )
: m_result(result),
  m_FileInfo(fileInfo),
  m_ExtraInfiniteFileInfo(isMainFileInfo),
  m_SkipSize(skipSize)
{
	cpf_DataFileTrace = new (std::nothrow) ACS_TRA_trace("CPF_DataFile_Request");
}

/*============================================================================
	ROUTINE: call
 ============================================================================ */
int CPF_DataFile_Request::call()
{
	TRACE(cpf_DataFileTrace, "%s", "Entering in call()");
	FMS_CPF_PrivateException resultOK(FMS_CPF_Exception::OK);

	std::string fileName(m_FileInfo.mainFileName);

	if(!m_ExtraInfiniteFileInfo && !m_FileInfo.subFileName.empty() )
		fileName += (SubFileSep + m_FileInfo.subFileName);

	TRACE(cpf_DataFileTrace, "call(), get info of file=<%s>, cpName:<%s>", fileName.c_str(), m_FileInfo.cpName.c_str() );

	FMS_CPF_FileId fileId(fileName);
	FileReference reference;

	// Open the CP file
	try
	{
		// Open logical src file
		reference = DirectoryStructureMgr::instance()->open(fileId, FMS_CPF_Types::NONE_, m_FileInfo.cpName.c_str());
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		TRACE(cpf_DataFileTrace, "call(), catched an exception on file=<%s> open", fileName.c_str());
		char errorMsg[1024]={'\0'};
		snprintf(errorMsg, 1023,"Data request on file=<%s>, open failed, error=<%s>", fileName.c_str(), ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		// Set operation result
		m_result.set(ex);
		return SUCCESS;
	}

	// Get the info on CP file
	try
	{
		// Check if infinite file info
		if(m_ExtraInfiniteFileInfo)
		{
			// get main infinite file info
			m_FileInfo.lastSubFileActive = static_cast<unsigned int>(reference->getActiveSubfile());
			m_FileInfo.lastSubFileSent = static_cast<unsigned int>(reference->getLastSentSubfile());
			reference->getTransferQueueDN(m_FileInfo.transferQueueDn);

			// get active subfile info
			getActiveSubfileInfo();
		}

		// Get the user structure with user access info
		FMS_CPF_Types::userType users = reference->getUsers();

		m_FileInfo.numOfReaders = 1;
		if( FMS_CPF_EXCLUSIVE != users.rcount)
			m_FileInfo.numOfReaders = users.rcount;

		m_FileInfo.numOfWriters = 1;
		if( FMS_CPF_EXCLUSIVE != users.wcount)
			m_FileInfo.numOfWriters = users.wcount;

		m_FileInfo.exclusiveAccess = 0;
		if( (FMS_CPF_EXCLUSIVE == users.wcount) || (FMS_CPF_EXCLUSIVE == users.rcount) )
			m_FileInfo.exclusiveAccess = 1;

		// get the file size as blocks number
		if(!m_SkipSize)
			m_FileInfo.fileSize = reference->getSize();

		// Set operation result
		m_result.set(resultOK);

	}
	catch(const FMS_CPF_PrivateException& ex)
	{
		TRACE(cpf_DataFileTrace, "call(), catched an exception on file=<%s> data request", fileName.c_str());
		char errorMsg[1024]={'\0'};
		snprintf(errorMsg, 1023,"Data request on file=<%s>,  error=<%s>", fileName.c_str(), ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		// Set operation result
		m_result.set(ex);
	}

	// Close file
	DirectoryStructureMgr::instance()->closeExceptionLess(reference, m_FileInfo.cpName.c_str());
	TRACE(cpf_DataFileTrace, "%s", "Leaving call()");
	return SUCCESS;
}

/*============================================================================
	ROUTINE: ~CPF_DataFile_Request
 ============================================================================ */
CPF_DataFile_Request::~CPF_DataFile_Request()
{
	if(NULL != cpf_DataFileTrace)
		delete cpf_DataFileTrace;
}

void CPF_DataFile_Request::getActiveSubfileInfo()
{
	TRACE(cpf_DataFileTrace, "Entering in %s", __func__);
	m_FileInfo.activeSubfileSize = 0U;
	m_FileInfo.activeSubfileNumOfReaders = 1U;
	m_FileInfo.activeSubfileNumOfWriters = 1U;
	m_FileInfo.activeSubfileExclusiveAccess = 0U;

	char subfileName[64] = {0};
	snprintf(subfileName, sizeof(subfileName)-1, "%s%c%.10d",
					m_FileInfo.mainFileName.c_str(), SubFileSep, m_FileInfo.lastSubFileActive);

	TRACE(cpf_DataFileTrace, "get subfile:<%s> info", subfileName);
	FMS_CPF_FileId fileId(subfileName);
	FileReference reference;

	// Open the active subfile
	try
	{
		// Open logical src file
		reference = DirectoryStructureMgr::instance()->open(fileId, FMS_CPF_Types::NONE_, m_FileInfo.cpName.c_str());

		// get the file size as blocks number
		m_FileInfo.activeSubfileSize = reference->getSize();

		// Get the user structure with user access info
		FMS_CPF_Types::userType users = reference->getUsers();

		if( FMS_CPF_EXCLUSIVE != users.rcount)
			m_FileInfo.activeSubfileNumOfReaders = users.rcount;

		if( FMS_CPF_EXCLUSIVE != users.wcount)
			m_FileInfo.activeSubfileNumOfWriters = users.wcount;

		if( (FMS_CPF_EXCLUSIVE == users.wcount) || (FMS_CPF_EXCLUSIVE == users.rcount) )
			m_FileInfo.activeSubfileExclusiveAccess = 1;

	}
	catch(const FMS_CPF_PrivateException& ex)
	{
		char errorMsg[1024] = {0};
		snprintf(errorMsg, 1023,"%s, Data request on active subfile:<%s>,  error:<%s>", __func__, subfileName, ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(cpf_DataFileTrace, "%s", errorMsg);
	}

	// Close subfile
	DirectoryStructureMgr::instance()->closeExceptionLess(reference, m_FileInfo.cpName.c_str());

	TRACE(cpf_DataFileTrace, "Leaving %s", __func__);
}
