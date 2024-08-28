/*
 * * @file fms_cpf_eventhandler.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_EventHandler.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_eventhandler.h module
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
#include <iostream>
#include <sys/procfs.h>
#include <sstream> 
#include <ace/ACE.h>
#include "fms_cpf_api_trace.h"
#include "ACS_TRA_trace.h"
#include "fms_cpf_eventhandler.h"
#include "fms_cpf_eventreporthandler.h"

#include "ACS_TRA_trace.h"

namespace eventFaults
{
  acs_aeh_specificProblem FMS_CPF_SpecificProblem = 23000;
  // Offset for event codes
  acs_aeh_probableCause FMS_CPF_Cause_APfault = "AP INTERNAL FAULT";
}

FMS_CPF_EventHandler *FMS_CPF_EventHandler::eventHandler_ = 0;
pthread_mutex_t FMS_CPF_EventHandler::eventHandlerMutex = PTHREAD_MUTEX_INITIALIZER;
std::string processName = "";



//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------

FMS_CPF_EventHandler::FMS_CPF_EventHandler ()
{
  char procBuf[2048];
  char arrProcId[10];
  std::string strProcId;
  size_t result;

  fms_cpf_eventhandler_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventHandler");

  TRACE(fms_cpf_eventhandler_trace,"%s", "FMS_CPF_EventHandler constructor entering");

  pid_t proc_id = ACE_OS::getpid();

  ACE_OS::snprintf(procBuf,2048,"/proc/%i/cmdline",proc_id);
  ACE_OS::snprintf(arrProcId,10,"%i", proc_id);
  strProcId = arrProcId;

  FILE * pFile = NULL;
  pFile = ACE_OS::fopen (procBuf,"r");
  if (pFile != NULL)
  {
      result = ACE_OS::fread(procBuf,1,2048,pFile);
      if(result <= 0)
      {
    	  TRACE(fms_cpf_eventhandler_trace, "FMS_CPF_EventHandler constructor error reading file %s ", procBuf);
      }
      else
      {
    	  std::string str = procBuf;
          size_t found = str.find_last_of("/\\");

          processName = str.substr(found+1);
          processName += ":" + strProcId;
      }

      ACE_OS::fclose (pFile);
  }
  else
  {
	  TRACE(fms_cpf_eventhandler_trace, "FMS_CPF_EventHandler constructor error open file %s ", procBuf);
  }

  TRACE(fms_cpf_eventhandler_trace,"%s", "FMS_CPF_EventHandler constructor exiting");
}

FMS_CPF_EventHandler::~FMS_CPF_EventHandler ()
{
	if(NULL != fms_cpf_eventhandler_trace)
	  delete fms_cpf_eventhandler_trace;

	pthread_mutex_destroy(&eventHandlerMutex);
}

FMS_CPF_EventHandler*
FMS_CPF_EventHandler::instance()
{
	pthread_mutex_lock(&eventHandlerMutex);
	if(!eventHandler_)
	{
		eventHandler_ = new FMS_CPF_EventHandler;
	}

	pthread_mutex_unlock(&eventHandlerMutex);

	return eventHandler_;
}


//------------------------------------------------------------------------------
//      Event message
//------------------------------------------------------------------------------

void FMS_CPF_EventHandler::event (const FMS_CPF_Exception& ex)
{

    TRACE(fms_cpf_eventhandler_trace,"%s", "FMS_CPF_EventHandler::event() SENDING REPORT");

    acs_aeh_specificProblem specificProblem = (eventFaults::FMS_CPF_SpecificProblem + (acs_aeh_specificProblem)ex.errorCode ());
 	FMS_CPF_EventReportHandler::processName = processName.c_str();
  	FMS_CPF_EventReport event(ex.detailInfo ().c_str());
  	event.setProblemText(ex.errorText ());
  	event.setEventCode(specificProblem);
  	event.setProbableCause("AP INTERNAL FAULT");
  	eventReportHandler_::instance()->event(event,false);
}

