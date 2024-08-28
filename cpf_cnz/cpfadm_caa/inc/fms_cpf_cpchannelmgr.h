/*
 * * @file fms_cpf_cpchannelmgr.h
 *	@brief
 *	Header file for FMS_CPF_CpChannelMgr class.
 *  This module contains the declaration of the class FMS_CPF_CpChannelMgr.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-10
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
 *	| 1.0.0  | 2011-11-10 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_CPCHANNELMGR_H_
#define FMS_CPF_CPCHANNELMGR_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include <ace/Task_T.h>
#include <ace/Synch.h>
#include <ace/Handle_Set.h>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class ACS_DSD_Server;
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_CpChannelMgr : public ACE_Task<ACE_MT_SYNCH>
{
 public:

	/**
	 * 	@brief	Constructor of FMS_CPF_CpChannelMgr class
	*/
	FMS_CPF_CpChannelMgr();

	/**
	 * 	@brief	Destructor of FMS_CPF_CpChannelMgr class
	*/
	virtual ~FMS_CPF_CpChannelMgr();

	/**
	 * 	@brief  Run by a daemon thread
	*/
	virtual int svc(void);

	/**
	 * 	@brief	This method initializes a task and prepare it for execution
	*/
	virtual int open(void *args = 0);

	/** @brief handle_input method
	 *
	 *	This method is called by reactor when input events occur on DSD
	 *
	 *	@param fd file descriptor
	*/
	virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);

	/** @brief handle_close method
	 *
	 *	This method is called when a <handle_*()> method returns -1 or when the
	 *	<remove_handler> method is called on an ACE_Reactor
	 *
	 *	@param handle file descriptor
	 *	@param close_mask indicates which event has triggered
	*/
	virtual int handle_timeout(const ACE_Time_Value& tv, const void*);

	/** @brief stopChannelMgr method
	 *
	 *	This method close channel manager, and wait for a graceful closure of all threads
	 *
	*/
	void stopChannelMgr();

	/** @brief setSystemType method
	 *
	 *	This method sets the system type Single CP / Multi CP
	 *
	*/
	inline void setSystemType(bool sysType) { m_IsMultiCP = sysType; };

 private:

	/** @brief initCPFServer method
	 *
	 *	This method publish CPF server and register its handles into reactor.
	 *
	 *	return true on success, false otherwise.
	*/
	bool initCPFServer();

	/** @brief publishCPFServer method
	 *
	 *	This method initialize and publish the CPF server towards DSD
	 *
	 *	return true on success, false otherwise.
	*/
	bool publishCPFServer();

	/**
	 * @brief  registerDSDHandles method
	 *
	 * 	   This method register the DSD handles in the reactor
	*/
	bool registerDSDHandles();

	/** @brief removeCPFServer method
	 *
	 *	This method unregister CPF server.
	 *
	 *	return true on success, false otherwise.
	*/
	void removeCPFServer();

	/**
	 * 	@brief	m_IsMultiCP
	 *
	 * 	Flag of system type (SCP/MCP)
	*/
	bool m_IsMultiCP;

	/**
	 * 	@brief	m_serverPublished
	*/
	bool m_serverPublished;

	/**
	 * 	@brief	m_handleRegistered
	*/
	bool m_handlesRegistered;

	/**
	 * 	@brief	m_timerId
	*/
	long m_timerId;

	/**
		@brief  m_DsdServer : handler of CP connection
	*/
	ACS_DSD_Server* m_DsdServer;


	ACE_Handle_Set m_dsdHandleSet;

	/**
		@brief  cmdListenerReactor
	*/
	ACE_Reactor* m_DsdServerReactor;

	/**
	 * 	@brief	fms_cpf_ChMgrTrace: trace object
	*/
	ACS_TRA_trace* fms_cpf_ChMgrTrace;
};

#endif /* FMS_CPF_CPCHANNELMGR_H_ */
