/*
 * * @file fms_cpf_removevolume.cpp
 *	@brief
 *	Class method implementation for CPF_RemoveVolume_Request.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_removevolume.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-28
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
 *	| 1.0.0  | 2011-06-28 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_removevolume.h"
#include "fms_cpf_common.h"
#include "fms_cpf_parameterhandler.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	ROUTINE: CPF_RemoveVolume_Request
 ============================================================================ */
CPF_RemoveVolume_Request::CPF_RemoveVolume_Request(ACE_Future<FMS_CPF_PrivateException>& result, const systemData& vInfo)
:m_result(result),
 m_SystemInfo(vInfo)
{
	cpf_RemoveVolumeTrace = new (std::nothrow) ACS_TRA_trace("CPF_RemoveVolume_Request");
}

/*============================================================================
	ROUTINE: call
 ============================================================================ */
int CPF_RemoveVolume_Request::call()
{
	TRACE(cpf_RemoveVolumeTrace, "%s", "Entering call()");
	std::string volumePath;

	// Get the default path for CPF.
	// In SCP the cpName is empty
	volumePath = ParameterHndl::instance()->getCPFroot(m_SystemInfo.cpName.c_str());

	// Assemble the full path
	volumePath += DirDelim + m_SystemInfo.volumeName;

	TRACE(cpf_RemoveVolumeTrace, "call(), remove the directory:<%s>", volumePath.c_str() );

	// Physical remove of the volume
	// operation will fail if the folder is not empty
	int operationResult = ACE_OS::rmdir(volumePath.c_str());

	// check operation result, ok also when the folder does not exist
	if( (-1 == operationResult ) && (ENOENT != errno) )
	{
		// remove volume failed
		char errorText[256]={0};
		std::string errorDetail(strerror_r(errno, errorText, 255));

		char logBuffer[256]={0};
		snprintf(logBuffer, 255, "CPF_RemoveVolume_Request, error:<%s> on volume:<%s> remove", errorDetail.c_str(), volumePath.c_str() );
		CPF_Log.Write(logBuffer, LOG_LEVEL_ERROR);
		TRACE(cpf_RemoveVolumeTrace, "%s", logBuffer);

		FMS_CPF_PrivateException errorMsg(FMS_CPF_Exception::PHYSICALERROR, errorDetail);
		m_result.set(errorMsg);
	}
	else
	{
		TRACE(cpf_RemoveVolumeTrace, "%s", "call(), volume removed");
		FMS_CPF_PrivateException resultOK(FMS_CPF_Exception::OK);
		m_result.set(resultOK);
	}

	TRACE(cpf_RemoveVolumeTrace, "%s", "Leaving call()");
	return 0;
}

/*============================================================================
	ROUTINE: ~CPF_RemoveVolume_Request
 ============================================================================ */
CPF_RemoveVolume_Request::~CPF_RemoveVolume_Request()
{
	if(NULL != cpf_RemoveVolumeTrace)
		delete cpf_RemoveVolumeTrace;
}

