//
/** @file fms_cpf_closecpmsg.h
 *	@brief
 *	Header file for FMS_CPF_CloseCPMSg class.
 *  This module contains the declaration of the class FMS_CPF_CloseCPMSg.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-11
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
 *	| 1.0.0  | 2011-11-11 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_CLOSECPMSG_H
#define FMS_CPF_CLOSECPMSG_H

#include "fms_cpf_cpmsg.h"
#include <ace/RW_Thread_Mutex.h>

class FileLock;

class FMS_CPF_CloseCPMsg : public FMS_CPF_CPMsg
{
public:

  FMS_CPF_CloseCPMsg(const char* aInBuffer, unsigned int aInBufferSize, FMS_CPF_CpChannel *CPChan);
  virtual ~FMS_CPF_CloseCPMsg();

  unsigned short go(bool &reply);
  unsigned short thGo();
  virtual uint16_t getOCSeq() const;
  virtual uint16_t getSBOCSeq() const;
  virtual bool messageIsFromEX();

  //  Check if message is a forced close
  virtual bool forcedClose() const;

private:
  virtual void unpack();
  virtual bool work();
  virtual void pack();
  void thWork();

  // In parameters
  unsigned short inFunctionCode;
  unsigned short inPriority;
  unsigned long	inFileReferenceUlong;
  unsigned char	inSubfileOption;
  uint16_t	inSBOCSequence;
  uint16_t  inOCSequence;

  FileLock* m_FileLock;

  ACS_TRA_trace* fms_cpf_CloseCPMsg;

  /**
   * 	@brief	m_mutex
   *
   * 	Mutex for internal sync
  */
  ACE_RW_Thread_Mutex m_mutex;

  // Out parameters
};

#endif
