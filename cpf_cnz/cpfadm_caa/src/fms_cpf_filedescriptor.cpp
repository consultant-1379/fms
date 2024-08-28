/*
 * * @file fms_cpf_filedescriptor.cpp
 *	@brief
 *	Class method implementation for FileDescriptor.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_filedescriptor.h module
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


#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filehandler.h"
#include "fms_cpf_common.h"
#include "fms_cpf_fileaccess.h"
#include "fms_cpf_filemgr.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_eventalarmhndl.h"
#include "fms_cpf_filetransferhndl.h"
#include "fms_cpf_blocktransfermgr.h"
#include "fms_cpf_tqchecker.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FileDescriptor::FileDescriptor(FileAccess* faccessp, FMS_CPF_Types::accessType access, bool inf) :
faccessp_(faccessp),
access_(access),
inf_(inf),
secure(true),
m_firstRun(true)
{
	m_fileRecordLength = faccessp_->filep_->attribute_.getRecordLength();
	fms_cpf_FileDspt = new (std::nothrow) ACS_TRA_trace("FMS_CPF_FileDescriptor");
}

FileDescriptor::~FileDescriptor()
{
	if(NULL != fms_cpf_FileDspt)
		delete fms_cpf_FileDspt;
}

//------------------------------------------------------------------------------
//      Equality operator
//------------------------------------------------------------------------------
bool FileDescriptor::operator== (const FileDescriptor& fd) const
{
  return &fd == this;
}

void FileDescriptor::setTmpName(const std::string& name)
{
	faccessp_->filep_->fileid_ = name;
}

//------------------------------------------------------------------------------
//      Rename file
//------------------------------------------------------------------------------
void FileDescriptor::rename(const FMS_CPF_FileId& fileid, const std::string& newFileDN)
							throw (FMS_CPF_PrivateException)
{
	if(access_ != FMS_CPF_Types::XR_XW_)
	{
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::ACCESSERROR);
	}

	// Rename physical file
	File* filep = faccessp_->filep_;

	if(faccessp_ == filep->faccessp_)
	{
		// Main file
		// Check if destination file is a main file
		if( !fileid.subfileAndGeneration().empty() )
		{
			throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTBASEFILE, fileid.data ());
		}

		// Check if destination file exists
		File* key = (DirectoryStructureMgr::instance())->find(fileid.file(), faccessp_->filep_->cpname_.c_str());

		if(key != NULL)
		{
			throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::FILEEXISTS, fileid.file ());
		}
  
		FileMgr filemgr(filep->fileid_, filep->volume_, filep->attribute_, faccessp_->filep_->cpname_.c_str());
		filemgr.rename(fileid);
		// Rename file entry in the file table
		filep->fileid_ = fileid;

		if(!newFileDN.empty())
			filep->setFileDN(newFileDN);
	}
	else
	{
		// Subfile
		// Check that main file is same
		if ( filep->fileid_ != fileid.file() )
		{
			throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTSAMEFILE, fileid.file() );
		}

		// Check if destination file is a subfile
		if(fileid.subfileAndGeneration().empty ())
		{
			throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTSUBFILE, fileid.data() );
		}

		if(filep->subFileExist(fileid.subfileAndGeneration()) )
		{
			throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::FILEEXISTS, fileid.data() );
		}

		FileMgr filemgr(faccessp_->fileid_, filep->volume_, filep->attribute_, faccessp_->filep_->cpname_.c_str());
		filemgr.rename(fileid);

		// update subFile list of composite file
		filep->changeSubFile(faccessp_->fileid_.subfileAndGeneration(), fileid.subfileAndGeneration());

	}
	// Update entry in file access
	faccessp_->fileid_ = fileid;
}


//------------------------------------------------------------------------------
//      Get file identity
//------------------------------------------------------------------------------
FMS_CPF_FileId FileDescriptor::getFileid() const
{
	return faccessp_->fileid_;
}

//------------------------------------------------------------------------------
//      Get volume name
//------------------------------------------------------------------------------

std::string FileDescriptor::getVolume() const
{
	return faccessp_->filep_->volume_;
}

//------------------------------------------------------------------------------
//      Set volume name
//------------------------------------------------------------------------------
void FileDescriptor::setVolume(const std::string& volume) throw (FMS_CPF_PrivateException)
{
  if(FileMgr::volumeExists(volume, faccessp_->filep_->cpname_) == false)
  {
	  throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::VOLUMENOTFOUND, volume);
  }
  faccessp_->filep_->volume_ = volume;
}

//------------------------------------------------------------------------------
//      Get file attributes
//------------------------------------------------------------------------------
FMS_CPF_Attribute FileDescriptor::getAttribute() const
{
  return faccessp_->filep_->attribute_;
}

//------------------------------------------------------------------------------
//      Change attributes
//------------------------------------------------------------------------------
void FileDescriptor::setAttribute(const FMS_CPF_Attribute& attribute) throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileDspt, "%s", "Entering in setAttribute()");

	File* filep = faccessp_->filep_;

	if(faccessp_ != filep->faccessp_)
	{
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::NOTBASEFILE);
	}

	if(filep->attribute_.type () != attribute.type ())
	{
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::ILLVALUE, "Illegal to change file type");
	}

	FMS_CPF_Types::fileAttributes oldfileattr = filep->attribute_;

	TRACE(fms_cpf_FileDspt, "%s", "setAttribute(), change attributes in the file table");
	// Change attributes in the file table
	filep->attribute_.setAttribute(attribute);

	if (faccessp_->icount_ > 0)
	{
		FMS_CPF_Types::fileAttributes newfileattr = filep->attribute_;

		if( ((newfileattr.infinite.maxtime == 0) && (oldfileattr.infinite.maxtime > 0) ) ||
			((newfileattr.infinite.maxtime > 0) && (oldfileattr.infinite.maxtime == 0)))
		{
			TRACE(fms_cpf_FileDspt, "%s", "setAttribute(), infinite file change time to switch");
			filep->attribute_.setChangeTime();
			filep->attribute_.setChangeAfterClose(true);
		}
	}

	// Change attribute file
	FileMgr filemgr(filep->fileid_, filep->volume_, filep->attribute_, faccessp_->filep_->cpname_.c_str());
	filemgr.setAttribute();

	if (faccessp_->icount_ == 0)
	{
		// Update file attributes with the new values
		filep->attribute_.update();
	}

	TRACE(fms_cpf_FileDspt, "%s", "Leaving setAttribute()");
}

//------------------------------------------------------------------------------
//      Get file record length
//------------------------------------------------------------------------------
unsigned short FileDescriptor::getRecordLength () const
{
	return m_fileRecordLength;
}

//------------------------------------------------------------------------------
//      Get access type
//------------------------------------------------------------------------------
FMS_CPF_Types::accessType FileDescriptor::getAccess() const
{
  return access_;
}

//------------------------------------------------------------------------------
//      Get number of users
//------------------------------------------------------------------------------
FMS_CPF_Types::userType FileDescriptor::getUsers() const
{
  return faccessp_->users_;
}

//------------------------------------------------------------------------------
//      Get size of the file
//------------------------------------------------------------------------------

size_t FileDescriptor::getSize() const throw(FMS_CPF_PrivateException)
{
  File* filep = faccessp_->filep_;
  FileMgr filemgr(faccessp_->fileid_, filep->volume_, filep->attribute_, faccessp_->filep_->cpname_.c_str());
  return filemgr.getSize();
}


//------------------------------------------------------------------------------
//      Get file path
//------------------------------------------------------------------------------
std::string FileDescriptor::getPath() const throw(FMS_CPF_PrivateException)
{
  File* filep = faccessp_->filep_;

  std::string path = ( ParameterHndl::instance()->getCPFroot(faccessp_->filep_->cpname_.c_str()) + DirDelim +
		          filep->volume_ + DirDelim + filep->fileid_.file() );

  if( !faccessp_->fileid_.subfileAndGeneration().empty() )
  {
    // Subfile
    path += DirDelim + faccessp_->fileid_.subfileAndGeneration ();
  }
  return path;
}

//------------------------------------------------------------------------------
//      Get last sent subfile
//------------------------------------------------------------------------------
unsigned long FileDescriptor::getLastSentSubfile() const throw(FMS_CPF_PrivateException)
{
  File* filep = faccessp_->filep_;
  if (faccessp_ != filep->faccessp_)
  {
    throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTBASEFILE);
  }
  unsigned long lastReported;
  filep->attribute_.getLastReportedSubfile(lastReported);
  return lastReported;
}

//------------------------------------------------------------------------------
//      set last sent subfile
//------------------------------------------------------------------------------
void FileDescriptor::setLastSentSubfile(const unsigned long& lastSentSubfile) throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileDspt, "Entering in %s", __func__);
	File* filep = faccessp_->filep_;
	if(faccessp_ != filep->faccessp_)
	{
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTBASEFILE);
    }

	// update last subfile sent
	filep->attribute_.setLastReportedSubfile(lastSentSubfile);

	if (faccessp_->icount_ <= 0)
	{
		std::string path( ParameterHndl::instance()->getCPFroot(faccessp_->filep_->cpname_.c_str()));
		path.push_back(DirDelim);
		path.append(filep->volume_);
		path.push_back(DirDelim);
		path.append(filep->fileid_.file());

		TRACE(fms_cpf_FileDspt, "CP file is close update attribute file:<%s>", path.c_str());

		filep->attribute_.saveFile(path);
	}
	TRACE(fms_cpf_FileDspt, "Leaving %s", __func__);
}
//------------------------------------------------------------------------------
//      Get active subfile
//------------------------------------------------------------------------------
unsigned long FileDescriptor::getActiveSubfile() const throw(FMS_CPF_PrivateException)
{
  File* filep = faccessp_->filep_;
  if (faccessp_ != filep->faccessp_)
  {
    throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTBASEFILE);
  }
  return filep->attribute_.getActiveSubfile();
}

//------------------------------------------------------------------------------
//      Get active subfile
//------------------------------------------------------------------------------
void FileDescriptor::getActiveSubfile (std::string &str) throw (FMS_CPF_PrivateException)
{
  char aStr[SUB_LENGTH] = {0};
  File* filep = faccessp_->filep_;
  if (faccessp_ != filep->faccessp_)
  {
    throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTBASEFILE);
  }
  ::sprintf( aStr,"%.10lu", filep->attribute_.getActiveSubfile() );
  str = aStr; 
}

//------------------------------------------------------------------------------
//      Get TQ DN
//------------------------------------------------------------------------------
void FileDescriptor::getTransferQueueDN(std::string& tqDN) throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileDspt, "%s, Entering", __func__);
	tqDN.clear();

	File* filep = faccessp_->filep_;
	if(faccessp_ != filep->faccessp_)
	{
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTBASEFILE);
	}

	FMS_CPF_Types::transferMode tqmode(FMS_CPF_Types::tm_NONE);
	filep->attribute_.getTQmode(tqmode);

	// check if the file is attached to a TQ
	if(FMS_CPF_Types::tm_NONE != tqmode)
	{
		TRACE(fms_cpf_FileDspt, "%s", "TQ defined");

		tqDN = filep->getTransferQueueDN();

		// Check if it is already defined
		if(tqDN.empty())
		{
			int errorCode = 0;
			bool result;
			// get the TQ name
			std::string transferQueueName;
			filep->attribute_.getTQname(transferQueueName);

			// Get TQ DN
			if(FMS_CPF_Types::tm_FILE == tqmode)
			{
				// get File TQ DN
				result = TQChecker::instance()->validateFileTQ(transferQueueName, tqDN, errorCode);
				TRACE(fms_cpf_FileDspt, "get FILE TQ:<%s> DN, OHI error:<%d>", transferQueueName.c_str(), errorCode);
			}
			else
			{
				// get Block TQ DN
				result = TQChecker::instance()->validateBlockTQ(transferQueueName, tqDN, errorCode);
				TRACE(fms_cpf_FileDspt, "get BLOCK TQ:<%s> DN, OHI error:<%d>", transferQueueName.c_str(), errorCode);
			}

			if(result)
			{
				// Store the TQ DN
				filep->setTransferQueueDN(tqDN);
			}
		}
	}

	TRACE(fms_cpf_FileDspt, "%s, Leaving TQ DN:<%s>", __func__, tqDN.c_str());
}

//------------------------------------------------------------------------------
//      Clear TQ DN
//------------------------------------------------------------------------------
void FileDescriptor::clearTransferQueueDN() throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileDspt, "%s, Entering", __func__);

	File* filep = faccessp_->filep_;
	if(faccessp_ != filep->faccessp_)
	{
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTBASEFILE);
	}

	filep->clearTransferQueueDN();

	TRACE(fms_cpf_FileDspt, "%s Leaving", __func__);
}
//------------------------------------------------------------------------------
//      Check if subfile switch ordered by command
//------------------------------------------------------------------------------
bool FileDescriptor::changeSubFileOnCommand(void) throw (FMS_CPF_PrivateException)
{
  File* filep = faccessp_->filep_;
  if (faccessp_ != filep->faccessp_)
  {
    throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTBASEFILE);
  }
  bool tmp = faccessp_->changeSubFileOnCommand;
  faccessp_->changeSubFileOnCommand = false;
  return tmp;
}

//------------------------------------------------------------------------------
//      Change active subfile
//------------------------------------------------------------------------------
unsigned long FileDescriptor::changeActiveSubfile() throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileDspt, "%s", "Entering in changeActiveSubfile()");

	File* filep = faccessp_->filep_;

	if(faccessp_ != filep->faccessp_)
	{
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::NOTBASEFILE);
	}

	unsigned long newActiveSubile;

	// Get active subfile number before the subfile switch
    unsigned long subFileToSend = filep->attribute_.getActiveSubfile();

	// Change active subfile
	// Check if file is open for infinite writing
	if (faccessp_->icount_ == 0) 
	{
		filep->attribute_.setChangeAfterClose(false);
	}

	// Increase the subfile number (N -> N+1)
	filep->attribute_.changeActiveSubfile();
	filep->attribute_.setChangeTime();

	// Get TQ name and mode before the update
    FMS_CPF_Types::transferMode tqmode;
    filep->attribute_.getTQmode(tqmode);
    std::string tqName;
    filep->attribute_.getTQname(tqName);

	try
	{
		switch(tqmode)
		{
			case FMS_CPF_Types::tm_FILE :
			{
				TRACE(fms_cpf_FileDspt, "changeActiveSubfile(), report subfile to file TQ:<%s>", tqName.c_str());
				// Check for first switch
				if( filep->createFileTQHandler() )
				{
					// Activate File TQ
					int tqActivatedResult = filep->getFileTQHandler()->setFileTQ(tqName);
					if( SUCCESS == tqActivatedResult)
					{
						//cease any alarm on this TQ
						EventReport::instance()->ceaseAlarm(tqName);

						unsigned long lastReported;

						filep->attribute_.getLastReportedSubfile(lastReported);

						// check if first subfile to transfer
						//Set previous sent subfile
						if( m_firstRun && (0 == lastReported) )
						{
							lastReported = (subFileToSend - 1);
							m_firstRun = false;
							filep->attribute_.setLastReportedSubfile(lastReported);
							TRACE(fms_cpf_FileDspt, "changeActiveSubfile(), set subfile:<%d> as last subfile reported", lastReported);
						}

						TRACE(fms_cpf_FileDspt, "changeActiveSubfile(), last subfile reported:<%d>, current to send<%d>", lastReported, subFileToSend);

						int moveResult;
						bool noError = true;
						// report the subfile
						while( ( (lastReported + 1) <= subFileToSend ) && noError)
						{
							lastReported++;
							// move subfile to TQ folder
							moveResult = filep->getFileTQHandler()->moveFileToTransfer(lastReported);
							if( SUCCESS == moveResult)
							{
								//Report the file to GOH
								if(filep->getFileTQHandler()->sendCurrentFile())
								{
									// Update last sent value
									filep->attribute_.setLastReportedSubfile(lastReported);
									TRACE(fms_cpf_FileDspt, "changeActiveSubfile(), subfile:<%d> reported", lastReported);
								}
								else
								{
									// Report error
									noError = false;
									std::string ohiError;
									filep->getFileTQHandler()->getOhiErrotText(ohiError);
									TRACE(fms_cpf_FileDspt, "changeActiveSubfile(), report subfile:<%d> failed, error:<%s>", lastReported, ohiError.c_str() );
									//raise alarm TQ error
									EventReport::instance()->raiseFileTQAlarm(tqName);
								}
							}
							else
							{
								// Move error
								TRACE(fms_cpf_FileDspt, "changeActiveSubfile(), move subfile:<%d> failed", lastReported);
								noError = false;
							}
						}
					}
					else
					{
						TRACE(fms_cpf_FileDspt, "changeActiveSubfile(), file TQ:<%s> activation error", tqName.c_str());
						// TQ activating error
						std::string mainFile = filep->fileid_.file();
						std::stringstream errorDetail;
						errorDetail << ",File: " << mainFile << " - ";
						errorDetail << "Transfer queue: " << tqName << std::ends;

						EventReport::instance()->reportException(errorDetail, FMS_CPF_PrivateException::TQNOTFOUND);

						//raise alarm TQ error
						EventReport::instance()->raiseFileTQAlarm(tqName);
					}
				}
				else
				{
					TRACE(fms_cpf_FileDspt, "changeActiveSubfile(), file TQ:<%s> handler allocation failure", tqName.c_str());
					// error happens on FileTQ handler allocation
					std::string mainFile = filep->fileid_.file();
					std::stringstream errorDetail;
					errorDetail << "File: " << mainFile << " - ";
					errorDetail << "Transfer queue: " << tqName << ", handler allocation failed" << std::ends;

					EventReport::instance()->reportException(errorDetail, FMS_CPF_PrivateException::INTERNALERROR );
				}
			}
			break;

			case FMS_CPF_Types::tm_BLOCK :
			{
				// BLOCK TQ
				TRACE(fms_cpf_FileDspt, "%s(), update subfile to block TQ:<%s>", __func__, tqName.c_str());

				// Check if Block Sender has been defined
				if(!filep->gohBlockHandlerExist)
				{
					char readySubfile[12] = {0};
					unsigned long lastReported;
					filep->attribute_.getLastReportedSubfile(lastReported);

					if( 0U != lastReported)
					{
						// Update last sent value
						sprintf(readySubfile, "%.10lu", (lastReported+1U) );
					}
					else
					{
						// Update last sent value
						sprintf(readySubfile, "%.10lu", subFileToSend);
					}

					std::string filePath(ParameterHndl::instance()->getCPFroot(filep->getCpName()));
					filePath += DirDelim;
					filePath.append(filep->getVolumeName());
					filePath += DirDelim;
					filePath.append(filep->getFileName());
					filePath += DirDelim;
					filePath.append(readySubfile);

					TRACE(fms_cpf_FileDspt, "%s(), file:<%s> attached to a blockTQ, Cp name:<%s>", __func__, filep->getFileName(), filep->getCpName() );

					filep->gohBlockHandlerExist = BlockTransferMgr::instance()->addBlockSender(filep->getFileName(),
																							   filep->getVolumeName(),
																							   filep->getCpName(),
																							   m_fileRecordLength,
																							   filePath );
			    }

				// Update with the current active subfile
				BlockTransferMgr::instance()->updateBlockSenderState(filep->getFileName(),
						   	   	   	   	   	   	   	   	   	   	   	 filep->getCpName(),
						   	   	   	   	   	   	   	   	   	   	   	 ++subFileToSend);
			}
			break;

			case FMS_CPF_Types::tm_NONE :
			{
				// Check if a TQ has been removed
		   	    FMS_CPF_Types::transferMode initTqmode;
				filep->attribute_.getInitTransfermode(initTqmode);

				// remove TQ handler if defined
				if( FMS_CPF_Types::tm_FILE == initTqmode )
				{
					filep->removeFileTQHandler();
				}
				else if( FMS_CPF_Types::tm_BLOCK == initTqmode && filep->gohBlockHandlerExist)
				{
					filep->gohBlockHandlerExist = false;
					BlockTransferMgr::instance()->removeBlockSender(filep->getFileName(), filep->getCpName());
				}

			}
			break;
		}

		// Update file attributes with the new values
		filep->attribute_.update();

		// create the new infinite subfile
		FileMgr filemgr(filep->fileid_, filep->volume_, filep->attribute_, faccessp_->filep_->cpname_.c_str());
		filemgr.changeActiveSubfile();

		// get the new active subfile
		newActiveSubile = filep->attribute_.getActiveSubfile();

		if( !tqName.empty())
		{
			// get TQ after attribute update
			std::string nextTQName;
			filep->attribute_.getTQname(nextTQName);

			// check for TQ change
			if(tqName.compare(nextTQName) != 0)
			{
				// TQ change, cease any raised alarm
				EventReport::instance()->ceaseAlarm(tqName);
			}
		}

	}//try end
	catch(const FMS_CPF_PrivateException& ex)
	{
		char errorLog[252]={'\0'};
		ACE_OS::snprintf(errorLog, 251, "changeActiveSubfile(), exception <%d>, error=<%s>", ex.errorCode(), ex.errorText());
		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		TRACE(fms_cpf_FileDspt, "%s", errorLog);
		// re-throw the same caught exception
		throw;
	}
	TRACE(fms_cpf_FileDspt, "%s", "Leaving changeActiveSubfile()");

	return newActiveSubile;
}

void FileDescriptor::changeActiveSubfileOnCommand() throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileDspt, "%s", "Entering in changeActiveSubfileOnCommand()");

	File* filep = faccessp_->filep_;

	if (faccessp_ != filep->faccessp_)
	{
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::NOTBASEFILE);
	}

	if (faccessp_->icount_ > 0)
	{
		// File is open
		unsigned int cmdDelay=1;
		// The infinite file is open set the flag for subfile switch
		faccessp_->changeSubFileOnCommand = true;
		TRACE(fms_cpf_FileDspt, "%s", "changeActiveSubfileOnCommand(), file is open set flag to switch");
		// Wait 1s before returning
		ACE_OS::sleep(cmdDelay);
	}
	else
	{
		// The infinite file is not open
		TRACE(fms_cpf_FileDspt, "%s", "changeActiveSubfileOnCommand(), change active subfile file is close");
		changeActiveSubfile();
	}

	TRACE(fms_cpf_FileDspt, "%s", "Leaving changeActiveSubfileOnCommand()");
}

//------------------------------------------------------------------------------
//      Update the attributes
//------------------------------------------------------------------------------
void FileDescriptor::updateAttributes()
{
	TRACE(fms_cpf_FileDspt, "%s", "Entering in updateAttributes()");
	File* filep = faccessp_->filep_;
    if (faccessp_ != filep->faccessp_)
    {
      throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTBASEFILE);
    }
    // Set member variable
	filep->attribute_.update();
	TRACE(fms_cpf_FileDspt, "%s", "Leaving updateAttributes()");
}

 
//------------------------------------------------------------------------------
//      Open physical file
//------------------------------------------------------------------------------
off64_t FileDescriptor::phyOpenWriteApp(const char* filePath, int &writeFd)
{
	TRACE(fms_cpf_FileDspt, "Entering in phyOpenWriteApp(), opening file<%s>", filePath );

    writeFd = ::open(filePath, O_WRONLY | O_BINARY);

    // Open error
    if(FAILURE == writeFd )
    {
   		char errorLog[512]={'\0'};
   		char errorText[256]={'\0'};
   		std::string errorDetail(strerror_r(errno, errorText, 255));
   		ACE_OS::snprintf(errorLog, 511, "%s(), write opening file<%s> failed, error=<%s>", __func__,filePath, errorDetail.c_str());
   		CPF_Log.Write(errorLog, LOG_LEVEL_ERROR);
   		TRACE(fms_cpf_FileDspt, "%s", errorLog);
   		return 0;
    }

    off64_t currentFileSize = lseek64(writeFd, 0, SEEK_END);

    if(FAILURE == currentFileSize)
    {
    	char errorLog[512]={'\0'};
    	char errorText[256]={'\0'};
    	std::string errorDetail(strerror_r(errno, errorText, 255));
    	ACE_OS::snprintf(errorLog, 511, "phyOpenWriteApp(), seek on file<%s> failed, error=<%s>", filePath, errorDetail.c_str());
    	CPF_Log.Write(errorLog, LOG_LEVEL_ERROR);
    	TRACE(fms_cpf_FileDspt, "%s", errorLog);
    }

    secure = true;

    File* filep = faccessp_->filep_;
    // Get TQ name and mode before the update
	FMS_CPF_Types::transferMode tqmode;
	filep->attribute_.getTQmode(tqmode);

	if(!filep->gohBlockHandlerExist && FMS_CPF_Types::tm_BLOCK == tqmode)
	{
		TRACE(fms_cpf_FileDspt, "%s(), file:<%s> attached to a blockTQ, Cp name:<%s>", __func__, filep->getFileName(), filep->getCpName() );
		std::string subFilePath;
		unsigned long lastReported;
		filep->attribute_.getLastReportedSubfile(lastReported);

		if( 0U != lastReported)
		{
			// Update with the last sent value
			subFilePath.assign(ParameterHndl::instance()->getCPFroot(filep->getCpName()));
			subFilePath += DirDelim;
			subFilePath.append(filep->getVolumeName());
			subFilePath += DirDelim;
			subFilePath.append(filep->getFileName());
			subFilePath += DirDelim;
			char readySubfile[12] = {0};
			sprintf(readySubfile, "%.10lu", (lastReported+1U) );
			subFilePath.append(readySubfile);
		}
		else
		{
			// Update with the current value
			subFilePath.assign(filePath);
		}

		filep->gohBlockHandlerExist = BlockTransferMgr::instance()->addBlockSender(filep->getFileName(),
																				   filep->getVolumeName(),
																				   filep->getCpName(),
																				   m_fileRecordLength,
																				   subFilePath );
	}

	TRACE(fms_cpf_FileDspt, "%s", "Leaving phyOpenWriteApp()");
	return currentFileSize;
}
//------------------------------------------------------------------------------
//      Open physical file
//------------------------------------------------------------------------------

off64_t FileDescriptor::phyOpenRW(const char* filePath, int &readFd, int &writeFd)
{
	TRACE(fms_cpf_FileDspt, "Entering in phyOpenRW(), opening file<%s>", filePath );

	// Open in read mode
	readFd = ::open(filePath, O_RDONLY | O_BINARY);

	if( FAILURE == readFd )
	{
		char errorLog[512]={'\0'};
		char errorText[256]={'\0'};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		ACE_OS::snprintf(errorLog, 511, "phyOpenRW(), opening file<%s> in read mode failed, error=<%s>", filePath, errorDetail.c_str());
		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		TRACE(fms_cpf_FileDspt, "%s", errorLog);
	}
	// Open in write mode
	writeFd = ::open(filePath, O_WRONLY | O_BINARY);

	if( FAILURE == writeFd )
	{
		char errorLog[512]={'\0'};
		char errorText[256]={'\0'};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		ACE_OS::snprintf(errorLog, 511, "%s(), write opening file<%s> in write mode failed, error=<%s>", __func__, filePath, errorDetail.c_str());
		CPF_Log.Write(errorLog, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileDspt, "%s", errorLog);
		return 0;
	}

	// Move write to end of file
	off64_t currentFileSize = lseek64(writeFd, 0, SEEK_END);

	if(FAILURE == currentFileSize)
	{
		char errorLog[512]={'\0'};
		char errorText[256]={'\0'};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		ACE_OS::snprintf(errorLog, 511, "phyOpenRW(), seek on file<%s> failed, error=<%s>", filePath, errorDetail.c_str());
		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		TRACE(fms_cpf_FileDspt, "%s", errorLog);
	}

	TRACE(fms_cpf_FileDspt, "%s", "Leaving phyOpenRW()");
	return currentFileSize;
}

//------------------------------------------------------------------------------
//      Close physical file
//------------------------------------------------------------------------------
int FileDescriptor::phyClose(int& readFD, int& writeFD)
{
	TRACE(fms_cpf_FileDspt, "%s", "Entering in phyClose()");
	int readFDCloseRes = 0;
	int writeFDCloseRes = 0;
	int result = SUCCESS;

	if(readFD > 0)
	{
		// Close read FD
		readFDCloseRes = ACE_OS::close(readFD);

		if(FAILURE == readFDCloseRes)
		{
			result = FAILURE;
			char errorLog[512]={'\0'};
			char errorText[256]={'\0'};
			std::string errorDetail(strerror_r(errno, errorText, 255));
			ACE_OS::snprintf(errorLog, 511, "phyClose(), close read FD failed, error=<%s>", errorDetail.c_str());
			CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
			TRACE(fms_cpf_FileDspt, "%s", errorLog);
		}
	}

	if(writeFD > 0)
	{
        //A successful close does not guarantee that the data has been successfully saved to disk,
		//as the kernel defers writes.
		ACE_OS::fsync(writeFD);

		// Close write FD
		writeFDCloseRes = ACE_OS::close(writeFD);
		if(FAILURE == writeFDCloseRes)
		{
			result = FAILURE;
			char errorLog[512]={'\0'};
			char errorText[256]={'\0'};
			std::string errorDetail(strerror_r(errno, errorText, 255));
			ACE_OS::snprintf(errorLog, 511, "phyClose(), close write FD failed, error=<%s>", errorDetail.c_str());
			CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
			TRACE(fms_cpf_FileDspt, "%s", errorLog);
		}
	}
	
	TRACE(fms_cpf_FileDspt, "%s", "Leaving phyClose()");
	return result;
}

//------------------------------------------------------------------------------
//      physical file exists
//------------------------------------------------------------------------------
int FileDescriptor::phyExist(const char* path)
{
  ACE_stat statbuf;
  //Checks if the physical file exists
  int status = ACE_OS::stat(path, &statbuf);
  return status;
}


//------------------------------------------------------------------------------
//      physical pwrite
//------------------------------------------------------------------------------

ssize_t FileDescriptor::phyPwrite(int fildes, const char* buf, size_t nbyte, unsigned long aRecordNumber)
{
	TRACE(fms_cpf_FileDspt, "%s", "Entering in phyPwrite()");
    char* bufPtr;
    bool allocated = false;

    unsigned short	recordLength = getRecordLength();

    if (nbyte != recordLength)
	{
		char errorLog[512]={'\0'};
		ACE_OS::snprintf(errorLog, 511, "phyPwrite(), error recordLength is <%d> but byte to write are <%zu>", recordLength, nbyte );
		TRACE(fms_cpf_FileDspt, "%s", errorLog);
	}

	// check if allocate extra memory
	if(nbyte < recordLength)
    {
		bufPtr = new char[recordLength];
		memset(bufPtr,0,recordLength);
		memcpy(bufPtr,buf,nbyte);
		allocated = true;
	}
	else
	{
		bufPtr = const_cast<char*>(buf);
	}

	off64_t oldPosition = lseek64(fildes, 0, SEEK_CUR);

	if(FAILURE == oldPosition)
	{
		char errorLog[512]={'\0'};
		char errorText[256]={'\0'};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		ACE_OS::snprintf(errorLog, 511, "phyPwrite(), lseek on file failed, error=<%s>", errorDetail.c_str());
		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		TRACE(fms_cpf_FileDspt, "%s", errorLog);
	}

	off64_t	offset = (aRecordNumber - 1) * recordLength;
	lseek64(fildes, offset, SEEK_SET);

	TRACE(fms_cpf_FileDspt, "%s", "phyPwrite(), write data to file");
	ssize_t status = ACE_OS::write(fildes, bufPtr, recordLength);

	if(FAILURE == status)
	{
		char errorLog[512]={'\0'};
		char errorText[256]={'\0'};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		ACE_OS::snprintf(errorLog, 511, "phyPwrite(), write to file failed, error=<%s>", errorDetail.c_str());
		CPF_Log.Write(errorLog, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileDspt, "%s", errorLog);
	}

    lseek64(fildes, oldPosition, SEEK_SET);

	/*
	 * TODO TRANSFER BLOCK
	    File* filep = faccessp_->filep_;
        if(filep->gohBlockHandlerExist != NULL)
		{
			GohdatablockMgr::instance()->setLastBlockNoWritten(filep->fileid_.file(), filep->cpname_.c_str(),aRecordNumber);
		}
	 */
	
    // Check if extra buffer allocated
    if(allocated)
    {
    	// reclaim the memory
    	delete[] bufPtr;
    }

    TRACE(fms_cpf_FileDspt, "%s", "Leaving phyPwrite()");
    return status;
}

//------------------------------------------------------------------------------
//     physical pread
//------------------------------------------------------------------------------
ssize_t FileDescriptor::phyPread(int fildes, char *buf, size_t nbyte, off64_t offset )
{
	TRACE(fms_cpf_FileDspt, "%s", "Entering in phyPread()");

	// Save the current position
	off64_t oldPosition = lseek64(fildes, 0, SEEK_CUR);

	// move the position to the offset from beginning of file
	off64_t localStatus = lseek64(fildes, offset, SEEK_SET);

	// read nbyte starting from new position
	ssize_t status = ACE_OS::read(fildes, buf, nbyte);

	if(FAILURE == status)
	{
		char errorLog[512]={'\0'};
		char errorText[256]={'\0'};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		ACE_OS::snprintf(errorLog, 511, "phyPread(), read to file failed, error=<%s>", errorDetail.c_str());
		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		TRACE(fms_cpf_FileDspt, "%s", errorLog);
	}

	// move the position to the oold position
	localStatus = lseek64(fildes, oldPosition, SEEK_SET);

	TRACE(fms_cpf_FileDspt, "%s", "Leaving phyPread()");
	return status;
}

//------------------------------------------------------------------------------
//  get physical fileSize
//------------------------------------------------------------------------------
off64_t FileDescriptor::phyGetFileSize(const char* path)
{
	TRACE(fms_cpf_FileDspt, "%s", "Entering in phyGetFileSize()");
	off64_t result;
	struct stat64 fileStatus;

    if(stat64(path, &fileStatus) != 0)
    {
    	char errorLog[512]={'\0'};
    	char errorText[256]={'\0'};
    	std::string errorDetail(strerror_r(errno, errorText, 255));
    	ACE_OS::snprintf(errorLog, 511, "phyGetFileSize(), stat64 on file<%s> failed, error=<%s>", path, errorDetail.c_str());
    	CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
    	TRACE(fms_cpf_FileDspt, "%s", errorLog);
    	result = -1;
    }
    else
    	result = fileStatus.st_size;

	TRACE(fms_cpf_FileDspt, "%s", "Leaving phyGetFileSize()");
    return result;
}

//------------------------------------------------------------------------------
// physical unifyFileSize
//------------------------------------------------------------------------------
long FileDescriptor::phyUnifyFileSize(const char* path, unsigned short aRecordLength)
{
	TRACE(fms_cpf_FileDspt, "%s", "Entering in phyUnifyFileSize()");

	off64_t oldUnixFileSize;
	long rest, cpFileSize;

	struct stat64 fileStatus;

	if(stat64(path, &fileStatus) != 0)
	{
		char errorLog[512]={'\0'};
		char errorText[256]={'\0'};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		ACE_OS::snprintf(errorLog, 511, "phyGetFileSize(), stat64 on file<%s> failed, error=<%s>", path, errorDetail.c_str());
		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		TRACE(fms_cpf_FileDspt, "%s", errorLog);
		return -1;
	}

	// get the file size in byte
    oldUnixFileSize = fileStatus.st_size;

    // Get the number of record stored into the file
    cpFileSize = static_cast<long> (oldUnixFileSize / aRecordLength);

    rest = static_cast<long> (oldUnixFileSize % aRecordLength);

    // check that the size is correct
    if(0 != rest)
    {
    	// correct the file size to the record length
    	off64_t newUnixFileSize;
    	newUnixFileSize = aRecordLength * (cpFileSize + 1);

    	int status = ::truncate64(path, newUnixFileSize);
    	if( FAILURE == status )
    	{
    		char errorLog[512]={'\0'};
			char errorText[256]={'\0'};
			std::string errorDetail(strerror_r(errno, errorText, 255));
			ACE_OS::snprintf(errorLog, 511, "phyGetFileSize(), truncate on file<%s> failed, error=<%s>", path, errorDetail.c_str());
			CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
			TRACE(fms_cpf_FileDspt, "%s", errorLog);
			return -1;
    	}

    	cpFileSize++;

		// Log the correction of the file size
    	std::stringstream warningDetail;
    	warningDetail << "Correction of file size: oldUnixFileSize=" << oldUnixFileSize <<  "newUnixFileSize="<< newUnixFileSize << std::ends;

		EventReport::instance()->reportException(warningDetail, FMS_CPF_PrivateException::PHYSICALERROR );

    }
    TRACE(fms_cpf_FileDspt, "%s", "Leaving phyUnifyFileSize()");
    return cpFileSize;
}

//------------------------------------------------------------------------------
// physical write to file
//------------------------------------------------------------------------------
ssize_t FileDescriptor::phyWrite( int handle,
							  const char *buf,
							  unsigned long aRecordsToWrite,
							  unsigned int count,
							  unsigned long& aLastRecordWritten
							 )
{ 
	TRACE(fms_cpf_FileDspt, "%s", "Entering in phyWrite()");

	ssize_t bytesWritten = 0;
    char* bufPtr;
    bool allocated = false;
    unsigned int expectedSize;
    unsigned short recordLength = getRecordLength();
    // the size in byte of records to write
    expectedSize = recordLength * aRecordsToWrite;
   
    if(count < expectedSize)
    {
		bufPtr = new char[expectedSize];
		memset(bufPtr, 0 , expectedSize);
		memcpy(bufPtr, buf, count);
		allocated = true;
    }
    else
    {
    	bufPtr = const_cast<char*>(buf);
    }
    TRACE(fms_cpf_FileDspt, "phyWrite(), write <%d> byte to file", expectedSize);
    // write data to the file
    bytesWritten = ACE_OS::write(handle, bufPtr, expectedSize);

    if(FAILURE == bytesWritten)
    {
    	char errorLog[512]={'\0'};
		char errorText[256]={'\0'};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		ACE_OS::snprintf(errorLog, 511, "phyWrite(), write to file failed, error=<%s>", errorDetail.c_str());
		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		TRACE(fms_cpf_FileDspt, "%s", errorLog);
    }

    // check if memory allocated
    if(allocated)
    {
    	// Reclaim the memory
        delete[] bufPtr;
    }

    // calculate last record written
    struct stat fileStatus;

    if(fstat(handle, &fileStatus) == FAILURE)
    {
    	char errorLog[512]={'\0'};
		char errorText[256]={'\0'};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		ACE_OS::snprintf(errorLog, 511, "phyWrite(), fstat on file failed, error=<%s>", errorDetail.c_str());
		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		TRACE(fms_cpf_FileDspt, "%s", errorLog);
    }
    else
    	aLastRecordWritten = static_cast<unsigned long> (fileStatus.st_size / recordLength);

    TRACE(fms_cpf_FileDspt,"Leaving phyWrite(), LastRecordWritten=<%d>, bytesWritten =<%d>", aLastRecordWritten, bytesWritten);

    return bytesWritten;
}

//------------------------------------------------------------------------------
// physical read of file
//------------------------------------------------------------------------------

ssize_t FileDescriptor::phyRead(int handle, void *buf, unsigned int count)
{ 
  return ACE_OS::read(handle, buf, count);
}

//------------------------------------------------------------------------------
//  physical file expand
//------------------------------------------------------------------------------

int FileDescriptor::phyFExpand(const char* filePath, off64_t aSubfileSize)
{
	TRACE(fms_cpf_FileDspt, "%s", "Entering in phyFExpand()");
	int status = truncate64(filePath, aSubfileSize);

	if(FAILURE == status)
	{
      	char errorLog[512]={'\0'};
  		char errorText[256]={'\0'};
  		std::string errorDetail(strerror_r(errno, errorText, 255));
  		ACE_OS::snprintf(errorLog, 511, "phyFExpand(), truncate file<%s> failed, error=<%s>", filePath, errorDetail.c_str());
  		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
  		TRACE(fms_cpf_FileDspt, "%s", errorLog);
    }

	TRACE(fms_cpf_FileDspt, "%s", "Leaving phyFExpand()");
	return status;
}

//	------------------------------------------------------------------------------
//  physical file rewrite
//------------------------------------------------------------------------------
off64_t FileDescriptor::phyRewrite(const char* path, int& readFD, int& writeFD)
{
	off64_t result;

	// Cut the file to zero size
	result = truncate64(path, 0);

	if(FAILURE == result)
	{
		char errorLog[512]={'\0'};
		char errorText[256]={'\0'};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		ACE_OS::snprintf(errorLog, 511, "phyRewrite(), truncate file<%s> failed, error=<%s>", path, errorDetail.c_str());
		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		TRACE(fms_cpf_FileDspt, "%s", errorLog);
		return result;
	}

	// reset the read FD to begin of file
	result = lseek64(readFD, 0, SEEK_SET);

	if(FAILURE == result)
	{
		char errorLog[512]={'\0'};
		char errorText[256]={'\0'};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		ACE_OS::snprintf(errorLog, 511, "phyRewrite(), seek of read FD on file<%s> failed, error=<%s>", path, errorDetail.c_str());
		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		TRACE(fms_cpf_FileDspt, "%s", errorLog);
		return result;
	}

	// reset the write FD to begin of file
	result = lseek64(writeFD, 0, SEEK_SET);

	if(FAILURE == result)
	{
		char errorLog[512]={'\0'};
		char errorText[256]={'\0'};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		ACE_OS::snprintf(errorLog, 511, "phyRewrite(), seek of write FD on file<%s> failed, error=<%s>", path, errorDetail.c_str());
		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		TRACE(fms_cpf_FileDspt, "%s", errorLog);
		return result;
	}

	return result;
}

//------------------------------------------------------------------------------
//  physical file seek
//------------------------------------------------------------------------------

off64_t FileDescriptor::phyLseek(int readFD, off64_t toPos)
{
  return lseek64(readFD, toPos, SEEK_SET);
}

