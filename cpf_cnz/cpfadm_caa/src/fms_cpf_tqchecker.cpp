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
 *  | 1.1.0  | 2013-06-13 | qvincon      | Block transfer handling             |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_tqchecker.h"
#include "fms_cpf_filetqchecker.h"
#include "fms_cpf_common.h"
#include "fms_cpf_exception.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"
#include "ACS_APGCC_CommonLib.h"

#include "aes_ohi_blockhandler2.h"

extern ACS_TRA_Logging CPF_Log;

namespace {
	const char TQVALIDCHARS[] = "_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	const char FileMTQRootParameter[]= "sourceDataForCpFile";
}
/*============================================================================
	METHOD: FMS_CPF_TQChecker
 ============================================================================ */
FMS_CPF_TQChecker::FMS_CPF_TQChecker()
{
	m_fileTQChk = NULL;
	m_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_TQChecker");
}

/*============================================================================
	METHOD: validateBlockTQ
 ============================================================================ */
bool FMS_CPF_TQChecker::validateBlockTQ(const std::string& tqName, int& errorCode)
{
	TRACE(m_trace, "%s", "Entering in validateBlockTQ()");
	bool blockTQFound = false;
	errorCode = static_cast<int>(FMS_CPF_Exception::TQNOTFOUND);

	// Validate block TQ
	unsigned int ohiError = AES_OHI_BlockHandler2::blockTransferQueueDefined(tqName.c_str());

	if( AES_OHI_NOERRORCODE == ohiError )
	{
		errorCode = static_cast<int>(FMS_CPF_Exception::OK);
		blockTQFound = true;
	}

	TRACE(m_trace, "Leaving validateBlockTQ(), Block TQ:<%s> is <%s>, OHI result:<%d>", tqName.c_str(),
										(blockTQFound ? "OK" :"NOT OK"), ohiError);
	return blockTQFound;
}

/*============================================================================
	METHOD: validateBlockTQ
 ============================================================================ */
bool FMS_CPF_TQChecker::validateBlockTQ(const std::string& tqName, std::string& tqDN, int& errorCode)
{
	TRACE(m_trace, "%s", "Entering in validateBlockTQ()");
	bool blockTQFound = false;
	errorCode = static_cast<int>(FMS_CPF_Exception::TQNOTFOUND);

	// Validate block TQ
	unsigned int ohiError = AES_OHI_BlockHandler2::blockTransferQueueDefined(tqName.c_str(), tqDN);

	if( AES_OHI_NOERRORCODE == ohiError )
	{
		errorCode = static_cast<int>(FMS_CPF_Exception::OK);
		blockTQFound = true;
	}

	TRACE(m_trace, "Leaving validateBlockTQ(), Block TQ:<%s> is <%s> DN:<%s>, OHI result:<%d>", tqName.c_str(),
										(blockTQFound ? "OK" :"NOT OK"), tqDN.c_str(), ohiError);
	return blockTQFound;
}

/*============================================================================
	METHOD: validateFileTQ
 ============================================================================ */
bool FMS_CPF_TQChecker::validateFileTQ(const std::string& tqName, int& errorCode)
{
	TRACE(m_trace, "%s", "Entering in validateFileTQ()");
	bool fileTQFound = false;

	// Check name correctness
	if(!checkTQName(tqName))
	{
		// name not valid
		errorCode = static_cast<int>(FMS_CPF_Exception::INVTQNAME);
	}
	else
	{
		// critical section
		ACE_GUARD_RETURN(ACE_RW_Thread_Mutex, guard, m_mutex, fileTQFound);

		if(NULL == m_fileTQChk)
		{
			// allocate the file TQ checker
			m_fileTQChk = new (std::nothrow) FMS_CPF_FileTQChecker();
		}

		// verify the TQ
		if(NULL != m_fileTQChk)
		{
			unsigned int ohiError;

			fileTQFound = m_fileTQChk->validateTQ(tqName, ohiError );

			switch(ohiError)
			{
				case AES_OHI_NOERRORCODE:
				{
					errorCode = static_cast<int>(FMS_CPF_Exception::OK);
					break;
				}

				case AES_OHI_TQNOTFOUND :
				{
					errorCode = static_cast<int>(FMS_CPF_Exception::TQNOTFOUND);
					break;
				}

				case AES_OHI_NOPROCORDER:
				{
					errorCode = static_cast<int>(FMS_CPF_Exception::TQNOTFOUND);
					delete m_fileTQChk;
					m_fileTQChk = NULL;
					break;
				}
				// In all other case return always the same error code
				case AES_OHI_NOSERVERACCESS:
				{
					delete m_fileTQChk;
					m_fileTQChk = NULL;
				}
				default:
					errorCode = static_cast<int>(FMS_CPF_Exception::TQNOTFOUND);
			}
		}
		else
		{
			CPF_Log.Write("FMS_CPF_TQChecker::validateFileTQ(), memory allocation error", LOG_LEVEL_ERROR);
			errorCode = static_cast<int>(FMS_CPF_Exception::INTERNALERROR);
		}
	}
	TRACE(m_trace, "%s", "Leaving validateFileTQ()");
	return fileTQFound;
}

/*============================================================================
	METHOD: validateFileTQ
 ============================================================================ */
bool FMS_CPF_TQChecker::validateFileTQ(const std::string& tqName, std::string& tqDN, int& errorCode)
{
	TRACE(m_trace, "%s", "Entering in validateFileTQ()");
	bool fileTQFound = false;

	// Check name correctness
	if(!checkTQName(tqName))
	{
		// name not valid
		errorCode = static_cast<int>(FMS_CPF_Exception::INVTQNAME);
	}
	else
	{
		// critical section
		ACE_GUARD_RETURN(ACE_RW_Thread_Mutex, guard, m_mutex, fileTQFound);

		if(NULL == m_fileTQChk)
		{
			// allocate the file TQ checker
			m_fileTQChk = new (std::nothrow) FMS_CPF_FileTQChecker();
		}

		// verify the TQ
		if(NULL != m_fileTQChk)
		{
			unsigned int ohiError;

			fileTQFound = m_fileTQChk->validateTQ(tqName, tqDN, ohiError );

			switch(ohiError)
			{
				case AES_OHI_NOERRORCODE:
				{
					TRACE(m_trace, "Valid TQ, DN:<%s>", tqDN.c_str());
					errorCode = static_cast<int>(FMS_CPF_Exception::OK);
					break;
				}

				case AES_OHI_TQNOTFOUND :
				{
					errorCode = static_cast<int>(FMS_CPF_Exception::TQNOTFOUND);
					break;
				}

				case AES_OHI_NOPROCORDER:
				{
					errorCode = static_cast<int>(FMS_CPF_Exception::TQNOTFOUND);
					delete m_fileTQChk;
					m_fileTQChk = NULL;
					break;
				}
				// In all other case return always the same error code
				case AES_OHI_NOSERVERACCESS:
				{
					delete m_fileTQChk;
					m_fileTQChk = NULL;
				}
				default:
					errorCode = static_cast<int>(FMS_CPF_Exception::TQNOTFOUND);
			}
		}
		else
		{
			CPF_Log.Write("FMS_CPF_TQChecker::validateFileTQ(), memory allocation error", LOG_LEVEL_ERROR);
			errorCode = static_cast<int>(FMS_CPF_Exception::INTERNALERROR);
		}
	}
	TRACE(m_trace, "%s", "Leaving validateFileTQ()");
	return fileTQFound;
}

/*============================================================================
	METHOD: removeSentFile
 ============================================================================ */
unsigned int FMS_CPF_TQChecker::removeSentFile(const std::string& tqName, const std::string& fileName)
{
	TRACE(m_trace, "%s", "Entering in removeSentFile()");
	unsigned int errorCode = AES_OHI_EXECUTEERROR;
	// critical section
	ACE_GUARD_RETURN(ACE_RW_Thread_Mutex, guard, m_mutex, errorCode);

	if(NULL == m_fileTQChk)
	{
		// allocate the file TQ checker
		m_fileTQChk = new (std::nothrow) FMS_CPF_FileTQChecker();
	}

	// verify the TQ
	if(NULL != m_fileTQChk)
	{
		errorCode = m_fileTQChk->removeSentFile(tqName, fileName);
	}

	TRACE(m_trace, "Leaving removeSentFile(), errorCode:<%d>", errorCode);
	return errorCode;
}
/*============================================================================
	METHOD: checkTQName
 ============================================================================ */
bool FMS_CPF_TQChecker::checkTQName(const std::string& tqName)
{
	TRACE(m_trace, "%s", "Entering in checkTQName()");
	bool validName = false;

	std::string tmpFileTQ(tqName);

	ACS_APGCC::toUpper(tmpFileTQ);

	if( !tmpFileTQ.empty() && ( (tmpFileTQ[0] >= 'A') && (tmpFileTQ[0] <= 'Z') ) )
	{
		size_t notValidPos = tmpFileTQ.find_first_not_of(TQVALIDCHARS);
		validName = (std::string::npos == notValidPos);
	}

	TRACE(m_trace, "Leaving checkTQName(), TQ name:<%s> is <%s>", tqName.c_str(), (validName ? "OK" : "NOT OK") );
	return validName;
}

/*============================================================================
	METHOD: createTQFolder
 ============================================================================ */
bool FMS_CPF_TQChecker::createTQFolder(const std::string& tqName, std::string& folderPath )
{
	TRACE(m_trace, "%s", "Entering in createTQFolder()");

	bool result = getFileMTQPath(folderPath);

	// get the transfer file queue root
	if(result)
	{
		folderPath += tqName;
		result = createFolder(folderPath);
	}

	TRACE(m_trace, "Leaving createTQFolder(): result:<%s>", (result ? "OK" : "NOT OK") );
	return result;
}

/*============================================================================
	METHOD: createTQFolder
 ============================================================================ */
bool FMS_CPF_TQChecker::createTQFolder(const std::string& tqName )
{
	TRACE(m_trace, "%s", "Entering in createTQFolder()");

	// assemble the TQ folder
	std::string tqFolder;

	bool result = getFileMTQPath(tqFolder);

	if(result)
	{
		tqFolder += tqName;
		result = createFolder(tqFolder);
	}
	TRACE(m_trace, "Leaving createTQFolder(): result:<%s>", (result ? "OK" : "NOT OK") );
	return result;
}

/*============================================================================
	METHOD: getFileMTQPath
 ============================================================================ */
bool FMS_CPF_TQChecker::getFileMTQPath(std::string& fileTQRoot)
{
	TRACE(m_trace, "%s", "Entering in getFileMTQPath()");
	bool result = true;

	// Check if already get
	if(m_fileTQRoot.empty())
	{
		// Gets mutual exclusion on retrieval operation
		ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, ;);
		// Check condition again after that it acquired the mutual exclusion
		if(m_fileTQRoot.empty())
		{
			result = false;
			ACS_APGCC_DNFPath_ReturnTypeT getResult;
			ACS_APGCC_CommonLib fileMPathHandler;
			int bufferLength = 512;
			char buffer[bufferLength];

			ACE_OS::memset(buffer, 0, bufferLength);
			// get the physical path
			getResult = fileMPathHandler.GetFileMPath(FileMTQRootParameter, buffer, bufferLength);

			if(ACS_APGCC_DNFPATH_SUCCESS == getResult)
			{
				// path get successful
				m_fileTQRoot = buffer;
				result = true;
			}
			else if(ACS_APGCC_STRING_BUFFER_SMALL == getResult)
			{
				// Buffer too small, but now we have the right size
				char buffer2[bufferLength+1];
				ACE_OS::memset(buffer2, 0, bufferLength+1);
				// try again to get
				getResult = fileMPathHandler.GetFileMPath(FileMTQRootParameter, buffer2, bufferLength);

				// Check if it now is ok
				if(ACS_APGCC_DNFPATH_SUCCESS == getResult)
				{
					// path get successful now
					m_fileTQRoot = buffer2;
					result = true;
				}
			}

			// In any case log the result
			char logBuffer[512] = {'\0'};

			if(result)
			{
				m_fileTQRoot += DirDelim;
				// path retrieved log it
				snprintf(logBuffer, 511, "FMS_CPF_TQChecker::getFileMTQPath(),  path of FileM:<%s> is <%s>", FileMTQRootParameter, m_fileTQRoot.c_str() );
				CPF_Log.Write(logBuffer, LOG_LEVEL_INFO);
			}
			else
			{
				// error occurs log it
				snprintf(logBuffer, 511, "FMS_CPF_TQChecker::getFileMTQPath(), error:<%d> on get path of FileM:<%s>", static_cast<int>(getResult), FileMTQRootParameter );
				CPF_Log.Write(logBuffer, LOG_LEVEL_ERROR);
			}
			TRACE(m_trace, "%s", logBuffer);
		}
	}

	fileTQRoot = m_fileTQRoot;
	TRACE(m_trace, "%s", "Leaving getFileMTQPath()");
	return result;
}

/*============================================================================
	METHOD: createFolder
 ============================================================================ */
bool FMS_CPF_TQChecker::createFolder(const std::string& folderPath)
{
	TRACE(m_trace, "%s", "Entering in createFolder()");
	bool result = true;
	ACE_stat folderInfo;
	int dirStatus = ACE_OS::stat(folderPath.c_str(), &folderInfo);

	// check if already exist
	if( (FAILURE == dirStatus) && (errno == ENOENT) )
	{
		// Set permission (RWXRW-RW-) on folder

                //TR HV44755 Added execute permission to GRP	        
        	mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IWOTH;  
		ACE_OS::umask(0);

		//create TQ folder
		if(ACS_APGCC::create_directories(folderPath.c_str(), mode) !=  1 )
		{
			result = false;
			// Get the system error
			char errorText[256]={'\0'};
			std::string errorDetail(strerror_r(errno, errorText, 255));
			// log the error
			char logBuffer[512] = {'\0'};
			snprintf(logBuffer, 511, "FMS_CPF_TQChecker::createFolder, error:<%s> on folder:<%s> creation", errorDetail.c_str(), folderPath.c_str() );
			CPF_Log.Write(logBuffer, LOG_LEVEL_ERROR);
			TRACE(m_trace, "createFolder(), failed TQ folder:<%s> creation", folderPath.c_str());
		}
	}
	else
	{
		// Check if a folder or file
		if(!S_ISDIR(folderInfo.st_mode))
		{
			// Not a folder
			result = false;
			char logBuffer[512] = {'\0'};
			snprintf(logBuffer, 511, "FMS_CPF_TQChecker::createFolder, exists a file with the folder:<%s> name", folderPath.c_str() );
			CPF_Log.Write(logBuffer, LOG_LEVEL_ERROR);
		}
	}

	TRACE(m_trace, "Leaving createFolder(): result:<%s>", (result ? "OK" : "NOT OK") );
	return result;
}

/*============================================================================
	METHOD: ~FMS_CPF_TQChecker
 ============================================================================ */
FMS_CPF_TQChecker::~FMS_CPF_TQChecker()
{
	if(NULL != m_fileTQChk)
		delete m_fileTQChk;

	if(NULL != m_trace)
		delete m_trace;

}
