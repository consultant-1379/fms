/*
 * * @file fms_cpf_infinitesubfiles_manager.h
 *	@brief
 *	Header file for FMS_CPF_InfiniteSubFiles_Manager class.
 *  This module contains the declaration of the class FMS_CPF_InfiniteSubFiles_Manager.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-14
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
 *	| 1.0.0  | 2011-06-14 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_INFINITESUBFILES_MANAGER_H_
#define FMS_CPF_INFINITESUBFILES_MANAGER_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include <ace/Singleton.h>
#include <ace/Task_T.h>
#include <ace/Synch.h>
#include <ace/RW_Thread_Mutex.h>

#include <list>
#include <string>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */

class FMS_CPF_RTO_InfiniteSubFile;
class FMS_CPF_CmdHandler;
class ACS_TRA_trace;
class ACE_Barrier;
class CPF_Message_Base;
/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_InfiniteSubFiles_Manager: public ACE_Task<ACE_MT_SYNCH>
{

 public:

	friend class ACE_Singleton<FMS_CPF_InfiniteSubFiles_Manager, ACE_Recursive_Thread_Mutex>;

	/**
	 * 	@brief  Run by a daemon thread
	*/
	virtual int svc(void);

	/**
	 * 	@brief	This method initializes a task and prepare it for execution
	*/
	virtual int open(void *args = 0);

	/** @brief setSystemParameters method
	 *
	 * 	This method copies the system parameters
	 *
	 * 	@param systemType : Flag of system type (SCP/MCP)
	 *
	 * 	@param mapConfig : a list of defined CP/BC name
	 *
	 *  @remarks Remarks
	 */
	void setSystemParameters(FMS_CPF_CmdHandler* cmdHandler, const bool systemType, const std::list<std::string>& listOfCP );

	/**
	   @brief  	This method returns the pointer to sync object
	*/
	inline ACE_Barrier* getSyncObj(){ return m_ThreadsSyncShutdown; };

	/**
	   @brief  	This method get the internal RT owner of infinite subfile
	*/
	inline FMS_CPF_RTO_InfiniteSubFile* getISFOwner(){ return m_RTO_InfiniteSubFile; };

	/**
	   @brief  	This method get the stop handle to terminate the internal thread
	*/
	void getStopHandle(int& stopFD){ stopFD = m_StopEvent; };

	/**
	   @brief  	This method get IMM reset handle
	*/
	void getImmResetHandle(int& resetFD){ resetFD = m_ImmResetEvent; };

	/**
	 * 	@brief	This method resets the IMM run-time owner
	*/
	void immOwnerReset();


	/**
	 * 	@brief	This method stops and waits threads termination
	*/
	void shutdown();

	/**
	 * 	@brief	fms_cpf_roManTrace: trace object
	*/
	ACS_TRA_trace* fms_cpf_roManTrace;

 private:

	enum { N_THREADS = 2 };

	/**
	 * 	@brief	Constructor of FMS_CPF_InfiniteSubFiles_Manager class
	*/
	FMS_CPF_InfiniteSubFiles_Manager();

	/**
	 * 	@brief	Destructor of FMS_CPF_InfiniteSubFiles_Manager class
	*/
	virtual ~FMS_CPF_InfiniteSubFiles_Manager();

	/**
	 * 	@brief	This method signals to the internal thread to stop
	*/
	bool stopInternalThread();

	/**
		@brief	This method implement a thread to handle the update request on InfiniteSubFile object

		@return	ACE_THR_FUNC_RETURN

		@exception	none
	*/
	static ACE_THR_FUNC_RETURN ISF_UpdateCallbackHnd(void* ptrParam);

	/**
		@brief	This method handles the received message

		@exception	none
	*/
	void handleMessageResult(int result, CPF_Message_Base* message);

	/**
		@brief	m_RTO_InfiniteSubFile : Runtime owner of infinite subfile
	*/
	FMS_CPF_RTO_InfiniteSubFile* m_RTO_InfiniteSubFile;

	/**
		@brief	m_ListOfCP : list of CP/BC names
	*/
	std::list<std::string> m_ListOfCP;

	/**
	 * 	@brief	m_IsMultiCP
	 *
	 * 	Flag of system type (SCP/MCP)
	*/
	bool m_IsMultiCP;

	/**
	 * 	@brief	m_ThreadsSyncShutdown: sync with internal thread shutdown
	*/
	ACE_Barrier* m_ThreadsSyncShutdown;

	/**
	   @brief  	m_StopEvent
	*/
	int m_StopEvent;

	/**
	   @brief  	m_ImmResetEvent
	*/
	int m_ImmResetEvent;

	/**
	 * 	@brief	m_mutex
	 *
	 * 	Mutex for internal sync
	*/
	ACE_RW_Thread_Mutex m_mutex;

	/**
	    @brief	m_cmdHandler :
	*/
	FMS_CPF_CmdHandler* m_cmdHandler;

};

typedef ACE_Singleton<FMS_CPF_InfiniteSubFiles_Manager, ACE_Recursive_Thread_Mutex> InfiteSubFileHndl;

#endif /* FMS_CPF_INFINITESUBFILES_MANAGER_H_ */
