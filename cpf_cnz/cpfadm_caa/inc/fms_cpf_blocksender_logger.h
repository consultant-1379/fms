/*
 * ** @file fms_cpf_blocksender_logger.h
 *	@brief
 *	Header file for FMS_CPF_BlockSender_Logger class.
 *  This module contains the declaration of the class FMS_CPF_BlockSender_Logger.
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

/*=====================================================================
 DIRECTIVE DECLARATION SECTION
 ==================================================================== */
#ifndef FMS_CPF_BLOCKSENDER_LOGGER_H_
#define FMS_CPF_BLOCKSENDER_LOGGER_H_

/*===================================================================
 INCLUDE DECLARATION SECTION
 =================================================================== */
#include <string>
#include <stdint.h>

/*===================================================================
 CLASS FORWARD DECLARATION SECTION
 =================================================================== */

class ACS_TRA_trace;

/*=====================================================================
 CLASS DECLARATION SECTION
 ==================================================================== */
class FMS_CPF_BlockSender_Logger
{
public:

	/**
	 * 	@brief	Constructor of FMS_CPF_BlockSender_Logger class
	 */
	FMS_CPF_BlockSender_Logger(const std::string& fileName, const std::string& cpName,
							   const std::string& subFilePath);

	/**
	 * 	@brief	Destructor of FMS_CPF_BlockSender_Logger class
	 */
	virtual ~FMS_CPF_BlockSender_Logger();

	/**
	*	@brief  This method updates the log file for the block send
	*
	*/
	void updateLogFile(const uint32_t& activeSubfile,
					   const uint32_t& absoluteBlockNum,
					   const uint32_t& blockNumOffset);

	/**
	*	@brief  This method updates the log file for the block send
	*
	*/
	void updateLogFile(const uint32_t& absoluteBlockNum);

	/**
	*	@brief  This method updates the log file with new file to send
	*
	*/
	void updateSubFileToLogFile(const uint32_t& nextSubfileTosend);


	/**
	*	@brief  This method gets the current subfile path to send
	*
	*/
	inline const char* getSubFileToSend() { return m_SubFileFullPath.c_str(); };

	/**
	*	@brief  This method gets the current subfile number to send
	*
	*/
	inline uint32_t getSubFileToSendAsNumber() { return m_SubfileToSend; };

	/**
	*	@brief  This method gets the last block sent
	*
	*/
	inline uint32_t getLastBlockCommited() { return m_AbsoluteBlockNumber; };

	/**
	*	@brief  This method sets the last block sent
	*
	*/
	inline void setLastBlockCommited(const uint32_t& lastCommitedBlock)
									{ m_AbsoluteBlockNumber = lastCommitedBlock; };

	/**
	*	@brief  This method gets the offset to seek in the current subfile
	*
	*/
	inline uint32_t getBlockNumberOffset() { return m_BlockNumberOffset; };

	/**
	*	@brief  This method sets the offset to seek in the current subfile
	*
	*/
	inline void setBlockNumberOffset(const uint32_t& blockNumberOffset)
									{ m_BlockNumberOffset = blockNumberOffset; };

	/**
	 *	@brief  This method creates the log file for the block send
	 *
	*/
	void initLogFile();

	/**
	 *	@brief  This method converts subfile name string to an int value
	 *
	*/
	uint32_t subFileStringToInt(const std::string& subFilePath);

private:

	/**
	 *	@brief  This method set the internal members
	 *
	*/
	void setInternalData();

	/**
	*	@brief  This method update active subfile
	*
	*/
	void updateActiveSubFile(const uint32_t& subFileNumber);

	/**
	 * 	@brief	m_FileName
	 *
	 * 	The CP file name source of blocks.
	 *
	 */
	std::string m_FileName;

	/**
	 * 	@brief	m_CpName
	 *
	 * 	The CP name to which the file belongs
	 *
	 */
	std::string m_CpName;

	/**
	 * 	@brief	m_LogFileFullPath
	 *
	 * 	The log file name with its path
	 *
	 */
	std::string m_LogFileFullPath;

	/**
	 * 	@brief	m_FileFullPath
	 *
	 * 	The Cp file path
	 *
	 */
	std::string m_FileFullPath;

	/**
	 * 	@brief	m_SubFileFullPath
	 *
	 * 	The Cp subfile path
	 *
	 */
	std::string m_SubFileFullPath;

	/**
	 * 	@brief	m_BlockNumberOffset
	 *
	 * 	The block number offset to position into the current subfile
	 *
	 */
	uint32_t m_BlockNumberOffset;

	/**
	 * 	@brief	m_AbsoluteBlockNumber
	 *
	 * 	The block number absolute
	 *
	 */
	uint32_t m_AbsoluteBlockNumber;

	/**
	 * 	@brief	m_ActiveSubfile
	 *
	 * 	The active subfile
	 *
	 */
	uint32_t m_SubfileToSend;

	/**
	 * 	@brief	m_trace: trace object
	*/
	ACS_TRA_trace* m_trace;

};

#endif /* FMS_CPF_BLOCKSENDER_LOGGER_H_ */
