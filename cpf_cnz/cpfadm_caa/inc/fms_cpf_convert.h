//
/** @file fms_cpf_convert.h
 *	@brief
 *	Header file for fms_cpf_convert class.
 *  This module contains the declaration of the class fms_cpf_convert.
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

#ifndef CONVERT_H
#define CONVERT_H

#include <netinet/in.h>
#include "ace/ACE.h"

#endif

class FMS_CPF_Convert
{
  union word16_t
  {
    char byte [2];
    ACE_UINT16 word16;
  };
 
  union word32_t
  {
    char byte [4];
    ACE_UINT32 word32;
  };

  public:

  inline static unsigned short 
  ushortCpToHost (char* buffer)
  {
    word16_t w;
    w.byte [0] = buffer [1];
    w.byte [1] = buffer [0];
    return ntohs(w.word16);
  };
 
  inline static void 
  ushortHostToCp (char* buffer,  unsigned short value)
  {
    word16_t w;
    w.word16 = htons(value);
    buffer [0] = w.byte [1];
    buffer [1] = w.byte [0];
  };
 
  inline static unsigned long 
  ulongCpToHost (char* buffer)
  {
    word32_t w;
    w.byte [0] = buffer [3];
    w.byte [1] = buffer [2];
    w.byte [2] = buffer [1];
    w.byte [3] = buffer [0];
    return ntohl(w.word32);
  };
 
  inline static void 
  ulongHostToCp (char* buffer, unsigned long value)
  {
    word32_t w;
    w.word32 = htonl(value);
    buffer [0] = w.byte [3];
    buffer [1] = w.byte [2];
    buffer [2] = w.byte [1];
    buffer [3] = w.byte [0];
  };

  inline static unsigned long 
  ulongNetToHost (char* buffer)
  {
    word32_t w;
    w.byte [0] = buffer [0];
    w.byte [1] = buffer [1];
    w.byte [2] = buffer [2];
    w.byte [3] = buffer [3];
    return ntohl(w.word32);
  };
 
  inline static void 
  ulongHostToNet (char* buffer, unsigned long value)
  {
    word32_t w;
    w.word32 = htonl(value);
    buffer [0] = w.byte [0];
    buffer [1] = w.byte [1];
    buffer [2] = w.byte [2];
    buffer [3] = w.byte [3];
  };

};

#endif
