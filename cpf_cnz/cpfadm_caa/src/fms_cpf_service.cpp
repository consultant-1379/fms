//
/** @file fms_cpf_service.cpp
 *	@brief
 *	Class method implementation for service.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_service.h module
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

#include "fms_cpf_service.h"
#include "fms_cpf_server.h"
#include "fms_cpf_common.h"
#include "fms_cpf_parameterhandler.h"

#include "ACS_TRA_trace.h"

#include <ace/Thread_Manager.h>
#include "ace/OS_NS_poll.h"

#include <poll.h>
#include <syslog.h>
#include <grp.h>
#include <sys/capability.h>

ACS_TRA_Logging CPF_Log = ACS_TRA_Logging();

/*===================================================================
                        DIRECTIVE DECLARATION SECTION
===================================================================== */
// Timeout of blocking pool in sec
#define TIMEOUT_HA_PIPE    5

/*===========================================================================
	ROUTINE: cpf_service_thread
 ========================================================================== */
ACE_THR_FUNC_RETURN FMS_CPF_Service::cpf_service_thread(void* ptrParam)
{

	FMS_CPF_Service* ptrCPF_Service = (FMS_CPF_Service*) ptrParam;

	CPF_Log.Write("CPF service worker thread started", LOG_LEVEL_INFO);

	if( NULL != ptrCPF_Service )
	{
		ptrCPF_Service->setWorkerThreadState(RUNNING);

		ptrCPF_Service->cpf_svc();

		ptrCPF_Service->setWorkerThreadState(STOPPED);
	}

	CPF_Log.Write("CPF service worker thread terminated", LOG_LEVEL_INFO);
	return 0;
}

/*===============================================================================
	ROUTINE: FMS_CPF_Service
 =============================================================================== */
FMS_CPF_Service::FMS_CPF_Service(const char* daemon_name, const char* username):ACS_APGCC_ApplicationManager(daemon_name, username)
{
	svc_run = true;
	isWorkerThreadOn = false;
	workerThreadState = NOTSTARTED;
	cpfServer = NULL;
	applicationThreadId = 0;
	fms_cpf_serviceTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_Service");

	// Open log4cplus log file
	CPF_Log.Open(FMS_CPF_LOG_APPENDER_NAME);

	// Clear CAP_SYS_RESOURCE bit thus root user cannot override disk quota limits
	cap_t cap = cap_get_proc();

	if(NULL != cap)
	{
		cap_value_t cap_list[1];
		cap_list[0] = CAP_SYS_RESOURCE;

		// Clear capability CAP_SYS_RESOURCE
		if(cap_set_flag(cap, CAP_EFFECTIVE, 1, cap_list, CAP_CLEAR) == FAILURE)
		{
			// handle error
			char msg_buff[128] = {0};
			snprintf(msg_buff, sizeof(msg_buff)-1, "%s, cap_set_flag() failed, error=%s", __func__, strerror(errno) );
			CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
		}
		else
		{
			// Change process capability
			if (cap_set_proc(cap) == FAILURE)
			{
				// handle error
				char msg_buff[128] = {0};
				snprintf(msg_buff, sizeof(msg_buff)-1, "%s, cap_set_proc() failed, error=%s", __func__, strerror(errno) );
				CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
			}
		}

		if(cap_free(cap) == FAILURE)
		{
			// handle error
			char msg_buff[128] = {0};
			snprintf(msg_buff, sizeof(msg_buff)-1, "%s, cap_free() failed, error=%s", __func__, strerror(errno) );
			CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
		}
	}
	else
	{
		// handle error
		char msg_buff[128] = {0};
		snprintf(msg_buff,sizeof(msg_buff)-1,"%s, cap_get_proc() failed, error=%s", __func__, strerror(errno) );
		CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
	}


	/* create the pipe for shutdown handler */
	if ( (pipe(readWritePipe)) < 0)
	{
		TRACE(fms_cpf_serviceTrace, "%s", "Pipe creation failed in FMS_CPF_Service");
	}

	if ( (fcntl(readWritePipe[READ_PIPE], F_SETFL, O_NONBLOCK)) < 0)
	{
		TRACE(fms_cpf_serviceTrace, "%s", "Pipe fcntl on read failed in FMS_CPF_Service");
	}

	if ( (fcntl(readWritePipe[WRITE_PIPE], F_SETFL, O_NONBLOCK)) < 0)
	{
		TRACE(fms_cpf_serviceTrace, "%s", "Pipe fcntl on write failed in FMS_CPF_Service");
	}
}

/*===============================================================================
	ROUTINE: ~FMS_CPF_Service
 =============================================================================== */
FMS_CPF_Service::~FMS_CPF_Service()
{
	if(NULL != fms_cpf_serviceTrace)
		delete fms_cpf_serviceTrace;
	close(readWritePipe[READ_PIPE]);
	close(readWritePipe[WRITE_PIPE]);
}
/*============================================================================
	ROUTINE: nsf_svc
 ============================================================================ */
ACS_APGCC_ReturnType FMS_CPF_Service::cpf_svc()
{
	TRACE(fms_cpf_serviceTrace, "%s", "Entering cpf_svc()");

	ACS_APGCC_ReturnType result = ACS_APGCC_SUCCESS;

	// Create a fd to wait for request from CPF main Thread
	struct pollfd fds[1];
	nfds_t nfds = 1;
	ACE_INT32 ret;
	ACE_Time_Value timeout;
	ACE_INT32 retCode;
    char msg_buff[256]={'\0'};

    __time_t secs = TIMEOUT_HA_PIPE;
	__suseconds_t usecs = 0;
	timeout.set(secs, usecs);

	fds[0].fd = readWritePipe[READ_PIPE];
	fds[0].events = POLLIN;

	TRACE(fms_cpf_serviceTrace, "%s", "Starting CPF Application Thread");

	while(svc_run && (result == ACS_APGCC_SUCCESS) )
	{
		ret = ACE_OS::poll(fds, nfds, &timeout);

		if( -1 == ret )
		{
			if(errno == EINTR)
			{
				continue;
			}
			snprintf(msg_buff,sizeof(msg_buff)-1,"FMS_CPF_Service::cpf_svc(), error=%s", strerror(errno) );
			CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
			result = ACS_APGCC_FAILURE;
			break;
		}

		/* Time out happened */
		if( 0 == ret )
		{
			continue;
		}

		if(fds[0].revents & POLLIN)
		{
			ACE_TCHAR ha_state[1] = {'\0'};
			ACE_TCHAR* ptr = (ACE_TCHAR*) &ha_state;
			ACE_INT32 len = sizeof(ha_state);

			// Reads on pipe
			while(len > 0)
			{
				retCode = read(readWritePipe[READ_PIPE], ptr, len);
				if ( ( retCode < 0 ) && ( EINTR != errno )  )
				{
					snprintf(msg_buff,sizeof(msg_buff)-1,"FMS_CPF_Service::cpf_svc(), Pipe Read interrupted by error: %s", strerror(errno) );
					CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
					result = ACS_APGCC_FAILURE;
					break;
				}
				else
				{
					ptr += retCode;
					len -= retCode;
				}
				// check if all data are been read
				if( 0 == retCode )
				{
					break;
				}
			}

			// Check data read
			if( 0 != len )
			{
				snprintf(msg_buff,sizeof(msg_buff)-1,"FMS_CPF_Service::cpf_svc(), error on read msg length, len= %d", len);
				CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
				result = ACS_APGCC_FAILURE;
				break;
			}

			len = sizeof(ha_state);

			if( 'A' == ha_state[0] )
			{
				/* State is ACTIVE : start application worker */

				TRACE(fms_cpf_serviceTrace, "%s", "cpf_svc(), HA state is active");

				cpfServer = new (std::nothrow) FMS_CPF_Server();

				if( NULL == cpfServer )
				{
					CPF_Log.Write("Memory allocation failed for FMS_CPF_Server object", LOG_LEVEL_FATAL);
					result = ACS_APGCC_FAILURE;
				}
				else
				{
					if(!cpfServer->startWorkerThreads())
					{
						CPF_Log.Write("CPF Application Thread was not started properly", LOG_LEVEL_FATAL);
						delete cpfServer;
						cpfServer = NULL;
						result =  ACS_APGCC_FAILURE;
					}
				}
			}
			else if ( 'P' == ha_state[0] )
			{
				/* State is PASSIVE : stop application worker if running*/

				TRACE(fms_cpf_serviceTrace, "%s", "cpf_svc(), HA state is passive");

				if( NULL != cpfServer )
				{
					TRACE(fms_cpf_serviceTrace, "%s", "Requesting CPF Application Thread to stop");

					cpfServer->stopWorkerThreads();

					TRACE(fms_cpf_serviceTrace, "%s", "CPF Server Stopped");

					delete cpfServer;
					cpfServer = NULL;
				}
			}
			else if ( 'S' == ha_state[0] )
			{
				/* State is STOP : stop application worker if running and close this thread */

				TRACE(fms_cpf_serviceTrace, "%s", "cpf_svc:: Request to stop application");
				// Request to stop the thread, perform the gracefully activities here

				if( NULL != cpfServer )
				{
					TRACE(fms_cpf_serviceTrace, "%s", "Requesting CPF Application Thread to stop");

					cpfServer->stopWorkerThreads();

					TRACE(fms_cpf_serviceTrace, "%s", "CPF Server Stopped");

					delete cpfServer;
					cpfServer = NULL;
				}

				break;
			}
		}
	}

	TRACE(fms_cpf_serviceTrace, "%s", "Leaving cpf_svc");
	return result;
}

/*================================================================================
	ROUTINE: performStateTransitionToActiveJobs
 ================================================================================ */
ACS_APGCC_ReturnType FMS_CPF_Service::performStateTransitionToActiveJobs(ACS_APGCC_AMF_HA_StateT previousHAState)
{
	TRACE(fms_cpf_serviceTrace, "%s", "Entering performStateTransitionToActiveJobs");

	ACE_TCHAR state[1] = {'A'};
	const ACE_TCHAR* cpfAppThread = "CPFServiceThread";

	CPF_Log.Write("CPF Service: received performStateTransitionToActiveJobs callback", LOG_LEVEL_INFO);
	/* Check if we have received the ACTIVE State Again.
	   This means that, our application is already Active and
	   again we have got a callback from AMF to go active.
	   Ignore this case anyway.
	*/
	if(ACS_APGCC_AMF_HA_ACTIVE == previousHAState)
	{
		TRACE(fms_cpf_serviceTrace, "%s", "Leaving  performStateTransitionToActiveJobs(), HA state is already active");
		CPF_Log.Write("FMS_CPF_Service: state is already active", LOG_LEVEL_INFO);
		return ACS_APGCC_SUCCESS;
	}

	/* Start off with the activities needs to be performed on ACTIVE
	   Check if it is due to State Transition ( Passive --> Active)
	*/
	if ( ACS_APGCC_AMF_HA_STANDBY == previousHAState )
	{
		// State Transition happened. Becoming Active
		if( write(readWritePipe[WRITE_PIPE], &state, sizeof(state)) <= 0 )
		{
			svc_run = false;
			CPF_Log.Write("FMS_CPF_Service (T: P->A): Error occurred while writing data into pipe", LOG_LEVEL_INFO);
			return ACS_APGCC_FAILURE;
		}

		CPF_Log.Write("FMS_CPF_Service: Transition To Active Jobs done, previous State Passive", LOG_LEVEL_INFO);
		TRACE(fms_cpf_serviceTrace, "%s", "Leaving performStateTransitionToActiveJobs");
		return ACS_APGCC_SUCCESS;
	}

	//ACTIVE State
	//Start the application worker thread.
	ACE_HANDLE cpfThreadHandle = ACE_Thread_Manager::instance()->spawn( &cpf_service_thread,
																		(void *)this ,
																		THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,
																		&applicationThreadId,
																		0,
																		ACE_DEFAULT_THREAD_PRIORITY,
																		-1,
																		0,
																		ACE_DEFAULT_THREAD_STACKSIZE,
																		&cpfAppThread
																	  );

	// Check spawn result
	if ( ACE_INVALID_HANDLE == cpfThreadHandle )
	{
		char logBuffer[256]={'\0'};
		snprintf(logBuffer, 255, "FMS_CPF_Service (T: <%d>->A): Error occurred while creating the CPFServiceThread", previousHAState);
		CPF_Log.Write(logBuffer, LOG_LEVEL_ERROR);
		return ACS_APGCC_FAILURE;
	}

	// Signal to the worker thread to become active
	if( write(readWritePipe[WRITE_PIPE], &state, sizeof(state)) <= 0 )
	{
		svc_run = false;
		char logBuffer[256]={'\0'};
		snprintf(logBuffer, 255, "FMS_CPF_Service (T: <%d>->A): Error occurred while writing data into pipe", previousHAState);
		CPF_Log.Write(logBuffer, LOG_LEVEL_ERROR);
		return ACS_APGCC_FAILURE;
	}

	isWorkerThreadOn = true;

	TRACE(fms_cpf_serviceTrace, "%s", "Leaving performStateTransitionToActiveJobs");
	CPF_Log.Write("CPF Service: Transition To Active Jobs done", LOG_LEVEL_INFO);
	return ACS_APGCC_SUCCESS;
}

/*===================================================================================
	ROUTINE: performStateTransitionToPassiveJobs
 ================================================================================== */
ACS_APGCC_ReturnType FMS_CPF_Service::performStateTransitionToPassiveJobs(ACS_APGCC_AMF_HA_StateT previousHAState)
{
	TRACE(fms_cpf_serviceTrace, "%s", "Entering performStateTransitionToPassiveJobs");

	ACE_TCHAR state[1] = {'P'};
	const ACE_TCHAR* cpfAppThread = "CPFServiceThread";

	CPF_Log.Write("CPF Service: received performStateTransitionToPassiveJobs callback", LOG_LEVEL_INFO);
	/* Check if we have received the PASSIVE State Again.
	 * This means that, our application was already Passive and
	 * again we have got a callback from AMF to go passive.
	 * Ignore this case anyway.
	*/
	if(ACS_APGCC_AMF_HA_STANDBY == previousHAState)
	{
		TRACE(fms_cpf_serviceTrace, "%s", "Leaving performStateTransitionToPassiveJobs(), HA state is already passive!");
		CPF_Log.Write("FMS_CPF_Service: state is already passive", LOG_LEVEL_INFO);
		return ACS_APGCC_SUCCESS;
	}

	/* Our application has received state PASSIVE from AMF.
     * Check if the state received is due to State Transition.
     * (Active->Passive).
     */
	if ( ACS_APGCC_AMF_HA_ACTIVE == previousHAState )
	{
		// State Transition happened. Becoming Passive
		if( write(readWritePipe[WRITE_PIPE], &state, sizeof(state)) <= 0 )
		{
			svc_run = false;
			CPF_Log.Write("FMS_CPF_Service (T: A->P): Error occurred while writing data into pipe.", LOG_LEVEL_INFO);
			return ACS_APGCC_FAILURE;
		}

		CPF_Log.Write("FMS_CPF_Service: Transition To Passive Jobs done, previous State Active", LOG_LEVEL_INFO);
		return ACS_APGCC_SUCCESS;
	}
	//Start the application thread in passive node.
	// spawn worker thread

	ACE_HANDLE cpfThreadHandle = ACE_Thread_Manager::instance()->spawn( &cpf_service_thread,
																		(void *)this ,
																		THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,
																		&applicationThreadId,
																		0,
																		ACE_DEFAULT_THREAD_PRIORITY,
																		-1,
																		0,
																		ACE_DEFAULT_THREAD_STACKSIZE,
																		&cpfAppThread
																	   );
	if ( ACE_INVALID_HANDLE == cpfThreadHandle)
	{
		char logBuffer[256]={'\0'};
		snprintf(logBuffer, 255, "FMS_CPF_Service (T: <%d>->P): Error occurred while creating the CPFServiceThread", previousHAState);
		CPF_Log.Write(logBuffer, LOG_LEVEL_ERROR);
		return ACS_APGCC_FAILURE;
	}

	if( write(readWritePipe[WRITE_PIPE], &state, sizeof(state)) <= 0 )
	{
		svc_run = false;
		char logBuffer[256]={'\0'};
		snprintf(logBuffer, 255, "FMS_CPF_Service (T: <%d>->P): Error occurred while writing data into pipe", previousHAState);
		CPF_Log.Write(logBuffer, LOG_LEVEL_ERROR);
		return ACS_APGCC_FAILURE;
	}

	isWorkerThreadOn = true;

	CPF_Log.Write("CPF Service: Transition To Passive Jobs done", LOG_LEVEL_INFO);
	TRACE(fms_cpf_serviceTrace, "%s", "Leaving performStateTransitionToPassiveJobs");
	return ACS_APGCC_SUCCESS;
}

/*============================================================================================
	ROUTINE: performStateTransitionToQueisingJobs
 ============================================================================================ */
ACS_APGCC_ReturnType FMS_CPF_Service::performStateTransitionToQueisingJobs(ACS_APGCC_AMF_HA_StateT previousHAState)
{
	// To avoid warning about unused parameter
	UNUSED(previousHAState);
	syslog(LOG_INFO, "received transition to state Queising.");
	/* We were active and now losing active state due to some shutdown admin
	 * operation performed on our SU.
	 * Inform the thread to go to "stop" state
    */
	// Inform the thread to go "stop" state
	ACS_APGCC_ReturnType result = stopWorkerThread();

	syslog(LOG_INFO, "transition to state Queising done.");

	return result;
}

/*=============================================================================================
	ROUTINE: performStateTransitionToQuiescedJobs
 ============================================================================================= */
ACS_APGCC_ReturnType FMS_CPF_Service::performStateTransitionToQuiescedJobs(ACS_APGCC_AMF_HA_StateT previousHAState)
{
	// To avoid warning about unused parameter
	UNUSED(previousHAState);

	syslog(LOG_INFO, "received transition to state Quiesced.");
	/* We were Active and now losting Active state due to Lock admin
	 * operation performed on our SU.
	 * Inform the thread to go to "stop" state
	 */

	ACS_APGCC_ReturnType result = stopWorkerThread();

	syslog(LOG_INFO, "transition to state Quiesced done.");

	return result;
}

/*====================================================================================
	ROUTINE: performComponentHealthCheck
 =================================================================================== */
ACS_APGCC_ReturnType FMS_CPF_Service::performComponentHealthCheck(void)
{
	ACS_APGCC_ReturnType result = ACS_APGCC_SUCCESS;

	/* Application has received health check callback from AMF. Check the
     * sanity of the application and reply to AMF that you are OK.
     */
	if(isWorkerThreadOn && isWorkerThreadOff() )
	{
		TRACE(fms_cpf_serviceTrace, "%s", "performComponentHealthCheck(), CPF server main thread not running!");
		CPF_Log.Write("FMS_CPF_Service: performComponentHealthCheck(), CPF server worker thread not running!", LOG_LEVEL_ERROR);
		result = ACS_APGCC_FAILURE;
	}
	return result;
}

/*=====================================================================
	ROUTINE: performComponentTerminateJobs
 ===================================================================== */
ACS_APGCC_ReturnType FMS_CPF_Service::performComponentTerminateJobs(void)
{
	syslog(LOG_INFO, "received transition to state Terminate.");

	ACS_APGCC_ReturnType result = stopWorkerThread();

	syslog(LOG_INFO, "transition to state Terminate done.");

	return result;
}

/*=======================================================================
	ROUTINE: performComponentRemoveJobs
 ======================================================================= */
ACS_APGCC_ReturnType FMS_CPF_Service::performComponentRemoveJobs(void)
{
	syslog(LOG_INFO, "received transition to state Remove.");

	ACS_APGCC_ReturnType result = stopWorkerThread();

	syslog(LOG_INFO, "transition to state Remove done.");

	return result;
}

/*======================================================================
	ROUTINE: performApplicationShutdownJobs
 ===================================================================== */
ACS_APGCC_ReturnType FMS_CPF_Service::performApplicationShutdownJobs()
{
	syslog(LOG_INFO, "received transition to state Shutdown.");

	ACS_APGCC_ReturnType result = stopWorkerThread();

	syslog(LOG_INFO, "transition to state Shutdown done.");
	return result;
}

/*======================================================================
	ROUTINE: stopWorkerThread
 ===================================================================== */
ACS_APGCC_ReturnType FMS_CPF_Service::stopWorkerThread()
{
	ACS_APGCC_ReturnType result = ACS_APGCC_SUCCESS;

	// Check the worker thread state
	if ( isWorkerThreadOn )
	{
		// State Stop
		ACE_TCHAR state[1] = {'S'};
		isWorkerThreadOn = false;
		syslog(LOG_INFO, "stopWorkerThread send stop signal.");

		// Inform the thread to go "stop" state
		// Write stop to the pipe
		if( write(readWritePipe[WRITE_PIPE], &state, sizeof(state)) <= 0 )
		{
			syslog(LOG_INFO, "stopWorkerThread pipe error:<%m>");
			svc_run = false;
			result = ACS_APGCC_FAILURE;
		}
		else
		{
			int grpId = 0;
			ACE_Thread_Manager::instance()->get_grp(applicationThreadId, grpId);

			syslog(LOG_INFO, "stopWorkerThread waiting on worker thread id<%d> termination", grpId);
			// Waiting on worker thread termination
			ACE_Thread_Manager::instance()->wait_grp(grpId);
		}
		syslog(LOG_INFO, "stopWorkerThread worker thread terminated");
	}

	return result;
}
