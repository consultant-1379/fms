/*
 * * @file fms_cpf_blocksender_scheduler.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_BlockSender_Scheduler.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_blocksender_scheduler.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2013-06-18
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
 *	| 1.0.0  | 2013-06-18 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_blocksender_scheduler.h"
#include "fms_cpf_blocksender.h"
#include "fms_cpf_message.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_common.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include <time.h>
#include <sys/time.h>

extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	METHOD: FMS_CPF_BlockSender_Scheduler
 ============================================================================ */
FMS_CPF_BlockSender_Scheduler::FMS_CPF_BlockSender_Scheduler(const std::string& fileName, bool isMultiCP)
: m_IsMultiCP(isMultiCP),
  m_FileName(fileName),
  m_BlockSendersMap(),
  m_ShutDown(false),
  m_CpTimeSlice(0),
  m_mutex(),
  m_DataReady(0),
  m_trace(new (std::nothrow) ACS_TRA_trace("FMS_CPF_BlockSender_Scheduler"))
{


}

/*============================================================================
	~METHOD: FMS_CPF_BlockSender_Scheduler
 ============================================================================ */
FMS_CPF_BlockSender_Scheduler::~FMS_CPF_BlockSender_Scheduler()
{
  	if(NULL != m_trace)
		delete m_trace;
}

/*============================================================================
	METHOD: svc
 ============================================================================ */
int FMS_CPF_BlockSender_Scheduler::svc()
{
	TRACE(m_trace, "Entering in %s()", __func__);

	ACE_Message_Block* base_msg;
	CPF_BlockSend_Msg* sendMsg;
	int getResult;

	if(m_IsMultiCP)
	{
		// MCP block transfer
		TRACE(m_trace, "%s(), starting block send in MCP", __func__);

		const time_t secOnWait = 1U;

		while(!m_ShutDown)
		{
			ACE_Time_Value timeout(ACE_OS::time(NULL) + secOnWait);
			// Wait for send request of some CP/BC
			m_DataReady.acquire(timeout);

			// get a send request of some CP/BC
			getResult = getq(base_msg);

			if(getResult >= 0)
			{
				 // Check request type
				 if(base_msg->msg_type() == CPF_Message_Base::MT_SEND_BLOCK)
				 {
					 // Refresh Cp Time slice since it can be change
					 m_CpTimeSlice = ParameterHndl::instance()->getCpTimeSlice();

					 sendMsg = reinterpret_cast<CPF_BlockSend_Msg*>(base_msg);

					 // Send all data for the current Time slice
					 if( handle_Blade_BlockTransfer(sendMsg->getCpName()) )
					 {
						 // Put in the queue to re-schedule again it later
						 if( putq(sendMsg) == FAILURE)
						 {
							 // Error on re-scheduling
							 char errorMsg[128] = {'\0'};
							 ACE_OS::snprintf(errorMsg, 127, "%s(), block sender re-schedule failed for file:<%s> of Cp:<%s>", __func__, m_FileName.c_str(), sendMsg->getCpName() );
							 TRACE(m_trace, "%s", errorMsg);
							 CPF_Log.Write(errorMsg, LOG_LEVEL_FATAL);
							 sendMsg->release();
						 }
					 }
					 else
					 {
						 TRACE(m_trace, "%s, removed sender of file:<%s>, Cp:<%s>", __func__, m_FileName.c_str(), sendMsg->getCpName() );
						 // Sender has been removed
						 // Release the send message
						 sendMsg->release();
					 }
				 }
			}
		}

		TRACE(m_trace, "%s(), block send in MCP terminated", __func__);
	}
	else
	{
		// SCP block transfer
		TRACE(m_trace, "%s(), waiting to start block send in SCP", __func__);

		getResult = getq(base_msg);

		if(getResult >= 0)
		{
			 // Check request type
			 if(base_msg->msg_type() == CPF_Message_Base::MT_SEND_BLOCK)
			 {
				 TRACE(m_trace, "%s(), starting block send in SCP", __func__);

				 // Start block send on SCP
				 sendMsg = reinterpret_cast<CPF_BlockSend_Msg*>(base_msg);
				 sendMsg->release();

				 // This method returns only on shutdown/remove request
				 handle_SingleCP_BlockTransfer();
			 }
		}

		TRACE(m_trace, "%s(), block send in SCP terminated", __func__);
	}

	// Delete all block senders
	cleanup();

	TRACE(m_trace, "Leaving  %s()", __func__);
	return SUCCESS;
}

/*============================================================================
	METHOD: open
 ============================================================================ */
int FMS_CPF_BlockSender_Scheduler::open(void *args)
{
	TRACE(m_trace, "Entering in %s()", __func__);
	//To avoid unused warning
	UNUSED(args);
	int result = activate();

	TRACE(m_trace, "Leaving  %s()", __func__);
	return result;
}

/*============================================================================
	METHOD: close
 ============================================================================ */
int FMS_CPF_BlockSender_Scheduler::close(u_long flags)
{
	TRACE(m_trace, "Entering in %s()", __func__);
	//To avoid unused warning
	UNUSED(flags);

	// Erase all queued messages
	msg_queue_->flush();

	TRACE(m_trace, "Leaving  %s()", __func__);
	return SUCCESS;
}

/*============================================================================
	METHOD: addBlockSender
 ============================================================================ */
bool FMS_CPF_BlockSender_Scheduler::addBlockSender(const std::string& cpName, const std::string& volumeName, const std::string& subFilePath, const int blockSize)
{
	TRACE(m_trace, "Entering in %s()", __func__);
	bool result = false;
	// critical section
	ACE_GUARD_RETURN(ACE_RW_Thread_Mutex, guard, m_mutex, result)

	// Create a block sender to handling sending of block of file belongs cpName
	FMS_CPF_BlockSender* newBlockSender = new (std::nothrow) FMS_CPF_BlockSender(m_FileName, volumeName, cpName, subFilePath, blockSize, m_IsMultiCP );

	if(NULL != newBlockSender)
	{
		// Insert the new block sender into the map <CpName, BlockSender>
		std::pair<std::map<std::string, FMS_CPF_BlockSender*>::iterator,bool> insertResult;

		insertResult = m_BlockSendersMap.insert(std::make_pair(cpName, newBlockSender ));

		// Check insert result
		if(insertResult.second)
		{
			TRACE(m_trace, "%s(), add a new block sender for CP:<%s> ", __func__, cpName.c_str());
			result = true;

			CPF_BlockSend_Msg* sendTask = new (std::nothrow) CPF_BlockSend_Msg(cpName);

			if( (NULL != sendTask) && (FAILURE != putq(sendTask)) )
			{
				TRACE(m_trace, "%s(), add new task for Cp:<%s>", __func__, cpName.c_str());
			}
		}
		else
		{
			delete newBlockSender;
			char errorMsg[512] = {'\0'};
			ACE_OS::snprintf(errorMsg, 511, "%s(), block sender insert failed for file:<%s> of Cp:<%s>", __func__, m_FileName.c_str(), cpName.c_str() );
			TRACE(m_trace, "%s", errorMsg);
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		}
	}
	else
	{
		char errorMsg[512] = {'\0'};
		ACE_OS::snprintf(errorMsg, 511, "%s(), block sender creation failed for file:<%s> of Cp:<%s>", __func__, m_FileName.c_str(), cpName.c_str() );
		TRACE(m_trace, "%s", errorMsg);
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
	}

	TRACE(m_trace, "Leaving %s(), result:<%s>", __func__, (result ? "OK": "NOT OK"));
	return result;
}

/*============================================================================
	METHOD: removeBlockSender
 ============================================================================ */
bool FMS_CPF_BlockSender_Scheduler::removeBlockSender(const std::string& cpName)
{
	TRACE(m_trace, "Entering in %s()", __func__);
	bool result = false;

	// Get the related block sender
	std::map<std::string, FMS_CPF_BlockSender*>::const_iterator element;
	element = m_BlockSendersMap.find(cpName);
	if( m_BlockSendersMap.end() != element )
	{
		 TRACE(m_trace, "%s(), remove sender of file:<%s>, Cp:<%s>", __func__, m_FileName.c_str(), cpName.c_str() );
		 element->second->setRemoveState();
	}

	TRACE(m_trace, "Leaving %s()", __func__);
	return result;
}

/*============================================================================
	METHOD: shutDown
 ============================================================================ */
void FMS_CPF_BlockSender_Scheduler::shutDown()
{
	TRACE(m_trace, "Entering in %s()", __func__);
	m_ShutDown = true;
	// Deactivate the queue and wakeup all threads waiting on the queue so they can continue
	msg_queue_->deactivate();
	m_DataReady.release();

	TRACE(m_trace, "Leaving %s()", __func__);
}

/*============================================================================
	METHOD: updateBlockSenderState
 ============================================================================ */
void FMS_CPF_BlockSender_Scheduler::updateBlockSenderState(const std::string& cpName, const unsigned long lastReadySubfile)
{
	TRACE(m_trace, "Entering in %s()", __func__);

	std::map<std::string, FMS_CPF_BlockSender*>::const_iterator element;

	if(m_IsMultiCP)
	{
		// Multi-CP set the last ready subfile directly on sender
		element = m_BlockSendersMap.find(cpName);

		if( m_BlockSendersMap.end() != element )
		{
			 TRACE(m_trace, "%s(), current active subfile:<%d> of file:<%s>, Cp:<%s>", __func__, lastReadySubfile, m_FileName.c_str(), cpName.c_str() );
			 element->second->setActiveSubFile(lastReadySubfile);

			 // Increment the semaphore by 1 to signal to svc thread that there are data to send
			 m_DataReady.release();
		}
		else
		{
			char errorMsg[128] = {'\0'};
			ACE_OS::snprintf(errorMsg, 127, "%s(), not found sender, update failed for file:<%s> of Cp:<%s>", __func__, m_FileName.c_str(), cpName.c_str() );
			TRACE(m_trace, "%s", errorMsg);
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		}
	}
	else
	{
		element = m_BlockSendersMap.find(std::string(DEFAULT_CPNAME));
		if( m_BlockSendersMap.end() != element )
		{
			 TRACE(m_trace, "%s(), current active subfile:<%d> of file:<%s>, Cp:<%s>", __func__, lastReadySubfile, m_FileName.c_str(), cpName.c_str() );
			 element->second->setActiveSubFile(lastReadySubfile);

			 if(msg_queue_->is_empty())
			 {
				 CPF_BlockUpdate_Msg* updateMsg = new (std::nothrow) CPF_BlockUpdate_Msg();

				 if( (NULL != updateMsg) && (FAILURE != putq(updateMsg)) )
				 {
				 	 TRACE(m_trace, "%s(), update active subFile:<%d>, Cp:<%s>", __func__, lastReadySubfile, cpName.c_str());
				 }
				 else
				 {
					 char errorMsg[512] = {'\0'};
					 ACE_OS::snprintf(errorMsg, 511, "%s(), update failed for file:<%s>", __func__, m_FileName.c_str());
					 TRACE(m_trace, "%s", errorMsg);
					 CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
				 }
			 }
		}
		else
		{
			char errorMsg[128] = {'\0'};
			ACE_OS::snprintf(errorMsg, 127, "%s(), not found sender, update failed for file:<%s> of Cp:<%s>", __func__, m_FileName.c_str(), cpName.c_str() );
			TRACE(m_trace, "%s", errorMsg);
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		}

	}

	TRACE(m_trace, "Leaving %s()", __func__);
}

/*============================================================================
	METHOD: cleanup
 ============================================================================ */
void FMS_CPF_BlockSender_Scheduler::cleanup()
{
	TRACE(m_trace, "Entering in %s()", __func__);
	std::map<std::string, FMS_CPF_BlockSender*>::const_iterator element;

	// critical section
	{
		ACE_GUARD(ACE_RW_Thread_Mutex, guard, m_mutex);

		// deallocate all channels
		for( element = m_BlockSendersMap.begin(); element != m_BlockSendersMap.end(); ++element )
		{
			// Reclaim the scheduler object memory
			delete element->second;
		}

		// clear the map
		m_BlockSendersMap.clear();
	}

	TRACE(m_trace, "Leaving %s()", __func__);
}

/*============================================================================
	METHOD: handle_SingleCP_BlockTransfer
 ============================================================================ */
void FMS_CPF_BlockSender_Scheduler::handle_SingleCP_BlockTransfer()
{
	TRACE(m_trace, "Entering in %s()", __func__);
	std::map<std::string, FMS_CPF_BlockSender*>::const_iterator element;
	element = m_BlockSendersMap.find(std::string(DEFAULT_CPNAME));

	if( m_BlockSendersMap.end() != element )
	{
		TRACE(m_trace, "%s(), starting sending loop", __func__);
		ACE_Message_Block* base_msg;
		int getResult;

		FMS_CPF_BlockSender::sendResult result = FMS_CPF_BlockSender::DATAEXIST;
		const int waitOnTqError = 100U;
		// Looping
		while(!m_ShutDown)
		{
			// check action to do
			if(FMS_CPF_BlockSender::DATAEXIST == result)
			{
				// there are other available data to report
				// try to send available data
				result = element->second->sendDataOnSCP(m_ShutDown);
				continue;
			}
			else if (FMS_CPF_BlockSender::TQERROR == result )
			{
				// try again to send
				result = FMS_CPF_BlockSender::DATAEXIST;
				// Some error returned from TQ,
				// wait a moment and re-try
				timespec waitOnError;
				waitOnError.tv_sec = 0U;
				waitOnError.tv_nsec = waitOnTqError * stdValue::NanoSecondsInMilliSecond;
				nanosleep(&waitOnError, NULL);
				continue;
			}

			// No more data to send
			// wait for update or shutdown
			getResult = getq(base_msg);

			if(getResult >= 0)
			{
				 // Check request type
				 if(base_msg->msg_type() == CPF_Message_Base::MT_UPDATE_BLOCK)
				 {
					 // Start new data send
					 CPF_BlockUpdate_Msg* updateMsg = reinterpret_cast<CPF_BlockUpdate_Msg*>(base_msg);
					 result = FMS_CPF_BlockSender::DATAEXIST;
					 updateMsg->release();
				 }
			}
		}
		TRACE(m_trace, "%s(), sending loop terminated", __func__);
	}
	else
	{
		TRACE(m_trace, "%s(), not found sender!", __func__);
	}

	TRACE(m_trace, "Leaving %s()", __func__);
}

/*============================================================================
	METHOD: handle_Blade_BlockTransfer
 ============================================================================ */
bool FMS_CPF_BlockSender_Scheduler::handle_Blade_BlockTransfer(const std::string& cpName)
{
	TRACE(m_trace, "Entering in %s(%s)", __func__, cpName.c_str());
	bool rescheduleMe = true;

	// Get the related block sender
	std::map<std::string, FMS_CPF_BlockSender*>::const_iterator element;
	element = m_BlockSendersMap.find(cpName);

	if( m_BlockSendersMap.end() != element )
	{
		// check if the sender has been removed
		bool isToRemove;
		element->second->getRemoveState(isToRemove);
		if( isToRemove )
		{
			// Sender must be removed
			delete element->second;
			m_BlockSendersMap.erase(cpName);
			rescheduleMe = false;
		}
		else
		{
			// Send file data
			TRACE(m_trace, "%s(), starting sending loop, Cp:<%s>", __func__, cpName.c_str());

			// try to send available data
			element->second->sendData(m_CpTimeSlice);

			TRACE(m_trace, "%s(), terminate sending loop, Cp:<%s>", __func__, cpName.c_str());
		}
	}
	else
	{
		TRACE(m_trace, "%s(), not found sender for Cp:<%s>", __func__, cpName.c_str());
	}
	TRACE(m_trace, "Leaving %s()", __func__);

	return rescheduleMe;
}
