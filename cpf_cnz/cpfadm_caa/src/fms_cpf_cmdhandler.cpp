/*
 * * @file fms_cpf_cmdhandler.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_CmdHandler.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_cmdhandler.h module
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

#include "fms_cpf_cmdhandler.h"
#include "fms_cpf_common.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

namespace cmdHandler{
  const int threadTermination = -1;
};
/*============================================================================
	ROUTINE: FMS_CPF_CmdHandler
 ============================================================================ */
FMS_CPF_CmdHandler::FMS_CPF_CmdHandler()
{
	// Initialize the svc state flag
	svc_run = false;

	fms_cpf_cmdhandlerTrace = new ACS_TRA_trace("FMS_CPF_CmdHandler");
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
FMS_CPF_CmdHandler::~FMS_CPF_CmdHandler()
{
	// Delete all queued request
	while(!((bool)m_Activation_Queue.is_empty()) )
	{
		// Dequeue the next method object
		auto_ptr<ACE_Method_Request> cmdRequest(m_Activation_Queue.dequeue());
	}
	if(NULL != fms_cpf_cmdhandlerTrace)
		delete fms_cpf_cmdhandlerTrace;
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
int FMS_CPF_CmdHandler::open(void *args)
{
	TRACE(fms_cpf_cmdhandlerTrace, "%s", "Entering open()");
	int result;
	//To avoid unused warning
	UNUSED(args);

	svc_run = true;
	result = activate();

    if(0 != result)
    {
    	CPF_Log.Write("FMS_CPF_CmdHandler::open,  error on start svc thread", LOG_LEVEL_ERROR);
    	TRACE(fms_cpf_cmdhandlerTrace, "%s", "open(), error on start svc thread");
    	svc_run = false;
    }
    TRACE(fms_cpf_cmdhandlerTrace, "%s", "Leaving open()");
	return result;
}

/*============================================================================
	ROUTINE: svc
 ============================================================================ */
int FMS_CPF_CmdHandler::svc()
{
	TRACE(fms_cpf_cmdhandlerTrace, "%s", "Entering svc()");
	int result = 0;

	while(svc_run)
	{
		// Dequeue the next method object
		auto_ptr<ACE_Method_Request> cmdRequest(m_Activation_Queue.dequeue());

		if(cmdRequest->call() == cmdHandler::threadTermination )
		{
			svc_run = false;
		}
	}
	TRACE(fms_cpf_cmdhandlerTrace, "%s", "Leaving svc()");
	return result;
}

/*============================================================================
	ROUTINE: enqueue
 ============================================================================ */
int FMS_CPF_CmdHandler::enqueue(ACE_Method_Request* cmdRequest)
{
	return m_Activation_Queue.enqueue(cmdRequest);
}
