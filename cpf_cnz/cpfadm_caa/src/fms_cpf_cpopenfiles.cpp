/*
 * * @file fms_cpf_cpopenfiles.cpp
 *	@brief
 *	Class method implementation for CPDOpenFiles.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_cpopenfiles.h module
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
#include "fms_cpf_cpopenfiles.h"
#include "fms_cpf_cpdfilethrd.h"
#include "fms_cpf_filelock.h"
#include "fms_cpf_common.h"
#include "fms_cpf_oc_buffermgr.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"


extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	ROUTINE: CPDOpenFiles
 ============================================================================ */
CPDOpenFiles::CPDOpenFiles()
{
	fms_cpf_OpenFilesTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_CPDOpenFiles");
}

/*============================================================================
	ROUTINE: ~CPDOpenFiles
 ============================================================================ */
CPDOpenFiles::~CPDOpenFiles()
{
	// delete trace object
	if(NULL != fms_cpf_OpenFilesTrace)
		delete fms_cpf_OpenFilesTrace;

}

/*============================================================================
	ROUTINE: lookup
 ============================================================================ */
CPDFileThrd* CPDOpenFiles::lookup(const uint32_t fileRef) throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_OpenFilesTrace, "%s", "Entering in lookup()");

	FileMapType::iterator msgSideElement;
	//Critical section
	{
		// Lock the map usage in exclusive mode
		ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_mutex, NULL);
		msgSideElement = m_OpenFiles.find(fileRef);
	}
	// check if found
	if(m_OpenFiles.end() == msgSideElement)
	{
		char errorMsg[512] = {'\0'};
		ACE_OS::snprintf(errorMsg, 511, "CPDOpenFiles::lookup() with file reference<%d> failed", fileRef);
		TRACE(fms_cpf_OpenFilesTrace, "%s", errorMsg);
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		throw FMS_CPF_PrivateException(FMS_CPF_Exception::INVALIDREF, errorMsg);
	}

	TRACE(fms_cpf_OpenFilesTrace, "Leaving lookup(), found data for reference<%d>", fileRef );

	return ((*msgSideElement).second).fileThrd;
}

/*============================================================================
	ROUTINE: lockOpen
 ============================================================================ */
FileLock* CPDOpenFiles::lockOpen(const std::string fileId, const std::string& cpName)
{
	TRACE(fms_cpf_OpenFilesTrace, "%s", "Entering in lockOpen()");

	FileLockListType::iterator element;
	FileLock* openLock = NULL;
	//Critical section
	{
		// Lock the map usage in exclusive mode
		ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_mutex, NULL);
		FileLock* fileLock;
		TRACE(fms_cpf_OpenFilesTrace, "lockOpen(), file to lock<%s>", fileId.c_str());
		for(element = m_OpenLocks.begin(); element != m_OpenLocks.end(); ++element)
		{
			fileLock = *element;
			if(fileId.compare(fileLock->getFileName()) == 0)
			{
				// FileLock object found on this file
				// check the cp name
				if(cpName.empty() || (cpName.compare(fileLock->getCpName()) == 0) )
				{
					openLock = fileLock;
					TRACE(fms_cpf_OpenFilesTrace, "%s", "lockOpen(), file lock found");
					break;
				}
			}
		 }

		 // FileLock object not found, new CP file opening
		 if(NULL == openLock)
		 {
			 TRACE(fms_cpf_OpenFilesTrace, "%s", "lockOpen(), file lock not found, create it");
			 openLock = new FileLock(fileId, cpName);
			 m_OpenLocks.push_back(openLock);
		 }

		 // Increase the number of user that want lock file access
		 openLock->increaseUsers();
		 TRACE(fms_cpf_OpenFilesTrace, "lockOpen(), acquire file lock, current users<%d>", openLock->getNumOfUsers());
	} // Critical section end

	// Acquire the lock file access
	if (NULL != openLock)
		openLock->lock();

	TRACE(fms_cpf_OpenFilesTrace, "%s", "Leaving lockOpen(), lock acquired");
	return openLock;
}

/*============================================================================
	ROUTINE: lockOpen
 ============================================================================ */
void CPDOpenFiles::unlockOpen(FileLock* openLock)
{
	TRACE(fms_cpf_OpenFilesTrace, "%s", "Entering in unlockOpen()");
	FileLockListType::iterator element;

	// Check if valid
	if(NULL != openLock )
	{
		//Critical section
		// Lock the map usage in exclusive mode
		ACE_GUARD(ACE_Thread_Mutex, guard, m_mutex);

		TRACE(fms_cpf_OpenFilesTrace, "%s", "unlockOpen(), file lock release");

		// Release the lock file access
		openLock->unlock();

		// Decrease the number of user that want lock file access
		openLock->decreaseUsers();
		TRACE(fms_cpf_OpenFilesTrace, "lockOpen(), file lock, current users<%d>", openLock->getNumOfUsers());


		if( openLock->getNumOfUsers() <=  0)
		{
			for(element = m_OpenLocks.begin(); element != m_OpenLocks.end(); ++element)
			{
				if(openLock == *element)
				{
					m_OpenLocks.remove(openLock);
					delete openLock;
					break;
				}
			}
			openLock = NULL;
		}
	}
	TRACE(fms_cpf_OpenFilesTrace, "%s", "Leaving unlockOpen()");
}

/*============================================================================
	ROUTINE: insert
 ============================================================================ */
void CPDOpenFiles::insert(const uint32_t fileRef, CPDFileThrd* fThrd, const std::string& cpName, bool cpSide)
{
	TRACE(fms_cpf_OpenFilesTrace, "%s", "Entering in insert()");

	std::pair<FileMapType::iterator, bool> insertResult;
	
	//Critical section
	{
		// Lock the map usage in exclusive mode
		ACE_GUARD(ACE_Thread_Mutex, guard, m_mutex);

		OcBufferMgr::instance()->clearFileRefs(cpName, cpSide);

		TRACE(fms_cpf_OpenFilesTrace, "insert(), current opened files:<%zd>, cpSide<%s>", m_OpenFiles.size(), (cpSide?"EX":"SB") );

		MsgSide messageSide;
		messageSide.cpSide = cpSide;
		messageSide.fileThrd = fThrd;
	
 	    insertResult = m_OpenFiles.insert(FileMapType::value_type(fileRef, messageSide));

		if(!insertResult.second)
		{
			char errorMsg[128]={'\0'};

			FileMapType::iterator element = m_OpenFiles.find(fileRef);

			if(m_OpenFiles.end() != element)
			{
				ACE_OS::snprintf(errorMsg, 127, "CPDOpenFiles::insert() Failed, duplicate keys, file ref<%d> already present", fileRef);
				CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
				TRACE(fms_cpf_OpenFilesTrace, "%s", errorMsg);
			}
			else
			{
				ACE_OS::snprintf(errorMsg, 127, "CPDOpenFiles::insert() Failed, file ref<%d> ", fileRef);
				CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
				TRACE(fms_cpf_OpenFilesTrace, "%s", errorMsg);
			}

		}
		else
		{
			TRACE(fms_cpf_OpenFilesTrace,"insert(), file reference<%d> inserted", fileRef );
		}
	}
    TRACE(fms_cpf_OpenFilesTrace, "%s", "Leaving insert()");
}

/*============================================================================
	ROUTINE: remove
 ============================================================================ */
void CPDOpenFiles::remove(const uint32_t fileRef, bool notRemoveReference)
{
	TRACE(fms_cpf_OpenFilesTrace, "%s", "Entering in remove()");
	FileMapType::iterator element;

	//Critical section
	{
		// Lock the map usage in exclusive mode
		ACE_GUARD(ACE_Thread_Mutex, guard, m_mutex);
	
		// find the element to remove
		element = m_OpenFiles.find(fileRef);
		TRACE(fms_cpf_OpenFilesTrace, "remove(), current opened files:<%zd>", m_OpenFiles.size() );

		// Check if found
		if(m_OpenFiles.end() == element)
		{
			// Avoid log for reference null
			if(0 != fileRef)
			{
				// element not found
				char errorMsg[128] = {'\0'};
				ACE_OS::snprintf(errorMsg, 127, "CPDOpenFiles::remove(), file ref<%d> not found", fileRef);
				CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
			}
			TRACE(fms_cpf_OpenFilesTrace, "remove(), file ref<%d> not found", fileRef);
		}
		else
		{
			// element found
			TRACE(fms_cpf_OpenFilesTrace, "CPDOpenFiles::remove(), file ref:<%d> found", fileRef);

			CPDFileThrd* fileThrd = element->second.fileThrd;

			if(!notRemoveReference)
			{
				fileThrd->removeFileRef();
			}
			// remove from internal map
			m_OpenFiles.erase(element);
		}
	}
	TRACE(fms_cpf_OpenFilesTrace, "%s", "Leaving remove()");
}

/*============================================================================
	ROUTINE: removeAll
 ============================================================================ */
void CPDOpenFiles::removeAll(bool cpSide)
{
	TRACE(fms_cpf_OpenFilesTrace, "%s", "Entering in removeAll()");
	char logMsg[64]={'\0'};

	//SYNC. Files should be closed before returning
	//Critical section
	{
		// Lock the map usage in exclusive mode
		ACE_GUARD(ACE_Thread_Mutex, guard, m_mutex);
		ACE_OS::snprintf(logMsg, 63, "removeAll(), CpSide:<%s> open files:<%zd>", cpSide ? "EX":"SB", m_OpenFiles.size() );

		TRACE(fms_cpf_OpenFilesTrace, "%s", logMsg);
		CPF_Log.Write(logMsg, LOG_LEVEL_INFO);

		CPDFileThrd* fileThrd;
		FileMapType::iterator element;

		element = m_OpenFiles.begin();
		while( element != m_OpenFiles.end())
		{
			if(element->second.cpSide == cpSide)
			{
				TRACE(fms_cpf_OpenFilesTrace, "removeAll(), found file reference<%d>", element->first);
				fileThrd = element->second.fileThrd;

				// Terminate the cpdfile thread
				fileThrd->setOnSync();
				fileThrd->shutDown();

				// Remove element from the map
				m_OpenFiles.erase(element++);
			}
			else
				++element;
		}
	}// critical section end
	TRACE(fms_cpf_OpenFilesTrace, "%s", "Leaving removeAll()");
}

/*============================================================================
	ROUTINE: shutDown
 ============================================================================ */
void CPDOpenFiles::shutDown()
{
	TRACE(fms_cpf_OpenFilesTrace, "%s", "Entering in shutDown()");
	//Critical section
	{
		// Lock the map usage in exclusive mode
		ACE_GUARD(ACE_Thread_Mutex, guard, m_mutex);

		CPDFileThrd* fileThrd;
		FileMapType::iterator element;

		element = m_OpenFiles.begin();

		for(element = m_OpenFiles.begin(); element != m_OpenFiles.end(); ++element)
		{
			fileThrd = element->second.fileThrd;
			// Service shutdown
			fileThrd->shutDown(true);
		}
		m_OpenFiles.clear();
	}// critical section end
	TRACE(fms_cpf_OpenFilesTrace, "%s", "Leaving shutDown()");
}

