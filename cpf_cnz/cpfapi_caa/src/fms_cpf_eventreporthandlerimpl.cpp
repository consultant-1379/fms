/*
 * * @file fms_cpf_eventreporthandlerimpl.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_EventReportHandlerimpl.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_eventreporthandlerimpl.h module
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
#include <ace/OS.h>
#include <string>
#include <map>
#include "fms_cpf_eventreport.h"
#include "fms_cpf_eventreporthandler.h"
#include "fms_cpf_eventreporthandlerimpl.h"
#include "fms_cpf_api_trace.h"
#include "ACS_TRA_trace.h"
#include <sstream>

#define objectOfReferenceMaxLen 2048


FMS_CPF_EventReportHandlerImpl::FMS_CPF_EventReportHandlerImpl()
{
	fms_cpf_eventreporthandlerimpl_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventReportHandlerImpl");
	evreport = new acs_aeh_evreport;

}


FMS_CPF_EventReportHandlerImpl::~FMS_CPF_EventReportHandlerImpl()
{
	delete evreport;
	if(NULL != fms_cpf_eventreporthandlerimpl_trace)
	  delete fms_cpf_eventreporthandlerimpl_trace;

}


int FMS_CPF_EventReportHandlerImpl::filter (FMS_CPF_EventReport &ex, FMS_CPF_EventReport *&pex)
{


	pex = &ex;	// default, point to incoming

	const int OVER_AGE	= 5;		// minutes
	const int ANCIENT   = 20;


	std::stringstream msg_stream;
	msg_stream << "This could be a cyclic event. It will not be repeated if it occurs again within "
			<< OVER_AGE << " minutes." << std::ends;
	std::string cyclicMsg = msg_stream.str();


	int over_aged		= OVER_AGE*60;	// seconds
	int ancient			= ANCIENT*60;

	std::multimap<std::string, FMS_CPF_EventReport*>::iterator itr;

	// Convert the eventCode to string for the detail info
	std::stringstream event_stream;
	event_stream << ex.eventCode() << std::ends;
	std::string event_to_log = event_stream.str();
	// find events in map
    int number = eventMap.count(event_to_log);
	itr = eventMap.find(event_to_log);
    
    // Get 'correct' event (with same "object of reference")
    while (number-- > 0)
    {
        if (strcmp(ex.objectOfReference(), (*itr).second->objectOfReference()) == 0)
        {
            break;
        }
        itr++;
    }
    if (number < 0) itr = eventMap.end();

	if (itr == eventMap.end() )
	{
	  // did not find ex in map, is new
      if (ex.kind() == FMS_CPF_EventReport::EV_ALARM)		// report alarm
	  {
    	// new alarm. go ahead and report. will be inserted
        TRACE(fms_cpf_eventreporthandlerimpl_trace,"%s", "eventhandler: new alarm report");
	  }
	  if (ex.kind() == FMS_CPF_EventReport::EV_CEASING)	// cease alarm
	  {
		TRACE(fms_cpf_eventreporthandlerimpl_trace,"%s", "eventhandler: alarm not reported, do not cease");
		return 0; // not found. do not report
	  }
	}
	else
	{
		TRACE(fms_cpf_eventreporthandlerimpl_trace, "eventhandler: found %s", event_to_log.c_str());
        if (ex.kind() == FMS_CPF_EventReport::EV_ALARM)	// report alarm
		{
		  TRACE(fms_cpf_eventreporthandlerimpl_trace,"%s", "eventhandler: alarm already reported, do not report");
		  return 0;								// already reported, do not report
		}
		if (ex.kind() == FMS_CPF_EventReport::EV_CEASING)	// cease alarm
		{
		  if ((*itr).second->kind() == FMS_CPF_EventReport::EV_ALARM)	// can only cease alarm
		  {
			  TRACE(fms_cpf_eventreporthandlerimpl_trace,"%s", "eventhandler: alarm found, cease");
			  // copy vital information from alarm
			  pex->setErrorCode((*itr).second->errorCode());
			  pex->setOsError((*itr).second->osError());
			  pex->setProblemData((*itr).second->problemData());
			  pex->setProblemText((*itr).second->problemText());
			  delete (*itr).second;		// destruct object
			  eventMap.erase(itr);		// erase from map
			  return 1;					// found. cease will be effective
	 	  }
		  else
		  {
			TRACE(fms_cpf_eventreporthandlerimpl_trace,"%s", "eventhandler: non-alarm found, can not cease");
			return 0;
		  }
	    }
	}

	// avoid duplicates...

	if (itr == eventMap.end() )		// new, first time event!
	{
		pex = new FMS_CPF_EventReport(ex);	// copy incoming
		TRACE(fms_cpf_eventreporthandlerimpl_trace,"%s", "first time");
		TRACE(fms_cpf_eventreporthandlerimpl_trace, "new %s", pex->str().c_str());
		pex->incCounter();		// increment counter
		TRACE(fms_cpf_eventreporthandlerimpl_trace,"%s", "insert in map");
		eventPair.first = event_to_log;
		eventPair.second = pex;
		eventMap.insert(eventPair);
		TRACE(fms_cpf_eventreporthandlerimpl_trace, "mapsize = %d (pex=%X)", eventMap.size(), pex);
	}
	else
	{
		if ( (*itr).second->age() > ancient ) 	// very very old, regard as new!
		{
			TRACE(fms_cpf_eventreporthandlerimpl_trace, "%s","times>1 & ancient!");
			(*itr).second->setProblemText("");		// no extra info
			(*itr).second->setCounter(1);					// set as new would be
			(*itr).second->resetAge();						// count from now
			TRACE(fms_cpf_eventreporthandlerimpl_trace, "ancient %s", (*itr).second->str().c_str());
		}
		else		// not THAT old
		{
  		  if ((*itr).second->counter() == 1)	// second time, issue warning for cyclic events
		  {

			(*itr).second->setProblemText(cyclicMsg.c_str());		// warn for cyclic
			(*itr).second->incCounter();					// increment counter
			TRACE(fms_cpf_eventreporthandlerimpl_trace, "times == 1 %s", (*itr).second->str().c_str());
            std::string buf2 (pex->problemText());
            buf2 = " " + cyclicMsg;
            pex->setProblemText(buf2.c_str());

	 	  }
		  else	// 2 or more times before! can be cyclic
		  {
		    if ((*itr).second->age() > over_aged)	// very old, regard as almost new!
		    {
		      std::stringstream buf_stream;
		      buf_stream << (*itr).second->counter() << " times since last cyclic warning " << std::ends;
		      std::string buf = buf_stream.str();
			  (*itr).second->setProblemText(buf.c_str());	// warn that nn event were identical
			  (*itr).second->setCounter(1);				// set as new
			  (*itr).second->resetAge();		// count from now
			  TRACE(fms_cpf_eventreporthandlerimpl_trace, "overaged %s", (*itr).second->str().c_str());
              std::string buf2(pex->problemText());
              buf2 = " " + buf;
              pex->setProblemText(buf2.c_str());
		    }
		    else
		    {
			   TRACE(fms_cpf_eventreporthandlerimpl_trace, "%s","times>1 but not overaged");
			   (*itr).second->incCounter();		// increment counter
			   TRACE(fms_cpf_eventreporthandlerimpl_trace, "not overaged %s", (*itr).second->str().c_str());
			   return 0;		// do not produce an event
		    }
		 }
	   }
	 }

	return 1;	// produce event

}

//	----------------------------------------
//	                    event
//	----------------------------------------
bool FMS_CPF_EventReportHandlerImpl::event (FMS_CPF_EventReport &ex, bool forced)
{
	TRACE(fms_cpf_eventreporthandlerimpl_trace, "%s","entering event()");

    ex.setKind(FMS_CPF_EventReport::EV_EVENT);
	FMS_CPF_EventReport *pex = 0;

	char process[80]= {0};
	char objectr[objectOfReferenceMaxLen]= {0};
	long specific = 0;

	if (filter(ex, pex) == 0)	// do not report
	{
  	  if (forced)
	  {
  		TRACE(fms_cpf_eventreporthandlerimpl_trace, "%s","forced");
		pex->setProblemText("");	// reset cyclic warning if forced event
	  }
	  else
	  {
		TRACE(fms_cpf_eventreporthandlerimpl_trace, "%s","filtered out...");
		return false;	// filtered out and not forced
	  }
	}

	// filter or forced flag said: go ahead

	ACE_OS::snprintf(process,80,	"%s:%d", FMS_CPF_EventReportHandler::processName.c_str(),getpid());

    if ((std::string)(pex->objectOfReference()) == "")
    {
	    ACE_OS::snprintf(objectr, objectOfReferenceMaxLen, "%s/%d", FMS_CPF_EventReportHandler::processName.c_str(),getpid());
    }
    else
    {
        (void) ACE_OS::strncpy(objectr, pex->objectOfReference(), objectOfReferenceMaxLen-1);
        objectr[objectOfReferenceMaxLen-1] = '\0';
    }

	specific = pex->eventCode();

	if (evreport->sendEventMessage(process,  // process name
											 specific,					// specific problem
											 "EVENT",					// perceived severity
											 pex->probableCause(),	    // probably cause
											 "APZ",						// object class of reference
											 objectr,					// object of reference
											 pex->problemData(),	// problem data
											 pex->problemText())	== ACS_AEH_error) // problem text
		{
			int err_code = evreport->getError();
			switch (err_code)
			{
			  case ACS_AEH_syntaxError:
				    TRACE(fms_cpf_eventreporthandlerimpl_trace, "eventhandler: AEH Syntax Error\n%s", pex->str().c_str());
				    break;

			  case ACS_AEH_eventDeliveryFailure:
				    TRACE(fms_cpf_eventreporthandlerimpl_trace, "eventhandler: AEH Event Delivery Failure %s", pex->str().c_str());
					break;
			}
			return false;	// did not send event
		}

	return true;	// event reported
}

//	----------------------------------------
//	                    alarm
//	----------------------------------------
bool FMS_CPF_EventReportHandlerImpl::alarm (FMS_CPF_EventReport &ex, const char *severity)
{
	TRACE(fms_cpf_eventreporthandlerimpl_trace, "%s","entering alarm()");

	ex.setKind(FMS_CPF_EventReport::EV_ALARM);

	FMS_CPF_EventReport *pex = 0;

  char process[80] = {0};
  char objectr[objectOfReferenceMaxLen] = {0};
  long specific = 0;

  if (filter(ex, pex) == 0)
  {
    return false;			// duplicate found, do not report
  }

  // filter said: go ahead
  TRACE(fms_cpf_eventreporthandlerimpl_trace, "pex=%X", pex);

  ACE_OS::snprintf(process,	80, "%s:%d", FMS_CPF_EventReportHandler::processName.c_str(),getpid());

  if ((std::string)pex->objectOfReference() == "")
  {
    ACE_OS::snprintf(objectr,objectOfReferenceMaxLen,"%s/%d", FMS_CPF_EventReportHandler::processName.c_str(),	getpid());
  }
  else
  {
    (void) ACE_OS::strncpy(objectr, pex->objectOfReference(), objectOfReferenceMaxLen-1);
    objectr[objectOfReferenceMaxLen-1] = '\0';
  }

  specific = pex->eventCode();


  if (evreport->sendEventMessage(process,					// process name
																specific,					// specific problem
																severity,					// perceived severity
																pex->probableCause(),	// probable cause
																"APZ",						// object class of reference
																objectr,					// object of reference
																pex->problemData(),	// problem data
																pex->problemText())	== ACS_AEH_error) // problem text
  {
    int err_code = evreport->getError();
	switch (err_code)
	{
	  case ACS_AEH_syntaxError:
		  TRACE(fms_cpf_eventreporthandlerimpl_trace, "eventhandler: AEH Syntax Error %s", pex->str().c_str());
		  break;

	  case ACS_AEH_eventDeliveryFailure:
		  TRACE(fms_cpf_eventreporthandlerimpl_trace, "eeventhandler: AEH Event Delivery Failure\n%s", pex->str().c_str());
		  break;
	}
	return false;		// alarm not reported
  }

	return true;	// alarm reported

}

//	----------------------------------------
//	                    cease
//	----------------------------------------
bool FMS_CPF_EventReportHandlerImpl::cease (FMS_CPF_EventReport &ex)
{
	TRACE(fms_cpf_eventreporthandlerimpl_trace, "%s","entering cease()");

	ex.setKind(FMS_CPF_EventReport::EV_CEASING);

	FMS_CPF_EventReport *pex = 0;

  char process[80] = {0};
  char objectr[objectOfReferenceMaxLen]= {0};
  long specific = 0;
	
  if (filter(ex, pex) == 0)	// alarm not found, do not cease
  {
	return false;
  }

	// filter said: go ahead

	ACE_OS::sprintf(process,	"%s:%d", FMS_CPF_EventReportHandler::processName.c_str(),	getpid());

    if ((std::string)pex->objectOfReference() == "")
    {
	    ACE_OS::snprintf(objectr,objectOfReferenceMaxLen,"%s/%d", FMS_CPF_EventReportHandler::processName.c_str(),	getpid());
    }
    else
    {
        (void) ACE_OS::strncpy(objectr, pex->objectOfReference(), objectOfReferenceMaxLen-1);
        objectr[objectOfReferenceMaxLen-1] = '\0';
    }

	specific = pex->eventCode();

	if (evreport->sendEventMessage(process,					// process name
																specific,					// specific problem
																"CEASING",				// perceived severity = CEASING
																pex->probableCause(),	// probable cause
																"APZ",						// object class of reference
																objectr,					// object of reference
																pex->problemData(),	// problem data
																pex->problemText())	 == ACS_AEH_error) // problem text
	{
	  int err_code = evreport->getError();
	  switch (err_code)
	  {
		case ACS_AEH_syntaxError:
			TRACE(fms_cpf_eventreporthandlerimpl_trace, "eventhandler: AEH Syntax Error %s", pex->str().c_str());
			break;

		case ACS_AEH_eventDeliveryFailure:
			TRACE(fms_cpf_eventreporthandlerimpl_trace, "eventhandler: AEH Event Delivery Failure %s", pex->str().c_str());
			break;
 	  }
			return false;		// alarm not reported
	}

	return true;
}

//	----------------------------------------
//	                    report
//	----------------------------------------
bool FMS_CPF_EventReportHandlerImpl::report (int anerror, acs_aeh_specificProblem eventnumber, const char *afile, int aline, const char *problemdata, const char *problemtext)
{
	TRACE(fms_cpf_eventreporthandlerimpl_trace, "%s","entering report()");

	char process[80]= {0};
	char objectr[objectOfReferenceMaxLen]= {0};
	char freetext[400] = {0};
	int specific = 0;

	ACE_OS::snprintf(process,80,"%s:%d ", FMS_CPF_EventReportHandler::processName.c_str(),getpid());

	ACE_OS::snprintf(freetext,400,"%s:%d %s",afile,aline,problemdata);

	ACE_OS::snprintf(objectr, objectOfReferenceMaxLen, "%s/%d", FMS_CPF_EventReportHandler::processName.c_str(),getpid());

	specific = eventnumber;

	if (evreport->sendEventMessage(process,					// process name
																specific,					// specific problem
																"EVENT",					// perceived severity
                                                                FMS_CPF_EventReport::defaultProbableCause.c_str(),	// probable cause
																"APZ",						// object class of reference
																objectr,					// object of reference
																freetext,					// problem data
																problemtext) == ACS_AEH_error) // problem text
		{
			FMS_CPF_EventReport ex(anerror, 0, afile, aline, freetext);
			ex.setEventCode(eventnumber);
			ex.setProblemText(problemtext);
			int err_code = evreport->getError();
			switch (err_code)
			{
				case ACS_AEH_syntaxError:
					TRACE(fms_cpf_eventreporthandlerimpl_trace, "eventhandler: AEH Syntax Error %s", ex.str().c_str());
					break;

				case ACS_AEH_eventDeliveryFailure:
					TRACE(fms_cpf_eventreporthandlerimpl_trace, "eventhandler: AEH Event Delivery Failure %s", ex.str().c_str());
					break;
			}
			return false;		// event not reported
		}

	return true;	// return reported

}

