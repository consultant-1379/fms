/*
 * * @file fms_cpf_createvolume.cpp
 *	@brief
 *	Class method implementation for CPF_CreateVolume_Request.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_createvolume.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-23
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
 *	| 1.0.0  | 2011-06-23 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_createvolume.h"
#include "fms_cpf_common.h"
#include "fms_cpf_parameterhandler.h"

#include "ACS_TRA_trace.h"
#include "ACS_APGCC_CommonLib.h"
#include "ACS_APGCC_Util.H"
#include <ace/os_include/sys/os_stat.h>

extern ACS_TRA_Logging CPF_Log;
/*============================================================================
	ROUTINE: CPF_CreateVolume_Request
 ============================================================================ */
CPF_CreateVolume_Request::CPF_CreateVolume_Request(ACE_Future<FMS_CPF_PrivateException>& result,const systemData& vInfo)
:m_result(result),
 m_SystemInfo(vInfo)
{
	cpf_CreateVolumeTrace = new ACS_TRA_trace("CPF_CreateVolume_Request");
}

int CPF_CreateVolume_Request::call()
{
	TRACE(cpf_CreateVolumeTrace, "%s", "Entering call()");

	std::string volumePath;

	// Get the default path for CPF.
	// In SCP the cpName is empty
	volumePath = ParameterHndl::instance()->getCPFroot(m_SystemInfo.cpName.c_str());

	// Assemble the full path
	volumePath += DirDelim + m_SystemInfo.volumeName;

	TRACE(cpf_CreateVolumeTrace, "call(), create the directory= %s", volumePath.c_str() );
	//check if volume exists
	ACE_stat buffer;
	int dirStatus = ACE_OS::stat(volumePath.c_str(), &buffer);
	if( ((dirStatus == -1) && (errno == ENOENT)) || ((buffer.st_mode & S_IFDIR) != S_IFDIR) )
	{
		//create volume directory

		int createResult = ACS_APGCC::create_directories(volumePath.c_str());
		TRACE(cpf_CreateVolumeTrace, "call(), volume creation res= %i", createResult);
		if(1 == createResult )
		{
			FMS_CPF_PrivateException resultOK(FMS_CPF_Exception::OK);
			m_result.set(resultOK);
			TRACE(cpf_CreateVolumeTrace, "%s", "call(), volume created");
			TRACE(cpf_CreateVolumeTrace, "%s", "Leaving call()");
			return 0;
		}
	}
	else if( ( dirStatus == 0 ) && ( (buffer.st_mode & S_IFDIR) == S_IFDIR )  )
	{
		FMS_CPF_PrivateException resultOK(FMS_CPF_Exception::VOLUMEEXISTS, m_SystemInfo.volumeName);
		m_result.set(resultOK);
		TRACE(cpf_CreateVolumeTrace, "%s", "call(), volume already present");
		TRACE(cpf_CreateVolumeTrace, "%s", "Leaving call()");
		return 0;
	}

	TRACE(cpf_CreateVolumeTrace, "call(), error=%i on create directory", errno );

	char errorText[256]={0};
	std::string errorDetail(strerror_r(errno, errorText, 255));
	FMS_CPF_PrivateException errorMsg(FMS_CPF_Exception::PHYSICALERROR, errorDetail);
	m_result.set(errorMsg);

	char log_buff[256]={0};
	snprintf(log_buff,sizeof(log_buff)-1,"CPF_CreateVolume_Request, error<%s> on volume<%s> creation", errorDetail.c_str(), volumePath.c_str() );
	CPF_Log.Write(log_buff,	LOG_LEVEL_ERROR);

	TRACE(cpf_CreateVolumeTrace, "%s", "Leaving call()");
    return 0;
}
/*============================================================================
	ROUTINE: ~CPF_CreateVolume_Request
 ============================================================================ */
CPF_CreateVolume_Request::~CPF_CreateVolume_Request()
{
	TRACE(cpf_CreateVolumeTrace, "%s", "destroyed!!");
	if(NULL != cpf_CreateVolumeTrace)
		delete cpf_CreateVolumeTrace;
}
