/*
 * * @file fms_cpf_eventreport.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_EventReport.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_eventreport.h module
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
#include <string>
#include "ACS_TRA_trace.h"
#include "fms_cpf_eventreport.h"
#include "fms_cpf_eventimplementation.h"

std::string FMS_CPF_EventReport::defaultProbableCause = "";

FMS_CPF_EventReport::FMS_CPF_EventReport()
{
	fms_cpf_eventreport_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventReport");
    implementation = new FMS_CPF_EventImplementation;

}

//	----------------------------------------
//	                    constructor
//	----------------------------------------

FMS_CPF_EventReport::FMS_CPF_EventReport (int anerror)
{
	fms_cpf_eventreport_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventReport");
    implementation = new FMS_CPF_EventImplementation(anerror);
}

//	----------------------------------------
//	                    constructor
//	----------------------------------------
FMS_CPF_EventReport::FMS_CPF_EventReport (int anerror, const char *mycomment)
{
	fms_cpf_eventreport_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventReport");
    implementation = new FMS_CPF_EventImplementation(anerror, mycomment);

}

//	----------------------------------------
//	                    constructor
//	----------------------------------------
FMS_CPF_EventReport::FMS_CPF_EventReport (int anerror, int oserror, const char *afile, int aline, const char *mycomment)
{
	fms_cpf_eventreport_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventReport");
    implementation = new FMS_CPF_EventImplementation(anerror, oserror, afile, aline, mycomment);
}

//	----------------------------------------
//	                    constructor
//	----------------------------------------
FMS_CPF_EventReport::FMS_CPF_EventReport (int anerror, int oserror, const char *afile, int aline, std::istream &istr)
{
	fms_cpf_eventreport_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventReport");
    implementation = new FMS_CPF_EventImplementation(anerror, oserror, afile, aline, istr);
}

//	----------------------------------------
//	                    constructor
//	----------------------------------------
FMS_CPF_EventReport::FMS_CPF_EventReport (const FMS_CPF_EventReport& ex)
{
	fms_cpf_eventreport_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventReport");
    implementation = new FMS_CPF_EventImplementation(*(ex.implementation));
}

FMS_CPF_EventReport& FMS_CPF_EventReport::operator=(const FMS_CPF_EventReport& ex)
{
	fms_cpf_eventreport_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventReport");
    implementation = new FMS_CPF_EventImplementation(*(ex.implementation));
    return *this;
}    
//	----------------------------------------
//	                    constructor
//	----------------------------------------
FMS_CPF_EventReport::FMS_CPF_EventReport (const char *problemdata)
{
	fms_cpf_eventreport_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventReport");
    implementation = new FMS_CPF_EventImplementation(problemdata);
}

FMS_CPF_EventReport::~FMS_CPF_EventReport()
{
	delete implementation;
	if (NULL != fms_cpf_eventreport_trace)
	{
		delete fms_cpf_eventreport_trace;
	}
}

//	----------------------------------------
//	                    errorCode
//	----------------------------------------

int FMS_CPF_EventReport::errorCode () const
{
	return implementation->errorCode();
}

//	----------------------------------------
//	                    setErrorCode
//	----------------------------------------

void FMS_CPF_EventReport::setErrorCode (int err)
{
    implementation->setErrorCode(err);
}

//	----------------------------------------
//	                    osError
//	----------------------------------------
int FMS_CPF_EventReport::osError ()
{
	return implementation->osError();
}

//	----------------------------------------
//	                    setOsError
//	----------------------------------------
void FMS_CPF_EventReport::setOsError (int err)
{
    implementation->setOsError(err);
}

//	----------------------------------------
//	                    evNumber
//	----------------------------------------

long FMS_CPF_EventReport::eventCode ()
{
	return implementation->eventCode();
}

//	----------------------------------------
//	                    setEvNumber
//	----------------------------------------

void FMS_CPF_EventReport::setEventCode (acs_aeh_specificProblem evnum)
{
    implementation->setEventCode(evnum);
}

//	----------------------------------------
//	                    problemData
//	----------------------------------------
const char * FMS_CPF_EventReport::problemData () const
{
	return implementation->problemData();
}

//	----------------------------------------
//	                    setProblemData
//	----------------------------------------
void FMS_CPF_EventReport::setProblemData (const char *problemdata)
{
    implementation->setProblemData(problemdata);
}

//	----------------------------------------
//	                    extraInfo
//	----------------------------------------
const char * FMS_CPF_EventReport::problemText () const
{
	return implementation->problemText();
}

//	----------------------------------------
//	                    setProblemText
//	----------------------------------------
void FMS_CPF_EventReport::setProblemText (const char *ptext)
{
    implementation->setProblemText(ptext);
}

//	----------------------------------------
//	                    kind
//	----------------------------------------
FMS_CPF_EventReport::EventType FMS_CPF_EventReport::kind ()
{

	return implementation->kind();

}

//	----------------------------------------
//	                    setKind
//	----------------------------------------
void FMS_CPF_EventReport::setKind (FMS_CPF_EventReport::EventType kind)
{
    implementation->setKind(kind);
}

//	----------------------------------------
//	                    counter
//	----------------------------------------
int FMS_CPF_EventReport::counter ()
{
	return implementation->counter();
}

//	----------------------------------------
//	                    setCounter
//	----------------------------------------
void FMS_CPF_EventReport::setCounter (int count)
{
    implementation->setCounter(count);
}

//	----------------------------------------
//	                    incCounter
//	----------------------------------------
void FMS_CPF_EventReport::incCounter ()
{
    implementation->incCounter();
}

//	----------------------------------------
//	                    age
//	----------------------------------------

int FMS_CPF_EventReport::age ()
{
	return implementation->age();
}

//	----------------------------------------
//	                    resetAge
//	----------------------------------------
void FMS_CPF_EventReport::resetAge ()
{
    implementation->resetAge();
}

//	----------------------------------------
//	                    operator<<
//	----------------------------------------
FMS_CPF_EventReport & FMS_CPF_EventReport::operator << (const char *mycomment)
{
	*implementation << mycomment;
    return *this;
}

//	----------------------------------------
//	                    operator<<
//	----------------------------------------
FMS_CPF_EventReport & FMS_CPF_EventReport::operator << (long detail)
{
	*implementation << detail;
    return *this;
}

//	----------------------------------------
//	                    operator<<
//	----------------------------------------
FMS_CPF_EventReport & FMS_CPF_EventReport::operator << (std::istream &is)
{
	*implementation << is;
    return *this;
}

//	----------------------------------------
//	                    str
//	----------------------------------------

std::string FMS_CPF_EventReport::str ()
{
	return implementation->str();
}

//	----------------------------------------
//	                    probableCause
//	----------------------------------------

const char * FMS_CPF_EventReport::probableCause () const
{
	return implementation->probableCause();
}

void FMS_CPF_EventReport::setProbableCause (const char *cause)
{
    implementation->setProbableCause(cause);
}

const char* FMS_CPF_EventReport::objectOfReference() const
{
	return implementation->objectOfReference();
}

//	----------------------------------------
//	                    setObjectOfReference
//	----------------------------------------
void FMS_CPF_EventReport::setObjectOfReference(const char* objRef)
{
    implementation->setObjectOfReference(objRef);
}
