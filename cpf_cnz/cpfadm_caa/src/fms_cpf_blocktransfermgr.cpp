/*
 * * @file fms_cpf_blocktransfermgr.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_BlockTransferMgr.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_blocktransfermgr.h module
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
#include "fms_cpf_blocktransfermgr.h"
#include "fms_cpf_blocksender_scheduler.h"
#include "fms_cpf_common.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"


extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	METHOD: addBlockSender
 ============================================================================ */
bool FMS_CPF_BlockTransferMgr::addBlockSender(const std::string& fileName, const std::string& volumeName,
											  const std::string& cpName, const int blockSize, const std::string& subFilePath )
{
	TRACE(m_trace, "Entering in %s()", __func__);
	bool result = false;
	// critical section
	ACE_GUARD_RETURN(ACE_RW_Thread_Mutex, guard, m_mutex, result);

	FMS_CPF_BlockSender_Scheduler* newScheduler = NULL;
	std::map<std::string, FMS_CPF_BlockSender_Scheduler*>::const_iterator element;

	element = m_SchedulersMap.find(fileName);

	if(m_SchedulersMap.end() == element)
	{
		TRACE(m_trace, "%s(), create scheduler for file:<%s>", __func__, fileName.c_str() );
		// Create a scheduler for this CP File
		newScheduler = new (std::nothrow) FMS_CPF_BlockSender_Scheduler(fileName, m_IsMultiCP);

		// Start the scheduler worker thread
		if( NULL != newScheduler )
		{
			if( (FAILURE != newScheduler->open()) && newScheduler->addBlockSender(cpName, volumeName, subFilePath, blockSize) )
			{
				// Add the new scheduler to the map
				m_SchedulersMap.insert(std::make_pair(fileName, newScheduler));
				result = true;
			}
			else
			{
				char errorMsg[512] = {'\0'};
				// Stop the scheduler thread
				newScheduler->shutDown();
				// Wait on worker thread termination
				// if this method is not called, ACE and the OS won't reclaim the thread stack and exit status
				// of a joinable thread, and the program will leak memory.
				newScheduler->wait();

				ACE_OS::snprintf(errorMsg, 511, "%s(), scheduler init failed for file:<%s>, Cp:<%s>", __func__, fileName.c_str(), cpName.c_str() );
				TRACE(m_trace, "%s", errorMsg);
				CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);

				// Reclaim the task memory
				delete newScheduler;
			}
		}
		else
		{
			char errorMsg[512] = {'\0'};
			ACE_OS::snprintf(errorMsg, 511, "%s(), scheduler creation failed for file:<%s>, Cp:<%s>", __func__, fileName.c_str(), cpName.c_str() );
			TRACE(m_trace, "%s", errorMsg);
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		}
	}
	else
	{
		// Scheduler already created for the CP file
		// Add a new block sender to the scheduler
		TRACE(m_trace, "%s(), found scheduler for file:<%s>", __func__, fileName.c_str() );
		result = element->second->addBlockSender(cpName, volumeName, subFilePath, blockSize);
	}

	TRACE(m_trace, "Leaving %s(), result:<%s>", __func__, (result ? "OK": "NOT OK"));
	return result;
}

/*============================================================================
	METHOD: updateBlockSenderState
 ============================================================================ */
void FMS_CPF_BlockTransferMgr::updateBlockSenderState(const std::string& fileName, const std::string& cpName,
													  const unsigned long lastReadySubfile)
{
	TRACE(m_trace, "Leaving  %s()", __func__);

	// critical section
	ACE_GUARD(ACE_RW_Thread_Mutex, guard, m_mutex);

	// Get scheduler for this File
	std::map<std::string, FMS_CPF_BlockSender_Scheduler*>::const_iterator element;
	element = m_SchedulersMap.find(fileName);

	if(m_SchedulersMap.end() != element)
	{
		// Scheduler found, update the current active subfile
		element->second->updateBlockSenderState(cpName, lastReadySubfile);
	}
	else
	{
		// Error handling
		char errorMsg[512] = {'\0'};
		ACE_OS::snprintf(errorMsg, 511, "%s(), scheduler not found for file:<%s>, Cp:<%s>", __func__, fileName.c_str(), cpName.c_str() );
		TRACE(m_trace, "%s", errorMsg);
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
	}
	TRACE(m_trace, "Leaving %s()", __func__);
}

/*============================================================================
	METHOD: removeBlockSender
 ============================================================================ */
void FMS_CPF_BlockTransferMgr::removeBlockSender(const std::string& fileName, const std::string& cpName)
{
	TRACE(m_trace, "Entering in %s()", __func__);

	// Get scheduler for this File
	std::map<std::string, FMS_CPF_BlockSender_Scheduler*>::iterator element;
	element = m_SchedulersMap.find(fileName);

	if(m_SchedulersMap.end() != element)
	{
		// Scheduler found
		if(m_IsMultiCP)
		{
			// Remove only sender of the specific CP
			element->second->removeBlockSender(cpName);
		}
		else
		{
			TRACE(m_trace, "%s(), remove scheduler of file:<%s>", __func__, fileName.c_str());
			// critical section
			ACE_GUARD(ACE_RW_Thread_Mutex, guard, m_mutex);

			// remove the scheduler
			// Stop the scheduler thread
			element->second->shutDown();
			// wait on  scv thread termination
			element->second->wait();

			// Reclaim the scheduler object memory
			delete element->second;

			// remove element from the map
			m_SchedulersMap.erase(element);
		}
	}
	else
	{
		// Error handling
		char errorMsg[128] = {'\0'};
		ACE_OS::snprintf(errorMsg, 127, "%s(), scheduler not found for file:<%s>, Cp:<%s>", __func__, fileName.c_str(), cpName.c_str() );
		TRACE(m_trace, "%s", errorMsg);
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
	}
	TRACE(m_trace, "Leaving %s()", __func__);
}

/*============================================================================
	METHOD: shutDown
 ============================================================================ */
void FMS_CPF_BlockTransferMgr::shutDown()
{
	TRACE(m_trace, "Entering in %s()", __func__);
	std::map<std::string, FMS_CPF_BlockSender_Scheduler*>::const_iterator element;

	// critical section
	{
		ACE_GUARD(ACE_RW_Thread_Mutex, guard, m_mutex);

		// Signal to every Scheduler to exit
		for( element = m_SchedulersMap.begin(); element != m_SchedulersMap.end(); ++element )
		{
			// Stop the scheduler thread
			element->second->shutDown();
		}

		// deallocate all channels
		for( element = m_SchedulersMap.begin(); element != m_SchedulersMap.end(); ++element )
		{
			// wait on  scv thread termination
			element->second->wait();
			// Reclaim the scheduler object memory
			delete element->second;
		}

		// clear the map
		m_SchedulersMap.clear();
	}

	TRACE(m_trace, "Leaving %s()", __func__);
}

/*============================================================================
	METHOD: FMS_CPF_BlockTransferMgr
 ============================================================================ */
FMS_CPF_BlockTransferMgr::FMS_CPF_BlockTransferMgr()
: m_IsMultiCP(false),
  m_SchedulersMap(),
  m_mutex(),
  m_trace(new (std::nothrow) ACS_TRA_trace("FMS_CPF_BlockTransferMgr"))
{

}

/*============================================================================
	METHOD: ~FMS_CPF_BlockTransferMgr
 ============================================================================ */
FMS_CPF_BlockTransferMgr::~FMS_CPF_BlockTransferMgr()
{
	if(NULL != m_trace)
		delete m_trace;
}
