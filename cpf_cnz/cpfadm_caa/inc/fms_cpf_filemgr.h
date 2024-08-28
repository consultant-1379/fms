/*
 * * @file fms_cpf_filemgr.h
 *	@brief
 *	Header file for FileTable class.
 *  This module contains the declaration of the class FileMgr.
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
#ifndef FILEMGR_H
#define FILEMGR_H

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_privateexception.h"
#include "fms_cpf_attribute.h"
#include "fms_cpf_fileid.h"

#include <string>
#include <dirent.h>

class BaseFileMgr;
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FileMgr 
{
 public:
	FileMgr(const FMS_CPF_FileId& fileid,
		    const std::string& volume,
		    const FMS_CPF_Attribute& attribute,
		    const char * cpname = "")
	throw (FMS_CPF_PrivateException);

	~FileMgr();

	static bool volumeExists(const std::string& volume, const std::string& cp)
	throw (FMS_CPF_PrivateException);

	void create() throw (FMS_CPF_PrivateException);

	void remove() throw (FMS_CPF_PrivateException);

	void rename(const FMS_CPF_FileId& fileid) throw (FMS_CPF_PrivateException);

	void setAttribute() throw (FMS_CPF_PrivateException);

	bool exists() throw (FMS_CPF_PrivateException);

	size_t getSize() const throw (FMS_CPF_PrivateException);

	void changeActiveSubfile() throw (FMS_CPF_PrivateException);

	void opendir() throw (FMS_CPF_PrivateException);

    FMS_CPF_FileId readdir() const;

    void closedir() throw (FMS_CPF_PrivateException);

    static std::string& getCPFhome() throw (FMS_CPF_PrivateException);

 private:
	std::string cpname_;

	BaseFileMgr*     filemgrp_;

	DIR*             dp_;

	/**		@brief	fms_cpf_FileMgr
	*/
	ACS_TRA_trace* fms_cpf_FileMgr;

};

#endif
