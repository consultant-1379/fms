/*
 * * @file fms_cpf_server.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_Server.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_server.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-08
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
 *	| 1.0.0  | 2011-06-08 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_server.h"
#include "fms_cpf_immhandler.h"
#include "fms_cpf_cmdlistener.h"
#include "fms_cpf_cpchannelmgr.h"
#include "fms_cpf_jtpconnectionsmgr.h"
#include "fms_cpf_common.h"
#include "fms_cpf_configreader.h"
#include "fms_cpf_cmdhandler.h"
#include "fms_cpf_cmdhandlerexit.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_eventalarmhndl.h"
#include "fms_cpf_blocktransfermgr.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include <ACS_CS_API.h>
#include <sys/eventfd.h>

extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	ROUTINE: FMS_CPF_Server
 ============================================================================ */
FMS_CPF_Server::FMS_CPF_Server()
{
	// Create the CPF Command handler object
	m_CmdHandle = new FMS_CPF_CmdHandler();

	// Create the IMM handler object
	IMM_Handler = new FMS_CPF_ImmHandler(m_CmdHandle);

	m_CpComHandler = new FMS_CPF_CpChannelMgr();

	// Create the system configuration reader object
	m_systemConfig = new FMS_CPF_ConfigReader();

	m_APICmdHandler = new FMS_CPF_CmdListener();

	m_JtpConHandler = new FMS_CPF_JTPConnectionsMgr();

	// create the file descriptor to signal stop
	m_StopEvent = eventfd(0,0);

	m_SysDiscoveryOn = false;

	fms_cpf_serverTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_Server");
}

/*============================================================================
	ROUTINE: ~FMS_CPF_Server
 ============================================================================ */
FMS_CPF_Server::~FMS_CPF_Server()
{
	if(NULL != IMM_Handler)
		delete IMM_Handler;

	if(NULL != m_CpComHandler)
		delete m_CpComHandler;

	if(NULL != m_APICmdHandler)
		delete m_APICmdHandler;

	if(NULL != m_CmdHandle)
		delete m_CmdHandle;

	if(NULL != m_JtpConHandler)
		delete m_JtpConHandler;

	if(NULL != m_systemConfig)
		delete m_systemConfig;

	if(NULL != fms_cpf_serverTrace)
		delete fms_cpf_serverTrace;

	ACE_OS::close(m_StopEvent);
}

/*============================================================================
	ROUTINE: startWorkerThreads
 ============================================================================ */
bool FMS_CPF_Server::startWorkerThreads()
{
	TRACE(fms_cpf_serverTrace, "%s", "Entering startWorkerThreads()");
	bool result = true;

	// discovery the system configuration (SCP/MCP)
	result = systemDiscovery();

	// False only in case of shutdown request
	if( result )
	{
		ParameterHndl::instance()->init(m_systemConfig);

		bool systemType = m_systemConfig->IsBladeCluster();

		DirectoryStructureMgr::instance()->initializeCpFileSystem(m_systemConfig);

		BlockTransferMgr::instance()->setSystemType(systemType);

		EventReport::instance()->setSystemType(systemType);

		if(SUCCESS == m_CmdHandle->open())
		{
			TRACE(fms_cpf_serverTrace, "%s", "startWorkerThreads(), CMD Handler opened");

			if(SUCCESS == m_APICmdHandler->open() )
			{
				TRACE(fms_cpf_serverTrace, "%s", "startWorkerThreads(), API Cmd Handler opened");

				m_CpComHandler->setSystemType(systemType);

				if(SUCCESS == m_CpComHandler->open() )
				{
					TRACE(fms_cpf_serverTrace, "%s", "startWorkerThreads(), CP Channel Manager opened");
					IMM_Handler->setSystemParameters(m_systemConfig);

					// Start the IMM handler thread
					if( SUCCESS == IMM_Handler->open() )
					{
						TRACE(fms_cpf_serverTrace, "%s", "startWorkerThreads, IMM Handler started");

						m_JtpConHandler->setSystemType(systemType);

						if( SUCCESS == m_JtpConHandler->open() )
						{
							TRACE(fms_cpf_serverTrace, "%s", "startWorkerThreads, JTP connections Handler started");
						}
						else
						{
							CPF_Log.Write("FMS_CPF_Server::startWorkerThreads, error on start JTP connections Handler", LOG_LEVEL_ERROR);
							TRACE(fms_cpf_serverTrace, "%s", "startWorkerThreads, error on start JTP connections Handler");
							// Stop Cp channels manager
							m_CpComHandler->stopChannelMgr();
							// Stop API listener
							m_APICmdHandler->stopListining();
							// Stop Internal Cmd handler
							stopCmdHandler();
							// Stop IMM handler
							stopImmHandler();
                                                         //HY19636
                                                        EventReport::instance()->shutdown();
							result = false;
						}
					}
					else
					{
						CPF_Log.Write("FMS_CPF_Server::startWorkerThreads, error on start IMM Handler", LOG_LEVEL_ERROR);
						TRACE(fms_cpf_serverTrace, "%s", "startWorkerThreads, error on start IMM Handler");
						// Stop Cp channels manager
						m_CpComHandler->stopChannelMgr();
						// Stop API listener
						m_APICmdHandler->stopListining();
						// Stop Internal Cmd handler
						stopCmdHandler();
                                                //HY19636
                                                EventReport::instance()->shutdown();
						result = false;
					}
				}
				else
				{
					CPF_Log.Write("FMS_CPF_Server::startWorkerThreads, error on start Cp Channel Manager", LOG_LEVEL_ERROR);
					TRACE(fms_cpf_serverTrace, "%s", "startWorkerThreads(), error on start Cp Channel Manager");

					// Stop Internal Cmd handler
					stopCmdHandler();

					// Stop API listener
					m_APICmdHandler->stopListining();
					result = false;
				}
			}
			else
			{
				CPF_Log.Write("FMS_CPF_Server::startWorkerThreads, error on start API Cmd Handler", LOG_LEVEL_ERROR);
				TRACE(fms_cpf_serverTrace, "%s", "startWorkerThreads(), error on start API Cmd Handler");
				// Stop Internal Cmd handler
				stopCmdHandler();
				result = false;
			}
		}
		else
		{
			CPF_Log.Write("FMS_CPF_Server::startWorkerThreads(), error on start Cmd handler", LOG_LEVEL_ERROR);
			TRACE(fms_cpf_serverTrace, "%s", "startWorkerThreads(),error on start Cmd handler");
			result = false;
		}
	}

	TRACE(fms_cpf_serverTrace, "%s", "Leaving startWorkerThreads()");
	return result;
}

/*============================================================================
	ROUTINE: systemDiscovery
 ============================================================================ */
bool FMS_CPF_Server::systemDiscovery()
{
	TRACE(fms_cpf_serverTrace, "%s", "Entering systemDiscovery()");
	bool result = false;
	m_SysDiscoveryOn = true;

	do{
		try
		{
			m_systemConfig->init();

			m_SysDiscoveryOn = false;
			result = true;
			TRACE(fms_cpf_serverTrace, "%s", "systemDiscovery, system configuration discovered");
		}
		catch(FMS_CPF_Exception& ex)
		{
			TRACE(fms_cpf_serverTrace, "%s", "systemDiscovery, system configuration discovery re-try after 5s");
			if(m_SysDiscoveryOn && (0 != waitBeforeRetry()) )
			{
				m_SysDiscoveryOn = false;
				TRACE(fms_cpf_serverTrace, "%s", "systemDiscovery, system configuration discovery stop re-try");
			}
		}
	}while(m_SysDiscoveryOn);

	TRACE(fms_cpf_serverTrace, "%s", "Leaving systemDiscovery");
	return result;
}

/*============================================================================
	ROUTINE: stopWorkerThreads
 ============================================================================ */
bool FMS_CPF_Server::stopWorkerThreads()
{
	TRACE(fms_cpf_serverTrace, "%s", "Entering stopWorkerThreads");
	bool result = true;
	IMM_Handler->setStopSignal(true); //HX99576

	// Check if query to CS is ongoing
	if(m_SysDiscoveryOn)
	{
		m_SysDiscoveryOn = false;
		ssize_t numByte;
		ACE_UINT64 stopEvent=1;

		// Signal to server to stop
		numByte = ::write(m_StopEvent, &stopEvent, sizeof(ACE_UINT64));

		if(sizeof(ACE_UINT64) != numByte)
		{
			CPF_Log.Write("FMS_CPF_Server::stopWorkerThreads, error on signal server stop", LOG_LEVEL_ERROR);
			TRACE(fms_cpf_serverTrace, "stopWorkerThreads(), error on signal server stop, numByte=%i, error=%i", numByte, errno);
		}
	}

	m_APICmdHandler->stopListining();

	stopCmdHandler();

	// Stop Cp channels manager
	m_CpComHandler->stopChannelMgr();

	// Stop IMM Handler
	result = stopImmHandler();

	// Stop block transfer manager
	BlockTransferMgr::instance()->shutDown();

	// Stop Jtp connection Handler
	m_JtpConHandler->stopJTPConnectionsMgr();

	EventReport::instance()->shutdown();

	TRACE(fms_cpf_serverTrace, "%s", "Leaving stopWorkerThreads");
	return result;
}

/*============================================================================
	ROUTINE: stopCmdHandler
 ============================================================================ */
bool FMS_CPF_Server::stopCmdHandler()
{
	TRACE(fms_cpf_serverTrace, "%s", "Entering stopCmdHandler()");
	bool result;
	ACE_Future<int> waitOnCmdHandlerExit;

	// Create an exit method request
	CPF_CmdHandlerExit_Request* cmdHandlerExit = new CPF_CmdHandlerExit_Request(waitOnCmdHandlerExit);

	// Enqueue to the cmd handler thread
	if( -1 != m_CmdHandle->enqueue(cmdHandlerExit))
	{
		int retCode;
       //	wait on answer
		waitOnCmdHandlerExit.get(retCode);
		result = true;
		TRACE(fms_cpf_serverTrace, "%s", "stopCmdHandler(), cmdHandler execute exit request");
	}
	else
	{
		CPF_Log.Write("FMS_CPF_Server::stopCmdHandler(), to enqueue the exit request to the cmdHandler", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_serverTrace, "%s", "stopCmdHandler(), error to enqueue the exit request to the cmdHandler");
		result = false;
	}
	TRACE(fms_cpf_serverTrace, "%s", "Leaving stopCmdHandler()");
	return result;
}
/*============================================================================
	ROUTINE: stopImmHandler
 ============================================================================ */
bool  FMS_CPF_Server::stopImmHandler()
{
	TRACE(fms_cpf_serverTrace, "%s", "Entering in stopImmHandler");
	ACE_UINT64 stopEvent=1;
	ssize_t numByte;
	ACE_INT32 eventfd;
	bool result = true;

	IMM_Handler->getStopHandle(eventfd);

	// Signal to IMM thread to stop
	numByte = ::write(eventfd, &stopEvent, sizeof(ACE_UINT64));

	if(sizeof(ACE_UINT64) != numByte)
	{
		CPF_Log.Write("FMS_CPF_Server::stopImmHandler, error on stop IMM_Handler", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_serverTrace, "%s", "stopImmHandler error on stop IMM_Handler");
		result = false;
	}
	else
	{
		TRACE(fms_cpf_serverTrace, "%s", "stopImmHandler, wait for ImmHandler termination");
		IMM_Handler->wait();
	}

	TRACE(fms_cpf_serverTrace, "%s", "Leaving stopImmHandler");
	return result;
}

/*============================================================================
	ROUTINE: waitBeforeRetry
 ============================================================================ */
int FMS_CPF_Server::waitBeforeRetry() const
{
	TRACE(fms_cpf_serverTrace, "%s", "Entering waitBeforeRetry");
	int result = 0;

	struct pollfd fds[1];
	nfds_t nfds =1;
	ACE_INT32 ret;
	char msg_buff[256]={'\0'};
	ACE_Time_Value timeout;

	// Initialize the pollfd structure
	ACE_OS::memset(fds, 0 , sizeof(fds));

	__time_t secs = 5;
	__suseconds_t usecs = 0;
	timeout.set(secs, usecs);

	fds[0].fd = m_StopEvent;
	fds[0].events = POLLIN;

	TRACE(fms_cpf_serverTrace, "%s", "waitBeforeRetry, waiting 5s before re-try");

	// Waits for 5s before re-try, exit in case of a stop service request
	while(true)
	{
		ret = ACE_OS::poll(fds, nfds, &timeout);

		// Error on poll
		if( -1 == ret )
		{
			// Error on poll
			if(errno == EINTR)
			{
				continue;
			}
			::snprintf(msg_buff,sizeof(msg_buff)-1,"FMS_CPF_Server::waitBeforeRetry, exit after error=%s", ::strerror(errno) );
			CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
			TRACE(fms_cpf_serverTrace, "%s","waitBeforeRetry, exit after error on poll");
			break;
		}

		/* Time out happened */
		if( 0 == ret )
		{
			TRACE(fms_cpf_serverTrace, "%s", "waitBeforeRetry, time-out expired");
			break;
		}

		if(fds[0].revents & POLLIN)
		{
			// Received signal of Server thread termination
			TRACE(fms_cpf_serverTrace, "%s", "waitBeforeRetry, received Server termination signal");
			result = 1;
			break;
		}
	}
	TRACE(fms_cpf_serverTrace, "%s", "Leaving waitBeforeRetry");
	return result;
}

/*============================================================================
	ROUTINE: waitOnShotdown
 ============================================================================ */
bool FMS_CPF_Server::waitOnShotdown()
{
	TRACE(fms_cpf_serverTrace, "%s", "Entering waitOnShotdown");
	// Check if IMM thread is already terminated
	if(IMM_Handler->getSvcState())
	{
		// wait for IMM thread termination
		struct pollfd fds[1];
		nfds_t nfds =1;
		ACE_INT32 ret;
		char msg_buff[256]={'\0'};
		ACE_Time_Value timeout;

		__time_t secs = 5;
		__suseconds_t usecs = 0;
		timeout.set(secs, usecs);

		fds[0].fd = IMM_Handler->getSvcEndHandle();
		fds[0].events = POLLIN;

		TRACE(fms_cpf_serverTrace, "%s", "waitOnShotdown, waiting for IMM thread termination");

		while(true)
		{
			ret = ACE_OS::poll(fds, nfds, &timeout);

			if( -1 == ret )
			{
				// Error on poll
				if(errno == EINTR)
				{
					continue;
				}
				::snprintf(msg_buff,sizeof(msg_buff)-1,"FMS_CPF_Server::waitOnShotdown, exit after error=%s", ::strerror(errno) );
				CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
				TRACE(fms_cpf_serverTrace, "%s","waitOnShotdown, exit after error on poll");
				break;
			}

			/* Time out happened */
			if( 0 == ret )
			{
				continue;
			}

			if(fds[0].revents & POLLIN)
			{
				// Received signal of IMM thread termination
				TRACE(fms_cpf_serverTrace, "%s", "waitOnShotdown, received IMM thread termination signal");
				break;
			}
		}
	}
	else
	{
		// IMM thread already terminated
		TRACE(fms_cpf_serverTrace, "%s", "waitOnShotdown, IMM thread already terminated");
	}
	TRACE(fms_cpf_serverTrace, "%s", "Leaving waitOnShotdown");
	return true;
}
