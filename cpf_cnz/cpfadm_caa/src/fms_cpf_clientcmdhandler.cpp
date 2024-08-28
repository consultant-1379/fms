/*
 * * @file fms_cpf_clientcmdhandler.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_ClientCmdHandler.
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

#include "fms_cpf_clientcmdhandler.h"
#include "fms_cpf_clientcmd_request.h"
#include "fms_cpf_common.h"
#include "fms_cpf_cmdhandler.h"
#include "fms_cpf_apdcommonfile.h"
#include "fms_cpf_parameterhandler.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include <sys/eventfd.h>

extern ACS_TRA_Logging CPF_Log;

namespace DSDConfig{
	const int DSDSuccess = 0;
}

/*============================================================================
	ROUTINE: FMS_CPF_ClientCmdHandler
 ============================================================================ */
FMS_CPF_ClientCmdHandler::FMS_CPF_ClientCmdHandler():
m_serverOnLine(false)
{
	// create the file descriptor to signal stop
	m_StopClientEvent = eventfd(0, 0);
	fms_cpf_clientcmdTrace = new (std::nothrow)ACS_TRA_trace("FMS_CPF_ClientCmdHandler");
}

/*============================================================================
	ROUTINE: handle_input
 ============================================================================ */
int FMS_CPF_ClientCmdHandler::handle_input(ACE_HANDLE fd)
{
	TRACE(fms_cpf_clientcmdTrace, "%s", "Entering in handle_input()");
	int result = 0;
    // to avoid unused warning
	UNUSED(fd);
	CPF_ClientCmd_Request* cmdClientReq = new CPF_ClientCmd_Request(m_StopClientEvent);

	// accept client connection
	if( m_CmdServerAcceptor.accept(cmdClientReq->getStream()) == 0 )
	{
		TRACE(fms_cpf_clientcmdTrace, "%s", "handle_input(), connection accepted");

		//check protocol data before continue
		if( cmdClientReq->checkConnectionData() )
		{
			TRACE(fms_cpf_clientcmdTrace, "%s", "handle_input(), enqueue client request");
			cmdClientReq->open();
		}
		else
		{
			TRACE(fms_cpf_clientcmdTrace, "%s", "handle_input(), client request discarded because not valid");
			delete cmdClientReq;
		}
	}
	else
	{
		TRACE(fms_cpf_clientcmdTrace, "%s", "handle_input(), failed to accept connection");
		delete cmdClientReq;
		// todo re-publish the server?
	}

	TRACE(fms_cpf_clientcmdTrace, "%s", "Leaving handle_input()");
	return result;
}

/*============================================================================
	ROUTINE: stopCmdClient
 ============================================================================ */
bool FMS_CPF_ClientCmdHandler::stopCmdClient()
{
	TRACE(fms_cpf_clientcmdTrace, "%s", "Entering in stopCmdClient");
	bool result = true;

	eventfd_t stopEvent = 1;
	if( eventfd_write(m_StopClientEvent, stopEvent) != 0)
	{
		CPF_Log.Write("FMS_CPF_ClientCmdHandler::stopCmdClient, error on stop cmd Clients", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_clientcmdTrace, "%s", "stopCmdClient error on stop cmd Clients");
		result = false;
	}

	TRACE(fms_cpf_clientcmdTrace, "%s", "Leaving stopCmdClient");
	return result;
}

/*============================================================================
	ROUTINE: handle_close
 ============================================================================ */
int FMS_CPF_ClientCmdHandler::handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask)
{
	TRACE(fms_cpf_clientcmdTrace, "%s", "Entering in handle_close()");
	// to avoid unsed warnig
	UNUSED(handle);
	UNUSED(close_mask);
	// Signal to all Cmd Client to terminate
	stopCmdClient();

	// DSD server closure
	stopDSDServer();

	TRACE(fms_cpf_clientcmdTrace, "%s", "Leaving handle_close()");
	return 0;
}

/*============================================================================
	ROUTINE: stopDSDServer
 ============================================================================ */
void FMS_CPF_ClientCmdHandler::stopDSDServer()
{
	TRACE(fms_cpf_clientcmdTrace, "%s", "Entering in stopDSDServer()");
	if(m_serverOnLine)
	{
		// Close the DSD server
		int result = m_CmdServerAcceptor.close();

		// check the closure result
		if(DSDConfig::DSDSuccess == result)
		{
			TRACE(fms_cpf_clientcmdTrace, "%s", "stopDSDServer(), DSD server closed");
			// set online flag
			m_serverOnLine = false;
		}
		else
		{
			TRACE(fms_cpf_clientcmdTrace, "%s", "stopDSDServer(), error on DSD server closure");
		}
	}
	else
	{
		TRACE(fms_cpf_clientcmdTrace, "%s", "stopDSDServer(), DSD server already closed");
	}
	TRACE(fms_cpf_clientcmdTrace, "%s", "Leaving stopDSDServer()");
}

/*============================================================================
	ROUTINE: initDSD
 ============================================================================ */
bool FMS_CPF_ClientCmdHandler::initDSDServer()
{
	TRACE(fms_cpf_clientcmdTrace, "%s", "Entering in initDSDServer()");

	// to avoid some erroneous publishing
	if(!m_serverOnLine)
	{
		TRACE(fms_cpf_clientcmdTrace, "%s", "initDSDServer(), publish by DSD");
		// Publish and register CPF DSD command server
		int result = m_CmdServerAcceptor.open(CPF_API_Protocol::CPF_ApplicationName, CPF_API_Protocol::FMS_Domain);

		if(DSDConfig::DSDSuccess == result)
		{
			TRACE(fms_cpf_clientcmdTrace, "initDSDServer(), DSD server published, applName=%s, domain=%s",
					CPF_API_Protocol::CPF_ApplicationName.c_str(), CPF_API_Protocol::FMS_Domain.c_str());

			// set online flag
			m_serverOnLine = true;
		}
		else
		{
			// DSD error
			char msgLog[1024]={0};
			ACE_OS::snprintf(msgLog, 1023, "initDSDServer(), DSD server error:<%i> on publishing, applName:<%s>, domain:<%s>",
								result, CPF_API_Protocol::CPF_ApplicationName.c_str(), CPF_API_Protocol::FMS_Domain.c_str());
			CPF_Log.Write(msgLog, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_clientcmdTrace, "%s", msgLog);
		}

	}

	TRACE(fms_cpf_clientcmdTrace, "%s", "Leaving initDSDServer()");
	return m_serverOnLine;
}

/*============================================================================
	ROUTINE: ~FMS_CPF_ClientCmdHandler
 ============================================================================ */
FMS_CPF_ClientCmdHandler::~FMS_CPF_ClientCmdHandler()
{
	stopDSDServer();

	//Close stop signal file descriptor
	ACE_OS::close(m_StopClientEvent);

	// delete trace object
	if(NULL != fms_cpf_clientcmdTrace)
	{
		delete fms_cpf_clientcmdTrace;
		fms_cpf_clientcmdTrace = NULL;
	}


}
