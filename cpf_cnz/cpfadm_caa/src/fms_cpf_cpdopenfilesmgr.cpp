/*
 * * @file fms_cpf_cpdopenfilesmgr.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_CpdOpenFilesMgr.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_cpdopenfilesmgr.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-18
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
 *	| 1.0.0  | 2011-11-18 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 *	| 1.1.0  | 2011-11-18 | qvincon      | ACE introduced.                     |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_cpdopenfilesmgr.h"
#include "fms_cpf_cpopenfiles.h"
#include "fms_cpf_cpdfilethrd.h"
#include "fms_cpf_filelock.h"
#include "fms_cpf_common.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	ROUTINE: FMS_CPF_CpdOpenFilesMgr
 ============================================================================ */
FMS_CPF_CpdOpenFilesMgr::FMS_CPF_CpdOpenFilesMgr()
{
	fms_cpf_OpensMgrTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_CpdOpenFilesMgr");
}

/*============================================================================
	ROUTINE: ~FMS_CPF_CpdOpenFilesMgr
 ============================================================================ */
FMS_CPF_CpdOpenFilesMgr::~FMS_CPF_CpdOpenFilesMgr()
{
	cpdOpenFilesMapType::iterator element;

	for(element =m_CpdOpenFilesTable.begin(); element != m_CpdOpenFilesTable.end(); ++element )
	{
		delete (*element).second;
	}

	m_CpdOpenFilesTable.clear();

	// delete trace object
	if(NULL != fms_cpf_OpensMgrTrace)
		delete fms_cpf_OpensMgrTrace;
}

/*============================================================================
	ROUTINE: getCPDOpenFilesForCP
 ============================================================================ */
CPDOpenFiles* FMS_CPF_CpdOpenFilesMgr::getCPDOpenFilesForCP(const std::string cpName)
{
	TRACE(fms_cpf_OpensMgrTrace, "%s", "Entering in getCPDOpenFilesForCP()");
	std::string cpDefName;
	CPDOpenFiles* cpdOpenFiles = NULL;

	if( cpName.empty() )
	{
		cpDefName = DEFAULT_CPNAME;
	}
	else
	{
		cpDefName = cpName;
	}
	// Critical section
	{
		// Lock the map usage in exclusive mode
		ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_mutex, NULL);

		// Search for cp name in table
		cpdOpenFilesMapType::iterator element = m_CpdOpenFilesTable.find(cpDefName);

		// Check find result
		if( m_CpdOpenFilesTable.end() != element )
		{
			// Element already present into the map
			// get pointer
			cpdOpenFiles = (*element).second;
			TRACE(fms_cpf_OpensMgrTrace, "%s", "getCPDOpenFilesForCP(), element found");
		}
		else
		{
			TRACE(fms_cpf_OpensMgrTrace, "%s", "getCPDOpenFilesForCP(), element not found, add it");
			// Element not present into the map
			// Create the CPDOpenFiles object for this CP
			cpdOpenFiles = new (std::nothrow) CPDOpenFiles();
			char logMsg[521] ={'\0'};

			if(NULL != cpdOpenFiles)
			{
				// Insert the new element into the map
				std::pair<cpdOpenFilesMapType::iterator, bool> result;
				result = m_CpdOpenFilesTable.insert(cpdOpenFilesMapType::value_type(cpDefName, cpdOpenFiles));

				ACE_OS::snprintf(logMsg, 511, "getCPDOpenFilesForCP(), CPDOpenFiles object for CP<%s> insert result:<%d>", cpDefName.c_str(), result.second);
				CPF_Log.Write(logMsg, LOG_LEVEL_ERROR);
				TRACE(fms_cpf_OpensMgrTrace, "%s", logMsg);
			}
			else
			{
				ACE_OS::snprintf(logMsg, 511, "getCPDOpenFilesForCP(), error to allocate a new CPDOpenFiles object for CP<%s>", cpDefName.c_str());
				TRACE(fms_cpf_OpensMgrTrace, "%s", logMsg);
				CPF_Log.Write(logMsg, LOG_LEVEL_ERROR);
			}
		}
	}// end critical section
	TRACE(fms_cpf_OpensMgrTrace, "%s", "Leaving getCPDOpenFilesForCP()");
	return cpdOpenFiles;
}

/*============================================================================
	ROUTINE: lookup
 ============================================================================ */
CPDFileThrd* FMS_CPF_CpdOpenFilesMgr::lookup(const uint32_t fileRef, const std::string& cpName)
{
	TRACE(fms_cpf_OpensMgrTrace, "%s", "Entering in lookup()");
	CPDOpenFiles* openFiles = NULL;
	CPDFileThrd* fileThrd = NULL;

	openFiles = getCPDOpenFilesForCP(cpName);
	if( NULL != openFiles )
	{
		fileThrd = openFiles->lookup(fileRef);
	}
	else
	{
		TRACE(fms_cpf_OpensMgrTrace, "%s", "lookup(), failed to get CPDOpenFiles object");
		CPF_Log.Write("FMS_CPF_CpdOpenFilesMgr::lookup(), failed to get CPDOpenFiles object", LOG_LEVEL_ERROR);
	}

	TRACE(fms_cpf_OpensMgrTrace, "%s", "Leaving lookup()");
	return fileThrd;
 }

/*============================================================================
	ROUTINE: lockOpen
 ============================================================================ */
FileLock* FMS_CPF_CpdOpenFilesMgr::lockOpen(const std::string fileId, const std::string& cpName)
{
	TRACE(fms_cpf_OpensMgrTrace, "%s", "Entering in lockOpen()");
	CPDOpenFiles* openFiles= NULL;
	FileLock* lock = NULL;

	openFiles = getCPDOpenFilesForCP(cpName);

	if( NULL != openFiles )
	{
		lock = openFiles->lockOpen(fileId, cpName);
	}
	else
	{
		TRACE(fms_cpf_OpensMgrTrace, "%s", "lockOpen(), failed to get CPDOpenFiles object");
		CPF_Log.Write("FMS_CPF_CpdOpenFilesMgr::lockOpen(), failed to get CPDOpenFiles object", LOG_LEVEL_ERROR);
	}
	TRACE(fms_cpf_OpensMgrTrace, "%s", "Leaving lockOpen()");
	return lock;
}

/*============================================================================
	ROUTINE: unlockOpen
 ============================================================================ */
void FMS_CPF_CpdOpenFilesMgr::unlockOpen(FileLock* openLock, const std::string& cpName)
{
	TRACE(fms_cpf_OpensMgrTrace, "%s", "Entering in unlockOpen()");
	CPDOpenFiles* openFiles= NULL;

	openFiles = getCPDOpenFilesForCP(cpName);

	if( NULL != openFiles )
	{
		openFiles->unlockOpen(openLock);
	}
	else
	{
		TRACE(fms_cpf_OpensMgrTrace, "%s", "unlockOpen(), failed to get CPDOpenFiles object");
		CPF_Log.Write("FMS_CPF_CpdOpenFilesMgr::unlockOpen(), failed to get CPDOpenFiles object", LOG_LEVEL_ERROR);
	}
	TRACE(fms_cpf_OpensMgrTrace, "%s", "Leaving unlockOpen()");
}

/*============================================================================
	ROUTINE: insert
 ============================================================================ */
void FMS_CPF_CpdOpenFilesMgr::insert(const uint32_t fileRef, CPDFileThrd* fThrd,
										const std::string& cpName, bool cpSide)
{
	TRACE(fms_cpf_OpensMgrTrace, "%s", "Entering in insert()");
	CPDOpenFiles* openFiles= NULL;

	openFiles = getCPDOpenFilesForCP(cpName);

	if( NULL != openFiles )
	{
		openFiles->insert(fileRef, fThrd, cpName, cpSide);
	}
	else
	{
		TRACE(fms_cpf_OpensMgrTrace, "%s", "insert(), failed to get CPDOpenFiles object");
		CPF_Log.Write("FMS_CPF_CpdOpenFilesMgr::insert(), failed to get CPDOpenFiles object", LOG_LEVEL_ERROR);
	}

	TRACE(fms_cpf_OpensMgrTrace, "%s", "Leaving insert()");
}

/*============================================================================
	ROUTINE: remove
 ============================================================================ */
void FMS_CPF_CpdOpenFilesMgr::remove(const uint32_t fileRef, bool notRemoveReference, const std::string& cpName)
{
	TRACE(fms_cpf_OpensMgrTrace, "%s", "Entering in remove()");
	CPDOpenFiles* openFiles= NULL;

	openFiles = getCPDOpenFilesForCP(cpName);

	if( NULL != openFiles )
	{
		openFiles->remove(fileRef, notRemoveReference);
	}
	else
	{
		TRACE(fms_cpf_OpensMgrTrace, "%s", "remove(), failed to get CPDOpenFiles object");
		CPF_Log.Write("FMS_CPF_CpdOpenFilesMgr::remove(), failed to get CPDOpenFiles object", LOG_LEVEL_ERROR);
	}

	TRACE(fms_cpf_OpensMgrTrace, "%s", "Leaving remove()");
}

/*============================================================================
	ROUTINE: remove
 ============================================================================ */
void FMS_CPF_CpdOpenFilesMgr::removeAll(bool cpSide, const std::string& cpName)
{
	TRACE(fms_cpf_OpensMgrTrace, "%s", "Entering in removeAll()");
	CPDOpenFiles* openFiles= NULL;

	openFiles = getCPDOpenFilesForCP(cpName);

	if( NULL != openFiles )
	{
		openFiles->removeAll(cpSide);
	}
	else
	{
		TRACE(fms_cpf_OpensMgrTrace, "%s", "removeAll(), failed to get CPDOpenFiles object");
		CPF_Log.Write("FMS_CPF_CpdOpenFilesMgr::removeAll(), failed to get CPDOpenFiles object", LOG_LEVEL_ERROR);
	}

	TRACE(fms_cpf_OpensMgrTrace, "%s", "Leaving removeAll()");
}

/*============================================================================
	ROUTINE: shutDown
 ============================================================================ */
void FMS_CPF_CpdOpenFilesMgr::shutDown()
{
	TRACE(fms_cpf_OpensMgrTrace, "%s", "Entering in shutDown()");
	// Critical section
	{
		// Lock the map usage in exclusive mode
		ACE_GUARD(ACE_Thread_Mutex, guard, m_mutex);
		TRACE(fms_cpf_OpensMgrTrace, "shutDown(), there are <%zd> elements to shutdown", m_CpdOpenFilesTable.size());
		// Search for cp name in table
		cpdOpenFilesMapType::iterator element;
		// Propagate the shutdown to all cpdopenfiles object
		for(element = m_CpdOpenFilesTable.begin(); element != m_CpdOpenFilesTable.end(); ++element)
		{
			element->second->shutDown();
		}
	}
	TRACE(fms_cpf_OpensMgrTrace, "%s", "Leaving shutDown()");
}
