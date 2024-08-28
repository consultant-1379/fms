/*
 * * @file fms_cpf_cpchannel.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_CpChannel.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_cpchannel.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-11
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
 *	| 1.0.0  | 2011-11-11 | qvincon      | File created.                       |
 *	| 1.1.0  | 2012-02-07 | qvincon      | TR fix.                       |
 *	+========+============+==============+=====================================+
 */
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_cpchannel.h"
#include "fms_cpf_cpopenchannels.h"
#include "fms_cpf_cpmsgfactory.h"
#include "fms_cpf_cpmsg.h"
#include "fms_cpf_message.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_common.h"

#include "ACS_DSD_Session.h"
#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include <ace/Barrier.h>

#include "ace/OS_NS_poll.h"
#include <sys/eventfd.h>

extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	ROUTINE: FMS_CPF_CpChannel
 ============================================================================ */
FMS_CPF_CpChannel::FMS_CPF_CpChannel(ACS_DSD_Session* dsdSession, const bool sysType):
m_dsdSession(dsdSession),
m_numOfHandles(0),
m_dsdHandles(NULL),
m_IsMultiCP(sysType),
m_CpName(""),
m_CpId(-1),
m_ThreadsSyncShutdown(NULL)
{
	// create the file descriptor to signal stop
	// initial value is 0
	m_closeEvent = eventfd(0, 0);
	fms_cpf_CpChTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_CpChannel");
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
int FMS_CPF_CpChannel::open(void *args)
{
	TRACE(fms_cpf_CpChTrace, "%s", "Entering open()");
	//To avoid unused warning
	UNUSED(args);
	int result;

	// Create object to sync thread termination
	m_ThreadsSyncShutdown = new (std::nothrow) ACE_Barrier(FMS_CPF_CpChannel::N_THREADS);

	// Check if allocated
	if(NULL == m_ThreadsSyncShutdown)
	{
		CPF_Log.Write("FMS_CPF_CpChannel::open(), error on ACE_Barrier allocation", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_CpChTrace, "%s", "open(), error on ACE_Barrier allocation");
		return FAILURE;
	}

	ACS_DSD_Node dsdNode;
	int dsdResult = m_dsdSession->get_remote_node(dsdNode);

	if( acs_dsd::ERR_NO_ERRORS != dsdResult)
	{
		char errorMsg[512]={'\0'};
		ACE_OS::snprintf(errorMsg, 511, "FMS_CPF_CpChannel::open, error: %d <%s>, to get_remote_node() on dsd session", dsdResult, m_dsdSession->last_error_text());
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_CpChTrace, "%s", errorMsg);
		return FAILURE;
	}
	else
	{
		// Get CpId of connected Cp
		m_CpId = dsdNode.system_id;

		// Get default CpName of connected Cp
		if(m_IsMultiCP)
		{
			m_CpName = ParameterHndl::instance()->getConfigReader()->cs_getDefaultCPName(m_CpId);
		}

		// To get the number of handle m_dsdHandles=NULL
		dsdResult = m_dsdSession->get_handles(m_dsdHandles, m_numOfHandles);

		if(acs_dsd::ERR_NOT_ENOUGH_SPACE != dsdResult )
		{
			char errorMsg[512]={'\0'};
			ACE_OS::snprintf(errorMsg, 511, "FMS_CPF_CpChannel::open, error: %d <%s>, to get_handles() on dsd session", dsdResult, m_dsdSession->last_error_text());
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_CpChTrace, "%s", errorMsg);
			return FAILURE;
		}

		TRACE(fms_cpf_CpChTrace,"svc, gets <%d> dsd session handles", m_numOfHandles);

		// Now m_numOfHandles indicates number of handles to retrieve
		m_dsdHandles = new acs_dsd::HANDLE[m_numOfHandles];

		dsdResult = m_dsdSession->get_handles(m_dsdHandles, m_numOfHandles);

		if( acs_dsd::ERR_NO_ERRORS != dsdResult )
		{
			char errorMsg[512]={'\0'};
			ACE_OS::snprintf(errorMsg, 511, "FMS_CPF_CpChannel::open, error: %d <%s>, to get_handles() on dsd session", dsdResult, m_dsdSession->last_error_text());
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_CpChTrace, "%s", errorMsg);
			return FAILURE;
		}

		TRACE(fms_cpf_CpChTrace, "open(), CP<%s, %i> connected", m_CpName.c_str(), m_CpId);
		// Start the worker thread
		result = activate();
	}

	return result;
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
void FMS_CPF_CpChannel::getDefaultCpName(std::string& cpName) const
{
	// In single CP must be an empty string for
	// a correct access to internal structures
	cpName = "";
	if(m_IsMultiCP)
	{
		cpName = m_CpName;
	}
}
/*============================================================================
		ROUTINE: svc
 ============================================================================ */
int FMS_CPF_CpChannel::close(u_long flags)
{
	TRACE(fms_cpf_CpChTrace,"close(), Channel of CP<%s> closed", m_CpName.c_str() );
	// to avoid unused warning message
	UNUSED(flags);

	// Just to be sure, shut the barrier down, aborting the wait of all waiting threads
	m_ThreadsSyncShutdown->shutdown();

	//Close the opened stream , it will be never NULL
	m_dsdSession->close();

	if( CpChannelsList::instance()->removeCpChannel(this) == false)
	{
		// Some problem occured to enqueue the remove request, is it a real case?
		// Log it
		CPF_Log.Write("FMS_CPF_CpChannel::close, error to enqueue channel remove request", LOG_LEVEL_ERROR);

		// if this method is not called, ACE and the OS won't reclaim the thread stack and exit status
		//of a joinable thread, and the program will leak memory.
		wait();

		// delete cpchannel, Reclaim the task memory
		delete this;
	}

	return SUCCESS;
}

/*============================================================================
	ROUTINE: receiveMsg
 ============================================================================ */
int FMS_CPF_CpChannel::sendMsg(const char* msgBuffer, unsigned int msgBufferSize)
{
	TRACE(fms_cpf_CpChTrace, "%s", "Entering in sendMsg()");
	int result;
	ssize_t sendByte;

	{
		// Get lock automatically released
		ACE_Guard<ACE_Thread_Mutex> guard(m_dsdSessionLock);
		sendByte = m_dsdSession->send(msgBuffer, msgBufferSize);
	}

	result = static_cast<int>(sendByte);
	// Check operation result
	if( sendByte <= SUCCESS )
	{
		// Some error happens
		char errorMsg[512]={'\0'};
		ACE_OS::snprintf(errorMsg, 511, "FMS_CPF_CpChannel::sendMsg, error: %d, <%s>", result, m_dsdSession->last_error_text());
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_CpChTrace, "%s", errorMsg);
	}

	TRACE(fms_cpf_CpChTrace, "%s", "Leaving sendMsg()");
	return result;
}
/*============================================================================
	ROUTINE: receiveMsg
 ============================================================================ */
int FMS_CPF_CpChannel::receiveMsg()
{
	TRACE(fms_cpf_CpChTrace, "%s", "Entering receiveMsg()");
	int result;
	size_t msgBufferSize = 65000U;
	ssize_t recByte;
	char* msgBuffer = new (std::nothrow) char[msgBufferSize];

	if(NULL == msgBuffer)
	{
		CPF_Log.Write("FMS_CPF_CpChannel::receiveMsg(), error to allocate message buffer", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_CpChTrace, "%s", "receiveMsg(), error to allocate message buffer");
		result = SUCCESS;
		return result;
	}

	{
		// Get lock automatically released
		ACE_Guard<ACE_Thread_Mutex> guard(m_dsdSessionLock);
		// Receive msg on DSD
		recByte = m_dsdSession->recv(msgBuffer, msgBufferSize);
	}

	// Check operation result
	if(recByte >= acs_dsd::ERR_NO_ERRORS )
	{
		// Pass received buffer to the message factory
		// the created message will be the owner of the memory buffer

		if(!CPMsgFactory::instance()->handleMsg(msgBuffer, recByte, this) )
		{
			// Memory problem but the channel could still work
			TRACE(fms_cpf_CpChTrace, "%s", "receiveMsg(), error to handle received buffer");
			// reclaim allocated memory
			delete[] msgBuffer;
		}
		result = SUCCESS;
	}
	else
	{
		result = static_cast<int>(recByte);
		char errorMsg[512]={'\0'};
		ACE_OS::snprintf(errorMsg, 511, "FMS_CPF_CpChannel::receiveMsg() from Cp:<%s>, error: %d, <%s>", m_CpName.c_str(), result, m_dsdSession->last_error_text());
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_CpChTrace, "%s", errorMsg);

		// reclaim allocated memory
		delete[] msgBuffer;
	}

	TRACE(fms_cpf_CpChTrace, "%s", "Leaving receiveMsg()");
	return result;
}

/*============================================================================
	ROUTINE: ReceiverCpDataThread
 ============================================================================ */
ACE_THR_FUNC_RETURN FMS_CPF_CpChannel::ReceiverCpDataThread(void* ptrParam)
{
	FMS_CPF_CpChannel* m_this = reinterpret_cast<FMS_CPF_CpChannel*> (ptrParam);

	ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_this->m_mutex, ;);
	pid_t threadId = syscall(SYS_gettid);
	TRACE(m_this->fms_cpf_CpChTrace, "Cp<%s> channel entering in ReceiverCpDataThread(), threadId:<%d>", m_this->getConnectedCpName(), threadId );
	bool threadRun = true;

	int nfds = m_this->getNumOfHandle() + 1;
	struct pollfd fds[nfds];

	// Initialize the pollfd structure
	ACE_OS::memset(fds, 0, sizeof(fds));

	fds[0].fd = m_this->m_closeEvent;
	fds[0].events = POLLIN;

	// set all session handles
	for(int idx = 1; idx < nfds; ++idx)
	{
		fds[idx].fd = m_this->m_dsdHandles[idx-1];
		fds[idx].events = POLLIN;
	}

	TRACE(m_this->fms_cpf_CpChTrace, "%s", "svc, start waiting on events");
	int pollResult;
	int recResult;

	// Start thread loop
	while(threadRun)
	{
		pollResult = ACE_OS::poll(fds, nfds);

		if( FAILURE == pollResult )
		{
			if(errno == EINTR)
			{
				continue;
			}
			TRACE(m_this->fms_cpf_CpChTrace, "%s","svc, exit after a poll error");
			break;
		}

		// channel shutdown request
		if(fds[0].revents & POLLIN)
		{
			// Received a stop request from server
			TRACE(m_this->fms_cpf_CpChTrace, "%s","svc, received a SHUTDOWN request");
			threadRun = false;
			// exit from the while loop
			continue;
		}

		// check which handle has been signaled
		for(int idx = 1; idx < nfds; ++idx)
		{
			if(fds[idx].revents & POLLIN)
			{
				// receive data message
				recResult = m_this->receiveMsg();

				// check operation result
				if(SUCCESS != recResult)
				{
					// Error on receive start channel closure
					threadRun = false;
					// to stop svc thread
					m_this->msg_queue_->deactivate();
				}
				// break for loop
				break;
			}
		}
	}

	TRACE(m_this->fms_cpf_CpChTrace, "Cp<%s> channel ReceiverCpDataThread() waiting svc end before exit, , threadId:<%d>", m_this->getConnectedCpName(), threadId );

	// Threads shutdown barrier, Block the caller until all N_THREADS count threads have called N_THREADS wait and
	// then allow all the caller threads to continue in parallel.
	m_this->m_ThreadsSyncShutdown->wait();

	return SUCCESS;
}
/*============================================================================
	ROUTINE: svc
 ============================================================================ */
int FMS_CPF_CpChannel::svc()
{
	TRACE(fms_cpf_CpChTrace, "Cp<%s> channel entering in svc()", m_CpName.c_str());
	int getResult;
	ACE_Message_Block* base_msg;
	FMS_CPF_CPMsg* cp_msg;
	bool svcRun = true;
	pid_t threadId = syscall(SYS_gettid);
	char logMsg[512] = {'\0'};

	// Start a different thread for receive CP data
	ACE_Thread_Manager::instance()->spawn(ReceiverCpDataThread, (void*)this, THR_NEW_LWP| THR_DETACHED | THR_INHERIT_SCHED);

	ACE_OS::snprintf(logMsg, 511, "FMS_CPF_CpChannel::svc(), Cp<%s> connected, svcThrdId:<%d>", m_CpName.c_str(), threadId );
	CPF_Log.Write(logMsg, LOG_LEVEL_INFO);
	TRACE(fms_cpf_CpChTrace, "Cp<%s> connected, svcThrdId:<%d>", m_CpName.c_str(), threadId);

	// thread loop
	while(svcRun)
	{
		// Waiting for a message
		getResult = getq(base_msg);

		//Check operation result
		if(FAILURE == getResult)
		{
			//some error or closure request
			// break the thread loop
			svcRun = false;
			continue;
		}

		// get received message type
		int msg_type = static_cast<int>(base_msg->msg_type());

		// check received message type
		switch(msg_type)
		{
			// same action for all
			case FMS_CPF_CPMsg::MT_SYNC_MSG :
			case FMS_CPF_CPMsg::MT_ECHO_MSG :
			case FMS_CPF_CPMsg::MT_OPEN_MSG :
			case FMS_CPF_CPMsg::MT_CLOSE_MSG :
			case FMS_CPF_CPMsg::MT_WRITEAND_MSG :
			case FMS_CPF_CPMsg::MT_READAND_MSG :
			case FMS_CPF_CPMsg::MT_WRITENEXT_MSG :
			case FMS_CPF_CPMsg::MT_READNEXT_MSG :
			case FMS_CPF_CPMsg::MT_REWRITE_MSG :
			case FMS_CPF_CPMsg::MT_RESET_MSG :
			case FMS_CPF_CPMsg::MT_CONNECT_MSG :
			case FMS_CPF_CPMsg::MT_UNKNOW_MSG :
			{
				cp_msg = reinterpret_cast<FMS_CPF_CPMsg*>(base_msg);
				if( sendMsg(cp_msg->replyBuffer(), cp_msg->replyBufferSize()) <= SUCCESS )
				{
					svcRun = false;
				}
			}
			break;

			default:
			{
				CPF_Log.Write("FMS_CPF_CpChannel::svc(), get an invalid message", LOG_LEVEL_WARN);
				TRACE(fms_cpf_CpChTrace, "%s", "svc(), get an invalid message");
			}
		}
		// Reclaim the allocated memory
		base_msg->release();
	}

	TRACE(fms_cpf_CpChTrace, "svc(), Cp<%s> channel going to close, svcThrdId:<%d>", m_CpName.c_str(), threadId );

	// Signal internal thread stop
	if( stopReceiverCpDataThread() )
	{
		// Threads shutdown barrier, Block the caller until all N_THREADS count threads have called N_THREADS wait and
		// then allow all the caller threads to continue in parallel.
		TRACE(fms_cpf_CpChTrace, "svc(), Cp<%s> channel wait on receive thread closure", m_CpName.c_str() );
		m_ThreadsSyncShutdown->wait();
	}

	ACE_OS::snprintf(logMsg, 511, "FMS_CPF_CpChannel::svc(),  svcThrdId:<%d> of Cp<%s> terminated",  threadId, m_CpName.c_str() );
	CPF_Log.Write(logMsg, LOG_LEVEL_INFO);

	TRACE(fms_cpf_CpChTrace, "Cp<%s> channel leaving svc()", m_CpName.c_str());
	return SUCCESS;
}

/*============================================================================
	ROUTINE: stopReceiverCpDataThread
 ============================================================================ */
bool FMS_CPF_CpChannel::stopReceiverCpDataThread()
{
	TRACE(fms_cpf_CpChTrace, "%s", "Entering in signalGlobalShutdown");
	bool result = true;
	uint64_t stopEvent = 1;
	ssize_t numByte;
	// Signal to internal thread to stop
	numByte = ::write(m_closeEvent, &stopEvent, sizeof(uint64_t));

	if(sizeof(uint64_t) != numByte)
	{
		result = false;
		CPF_Log.Write("stopReceiverCpDataThread, error on signal internal thread closure", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_CpChTrace, "%s", "stopReceiverCpDataThread, error on internal thread closure");
	}

	TRACE(fms_cpf_CpChTrace, "%s", "Leaving signalGlobalShutdown");
	return result;
}
/*============================================================================
	ROUTINE: channelShutDown
 ============================================================================ */
void FMS_CPF_CpChannel::channelShutDown()
{
	TRACE(fms_cpf_CpChTrace, "%s", "Entering in channelShutDown()");
	// This method is called only on service shutdown
	// Deactivate the queue and wakeup all threads waiting on the queue so they can continue
	msg_queue_->deactivate();

	TRACE(fms_cpf_CpChTrace, "%s", "Leaving channelShutDown()");
}
/*============================================================================
	ROUTINE: FMS_CPF_CpChannel
 ============================================================================ */
FMS_CPF_CpChannel::~FMS_CPF_CpChannel()
{
	ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, ;);
	//Close stop signal file descriptor
	ACE_OS::close(m_closeEvent);

	// delete the dsd session object,it will be never NULL
	delete m_dsdSession;

	if(NULL != fms_cpf_CpChTrace)
		delete fms_cpf_CpChTrace;

	if( NULL != m_ThreadsSyncShutdown)
		delete m_ThreadsSyncShutdown;

	if( NULL != m_dsdHandles)
		delete[] m_dsdHandles;
}
