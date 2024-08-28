/*
 * * @file fms_cpf_eventimplementation.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_Eventimplementation.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_eventimplementation.h module
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
#include <iostream>
#include <sstream>
#include <string>
#include "ACS_TRA_trace.h"
#include "fms_cpf_eventimplementation.h"

using namespace std;

FMS_CPF_EventImplementation::FMS_CPF_EventImplementation()
{
	fms_cpf_eventimpl_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventImplementation");
	evTime       = new ACE_Time_Value(ACE_OS::time());
	error_       = 0;	// default internal error
	oserror_     = 0;
	file_	     = "";
	line_	     = 0;
	problemdata_ = "";
	problemtext_ = "";
	event_		 = 0;
	counter_	 = 0;
	kind_		 = FMS_CPF_EventReport::EV_GENERAL;
    cause_       = FMS_CPF_EventReport::defaultProbableCause;
    objRef_      = "";
}

//	----------------------------------------
//	                    constructor
//	----------------------------------------

FMS_CPF_EventImplementation::FMS_CPF_EventImplementation (int anerror)
{
	fms_cpf_eventimpl_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventImplementation");
	evTime			= new ACE_Time_Value(ACE_OS::time());
	error_			= anerror;
	oserror_		= 0;
	file_			= "";
	line_			= 0;
	problemdata_	= "";
	problemtext_	= "";
	event_			= 0;
	counter_		= 0;
	kind_			= FMS_CPF_EventReport::EV_GENERAL;
    cause_          = FMS_CPF_EventReport::defaultProbableCause;
    objRef_         = "";
}

FMS_CPF_EventImplementation::FMS_CPF_EventImplementation (int anerror, const char *mycomment)
{
	fms_cpf_eventimpl_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventImplementation");
	evTime			= new ACE_Time_Value(ACE_OS::time());
	error_			= anerror;
	oserror_		= 0;
	file_			= "";
	line_			= 0;
	problemdata_	= mycomment;
	problemtext_	= "";
	event_			= 0;
	counter_		= 0;
	kind_			= FMS_CPF_EventReport::EV_GENERAL;
    cause_          = FMS_CPF_EventReport::defaultProbableCause;
    objRef_         = "";
}

FMS_CPF_EventImplementation::FMS_CPF_EventImplementation (int anerror, int oserror, const char *afile, int aline, const char *mycomment)
{
	fms_cpf_eventimpl_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventImplementation");
	evTime			= new ACE_Time_Value(ACE_OS::time());
	error_			= anerror;
	oserror_		= oserror;
	file_			= afile;
	line_			= aline;
	problemdata_	= mycomment;
	problemtext_	= "";
	event_			= 0;
	counter_		= 0;
	kind_			= FMS_CPF_EventReport::EV_GENERAL;
    cause_          = FMS_CPF_EventReport::defaultProbableCause;
    objRef_         = "";
}

FMS_CPF_EventImplementation::FMS_CPF_EventImplementation (int anerror, int oserror, const char *afile, int aline, std::istream &istr)
{
	fms_cpf_eventimpl_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventImplementation");
	evTime			= new ACE_Time_Value(ACE_OS::time());
	error_			= anerror;
	oserror_		= oserror;
	file_			= afile;
	line_			= aline;
	event_			= 0;
	counter_		= 0;
	kind_			= FMS_CPF_EventReport::EV_GENERAL;
	std::string str;
	istr >> str;
	problemdata_ += str;
	problemtext_	= "";
    cause_          = FMS_CPF_EventReport::defaultProbableCause;
    objRef_         = "";
}

FMS_CPF_EventImplementation::FMS_CPF_EventImplementation (const FMS_CPF_EventImplementation& ex)
{
	fms_cpf_eventimpl_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventImplementation");
	evTime				= new ACE_Time_Value(*ex.evTime);
	error_				= ex.error_;
	oserror_			= ex.oserror_;
	file_				= ex.file_;
	line_				= ex.line_;
	problemdata_	= ex.problemdata_;
	problemtext_	= ex.problemtext_;
	event_				= ex.event_;
	counter_			= ex.counter_;
	kind_				= ex.kind_;
    cause_          = ex.cause_;
    objRef_         = ex.objRef_;
}

FMS_CPF_EventImplementation::FMS_CPF_EventImplementation (const char *problemdata)
{
	fms_cpf_eventimpl_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventImplementation");
	evTime				= new ACE_Time_Value(ACE_OS::time());
	error_				= 0;
	oserror_			= 0;
	file_				= "";
	line_				= 0;
	problemdata_	= problemdata;
	problemtext_	= "";
	event_				= 0;
	counter_			= 0;
	kind_				= FMS_CPF_EventReport::EV_GENERAL;
    cause_          = FMS_CPF_EventReport::defaultProbableCause;
    objRef_         = "";
}


FMS_CPF_EventImplementation::~FMS_CPF_EventImplementation()
{
	delete evTime;
	if (NULL != fms_cpf_eventimpl_trace)
	{
		delete fms_cpf_eventimpl_trace;
	}
}



//	----------------------------------------
//	                    errorCode
//	----------------------------------------
int FMS_CPF_EventImplementation::errorCode () const
{
	return error_;
}

void FMS_CPF_EventImplementation::setErrorCode (int err)
{
	error_ = err;
}

//	----------------------------------------
//	                    osError
//	----------------------------------------

int FMS_CPF_EventImplementation::osError ()
{
	return oserror_;
}

//	----------------------------------------
//	                    setOsError
//	----------------------------------------
void FMS_CPF_EventImplementation::setOsError (int err)
{
	oserror_ = err;
}

//	----------------------------------------
//	                    evNumber
//	----------------------------------------
long FMS_CPF_EventImplementation::eventCode ()
{
	return event_;
}

//	----------------------------------------
//	                    setEvNumber
//	----------------------------------------
void FMS_CPF_EventImplementation::setEventCode (acs_aeh_specificProblem evnum)
{
	event_ = evnum;
}

//	----------------------------------------
//	                    problemData
//	----------------------------------------
const char * FMS_CPF_EventImplementation::problemData () const
{
	return problemdata_.c_str();
}

//	----------------------------------------
//	                    setProblemData
//	----------------------------------------
void FMS_CPF_EventImplementation::setProblemData (const char *problemdata)
{
	problemdata_ = problemdata;
}

//	----------------------------------------
//	                    extraInfo
//	----------------------------------------
const char * FMS_CPF_EventImplementation::problemText () const
{
	return problemtext_.c_str();
}

//	----------------------------------------
//	                    setProblemText
//	----------------------------------------
void FMS_CPF_EventImplementation::setProblemText (const char *ptext)
{
	problemtext_ = ptext;
}

//	----------------------------------------
//	                    kind
//	----------------------------------------
FMS_CPF_EventReport::EventType FMS_CPF_EventImplementation::kind ()
{
	return kind_;
}

//	----------------------------------------
//	                    setKind
//	----------------------------------------
void FMS_CPF_EventImplementation::setKind (FMS_CPF_EventReport::EventType kind)
{
	kind_ = kind;
}

//	----------------------------------------
//	                    counter
//	----------------------------------------
int FMS_CPF_EventImplementation::counter ()
{
	return counter_;
}

//	----------------------------------------
//	                    setCounter
//	----------------------------------------
void FMS_CPF_EventImplementation::setCounter (int count)
{
	counter_ = count;
}

//	----------------------------------------
//	                    incCounter
//	----------------------------------------
void FMS_CPF_EventImplementation::incCounter ()
{
	counter_ ++;
}
//	----------------------------------------
//	                    age
//	----------------------------------------
int FMS_CPF_EventImplementation::age ()
{
	ACE_Time_Value now(ACE_OS::time());
	int seconds = now.sec() - evTime->sec();
	return seconds;
}

//	----------------------------------------
//	                    resetAge
//	----------------------------------------
void FMS_CPF_EventImplementation::resetAge ()
{
	ACE_Time_Value now(ACE_OS::time());
	evTime->set(ACE_OS::time());
}

//	----------------------------------------
//	                    operator<<
//	----------------------------------------
FMS_CPF_EventImplementation & FMS_CPF_EventImplementation::operator << (const char *mycomment)
{
	problemdata_ += mycomment;
	return *this;
}

//	----------------------------------------
//	                    operator<<
//	----------------------------------------
FMS_CPF_EventImplementation & FMS_CPF_EventImplementation::operator << (long detail)
{
	char buff[30] = {0};
	ACE_OS::snprintf(buff,30,"%d", detail);
	problemdata_ += buff;
	return *this;
}

//	----------------------------------------
//	                    operator<<
//	----------------------------------------
FMS_CPF_EventImplementation & FMS_CPF_EventImplementation::operator << (std::istream &is)
{
	std::string str;
	is >> str;
	problemdata_ += str;
	return *this;
}

//	----------------------------------------
//	                    str
//	----------------------------------------
std::string FMS_CPF_EventImplementation::str ()
{
	std::stringstream ost(std::stringstream::in | std::stringstream::out);
	ost.clear();
	ost << "File: "	<< file_ << " " << std::endl;
	ost << "Line: " << line_ << " " << std::endl;
	ost << "Error: (" << error_ << ") " << std::endl;
	//ost << "ErrorMessage: " << errorMessages[error_].errorText << " " << std::endl;
	ost << "Counter: " << counter_ << " " << std::endl;
	ost << "Event nr: " << event_ << " " << std::endl;
	ost << "Kind: " << kind_ << " " << std::endl;
	ost << "Age: " << this->age() << " " << std::endl;
    ost << "Cause: " << cause_ << " " << std::endl;
    ost << "Object of reference: " << objRef_ << " " << std::endl;
	ost << "Problem data: " << problemdata_ << " " << std::endl;
	ost << "Problem text: " << problemtext_ << " " << std::endl;
	ost << "OS error: (" << oserror_ << ") " << strerror(oserror_) << " " << std::endl;
	ost << '\0';
	return ost.str();
}

//	----------------------------------------
//	                    probableCause
//	----------------------------------------
const char * FMS_CPF_EventImplementation::probableCause () const
{
	return cause_.c_str();
}

//	----------------------------------------
//	                    setProbableCause
//	----------------------------------------
void FMS_CPF_EventImplementation::setProbableCause (const char *cause)
{
	cause_ = cause;
}

//	----------------------------------------
//	                    objectOfReference
//	----------------------------------------
const char* FMS_CPF_EventImplementation::objectOfReference() const
{
	return objRef_.c_str();
}

//	----------------------------------------
//	                    setObjectOfReference
//	----------------------------------------
void FMS_CPF_EventImplementation::setObjectOfReference(const char* objRef)
{
	objRef_ = objRef;
}

