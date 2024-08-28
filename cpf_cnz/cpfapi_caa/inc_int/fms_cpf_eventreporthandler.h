/*
 * *@file fms_cpf_eventreporthandler.h
 *	This module contains the declaration of the class FMS_CPF_eventReporthandler.
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

#ifndef FMS_CPF_EVENTREPORTHANDLER_H
#define FMS_CPF_EVENTREPORTHANDLER_H

#include <ace/Singleton.h>
#include <ace/Synch.h>
#include "acs_aeh_evreport.h"
#include <string>
#include "fms_cpf_eventreport.h"
/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class ACS_TRA_trace;


/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */



class FMS_CPF_EventReportHandlerImpl;

class FMS_CPF_EventReportHandler
{

 public:

	friend class ACE_Singleton<FMS_CPF_EventReportHandler, ACE_Recursive_Thread_Mutex>;


  FMS_CPF_EventReportHandler();
  // Description:
  //    Creates an empty event handler object.
  //    NOTE! Use the same handler object with Instance() (see below)
  //          when events are to be compared with each other.


  virtual ~FMS_CPF_EventReportHandler();

  int filter (FMS_CPF_EventReport &ex, FMS_CPF_EventReport *&pex);
  // Description:
  //    Applies filter on event.
  // Parameters: 
  //    IN: ex                  Event to be filtered
  //    OUT: pex                Event to be reported
  // Return value: 
  //    Event is EV_CEASING: 
  //    0                       Cease not successful
  //    1                       Cease successful
  //    Other event:
  //    0                       Event filtered out
  //    1                       Event not filtered out
  // Additional information:
  //    If the return value is 1 the event 'pex' should
  //    be reported to AP Event Handler (AEH). Otherwise
  //    it should not be reported.
  //    The event will be filtered out if there has been
  //    two events with the same event number and object of
  //    reference not older than 5 minutes.
  //    The second similar event will have a warning added
  //    to problem text.
  //    When a previous similar event happened between 5 and
  //    20 minutes ago, a text is added to problem text.

  bool event (FMS_CPF_EventReport &ex, bool forced = false);
  // Description:
  //    Calls filter() and creates an event report from an event
  //    and sends it to AEH depending on the result from filter().
  // Parameters: 
  //    IN: ex                  Event to be reported
  //    IN: forced              Event will not be filtered out
  // Return value: 
  //    true                    Event has been reported
  //    false                   Event has been filtered out
  // Additional information:
  //    The process name in the event report is "<name>:<processid>"
  //    where name is 'processName' (see below). The object of
  //    reference in the event report is "<name>/<processid>" if
  //    object of reference from the event is an empty string.

  bool alarm (FMS_CPF_EventReport &ex, const char *severity);
  // Description:
  //    Calls filter() and creates an alarm report from an event
  //    and sends it to AEH depending on the result from filter().
  // Parameters: 
  //    IN: ex                  Alarm to be reported
  //    IN: severity            Level of severity (A1, A2, A3, O1, O2)
  // Return value: 
  //    true                    Alarm has been reported
  //    false                   Alarm has been filtered out
  // Additional information:
  //    The process name in the alarm report is "<name>:<processid>"
  //    where name is 'processName' (see below). The object of
  //    reference in the alarm report is "<name>/<processid>" if
  //    object of reference from the event is an empty string.

  bool cease (FMS_CPF_EventReport &ex);
  // Description:
  //    Calls filter() and ceases an alarm report in AEH from an event
  //    depending on the result from filter().
  // Parameters: 
  //    IN: ex                  Alarm to be ceased
  // Return value: 
  //    true                    Alarm has been ceased
  //    false                   Alarm has not been found
  // Additional information:
  //    The process name in the cease report is "<name>:<processid>"
  //    where name is 'processName' (see below). The object of
  //    reference in the cease report is "<name>/<processid>" if
  //    object of reference from the event is an empty string.

  bool report (int anerror, acs_aeh_specificProblem eventnumber,
               const char *afile,
               int aline,
               const char *problemdata,
               const char *problemtext);
  // Description:
  //    Creates a simple event report and sends it to AEH.
  // Parameters: 
  //    IN: anerror             Error code
  //    IN: eventnumber         Event number 23000 - 23999 (FMS)
  //    IN: afile               Source file where problem appeared
  //    IN: aline               Code line where fault appeared
  //    IN: problemdata         Problem data
  //    IN: problemtext         Problem text
  // Return value: 
  //    true                    Event reported
  //    false                   Event not reported
  // Additional information:
  //    The process name in the event report is "<name>:<processid>"
  //    where name is 'processName' (see below). The object of
  //    reference in the event report is "<name>/<processid>".
  //    The problem data in the event report is 
  //    "<afile>:<aline> <problemdata>".
  //    Probable cause in the event report is 'defaultProbableCause'
  //    from the class FMS_FCC_Event.


  //static FMS_CPF_EventReportHandler * Instance ();
  // Description:
  //    Singleton instanciator. Creates a new FMS_FCC_EventHandler object
  //    if it does not exist otherwise returns previously created object.

  // Points to the single FMS_FCC_EventHandler object
  //static FMS_CPF_EventReportHandler *instance;

  // Process name in event report (see above)
  static std::string processName;

private:

  FMS_CPF_EventReportHandler(const FMS_CPF_EventReportHandler&);

  FMS_CPF_EventReportHandler& operator=(const FMS_CPF_EventReportHandler&);

  FMS_CPF_EventReportHandlerImpl* implementation;

  ACS_TRA_trace *fms_cpf_eventreporthandler_trace;

};


typedef ACE_Singleton<FMS_CPF_EventReportHandler, ACE_Recursive_Thread_Mutex> eventReportHandler_;

#endif
