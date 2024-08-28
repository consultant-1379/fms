/*
** @file fms_cpf_blocktransfermgr.h
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
#ifndef FMS_CPF_BLOCKTRANSFERMGR_H_
#define FMS_CPF_BLOCKTRANSFERMGR_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include <ace/Singleton.h>
#include <ace/Synch.h>
#include <ace/RW_Thread_Mutex.h>

#include <map>
#include <string>


/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class FMS_CPF_BlockSender_Scheduler;
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FMS_CPF_BlockTransferMgr
{
 public:

	friend class ACE_Singleton<FMS_CPF_BlockTransferMgr, ACE_Recursive_Thread_Mutex>;

	/**
	 * 	@brief	This method set the system type
	*/
	inline void setSystemType(bool isMultiCp) { m_IsMultiCP = isMultiCp; };

	/** @brief addBlockSender method
	 *
	 *	This method inserts a new block sender
	 *
	 *	@param fileName : CP File name
	 *	@param cpName :  CP name
	 *	@param blockSize : block size to send
	 *	@param subFilePath : physical path of the first subfile to transfer
	 *
	*/
	bool addBlockSender(const std::string& fileName, const std::string& volumeName, const std::string& cpName,
						const int blockSize, const std::string& subFilePath );

	/** @brief updateBlockSenderState method
	 *
	 *	This method updates the last subfile that can be transfered
	 *
	 *	@param fileName : CP File name
	 *	@param cpName :  CP name
	 *	@param lastReadySubfile : last closed subfile
	 *
	*/
	void updateBlockSenderState(const std::string& fileName, const std::string& cpName, const unsigned long lastReadySubfile);

	/** @brief removeBlockSender method
	 *
	 *	This method updates the last subfile that can be transfered
	 *
	 *	@param fileName : CP File name
	 *	@param cpName :  CP name
	 *
	*/
	void removeBlockSender(const std::string& fileName, const std::string& cpName);

	/**
	 *	@brief shutDown
	 *
	 *	This method terminates all block senders objects
	*/
	void shutDown();

 private:

	/**
	 * 	@brief	Constructor of FMS_CPF_BlockTransferMgr class
	*/
	FMS_CPF_BlockTransferMgr();

	/**
	 * 	@brief	Destructor of FMS_CPF_BlockTransferMgr class
	*/
	virtual ~FMS_CPF_BlockTransferMgr();

	/**
	 * 	@brief	m_IsMultiCP
	 *
	 * 	Flag of system type (SCP/MCP)
	*/
	bool m_IsMultiCP;

	/**
	 * 	@brief	m_SchedulersMap
	 *
	 * 	Map of pair CP File name and its Scheduler.
	 *  On Multi-CP System, all CP Files with same name, but defined in different CP File System,
	 *  have the same scheduler.
	*/
	std::map<std::string, FMS_CPF_BlockSender_Scheduler*> m_SchedulersMap;

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

typedef ACE_Singleton<FMS_CPF_BlockTransferMgr, ACE_Recursive_Thread_Mutex> BlockTransferMgr;

#endif /* FMS_CPF_BLOCKTRANSFERMGR_H_ */
