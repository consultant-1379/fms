/*
** @file fms_cpf_blocksender_scheduler.h
 *	@brief
 *	Header file for FMS_CPF_BlockTransferMgr class.
 *  This module contains the declaration of the class FMS_CPF_BlockTransferMgr.
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

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_BLOCKSENDER_SCHEDULER_H_
#define FMS_CPF_BLOCKSENDER_SCHEDULER_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include <ace/Task_T.h>
#include <ace/Synch.h>
#include <ace/RW_Thread_Mutex.h>
#include <ace/Semaphore.h>

#include <map>
#include <string>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class FMS_CPF_BlockSender;
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FMS_CPF_BlockSender_Scheduler : public ACE_Task<ACE_MT_SYNCH>
{

 public:

	/**
	 * 	@brief	Constructor of FMS_CPF_BlockSender_Scheduler class
	*/
	FMS_CPF_BlockSender_Scheduler(const std::string& fileName, bool isMultiCP);

	/**
	 * 	@brief	Destructor of FMS_CPF_BlockSender_Scheduler class
	*/
	virtual ~FMS_CPF_BlockSender_Scheduler();

	/**
	 * 	@brief  Run by a daemon thread
	*/
	virtual int svc(void);

	/**
	 * 	@brief	This method initializes a task and prepare it for execution
	*/
	virtual int open(void *args = 0);

	/**
		@brief  This method is called by ACE_Thread_Exit, as hook, on svc termination
	*/
	virtual int close(u_long flags = 0);

	/**
	 *	@brief  This method adds a new block sender for a CP.
	 *
	 *	@param cpName :  CP name
	 *	@param blockSize : block size to send
	 *	@param subFilePath : physical path of the first subfile to transfer
	 *
	*/
	bool addBlockSender(const std::string& cpName, const std::string& volumeName, const std::string& subFilePath, const int blockSize);

	/**
	 *	@brief  This method remove a block sender for a CP.
	 *
	 *	@param cpName :  CP name
	 *
	*/
	bool removeBlockSender(const std::string& cpName);

	/**
	 *	@brief  This method stops scheduler running
	 *
	*/
	void shutDown();

	/** @brief updateBlockSenderState method
	 *
	 *	This method updates the last subfile that can be transfered
	 *
	 *	@param cpName :  CP name
	 *	@param lastReadySubfile : last closed subfile
	 *
	*/
	void updateBlockSenderState(const std::string& cpName, const unsigned long lastReadySubfile);

 private:

	/**
	 *	@brief  This method deletes all dynamically allocated objects
	 *
	*/
	void cleanup();

	/**
	 *	@brief  This method handles the block transfer in SCP
	 *
	*/
	void handle_SingleCP_BlockTransfer();

	/**
	 *	@brief  This method handles the block transfer in MCP
	 *	@brief  for a blade.
	 *
	 *	@param: cpName :  CP name to handle
	 *
	*/
	bool handle_Blade_BlockTransfer(const std::string& cpName);

	/**
	 * 	@brief	m_IsMultiCP
	 *
	 * 	Flag of system type (SCP/MCP)
	*/
	bool m_IsMultiCP;

	/**
	 * 	@brief	m_FileName
	 *
	 * 	The CP file name source of blocks.
	 *
	*/
	std::string m_FileName;

	/**
	 * 	@brief	m_BlockSendersMap
	 *
	 * 	Map of pair CP and its Block Sender.
	 *
	*/
	std::map<std::string, FMS_CPF_BlockSender*> m_BlockSendersMap;

	/**
	 * 	@brief	m_ShutDown
	 *
	 * 	The flag to indicates a shut down request
	 *
	*/
	volatile bool  m_ShutDown;

	/**
	 * 	@brief	m_CpSliceTime
	 *
	 * 	The Cp slice time to send data, expressed in milliseconds
	 *
	*/
	long int m_CpTimeSlice;

	/**
	 * 	@brief	m_mutex
	 *
	 * 	Mutex for internal map access
	*/
	ACE_RW_Thread_Mutex m_mutex;

	/**
	 * 	@brief	m_DataReady
	 *
	 * 	Semaphore used in MCP to signal data ready to send
	*/
	ACE_Semaphore m_DataReady;

	/**
	 * 	@brief	m_trace: trace object
	*/
	ACS_TRA_trace* m_trace;

};

#endif /* FMS_CPF_BLOCKSENDER_SCHEDULER_H_ */
