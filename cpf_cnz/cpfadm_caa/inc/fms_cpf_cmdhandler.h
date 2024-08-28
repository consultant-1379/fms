/*
 * * @file fms_cpf_cmdhandler.h
 *	@brief
 *	Header file for FMS_CPF_CmdHandler class.
 *  This module contains the declaration of the class FMS_CPF_CmdHandler.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-23
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
 *	| 1.0.0  | 2011-06-23 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_CMDHANDLER_H_
#define FMS_CPF_CMDHANDLER_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include <ace/Task.h>
#include <ace/Activation_Queue.h>
#include <ace/Method_Request.h>

class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FMS_CPF_CmdHandler: public ACE_Task_Base {
 public:
	/**
		@brief	Constructor of FMS_CPF_CmdHandler class
	*/
	FMS_CPF_CmdHandler();

	/**
		@brief	Destructor of FMS_CPF_CmdHandler class
	*/
	virtual ~FMS_CPF_CmdHandler();

	/**
	   @brief  		Run by a daemon thread
	*/
	virtual int svc(void);

	/**
	   @brief  		This method initializes a task and prepare it for execution
	*/
	virtual int open (void *args = 0);

	/**
	   @brief  		This method enqueue a command in the queue
	*/
	int enqueue(ACE_Method_Request* cmdRequest);

 private:

	/**
	   @brief  	svc_run: svc state flag
	*/
	bool svc_run;

	/**
	   @brief  	m_Activation_Queue: queue of command to execute
	*/
	ACE_Activation_Queue m_Activation_Queue;

	ACS_TRA_trace* fms_cpf_cmdhandlerTrace;

};

#endif /* FMS_CPF_CMDHANDLER_H_ */
