/*
 * * @file fms_cpf_infinitefilemgr.h
 *	@brief
 *	Header file for InfiniteFileMgr class.
 *  This module contains the declaration of the class InfiniteFileMgr.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-07-07
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
 *	| 1.0.0  | 2011-10-12 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef INFINITEFILEMGR_H
#define INFINITEFILEMGR_H

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_basefilemgr.h"
#include "fms_cpf_privateexception.h"

class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class InfiniteFileMgr: public BaseFileMgr
{
  friend class FileMgr;

 public:
		void create() throw(FMS_CPF_PrivateException);

		void changeActiveSubfile() throw(FMS_CPF_PrivateException);

		void removeBlockTransferFileLog();

 private:
  
		InfiniteFileMgr( const FMS_CPF_FileId& fileid,
					   	 const std::string& volume,
					   	 const FMS_CPF_Attribute& attribute,
					   	 const char* cpname = "");

		virtual ~InfiniteFileMgr();

		/**		@brief	fms_cpf_InfFileMgr
		*/
		ACS_TRA_trace* fms_cpf_InfFileMgr;
};
  
#endif
