/*
 * * @file fms_cpf_infinitecpdfile.cpp
 *	@brief
 *	Class method implementation for InfiniteCPDFile.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_infinitecpdfile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-22
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
 *	| 1.0.0  | 2011-11-22 | qvincon      | File imported.                      |
 *	+========+============+==============+=====================================+
 *	| 1.1.0  | 2014-04-28 | qvincon      | Infinite subfiles removed from IMM  |
 *	+========+============+==============+=====================================+
 */
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_infinitecpdfile.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_common.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

#include <sys/times.h>
#include <sys/timeb.h>
#include <sys/stat.h>

/*============================================================================
	ROUTINE: InfiniteCPDFile
 ============================================================================ */
InfiniteCPDFile::InfiniteCPDFile(std::string& cpname) :
CPDFile(cpname),
m_CurrentRecord(1),
m_WriteFd(-1),
m_ActiveSubFile(1),
m_SubFileReference(0),
m_AccessMode(FMS_CPF_Types::NONE_)
{
	fms_cpf_InfCPDFileTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_InfiniteCPDFile");
}

/*============================================================================
	ROUTINE: ~InfiniteCPDFile
 ============================================================================ */
InfiniteCPDFile::~InfiniteCPDFile()
{
	if( getFileReference() != FileReference::NOREFERENCE )
	{
		TRACE(fms_cpf_InfCPDFileTrace, "%s", "Destroying InfiniteCPDFile, close open file");
		try
		{
			// 0=close, 1=remove
			close ( static_cast<unsigned char>(0) );
		}
		catch(const FMS_CPF_PrivateException& ex)
		{
			 char errorLog[64]={'\0'};
			 ACE_OS::snprintf(errorLog, 63, "InfiniteCPDFile::~InfiniteCPDFile, catch exception<%d> ", ex.errorCode() );
			 CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		}
	}

	if(NULL != fms_cpf_InfCPDFileTrace)
  		delete fms_cpf_InfCPDFileTrace;
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
void InfiniteCPDFile::open(const FMS_CPF_FileId& aFileId,
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
	TRACE(fms_cpf_InfCPDFileTrace, "open(...), file <%s>, access mode=<%d>, subFileOption=<%d>",
									aFileId.data(), static_cast<int>(aAccessType),  static_cast<int>(aSubfileOption));
	//DBG CPF_Log.Write("InfiniteCPDFile::open, IN", LOG_LEVEL_INFO);

	// To avoid warning messages
	UNUSED(aSubfileSize);
	UNUSED(isInfiniteSubFile);

	FileReference tempFileReference;
	m_MainFileName = aFileId.file();

	if(aSubfileOption == 255) // HE96743
	{
		// Note that this is a special case of aSubfileOption (255) to be able to
		// open the file similar to cpfife as a "normal" file and not an infinite file.
		tempFileReference = DirectoryStructureMgr::instance()->open(aFileId, FMS_CPF_Types::NONE_, m_CpName.c_str());
	}
	else
	{
		tempFileReference = DirectoryStructureMgr::instance()->open(aFileId, FMS_CPF_Types::NONE_, m_CpName.c_str(), true);
	}

	TRACE(fms_cpf_InfCPDFileTrace, "open(), file reference=<%d>", tempFileReference.getReferenceValue());
	// Now the file is open no exception occurs
	try
	{
		m_VolumeName = tempFileReference->getVolume();

		setFileReference(tempFileReference);
		m_AccessMode = (FMS_CPF_Types::accessType)aAccessType;

		// Set internal variables
		m_ActiveSubFile = getFileReference()->getActiveSubfile();

		openSubfile();

		TRACE(fms_cpf_InfCPDFileTrace,"%s","open(), subfile opened");
		//DBG CPF_Log.Write("InfiniteCPDFile::open, subfile opened", LOG_LEVEL_INFO);
		m_CurrentRecord = findRecord();

		// Assign return values
		outFileReferenceUlong = getFileReference();
		outRecordLength = getRecordLength();
		outFileType = getFileReference()->getAttribute().type();

		std::string filePath;
		activeSubfilePath(filePath);

		outFileSize = unifyFileSize(filePath, outRecordLength);
	}
	catch(const FMS_CPF_PrivateException& ex)
	{
		char errorLog[256]={'\0'};
	  	ACE_OS::snprintf(errorLog, 255, "InfiniteCPDFile::open, catch exception<%d>, file=<%s>", ex.errorCode(), aFileId.data() );
		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		TRACE(fms_cpf_InfCPDFileTrace,"%s", errorLog);
		DirectoryStructureMgr::instance()->closeExceptionLess(tempFileReference, m_CpName.c_str() );
		// re-throw the same catched exception
		throw;
	}
	TRACE(fms_cpf_InfCPDFileTrace, "%s", "Leaving open");
	//DBG CPF_Log.Write("InfiniteCPDFile::open, OUT", LOG_LEVEL_INFO);
}

/*============================================================================
	ROUTINE: close
 ============================================================================ */
void InfiniteCPDFile::close(unsigned char aSubfileOption,
							bool noSubFileChange,
							bool syncReceived
						   ) throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_InfCPDFileTrace, "%s", "Entering in close");
	//DBG CPF_Log.Write("InfiniteCPDFile::close, IN", LOG_LEVEL_INFO);

	// to avoid unused warning message
	UNUSED(aSubfileOption);
	try
	{
		// Close active subfile
	    closeSubfile();
	
		// Check if subfile change is disabled
		if(!noSubFileChange && checkRelease())
		{
			bool secure;
			int changeCheck;
			changeCheck = mustChangeSubfile(secure);
			TRACE(fms_cpf_InfCPDFileTrace, "close, change subFile check=<%d>", changeCheck );

			if( (1 == changeCheck))
			{
				// If release flag is true then a subfile change should be
				// be done at closing since MAXTIME and/or MAXSIZE is true also
				TRACE(fms_cpf_InfCPDFileTrace, "%s", "close, MAXTIME and/or MAXSIZE is true" );
				//DBG CPF_Log.Write("InfiniteCPDFile::close, close, MAXTIME and/or MAXSIZE is true", LOG_LEVEL_INFO);

				m_ActiveSubFile = getFileReference()->changeActiveSubfile();

			}
			else
			{
				if( (0 == changeCheck) && !syncReceived)
				{
					// If only release flag is true then a subfile change should be
					// be done at closing instead of at opening.
					TRACE(fms_cpf_InfCPDFileTrace, "%s", "close, not received sync" );
					m_ActiveSubFile = getFileReference()->changeActiveSubfile();
				}
		  }
	   }

	   if(getFileReference() != FileReference::NOREFERENCE )
	   {
		   TRACE(fms_cpf_InfCPDFileTrace, "%s", "close, main file" );

		   DirectoryStructureMgr::instance()->closeExceptionLess(getFileReference(), m_CpName.c_str());
	
		   setFileReference(FileReference::NOREFERENCE);
	   }
  }
  catch(const FMS_CPF_PrivateException& ex)
  {
	  char errorLog[256]={'\0'};
	  ACE_OS::snprintf(errorLog, 255, "InfiniteCPDFile::close, catch exception<%d>", ex.errorCode() );
	  CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
	  TRACE(fms_cpf_InfCPDFileTrace,"%s", errorLog);

	  closeSubfile();

	  if(getFileReference() != FileReference::NOREFERENCE )
	  {
	  	   TRACE(fms_cpf_InfCPDFileTrace, "%s", "close, main file" );

	  	   DirectoryStructureMgr::instance()->closeExceptionLess(getFileReference(), m_CpName.c_str());

	  	   setFileReference(FileReference::NOREFERENCE);
	  }

	  // re-throw the same catched exception
	  throw;
  }

  TRACE(fms_cpf_InfCPDFileTrace, "%s", "Leaving close" );
  //DBG CPF_Log.Write("InfiniteCPDFile::close, OUT", LOG_LEVEL_INFO);
}

/*============================================================================
	ROUTINE: writerand
 ============================================================================ */
void InfiniteCPDFile::writerand(unsigned long aRecordNumber,
							    char* aDataRecord,
							    unsigned long aDataRecordSize
							   ) throw(FMS_CPF_PrivateException)
{
	// To avoid warning message
	UNUSED(aRecordNumber);
	UNUSED(aDataRecord);
	UNUSED(aDataRecordSize);
	CPF_Log.Write("infiniteCPDFile::writerand", LOG_LEVEL_INFO);
 	throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::TYPEERROR, "infiniteCPDFile::writerand");
}

/*============================================================================
	ROUTINE: readrand
 ============================================================================ */
void InfiniteCPDFile::readrand(unsigned long aRecordNumber,
							   char* aDataRecord
							  ) throw(FMS_CPF_PrivateException)
{
	UNUSED(aRecordNumber);
	UNUSED(aDataRecord);
	CPF_Log.Write("infiniteCPDFile::readrand", LOG_LEVEL_INFO);
	throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::TYPEERROR, "infiniteCPDFile::readrand");
}

/*============================================================================
	ROUTINE: writenext
 ============================================================================ */
void InfiniteCPDFile::writenext(unsigned long aRecordsToWrite,
 	  	   	   	   	   	   	    char* aDataRecords,
							    unsigned long aDataRecordsSize,
							    unsigned long& aLastRecordWritten
							   ) throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_InfCPDFileTrace, "%s", "Entering in writenext");

    try
    {
		unsigned long	subfileBufferSize	= 0;
		unsigned short	recordLength = getRecordLength ();
		bool secure = true;

		unsigned long nextSubFile = getFileReference()->getActiveSubfile();

		if(m_ActiveSubFile != nextSubFile)
		{
			TRACE(fms_cpf_InfCPDFileTrace, "writenext(), switch subFile from <%d> to <%d>", m_ActiveSubFile, nextSubFile);

			// Close active subfile
			closeSubfile();

			// set internal variables
			m_ActiveSubFile = nextSubFile;
			m_CurrentRecord = 1;
			// create or open subfile if it already exists
			openSubfile();
		}

		int changeCkeck;
		
		TRACE(fms_cpf_InfCPDFileTrace, "writenext(), start writing of <%d> records", aRecordsToWrite);

		for(unsigned int i = 0; i < aRecordsToWrite; i++)
		{
			subfileBufferSize = 0;
			if(aDataRecordsSize >= (i * recordLength) )
			{
				subfileBufferSize = aDataRecordsSize - i * recordLength;
			}

			if(aDataRecordsSize > recordLength)
			{
				if(subfileBufferSize < recordLength)
				{
					writerandSubfile(m_CurrentRecord, &aDataRecords[i * recordLength], subfileBufferSize);
				}
				else
				{
					writerandSubfile(m_CurrentRecord, &aDataRecords[i * recordLength], recordLength);
				}
			}
			else
			{
				writerandSubfile(m_CurrentRecord, &aDataRecords [i * recordLength], aDataRecordsSize);
			}

			m_CurrentRecord++;

			changeCkeck = mustChangeSubfile(secure);

			TRACE(fms_cpf_InfCPDFileTrace, "writenext(), current Record:<%d>, change subFile:<%d>", m_CurrentRecord, changeCkeck);

			// Time or size is fulfilled
			if(changeCkeck == 1)
			{
				//Release condition not set
				if(checkRelease() == false)
				{
					TRACE(fms_cpf_InfCPDFileTrace, "writenext(), current active subFile:<%d>", m_ActiveSubFile);

					switchSubFile();
				}
			}
		}//end for

		TRACE(fms_cpf_InfCPDFileTrace, "%s", "writenext(), all records are been written");

		if(secure)
		{
		  struct stat64 fileStatus;
		  fstat64(m_WriteFd, &fileStatus);
		  aLastRecordWritten = static_cast<unsigned long>( fileStatus.st_size / recordLength);
		}
		else
		  aLastRecordWritten = m_CurrentRecord - 1;

		TRACE(fms_cpf_InfCPDFileTrace, "writenext(), last record written:<%d>", aLastRecordWritten);

    }
    catch(const FMS_CPF_PrivateException& ex)
    {
		  char errorLog[256] = {'\0'};
		  ACE_OS::snprintf(errorLog, 255,"InfiniteCPDFile::writenext(), exception <%d> catched, detail:<%s>", ex.errorCode(), ex.detailInfo().c_str());
		  CPF_Log.Write(errorLog, LOG_LEVEL_INFO);
		  TRACE(fms_cpf_InfCPDFileTrace, "%s", errorLog);

		  // re-throw the same caught exception
		  throw;
	}
    TRACE(fms_cpf_InfCPDFileTrace, "%s", "Leaving writenext()");
}

/*============================================================================
	ROUTINE: readnext
 ============================================================================ */
void InfiniteCPDFile::readnext(unsigned long aRecordsToRead,
							   char* aDataRecords,
							   unsigned short& aRecordsRead
  	  	 	 	 	 	 	  ) throw(FMS_CPF_PrivateException)
{
	//To avoid unused warning
	UNUSED(aRecordsToRead);
	UNUSED(aDataRecords);
	aRecordsRead = 0;
	CPF_Log.Write("infiniteCPDFile::readnext", LOG_LEVEL_INFO);
	throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::TYPEERROR, "infiniteCPDFile::readnext");
}

/*============================================================================
	ROUTINE: rewrite
 ============================================================================ */
void InfiniteCPDFile::rewrite() throw(FMS_CPF_PrivateException)
{
	CPF_Log.Write("infiniteCPDFile::rewrite", LOG_LEVEL_INFO);
	throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::TYPEERROR, "infiniteCPDFile::rewrite");
}

/*============================================================================
	ROUTINE: reset
 ============================================================================ */
void InfiniteCPDFile::reset(unsigned long aRecordNumber) throw(FMS_CPF_PrivateException)
{
	//To avoid unused warning
	UNUSED(aRecordNumber);
	CPF_Log.Write("infiniteCPDFile::reset", LOG_LEVEL_INFO);
	throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::TYPEERROR, "infiniteCPDFile::reset");
}

/*============================================================================
	ROUTINE: getAttributes
 ============================================================================ */
FMS_CPF_Types::infiniteType InfiniteCPDFile::getAttributes()
{
	FMS_CPF_Attribute attribute = getFileReference()->getAttribute();
	FMS_CPF_Types::fileAttributes fileattr = attribute;
	FMS_CPF_Types::infiniteType value = fileattr.infinite;
	return value;
}

/*============================================================================
	ROUTINE: openSubfile
 ============================================================================ */
void InfiniteCPDFile::openSubfile() throw(FMS_CPF_PrivateException)
{
	 TRACE(fms_cpf_InfCPDFileTrace, "%s", "Entering in openSubfile()" );

	 std::string subFileName;
	 ulong2subfile(m_ActiveSubFile, subFileName);

	 // get the main file name
	 std::string subFileId = getFileReference()->getFileid().file();
	 // complete subfile name
	 subFileId += "-" + subFileName;

	 TRACE(fms_cpf_InfCPDFileTrace, "openSubfile(), subFile=<%s>", subFileId.c_str() );

	 if( DirectoryStructureMgr::instance()->exists(subFileId, m_CpName.c_str()) )
	 {
		 TRACE(fms_cpf_InfCPDFileTrace, "%s", "openSubfile(), subfile exists open it");
		 m_SubFileReference = DirectoryStructureMgr::instance()->open(subFileId, m_AccessMode, m_CpName.c_str());
	 }
	 else
	 {
		 TRACE(fms_cpf_InfCPDFileTrace, "%s", "openSubfile(), create subfile");
		 m_SubFileReference = DirectoryStructureMgr::instance()->create(subFileId,  m_AccessMode, m_CpName.c_str());
	 }

	 TRACE(fms_cpf_InfCPDFileTrace, "%s", "openSubfile(), subfile opened/created" );

	 try
	 {
		 std::string filePath = m_SubFileReference->getPath();

		 // Open subfile
		 off64_t status = getFileReference()->phyOpenWriteApp(filePath.c_str(), m_WriteFd);

		 if(m_WriteFd == -1)
		 {
			 char errorLog[128]={'\0'};
			 ACE_OS::snprintf(errorLog, 127, "InfiniteCPDFile::openSubfile(), opening file<%s> failed", filePath.c_str());
			 TRACE(fms_cpf_InfCPDFileTrace, "%s",errorLog);
			 throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorLog );
		 }
    
		 if (status == -1)
		 {
			 char errorLog[128]={'\0'};
			 ACE_OS::snprintf(errorLog, 127, "InfiniteCPDFile::openSubfile(), lseek on file<%s> failed", filePath.c_str());
			 TRACE(fms_cpf_InfCPDFileTrace, "%s",errorLog);
			 throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorLog );
		 }
	 }
	 catch(const FMS_CPF_PrivateException& ex)
	 {
		 char errorLog[256]={'\0'};
		 ACE_OS::snprintf(errorLog, 255, "InfiniteCPDFile::openSubfile, catch exception=<%d>", ex.errorCode() );
		 CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		 TRACE(fms_cpf_InfCPDFileTrace,"%s", errorLog);
		 closeWithoutException(m_SubFileReference);
		 // re-throw the same catched exception
		 throw;
	 }

	 TRACE(fms_cpf_InfCPDFileTrace, "%s", "Leaving openSubfile");
}

/*============================================================================
	ROUTINE: closeSubfile
 ============================================================================ */
void InfiniteCPDFile::closeSubfile() throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_InfCPDFileTrace, "%s", "Entering in closeSubfile");

	int status = 0;

	// Close the write file descriptor
	if(m_WriteFd > 0)
	{
		TRACE(fms_cpf_InfCPDFileTrace, "%s", "closeSubfile, write FD closure");
		int dummyFD = 0;
		status = getFileReference()->phyClose(dummyFD, m_WriteFd);
		m_WriteFd = 0;
		// Part solution to HD46742 and HD47588
		getFileReference()->updateAttributes();
		TRACE(fms_cpf_InfCPDFileTrace, "%s", "closeSubfile, attrinute updated");
	}

	if(m_SubFileReference != FileReference::NOREFERENCE)
	{
		DirectoryStructureMgr::instance()->closeExceptionLess(m_SubFileReference, m_CpName.c_str());
		m_SubFileReference = FileReference::NOREFERENCE;
	}

	if(status == -1)
	{
		TRACE(fms_cpf_InfCPDFileTrace, "%s", "closeSubfile, error on write FD closure");
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, "InfiniteCPDFile::closeSubfile, error on write FD closure");
	}
	TRACE(fms_cpf_InfCPDFileTrace, "%s", "Leaving closeSubfile");
}

/*============================================================================
	ROUTINE: writerandSubfile
 ============================================================================ */
void InfiniteCPDFile::writerandSubfile(unsigned long aRecordNumber,
									   char* aBuffer,
									   unsigned long aBufferSize
									  ) throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_InfCPDFileTrace, "%s", "Entering in writerandSubfile");
    ssize_t	bytesWritten = 0;

    checkWriteAccess();

    TRACE(fms_cpf_InfCPDFileTrace, "Entering in writerandSubfile, write record:<%d>, buffer size:<%d>", aRecordNumber, aBufferSize );

    bytesWritten = getFileReference()->phyPwrite(m_WriteFd, aBuffer, aBufferSize, aRecordNumber);

    if( (FAILURE == bytesWritten) || (bytesWritten < aBufferSize ))
    {
    	char errorLog[512]={'\0'};
    	ACE_OS::snprintf(errorLog, 511, "InfiniteCPDFile::writerandSubfile(), bytesToWrite=<%zu>, bytesWritten=<%zu>, error:<%d>", aBufferSize, bytesWritten, errno);
    	CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
    	TRACE(fms_cpf_InfCPDFileTrace, "%s", errorLog);
        throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorLog);
    }

    TRACE(fms_cpf_InfCPDFileTrace, "%s", "Leaving writerandSubfile");
}

/*============================================================================
	ROUTINE: activeSubfilePath
 ============================================================================ */
void InfiniteCPDFile::activeSubfilePath(std::string& path) throw(FMS_CPF_PrivateException)
{
	std::string subFileName;
	m_ActiveSubFile = getFileReference()->getActiveSubfile();
	ulong2subfile(m_ActiveSubFile,subFileName);

	path = getFileReference()->getPath();
	path += DirDelim + subFileName;

	TRACE(fms_cpf_InfCPDFileTrace,"activeSubfilePath(), active subfile:<%s>, path=<%s>", subFileName.c_str(), path.c_str() );
}

/*============================================================================
	ROUTINE: checkAttributes
 ============================================================================ */
bool InfiniteCPDFile::checkAttributes()
{
	bool result = false;
	FMS_CPF_Types::infiniteType attr = getAttributes();
	
	if((attr.maxtime !=0) || (attr.maxsize != 0))
	{
		result = true;
	}

	TRACE(fms_cpf_InfCPDFileTrace,"checkAttributes(), result<%s>", (result ? "TRUE":"FALSE") );
	return result;
}

/*============================================================================
	ROUTINE: checkRelease
 ============================================================================ */
bool InfiniteCPDFile::checkRelease()
{
	bool result;
	FMS_CPF_Types::infiniteType attr = getAttributes();

	result = attr.release;

	TRACE(fms_cpf_InfCPDFileTrace,"checkRelease(), result<%s>", (result ? "TRUE":"FALSE") );
	return result;
}

/*============================================================================
	ROUTINE: mustChangeSubfile
 ============================================================================ */
int InfiniteCPDFile::mustChangeSubfile(bool& secure) throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_InfCPDFileTrace, "%s", "Entering in mustChangeSubfile");
	secure = true;

	bool maxTimeSet = false;
	bool maxSizeSet = false;
	FMS_CPF_Types::infiniteType attr = getAttributes();
	std::string path;

	// get subfile path
	activeSubfilePath(path);
	
	off64_t fileSize = getFileSize(path.c_str());

	if((fileSize == 0) && (attr.release == true) &&
	   (attr.maxsize == 0) && (attr.maxtime == 0) )
	{
		TRACE(fms_cpf_InfCPDFileTrace, "mustChangeSubfile(), fileSize<%ld>, release<%s>", fileSize, (attr.release ? "TRUE" : "FALSE"));
		return 0;
	}

	TRACE(fms_cpf_InfCPDFileTrace, "mustChangeSubfile(), maxSize<%d>, maxTime<%d>", attr.maxsize, attr.maxtime);

	if(attr.maxtime != 0) maxTimeSet = true;

	if(attr.maxsize != 0) maxSizeSet = true;
	
	//Is either of maxtime or maxsize set?
	if(maxTimeSet || maxSizeSet)
	{
		TRACE(fms_cpf_InfCPDFileTrace, "%s", "mustChangeSubfile, maxTime or maxSize");
		fileSize = (fileSize / getRecordLength());

		//Check if it is maxsize that is valid.
		if((attr.maxsize != 0) && (fileSize >= attr.maxsize))
		{
			// Attribute value reached and time to change subfile
			return 1;
		}
		else
		{
			TRACE(fms_cpf_InfCPDFileTrace, "%s", "mustChangeSubfile, check time");
			//Is it maxtime that is used?
			if(attr.maxtime != 0)
			{
				time_t fileTime = getFileReference()->getAttribute().getChangeTime();
				struct timeb timebuffer;
				ftime(&timebuffer);
			
				time_t currentTime = timebuffer.time;
			 
				if (currentTime > fileTime)
				{
					// Attribute value reached and time to change subfile
					return 1;
				}
			}
		}
	}
	
	if (maxSizeSet || maxTimeSet)
	{
		//Attribute values set but not fulfilled
		return 2;
	}

	TRACE(fms_cpf_InfCPDFileTrace, "%s", "Leaving mustChangeSubfile");
	//Attribute values not set at all
	return 0;
}

/*============================================================================
	ROUTINE: ulong2subfile
 ============================================================================ */
void InfiniteCPDFile::ulong2subfile(unsigned long value, std::string& strValue)
{
  char tmpName[12]={'\0'};
  ACE_OS::snprintf(tmpName, 11, "%.10lu", value);
  strValue = tmpName;
}

/*============================================================================
	ROUTINE: existSubfile
 ============================================================================ */
bool InfiniteCPDFile::existSubfile()
{
	std::string path;
	activeSubfilePath(path);
  
	bool result =(getFileReference()->phyExist(path.c_str()) == 0 );

	TRACE(fms_cpf_InfCPDFileTrace,"existSubfile(), result<%s>", (result ? "TRUE":"FALSE") );
	return result;
}

/*============================================================================
	ROUTINE: findRecord
 ============================================================================ */
unsigned long InfiniteCPDFile::findRecord()
{
	TRACE(fms_cpf_InfCPDFileTrace, "%s","Entering in findRecord()");
	// Must start with 1
	unsigned long currentRecord = 1;
	unsigned short recordLength;
	std::string path;

	activeSubfilePath(path);
	off64_t currentFileSize = getFileSize(path.c_str());

	if(currentFileSize != -1L)
	{
		recordLength = getRecordLength();
		currentRecord = static_cast<unsigned long>(currentFileSize/recordLength + 1);
	}

	TRACE(fms_cpf_InfCPDFileTrace, "Leaving findRecord(), current record:<%d>", currentRecord);
	return currentRecord;
}

/*============================================================================
	ROUTINE: checkSwitchSubFile
 ============================================================================ */
void InfiniteCPDFile::checkSwitchSubFile(bool onTime, bool fileClosed, bool& noMoreTimeSwitch)
{
	TRACE(fms_cpf_InfCPDFileTrace, "Entering in checkSwitchSubFile(), onTime=<%s>, fileClosed=<%s>", (onTime ? "TRUE":"FALSE"), (fileClosed ? "TRUE":"FALSE") );

    // noMoreTimeSwitch is used when an infinite file is closed and
    // there should be a switch due to maxtime. It indicates when
    // there will be no more switch (not valid when the file is open).
    noMoreTimeSwitch = false;

    // Check if cpfife command
    if(getFileReference()->changeSubFileOnCommand())
    {
    	TRACE(fms_cpf_InfCPDFileTrace, "%s", "checkSwitchSubFile, switch on command");

    	switchSubFile();

    	TRACE(fms_cpf_InfCPDFileTrace, "checkSwitchSubFile, new active subFile:<%d>", m_ActiveSubFile);
    }
    else if(onTime)
    {
    	TRACE(fms_cpf_InfCPDFileTrace, "%s", "checkSwitchSubFile, check if switch on time");

    	// get the change time of subfile
    	time_t chTime = getFileReference()->getAttribute().getChangeTime();

    	if(chTime > 0)
    	{
    		// Get current time
    		struct timeb timebuffer;
    		ftime(&timebuffer);

    		time_t timeNow = timebuffer.time;

    		TRACE(fms_cpf_InfCPDFileTrace, "checkSwitchSubFile, Time left for next Subfile Switch)= %lu", (chTime-timeNow) );

    		// Check if time to switch
	  	 	if(timeNow > chTime)
	  	 	{
	  	 		if( (m_CurrentRecord > 1) && (fileClosed || !checkRelease()) )
	  	 		{
					TRACE(fms_cpf_InfCPDFileTrace, "checkSwitchSubFile, current record:<%d>", m_CurrentRecord);
	  	 			switchSubFile();
	  	 		}
	  	 		noMoreTimeSwitch = true;
	  	 	}
	  	 	else
	  	 	{
	  	 		if(m_CurrentRecord == 1)
	  	 		{
	  	 			noMoreTimeSwitch = true;
	  	 		}
	  	 	}
    	}
    	else
    	{
    		noMoreTimeSwitch = true;
    	}
    }

    TRACE(fms_cpf_InfCPDFileTrace, "Leaving checkSwitchSubFile, noMoreTimeSwitch=<%s>", (noMoreTimeSwitch ? "TRUE":"FALSE") );
}

/*============================================================================
	ROUTINE: switchSubFile
 ============================================================================ */
void InfiniteCPDFile::switchSubFile()
{
	TRACE(fms_cpf_InfCPDFileTrace,"%s", "Entering switchSubFile");

	// close current subfile
	closeSubfile();

	// Increase active subfile
	// Set internal variables
	m_ActiveSubFile = getFileReference()->changeActiveSubfile();
	m_CurrentRecord = 1;

	// open the new subfile and update IMM
	openSubfile();

	TRACE(fms_cpf_InfCPDFileTrace, "Leaving switchSubFile, new active subFile:<%d>", m_ActiveSubFile);
}
