/*
 * * @file fms_cpf_cmdhandlerexit.cpp
 *	@brief
 *	Class method implementation for CPF_CmdHandlerExit_Request.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_cmdhandlerexit.h module
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
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_cmdhandlerexit.h"

/*============================================================================
	ROUTINE: CPF_CmdHandlerExit_Request
 ============================================================================ */
CPF_CmdHandlerExit_Request::CPF_CmdHandlerExit_Request(ACE_Future<int>& result)
:m_result(result)
{

}

/*============================================================================
	ROUTINE: call
 ============================================================================ */
int CPF_CmdHandlerExit_Request::call()
{
	// unlocking the action caller
	m_result.set(0);

	// signal to the cmdHandler to terminate the execution
	return -1;
}

