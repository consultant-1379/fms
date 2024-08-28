/*
 * * @file cpf_cmdhandlerexit_request.h
 *	@brief
 *	Header file for CPF_CmdHandlerExit_Request class.
 *  This module contains the declaration of the class CPF_CmdHandlerExit_Request.
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
#ifndef FMS_CPF_CMDHANDLEREXIT_H_
#define FMS_CPF_CMDHANDLEREXIT_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include <ace/Method_Request.h>
#include <ace/Future.h>

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_CmdHandlerExit_Request: public ACE_Method_Request {
 public:

	/**
		@brief	Constructor of CPF_CmdHandlerExit_Request class
	*/
	CPF_CmdHandlerExit_Request(ACE_Future<int>& result);
	/**
		@brief	Destructor of CPF_CmdHandlerExit_Request class
	*/
	virtual ~CPF_CmdHandlerExit_Request(){};

	/**
		@brief	This method implements the action requested
	*/
	virtual int call();

 private:
	ACE_Future<int> m_result;

	// Disallow copying and assignment.
	CPF_CmdHandlerExit_Request (const CPF_CmdHandlerExit_Request &);
 	void operator= (const CPF_CmdHandlerExit_Request &);

};

#endif /* CPF_CMDHANDLEREXIT_REQUEST_H_ */
