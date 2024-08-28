/*
 * * @file fms_cpf_releasesubfile.cpp
 *	@brief
 *	Class method implementation for CPF_ReleaseISF_Request.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_releasesubfile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-01-12
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
 *	| 1.0.0  | 2012-01-12 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_releasesubfile.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_common.h"

#include "fms_cpf_filereference.h"
#include "fms_cpf_fileid.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;
namespace
{
	int UPDATE_RT_OBJECTS = 2;
}
/*============================================================================
	ROUTINE: CPF_ReleaseISF_Request
 ============================================================================ */
CPF_ReleaseISF_Request::CPF_ReleaseISF_Request(const fileData& fileInfo) :
m_FileInfo(fileInfo)
{
	cpf_ReleaseISFTrace = new (std::nothrow) ACS_TRA_trace("CPF_ReleaseISF_Request");
}

bool CPF_ReleaseISF_Request::executeReleaseISF(FMS_CPF_PrivateException& switchResult)
{
	TRACE(cpf_ReleaseISFTrace, "%s", "Entering in executeReleaseISF()");
	bool result = false;

	FMS_CPF_PrivateException resultOK(FMS_CPF_Exception::OK);
	FileReference srcFileReference;
	try{

		FMS_CPF_FileId srcFileId(m_FileInfo.fileName);
		TRACE(cpf_ReleaseISFTrace, "executeReleaseISF(), open infinite file<%s>", m_FileInfo.fileName.c_str());

		srcFileReference = (DirectoryStructureMgr::instance())->open(srcFileId, FMS_CPF_Types::NONE_, m_FileInfo.cpName.c_str());
		// Order the subfile switch
		srcFileReference->changeActiveSubfileOnCommand();

		TRACE(cpf_ReleaseISFTrace, "%s", "executeReleaseISF(), subfile switch ordered");

		result = true;
		switchResult = resultOK;
		DirectoryStructureMgr::instance()->closeExceptionLess(srcFileReference, m_FileInfo.cpName.c_str() );
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		switchResult = ex;
		// Close src file
		DirectoryStructureMgr::instance()->closeExceptionLess(srcFileReference, m_FileInfo.cpName.c_str() );
		TRACE(cpf_ReleaseISFTrace, "executeImport() exception=<%s%s> on subfile switch", ex.errorText(), ex.detailInfo().c_str());
	}

	TRACE(cpf_ReleaseISFTrace, "%s", "Leaving executeReleaseISF()");
	return result;
}

/*============================================================================
	ROUTINE: ~CPF_ReleaseISF_Request
 ============================================================================ */
CPF_ReleaseISF_Request::~CPF_ReleaseISF_Request()
{
	if(NULL != cpf_ReleaseISFTrace)
		delete cpf_ReleaseISFTrace;
}
