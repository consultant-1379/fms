/*
 * * @file fms_cpf_cmdlistener.h
 *	@brief
 *	Header file for FMS_CPF_CmdListener class.
 *  This module contains the declaration of the class FMS_CPF_CmdListener.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-08-04
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
 *	| 1.0.0  | 2011-08-04 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_CMDLISTENER_H_
#define FMS_CPF_CMDLISTENER_H_


/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include <ace/Task.h>
#include <vector>

class FMS_CPF_CmdHandler;
class FMS_CPF_ClientCmdHandler;
class ACS_TRA_trace;
class ACE_Reactor;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FMS_CPF_CmdListener : public ACE_Task_Base
{
 public:

	/**
		@brief	Constructor of FMS_CPF_CmdListener class
	*/
	FMS_CPF_CmdListener();

	/**
		@brief	Destructor of FMS_CPF_CmdHandler class
	*/
	virtual ~FMS_CPF_CmdListener();

	/**
	   @brief  Run by a daemon thread
	*/
	virtual int svc(void);

	/**
	   @brief  This method initializes a task and prepare it for execution
	*/
	virtual int open (void *args = 0);

	/**
	   @brief  This method stop the thread execution
	*/
	int stopListining();

 private:

	/**
	 * @brief  registerDSDHandles method
	 *
	 * 	   This method register the DSD handles in the reactor
	*/
	bool registerDSDHandles();

	/**
	 * @brief  removeDSDHandles method
	 *
	 * 	   This method remove the DSD handles in the reactor
	*/
	void removeDSDHandles();

	/**
	   @brief  cmdListenerReactor
	*/
	ACE_Reactor* m_CmdListenerReactor;

	/**
	   @brief  clientHandler : handler of client connection
	*/
	FMS_CPF_ClientCmdHandler* m_ClientHandler;

	/**
	   @brief  fdList : list of DSD handles
	*/
	std::vector<ACE_HANDLE> fdList;

	/**
	   @brief  DSDInitiate
	*/
	bool m_DSDInitiate;

	/**
	   @brief  	m_StopEvent
	*/
	bool m_StopEvent;

	ACS_TRA_trace* fms_cpf_cmdlistenerTrace;

};

#endif /* FMS_CPF_CMDLISTENER_H_ */
