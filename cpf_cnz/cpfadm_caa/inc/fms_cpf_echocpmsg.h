//
/** @file fms_cpf_echocpmsg.h
 *	@brief
 *	Header file for FMS_CPF_EchoCPMsg class.
 *  This module contains the declaration of the class FMS_CPF_EchoCPMsg.
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

#ifndef FMS_CPF_ECHOCPMSG
#define FMS_CPF_ECHOCPMSG

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_cpmsg.h"

class FMS_CPF_EchoCPMsg : public FMS_CPF_CPMsg
{
public:

	/**
		@brief		constructor of FMS_CPF_EchoCPMsg class

		@param		aInBuffer

		@param		aInBufferSize
		
		@param		CPChan
	*/
  FMS_CPF_EchoCPMsg(const char* aInBuffer, unsigned int aInBufferSize, FMS_CPF_CpChannel *CPChan);
  
  /**
  		@brief		destructor of FMS_CPF_EchoCPMsg class

  */
  virtual ~FMS_CPF_EchoCPMsg();

private:
  /**
		@brief	unpack
  */
  virtual void unpack();
  /**
		@brief	work
  */  
  virtual bool work();
  /**
		@brief	pack
  */    
  virtual void pack();
  
  unsigned short inFunctionCode;
  
  ACS_TRA_trace* fms_cpf_EchoCP;
  
};

#endif
