/*
 * * @file fms_cpf_filetransferhndl.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_FileTransferHndl.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_filetransferhndl.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-03-15
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
 *	| 1.0.0  | 2012-03-15 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_oi_backuprestore.h"
#include "fms_cpf_common.h"
#include "acs_apgcc_adminoperation.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include <syslog.h>

#include "boost/random.hpp"
#include <ctime>


namespace backupObject{

	const char ImmImplementerName[] = "CPF_OI_Backup";
}
/*============================================================================
	METHOD: FMS_CPF_OI_BackupRestore
 ============================================================================ */
FMS_CPF_OI_BackupRestore::FMS_CPF_OI_BackupRestore() :
	acs_apgcc_objectimplementerinterface_V3(BRInfo::cpfBackupDN, backupObject::ImmImplementerName, ACS_APGCC_ONE),
	m_BackupState(false)
{
	m_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_OI_BackupRestore");
}

/*============================================================================
	METHOD: create
 ============================================================================ */
void FMS_CPF_OI_BackupRestore::adminOperationCallback(ACS_APGCC_OiHandle oiHandle,ACS_APGCC_InvocationType invocation, const char* p_objName, ACS_APGCC_AdminOperationIdType operationId,  ACS_APGCC_AdminOperationParamType** paramList)
{
	TRACE(m_trace, "adminOperationCallback(), action request:<%d>", operationId);

	// To avoid warning about unused parameter
	UNUSED(p_objName);

	unsigned long long requestIdValue = 0;

	// extract the parameters
	for(size_t idx = 0; paramList[idx] != NULL ; ++idx)
	{
		// check the parameter name
		if( 0 == ACE_OS::strcmp(BRInfo::requestIdParameter, paramList[idx]->attrName) )
		{
			requestIdValue = (*reinterpret_cast<unsigned long long*>(paramList[idx]->attrValues));
			TRACE(m_trace, "adminOperationCallback(...), requestID:<%llu>", requestIdValue);
			break;
		}
	}

	ACS_CC_ReturnType result = adminOperationResult(oiHandle, invocation, actionResult::SUCCESS);

	if(ACS_CC_SUCCESS != result)
	{
		TRACE(m_trace, "%s", "adminOperationCallback(), error on action result reply");
		syslog(LOG_ERR, "FMS_CPF_OI_BackupRestore::adminOperationCallback(), error on action result reply");
	}

	backupAction actionToExe = (backupAction)operationId;

	syslog(LOG_INFO, "admin operation, operationId:<%d>", static_cast<int>(operationId));
	// check the operation type
	switch(actionToExe)
	{
		case PERMIT_BACKUP2 :
		{
			m_BackupState = true;
			TRACE(m_trace, "%s", "adminOperationCallback(), received permit backup");
			break;
		}

		case PREPARE_BACKUP2:
		{
			TRACE(m_trace, "%s", "adminOperationCallback(), received prepare backup");
			break;
		}

		case CANCEL_BACKUP2 :
		case COMMIT_BACKUP2 :
		{
			m_BackupState = false;
			TRACE(m_trace, "%s", "adminOperationCallback(), backup terminated");
			break;
		}
	}

	// call action result
	replyToBRF(requestIdValue, actionToExe);

	TRACE(m_trace, "%s", "Leaving adminOperationCallback()");
}

/*============================================================================
	METHOD: replyToBRF
 ============================================================================ */
void FMS_CPF_OI_BackupRestore::replyToBRF(unsigned long long& requestId, backupAction operationID)
{
	TRACE(m_trace, "%s", "Entering in replyToBRF()");

	std::vector<ACS_APGCC_AdminOperationParamType> actionParameters;

	ACS_APGCC_AdminOperationParamType requestParameter;
	requestParameter.attrName = BRInfo::requestIdParameter;
	requestParameter.attrType = ATTR_UINT64T;
	requestParameter.attrValues = reinterpret_cast<void*>(&requestId);

	actionParameters.push_back(requestParameter);

	ACS_APGCC_AdminOperationParamType resulCodeParameter;
	resulCodeParameter.attrName = BRInfo::resulCodeParameter;
	resulCodeParameter.attrType = ATTR_INT32T;
	resulCodeParameter.attrValues = reinterpret_cast<void*>(&BRInfo::brfSuccess);

	actionParameters.push_back(resulCodeParameter);

	ACS_APGCC_AdminOperationParamType messageParameter;
	messageParameter.attrName = BRInfo::messageParameter;
	messageParameter.attrType = ATTR_STRINGT;
	char messageValue[] = " ";
	void* tmpValue[1]={ reinterpret_cast<void*>(messageValue) };
	messageParameter.attrValues = tmpValue;

	actionParameters.push_back(messageParameter);

	acs_apgcc_adminoperation brfActionInvoke;

	syslog(LOG_INFO, "replyToBRF(), requestId:<%llu>, resultCode:<%d>", requestId, BRInfo::brfSuccess);

	if( brfActionInvoke.init() == ACS_CC_SUCCESS)
	{
		int returnValue = 0;

		// The continuationId parameter must be set to 0
		// if the invocation shall not be continued
		ACS_APGCC_ContinuationIdType continuationId = 0;

		// set time-out to 30sec
		long long int timeOutVal = 30*(1000000000LL);

		ACS_CC_ReturnType result;
		int maxRetry = 60;
		bool tryAgain = true;

		do{

			result = brfActionInvoke.adminOperationInvoke(BRInfo::backupParent,
															continuationId,
															BRInfo::reportActionResult,
															actionParameters,
															&returnValue,
															timeOutVal);

			if(ACS_CC_SUCCESS != result)
			{
				char errMsg[512] = {0};
				ACE_OS::snprintf(errMsg, 511, "replyToBRF(operationId:<%d>), error:<%d> on adminOperationInvoke() on object:<%s>, returnValue:<%d>", operationID,
																																					 brfActionInvoke.getInternalLastError(),
																																					 BRInfo::backupParent,
																																					 returnValue);
				TRACE(m_trace, "%s", errMsg);

				// wait before call the action result, without it always the action call fails
				{
					boost::mt19937 gen;
					gen.seed(static_cast<unsigned int>(std::time(0)));
					boost::uniform_int<> random_milliseconds( 500, 999 );

					int milliseconds = random_milliseconds(gen);
					long microseconds = milliseconds * 1000;

					ACE_Time_Value interval(0, microseconds);
					ACE_OS::sleep(interval);
				}

				maxRetry--;
			}
			else
			{
				syslog(LOG_INFO, "replyToBRF(), success: return code from BRF <%d>", returnValue);
				TRACE(m_trace, "replyToBRF(), success: return code from BRF <%d>", returnValue);
				tryAgain = false;
			}

		}while(tryAgain && (maxRetry > 0));

		brfActionInvoke.finalize();
	}
	else
		syslog(LOG_INFO, "replyToBRF(), error:<%d> on admin init", brfActionInvoke.getInternalLastError());

	TRACE(m_trace, "%s", "Leaving replyToBRF()");
}

/*============================================================================
	METHOD: create
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_BackupRestore::create(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *className, const char* parentname, ACS_APGCC_AttrValues **attr)
{
	UNUSED(oiHandle);
	UNUSED(ccbId);
	UNUSED(className);
	UNUSED(parentname);
	UNUSED(attr);
	return ACS_CC_FAILURE;
}


/*============================================================================
	METHOD: deleted
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_BackupRestore::deleted(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName)
{
	UNUSED(oiHandle);
	UNUSED(ccbId);
	UNUSED(objName);
	return ACS_CC_FAILURE;
}

/*============================================================================
	METHOD: modify
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_BackupRestore::modify(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName, ACS_APGCC_AttrModification **attrMods)
{
	UNUSED(oiHandle);
	UNUSED(ccbId);
	UNUSED(objName);
	UNUSED(attrMods);
	return ACS_CC_FAILURE;
}

/*============================================================================
	METHOD: complete
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_BackupRestore::complete(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	UNUSED(oiHandle);
	UNUSED(ccbId);
	return ACS_CC_FAILURE;
}

/*============================================================================
	METHOD: abort
 ============================================================================ */
void FMS_CPF_OI_BackupRestore::abort(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	UNUSED(oiHandle);
	UNUSED(ccbId);
}

/*============================================================================
	METHOD: apply
 ============================================================================ */
void FMS_CPF_OI_BackupRestore::apply(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	UNUSED(oiHandle);
	UNUSED(ccbId);
}

/*============================================================================
	METHOD: complete
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_BackupRestore::updateRuntime(const char* p_objName, const char** p_attrName)
{
	UNUSED(p_objName);
	UNUSED(p_attrName);
	return ACS_CC_FAILURE;
}

/*============================================================================
	METHOD: ~FMS_CPF_OI_BackupRestore
 ============================================================================ */
FMS_CPF_OI_BackupRestore::~FMS_CPF_OI_BackupRestore()
{
	if(NULL != m_trace)
		delete m_trace;
}
