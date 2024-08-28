/*
 * fms_cpf_admin_operation.cpp
 *
 *  Created on: Oct 17, 2011
 *      Author: enungai
 */

#include "fms_cpf_api_trace.h"
#include "fms_cpf_adminoperation.h"
#include "fms_cpf_exception.h"

#include "ACS_TRA_trace.h"
#include <sys/eventfd.h>

#include <sstream>

namespace actionResult {
	int SUCCESS = 1; // SA_AIS_OK
	int FAILED = 21; // SA_AIS_ERR_FAILED_OPERATION
	int NO_OPERATION = 28; // SA_AIS_ERR_NO_OP
};


namespace exitCodePar{
	char errorCode[] = "errorCode";
	char errorMessage[] = "errorMessage";
}

/*============================================================================
	ROUTINE: FMS_CPF_AdminOperation
 ============================================================================ */
FMS_CPF_AdminOperation::FMS_CPF_AdminOperation():
		m_ActionExitCode(FMS_CPF_Exception::OK),
		m_ActionErrorDetail(),
		m_InternalError(actionResult::SUCCESS)
{
	// create the file descriptor to signal stop
	m_StopEvent = eventfd(0,0);
	fmsCpfAsyncAction = new (std::nothrow) ACS_TRA_trace("FMS_CPF_AdminOperation");
}

/*============================================================================
	ROUTINE: ~FMS_CPF_AdminOperation
 ============================================================================ */
FMS_CPF_AdminOperation::~FMS_CPF_AdminOperation()
{
	// In order to close the handle
	::close(m_StopEvent);

	if(NULL != fmsCpfAsyncAction)
		delete fmsCpfAsyncAction;
}

/*============================================================================
	ROUTINE: sendAsyncActionToServer
 ============================================================================ */
int FMS_CPF_AdminOperation::sendAsyncActionToServer(const std::string& p_objName, ACS_APGCC_AdminOperationIdType operationId, std::vector<ACS_APGCC_AdminOperationParamType>& paramVector)
{
	TRACE(fmsCpfAsyncAction, "---------------------- SendAsyncActionToServer(), object DN:<%s>, OperationId:<%d>", p_objName.c_str(), operationId);

	m_ActionExitCode = FMS_CPF_Exception::GENERAL_FAULT;
	ACS_CC_ReturnType result;

	TRACE(fmsCpfAsyncAction, "%s", "SendAsyncActionToServer(), init()" );
	// Allocate IMM resources
	result = acs_apgcc_adminoperationasync_V2::init();

	TRACE(fmsCpfAsyncAction, "%s", "SendAsyncActionToServer(), post init()" );
	// Check if the Async Action Class has been initialized
	if(ACS_CC_SUCCESS == result)
	{
		TRACE(fmsCpfAsyncAction, "%s", "SendAsyncActionToServer(), activate()" );
		// start event loop and
		// Check if the svc thread is started
		if(0 == activate( (THR_NEW_LWP| THR_JOINABLE | THR_INHERIT_SCHED)) )
		{
			// prepare invocation and operation
			ACS_APGCC_InvocationType invocation = 1;
			TRACE(fmsCpfAsyncAction, "%s", "SendAsyncActionToServer(), adminOperationInvokeAsync()" );
			// Invoke action
			result = adminOperationInvokeAsync(invocation, p_objName.c_str(), 0, operationId, paramVector);

			if(ACS_CC_SUCCESS != result)
			{
				// action invoke fails
				m_InternalError = getInternalLastError();
				TRACE(fmsCpfAsyncAction, "SendAsyncActionToServer(), error:<%d> on async operation invoke", m_InternalError );
				// operation call failed stop internal thread
				stopInternalThread();
			}

			TRACE(fmsCpfAsyncAction, "%s", "SendAsyncActionToServer(), wait internal thread closure" );

			// Wait for the svc closure
			wait();
		}
		else
		{
			TRACE(fmsCpfAsyncAction, "%s", "SendAsyncActionToServer(), failed to start internal thread");
		}

		// Deallocate IMM resources
		acs_apgcc_adminoperationasync_V2::finalize();
	}
	else
	{
		m_InternalError = getInternalLastError();
		TRACE(fmsCpfAsyncAction, "SendAsyncActionToServer(), error:<%d> on init()", m_InternalError);
	}

	TRACE(fmsCpfAsyncAction, "Leaving SendAsyncActionToServer(), return exitCode:<%d>", m_ActionExitCode);
	return m_ActionExitCode;
}

/*============================================================================
	ROUTINE: svc
 ============================================================================ */
int FMS_CPF_AdminOperation::svc()
{
	TRACE(fmsCpfAsyncAction, "%s", "Entering in svc");
	bool loop = true;

	const nfds_t nfds = 2;
	struct pollfd fds[nfds];
	ACE_INT32 ret;

	// Initialize the pollfd structure
	ACE_OS::memset(fds, 0 , sizeof(fds));

	fds[0].fd = m_StopEvent;
	fds[0].events = POLLIN;

	fds[1].fd = getSelObj();
	fds[1].events = POLLIN;

	while(loop)
	{
		ret = ACE_OS::poll(fds, nfds);
		if( -1 == ret )
		{
			if(errno == EINTR)
			{
				continue;
			}
			break;
		}

	    if(fds[0].revents & POLLIN)
		{
	    	// Received a IMM request
	    	TRACE(fmsCpfAsyncAction, "%s", "svc(), stop event received");
			break;
		}

	    if(fds[1].revents & POLLIN)
		{
			// Received a IMM callback
	    	TRACE(fmsCpfAsyncAction, "%s", "svc(), callback event received");
	    	acs_apgcc_adminoperationasync_V2::dispatch(ACS_APGCC_DISPATCH_ONE);
			break;
		}
	}

	TRACE(fmsCpfAsyncAction, "%s", "Leaving svc()");
	return 0;
}

/*============================================================================
	ROUTINE: objectManagerAdminOperationCallback
 ============================================================================ */
void FMS_CPF_AdminOperation::objectManagerAdminOperationCallback( ACS_APGCC_InvocationType invocation,
																  int returnVal,
																  int error,
																  ACS_APGCC_AdminOperationParamType** outParamVector
																)
{
	TRACE(fmsCpfAsyncAction, "CallBack received, invocation:<%d>, returnVal:<%d>, error:<%d>",invocation, returnVal, error);

	m_InternalError = error;

	// Check if a IMM error occurs
	if(actionResult::SUCCESS == error)
	{
		TRACE(fmsCpfAsyncAction, "%s", "CallBack, get server operation reply");
		// Get Server answer to the action
		// extract the parameters
		for(size_t idx = 0; outParamVector[idx] != NULL ; idx++)
		{
			// get errorCode parameter
			if( 0 == ACE_OS::strcmp(exitCodePar::errorCode, outParamVector[idx]->attrName) )
			{
				std::stringstream erroCodeStr(reinterpret_cast<char *>(outParamVector[idx]->attrValues));
				erroCodeStr >> m_ActionExitCode;

				TRACE(fmsCpfAsyncAction, "CallBack, found errorCode:<%d>", m_ActionExitCode );
			}

			// get errorDetail parameter
			if( 0 == ACE_OS::strcmp(exitCodePar::errorMessage, outParamVector[idx]->attrName) )
			{
				m_ActionErrorDetail = reinterpret_cast<char *>(outParamVector[idx]->attrValues);
				TRACE(fmsCpfAsyncAction, "CallBack, found errorDetail:<%s>", m_ActionErrorDetail.c_str() );
			}
		}
	}

	TRACE(fmsCpfAsyncAction, "%s", "Leaving CallBack");

}

/*============================================================================
	ROUTINE: stopInternalThread
 ============================================================================ */
void FMS_CPF_AdminOperation::stopInternalThread()
{
	TRACE(fmsCpfAsyncAction, "%s", "Entering stopInternalThread");
	ssize_t numByte;
	ACE_UINT64 stopEvent=1;

	// Signal to server to stop
	numByte = ::write(m_StopEvent, &stopEvent, sizeof(ACE_UINT64));

	TRACE(fmsCpfAsyncAction, "%s", "Leaving stopInternalThread");
}

