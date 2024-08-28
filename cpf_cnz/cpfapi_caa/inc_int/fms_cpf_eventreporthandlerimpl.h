/*
 * *@file fms_cpf_eventreporthandlerimpl.h
 *	This module contains the declaration of the class FMS_CPF_eventReporthandlerimpl.
 *
 *  Created on: Oct 17, 2011
 *      Author: enungai
 *
 *
 *	COPYRIGHT Ericsson AB, 2010
 *	All rights reserved.
 *
 *	The information in this document is the property of Ericsson.
 *	Except as specifically authorized in writing by Ericsson, the receiver of
 *	this document shall keep the information contained herein confidential and
 *	shall protect the same in whole or in part from disclosure and dissemination
 *	to third parties. Disclosure and dissemination to the receivers employees
 *	shall only be made on a strict need to know basis.
 *
 *	REVISION INFO
 *	+========+============+==============+=====================================+
 *	| REV    | DATE       | AUTHOR       | DESCRIPTION                         |
 *	+========+============+==============+=====================================+
 *	| 1.0.0  | 2011-10-20 | enungai      | File created.                       |
 *	+========+============+==============+=====================================+*/


#ifndef FMS_CPF_EVENTREPORTHANDLERIMPL_H
#define FMS_CPF_EVENTREPORTHANDLERIMPL_H

#include <map>
#include <utility>
#include <string>
#include "acs_aeh_evreport.h"
#include "fms_cpf_eventreport.h"

class ACS_TRA_trace;

class FMS_CPF_EventReportHandlerImpl
{

  public:
      // Constructor
	  FMS_CPF_EventReportHandlerImpl();

      // Destructor
      virtual ~FMS_CPF_EventReportHandlerImpl();


      //	Parameters:
      //	IN: ex		fms_afp_event &
      //	OUT: pex		fms_afp_event *&
      //
      //	Return value:
      //	int:
      int filter (FMS_CPF_EventReport &ex, FMS_CPF_EventReport *&pex);

      //	Parameters:
      //	IN: ex		an fms_afp_event
      //	IN: forced		bool flag
      //
      //	Return value:
      //	bool: true if event is reported
      //	false if event is filtered out
      bool event (FMS_CPF_EventReport &ex, bool forced = false);

      //	Parameters:
      //	IN: ex		an fms_afp_event
      //	IN: severity	char string. one of
      //	A1, A2, A3, O1, O2
      //
      //	Return value:
      //	bool
      bool alarm (FMS_CPF_EventReport &ex, const char *severity);

      //	Parameters:
      //	IN: ex		an fms_afp_event
      //
      //	Return value:
      //	bool: true if cease was effective
      //	false if cease was ineffective
      bool cease (FMS_CPF_EventReport &ex);

      //	Parameters:
      //	IN: anevnr	event number 0-99
      //	IN: afile		a file name (use __FILE__ )
      //	IN: aline		a line number (use __LINE__ )
      //	IN: problemdata	a free comment
      //	IN: problemtext	formatted text
      //
      //	Return value:
      //	bool: true if event was reported
      //	false if event was not reported
      bool report (int anerror, acs_aeh_specificProblem eventnumber, 	// event number 0-99
                   const char *afile, 	// source file where problem appeared
                   int aline, 	// code line where fault appeared
                   const char *problemdata, 	// free text
                   const char *problemtext	// formatted problem text
      );


  private:
    // Data Members for Class Attributes

      acs_aeh_evreport *evreport;
      std::multimap< std::string,FMS_CPF_EventReport* > eventMap;
      std::pair< std::string,FMS_CPF_EventReport* > eventPair;
      ACS_TRA_trace *fms_cpf_eventreporthandlerimpl_trace;

};


#endif
