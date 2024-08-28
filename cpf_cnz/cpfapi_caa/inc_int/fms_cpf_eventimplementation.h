/*
 * *@file fms_cpf_eventimplementation.h
 *	This module contains the declaration of the class FMS_CPF_eventimplementation.
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

#ifndef FMS_CPF_EVENTIMPLEMENTATION_H
#define FMS_CPF_EVENTIMPLEMENTATION_H


#include "acs_aeh_evreport.h"
#include <iostream>
#include <string>
#include "fms_cpf_eventreport.h"

class ACS_TRA_trace;

class FMS_CPF_EventImplementation
{

  public:

      FMS_CPF_EventImplementation();

    //## Constructors (specified)
      //	Constructor.
      //
      //	Parameters:
      //	IN: anerror	errorcode
      //
      //	Return value:
      //	-
      FMS_CPF_EventImplementation (int anerror);

      //	Constructor.
      //
      //	Parameters:
      //	IN: anerror	errorcode
      //	IN: mycomment	comment text
      //
      //	Return value:
      //	-
      FMS_CPF_EventImplementation (int anerror, const char *mycomment);

      //	Constructor.
      //
      //	Parameters:
      //	IN: anerror	errorcode
      //	IN: oserror	OS error number
      //	IN: afile		file name where the error occured
      //	IN: aline		line number where the error occured
      //	IN: mycomment	comment text
      //
      //
      //	Return value:
      //	-
      FMS_CPF_EventImplementation (int anerror, int oserror, const char *afile, int aline, const char *mycomment);

      //	Constructor.
      //
      //	Parameters:
      //	IN: anerror	errorcode
      //	IN: oserror	OS error number
      //	IN: afile		file name where the error occured
      //	IN: aline		line number where the error occured
      //	IN: istr		istream with comment text
      //
      //
      //	Return value:
      //	-
      FMS_CPF_EventImplementation (int anerror, int oserror, const char *afile, int aline, std::istream &istr);

      //	Constructor.
      //
      //	Parameters:
      //	IN: ex		another fms_afp_event
      //
      //	Return value:
      //	-

      FMS_CPF_EventImplementation (const FMS_CPF_EventImplementation& ex);

      //	Constructor.
      //
      //	Parameters:
      //	IN: problemtext	problem data
      //
      //	Return value:
      //	-
      FMS_CPF_EventImplementation (const char *problemdata);

    //## Destructor (generated)
      virtual ~FMS_CPF_EventImplementation();

      //	Retrieves stored error code.
      //
      //	Parameters:
      //	-
      //
      //	Return value:
      //	enum error value
      int errorCode () const;

      //	Stores new error code.
      //
      //	Parameters:
      //	IN: err             new error code
      //
      //	Return value:
      //	-
      void setErrorCode (int err);

      int osError ();

      //	Stores new os error code.
      //
      //	Parameters:
      //	IN: err             new os error code
      //
      //	Return value:
      //	-
      void setOsError (int err);

      //	Returns the event number
      //
      //	Parameters:
      //	-
      //
      //	Return value:
      //	fms_afp_events		event number 0-99
      long eventCode ();

      //	Set the event number
      //
      //	Parameters:
      //	int		event nr 0-99 (default 0)
      //
      //	Return value:
      //	-
      void setEventCode (acs_aeh_specificProblem evnum);

      //	Retrieves stored comment string.
      //
      //	Parameters:
      //	-
      //
      //	Return value:
      //	comment string
      const char * problemData () const;

      //	Set the problem data
      //
      //	Parameters:
      //	const char *		free text
      //
      //	Return value:
      //	-
      void setProblemData (const char *problemdata);

      //	Retrieves stored extra info string.
      //
      //	Parameters:
      //	-
      //
      //	Return value:
      //	extra info string

      const char * problemText () const;

      //	Set the problem text
      //
      //	Parameters:
      //	const char *		formatted text
      //
      //	Return value:
      //	-
      void setProblemText (const char *ptext);

      //	Returns the kind of exception
      //
      //	Parameters:
      //	-
      //
      //	Return value:
      //	int		kind
      //	EV_GENERAL=general exception
      //	EV_EVENT=event
      //	EV_ALARM=alarm
      //	EV_CEASING=cease
      FMS_CPF_EventReport::EventType kind ();

      //	Set the kind of exception
      //
      //	Parameters:
      //	int		kind
      //	EV_GENERAL=general exception
      //	EV_EVENT=event
      //	EV_ALARM=alarm
      //	EV_CEASING=cease
      //
      //	Return value:
      //	-
      void setKind (FMS_CPF_EventReport::EventType kind);

      //	Returns the number of times the exception has occurred
      //
      //	Parameters:
      //	-
      //
      //	Return value:
      //	int		count
      int counter ();

      //	Set the number-of-exceptions counter
      //
      //	Parameters:
      //	int		counter  (default 0)
      //
      //	Return value:
      //	-
      void setCounter (int count = 0);

      //	Increment the counter for number-of-exceptions
      //
      //	Parameters:
      //	-
      //
      //	Return value:
      //	-
      void incCounter ();

      //	Returns the age of the object in seconds
      //
      //	Parameters:
      //	-
      //
      //	Return value:
      //	int with age in seconds
      int age ();

      //	Resets the age of the object to zero
      //
      //	Parameters:
      //	-
      //
      //	Return value:
      //	-
      void resetAge ();

      //	Store string in comment text.
      //
      //	Usage:
      //	myex << "Another comment";
      FMS_CPF_EventImplementation & operator << (const char *mycomment);

      //	Store number in comment text.
      //
      //	Usage:
      //	myex << my_int;
      FMS_CPF_EventImplementation & operator << (long detail);

      //	Store istream in comment text.
      //
      //	Usage:
      //	myex << mstre;
      FMS_CPF_EventImplementation & operator << (std::istream &is);

      //	Retrieve complete info string.
      //
      //	Parameters:
      //	-
      //
      //	Return value:
      //	string with all info.
      std::string str ();

    // Data Members for Class Attributes
    // Additional Public Declarations

      const char * probableCause () const;

      void setProbableCause (const char *cause);

      const char* objectOfReference() const;

      void setObjectOfReference(const char* objRef);


  private:
       int oserror_;
      int error_;
      std::string file_;
      int line_;
      std::string problemdata_;
      int counter_;

      //	Kind of exception:
      //	EV_GENERAL = general exception
      //	EV_EVENT = event
      //	EV_ALARM = alarm
      //	EV_CEASING = alarm ceasing

      FMS_CPF_EventReport::EventType kind_;
      long event_;
      std::string problemtext_;

      ACE_Time_Value *evTime;
      std::string cause_;
      std::string objRef_;
      ACS_TRA_trace *fms_cpf_eventimpl_trace;
};


#endif
