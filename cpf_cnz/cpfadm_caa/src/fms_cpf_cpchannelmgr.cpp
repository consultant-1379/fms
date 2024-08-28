/*
 * * @file fms_cpf_cpchannelmgr.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_CpChannelMgr.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_cpchannelmgr.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-10
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
 *	| 1.0.0  | 2011-11-10 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_cpchannelmgr.h"
#include "fms_cpf_cpopenchannels.h"
#include "fms_cpf_cpdopenfilesmgr.h"
#include "fms_cpf_oc_buffermgr.h"
#include "fms_cpf_common.h"
#include "fms_cpf_parameterhandler.h"

#include "ACS_DSD_Session.h"
#include "ACS_DSD_Server.h"
#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"


#include "ace/TP_Reactor.h"
#include "ace/Reactor.h"
#include <netinet/tcp.h> // FOR TCP_NODELAY

extern ACS_TRA_Logging CPF_Log;

namespace CPF_DSD
{
	const std::string serviceDomain("CPFP");
	const std::string serviceName("FMS_CPF");
}
/*============================================================================
	ROUTINE: FMS_CPF_ClientCmdHandler
 ============================================================================ */
FMS_CPF_CpChannelMgr::FMS_CPF_CpChannelMgr():
m_IsMultiCP(false),
m_serverPublished(false),
m_handlesRegistered(false),
m_timerId(-1),
m_DsdServer(NULL)
{
	// Instance a Reactor to handle DSD events
	ACE_TP_Reactor* tp_reactor_impl = new ACE_TP_Reactor();

	// the reactor will delete the implementation on destruction
	m_DsdServerReactor = new ACE_Reactor(tp_reactor_impl, true);

	// Initialize the ACE_Reactor
	m_DsdServerReactor->open(1);

	fms_cpf_ChMgrTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_CpChannelMgr");
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
int FMS_CPF_CpChannelMgr::open(void *args)
{
	TRACE(fms_cpf_ChMgrTrace, "%s", "Entering open()");
	// To avoid warning about unused parameter
	UNUSED(args);
	int result;

	// start event loop by svc thread
	result = activate();

	// Check if the svc thread is started
	if(SUCCESS != result)
	{
		CPF_Log.Write("FMS_CPF_CpChannelMgr::open,  error on start svc thread", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_ChMgrTrace, "%s", "open(), error on start svc thread");
	}
	else
	{
		// set the system type to the channel list handler
		CpChannelsList::instance()->setSystemType(m_IsMultiCP);

		// start the channel list handler thread
		CpChannelsList::instance()->open();

		// Publishing CPF server to DSD
		if( !initCPFServer() )
		{
			//Some error occurs
			// Re-try after 1 sec
			ACE_Time_Value interval(1);

			// Schedule a timer event that will expire after an <delay> amount of time
			// It is scheduled just one time after 1 sec
			m_timerId = m_DsdServerReactor->schedule_timer(this, 0, interval);
		}
	}

	TRACE(fms_cpf_ChMgrTrace, "%s", "Leaving open()");
	return result;
}

/*============================================================================
	ROUTINE: handle_timeout
 ============================================================================ */
int FMS_CPF_CpChannelMgr::handle_input(ACE_HANDLE fd)
{
	TRACE(fms_cpf_ChMgrTrace, "%s", "Entering in handle_input()");

	//To avoid unused warning
	UNUSED(fd);
	int dsdResult;

	ACS_DSD_Session* dsdSession = NULL;
	dsdSession = new (std::nothrow) ACS_DSD_Session();

	if(NULL == dsdSession)
	{
		// the dsd session not created
		TRACE(fms_cpf_ChMgrTrace, "%s", "handle_input(), Failed creation of a dsd session object");
		CPF_Log.Write("FMS_CPF_CpChannelMgr::handle_input, Failed creation of a dsd session object", LOG_LEVEL_ERROR);
		// In any case listen always on this dsd handle
		return SUCCESS;
	}

	// Initialize DSD resources
	dsdResult = m_DsdServer->accept( *dsdSession );

	//TR_HU91839 start
	if (acs_dsd::ERR_NO_ERRORS == dsdResult )
	{
		int flag = 1;
		// Disabling Nagle's Algorithm
		int nagle_result = dsdSession->set_option(IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

		if (nagle_result < 0)
		{
			CPF_Log.Write("FMS_CPF_CpChannelMgr::Failed to set TCP_NODELAY", LOG_LEVEL_ERROR);
		}
	}//TR_HU91839 END


	// Check the operation result
	if(acs_dsd::ERR_NO_ERRORS == dsdResult)
	{
		// Connection established
		bool addResult = CpChannelsList::instance()->addCpChannel(dsdSession);

		//In any case the dsd session, now is handled by CpChannelsList
		dsdSession = NULL;
		// Check the operation result
		if(addResult)
		{
			TRACE(fms_cpf_ChMgrTrace, "%s", "handle_input(), new Cp Channel added");
		}
		else
		{
			// the dsd session has been deleted by CpChannelsList
			TRACE(fms_cpf_ChMgrTrace, "%s", "handle_input(), Failed to add new Cp Channel");
			CPF_Log.Write("FMS_CPF_CpChannelMgr::handle_input, Failed to add new Cp Channel", LOG_LEVEL_ERROR);
		}
	}
	else
	{
		TRACE(fms_cpf_ChMgrTrace, "handle_input(), Failed in call to DSD accept(). Error : %d, <%s>", dsdResult, m_DsdServer->last_error_text());
		char errorMsg[1024]={'\0'};
		ACE_OS::snprintf(errorMsg, 1023, "FMS_CPF_CpChannelMgr::handle_input, error on DSD accept(),Error : %d, <%s>", dsdResult, m_DsdServer->last_error_text());
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		ACE_OS::sleep(1);

		//Release the dsd session object
		delete dsdSession;
	}

	TRACE(fms_cpf_ChMgrTrace, "%s", "Leaving handle_input()");
	// In any case listen always on this dsd handle
	return SUCCESS;
}

/*============================================================================
	ROUTINE: stopChannelMgr
 ============================================================================ */
void FMS_CPF_CpChannelMgr::stopChannelMgr()
{
	TRACE(fms_cpf_ChMgrTrace, "%s", "Entering in stopChannelMgr()");
	// Stop event loop of reactor, so svc thread will be terminated
	m_DsdServerReactor->end_reactor_event_loop();

	TRACE(fms_cpf_ChMgrTrace, "%s", "stopChannelMgr(), DSD reactor stopped");
	// wait on svc termination
	this->wait();

	TRACE(fms_cpf_ChMgrTrace, "%s", "stopChannelMgr(), Shutdown all CP channels");
	// Shutdown all CP channels
	CpChannelsList::instance()->channelsShutDown();

	TRACE(fms_cpf_ChMgrTrace, "%s", "stopChannelMgr(), Shutdown all OpenFiles objects");
	// Shutdown all OpenFiles objects
	CPDOpenFilesMgr::instance()->shutDown();

	TRACE(fms_cpf_ChMgrTrace, "%s", "stopChannelMgr(), Shutdown EX/SB OC buffers");
	// Shutdown EX/SB OC buffers
	OcBufferMgr::instance()->shutdown();

	// remove DSD object ant its resource
	removeCPFServer();
	TRACE(fms_cpf_ChMgrTrace, "%s", "Leaving stopChannelMgr()");
}

/*============================================================================
	ROUTINE: svc
 ============================================================================ */
int FMS_CPF_CpChannelMgr::svc()
{
	TRACE(fms_cpf_ChMgrTrace, "%s", "Entering svc()");
	int result = SUCCESS;
	TRACE(fms_cpf_ChMgrTrace, "%s", "svc(), starting reactor event loop");

	// start dispatching of DSD / timer event
	m_DsdServerReactor->run_reactor_event_loop();

	TRACE(fms_cpf_ChMgrTrace, "%s", "Leaving svc()");
	return result;
}

/*============================================================================
	ROUTINE: removeCPFServer
 ============================================================================ */
void FMS_CPF_CpChannelMgr::removeCPFServer()
{
	TRACE(fms_cpf_ChMgrTrace, "%s", "Entering in removeCPFServer()");

	// Check if handles have been registered to reactor
	if( m_handlesRegistered )
	{
		// unregister dsd handles from reactor
		m_DsdServerReactor->remove_handler(m_dsdHandleSet, ACE_Event_Handler::ALL_EVENTS_MASK | ACE_Event_Handler::DONT_CALL);
		m_handlesRegistered = false;
	}

	// check if CPF Server has been published
	if( m_serverPublished )
	{
		// unregister CPF server from DSD
		m_DsdServer->unregister();
		m_DsdServer->close();
		// delete DSD server object
		delete m_DsdServer;
		m_DsdServer = NULL;
		m_serverPublished = false;
	}

	TRACE(fms_cpf_ChMgrTrace, "%s", "Leaving removeCPFServer()");
}

/*============================================================================
	ROUTINE: handle_timeout
 ============================================================================ */
int FMS_CPF_CpChannelMgr::handle_timeout(const ACE_Time_Value&, const void* arg)
{
	TRACE(fms_cpf_ChMgrTrace, "%s", "Entering in handle_timeout()");
	//To avoid warning
	UNUSED(arg);

	// Publishing CPF server to DSD
	if( !initCPFServer() )
	{
		//Some error occurs again re-schedule the timer
		// Re-try after 1 sec
		ACE_Time_Value interval(1);

		// Schedule a timer event that will expire after an <delay> amount of time.
		// It is scheduled just one time
		m_timerId = m_DsdServerReactor->schedule_timer(this, 0, interval);
		TRACE(fms_cpf_ChMgrTrace, "%s", "handle_timeout(), Timer rescheduled again between 1 sec");
	}
	else
	{
		TRACE(fms_cpf_ChMgrTrace, "%s", "handle_timeout(), CPF server up and running");
		//now there is not timer scheduled
		m_timerId = -1;
	}
	TRACE(fms_cpf_ChMgrTrace, "%s", "Leaving in handle_timeout()");
	return SUCCESS;
}

/*============================================================================
	ROUTINE: initCPFServer
 ============================================================================ */
bool FMS_CPF_CpChannelMgr::initCPFServer()
{
	TRACE(fms_cpf_ChMgrTrace, "%s", "Entering in initCPFServer()");
	bool result = false;

	// Publish CPF server to DSD
	if( publishCPFServer() )
	{
		// CPF server published, register its handles
		if( registerDSDHandles() )
		{
			result = true;
		}
		else
		{
			// Some problem happens remove all
			removeCPFServer();
		}
	}
	TRACE(fms_cpf_ChMgrTrace, "%s", "Leaving initCPFServer()");
	return result;
}

/*============================================================================
	ROUTINE: publishCPFServer
 ============================================================================ */
bool FMS_CPF_CpChannelMgr::publishCPFServer()
{
	TRACE(fms_cpf_ChMgrTrace, "%s", "Entering in publishCPFServer()");

	// To avoid multiple accidental calls
	if(m_serverPublished)
	{
		TRACE(fms_cpf_ChMgrTrace, "%s", "CPF Server already published on DSD");
		CPF_Log.Write("FMS_CPF_CpChannelMgr::publishCPFServer, CPF Server already published on DSD", LOG_LEVEL_WARN);
		return m_serverPublished;
	}

	int dsdResult;

	// create DSD server object
	m_DsdServer = new (std::nothrow) ACS_DSD_Server(acs_dsd::SERVICE_MODE_INET_SOCKET);     // DSD server instance

	if(NULL != m_DsdServer)
	{
		// Initialize DSD resources
		dsdResult = m_DsdServer->open();

		// Check the operation result
		if(acs_dsd::ERR_NO_ERRORS == dsdResult)
		{
			// Publish the CPF service
			dsdResult = m_DsdServer->publish(CPF_DSD::serviceName.c_str(), CPF_DSD::serviceDomain.c_str());

			// Check the operation result
			if( acs_dsd::ERR_NO_ERRORS == dsdResult)
			{
				m_serverPublished = true;
				TRACE(fms_cpf_ChMgrTrace, "CPF Server<%s,%s> published on DSD", CPF_DSD::serviceName.c_str(), CPF_DSD::serviceDomain.c_str());
				CPF_Log.Write("FMS_CPF_CpChannelMgr::publishCPFServer, CPF Server published on DSD", LOG_LEVEL_INFO);
			}
			else
			{
			  TRACE(fms_cpf_ChMgrTrace, "Failed to publish CPF Server, Error code: %d , <%s>", dsdResult, m_DsdServer->last_error_text());
			  CPF_Log.Write("FMS_CPF_CpChannelMgr::publishCPFServer, Failed to publish DSD Server", LOG_LEVEL_ERROR);
			  // Close and delete DSD server object
			  m_DsdServer->close();
			  delete m_DsdServer;
			  m_DsdServer = NULL;
			}
		}
		else
		{
		  TRACE(fms_cpf_ChMgrTrace, "Failed to open DSD Server, Error code: %d , <%s>", dsdResult, m_DsdServer->last_error_text());
		  CPF_Log.Write("FMS_CPF_CpChannelMgr::publishCPFServer, ERROR: Failed to open DSD Server", LOG_LEVEL_ERROR);
		  // delete DSD server object
		  delete m_DsdServer;
		  m_DsdServer = NULL;
		}
	}
	else
	{
		TRACE(fms_cpf_ChMgrTrace, "%s", "ERROR: Failed to create DSD Server");
		CPF_Log.Write("FMS_CPF_CpChannelMgr::publishCPFServer, ERROR: Failed to create DSD Server", LOG_LEVEL_ERROR);
	}

	TRACE(fms_cpf_ChMgrTrace, "%s", "Leaving in publishCPFServer()");
	return m_serverPublished;
}

/*============================================================================
	ROUTINE: registerDSDHandles
 ============================================================================ */
bool FMS_CPF_CpChannelMgr::registerDSDHandles()
{
	TRACE(fms_cpf_ChMgrTrace, "%s", "Entering registerDSDHandles()");

	// To avoid multiple accidental calls
	if(m_handlesRegistered || !m_serverPublished)
	{
		TRACE(fms_cpf_ChMgrTrace, "%s", "DSD handles already registered or CPF Server not published");
		CPF_Log.Write("FMS_CPF_CpChannelMgr::initDSDServer, DSD handles already registered or CPF Server not published", LOG_LEVEL_WARN);
		return m_handlesRegistered;
	}

	int dsdHandleCount = 0;
	// To get the number of handle
	int dsdResult = m_DsdServer->get_handles(NULL, dsdHandleCount);

	if(acs_dsd::ERR_NOT_ENOUGH_SPACE != dsdResult)
	{
		// Some error happens
		TRACE(fms_cpf_ChMgrTrace, "Failed to get number of DSD handles. Error: %d, <%s>", dsdResult, m_DsdServer->last_error_text() );
		CPF_Log.Write("FMS_CPF_CpChannelMgr::registerDSDHandles, Failed to get number of handles", LOG_LEVEL_ERROR);
		return m_handlesRegistered;
	}

	// Now dsdHandleCount has the correct number of handles to retrieve
	acs_dsd::HANDLE dsdHandles[dsdHandleCount];

	dsdResult = m_DsdServer->get_handles(dsdHandles, dsdHandleCount);

	if( acs_dsd::ERR_NO_ERRORS == dsdResult )
	{
		ACE_Handle_Set tmpDSDHandleSet;
		for(int idx=0; idx < dsdHandleCount; ++idx)
		{
			tmpDSDHandleSet.set_bit( dsdHandles[idx] );
		}

		int result = m_DsdServerReactor->register_handler(tmpDSDHandleSet, this, ACE_Event_Handler::ACCEPT_MASK);

		if( result < 0 )
		{
			TRACE(fms_cpf_ChMgrTrace, "%s", "Failed to register DSD handles into reactor");
			CPF_Log.Write("FMS_CPF_CpChannelMgr::registerDSDHandles, Failed to register DSD handles into reactor", LOG_LEVEL_ERROR);
		}
		else
		{
			TRACE(fms_cpf_ChMgrTrace, "%s", "DSD handles registered to reactor");
			// Copy the DSD handle for remove handle on closure
			for(int idx=0; idx < dsdHandleCount; ++idx)
			{
				m_dsdHandleSet.set_bit( dsdHandles[idx] );
			}

			m_handlesRegistered = true;
		}
	}
	else
	{
		TRACE(fms_cpf_ChMgrTrace, "Failed to get DSD handles. Error: %d, <%s>", dsdResult, m_DsdServer->last_error_text() );
		CPF_Log.Write("FMS_CPF_CpChannelMgr::registerDSDHandles, Failed to get DSD handles");
	}

	TRACE(fms_cpf_ChMgrTrace, "%s", "Entering registerDSDHandles()");
	return m_handlesRegistered;
}

/*============================================================================
	ROUTINE: ~FMS_CPF_CpChannelMgr
 ============================================================================ */
FMS_CPF_CpChannelMgr::~FMS_CPF_CpChannelMgr()
{
	// delete reactor object
	if( NULL != m_DsdServerReactor )
		delete m_DsdServerReactor;

	if( NULL != m_DsdServer )
		delete m_DsdServer;

	// delete trace object
	if(NULL != fms_cpf_ChMgrTrace)
		delete fms_cpf_ChMgrTrace;

}
