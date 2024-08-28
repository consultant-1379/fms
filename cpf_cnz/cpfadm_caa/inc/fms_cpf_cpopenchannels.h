/*
 * * @file fms_cpf_cpopenchannels.h
 *	@brief
 *	Header file for FMS_CPF_CpOpenChannels class.
 *  This module contains the declaration of the class FMS_CPF_CpOpenChannels.
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

#ifndef FMS_CPF_CPOPENCHANNELS_H_
#define FMS_CPF_CPOPENCHANNELS_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include <ace/Task_T.h>
#include <ace/Synch.h>
#include <ace/RW_Thread_Mutex.h>
#include <ace/Singleton.h>
#include <ace/Atomic_Op_T.h>

#include <list>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class ACE_Message_Block;
class FMS_CPF_CpChannel;
class ACS_DSD_Session;
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_CpOpenChannels : public ACE_Task<ACE_MT_SYNCH>
{
 public:

	friend class ACE_Singleton<FMS_CPF_CpOpenChannels, ACE_Recursive_Thread_Mutex>;

	/** @brief addCpChannel method
	 *
	 *	This method is called by FMS_CPF_CpChannelMgr when input events occur on DSD.
	 *	It adds a new cp channel in the list of opened channels
	 *
	 *	@param dsdSession: Accepted DSD session stream
	*/
	bool addCpChannel( ACS_DSD_Session* dsdSession);

	/** @brief removeCpChannel method
	 *
	 *	This method is called by FMS_CPF_CpChannel object when the connection owned by it
	 *	has been closed.
	 *
	 *	@param FMS_CPF_CpChannel: Pointer of the FMS_CPF_CpChannel object to remove.
	*/
	bool removeCpChannel( FMS_CPF_CpChannel* cpChannel);

	/** @brief putMsgToCpChannel method
	 *
	 *	This method puts a message in the message queue of CP channel
	 *
	 *	@param FMS_CPF_CpChannel: Pointer of the FMS_CPF_CpChannel object.
	 *
	 *	@param message: message to put.
	*/
	bool putMsgToCpChannel(FMS_CPF_CpChannel* cpChannel, ACE_Message_Block* message);

	/** @brief addCpChannel method
	 *
	 *	This method is called by FMS_CPF_CpChannelMgr object on CPF service closure
	 *
	*/
	void channelsShutDown();

	/**
	 * 	@brief  Run by a daemon thread
	*/
	virtual int svc(void);

	/**
	 * 	@brief	This method initializes a task and prepare it for execution
	*/
	virtual int open(void *args = 0);

	/** @brief setSystemType method
	 *
	 *	This method is called by FMS_CPF_CpChannelMgr object on to set environment type
	 *
	*/
	inline void setSystemType(const bool sysType){ m_IsMultiCP = sysType; };

 private:
	/** @brief addCpChannel method
	 *
	 *	This method is called internally to physical channel deletion
	 *
	 *	@param FMS_CPF_CpChannel: Pointer of the FMS_CPF_CpChannel object to remove.
	*/
	bool deleteCpChannel( FMS_CPF_CpChannel* cpChannel);

	/**
	 * 	@brief	Constructor of FMS_CPF_CpOpenChannels class
	*/
	FMS_CPF_CpOpenChannels();

	/**
	 * 	@brief	Destructor of FMS_CPF_CpOpenChannels class
	*/
	virtual ~FMS_CPF_CpOpenChannels();

	/**
	 * 	@brief	m_IsMultiCP
	 *
	 * 	Flag of system type (SCP/MCP)
	*/
	bool m_IsMultiCP;

	/**
	   @brief  	m_channelsList, list of active channels
	*/
	std::list<FMS_CPF_CpChannel*> m_channelsList;

	/**
	   @brief  	m_serviceShutDown, indicates a global shutdown
	*/
	ACE_Atomic_Op <ACE_Thread_Mutex, bool> m_serviceShutDown;

	/**
	 * 	@brief	m_mutex
	 *
	 * 	Mutex for internal list access
	*/
	ACE_RW_Thread_Mutex m_mutex;

	/**
	 * 	@brief	fms_cpf_OpenChsTrace: trace object
	*/
	ACS_TRA_trace* fms_cpf_OpenChsTrace;
};

typedef ACE_Singleton<FMS_CPF_CpOpenChannels, ACE_Recursive_Thread_Mutex> CpChannelsList;

#endif /* FMS_CPF_CPOPENCHANNELS_H_ */
