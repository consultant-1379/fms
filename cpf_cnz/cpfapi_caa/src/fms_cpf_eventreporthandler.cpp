/*
 * * @file fms_cpf_eventreporthandler.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_EventReportHandler.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_eventreporthandler.h module
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
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "acs_aeh_evreport.h"
#include "fms_cpf_eventreport.h"
#include "fms_cpf_eventreporthandler.h"
#include "fms_cpf_eventreporthandlerimpl.h"
#include "ACS_TRA_trace.h"

std::string FMS_CPF_EventReportHandler::processName = "";

//FMS_CPF_EventReportHandler* FMS_FCC_EventHandler_R1::instance = 0;


FMS_CPF_EventReportHandler::FMS_CPF_EventReportHandler()
{
	fms_cpf_eventreporthandler_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventReportHandler");
    implementation = new FMS_CPF_EventReportHandlerImpl;
}


FMS_CPF_EventReportHandler::~FMS_CPF_EventReportHandler()
{
	delete implementation;
	if(NULL != fms_cpf_eventreporthandler_trace)
	  delete fms_cpf_eventreporthandler_trace;

}


//	----------------------------------------
//	                    filter
//	----------------------------------------

int FMS_CPF_EventReportHandler::filter (FMS_CPF_EventReport &ex, FMS_CPF_EventReport *&pex)
{
	return implementation->filter(ex, pex);
}

//	----------------------------------------
//	                    event
//	----------------------------------------
bool FMS_CPF_EventReportHandler::event (FMS_CPF_EventReport &ex, bool forced)
{
	return implementation->event(ex, forced);
}

//	----------------------------------------
//	                    alarm
//	----------------------------------------
bool FMS_CPF_EventReportHandler::alarm (FMS_CPF_EventReport &ex, const char *severity)
{
	return implementation->alarm(ex, severity);
}

//	----------------------------------------
//	                    cease
//	----------------------------------------
bool FMS_CPF_EventReportHandler::cease (FMS_CPF_EventReport &ex)
{
	return implementation->cease(ex);
}

//	----------------------------------------
//	                    report
//	----------------------------------------
bool FMS_CPF_EventReportHandler::report (int anerror, acs_aeh_specificProblem eventnumber, const char *afile, int aline, const char *problemdata, const char *problemtext)
{
	return implementation->report(anerror, eventnumber, afile, aline, problemdata, problemtext);
}

//	----------------------------------------
//	                    Instance
//	----------------------------------------
/*FMS_CPF_EventReportHandler* FMS_CPF_EventReportHandler::Instance ()
{
	if (instance == 0)
		{
			instance = new FMS_CPF_EventReportHandler();
		}

	return instance;

}
*/
