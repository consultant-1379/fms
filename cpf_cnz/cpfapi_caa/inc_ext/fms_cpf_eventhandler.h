/*
 * * @file fms_cpf_eventhandler.h
 *	@brief
 *	Header file for EventHandler class.
 *  This module contains the declaration of the class EventHandler.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-15
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
 *	| 1.0.0  | 2011-11-15 | qvincon      | File imported/updated.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_EVENTHANDLER_H
#define FMS_CPF_EVENTHANDLER_H

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include <string>
#include "acs_aeh_evreport.h"
#include "fms_cpf_exception.h"
#include <pthread.h>
/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class ACS_TRA_trace;

/*=====================================================================
					Constants used in event reporting
==================================================================== */


/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_EventHandler
{

public:


	static FMS_CPF_EventHandler* instance ();
	 
	 /** @brief event method
	 *
	 *	This method manage the event occurred
	 *
	 *  @param exception: type of exception.
	 */
   void event (const FMS_CPF_Exception& exception);

private:

	
	/**
	 * 	@brief	Constructor FMS_CPF_EventHandler class
	*/
	FMS_CPF_EventHandler ();
	
	/**
	 * 	@brief	Destructor of FMS_CPF_EventHandler class
  */
  ~FMS_CPF_EventHandler ();

	
	/**
	 * 	@brief	fms_cpf_eventhandler_trace: trace object
 	*/
	ACS_TRA_trace *fms_cpf_eventhandler_trace;

	static FMS_CPF_EventHandler* eventHandler_;
	static pthread_mutex_t  eventHandlerMutex;

};

#endif /* FMS_CPF_EVENTHANDLER_H */
