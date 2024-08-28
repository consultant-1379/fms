/*
 * * @file fms_cpf_basefilemgr.h
 *	@brief
 *	Header file for BaseFileMgr class.
 *  This module contains the declaration of the class BaseFileMgr.
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
 *	| 1.0.0  | 2011-07-07 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef BASEFILEMGR_H
#define BASEFILEMGR_H

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


#include "fms_cpf_fileid.h"
#include "fms_cpf_attribute.h"

#include <string>

const std::string ATTRIBUTE = ".attribute";

class FileMgr;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class BaseFileMgr
{
	friend class FileMgr;

 public:

	BaseFileMgr( const FMS_CPF_FileId& fileid,
			     const std::string& volume,
		  	     const FMS_CPF_Attribute& attribute,
			     const char* cpname = ""
			    );
 protected:

	virtual ~BaseFileMgr();

	virtual void create() = 0;

	virtual size_t getSize() const throw(FMS_CPF_PrivateException);

	virtual void changeActiveSubfile() = 0;

	virtual void removeBlockTransferFileLog() = 0;

	FMS_CPF_FileId    fileid_;
	std::string	    volume_;
	FMS_CPF_Attribute attribute_;
	static std::string  cpfhome_;
	std::string		cpname_;
};
  
#endif
