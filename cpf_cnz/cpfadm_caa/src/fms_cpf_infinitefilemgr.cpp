/*
 * * @file fms_cpf_infitefilemgr.cpp
 *	@brief
 *	Class method implementation for InfiniteFileMgr.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_infinitefilemgr.h module
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
 *	| 1.0.0  | 2011-10-12 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_infinitefilemgr.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_common.h"
#include "fms_cpf_eventalarmhndl.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include "ace/ACE.h"

extern ACS_TRA_Logging CPF_Log;
//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
InfiniteFileMgr::InfiniteFileMgr(const FMS_CPF_FileId& fileid,
                                  const std::string& volume,
								  const FMS_CPF_Attribute& attribute,
								  const char* cpname) :
BaseFileMgr(fileid, volume, attribute, cpname)
{
	fms_cpf_InfFileMgr =  new (std::nothrow) ACS_TRA_trace("FMS_CPF_InfiniteFileMgr");
}


//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------

InfiniteFileMgr::~InfiniteFileMgr()
{
	if(NULL != fms_cpf_InfFileMgr)
		delete fms_cpf_InfFileMgr;
}

//------------------------------------------------------------------------------
//      Create file
//------------------------------------------------------------------------------
void InfiniteFileMgr::create() throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_InfFileMgr, "%s", "Entering in create()");
   	std::string infiniteFilePath;

	infiniteFilePath = ParameterHndl::instance()->getCPFroot(cpname_.c_str()) + DirDelim +
						volume_ + DirDelim + fileid_.file();

    // Infinite file creation
	// Set permission 755 (RWXR-XR-X) on folder
	mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
	ACE_OS::umask(0);
	// Creation of infinite file
	if( (ACE_OS::mkdir(infiniteFilePath.c_str(), mode ) == FAILURE) && (errno != EEXIST) )
	{
		char errorText[256] = {'\0'};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		char errorMsg[512] = {0};
		snprintf(errorMsg, 511, "Create(), infinite file creation to the path:<%s> failed, error=<%s>", infiniteFilePath.c_str(), errorDetail.c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::PHYSICALERROR, errorDetail );
	}

	// Create first subfile
	std::string filePath = infiniteFilePath + DirDelim;
	std::string subFileName;
	attribute_.changeActiveSubfile();
	attribute_.getActiveSubfile(subFileName);

	filePath.append(subFileName);

	// Flag of file creation
	int oflag = O_WRONLY | O_CREAT | O_TRUNC | _O_BINARY;

	// Set permission 744 (RWXR--R--) on subfile
	mode = S_IRWXU | S_IRGRP | S_IROTH;

	ACE_OS::umask(0);

	// Create the physical subFile
	int fileHandle = ACE_OS::open(filePath.c_str(), oflag, mode);
	if(FAILURE != fileHandle)
	{
		TRACE(fms_cpf_InfFileMgr, "%s", "create(), first subfile created");
		ACE_OS::close(fileHandle);
	}
	else
	{
		TRACE(fms_cpf_InfFileMgr, "%s", "create() error on subfile creation");
		char errorText[256] = {0};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		char errorMsg[512] = {0};
		snprintf(errorMsg, 511, "Create(), infinite subfile file creation to the path:<%s> failed, error=<%s>", filePath.c_str(), errorDetail.c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);

		EventReport::instance()->reportException(errorDetail, FMS_CPF_PrivateException::PHYSICALERROR );
		::remove(infiniteFilePath.c_str());
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
	}
	TRACE(fms_cpf_InfFileMgr, "%s", "Leaving in create()");
}

//------------------------------------------------------------------------------
//      Change active subfile
//------------------------------------------------------------------------------
void InfiniteFileMgr::changeActiveSubfile() throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_InfFileMgr, "%s", "Entering in changeActiveSubfile()");
	// Create data file
	std::string subfile;
	attribute_.getActiveSubfile(subfile);

	std::string path = (ParameterHndl::instance()->getCPFroot(cpname_.c_str())) + DirDelim
						+ volume_ + DirDelim + fileid_.file();

	int fd;
	// Flag of file creation
	int oflag = O_WRONLY | O_CREAT | O_TRUNC | O_EXCL | _O_BINARY;
	// Set permission 744 (RWXR--R--) on subfile
	mode_t mode = S_IRWXU | S_IRGRP | S_IROTH;
	ACE_OS::umask(0);
	bool retryOnError = false;

	do
	{
		std::string subfilePath = path + DirDelim;
		subfilePath += subfile;

		// Create the physical subFile
		if((fd = ACE_OS::open(subfilePath.c_str(), oflag, mode)) != -1)
		{
			TRACE(fms_cpf_InfFileMgr, "changeActiveSubfile(), new subfile<%s> created", subfilePath.c_str());
			ACE_OS::close(fd);
			retryOnError = false;
		}
		else
		{
			//Error on subfile creation
			if(EEXIST == errno)
			{
				attribute_.changeActiveSubfile();
				attribute_.getActiveSubfile(subfile);
				retryOnError = true;

				char errorMsg[256] = {0};
				snprintf(errorMsg, 255, "InfiniteFileMgr::changeActiveSubfile(), new subfile<%s> already exists", subfilePath.c_str());
				CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
				TRACE(fms_cpf_InfFileMgr, "%s", errorMsg);
			}
			else
			{
				retryOnError = false;
				char errorText[256] = {0};
				char errorMsg[512] = {0};
				std::string errorDetail(strerror_r(errno, errorText, 255));
				snprintf(errorMsg, 511, "InfiniteFileMgr::changeActiveSubfile(), error<%s> to create new subfile<%s>", errorDetail.c_str(), subfilePath.c_str());
				CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
				TRACE(fms_cpf_InfFileMgr, "%s", errorMsg);
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
			}
		}
	}while(retryOnError);

	// Save the attribute file
	try
	{
		attribute_.saveFile(path);
	}
	catch (FMS_CPF_PrivateException& exp )
	{
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR,	"Could not write attribute file");;
	}

	TRACE(fms_cpf_InfFileMgr, "%s", "Leaving in changeActiveSubfile()");
}

//------------------------------------------------------------------------------
//      Remove the block transfer log file
//------------------------------------------------------------------------------
void InfiniteFileMgr::removeBlockTransferFileLog()
{
	TRACE(fms_cpf_InfFileMgr, "Entering in %s", __func__);
	// set the log file name with full path
	std::string fileLogPath(ParameterHndl::instance()->getCPFlogDir(cpname_.c_str()) + DirDelim );
	fileLogPath.append(fileid_.file());
	fileLogPath.append(logExt);

	TRACE(fms_cpf_InfFileMgr, "%s(), remove log file:<%s>", __func__, fileLogPath.c_str());
	if( (remove(fileLogPath.c_str()) == FAILURE) && (errno != ENOENT) )
	{
		char errorText[64] = {0};
		std::string errorDetail(strerror_r(errno, errorText, 63));
		char errorMsg[128] = {0};
		snprintf(errorMsg, 511, "%s(), remove of log file:<%s> failed, error=<%s>", __func__, fileLogPath.c_str(), errorDetail.c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
	}
	TRACE(fms_cpf_InfFileMgr, "Leaving %s", __func__);
}
