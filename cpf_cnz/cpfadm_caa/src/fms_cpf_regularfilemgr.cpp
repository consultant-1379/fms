/*
 * * @file fms_cpf_regularfilemgr.cpp
 *	@brief
 *	Class method implementation for RegularFileMgr.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_regularfilemgr.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-07-05
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
 *	| 1.0.0  | 2011-07-05 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_regularfilemgr.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_common.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include "ace/ACE.h"
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

extern ACS_TRA_Logging CPF_Log;

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
RegularFileMgr::RegularFileMgr( const FMS_CPF_FileId& fileid,
                                const std::string& volume,
								const FMS_CPF_Attribute& attribute,
								const char * cpname) :
BaseFileMgr(fileid, volume, attribute, cpname)
{
	fms_cpf_RegFileMgr =  new (std::nothrow) ACS_TRA_trace("FMS_CPF_RegularFileMgr");
}


//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------

RegularFileMgr::~RegularFileMgr ()
{
	if(NULL != fms_cpf_RegFileMgr)
			delete fms_cpf_RegFileMgr;
}


//------------------------------------------------------------------------------
//      Create file
//------------------------------------------------------------------------------

void RegularFileMgr::create() throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_RegFileMgr, "%s", "Entering in create()");

	// Create data file
    	std::string path =( ParameterHndl::instance()->getCPFroot(cpname_.c_str()) + DirDelim + volume_ + DirDelim +
    		       fileid_.file() );

	TRACE(fms_cpf_RegFileMgr, "create(), file=<%s>", path.c_str() );

	//Check for composite or simple
	if(attribute_.composite() == false )
	{
        // simple file creation
		// creation flag
		int oflag = O_WRONLY | O_CREAT | O_TRUNC | _O_BINARY;
		// Set permission 744 (RWXR--R--) on file
		mode_t mode = S_IRWXU | S_IRGRP | S_IROTH;
		ACE_OS::umask(0);

		int fileHandle = ACE_OS::open(path.c_str(), oflag, mode);
		if(FAILURE != fileHandle)
		{
			TRACE(fms_cpf_RegFileMgr, "%s", "create(), simple file created");
			ACE_OS::close(fileHandle);
		}
		else
		{
			TRACE(fms_cpf_RegFileMgr, "%s", "create(), simple file creation failed");

			char errorText[256] = {0};
			std::string errorDetail(strerror_r(errno, errorText, 255));
			char errorMsg[512] = {0};
			snprintf(errorMsg, 511, "Create(), simple file creation to the path:<%s> failed, error=<%s>", path.c_str(), errorDetail.c_str() );
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
			throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::PHYSICALERROR, errorDetail );
		}
	}
	else
	  {
		// composite file creation
		// Set permission 755 (RWXR-XR-X) on folder
		mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
		ACE_OS::umask(0);
		if(( ACE_OS::mkdir(path.c_str(), mode ) == FAILURE) && (errno != EEXIST) )
		{
			 TRACE(fms_cpf_RegFileMgr, "%s", "error in  create(), file composite in path = <%s>",path.c_str());
			 char errorText[256] = {0};
			 std::string errorDetail(strerror_r(errno, errorText, 255));
			 char errorMsg[512] = {0};
			 snprintf(errorMsg, 511, "Create(), composite file creation to the path:<%s> failed, error=<%s>", path.c_str(), errorDetail.c_str() );
			 CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);

			 throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::PHYSICALERROR, errorDetail );
		}
		TRACE(fms_cpf_RegFileMgr, "%s", "create(), file composite has been created");
		
		if(cpname_.compare(DEFAULT_CPNAME)==0)
		{
			TRACE(fms_cpf_RegFileMgr, "error in  create(), compare = <%d>",cpname_.compare(DEFAULT_CPNAME));
			std::string compositeFile = ParameterHndl::instance()->getDataDiskRoot();
			compositeFile.push_back(DirDelim);
			compositeFile.append(cpf_imm::CompositeFileObjectName); 
			if(( ACE_OS::mkdir(compositeFile.c_str(), mode ) == FAILURE) && (errno != EEXIST) )
                        {
                        	char errorText[256] = {0};
                                std::string errorDetail(strerror_r(errno, errorText, 255));
                                char errorMsg[512] = {0};
                                snprintf(errorMsg, 511, "Create(), compositeFile dir creation to the path:<%s> failed, error=<%s>", compositeFile.c_str(), errorDetail.c_str());
                                CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
                        }
			
                        std::string subFile = compositeFile; 
                        subFile.push_back(DirDelim);
                        subFile.append(fileid_.file());
			if(( ACE_OS::mkdir(subFile.c_str(), mode ) == FAILURE) && (errno != EEXIST) )
                        {
                                char errorText[256] = {0};
                                std::string errorDetail(strerror_r(errno, errorText, 255));
                                char errorMsg[512] = {0};
                                snprintf(errorMsg, 511, "Create(), compositeFile fileId dir creation to the path:<%s> failed, error=<%s>", subFile.c_str(), errorDetail.c_str());
                                CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
                        }
		}
	  }

	TRACE(fms_cpf_RegFileMgr, "%s", "Leaving create()");
}


//------------------------------------------------------------------------------
//      Change active subfile
//------------------------------------------------------------------------------
void RegularFileMgr::changeActiveSubfile() throw (FMS_CPF_PrivateException)
{
  throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::TYPEERROR);
}

// Only for Infinite file
void RegularFileMgr::removeBlockTransferFileLog()
{
	// do nothing
}
