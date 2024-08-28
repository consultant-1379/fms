/*
 * * @file fms_cpf_tqfilechecker.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_FileTQChecker.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_tqfilechecker.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-03-13
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
 *	| 1.0.0  | 2012-03-13 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_filetqchecker.h"
#include "fms_cpf_common.h"
#include "fms_cpf_exception.h"

#include "ACS_TRA_trace.h"
#include <ace/Guard_T.h>

/*============================================================================
	METHOD: FMS_CPF_FileTQChecker
 ============================================================================ */
FMS_CPF_FileTQChecker::FMS_CPF_FileTQChecker()
: AES_OHI_ExtFileHandler2(OHI_USERSPACE::SUBSYS, OHI_USERSPACE::APPNAME)
{
	m_connectionStatus = false;
	m_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_FileTQChecker");
}

/*============================================================================
	METHOD: validateTQ
 ============================================================================ */
bool FMS_CPF_FileTQChecker::validateTQ(const std::string& tqName, unsigned int& errorCode)
{
	TRACE(m_trace, "Entering in validateTQ(<%s>)", tqName.c_str());
	bool tqFound = false;
	ACE_GUARD_RETURN(ACE_RW_Thread_Mutex, guard, m_mutex, tqFound);

	// Check if not already connected
	if(!m_connectionStatus)
	{
		// connect to the AFP server
		errorCode = attach();
		m_connectionStatus = (AES_OHI_NOERRORCODE == errorCode);
		TRACE(m_trace, "validateTQ(), connection to AFP server: <%s>, errorCode:<%d>", (m_connectionStatus ? "OK" : "NOT OK"), errorCode );
	}

	// validate the file TQ
	if(m_connectionStatus)
	{
		errorCode = fileTransferQueueDefined(tqName.c_str());

		TRACE(m_trace, "validateTQ(), File TQ check returns errorCode:<%d>", errorCode );

		tqFound = (AES_OHI_NOERRORCODE == errorCode);
		// Some error occurs
		if(!tqFound && (AES_OHI_TQNOTFOUND != errorCode) )
		{
			detach();
			m_connectionStatus = false;
		}
	}

	TRACE(m_trace, "Leaving validateTQ(), File TQ:<%s>, error code:<%d>", (tqFound ? "FOUND" : "NOT FOUND"), errorCode);
	return tqFound;
}

/*============================================================================
	METHOD: validateTQ
 ============================================================================ */
bool FMS_CPF_FileTQChecker::validateTQ(const std::string& tqName, std::string& tqDN, unsigned int& errorCode)
{
	TRACE(m_trace, "Entering in validateTQ(<%s>)", tqName.c_str());
	bool tqFound = false;
	ACE_GUARD_RETURN(ACE_RW_Thread_Mutex, guard, m_mutex, tqFound);

	// Check if not already connected
	if(!m_connectionStatus)
	{
		// connect to the AFP server
		errorCode = attach();
		m_connectionStatus = (AES_OHI_NOERRORCODE == errorCode);
		TRACE(m_trace, "validateTQ(), connection to AFP server: <%s>, errorCode:<%d>", (m_connectionStatus ? "OK" : "NOT OK"), errorCode );
	}

	// validate the file TQ
	if(m_connectionStatus)
	{
		errorCode = fileTransferQueueDefined(tqName.c_str(), tqDN);

		TRACE(m_trace, "validateTQ(), File TQ check returns errorCode:<%d>", errorCode );

		tqFound = (AES_OHI_NOERRORCODE == errorCode);
		// Some error occurs
		if(!tqFound && (AES_OHI_TQNOTFOUND != errorCode) )
		{
			detach();
			m_connectionStatus = false;
		}
	}

	TRACE(m_trace, "Leaving validateTQ(), File TQ:<%s>, error code:<%d>", (tqFound ? "FOUND" : "NOT FOUND"), errorCode);
	return tqFound;
}

/*============================================================================
	METHOD: removeSentFile
 ============================================================================ */
unsigned int FMS_CPF_FileTQChecker::removeSentFile(const std::string& tqName, const std::string& fileName)
{
	TRACE(m_trace, "%s", "Entering in removeSentFile");

	unsigned int errorCode = AES_OHI_EXECUTEERROR;
	ACE_GUARD_RETURN(ACE_RW_Thread_Mutex, guard, m_mutex, errorCode);

	// Check if not already connected
	if(!m_connectionStatus)
	{
		// connect to the AFP server
		errorCode = attach();
		m_connectionStatus = (AES_OHI_NOERRORCODE == errorCode);
		TRACE(m_trace, "removeSentFile(), connection to AFP server: <%s>, errorCode:<%d>", (m_connectionStatus ? "OK" : "NOT OK"), errorCode );
	}

	// remove file from TQ
	if(m_connectionStatus)
	{
		TRACE(m_trace, "removeSentFile(), TQ:<%s>, file:<%s>", tqName.c_str(), fileName.c_str() );
		errorCode = removeFile(tqName.c_str(), fileName.c_str());
	}

	TRACE(m_trace, "Leaving removeSentFile(), error code:<%d>", errorCode);
	return errorCode;
}

/*============================================================================
	METHOD: handleEvent
 ============================================================================ */
unsigned int FMS_CPF_FileTQChecker::handleEvent(AES_OHI_Eventcodes eventCode)
{
	if(AES_OHI_EVELOSTSERVER == eventCode)
	{
		TRACE(m_trace, "%s", "handleEvent(), connection to AFP server lost");
		// Lost server connection
		m_connectionStatus = false;
	}
	TRACE(m_trace, "%s", "Leaving handleEvent()");
	return AES_OHI_NOERRORCODE;
}

/*============================================================================
	METHOD: ~FMS_CPF_FileTQChecker
 ============================================================================ */
FMS_CPF_FileTQChecker::~FMS_CPF_FileTQChecker()
{
	if(m_connectionStatus)
		detach();

	if(NULL != m_trace)
		delete m_trace;
}
