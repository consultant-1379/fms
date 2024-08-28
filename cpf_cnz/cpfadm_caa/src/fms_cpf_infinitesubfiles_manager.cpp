/*
 * * @file fms_cpf_infinitesubfiles_manager.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_InfiniteSubFiles_Manager.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_infinitesubfiles_manager.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-14
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
 *	| 1.0.0  | 2011-10-15 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 *
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_infinitesubfiles_manager.h"
#include "fms_cpf_rto_infinitesubfile.h"
#include "fms_cpf_cmdhandler.h"
#include "fms_cpf_message.h"
#include "fms_cpf_common.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include "ace/OS_NS_poll.h"
#include <ace/Barrier.h>
#include <sys/eventfd.h>

extern ACS_TRA_Logging CPF_Log;

namespace infiniteSubFileClass
{
	const std::string ImmImplementerName = "CPF_RTO_InfiniteSubFile";
}


/*============================================================================
	ROUTINE: FMS_CPF_InfiniteSubFiles_Manager
 ============================================================================ */
FMS_CPF_InfiniteSubFiles_Manager::FMS_CPF_InfiniteSubFiles_Manager():
m_RTO_InfiniteSubFile(NULL),
m_IsMultiCP(false),
m_cmdHandler(NULL)
{
	fms_cpf_roManTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_ISF_Manager");

	// Create object to sync thread termination
    m_ThreadsSyncShutdown = new ACE_Barrier(FMS_CPF_InfiniteSubFiles_Manager::N_THREADS);

	// create the file descriptor to signal stop
	m_StopEvent = eventfd(0,0);

	m_ImmResetEvent = eventfd(0,0);
}

/*============================================================================
	ROUTINE: ~FMS_CPF_InfiniteSubFiles_Manager
 ============================================================================ */
FMS_CPF_InfiniteSubFiles_Manager::~FMS_CPF_InfiniteSubFiles_Manager()
{
	ACE_OS::close(m_StopEvent);

	ACE_OS::close(m_ImmResetEvent);

	if(NULL != m_RTO_InfiniteSubFile)
	{
		delete m_RTO_InfiniteSubFile;
	}

	if(NULL != fms_cpf_roManTrace)
		delete fms_cpf_roManTrace;

	if(NULL != m_ThreadsSyncShutdown)
		delete m_ThreadsSyncShutdown;
}

/*============================================================================
	ROUTINE: stopInternalThread
 ============================================================================ */
bool FMS_CPF_InfiniteSubFiles_Manager::stopInternalThread()
{
	TRACE(fms_cpf_roManTrace, "%s", "Entering in stopInternalThread");
	ACE_UINT64 stopEvent=1;
	ssize_t numByte;
	bool result = true;

	// Signal to internal thread to stop
	numByte = ::write(m_StopEvent, &stopEvent, sizeof(ACE_UINT64));

	if(sizeof(ACE_UINT64) != numByte)
	{
		CPF_Log.Write("FMS_CPF_InfiniteSubFiles_Manager::stopInternalThread, error on stop internal thread", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_roManTrace, "%s", "stopInternalThread, error on stop internal thread");
		result = false;
	}

	TRACE(fms_cpf_roManTrace, "%s", "Leaving stopInternalThread");
	return result;
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
int FMS_CPF_InfiniteSubFiles_Manager::open(void *args)
{
	TRACE(fms_cpf_roManTrace, "%s", "Entering open()");
	int result = FAILURE;
	// To avoid warning about unused parameter
	UNUSED(args);

	m_RTO_InfiniteSubFile = new (std::nothrow) FMS_CPF_RTO_InfiniteSubFile(m_cmdHandler);

	if(NULL != m_RTO_InfiniteSubFile)
	{
		bool immResult = m_RTO_InfiniteSubFile->registerToImm(m_IsMultiCP, infiniteSubFileClass::ImmImplementerName);

		if(immResult)
		{
			// Start the thread to handler Infinite SubFile
			result = activate();
			if( FAILURE == result )
			{
				CPF_Log.Write("FMS_CPF_InfiniteSubFiles_Manager::open, error on start svc thread", LOG_LEVEL_ERROR);
				TRACE(fms_cpf_roManTrace, "%s", "Leaving open(), error on start svc thread");
			}
		}
		else
		{
			CPF_Log.Write("FMS_CPF_InfiniteSubFiles_Manager::open(), registerToImm() return false", LOG_LEVEL_ERROR);
			TRACE(fms_cpf_roManTrace, "%s", "Leaving open(), error on registerToImm()");
		}
	}
	else
	{
		CPF_Log.Write("FMS_CPF_InfiniteSubFiles_Manager::open, error on RT ISF Owner allocation", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_roManTrace, "%s", "Leaving open(), error on RT ISF Owner allocation");
	}

	TRACE(fms_cpf_roManTrace, "%s", "Leaving open()");
	return result;
}

/*============================================================================
	ROUTINE: setSystemParameters
 ============================================================================ */
void  FMS_CPF_InfiniteSubFiles_Manager::setSystemParameters(FMS_CPF_CmdHandler* cmdHandler, const bool systemType, const std::list<std::string>& listOfCP )
{
	TRACE(fms_cpf_roManTrace,"%s","Entering in setSystemParameters");
	// make a copy of system parameters
	m_cmdHandler = cmdHandler;
	m_IsMultiCP = systemType;
	m_ListOfCP = listOfCP;
	TRACE(fms_cpf_roManTrace,"%s","Leaving setSystemParameters");
}

/*============================================================================
	ROUTINE: immOwnerReset
 ============================================================================ */
void FMS_CPF_InfiniteSubFiles_Manager::immOwnerReset()
{
	TRACE(fms_cpf_roManTrace, "%s", "Entering in immOwnerReset()");

	m_RTO_InfiniteSubFile->finalize();

	bool retry = true;
	do
	{
		if( m_RTO_InfiniteSubFile->init(infiniteSubFileClass::ImmImplementerName) == ACS_CC_FAILURE)
		{
			char msgBuff[256] = {0};
			ACE_OS::snprintf(msgBuff, 255, "FMS_CPF_InfiniteSubFiles_Manager::immOwnerReset(), error<%d> on register RT owner", m_RTO_InfiniteSubFile->getInternalLastError() );

			CPF_Log.Write(msgBuff, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_roManTrace, "%s", msgBuff);

			nfds_t nfds = 1;
			struct pollfd fds[nfds];

			// Initialize the pollfd structure
			ACE_OS::memset(fds, 0 , sizeof(fds));

			__time_t secs = 1;
			ACE_Time_Value timeout(secs);

			fds[0].fd = m_StopEvent;
			fds[0].events = POLLIN;
			ACE_INT32 ret = ACE_OS::poll(fds, nfds, &timeout);

			// Error on poll or Time-Out
			if( ( -1 == ret ) || ( 0 == ret ) )
			{
				TRACE(fms_cpf_roManTrace, "%s", "immOwnerReset(), retry run-time owner set");
				continue;
			}

			if( fds[0].revents & POLLIN )
			{
				// Received signal of Server thread termination
				TRACE(fms_cpf_roManTrace, "%s", "immOwnerReset(), received Server termination signal");
				break;
			}
		}
		else
		{
			retry = false;
			TRACE(fms_cpf_roManTrace, "%s", "immOwnerReset(), IMM Run-Time owner set");
		}

	}while(retry);

	TRACE(fms_cpf_roManTrace, "%s", "Leaving immOwnerReset()");
}

/*============================================================================
	ROUTINE: ISF_UpdateCallbackHnd
 ============================================================================ */
ACE_THR_FUNC_RETURN FMS_CPF_InfiniteSubFiles_Manager::ISF_UpdateCallbackHnd(void* ptrParam)
{
	FMS_CPF_InfiniteSubFiles_Manager* m_this = reinterpret_cast<FMS_CPF_InfiniteSubFiles_Manager*> (ptrParam);
	TRACE(m_this->fms_cpf_roManTrace, "%s","Entering in ISFUpdateCallbackHnd thread");

	bool thread_Run = true;
	int stopEventHandle;
	int immResetHandle;
	const nfds_t nfds = 3;
	struct pollfd fds[nfds];
	char msg_buff[256]={'\0'};
	ACE_INT32 ret;
	ACS_CC_ReturnType result;
	FMS_CPF_RTO_InfiniteSubFile* m_ISF_RTOwner = m_this->getISFOwner();

	// Initialize the pollfd structure
	ACE_OS::memset(fds, 0, sizeof(fds));

	m_this->getStopHandle(stopEventHandle);

	// Stop service handle
	fds[0].fd = stopEventHandle;
	fds[0].events = POLLIN;

	// InfiniteSubFile callback handle
	fds[1].fd = m_ISF_RTOwner->getSelObj();
	fds[1].events = POLLIN;

	m_this->getStopHandle(immResetHandle);

	// Imm Reset handle
	fds[2].fd = immResetHandle;
	fds[2].events = POLLIN;

	// waiting for IMM requests or stop
	while(thread_Run)
	{
		ret = ACE_OS::poll(fds, nfds);

		if( -1 == ret )
		{
			if(errno == EINTR)
			{
				continue;
			}
			snprintf(msg_buff,sizeof(msg_buff)-1,"ISF_UpdateCallbackHnd, exit after error=%s", strerror(errno) );
			CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
			TRACE(m_this->fms_cpf_roManTrace, "%s","ISF_UpdateCallbackHnd, exit after poll error");
			break;
		}

		// Shutdown signal
		if(fds[0].revents & POLLIN)
		{
			// Received a stop request from server
			TRACE(m_this->fms_cpf_roManTrace, "%s","ISF_UpdateCallbackHnd, received a stop request from server");
			ACE_UINT64 value;
			// Reads on pipe
			read(stopEventHandle, &value, sizeof(ACE_UINT64));
			thread_Run = false;
			break;
		}

		// Update callback
		if(fds[1].revents & POLLIN)
		{
			// Received a IMM request on an ISF
			TRACE(m_this->fms_cpf_roManTrace, "%s","ISF_UpdateCallbackHnd, received update request on a ISF");

			// lock handle usage
			ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_this->m_mutex, ;);
			result = m_ISF_RTOwner->dispatch(ACS_APGCC_DISPATCH_ONE);

			if(ACS_CC_SUCCESS != result)
			{
				snprintf(msg_buff,sizeof(msg_buff)-1, "ISF_UpdateCallbackHnd, error on ISF dispatch event" );
				CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
				TRACE(m_this->fms_cpf_roManTrace, "%s","ISF_UpdateCallbackHnd, error on ISF dispatch event");

				// Reset the run-time owner
				m_this->immOwnerReset();
				// Get the new registered handle
				fds[1].fd = m_ISF_RTOwner->getSelObj();

			}
			continue;
		}

		// Imm reset signal
		if(fds[2].revents & POLLIN)
		{
			CPF_Log.Write("ISF_UpdateCallbackHnd, IMM Bad Handle", LOG_LEVEL_ERROR);

			ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_this->m_mutex, ;);
			// Reset the run-time owner
			m_this->immOwnerReset();

			// Get the new registered handle
			fds[1].fd = m_ISF_RTOwner->getSelObj();

			continue;
		}
	}

	// Threads shutdown barrier, Block the caller until all N_THREADS count threads have called N_THREADS wait and
	// then allow all the caller threads to continue in parallel.
	m_this->getSyncObj()->wait();

	TRACE(m_this->fms_cpf_roManTrace, "%s","Leaving ISFUpdateCallbackHnd thread");
	return SUCCESS;
}

/*============================================================================
	ROUTINE: handleMessageResult
 ============================================================================ */
void FMS_CPF_InfiniteSubFiles_Manager::handleMessageResult(int result, CPF_Message_Base* message)
{
	TRACE(fms_cpf_roManTrace, "%s", "Entering in handleMessageResult()");

	if( FMS_CPF_RTO_InfiniteSubFile::OK == result )
	{
		// release the message memory
		message->release();
	}
	else if( FMS_CPF_RTO_InfiniteSubFile::Retry == result )
	{
		if(message->decreaseMessageLife() > 0 )
		{
			// re-schedule the message again
			if ( FAILURE == putq(message) )
			{
				// release the message memory
				CPF_Log.Write("ISF_Manager::svc(), putq() failed of message with Retry");
				message->release();
			}
		}
		else
		{
			// release the message memory
			CPF_Log.Write("ISF_Manager::svc(), reached end of life of a message");
			message->release();
		}
	}
	else if( FMS_CPF_RTO_InfiniteSubFile::ResetAndRetry == result )
	{
		ACE_UINT64 resetEvent = 1;
		// Signal to reset Imm runtime owner
		ACE_OS::write(m_StopEvent, &resetEvent, sizeof(ACE_UINT64));

		// re-schedule the message again
		if ( FAILURE == putq(message) )
		{
			// release the message memory
			CPF_Log.Write("ISF_Manager::svc(), putq() failed of msg with ResetAndRetry");
			message->release();
		}
	}
	TRACE(fms_cpf_roManTrace,"%s","Leaving handleMessageResult()");
}

/*============================================================================
	ROUTINE: svc
 ============================================================================ */
int FMS_CPF_InfiniteSubFiles_Manager::svc()
{
	TRACE(fms_cpf_roManTrace,"%s","Entering in svc");
	bool svc_run = true;
	int MAX_RETRY = 5;
	int num_of_retry = 0;
	int getResult;
	int operationResult;
	int msg_type;
	ACE_Message_Block* base_msg;
	CPF_CreateISF_Msg* createISF_msg;
	CPF_DeleteISF_Msg* deleteISF_msg;

	// Internal map of DNs handling
	CPF_InsertDN_Msg* insertDN_msg;
	CPF_UpdateDN_Msg* updateDN_msg;
	CPF_RemoveDN_Msg* removeDN_msg;
	CPF_ClearDNMap_Msg* clearMap_msg;

	// Update ISF state of IMM according to the data disk info
	if(m_IsMultiCP)
	{
		std::list<std::string>::const_iterator cpElement;
		for( cpElement = m_ListOfCP.begin(); cpElement != m_ListOfCP.end(); ++cpElement)
			m_RTO_InfiniteSubFile->updateInitialState( (*cpElement) );
	}
	else
	{
		std::string cpName(DEFAULT_CPNAME);
		m_RTO_InfiniteSubFile->updateInitialState(cpName);
	}

	// Start a different thread for infiniteSubFile in order to handle objects update
	ACE_Thread::spawn(ISF_UpdateCallbackHnd, (void*)this, THR_DETACHED);

	TRACE(fms_cpf_roManTrace, "%s", "svc thread, waiting for infinite subfile request or service stop");

	while(svc_run)
	{
		getResult = getq(base_msg);

		if(FAILURE == getResult)
		{
			CPF_Log.Write("FMS_CPF_InfiniteSubFiles_Manager::svc, getq of a message failed", LOG_LEVEL_ERROR);
			TRACE(fms_cpf_roManTrace, "%s", "svc, getq of a message failed");

			if(MAX_RETRY == num_of_retry)
			{
				CPF_Log.Write("FMS_CPF_InfiniteSubFiles_Manager::svc, reached the max numbero of getq failed", LOG_LEVEL_ERROR);
				svc_run = false;
			}
			num_of_retry++;
			continue;
		}

		num_of_retry = 0;

		msg_type = base_msg->msg_type();

		 // Dispatch on message type
		switch(msg_type)
		{
			case CPF_Message_Base::MT_EXIT :
			{
				// Received the termination message
				TRACE(fms_cpf_roManTrace, "%s", "svc, received EXIT message");
				// release the message memory
				base_msg->release();
				svc_run = false;
			}
			break;

			case CPF_Message_Base::MT_CREATE_ISF :
			{
				TRACE(fms_cpf_roManTrace, "%s", "svc, received CREATE ISF message");

				createISF_msg = reinterpret_cast<CPF_CreateISF_Msg*>(base_msg);

				// Retrieve message info
				CPF_CreateISF_Msg::createISFData dataISF;
				createISF_msg->getCreationData(dataISF);

				// Check if is the first creation of subfile after that a new infinite file has been created
				if(createISF_msg->isFirstISF())
				{
					// wait to be sure that the Infinite file creation was committed by IMM
					ACE_OS::sleep(1);
				}

				{
					// lock handle usage
					ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, ;);
					// Call subfile creation method
					operationResult = m_RTO_InfiniteSubFile->createISF(dataISF.subFileValue,
														  dataISF.infiniteFileName,
														  dataISF.volumeName,
														  dataISF.cpName );
				}

				// check the operation result
				handleMessageResult(operationResult, createISF_msg);
		 	}
			break;

			case CPF_Message_Base::MT_DELETE_ISF :
			{
				TRACE(fms_cpf_roManTrace, "%s", "svc, received DELETE ISF message");

				deleteISF_msg = reinterpret_cast<CPF_DeleteISF_Msg*>(base_msg);

				if(deleteISF_msg->isKnowDNofISF())
				{
				    // lock handle usage
				    ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, ;);
				    // Call subfile delete method
				    operationResult = m_RTO_InfiniteSubFile->deleteISF(deleteISF_msg->getISFDN());
				}
				else
				{
				    // Retrieve message info
				    CPF_DeleteISF_Msg::deleteISFData dataISF;
				    deleteISF_msg->getCreationData(dataISF);

				    // lock handle usage
				    ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, ;);
				    // Call subfile delete method
				    operationResult = m_RTO_InfiniteSubFile->deleteISF(dataISF.subFileValue,
                                                                                       dataISF.infiniteFileName,
                                                                                       dataISF.volumeName,
                                                                                       dataISF.cpName
                                                                                       );

				}

				// check the operation result
				handleMessageResult(operationResult, deleteISF_msg);
			}
			break;

			case CPF_Message_Base::MT_INSERT_DNENTRY :
			{
				TRACE(fms_cpf_roManTrace, "%s", "svc, received INSERT DN message");

				insertDN_msg = reinterpret_cast<CPF_InsertDN_Msg*>(base_msg);

				// Retrieve message info
				CPF_InsertDN_Msg::infoEntry dataEntry;
				insertDN_msg->getData(dataEntry);

				// Call subfile creation method
				m_RTO_InfiniteSubFile->insertMapEntry(dataEntry.fileName,
													  dataEntry.cpName,
													  dataEntry.fileDN);
				// release the message memory
				insertDN_msg->release();
			}
			break;

			case CPF_Message_Base::MT_UPDATE_DNENTRY :
			{
				TRACE(fms_cpf_roManTrace, "%s", "svc, received UPDATE DN message");

				updateDN_msg = reinterpret_cast<CPF_UpdateDN_Msg*>(base_msg);

				// Retrieve message info
				CPF_UpdateDN_Msg::infoEntry dataEntry;
				updateDN_msg->getData(dataEntry);

				// Call subfile creation method
				m_RTO_InfiniteSubFile->updateMapEntry(dataEntry.fileName,
													  dataEntry.cpName,
													  dataEntry.newFilename,
													  dataEntry.fileDN);
				// release the message memory
				updateDN_msg->release();
			}
			break;

			case CPF_Message_Base::MT_REMOVE_DNENTRY :
			{
				TRACE(fms_cpf_roManTrace, "%s", "svc, received REMOVE DN message");

				removeDN_msg = reinterpret_cast<CPF_RemoveDN_Msg*>(base_msg);

				// Retrieve message info
				CPF_RemoveDN_Msg::infoEntry dataEntry;
				removeDN_msg->getData(dataEntry);

				// Call subfile creation method
				m_RTO_InfiniteSubFile->removeMapEntry(dataEntry.fileName,
													  dataEntry.cpName );
				// release the message memory
				removeDN_msg->release();
			}
			break;

			case CPF_Message_Base::MT_CLEAR_DNMAP :
			{
				TRACE(fms_cpf_roManTrace, "%s", "svc, received CLEAR MAP message");

				clearMap_msg = reinterpret_cast<CPF_ClearDNMap_Msg*>(base_msg);

				// Call subfile creation method
				m_RTO_InfiniteSubFile->clearMap();

				// release the message memory
				clearMap_msg->release();
			}
			break;

			default:
			{
				CPF_Log.Write("FMS_CPF_InfiniteSubFiles_Manager::svc, received an unknown message", LOG_LEVEL_ERROR);
				TRACE(fms_cpf_roManTrace, "%s", "svc, received an unknown message");
			}
		}
		TRACE(fms_cpf_roManTrace, "%s", "svc, received message handled");
	}

	if( stopInternalThread() )
	{
		TRACE(fms_cpf_roManTrace, "%s", "svc, waiting on internal thread termination");
		// Threads shutdown barrier, Block the caller until all N_THREADS count threads have called N_THREADS wait and
		// then allow all the caller threads to continue in parallel.
		m_ThreadsSyncShutdown->wait();
	}
	// finalize interaction with IMM
	m_RTO_InfiniteSubFile->finalize();

	delete m_RTO_InfiniteSubFile;
	m_RTO_InfiniteSubFile = NULL;
	TRACE(fms_cpf_roManTrace,"%s","Leaving svc");
	return SUCCESS;
}

void FMS_CPF_InfiniteSubFiles_Manager::shutdown()
{
	TRACE(fms_cpf_roManTrace,"%s","Entering in shutdown()");
	CPF_Close_Msg* closeMsg = new CPF_Close_Msg();
	// Insert close message in the internal queue
	if(FAILURE != putq(closeMsg) )
	{
		TRACE(fms_cpf_roManTrace,"%s","shutdown(), wait for svc termination");
		wait();
	}
	else
	{
		closeMsg->release();
		TRACE(fms_cpf_roManTrace,"%s","shutdown(), error on put the close message");
		CPF_Log.Write("FMS_CPF_InfiniteSubFiles_Manager::shutdown, error on put the close message");
	}
	TRACE(fms_cpf_roManTrace,"%s","Leaving shutdown()");
}
