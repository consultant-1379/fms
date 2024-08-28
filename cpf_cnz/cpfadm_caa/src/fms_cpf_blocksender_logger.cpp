/*
 * * @file fms_cpf_blocksender_logger.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_BlockSender_Logger.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_blocksender_logger.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2013-06-24
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
 *	| 1.0.0  | 2013-06-24 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_blocksender_logger.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_common.h"


#include "ACS_TRA_trace.h"

#include <boost/filesystem.hpp>
#include <fstream>
#include <sstream>
#include <inttypes.h>

typedef struct blockTransferInfo
{
    uint32_t subfileToSend;
    uint32_t absoluteBlockNumber;
    uint32_t blockNumberOffset;
    char subfilePath[64];
}blockTransferInfo_t;

/*============================================================================
	METHOD: FMS_CPF_BlockSender_Logger
 ============================================================================ */
FMS_CPF_BlockSender_Logger::FMS_CPF_BlockSender_Logger(const std::string& fileName, const std::string& cpName,
		   	   	   	   	   	   	   	   	   	   	   	   const std::string& subFilePath)
: m_FileName(fileName),
  m_CpName(cpName),
  m_LogFileFullPath(),
  m_FileFullPath(),
  m_SubFileFullPath(subFilePath),
  m_BlockNumberOffset(0U),
  m_AbsoluteBlockNumber(0U),
  m_SubfileToSend(1),
  m_trace(new (std::nothrow) ACS_TRA_trace("FMS_CPF_BlockSender_Logger"))
{
	setInternalData();
}

/*============================================================================
	METHOD: ~FMS_CPF_BlockSender_Logger
 ============================================================================ */
FMS_CPF_BlockSender_Logger::~FMS_CPF_BlockSender_Logger()
{
	if( NULL != m_trace)
		delete m_trace;
}

/*============================================================================
	METHOD: updateLogFile
 ============================================================================ */
void FMS_CPF_BlockSender_Logger::updateLogFile(const uint32_t& activeSubfile, const uint32_t& absoluteBlockNum,
				   	   	   	   	   	   	   	   const uint32_t& blockNumOffset)
{
	TRACE(m_trace, "Entering in %s()", __func__);

	// Update internal members
	updateActiveSubFile(activeSubfile);
	m_BlockNumberOffset = blockNumOffset;
	m_AbsoluteBlockNumber = absoluteBlockNum;

	// Store to log file
	blockTransferInfo_t blockSendInfo;
	memset(blockSendInfo.subfilePath,0, sizeof(blockSendInfo.subfilePath)/sizeof(char));
	size_t pathLength = m_SubFileFullPath.length();
	m_SubFileFullPath.copy(blockSendInfo.subfilePath, pathLength);

	blockSendInfo.absoluteBlockNumber = m_AbsoluteBlockNumber;
	blockSendInfo.blockNumberOffset = m_BlockNumberOffset;
	blockSendInfo.subfileToSend = m_SubfileToSend;

	std::ofstream output_file(m_LogFileFullPath.c_str(), std::ios::binary);
	output_file.write((char*)&blockSendInfo, sizeof(blockSendInfo));
	output_file.close();

	TRACE(m_trace, "Leaving in %s()", __func__);
}

/*============================================================================
	METHOD: updateLogFile
 ============================================================================ */
void FMS_CPF_BlockSender_Logger::updateLogFile(const uint32_t& absoluteBlockNum)
{
	TRACE(m_trace, "Entering in %s()", __func__);

	m_AbsoluteBlockNumber = absoluteBlockNum;

	// Store to log file
	blockTransferInfo_t blockSendInfo;

	memset(blockSendInfo.subfilePath, 0, sizeof(blockSendInfo.subfilePath)/sizeof(char));
	size_t pathLength = m_SubFileFullPath.length();
	m_SubFileFullPath.copy(blockSendInfo.subfilePath, pathLength);

	blockSendInfo.absoluteBlockNumber = m_AbsoluteBlockNumber;
	blockSendInfo.blockNumberOffset = m_BlockNumberOffset;
	blockSendInfo.subfileToSend = m_SubfileToSend;

	std::ofstream output_file(m_LogFileFullPath.c_str(), std::ios::binary);
	output_file.write((char*)&blockSendInfo, sizeof(blockSendInfo));
	output_file.close();

	TRACE(m_trace, "Leaving in %s()", __func__);
}

/*============================================================================
	METHOD: initLogFile
 ============================================================================ */
void FMS_CPF_BlockSender_Logger::updateSubFileToLogFile(const uint32_t& nextSubfileTosend)
{
	TRACE(m_trace, "Entering in %s()", __func__);

	updateActiveSubFile(nextSubfileTosend);

	// Store to log file
	blockTransferInfo_t blockSendInfo;

	memset(blockSendInfo.subfilePath, 0, sizeof(blockSendInfo.subfilePath)/sizeof(char));
	size_t pathLength = m_SubFileFullPath.length();
	m_SubFileFullPath.copy(blockSendInfo.subfilePath, pathLength);

	blockSendInfo.absoluteBlockNumber = m_AbsoluteBlockNumber;
	blockSendInfo.blockNumberOffset = m_BlockNumberOffset;
	blockSendInfo.subfileToSend = m_SubfileToSend;

	std::ofstream output_file(m_LogFileFullPath.c_str(), std::ios::binary);
	output_file.write((char*)&blockSendInfo, sizeof(blockSendInfo));
	output_file.close();

	TRACE(m_trace, "Leaving in %s()", __func__);
}

/*============================================================================
	METHOD: initLogFile
 ============================================================================ */
void FMS_CPF_BlockSender_Logger::initLogFile()
{
	TRACE(m_trace, "Entering in %s()", __func__);

	blockTransferInfo_t blockSendInfo;

	ACE_stat statbuf;
	//Checks if the physical file exists
	if(( ACE_OS::stat(m_LogFileFullPath.c_str(), &statbuf) == -1) && (ENOENT == errno) )
	{
		// File log not exist create it
		ofstream output_file(m_LogFileFullPath.c_str(), ios::binary);

		memset(blockSendInfo.subfilePath, 0, sizeof(blockSendInfo.subfilePath)/sizeof(char));
		size_t pathLength = m_SubFileFullPath.length();
		m_SubFileFullPath.copy(blockSendInfo.subfilePath, pathLength);

		blockSendInfo.absoluteBlockNumber = m_AbsoluteBlockNumber;
		blockSendInfo.blockNumberOffset = m_BlockNumberOffset;
		blockSendInfo.subfileToSend = m_SubfileToSend;

		output_file.write((char*)&blockSendInfo, sizeof(blockSendInfo));
	    output_file.close();
	    TRACE(m_trace, "%s(), create log file:<%s>, absBlockNum:<%d>, offSet:<%d>, subfile:<%d>, path:<%s> ", __func__,
	    						m_LogFileFullPath.c_str(), m_AbsoluteBlockNumber, m_BlockNumberOffset, m_SubfileToSend, m_SubFileFullPath.c_str());
	}
	else
	{
		// File log exist read it
		std::ifstream input_file(m_LogFileFullPath.c_str() , std::ios::binary);
		input_file.read((char*)&blockSendInfo, sizeof(blockSendInfo));
		input_file.close();

		m_SubFileFullPath.assign(blockSendInfo.subfilePath);
		m_AbsoluteBlockNumber = blockSendInfo.absoluteBlockNumber;
		m_BlockNumberOffset = blockSendInfo.blockNumberOffset;
		m_SubfileToSend = blockSendInfo.subfileToSend;

		TRACE(m_trace, "%s(), read log file:<%s>, absBlockNum:<%d>, offSet:<%d>, subfile:<%d>, path:<%s> ", __func__,
			    						m_LogFileFullPath.c_str(), m_AbsoluteBlockNumber, m_BlockNumberOffset, m_SubfileToSend, m_SubFileFullPath.c_str());

	}


	TRACE(m_trace, "Leaving in %s()", __func__);
}

/*============================================================================
	METHOD: setInternalData
 ============================================================================ */
void FMS_CPF_BlockSender_Logger::setInternalData()
{
	TRACE(m_trace, "Entering in %s()", __func__);

	// set the log file name with full path
	std::string logDir(ParameterHndl::instance()->getCPFlogDir(m_CpName.c_str()) + DirDelim );
	m_LogFileFullPath = logDir + m_FileName;
	m_LogFileFullPath.append(logExt);

	TRACE(m_trace, "%s(), log file:<%s>", __func__, m_LogFileFullPath.c_str());

	size_t slashPos = m_SubFileFullPath.find_last_of(DirDelim);
	// Check if the tag is present
	if( std::string::npos != slashPos )
	{
		// set CP file path
		m_FileFullPath = m_SubFileFullPath.substr(0, slashPos + 1);

		// set active subfile
		m_SubfileToSend = subFileStringToInt(m_SubFileFullPath);

		TRACE(m_trace, "%s(), file path:<%s>", __func__, m_FileFullPath.c_str());
	}

	TRACE(m_trace, "Leaving in %s()", __func__);
}

/*============================================================================
	METHOD: subFileNameToInt
 ============================================================================ */
uint32_t FMS_CPF_BlockSender_Logger::subFileStringToInt(const std::string& subFilePath)
{
	TRACE(m_trace, "Entering in %s()", __func__);
	uint32_t subfileNumber = 0;

	size_t slashPos = subFilePath.find_last_of(DirDelim);

	// Check if the tag is present
	if( std::string::npos != slashPos )
	{
		// get active subfile
		std::string subfile = subFilePath.substr(slashPos + 1);
		std::istringstream strintToInt(subfile);
		strintToInt >> subfileNumber;

		TRACE(m_trace, "%s(), subfile path:<%s>, active subFile:<%d>", __func__, subFilePath.c_str(), subfileNumber);
	}

	TRACE(m_trace, "Leaving in %s()", __func__);

	return subfileNumber;
}

/*============================================================================
	METHOD: updateActiveSubFile
 ============================================================================ */
void FMS_CPF_BlockSender_Logger::updateActiveSubFile(const uint32_t& subFileNumber)
{
	TRACE(m_trace, "Entering in %s()", __func__);

	m_SubfileToSend = subFileNumber;
	m_SubFileFullPath.clear();

	char subfileName[stdValue::SUB_LENGTH] = {'\0'};

	// Update last sent value
	ACE_OS::snprintf(subfileName, stdValue::SUB_LENGTH, "%.10i", m_SubfileToSend);

	m_SubFileFullPath = m_FileFullPath;
	m_SubFileFullPath.append(subfileName);

	TRACE(m_trace, "Leaving in %s(),", __func__);
}
