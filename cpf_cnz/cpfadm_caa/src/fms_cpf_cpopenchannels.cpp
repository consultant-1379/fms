/*
 * * @file fms_cpf_cpopenchannels.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_CpOpenChannels.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_cpopenchannels.h module
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
 *	+========+============+==============+=====================================+
 */
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_cpopenchannels.h"
#include "fms_cpf_cpchannel.h"
#include "fms_cpf_message.h"
#include "fms_cpf_common.h"

#include "ACS_DSD_Session.h"
#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include <sys/eventfd.h>

extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	ROUTINE: FMS_CPF_CpOpenChannels
 ============================================================================ */
FMS_CPF_CpOpenChannels::FMS_CPF_CpOpenChannels():
m_IsMultiCP(false),
m_serviceShutDown(false)
{
	// Define trace
	fms_cpf_OpenChsTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_CpOpenChannels");
}

/*============================================================================
	ROUTINE: removeCpChannel
 ============================================================================ */
bool FMS_CPF_CpOpenChannels::removeCpChannel( FMS_CPF_CpChannel* cpChannel)
{
	TRACE(fms_cpf_OpenChsTrace, "%s", "Entering removeCpChannel()");
	bool result = false;

	// Check the current global state
	if( m_serviceShutDown.value())
	{
		// Skip this request because all channel will be deleted.
		TRACE(fms_cpf_OpenChsTrace, "%s", "removeCpChannel(), remove channel skipped because service shutdown");
		result = true;
		return result;
	}

	CPF_RemoveChannel_Msg* removeMsg = new (std::nothrow) CPF_RemoveChannel_Msg(cpChannel);

	if(NULL != removeMsg)
	{
		if( putq(removeMsg) < 0 )
		{
			TRACE(fms_cpf_OpenChsTrace, "%s", "removeCpChannel(), failed to enqueue remove request");
			// deallocate the message
			removeMsg->release();
		}
		else
		{
			TRACE(fms_cpf_OpenChsTrace, "%s", "removeCpChannel(), request enqueued");
			result= true;
		}
	}
	TRACE(fms_cpf_OpenChsTrace, "%s", "Leaving removeCpChannel()");
	return result;
}

/*============================================================================
	ROUTINE: deleteCpChannel
 ============================================================================ */
bool FMS_CPF_CpOpenChannels::deleteCpChannel(FMS_CPF_CpChannel* cpChannel)
{
	TRACE(fms_cpf_OpenChsTrace, "%s", "Entering deleteCpChannel()");
	bool found = false;

	std::list<FMS_CPF_CpChannel*>::iterator index;
	{
		// Lock the list usage in exclusive write mode
		ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex, guard, m_mutex, false);

		// Find the channel in the list
		for(index = m_channelsList.begin(); index != m_channelsList.end(); ++index )
		{
			// check it
			if( cpChannel == (*index) )
			{
				found = true;
				// remove the entry in the list
				m_channelsList.erase(index);
				// if this method is not called, ACE and the OS won't reclaim the thread stack and exit status
				//of a joinable thread, and the program will leak memory.
				cpChannel->wait();

				// delete cpchannel, Reclaim the task memory
				delete cpChannel;
				break;
			}
		}
	}
	// check if found or not
	if(!found)
	{
		// This must be never happen, in any case never say never.
		TRACE(fms_cpf_OpenChsTrace, "%s", "deleteCpChannel(), channel not found");
		CPF_Log.Write("FMS_CPF_CpOpenChannels::deleteCpChannel, channel not found", LOG_LEVEL_ERROR);

		cpChannel->wait();
		// Delete it
		delete cpChannel;
	}

	TRACE(fms_cpf_OpenChsTrace, "%s", "Leaving deleteCpChannel()");
	return found;
}

/*============================================================================
	ROUTINE: putMsgToCpChannel
 ============================================================================ */
bool FMS_CPF_CpOpenChannels::putMsgToCpChannel(FMS_CPF_CpChannel* cpChannel, ACE_Message_Block* message)
{
	TRACE(fms_cpf_OpenChsTrace, "%s", "Entering putMsgToCpChannel()");
	bool result = false;
	bool found = false;
	std::list<FMS_CPF_CpChannel*>::iterator index;
	{
		// Lock the list usage in exclusive write mode
		ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex, guard, m_mutex, false);

		// Find the channel in the list
		for(index = m_channelsList.begin(); index != m_channelsList.end(); ++index )
		{
			// check it
			if( cpChannel == (*index) )
			{
				found = true;
				// found the channel, it is still live
				int putResult = cpChannel->putq(message);
				if(FAILURE != putResult)
				{
					TRACE(fms_cpf_OpenChsTrace, "%s", "putMsgToCpChannel(), message queued");
					result = true;
				}
				else
				{
					TRACE(fms_cpf_OpenChsTrace, "%s", "putMsgToCpChannel(), failed to put a message");
					CPF_Log.Write("FMS_CPF_CpOpenChannels::putMsgToCpChannel, failed to put a message", LOG_LEVEL_ERROR);
				}
				break;
			}
		}
	}
	if(!found)
	{
		TRACE(fms_cpf_OpenChsTrace, "%s", "putMsgToCpChannel(), channel not found");
		CPF_Log.Write("FMS_CPF_CpOpenChannels::putMsgToCpChannel, channel not found", LOG_LEVEL_ERROR);
	}
	TRACE(fms_cpf_OpenChsTrace, "%s", "Leaving putMsgToCpChannel()");
	return result;

}
/*============================================================================
	ROUTINE: addCpChannel
 ============================================================================ */
bool FMS_CPF_CpOpenChannels::addCpChannel( ACS_DSD_Session* dsdSession )
{
	bool result = false;
	int  openResult;

	TRACE(fms_cpf_OpenChsTrace, "%s", "Entering addCpChannel()");

	FMS_CPF_CpChannel* cpChannel = NULL;
	cpChannel = new (std::nothrow) FMS_CPF_CpChannel(dsdSession, m_IsMultiCP);

	if(NULL == cpChannel)
	{
		TRACE(fms_cpf_OpenChsTrace, "%s", "addCpChannel(), failed to create FMS_CPF_CpChannel object");
		CPF_Log.Write("FMS_CPF_CpOpenChannels::addCpChannel, failed to create FMS_CPF_CpChannel object", LOG_LEVEL_ERROR);
		//Close the opened stream
		dsdSession->close();
		// delete the dsd session
		delete dsdSession;
	}

	// start the new CP channel thread
	openResult = cpChannel->open();

	// check operation result
	if( FAILURE == openResult )
	{
		// some error on thread start-up
		TRACE(fms_cpf_OpenChsTrace, "%s", "addCpChannel(), error on CP channel thread start");
		CPF_Log.Write("FMS_CPF_CpOpenChannels::addCpChannel, error on CP channel thread start", LOG_LEVEL_ERROR);
		// Deallocate CP channel object, it will delete also DSD session
		delete cpChannel;
	}
	else
	{
		TRACE(fms_cpf_OpenChsTrace, "%s", "addCpChannel(), CP channel thread started");
		result = true;
		// Lock the list usage in exclusive write mode
		ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex, guard, m_mutex, false );
		// inserts the new channel pointer into list,
		m_channelsList.push_back(cpChannel);
	}

	TRACE(fms_cpf_OpenChsTrace, "%s", "Leaving addCpChannel()");
	return result;
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
int FMS_CPF_CpOpenChannels::open(void *args)
{
	TRACE(fms_cpf_OpenChsTrace, "%s", "Entering open()");
	int result = FAILURE;
	// To avoid warning about unused parameter
	UNUSED(args);
	m_serviceShutDown = false;
	// Start the thread to handler channel deletion
	result = activate();

	if( FAILURE == result )
	{
		CPF_Log.Write("FMS_CPF_CpOpenChannels::open, error on start svc thread", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_OpenChsTrace, "%s", "Leaving open(), error on start svc thread");
	}

	TRACE(fms_cpf_OpenChsTrace, "%s", "Leaving open()");
	return result;
}

/*============================================================================
	ROUTINE: svc
 ============================================================================ */
int FMS_CPF_CpOpenChannels::svc()
{
	TRACE(fms_cpf_OpenChsTrace,"%s","Entering in svc");
	bool svc_run = true;
	int MAX_RETRY = 5;
	int num_of_retry = 0;
	int getResult;
	int msg_type;
	ACE_Message_Block* base_msg;
	CPF_RemoveChannel_Msg* removeChannel_Msg;

	TRACE(fms_cpf_OpenChsTrace, "%s", "svc thread, waiting for Cp channel remove request");

	while(svc_run)
	{
		getResult = getq(base_msg);

		if(FAILURE == getResult)
		{
			CPF_Log.Write("FMS_CPF_CpOpenChannels::svc, getq of a message failed", LOG_LEVEL_ERROR);
			TRACE(fms_cpf_OpenChsTrace, "%s", "svc, getq of a message failed");

			if(MAX_RETRY == num_of_retry)
			{
				CPF_Log.Write("FMS_CPF_CpOpenChannels::svc, reached the max number of getq failed", LOG_LEVEL_ERROR);
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
				TRACE(fms_cpf_OpenChsTrace, "%s", "svc, received EXIT message");
				// release the message memory
				base_msg->release();
				svc_run = false;
			}
			break;

			case CPF_Message_Base::MT_REMOVE_CHANNEL:
			{
				TRACE(fms_cpf_OpenChsTrace, "%s", "svc, received REMOVE CHANNEL message");

				removeChannel_Msg = reinterpret_cast<CPF_RemoveChannel_Msg*>(base_msg);

				if( !deleteCpChannel(removeChannel_Msg->getCpChannel()))
				{
					// some error occurs
					TRACE(fms_cpf_OpenChsTrace, "%s", "svc, REMOVE CHANNEL failed");
					CPF_Log.Write("FMS_CPF_CpOpenChannels::svc, REMOVE CHANNEL failed", LOG_LEVEL_ERROR);
				}

				// release the message memory
				removeChannel_Msg->release();
			}
			break;

			default:
			{
				CPF_Log.Write("FMS_CPF_CpOpenChannels::svc, received an unknown message", LOG_LEVEL_ERROR);
				TRACE(fms_cpf_OpenChsTrace, "%s", "svc, received an unknown message");
			}
		}
	}

	CPF_Log.Write("FMS_CPF_CpOpenChannels,  svc thread terminated", LOG_LEVEL_ERROR);
	TRACE(fms_cpf_OpenChsTrace, "%s", "Leaving svc");

	return SUCCESS;
}
/*============================================================================
	ROUTINE: channelsShutDown
 ============================================================================ */
void FMS_CPF_CpOpenChannels::channelsShutDown()
{
	TRACE(fms_cpf_OpenChsTrace, "%s", "Entering channelsShutDown()");

	// Set the global shutdown flag, after that any remove request will be skipped
	m_serviceShutDown = true;

	CPF_Close_Msg* closeMsg = new CPF_Close_Msg();

	// Insert close message in the internal queue
	if(FAILURE != putq(closeMsg) )
	{
		TRACE(fms_cpf_OpenChsTrace,"%s","channelsShutDown(), wait for svc termination");
		this->wait();
		TRACE(fms_cpf_OpenChsTrace,"%s","channelsShutDown(), svc has been terminated");
	}
	else
	{
		closeMsg->release();
		TRACE(fms_cpf_OpenChsTrace,"%s","channelsShutDown(), error on put the close message");
		CPF_Log.Write("FMS_CPF_CpOpenChannels::channelsShutDown, error on put the close message");
	}

	FMS_CPF_CpChannel* cpChannel;
	std::list<FMS_CPF_CpChannel*>::iterator index;
	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, guard, m_mutex);

		// Signal to every Channel to exit
		for(index = m_channelsList.begin(); index != m_channelsList.end(); ++index )
		{
			cpChannel = (*index);
			// channel shutdown
			cpChannel->channelShutDown();
		}
		// deallocate all channels
		for(index = m_channelsList.begin(); index != m_channelsList.end(); ++index )
		{
			cpChannel = (*index);
			// wait on  scv thread termination
			cpChannel->wait();
			// Reclaim the task memory
			delete cpChannel;
		}

		// clear the list;
		m_channelsList.clear();
	}

	TRACE(fms_cpf_OpenChsTrace, "%s", "Leaving channelsShutDown()");
}

/*============================================================================
	ROUTINE: ~FMS_CPF_CpOpenChannels
 ============================================================================ */
FMS_CPF_CpOpenChannels::~FMS_CPF_CpOpenChannels()
{
	if(NULL != fms_cpf_OpenChsTrace)
			delete fms_cpf_OpenChsTrace;
}
