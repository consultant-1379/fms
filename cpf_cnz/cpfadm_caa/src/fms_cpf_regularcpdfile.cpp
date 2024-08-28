/*
 * * @file fms_cpf_regularcpdfile.cpp
 *	@brief
 *	Class method implementation for InfiniteCPDFile.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_infinitecpdfile.h module
 *
 *	@author enungai (Nunziante Gaito)
 *	@date 2011-11-21
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
 *	| 1.0.0  | 2011-11-21 | enungai      | File imported.                      |
 *	+========+============+==============+=====================================+
 *	| 1.1.0  | 2014-04-28 | qvincon      | Infinite subfiles removed from IMM  |
 *	+========+============+==============+=====================================+
 */

#include "fms_cpf_regularcpdfile.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filereference.h"
#include "fms_cpf_common.h"

#include "fms_cpf_file.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

namespace{
	const int SOO_CREATED_OPENED = 1; //The subfile will be created and then opened
};

RegularCPDFile::RegularCPDFile(std::string& cpname)
: CPDFile(cpname),
  unixReadFd(0),
  unixWriteFd(0)
{
  fms_cpf_regfileTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_RegCPDFile");
  TRACE(fms_cpf_regfileTrace, "%s","RegularCPDFile()");
}

//------------------------------------------------------------------------------
// ~RegularCPDFile
//------------------------------------------------------------------------------
RegularCPDFile::~RegularCPDFile()
{
	TRACE(fms_cpf_regfileTrace, "%s","~RegularCPDFile() before close");
	try
	{
    	close((unsigned char)0);  // 0=close, 1=remove
	}
	catch(const FMS_CPF_PrivateException& ex)
	{
		TRACE(fms_cpf_regfileTrace, "%s","~RegularCPDFile() exception during the close function; error = %s", ex.errorText());
	}

	delete fms_cpf_regfileTrace;
}

//------------------------------------------------------------------------------
// open
//
// 	compositeClass	mainName	Action
//------------------------------------------------------------------------------
//	false			false		FileAccess, UnixOpen
//	false			true		FileAccess
//	true			false		Illegal
//	true			true		FileAccess, UnixOpen
//------------------------------------------------------------------------------
void RegularCPDFile::open(const FMS_CPF_FileId& aFileId,
						  unsigned char aAccessType,
						  unsigned char aSubfileOption,
						  off64_t aSubfileSize,
						  unsigned long& outFileReferenceUlong,
					      unsigned short& outRecordLength,
						  unsigned long& outFileSize,
						  unsigned char& outFileType,
						  bool isInfiniteSubFile
						 ) throw(FMS_CPF_PrivateException)
{
	TRACE (fms_cpf_regfileTrace, "%s", "Entering in open(...)");
	off64_t	status;

	bool subFile = isSubFile(aFileId);
	FileReference tempFileReference;
	TRACE (fms_cpf_regfileTrace, "Open Subfileopt = %d, isSubFile = %d, isInfinite = %d", aSubfileOption, subFile, isInfiniteSubFile);

	if((SOO_CREATED_OPENED == aSubfileOption) && subFile)
	{
		if(isInfiniteSubFile)
		{
		   TRACE(fms_cpf_regfileTrace, "%s", "open(), Infinite file handling");
		   tempFileReference = DirectoryStructureMgr::instance()->create(aFileId,(FMS_CPF_Types::accessType)aAccessType, m_CpName.c_str());
		}
		else
		{
			TRACE(fms_cpf_regfileTrace, "%s", "open(), Regular file handling");

			bool fileExist = DirectoryStructureMgr::instance()->exists(aFileId, m_CpName.c_str());

			if(fileExist)
			{
				TRACE(fms_cpf_regfileTrace, "%s", "open(), SubFile to create already exist");
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILEEXISTS, aFileId.data() );
			}
			else
			{
				std::list<std::string> subFileNameList;
				subFileNameList.push_back(aFileId.subfileAndGeneration()); //HT74565

				// Get the file volume
				FMS_CPF_FileId mainfileid( aFileId.file());
				FileReference ref = DirectoryStructureMgr::instance()->open(mainfileid, FMS_CPF_Types::NONE_, m_CpName.c_str());
				std::string volume = ref->getVolume();
				DirectoryStructureMgr::instance()->closeExceptionLess(ref, m_CpName.c_str());

				TRACE(fms_cpf_regfileTrace, "open(), before to call the createInternalSubFiles file<%s>, volume<%s>", aFileId.data(), volume.c_str() );

				FMS_CPF_File fileToCreate;
				int retCode = fileToCreate.createInternalSubFiles(aFileId.file(), subFileNameList, volume, m_CpName);

				TRACE(fms_cpf_regfileTrace, "open() after the createInternalSubFiles function call retCode = %d" , retCode);

				if(SUCCESS != retCode)
				{
					char errorMsg[128] = {'\0'};
					ACE_OS::snprintf(errorMsg, 127, "RegularCPDFile::open(), error on subfiles<%s> creation error<%d>", aFileId.data(), retCode);

					TRACE(fms_cpf_regfileTrace, "%s", errorMsg);
					throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR);
				}

				tempFileReference = DirectoryStructureMgr::instance()->open(aFileId,(FMS_CPF_Types::accessType)aAccessType, m_CpName.c_str());
			}
		}
	}
	else
	{
		TRACE(fms_cpf_regfileTrace, "%s", "open(), opening an existing subfile");
		tempFileReference = DirectoryStructureMgr::instance()->open(aFileId,(FMS_CPF_Types::accessType)aAccessType, m_CpName.c_str());
	}

	setFileReference(tempFileReference);

	bool compositeClass = isCompositeClass();

	std::string filePath = getFileReference()->getPath();

	outFileSize = 0;
	// Assign return values
	outFileReferenceUlong = getFileReference();

	outRecordLength = getRecordLength();

	outFileType = getFileReference()->getAttribute().type();

	if((compositeClass == false && !subFile) || (compositeClass && subFile))
	{
		// Create subfile
		if( compositeClass && subFile && (aSubfileSize != 0))
		{
			TRACE (fms_cpf_regfileTrace, "Open(), file size from CP<%lld>", aSubfileSize);

			status = getFileReference()->phyFExpand(filePath.c_str(), aSubfileSize);

			if(status == FAILURE)
			{
				char errorMsg[128] = {'\0'};
				ACE_OS::snprintf(errorMsg, 127, "RegularCPDFile::open(), failed phyFExpand on file<%s>, size:<%zd>", filePath.c_str(), aSubfileSize);
				TRACE(fms_cpf_regfileTrace, "%s", errorMsg);
				throw FMS_CPF_PrivateException(FMS_CPF_Exception::NOMEMORY, errorMsg);
			}
		}

		// Open subfile or file
		status = getFileReference()->phyOpenRW(filePath.c_str(), unixReadFd, unixWriteFd);

		TRACE(fms_cpf_regfileTrace, "open(), read FD<%d>, write FD<%d>, file<%s> size<%lld>", unixReadFd, unixWriteFd, filePath.c_str(), status);

		if(unixReadFd == FAILURE)
		{
			char errorMsg[128] = {'\0'};
			ACE_OS::snprintf(errorMsg, 127, "RegularCPDFile::open(), file<%s> failed to open in O_RDONLY", filePath.c_str());
			TRACE(fms_cpf_regfileTrace, "%s", errorMsg);
			throw FMS_CPF_PrivateException(FMS_CPF_Exception::PHYSICALERROR, errorMsg);
		}

		if(unixWriteFd == FAILURE)
		{
			char errorMsg[128] = {'\0'};
			ACE_OS::snprintf(errorMsg, 127, "RegularCPDFile::open(), file<%s> failed to open in O_RDONLY", filePath.c_str());
			TRACE(fms_cpf_regfileTrace, "%s", errorMsg);
			throw FMS_CPF_PrivateException(FMS_CPF_Exception::PHYSICALERROR, errorMsg);
		}

		if(status == FAILURE)
		{
			char errorMsg[128] = {'\0'};
			ACE_OS::snprintf(errorMsg, 127, "RegularCPDFile::open(), file<%s> failed seek operation", filePath.c_str());
			TRACE(fms_cpf_regfileTrace, "%s", errorMsg);
			throw FMS_CPF_PrivateException(FMS_CPF_Exception::PHYSICALNOTFOUND, errorMsg);
		}

		outFileSize = unifyFileSize(filePath, outRecordLength);
	}

	TRACE(fms_cpf_regfileTrace, "%s", "Leaving open(...)");
}

//------------------------------------------------------------------------------
// close
//------------------------------------------------------------------------------
void RegularCPDFile::close(unsigned char aSubfileOption,
						   bool noSubFileChange,
						   bool syncReceived
	  	  	  	  	  	  ) throw(FMS_CPF_PrivateException)
{
	TRACE (fms_cpf_regfileTrace, "%s", "Entering in close(...)");
	UNUSED(syncReceived);
	UNUSED(noSubFileChange);
	int statusClose = 0;
	std::string removePath;

	try
	{
		// A subfile shall sometimes be removed
		if(getFileReference() != FileReference::NOREFERENCE)
		{
			TRACE(fms_cpf_regfileTrace, "close(), FD closure, aSubfileOption:<%d>", aSubfileOption);
			// Close the Unix read/write descriptor
			statusClose = getFileReference()->phyClose(unixReadFd, unixWriteFd);
			unixReadFd = 0;
			unixWriteFd = 0;

			if(isCompositeClass() && aSubfileOption == SOO_CREATED_OPENED)
			{
				std::string fileName = getFileName();
				std::string volumeName = getFileReference()->getVolume();

				TRACE(fms_cpf_regfileTrace, "%s", "close(), close file reference");
				// Close file reference to allow deletion
				DirectoryStructureMgr::instance()->closeExceptionLess(getFileReference(), m_CpName.c_str());

				TRACE(fms_cpf_regfileTrace, "close(), remove subfile:<%s>", fileName.c_str());
				FMS_CPF_File fileToDelete;
				int result = fileToDelete.deleteInternalSubFiles(fileName, volumeName, getCPName());
				if(SUCCESS != result)
				{
					char errorMsg[512] = {'\0'};
					ACE_OS::snprintf(errorMsg, 511, "RegularCPDFile::close(), error:<%d> on remove file:<%s>, volume:<%s>, Cp:<%s>", result, fileName.c_str(), volumeName.c_str(), getCPName().c_str() );
					CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
				}
			}
			else
			{
				TRACE(fms_cpf_regfileTrace, "%s", "close(), close file reference");
				DirectoryStructureMgr::instance()->closeExceptionLess(getFileReference(), m_CpName.c_str());
			}
			setFileReference(FileReference::NOREFERENCE);
		}

		if(statusClose == FAILURE)
		{
			char errorMsg[512] = {'\0'};
			ACE_OS::snprintf(errorMsg, 511, "failed to close FD read<%d>/write<%d>", unixReadFd, unixWriteFd);
			TRACE(fms_cpf_regfileTrace, "close(), error:<%s>", errorMsg);
			throw FMS_CPF_PrivateException(FMS_CPF_Exception::PHYSICALERROR, errorMsg);
		}
	}
	catch(const FMS_CPF_PrivateException& ex)
	{
		char errorMsg[512] = {'\0'};
		ACE_OS::snprintf(errorMsg, 511, "RegularCPDFile::close(), catched exception:<%d>, error: <%s>", ex.errorCode(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		// re-throw the same caught exception
		throw;
	}
	TRACE(fms_cpf_regfileTrace, "%s", "Leaving close(...)");
}

//------------------------------------------------------------------------------
// writerand
//------------------------------------------------------------------------------
void RegularCPDFile::writerand(unsigned long aRecordNumber,
							   char* aDataRecord,
							   unsigned long aDataRecordSize
							  ) throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_regfileTrace, "%s", "Entering in writerand()");
    int	bytesWritten = 0;
    // Is this a simple file or a subfile
    checkFileClass ();
    checkWriteAccess();
    bytesWritten = getFileReference()->phyPwrite(unixWriteFd, aDataRecord, aDataRecordSize, aRecordNumber);

    if( (FAILURE == bytesWritten) || ( bytesWritten < static_cast<long>(aDataRecordSize)) )
    {
    	char errorMsg[64] = {'\0'};
    	ACE_OS::snprintf(errorMsg, 63, "writerand(),  phyPwrite failed write FD<%d>; bytesToWrite =%ld, bytesWritten=%d", unixWriteFd, aDataRecordSize, bytesWritten);
    	TRACE(fms_cpf_regfileTrace, "%s", errorMsg);
    	throw FMS_CPF_PrivateException(FMS_CPF_Exception::PHYSICALERROR, errorMsg);
    }
    TRACE(fms_cpf_regfileTrace, "%s", "Leaving writerand()");
}

//------------------------------------------------------------------------------
// readrand
//------------------------------------------------------------------------------
void RegularCPDFile::readrand(unsigned long aRecordNumber,
							  char* aDataRecord
						     ) throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_regfileTrace, "%s", "Entering in readrand()");
    unsigned short recordLength = getRecordLength();
    unsigned long readPosition = (aRecordNumber - 1) * recordLength;
    int	status = 0;

    // Is this a simple file or a subfile
    checkFileClass();
    checkReadAccess(); 

    status = getFileReference()->phyPread(unixReadFd, aDataRecord, recordLength, readPosition);
    if(status != recordLength)
    {
    	throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::WRONGRECNUM);
    }
    else if(FAILURE == status)
    {
    	char errorMsg[64] = {'\0'};
    	ACE_OS::snprintf(errorMsg, 63, "RegularCPDFile::readrand(), phyPread failed read FD<%d>, recordLength =%d; status =%d", unixReadFd, recordLength, status);
    	TRACE(fms_cpf_regfileTrace, "%s", errorMsg);
    	throw FMS_CPF_PrivateException(FMS_CPF_Exception::PHYSICALERROR, errorMsg);
    }
    TRACE(fms_cpf_regfileTrace, "%s", "Leaving readrand()");
}

//------------------------------------------------------------------------------
// writenext
//------------------------------------------------------------------------------
void RegularCPDFile::writenext(unsigned long aRecordsToWrite,
							   char* aDataRecords,
							   unsigned long aDataRecordsSize,
							   unsigned long& aLastRecordWritten
							  ) throw(FMS_CPF_PrivateException)
{
	 TRACE(fms_cpf_regfileTrace, "%s", "Entering in writenext()");

	 int bytesWritten = 0;
	 // Is this a simple file or a subfile
	 checkFileClass();
	 checkWriteAccess();

	 TRACE(fms_cpf_regfileTrace, "writenext(), record to write<%d>, buffer size<%d>, last record written<%d>",
			 aRecordsToWrite, aDataRecordsSize, aLastRecordWritten);

	 bytesWritten = getFileReference()->phyWrite(unixWriteFd, aDataRecords, aRecordsToWrite,
			 	 	 	 	 	 	 	 	 	 aDataRecordsSize, aLastRecordWritten);

     if( (FAILURE == bytesWritten) || (bytesWritten < static_cast<long>(aDataRecordsSize)) )
     {
    	 char errorMsg[512] = {'\0'};
    	 ACE_OS::snprintf(errorMsg, 511, "RegularCPDFile::writenext(), write FD<%d>; bytesToWrite<%ld>; bytesWritten<%d>",unixWriteFd, aDataRecordsSize, bytesWritten);
    	 TRACE(fms_cpf_regfileTrace, "%s", errorMsg);
    	 throw FMS_CPF_PrivateException(FMS_CPF_Exception::PHYSICALERROR, errorMsg);
     }

     TRACE(fms_cpf_regfileTrace, "%s", "Leaving writenext()");
}

//------------------------------------------------------------------------------
// readnext
//------------------------------------------------------------------------------
void RegularCPDFile::readnext(unsigned long	aRecordsToRead,
							  char* aDataRecords,
							  unsigned short& aRecordsRead
							 ) throw(FMS_CPF_PrivateException)
{
	 TRACE(fms_cpf_regfileTrace, "%s", "Entering in readnext()");
     unsigned short recordLength = getRecordLength();
     unsigned long bytesToRead = aRecordsToRead * recordLength;
     int bytesRead = 0;

     // Is this a simple file or a subfile
     checkFileClass();
     checkReadAccess();

     bytesRead = getFileReference()->phyRead(unixReadFd, aDataRecords, bytesToRead);
     if(FAILURE == bytesRead)
     {
    	 char errorMsg[512] = {'\0'};
    	 ACE_OS::snprintf(errorMsg, 511, "RegularCPDFile::readnext(), phyRead failed FD read<%d>; bytesToRead =%ld; bytesRead =%d", unixReadFd, bytesToRead, bytesRead);
    	 TRACE(fms_cpf_regfileTrace, "%s", errorMsg);
    	 throw FMS_CPF_PrivateException(FMS_CPF_Exception::PHYSICALERROR, errorMsg);
     }

     aRecordsRead = bytesRead / recordLength;

     if (bytesRead == 0)
     {
    	 throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::WRONGRECNUM);
     }

     TRACE(fms_cpf_regfileTrace, "%s", "Leaving readnext()");
}

//------------------------------------------------------------------------------
// rewrite
//------------------------------------------------------------------------------
void RegularCPDFile::rewrite() throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_regfileTrace, "%s", "Entering in rewrite()");
    std::string filePath = getFileReference()->getPath();
    int	status = 0;

    // Is this a simple file or a subfile
    checkFileClass();
    checkWriteAccess(); 

    status = getFileReference()->phyRewrite(filePath.c_str(), unixReadFd, unixWriteFd);
    if(FAILURE == status)
    {
    	char errorMsg[64] = {'\0'};
    	ACE_OS::snprintf(errorMsg, 63, "RegularCPDFile::rewrite(), failed phyRewrite, read FD<%d> write FD<%d>", unixReadFd, unixWriteFd);
    	TRACE(fms_cpf_regfileTrace, "%s", errorMsg);
    	throw FMS_CPF_PrivateException(FMS_CPF_Exception::PHYSICALERROR, errorMsg);
    }

    TRACE(fms_cpf_regfileTrace, "%s", "Leaving rewrite()");
}

//------------------------------------------------------------------------------
// reset
//------------------------------------------------------------------------------
void RegularCPDFile::reset(unsigned long aRecordNumber ) throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_regfileTrace, "%s", "Entering in reset()");
    unsigned short recordLength = getRecordLength();
    unsigned long readPosition = (aRecordNumber - 1) * recordLength;
    off64_t status = 0;

	// Is this a simple file or a subfile
    checkFileClass();
    checkReadAccess(); 

    status = getFileReference()->phyLseek(unixReadFd, readPosition);

    if(status == FAILURE)
    {
      char errorMsg[64] = {'\0'};
      ACE_OS::snprintf(errorMsg, 63, "RegularCPDFile::reset(),  failed phyLseek read FD<%d>", unixReadFd);
      TRACE(fms_cpf_regfileTrace, "%s", errorMsg);
      throw FMS_CPF_PrivateException(FMS_CPF_Exception::PHYSICALERROR, errorMsg);
    }
    TRACE(fms_cpf_regfileTrace, "%s", "Leaving reset()");
}
//------------------------------------------------------------------------------
// checkSwitchSubFile
//------------------------------------------------------------------------------
void RegularCPDFile::checkSwitchSubFile(bool onTime, bool fileClosed, bool& noMoreTimeSwitch)
{
	// To avoid unused warning message
	UNUSED(onTime);
	UNUSED(fileClosed);
	noMoreTimeSwitch = true;
}
