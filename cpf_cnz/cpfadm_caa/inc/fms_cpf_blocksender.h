/*
 * ** @file fms_cpf_blocksender.h
 *	@brief
 *	Header file for FMS_CPF_BlockSender class.
 *  This module contains the declaration of the class FMS_CPF_BlockSender.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2013-06-21
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
 *	| 1.0.0  | 2013-06-21 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_BLOCKSENDER_H_
#define FMS_CPF_BLOCKSENDER_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "aes_ohi_blockhandler2.h"

#include <ace/RW_Thread_Mutex.h>
#include <string>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */

class ACS_TRA_trace;
//class AES_OHI_BlockHandler2;
class FMS_CPF_BlockSender_Logger;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FMS_CPF_BlockSender
{
 public:

	enum sendResult {
		  DATAEXIST = 0,
		  NOMOREDATAEXIST,
		  TQERROR
	};

	/**
	 * 	@brief	Constructor of FMS_CPF_BlockSender class
	*/
	FMS_CPF_BlockSender(const std::string& fileName, const std::string& volumeName, const std::string& cpName,
						const std::string& subFilePath, const int blockSize, bool systemType);

	/**
	 * 	@brief	Destructor of FMS_CPF_BlockSender class
	*/
	virtual ~FMS_CPF_BlockSender();

	/**
	 *	@brief  This method updates the active subfile
	 *
	*/
	void setActiveSubFile(const uint32_t& activeSubFile) { m_activeSubFile = activeSubFile; };

	/**
	 *	@brief  This method sends all blocks of a subfile
	 *
	*/
	sendResult sendDataOnSCP(volatile bool& stopRequest);

	/**
	 *	@brief  This method sends all blocks of a subfile in a MCP environment
	 *
	*/
	void sendData(const long& timeSlice);

	/**
	 *	@brief  This method gets the remove state
	 *
	*/
	void getRemoveState(bool& isToRemove);

	/**
	 *	@brief  This method sets the remove state to true
	 *
	*/
	void setRemoveState();

 private:

	/**
	 *	@brief  This method sends all blocks of a subfile
	 *
	*/
	sendResult sendSubFileBlocks();

	/**
	 *	@brief  This method attach the OHI block TQ object
	 *
	*/
	bool attachToBlockTQ();

	/**
	 *	@brief  This method detach the OHI block TQ object
	 *
	*/
	void detachToBlockTQ();

	/**
	 *	@brief  This method starts a block transfer queue transaction
	 *
	*/
	bool blockTransfer_TranscationBegin();

	/**
	 *	@brief  This method send a buffer to the block transfer queue
	 *
	*/
	bool sendBlockToTQ(const char* buffer, uint32_t& blockSentNumber);

	/**
	 *	@brief  This method terminates and commit a block transfer transaction
	 *
	*/
	void blockTransfer_TranscationEnd(const uint32_t& currentSentBlock, bool endOfFile);

	/**
	 *	@brief  This method creates the OHI block TQ object
	 *
	*/
	bool createBlockTQHandler();

	/**
	 *	@brief  This method opens the input subfile
	 *
	*/
	bool openSubFileToSend(int& readFD);

	/**
	 *	@brief  This method removes the subfile and updates IMM
	 *
	*/
	void updateInfiniteFileState();

	/**
	 * 	@brief	m_FileName
	 *
	 * 	The CP file name source of blocks.
	 *
	*/
	std::string m_FileName;

	/**
	 * 	@brief	m_VolumeName
	 *
	 * 	The volume of the CP file.
	 *
	*/
	std::string m_VolumeName;

	/**
	 * 	@brief	m_TqName
	 *
	 * 	The block transfer queue name.
	 *
	*/
	std::string m_TqName;

	/**
	 * 	@brief	m_CpName
	 *
	 * 	The CP name to which the file belongs
	 *
	*/
	std::string m_CpName;

	/**
	 * 	@brief	m_IsMultiCP
	 *
	 * 	Flag of system type (SCP/MCP)
	*/
	bool m_IsMultiCP;

	/**
	 * 	@brief	m_TqStreamId
	 *
	 * 	The block transfer queue stream identity
	 *
	*/
	std::string m_TqStreamId;

	/**
	 * 	@brief	m_BlockSize
	 *
	 * 	The block size. It is also the record length of the file
	 *
	*/
	int m_BlockSize;

	/**
	 * 	@brief	m_activeSubFile
	 *
	 * 	The active subfile of infinite file ready to send
	 *
	*/
	volatile uint32_t m_activeSubFile;

	/**
	 * 	@brief	m_subFileToSend
	 *
	 * 	The current subfile of infinite file to send
	 *
	*/
	uint32_t m_subFileToSend;

	/**
	 * 	@brief	m_readFD
	 *
	 * 	File descriptor of opened subfile
	 *
	*/
	int m_readFD;

	/**
	 * 	@brief	m_RemoveMe
	 *
	 * 	Flag to indicates that this sender has been removed
	 *
	*/
	bool m_RemoveState;

	/**
	 * 	@brief	m_ohiBlockHandler
	 *
	 * 	OHI block handler object
	 *
	*/
	AES_OHI_BlockHandler2* m_ohiBlockHandler;

	/**
	 * 	@brief	m_lastOhiError
	 *
	 * 	Last OHI error code
	 *
	*/
	unsigned int m_lastOhiError;

	/**
	 * 	@brief	m_LogFileHandler
	 *
	 * 	Log file handler object
	 *
	*/
	FMS_CPF_BlockSender_Logger* m_LogFileHandler;

	/**
	 * 	@brief	m_mutex
	 *
	 * 	Mutex for internal map access
	*/
	ACE_RW_Thread_Mutex m_mutex;

	/**
	 * 	@brief	m_trace: trace object
	*/
	ACS_TRA_trace* m_trace;
};

#endif /* FMS_CPF_BLOCKSENDER_H_ */
