/*
 * * @file fms_cpf_filetransferhndl.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_FileTransferHndl.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_filetransferhndl.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-03-15
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
 *	| 1.0.0  | 2012-03-15 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 *  | 1.1.0  | 2014-04-29 | qvincon      | Infinite subfiles removed from IMM  |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#define BOOST_FILESYSTEM_VERSION 3

#include "fms_cpf_filetransferhndl.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_tqchecker.h"
#include "fms_cpf_common.h"
#include "fms_cpf_privateexception.h"

#include "aes_ohi_filehandler.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <sstream>
#include <grp.h>

extern ACS_TRA_Logging CPF_Log;

FMS_CPF_FileTransferHndl::FMS_CPF_FileTransferHndl(const std::string& infiniteFileName, const std::string& mainFilePath, const std::string& cpName)
: m_infiniteFileName(infiniteFileName),
  m_infiniteFilePath(mainFilePath),
  m_cpName(cpName),
  m_fileTQ(),
  m_TQPath(),
  m_fileToSend(),
  m_srcFilePath(),
  m_dstFilePath(),
  m_currentSubFile(0),
  m_isFaultyStatus(false),
  m_lastOhiError(0),
  m_ohiFileHandler(NULL)
{
	m_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_FileTransferHndl");
}

/*============================================================================
	METHOD: setFileTQ
 ============================================================================ */
int FMS_CPF_FileTransferHndl::setFileTQ(const std::string& currentTQ)
{
	TRACE(m_trace, "Entering in setFileTQ(<%s>)", currentTQ.c_str());
	int result = 0;

	// Check if tq has been changed
	if( (m_fileTQ.compare(currentTQ)!= 0) || m_isFaultyStatus)
	{
		TRACE(m_trace, "setFileTQ(), current TQ:<%s> changed", m_fileTQ.c_str());

		// Delete previous handler
		removeTQHandler();

		// check the file TQ
		if(!currentTQ.empty())
		{
			m_isFaultyStatus = true;
			TRACE(m_trace, "%s", "setFileTQ(), validate new file TQ");
			if( TQChecker::instance()->validateFileTQ(currentTQ, result) )
			{
				// Create TQ folder if not exist
				if( TQChecker::instance()->createTQFolder(currentTQ, m_TQPath) )
				{
					TRACE(m_trace, "%s", "setFileTQ(), create file TQ handler");
					result = createTQHandler(currentTQ);
				}
				else
					result = static_cast<int>(FMS_CPF_PrivateException::PHYSICALERROR);
			}
		}

		// set the new current file TQ
		if(SUCCESS == result)
		{
			m_fileTQ = currentTQ;
			m_isFaultyStatus = false;
		}
	}
	TRACE(m_trace, "Leaving setFileTQ(), result:<%d>", result);
	return result;
}

/*============================================================================
	METHOD: moveFileToTransfer
 ============================================================================ */
int FMS_CPF_FileTransferHndl::moveFileToTransfer(const unsigned long infiniteSubFile)
{
	TRACE(m_trace, "Entering in moveFileToTransfer(<%d>)", infiniteSubFile);
	int result = SUCCESS;
	char strSubFile[12] = {'\0'};

	// current subfile to send
	m_currentSubFile = infiniteSubFile;
	ACE_OS::snprintf(strSubFile, 11, "%.10ld", m_currentSubFile);

	// full subfile path
	std::string sourcePath(m_infiniteFilePath);
	sourcePath += strSubFile;

	m_srcFilePath = sourcePath;

	TRACE(m_trace, "moveFileToTransfer(), src file:<%s>", sourcePath.c_str());
	makeFileNameToSend(strSubFile);

	// full destination file path
	std::string destinationPath(m_TQPath);
	destinationPath += DirDelim +  m_fileToSend;

	m_dstFilePath = destinationPath;

	TRACE(m_trace, "moveFileToTransfer(), dst file:<%s>", destinationPath.c_str());

	boost::system::error_code moveResult;

	// copy and rename subfile to TQ folder
	boost::filesystem::copy_file(sourcePath, destinationPath, moveResult);
	int copyResult = moveResult.value();
	// Check move result
	if( (SUCCESS != copyResult) )
	{
		// Error on file move
		char log_buff[512] = {'\0'};
		snprintf(log_buff, 511,"FMS_CPF_FileTransferHndl::moveFileToTransfer(), error:<%d> on move of file from <%s> to <%s> ", copyResult, sourcePath.c_str(), destinationPath.c_str() );
		CPF_Log.Write(log_buff,	LOG_LEVEL_ERROR);

		// Destination file already present
		if(EEXIST == copyResult)
		{
			TRACE(m_trace, "%s", "moveFileToTransfer(), destination file already exist");
			result = moveWithTimeStamp();
		}
		else if(EDQUOT == copyResult) // Quota disk reached
		{
			// return the error
			result = copyResult;

			// In this case the move fails, however into the destination folder
			// an empty file is always created
			boost::system::error_code delResult;
			boost::filesystem::remove(destinationPath, delResult);

		}
		else if(ENOENT != copyResult) //ENOENT source file not exist, so already moved
			result = copyResult;

	}

	TRACE(m_trace, "Leaving moveFileToTransfer(), result:<%d>", result);
	return result;
}

/*============================================================================
	METHOD: moveFileToTransfer
 ============================================================================ */
int FMS_CPF_FileTransferHndl::moveFileToTransfer(const std::string& subFileName)
{
	TRACE(m_trace, "Entering in %s :<%s>", __func__, subFileName.c_str());
	int result = SUCCESS;

	// full subfile path
	std::string sourcePath(m_infiniteFilePath);
	sourcePath.push_back(DirDelim);
	sourcePath += subFileName;

	m_srcFilePath = sourcePath;

	TRACE(m_trace, "%s, src file:<%s>", __func__, sourcePath.c_str());
	makeFileNameToSend(subFileName.c_str());

	// full destination file path
	std::string destinationPath(m_TQPath);
	destinationPath.push_back(DirDelim);
	destinationPath.append(m_fileToSend);

	m_dstFilePath = destinationPath;

	TRACE(m_trace, "%s, dst file:<%s>", __func__, destinationPath.c_str());

	boost::system::error_code moveResult;

	// copy and rename subfile to TQ folder
	boost::filesystem::copy_file(sourcePath, destinationPath, moveResult);
	int copyResult = moveResult.value();
	// Check move result
	if( (SUCCESS != copyResult) )
	{
		// Error on file move
		char log_buff[512] = {'\0'};
		snprintf(log_buff, 511,"%s, error:<%d> on move of file from <%s> to <%s> ", __func__, copyResult, sourcePath.c_str(), destinationPath.c_str() );
		CPF_Log.Write(log_buff,	LOG_LEVEL_ERROR);

		// Destination file already present
		if(EEXIST == copyResult)
		{
			TRACE(m_trace, "%s, destination file already exist", __func__);
			result = moveWithTimeStamp();
		}
		else if(EDQUOT == copyResult) // Quota disk reached
		{
			// return the error
			result = copyResult;

			// In this case the move fails, however into the destination folder
			// an empty file is always created
			boost::system::error_code delResult;
			boost::filesystem::remove(destinationPath, delResult);
		}
		else if(ENOENT != copyResult) //ENOENT source file not exist, so already moved
			result = copyResult;
	}

	TRACE(m_trace, "Leaving %s, result:<%d>", __func__, result);
	return result;
}

/*============================================================================
	METHOD: sendCurrentFile
 ============================================================================ */
bool FMS_CPF_FileTransferHndl::sendCurrentFile(bool isRelCmdHdf)
{
	TRACE(m_trace, "Entering in sendCurrentFile(<%s>)", m_fileToSend.c_str());
	bool result = false;
	if(NULL != m_ohiFileHandler)
	{
		boost::system::error_code delResult;
		int removeResult;

		m_lastOhiError = m_ohiFileHandler->send(m_fileToSend.c_str());
		if( (AES_OHI_NOERRORCODE != m_lastOhiError) && (AES_OHI_SENDITEMEXIST != m_lastOhiError) )
		{
			char logBuffer[256] = {'\0'};
			// File moved but some error happens, remove file from the TQ folder
			// full subfile path
			if(AES_OHI_FILENOTFOUND == m_lastOhiError)
			{
				snprintf(logBuffer, 255, "FMS_CPF_FileTransferHndl::sendCurrentFile(), send file:<%s> to file TQ:<%s> failed, file not found", m_fileToSend.c_str(), m_fileTQ.c_str() );
				CPF_Log.Write(logBuffer, LOG_LEVEL_WARN);
				if( !checkFilesExist() )
				{
					snprintf(logBuffer, 255, "FMS_CPF_FileTransferHndl::sendCurrentFile(), file:<%s> and subfile:<%ld> are not found", m_fileToSend.c_str(), m_currentSubFile );
					CPF_Log.Write(logBuffer, LOG_LEVEL_WARN);
					// File and subfile are not present, can only send next subfile
					result = true;
				}
			}
			else
			{
				boost::filesystem::remove(m_dstFilePath, delResult);
				removeResult = delResult.value();
				// Check remove result, but in any case the move operation has been done with success
				if(0 != removeResult)
				{
					// remove error log it
					snprintf(logBuffer, 255, "FMS_CPF_FileTransferHndl::sendCurrentFile(), error:<%d> on remove subfile<%s>", removeResult, m_dstFilePath.c_str() );
					CPF_Log.Write(logBuffer,	LOG_LEVEL_ERROR);
				}
				// Failed send
				m_isFaultyStatus = true;

				snprintf(logBuffer, 255, "FMS_CPF_FileTransferHndl::sendCurrentFile(), send file:<%s> to file TQ:<%s> failed with OHI error:<%d>", m_fileToSend.c_str(), m_fileTQ.c_str(), m_lastOhiError );
				CPF_Log.Write(logBuffer, LOG_LEVEL_ERROR);
				removeTQHandler();
			}
		}
		else
		{
			if(AES_OHI_SENDITEMEXIST == m_lastOhiError)
			{
				char logBuffer[256] = {'\0'};
				snprintf(logBuffer, 255, "FMS_CPF_FileTransferHndl::sendCurrentFile(), file:<%s> already reported to file TQ:<%s> ", m_fileToSend.c_str(), m_fileTQ.c_str() );
				CPF_Log.Write(logBuffer,	LOG_LEVEL_WARN);
			}

			if(!isRelCmdHdf)
			{
				// File moved, remove subfile from CP file system
				// full subfile path
				boost::system::error_code delResult;
				boost::filesystem::remove(m_srcFilePath, delResult);
				removeResult = delResult.value();

				// Check remove result, but in any case the move operation has been done with success
				if(SUCCESS != removeResult)
				{
					// remove error log it
					char logBuffer[256] = {'\0'};
					snprintf(logBuffer, 255, "FMS_CPF_FileTransferHndl::sendCurrentFile(), error:<%d> on remove subfile<%s>", removeResult, m_srcFilePath.c_str() );
					CPF_Log.Write(logBuffer,	LOG_LEVEL_ERROR);
				}
			}

			result = true;
		}
	}

	TRACE(m_trace, "Leaving sendCurrentFile(), result:<%s>", (result ? "OK": "FAILED"));
	return result;
}

/*============================================================================
	METHOD: getOhiErrotText
 ============================================================================ */
void FMS_CPF_FileTransferHndl::getOhiErrotText(std::string& errorText)
{
	TRACE(m_trace, "%s", "Entering in getOhiErrotText()");
	if(NULL != m_ohiFileHandler && ( 0 != m_lastOhiError) )
	{
		errorText = m_ohiFileHandler->getErrCodeText(m_lastOhiError);
	}
	TRACE(m_trace, "Leaving getOhiErrotText(), error:<%d>, text:<%s>", m_lastOhiError, errorText.c_str());
}

/*============================================================================
	METHOD: createTQHandler
 ============================================================================ */
int FMS_CPF_FileTransferHndl::createTQHandler(const std::string& tqName)
{
	TRACE(m_trace, "%s", "Entering in createTQHandler()");
	int result = SUCCESS;

	m_ohiFileHandler = new (std::nothrow) AES_OHI_FileHandler(OHI_USERSPACE::SUBSYS,
															  OHI_USERSPACE::APPNAME,
															  tqName.c_str(),
															  OHI_USERSPACE::eventText,
															  m_TQPath.c_str());
	if(NULL != m_ohiFileHandler )
	{
		m_lastOhiError = m_ohiFileHandler->attach();
		TRACE(m_trace, "createTQHandler(), attach result :<%d>", m_lastOhiError );
		if( AES_OHI_NOERRORCODE != m_lastOhiError)
		{
			// Failed attach
			result = static_cast<int>(FMS_CPF_Exception::TQNOTFOUND);
			delete m_ohiFileHandler;
			m_ohiFileHandler = NULL;
			char log_buff[512] = {'\0'};
			snprintf(log_buff, 511,"FMS_CPF_FileTransferHndl::createTQHandler(), attach to file TQ:<%s> failed with OHI error:<%d>", m_fileTQ.c_str(), m_lastOhiError );
			CPF_Log.Write(log_buff,	LOG_LEVEL_ERROR);
		}
	}
	else
	{
		result = static_cast<int>(FMS_CPF_Exception::INTERNALERROR);
		char log_buff[512] = {'\0'};
		snprintf(log_buff, 511,"FMS_CPF_FileTransferHndl::createTQHandler(), failed memory allocation for TQ:<%s> handler", m_fileTQ.c_str());
		CPF_Log.Write(log_buff,	LOG_LEVEL_ERROR);
	}
	TRACE(m_trace, "%s", "Leaving createTQHandler()");
	return result;
}

/*============================================================================
	METHOD: checkFilesExist
 ============================================================================ */
bool FMS_CPF_FileTransferHndl::checkFilesExist()
{
	TRACE(m_trace, "%s", "Entering in checkFilesExist()");

	boost::system::error_code existResult;

	bool subFileExist = boost::filesystem::exists(m_srcFilePath, existResult);
	bool transferFileExist = boost::filesystem::exists(m_dstFilePath, existResult);

	bool result = (subFileExist || transferFileExist);

	TRACE(m_trace, "%s", "Leaving checkFilesExist()");
	return result;
}

/*============================================================================
	METHOD: moveWithTimeStamp
 ============================================================================ */
int FMS_CPF_FileTransferHndl::moveWithTimeStamp()
{
	TRACE(m_trace, "%s", "Entering in moveWithTimeStamp()");
	timeval time;
	// Get time of day
	int result = gettimeofday(&time, NULL);

	if( SUCCESS == result )
	{
		std::stringstream timeStamp;
		timeStamp << "_" << time.tv_usec << std::ends;

		m_dstFilePath.append(timeStamp.str());
		m_fileToSend.append(timeStamp.str());

		boost::system::error_code copyResult;

		// copy and rename subfile to TQ folder
		boost::filesystem::copy_file(m_srcFilePath, m_dstFilePath, copyResult);

		result = copyResult.value();
		if( SUCCESS != result )
		{
			// Error on file move
			char logBuffer[512] = {'\0'};
			snprintf(logBuffer, 511,"FMS_CPF_FileTransferHndl::moveWithTimeStamp(), error:<%d> on move of file from <%s> to <%s> ", result, m_srcFilePath.c_str(), m_dstFilePath.c_str() );
			CPF_Log.Write(logBuffer, LOG_LEVEL_ERROR);
		}
	}
	TRACE(m_trace, "%s", "Leaving moveWithTimeStamp()");
	return result;
}

/*============================================================================
	METHOD: removeTQHandler
 ============================================================================ */
void FMS_CPF_FileTransferHndl::removeTQHandler()
{
	TRACE(m_trace, "%s", "Entering in removeTQHandler()");
	if(NULL != m_ohiFileHandler)
	{
		TRACE(m_trace, "%s", "removeTQHandler(), file TQ detach");
		m_ohiFileHandler->detach();
		delete m_ohiFileHandler;
		m_ohiFileHandler = NULL;
	}
	TRACE(m_trace, "%s", "Leaving removeTQHandler()");
}


/*============================================================================
	METHOD: makeFileNameToSend
 ============================================================================ */
void FMS_CPF_FileTransferHndl::makeFileNameToSend(const char* subFile)
{
	TRACE(m_trace, "%s", "Entering in makeFileNameToSend()");

	if(m_cpName.empty())
	{
		//SCP
		m_fileToSend = m_infiniteFileName + '-' + subFile;
	}
	else
	{
		//MCP
		m_fileToSend = m_cpName + '+' + m_infiniteFileName + '-' + subFile;
	}

	TRACE(m_trace, "Leaving makeFileNameToSend(), file to send:<%s>", m_fileToSend.c_str());
}

/*============================================================================
	METHOD: ~FMS_CPF_FileTransferHndl
 ============================================================================ */
FMS_CPF_FileTransferHndl::~FMS_CPF_FileTransferHndl()
{
	removeTQHandler();

	if(NULL != m_trace)
		delete m_trace;
}
/*============================================================================
        METHOD: moveFileToTransfer
 ============================================================================ */
int FMS_CPF_FileTransferHndl::linkFileToTransfer(const std::string& subFileName)
{
        TRACE(m_trace, "Entering in %s :<%s>", __func__, subFileName.c_str());
        int result = SUCCESS;

        // full subfile path
        std::string sourcePath(m_infiniteFilePath);
        sourcePath.push_back(DirDelim);
        sourcePath += subFileName;

        m_srcFilePath = sourcePath;

        TRACE(m_trace, "%s, src file:<%s>, m_fileToSend: <%s>, subFileName: <%s>, m_TQPath: <%s>", __func__, sourcePath.c_str(), m_fileToSend.c_str(), subFileName.c_str(), m_TQPath.c_str());

        m_fileToSend = subFileName;
        // full destination file path
        std::string destinationPath(m_TQPath);
        destinationPath.push_back(DirDelim);
        destinationPath.append(m_fileToSend);

        m_dstFilePath = destinationPath;

        TRACE(m_trace, "%s, dst file:<%s>", __func__, destinationPath.c_str());

        boost::system::error_code linkResult;

        // copy and rename subfile to TQ folder
        boost::filesystem::create_hard_link(sourcePath, m_dstFilePath, linkResult);
        int copyResult = linkResult.value();
        // Check move result
        if( (SUCCESS != copyResult) )
        {
                // Error on file link
                char log_buff[512] = {'\0'};
                snprintf(log_buff, 511,"%s, error:<%d> on hardlink of file from <%s> to <%s> ", __func__, copyResult, sourcePath.c_str(), destinationPath.c_str() );
                CPF_Log.Write(log_buff, LOG_LEVEL_ERROR);

                // Destination file already present
                if(EEXIST == copyResult)
                {
                        TRACE(m_trace, "%s, destination file already exist", __func__);
                }
                else if(EDQUOT == copyResult) // Quota disk reached
                {
                        // return the error
                        result = copyResult;

                        // In this case the hardlink fails, however into the destination folder
                        // an empty file is always created
                        //boost::system::error_code delResult;
                        //boost::filesystem::remove(destinationPath, delResult);
                }
                else if(ENOENT != copyResult) //ENOENT source file not exist, so already moved
                        result = copyResult;
        }

        TRACE(m_trace, "Leaving %s, result:<%d>", __func__, result);
        return result;
}

