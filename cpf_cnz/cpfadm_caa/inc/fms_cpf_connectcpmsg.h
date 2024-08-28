//
/** @file fms_cpf_connectcpmsg.h
 *	@brief
 *	Header file for FMS_CPF_ConnectCPMsg class.
 *  This module contains the declaration of the class FMS_CPF_ConnectCPMsg.
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

#ifndef FMS_CPF_CONNECTCPMSG_H
#define FMS_CPF_CONNECTCPMSG_H

#include "fms_cpf_cpmsg.h"

class FMS_CPF_ConnectCPMsg : public FMS_CPF_CPMsg
{
public:

  FMS_CPF_ConnectCPMsg(const char* aInBuffer, unsigned int aInBufferSize, FMS_CPF_CpChannel *CPChan);
  virtual ~FMS_CPF_ConnectCPMsg();

private:
  virtual void unpack();
  virtual bool work() { return true; };
  virtual void pack();

  // In parameters
  unsigned short inFunctionCode;
  unsigned short inSubnetId;
  unsigned short APFMItwoBuffers; //020920
  
  ACS_TRA_trace* fms_cpf_ConnectCPMsg;

  // Out parameters
};

#endif
