/*
 * *@file fms_cpf_eventreport.h
 *	This module contains the declaration of the class FMS_CPF_eventReport.
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

#ifndef FMS_CPF_EVENTREPORT_H
#define FMS_CPF_EVENTREPORT_H

#include "acs_aeh_evreport.h"
#include <iostream>
#include <string>

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class ACE_Time_Value;
class FMS_CPF_EventImplementation;
class ACS_TRA_trace;


class FMS_CPF_EventReport
{

 public:

  typedef enum
  {
    // To separate event, alarm and ceasing.
    EV_GENERAL = 100,
    EV_EVENT = 101,
    EV_ALARM = 102,
    EV_CEASING=103
  } EventType;

  /** @brief Constructor method
   *  Creates an empty event object.
   */
  FMS_CPF_EventReport();

  /** @brief Constructor method
   *  Creates an event object.
   *  @param anerror: errore code
   */
  FMS_CPF_EventReport(int );

  /** @brief Constructor method
   *  Creates an event object.
   *  @param anerror: errore code
   *  @param mycomment: problem data
   */
  FMS_CPF_EventReport(int anerror, const char *mycomment);

  /** @brief Constructor method
   *  Creates an event object.
   *  @param anerror: errore code
   *  @param oserror: OS error code
   *  @param afile: File name where the error occured
   *  @param aline: Line number where the error occured
   *  @param mycomment: problem data
   */
  FMS_CPF_EventReport (int anerror, int oserror, const char *afile, int aline, const char *mycomment);

  /** @brief Constructor method
   *  Creates an event object.
   *  @param anerror: errore code
   *  @param oserror: OS error code
   *  @param afile: File name where the error occured
   *  @param aline: Line number where the error occured
   *  @param istr: problem data
   */
  FMS_CPF_EventReport (int anerror, int oserror, const char *afile, int aline, std::istream &istr);

  /** @brief Copy Constructor method
   *  Creates an event object.
   */
  FMS_CPF_EventReport (const FMS_CPF_EventReport &ex);

  /** @brief operator
   */
  FMS_CPF_EventReport &operator=(const FMS_CPF_EventReport &ex);

  /** @brief Constructor method
   *  Creates an event object.
   *  @param problemdata: problem data
   */
  FMS_CPF_EventReport (const char *problemdata);

  /** @brief destructor method
   */
  virtual ~FMS_CPF_EventReport();

  /** @brief errorCode method
   *  Retrieves stored error code
   */
  int errorCode () const;

  /** @brief setErrorCode method
   *  Stores new error code.
   */
  void setErrorCode (int err);

  /** @brief osError method
   *  Retrieves stored error code
   */
  int osError ();

  /** @brief setOsError method
   *  Stores new OS error code.
   */
  void setOsError (int err);

  /** @brief eventCode method
   *   Retrieves stored event number.
   */
  long eventCode ();

  /** @brief setEventCode method
   *   stores new event number.
   */
  void setEventCode (acs_aeh_specificProblem evnum);

  const char * problemData () const;
  /** @brief
  *    Retrieves stored problem data.
  * Return value:
  *    Problem data
  */

  void setProblemData (const char *problemdata);
  /** @brief Stores new problem data.
  * Parameters:
  *    IN: problemdata         New problem data
  */

  const char * problemText () const;
  /** @brief
  *    Retrieves stored problem text.
  *    Return value:
  *    Problem text
  */

  void setProblemText (const char *ptext);
  /** @brief
   *    Stores new problem data.
   * Parameters:
   *    IN: ptext               New problem text
   */

  EventType kind ();
  /** @brief Description:
  *    Returns the kind of exception.
  * Return value:
  *    EV_GENERAL      General exception
  *    EV_EVENT        Event
  *    EV_ALARM        Alarm
  *    EV_CEASING      Cease
  */

  void setKind (EventType kind);
  /** @brief Description:
  *    Sets the kind of exception.
  *  Parameters:
  *    IN: kind                Kind of exception
  */

  int counter ();
  /** @brief Description:
  *    Returns the number of times the exception has occurred.
  * Return value:
  *    Count
  */

  void setCounter (int count = 0);
  /** @brief Description:
  *    Set the number-of-exceptions counter
  * Parameters:
  *    IN: count               Number of exceptions
  */

  void incCounter ();
  /** @brief Description:
  *    Increment the counter for number-of-exceptions
  */

  int age ();
  /** @brief Description:
  *    Returns the age of the object in seconds.
  * Return value:
  *    Age in seconds
  */


  void resetAge ();
  /** @brief Description:
  *    Resets the age of the object to zero.
  */

  FMS_CPF_EventReport & operator << (const char *mycomment);
  /** @brief Description:
  *    Adds string 'mycomment' to problem data.
  * Parameters:
  *    IN: mycomment               Problem data
  */

  FMS_CPF_EventReport & operator << (long detail);
  /** @brief Description:
  *    Converts 'detail' to a string and adds it to problem data.
  * Parameters:
  *    IN: detail                  Problem data
  */

  FMS_CPF_EventReport & operator << (std::istream &is);
  /** @brief Description:
  *    Adds data from 'is' to problem data.
  * Parameters:
  *    IN: is                      Problem data
  */

  std::string str ();
  // Description:
  //    Retrieves complete information about the event.
  // Return value: 
  //    Information string

  const char * probableCause () const;
  // Description:
  //    Retrieves stored probable cause.
  // Return value: 
  //    Probable cause

  void setProbableCause (const char *cause);
  // Description:
  //    Stores new probable cause.
  // Parameters: 
  //    IN: cause               New probable cause

  // Default probable cause is used in the constructors

  static std::string defaultProbableCause;

  const char* objectOfReference() const;
  // Description:
  //    Retrieves stored object of reference.
  // Return value: 
  //    Object of reference

  void setObjectOfReference(const char* objRef);
  // Description:
  //    Stores new object of reference.
  // Parameters: 
  //    IN: objRef              New object of reference

private:

  ACS_TRA_trace *fms_cpf_eventreport_trace;
  FMS_CPF_EventImplementation* implementation;

};


#endif
