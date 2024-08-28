//**********************************************************************
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

#include "fms_cpf_types.h"

#include "fms_cpf_filemgr.h"
#include "fms_cpf_regularfilemgr.h"
#include "fms_cpf_infinitefilemgr.h"
#include "fms_cpf_common.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_eventalarmhndl.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"

#include <ace/ACE.h>
#include <sys/stat.h>
#include <fstream>

#define BOOST_FILESYSTEM_VERSION 3

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

extern ACS_TRA_Logging CPF_Log;
//------------------------------------------------------------------------------
//      Constructor 
//------------------------------------------------------------------------------
FileMgr::FileMgr (const FMS_CPF_FileId& fileid, 
                  const std::string& volume,
                  const FMS_CPF_Attribute& attribute,
				  const char * cpname)
throw (FMS_CPF_PrivateException) :
cpname_(cpname),
filemgrp_ (NULL),
dp_(NULL)
{
	fms_cpf_FileMgr =  new (std::nothrow) ACS_TRA_trace("FMS_CPF_FileMgr");
	TRACE(fms_cpf_FileMgr, "%s", "Entering in constructor");
	std::string myVolume = volume;
	ACS_APGCC::toUpper(myVolume);

	switch( attribute.type() )
	  {
		case FMS_CPF_Types::ft_REGULAR:
		  TRACE(fms_cpf_FileMgr, "%s", "create a regular file manager");
		  filemgrp_ = new RegularFileMgr(fileid, myVolume, attribute, cpname);
		  break;

		case FMS_CPF_Types::ft_INFINITE:
		  filemgrp_ = new InfiniteFileMgr(fileid, myVolume, attribute, cpname);
		  break;

		default:
		 throw  FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR);
	  }

	TRACE(fms_cpf_FileMgr, "%s", "Leaving constructor");
}


//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FileMgr::~FileMgr()
{
	if(NULL != fms_cpf_FileMgr)
		delete fms_cpf_FileMgr;

	if(NULL != filemgrp_)
		delete filemgrp_;
}


//------------------------------------------------------------------------------
//      Check volume
//------------------------------------------------------------------------------
bool FileMgr::volumeExists(const std::string& volume, const std::string& cp)
throw (FMS_CPF_PrivateException)
{
	bool result = true;
	// assemble the volume path
	std::string path = (ParameterHndl::instance()->getCPFroot(cp.c_str()) + DirDelim + volume);

	//check if volume exists
	ACE_stat statbuf;
	if(ACE_OS::stat(path.c_str(), &statbuf) == -1)
    {
		if (errno == ENOENT)
		{
			result = false;
		}
		else
		{
			char errorText[256]={0};
			std::string errorDetail(strerror_r(errno, errorText, 255));
			EventReport::instance()->reportException(errorDetail, FMS_CPF_PrivateException::PHYSICALERROR);
		    throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail );
		}
    }
    else
    {
    	// check if a folder
		if(S_ISDIR(statbuf.st_mode) == false)
		{
			result = false;
		}
    }
    return result;
}

//------------------------------------------------------------------------------
//      Create file
//------------------------------------------------------------------------------
void FileMgr::create() throw (FMS_CPF_PrivateException)
{
	 TRACE(fms_cpf_FileMgr, "%s", "Entering in create()");

	 if(volumeExists(filemgrp_->volume_, filemgrp_->cpname_) == false )
	 {
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::VOLUMENOTFOUND,
										filemgrp_->volume_.c_str());
	 }

	 std::string path = (ParameterHndl::instance()->getCPFroot(cpname_.c_str()) + DirDelim +
			   filemgrp_->volume_ + DirDelim + filemgrp_->fileid_.file());

     TRACE(fms_cpf_FileMgr, "create(), file=<%s>", path.c_str());

     // cheks file type
     if(filemgrp_->fileid_.subfileAndGeneration().empty())
     {
    	 TRACE(fms_cpf_FileMgr, "%s", "create(), composite or simple file");
    	 //Composite file or simple
    	 // Create data file
		 filemgrp_->create();

		// Create attribute file
		//try{ /****** TODO to be analyze in next step
		 filemgrp_->attribute_.saveFile(path);
		//}
		//catch (FMS_CPF_PrivateException &dx)
		//{
			/****** TODO to be analyze in next step
			//HK91593:start
			//Roll Back the creation of main file as the creation of attribute file failed
			string filespec = path + "\\*";
			struct _finddata_t subfilename;
			long hFile = _findfirst(filespec.c_str(), &subfilename);
			while(_findnext(hFile, &subfilename)==0);
			string subfilepath = path + "\\" + subfilename.name;
			if(::remove(subfilepath.c_str()) == -1)
			{
				if(ACS_TRA_ON(fms_cpf_fileMgr))
				{
					ACS_TRA_event(&fms_cpf_fileMgr,"could not remove the subfile");
				}
			}
			else
			{
				if(ACS_TRA_ON(fms_cpf_fileMgr))
				{
					ACS_TRA_event(&fms_cpf_fileMgr,"remove the subfile sucessfully");
				}
			}
			_findclose(hFile);
			//Remove the directory (Main file)
			if(::rmdir(path.c_str())== -1)
			{
				if(ACS_TRA_ON(fms_cpf_fileMgr))
				{
					ACS_TRA_event(&fms_cpf_fileMgr,"could not remove the directory::mainfile");
				}
			}
		   //HK91593:end
			FMS_CPF_PrivateException ex (FMS_CPF_PrivateException::PHYSICALERROR,
										"Could not create attribute file");
			FMS_CPF_EventHandler::instance()->event (ex);
			throw ex;

		}TODO to be analyze in next step ********/
	  }
	  else
	  {
		// Subfile
		path += DirDelim + filemgrp_->fileid_.subfileAndGeneration();

		TRACE(fms_cpf_FileMgr, "create(), subfile=%s creation", path.c_str() );

		// Flag of file creation
		int oflag = O_WRONLY | O_CREAT | O_TRUNC | _O_BINARY;
		// Set permission 744 (RWXR--R--) on subfile
		mode_t mode = S_IRWXU | S_IRGRP | S_IROTH | S_ISUID;
		ACE_OS::umask(0);
		// Create the file
		int fileHandle = ACE_OS::open(path.c_str(), oflag, mode);
		if(FAILURE != fileHandle)
		{
			TRACE(fms_cpf_FileMgr, "%s", "create(), subfile created");
			ACE_OS::close(fileHandle);
		}
		else
		{
			TRACE(fms_cpf_FileMgr, "%s", "create(), subfile creation failed");

			char errorText[256] = {0};
			std::string errorDetail(strerror_r(errno, errorText, 255));
			char errorMsg[512] = {0};
			snprintf(errorMsg, 511, "Create(), subfile creation to the path:<%s> failed, error=<%s>", path.c_str(), errorDetail.c_str() );
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
			EventReport::instance()->reportException(errorDetail, FMS_CPF_PrivateException::PHYSICALERROR);
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail );
		 }
	  }
     TRACE(fms_cpf_FileMgr, "%s", "Leaving create()");
}


//------------------------------------------------------------------------------
//      Remove file
//------------------------------------------------------------------------------
void FileMgr::remove() throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileMgr, "%s", "Entering in remove()");

	std::string path = (ParameterHndl::instance()->getCPFroot(cpname_.c_str()) + DirDelim + filemgrp_->volume_ +
		                DirDelim + filemgrp_->fileid_.file() );

	//TRHY46076 start
	std::string compositeFilePath = ParameterHndl::instance()->getDataDiskRoot(); 
	compositeFilePath.push_back(DirDelim);
	compositeFilePath.append(cpf_imm::CompositeFileObjectName);
	compositeFilePath.push_back(DirDelim);
	compositeFilePath.append(filemgrp_->fileid_.file());
	//TRHY46076
    //Checks file type
    if(filemgrp_->fileid_.subfileAndGeneration().empty() )
    {
    	TRACE(fms_cpf_FileMgr, "remove(), main file=<%s>", path.c_str());

    	try
    	{
    		int numOfSubFile = boost::filesystem::remove_all(path);
    		TRACE(fms_cpf_FileMgr, "remove(), <%d> subfiles removed", numOfSubFile);
		//TRHY46076
		int numOfCompositeFiles = boost::filesystem::remove_all(compositeFilePath);
		TRACE(fms_cpf_FileMgr, "remove(), <%d> compositeFile dir subfiles removed", numOfCompositeFiles);
    	}
    	catch(const boost::filesystem::filesystem_error& ex)
    	{
    		char errorMsg[128]={0};
			snprintf(errorMsg, 127, "Remove file=<%s> failed", ex.what());
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_FileMgr, "%s", errorMsg);
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR);
    	}

		// Remove attribute file
		std::string attributeFilePath(path + ATTRIBUTE);
		if(::remove(attributeFilePath.c_str()) == FAILURE)
		{
			// Try to change permission of file
			//Composite file
			if(::access(attributeFilePath.c_str(), W_OK | R_OK) == FAILURE)
			{
				TRACE(fms_cpf_FileMgr, "%s", "remove(), change access permission on attribute file");
				chmod(attributeFilePath.c_str(), S_IREAD | S_IWRITE);
			}

			if(::remove(attributeFilePath.c_str()) == FAILURE)
			{
				TRACE(fms_cpf_FileMgr, "%s", "remove(), fail to remove attribute file");
				char errorText[64]={0};
				char errorMsg[128]={0};

				std::string errorDetail(strerror_r(errno, errorText, 63));
				snprintf(errorMsg, 127, "Remove attribute file=<%s> failed, error=<%s>", attributeFilePath.c_str(), errorDetail.c_str() );
				CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
			}
		}
		// Check if there is a block transfer log to delete
		filemgrp_->removeBlockTransferFileLog();
	}
	else
	{
		// Subfile
		path += DirDelim + filemgrp_->fileid_.subfileAndGeneration();

		TRACE(fms_cpf_FileMgr, "remove(), subfile=%s", path.c_str());
		// Remove subfile
		if(::remove(path.c_str()) == FAILURE)
		{
			TRACE(fms_cpf_FileMgr, "%s", "remove(), fail to remove subfile");
			char errorText[64]={0};
			char errorMsg[128]={0};
			std::string errorDetail(strerror_r(errno, errorText, 63));
			snprintf(errorMsg, 127,"%s(), Remove subfile=<%s> failed, error=<%s>",__func__, path.c_str(), errorDetail.c_str() );
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		}
	}
    TRACE(fms_cpf_FileMgr, "%s", "Leaving remove()");
}


//------------------------------------------------------------------------------
//      Rename file
//------------------------------------------------------------------------------
void FileMgr::rename(const FMS_CPF_FileId& fileid) throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileMgr, "%s", "Entering in rename()");

	// File to rename
	std::string path1 =( ParameterHndl::instance()->getCPFroot(cpname_.c_str()) + DirDelim +
						filemgrp_->volume_ + DirDelim + filemgrp_->fileid_.file() );
	// New file name
    std::string path2 = (ParameterHndl::instance()->getCPFroot(cpname_.c_str()) + DirDelim +
    					filemgrp_->volume_ + DirDelim + fileid.file() );

	//check file type
    if(filemgrp_->fileid_.subfileAndGeneration().empty() == true)
    {
    	// Main file

    	TRACE(fms_cpf_FileMgr, "rename(), old name=%s , new name=%s", path1.c_str(), path2.c_str());
        // Rename file
        if(::rename(path1.c_str(), path2.c_str()) != 0)
        {
        	TRACE(fms_cpf_FileMgr, "%s", "rename(), operation failed");

        	char errorText[256] = {0};
     		std::string errorDetail(strerror_r(errno, errorText, 255));

     		EventReport::instance()->reportException(errorDetail, FMS_CPF_PrivateException::PHYSICALERROR);
            throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
        }

        // Rename attribute file
		if(::rename((path1 + ATTRIBUTE).c_str(), (path2 + ATTRIBUTE).c_str()) != 0)
		{
			TRACE(fms_cpf_FileMgr, "%s", "rename(), Could not rename attribute file");
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, "Could not rename attribute file");
		}
	}
	else
	{
		// Subfile
		path1 += DirDelim + filemgrp_->fileid_.subfileAndGeneration();
		path2 += DirDelim + fileid.subfileAndGeneration();

		TRACE(fms_cpf_FileMgr, "rename() subfile, old name=%s , new name=%s", path1.c_str(), path2.c_str());
    
		// Rename file
		if(::rename(path1.c_str(), path2.c_str()) != 0)
		{
			TRACE(fms_cpf_FileMgr, "%s", "rename(), Could not rename subfile");
			char errorText[256] = {0};
			std::string errorDetail(strerror_r(errno, errorText, 255));

			EventReport::instance()->reportException(errorDetail, FMS_CPF_PrivateException::PHYSICALERROR);

		    throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
		}
	}
    TRACE(fms_cpf_FileMgr, "%s", "Leaving rename()");
}


//------------------------------------------------------------------------------
//      Change file attributes
//------------------------------------------------------------------------------
void FileMgr::setAttribute() throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileMgr, "%s", "Entering in setAttribute()");

	string path = ( ParameterHndl::instance()->getCPFroot(cpname_.c_str()) + DirDelim + filemgrp_->volume_ +
    			  DirDelim + filemgrp_->fileid_.file() );

    TRACE(fms_cpf_FileMgr, "setAttribute(), file=%s", path.c_str() );

    // Update attribute file
    filemgrp_->attribute_.saveFile(path);

    TRACE(fms_cpf_FileMgr, "%s", "Leaving setAttribute()");
}


//------------------------------------------------------------------------------
//      Check if file exists
//------------------------------------------------------------------------------
bool FileMgr::exists() throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileMgr, "%s", "Entering in exists()");
	bool result = true;
	std::string path = ( ParameterHndl::instance()->getCPFroot(cpname_.c_str()) + DirDelim + filemgrp_->volume_ +
			 	 	 	 DirDelim + filemgrp_->fileid_.file() );

	//Check file type
    if(filemgrp_->fileid_.subfileAndGeneration().empty() == false)
    {
       // Subfile
       path += DirDelim + filemgrp_->fileid_.subfileAndGeneration ();
    }

    TRACE(fms_cpf_FileMgr, "exists(), file=%s", path.c_str() );

    ACE_stat statbuf;
    //Checks if the physical file exists
    if(ACE_OS::stat(path.c_str(), &statbuf) == -1)
    {
    	// Check the error code
    	if(errno != ENOENT)
    	{
    		TRACE(fms_cpf_FileMgr, "exists(), error=%i", errno);
    		//Some other error occurs
    		char errorText[256] = {0};
    		std::string errorDetail(strerror_r(errno, errorText, 255));

    		EventReport::instance()->reportException(errorDetail, FMS_CPF_PrivateException::PHYSICALERROR);

    		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
    	}
    	TRACE(fms_cpf_FileMgr,"%s" ,"exists(), file does not exist");
    	result = false;
    }

    TRACE(fms_cpf_FileMgr,"%s" ,"Leaving exists()");
	return result;
}

//------------------------------------------------------------------------------
//      Get size of the file
//------------------------------------------------------------------------------
size_t FileMgr::getSize() const throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileMgr, "%s", "getSize()");
	return filemgrp_->getSize();
}


//------------------------------------------------------------------------------
//      Change active subfile
//------------------------------------------------------------------------------
void FileMgr::changeActiveSubfile() throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileMgr, "%s", "Entering in changeActiveSubfile()");

	filemgrp_->changeActiveSubfile();

    TRACE(fms_cpf_FileMgr, "%s", "Leaving changeActiveSubfile()");
}


//------------------------------------------------------------------------------
//      Open directory
//------------------------------------------------------------------------------
void FileMgr::opendir() throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileMgr, "%s", "Entering in opendir()");

	std::string path = (ParameterHndl::instance()->getCPFroot(cpname_.c_str()) + DirDelim + filemgrp_->volume_ +
						DirDelim + filemgrp_->fileid_.file() );

	TRACE(fms_cpf_FileMgr, "opendir(), dir=%s", path.c_str() );

	// open the directory
	if((dp_ = ::opendir(path.c_str())) == 0)
	{
		TRACE(fms_cpf_FileMgr, "opendir(), fail error=%s", ::strerror(errno) );
		throw 	FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, ::strerror(errno));
    }
	TRACE(fms_cpf_FileMgr, "%s", "Leaving opendir()");
}
  

//------------------------------------------------------------------------------
//      Read next entry from directory
//------------------------------------------------------------------------------
FMS_CPF_FileId FileMgr::readdir() const
{
	TRACE(fms_cpf_FileMgr, "%s", "Entering in readdir()");

	struct dirent* dirp;
	FMS_CPF_FileId fileid;
	while( (dirp = ::readdir(dp_)) != 0)
    {
		fileid = filemgrp_->fileid_.file() + SubFileSep + dirp->d_name;

		if(fileid.isValid() == true)
		{
			TRACE(fms_cpf_FileMgr, "%s", "readdir(), found a valid file");
			return fileid;
		}
    }

	TRACE(fms_cpf_FileMgr, "%s", "readdir(), not found other valid files");
	return FMS_CPF_FileId::NIL;
}


//------------------------------------------------------------------------------
//      Close directory
//------------------------------------------------------------------------------
void FileMgr::closedir() throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileMgr, "%s", "Entering in closedir()");

	if(::closedir(dp_) != 0)
	{
		TRACE(fms_cpf_FileMgr, "closedir(), fail to close dir, error=%i", errno);
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, "FileMgr::closedir");
	}
	TRACE(fms_cpf_FileMgr, "%s", "Leaving closedir()");
}
