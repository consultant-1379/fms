/*
 * * @file fms_cpf_basefilemgr.cpp
 *	@brief
 *	Class method implementation for BaseFileMgr.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_basefilemgr.h module
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

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_basefilemgr.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_common.h"

#include <sys/stat.h>
//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
BaseFileMgr::BaseFileMgr( const FMS_CPF_FileId& fileid,
			  	  	  	  const std::string& volume,
			  	  	  	  const FMS_CPF_Attribute& attribute,
			  	  	  	  const char* cpname) :
fileid_(fileid),
volume_(volume),
attribute_(attribute),
cpname_(cpname)
{
}

//------------------------------------------------------------------------------
//      Get size of the file
//------------------------------------------------------------------------------
size_t BaseFileMgr::getSize() const throw(FMS_CPF_PrivateException)
{
    std::string path = (ParameterHndl::instance()->getCPFroot(cpname_.c_str()) + DirDelim + volume_ + DirDelim +
		   	   	    	fileid_.file() );

    // Check the file type
	if(fileid_.subfileAndGeneration().empty() == false)
	{
		// Subfile
		path += DirDelim + fileid_.subfileAndGeneration();
	}

	struct stat64 buf;

	if(stat64(path.c_str(), &buf) != 0)
	{
		char errorText[256] = {0};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::PHYSICALERROR, errorDetail );
	}

	size_t size;
	// Check if a folder or file
	if(S_ISDIR(buf.st_mode))
	{
		// Directory
		size = 0;
    }
	else
	{
		// Regular file
		unsigned int recordLength = attribute_.getRecordLength();
		uint64_t quot;
		uint64_t rem;

		quot = buf.st_size / recordLength;
		rem = buf.st_size % recordLength;
		size = quot + (rem ? 1: 0);
	}

	return size;
}
//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
BaseFileMgr::~BaseFileMgr()
{
}
