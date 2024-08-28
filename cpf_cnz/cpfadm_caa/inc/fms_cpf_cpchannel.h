/*
 * * @file fms_cpf_cpchannel.h
 *	@brief
 *	Header file for FMS_CPF_CpChannel class.
 *  This module contains the declaration of the class FMS_CPF_CpChannel.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-11
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
 *	| 1.0.0  | 2011-11-11 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_CPCHANNEL_H_
#define FMS_CPF_CPCHANNEL_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include <ace/Task_T.h>
#include <ace/Synch.h>

#include<ACS_DSD_Macros.h>
#include <string>
#include <ace/RW_Thread_Mutex.h>
/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class ACE_Barrier;
class FMS_CPF_CpOpenChannels;
class ACS_DSD_Session;
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FMS_CPF_CpChannel : public ACE_Task<ACE_MT_SYNCH>
{
 public:

	friend class FMS_CPF_CpOpenChannels;

	/**
	 * 	@brief	Constructor of FMS_CPF_CpChannel class
	*/
	FMS_CPF_CpChannel(ACS_DSD_Session* dsdSession, const bool sysType);

	/**
	 * 	@brief	Destructor of FMS_CPF_CpChannel class
	*/
	virtual ~FMS_CPF_CpChannel();

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
		@brief  This method return the default name of connected CP
	*/
	void getDefaultCpName(std::string& cpName) const;

	/**
		@brief  This method returns the numeric Id of connected CP
	*/
	int getCpId() const { return m_CpId; };

	/**
		@brief  This method returns the number of dsd session handles
	*/
	int getNumOfHandle() const {return m_numOfHandles;};

	/**
		@brief  This method returns the name of the connected CP
	*/
	const char* getConnectedCpName() const { return m_CpName.c_str(); };

 private:

	enum { N_THREADS = 2 };

	/**
		@brief  Internal Thread to handle dsd receive data
	*/
	static ACE_THR_FUNC_RETURN ReceiverCpDataThread(void* ptrParam);

	/**
		@brief  This method is called to close internal receiver thread
	*/
	bool stopReceiverCpDataThread();

	/**
		@brief  This method is called by internal thread to get CP data
	*/
	int receiveMsg();

	/**
		@brief  This method is called to send message reply to CP
	*/
	int sendMsg(const char* msgBuffer, unsigned int msgBufferSize);

	/**
		@brief  This method is called by FMS_CPF_CpOpenChannels on CPF server shutdown
	*/
	void channelShutDown();

	/**
	 * 	@brief	m_dsdSession: accepted DSD stream
	 */
	ACS_DSD_Session* m_dsdSession;

	/**
	 * 	@brief	m_numOfHandles: number of session handles
	 */
	int m_numOfHandles;

	/**
	 * 	@brief	m_dsdHandles: array of handles
	 */
	acs_dsd::HANDLE* m_dsdHandles;

	/**
	 * 	@brief	m_IsMultiCP
	 *
	 * 	Flag of system type (SCP/MCP)
	*/
	const bool m_IsMultiCP;

	/**
	   @brief  	m_shutDownEvent, signaled on CPF service closure
	*/
	int m_closeEvent;

	/**
	   @brief  	m_CpName, Default name of connected CP
	*/
	std::string m_CpName;

	/**
	   @brief  	m_CpId, numeric Id of connected CP
	*/
	int32_t m_CpId;

	/**
	 * 	@brief	m_dsdSessionLock, Mutex for internal dsd session access lock
	*/
	ACE_Thread_Mutex m_dsdSessionLock;

	/**
	 * 	@brief	m_ThreadsSyncShutdown: sync with internal thread shutdown
	*/
	ACE_Barrier* m_ThreadsSyncShutdown;

	/**
	 * 	@brief	fms_cpf_CpChTrace: trace object
	*/
	ACS_TRA_trace* fms_cpf_CpChTrace;

	/**
	 * 	@brief	m_mutex
	 *
	 * 	Mutex for internal sync
	*/
	ACE_RW_Thread_Mutex m_mutex;
};

#endif /* FMS_CPF_CPCHANNEL_H_ */
