/*
 * * @file fms_cpf_cmdlistener.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_CmdListener.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_cmdlistener.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-08-04
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
 *	| 1.0.0  | 2011-08-04 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_cmdlistener.h"
#include "fms_cpf_common.h"
#include "fms_cpf_clientcmdhandler.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include "ace/TP_Reactor.h"
#include "ace/Reactor.h"

extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	ROUTINE: FMS_CPF_CmdListener
 ============================================================================ */
FMS_CPF_CmdListener::FMS_CPF_CmdListener():
m_DSDInitiate(false),
m_StopEvent(false)
{

	// Instance a Reactor to handle DSD events
	ACE_TP_Reactor* tp_reactor_impl = new ACE_TP_Reactor();

	// the reactor will delete the implementation on destruction
	m_CmdListenerReactor = new ACE_Reactor(tp_reactor_impl, true);

	// Initialize the ACE_Reactor
	m_CmdListenerReactor->open(1);

	// Initialize the DSD event handler
	m_ClientHandler = new FMS_CPF_ClientCmdHandler();

	fms_cpf_cmdlistenerTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_CmdListener");

}

/*============================================================================
	ROUTINE: stopListining
 ============================================================================ */
int FMS_CPF_CmdListener::stopListining()
{
	TRACE(fms_cpf_cmdlistenerTrace, "%s", "Entering stopListining()");
	int result;

	// set stop request
	m_StopEvent = true;

	// remove the registered handle
	removeDSDHandles();

	// stop the reactor event dispatching loop
	result = m_CmdListenerReactor->end_reactor_event_loop();

	// Wait for SVC termination
	wait();

	TRACE(fms_cpf_cmdlistenerTrace, "%s", "Leaving stopListining()");
	return result;
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
int FMS_CPF_CmdListener::open(void *args)
{
	TRACE(fms_cpf_cmdlistenerTrace, "%s", "Entering open()");
	// To avoid warning about unused parameter
	UNUSED(args);
	int result;

	if( m_ClientHandler->initDSDServer() && registerDSDHandles())
	{
		// set the flag to skip DSD init retry;
		m_DSDInitiate = true;
	}
	else
	{
		TRACE(fms_cpf_cmdlistenerTrace, "%s", "open(), error on init DSD server retry after 5 sec");
		m_ClientHandler->stopDSDServer();
	}

	// start event loop by svc thread
	result = activate();

	// Check if the svc thread is started
	if(0 != result)
	{
		CPF_Log.Write("FMS_CPF_CmdListener::open,  error on start svc thread", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_cmdlistenerTrace, "%s", "open(), error on start svc thread");
	}

	TRACE(fms_cpf_cmdlistenerTrace, "%s", "Leaving open()");
	return result;
}

/*============================================================================
	ROUTINE: registerDSDHandles
 ============================================================================ */
bool FMS_CPF_CmdListener::registerDSDHandles()
{
	TRACE(fms_cpf_cmdlistenerTrace, "%s", "Entering registerDSDHandles()");
	bool result = true;
	int idx = 0;
	ACE_HANDLE dsdHnl;

	fdList.clear();

	// get the first DSD handle
	dsdHnl =  m_ClientHandler->get_handle(idx);

	// Check if a valid handle
	while( ACE_INVALID_HANDLE != dsdHnl )
	{
		TRACE(fms_cpf_cmdlistenerTrace, "registerDSDHandles(), handle=%i registered to reactor", idx);
		// Register to reactor the DSD handle
		m_CmdListenerReactor->register_handler(dsdHnl,  m_ClientHandler, ACE_Event_Handler::ACCEPT_MASK );
		// Store the handle for remove it from reactor
		fdList.push_back(dsdHnl);
		// get the next DSD handle
		dsdHnl =  m_ClientHandler->get_handle(++idx);
	}

	// Checks if at least one handle is valid
    if(0 == idx)
    {
    	TRACE(fms_cpf_cmdlistenerTrace, "%s", "registerDSDHandles(), no valid DSD handles");
    	result = false;
    }

	TRACE(fms_cpf_cmdlistenerTrace, "%s", "Leaving registerDSDHandles()");
	return result;
}

/*============================================================================
	ROUTINE: removeDSDHandles
 ============================================================================ */
void FMS_CPF_CmdListener::removeDSDHandles()
{
	TRACE(fms_cpf_cmdlistenerTrace, "%s", "Entering removeDSDHandles()");

	std::vector<ACE_HANDLE>::iterator it;

	// Remove all registered Handle from reactor
	for( it = fdList.begin(); it != fdList.end(); ++it )
	{
		m_CmdListenerReactor->remove_handler( (*it), ACE_Event_Handler::ACCEPT_MASK );
	}

	fdList.clear();

	TRACE(fms_cpf_cmdlistenerTrace, "%s", "Leaving removeDSDHandles()");
}

/*============================================================================
	ROUTINE: svc
 ============================================================================ */
int FMS_CPF_CmdListener::svc()
{
	TRACE(fms_cpf_cmdlistenerTrace, "%s", "Entering svc()");
	int result = 0;

	// Try to publish DSD server if it has not already done
	// both conditions false to continue
	while(!m_StopEvent && !m_DSDInitiate)
	{
		// Retry
		if( m_ClientHandler->initDSDServer() && registerDSDHandles() )
		{
			// set the flag to exit by the while loop;
			m_DSDInitiate = true;
		}
		else
		{
			TRACE(fms_cpf_cmdlistenerTrace, "%s", "svc(), error on init DSD server retry after 1 sec");
			m_ClientHandler->stopDSDServer();
			// wait 1 sec. between each retry
			ACE_OS::sleep(1);
		}
	}

	// if the stop signal is not yet arrived, run the reactor loop
	if(!m_StopEvent)
	{
		TRACE(fms_cpf_cmdlistenerTrace, "%s", "svc(), starting reactor event loop");
		// start listening on DSD handle
		m_CmdListenerReactor->run_reactor_event_loop();
	}

	TRACE(fms_cpf_cmdlistenerTrace, "%s", "Leaving svc()");
	return result;
}

/*============================================================================
	ROUTINE: ~FMS_CPF_CmdListener
 ============================================================================ */
FMS_CPF_CmdListener::~FMS_CPF_CmdListener()
{
	// delete reactor object
	if(NULL != m_CmdListenerReactor)
		delete m_CmdListenerReactor;

	// delete client handler object
	if(NULL != m_ClientHandler)
		delete m_ClientHandler;

	// delete trace object
	if(NULL != fms_cpf_cmdlistenerTrace)
	{
		delete fms_cpf_cmdlistenerTrace;
		fms_cpf_cmdlistenerTrace = NULL;
	}

}
