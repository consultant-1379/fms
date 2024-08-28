/*
 * * @file fms_cpf_importfile.cpp
 *	@brief
 *	Class method implementation for CPF_ImportFile_Request.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_importfile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-09-23
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
 *	| 1.0.0  | 2011-09-23 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#define BOOST_FILESYSTEM_VERSION 3

#include "fms_cpf_importfile.h"
#include "fms_cpf_common.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filereference.h"
#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_attribute.h"

#include "fms_cpf_file.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"
#include <ace/ACE.h>
#include <sys/stat.h>

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

extern ACS_TRA_Logging CPF_Log;

namespace unzipInfo{
		std::string extensionZIP("ZIP");
		std::string zipCmd("/usr/bin/unzip");
		std::string extractOption(" -jod ");
		std::string listOption(" -Z2 ");
		const char optionSpace= ' ';
}

// detail on error
namespace {
	const char FMSGROUPNAME[] = "FMSUSRGRP";
}

CPF_ImportFile_Request::FileSearch::FileSearch() :
m_first(true),
dirp(NULL),
direntp(NULL),
m_path("")
{
}

CPF_ImportFile_Request::FileSearch::FileSearch(const std::string& path) :
m_first(true),
dirp(NULL),
direntp(NULL),
m_path(path)
{
}

bool CPF_ImportFile_Request::FileSearch::open()
{
	bool result = false;
	if(m_first)
	{
		if((dirp = ACE_OS::opendir(m_path.c_str())) != 0)
		{
			m_first = false;
			result = true;
		}
	}
	return result;
}

bool CPF_ImportFile_Request::FileSearch::find( std::string& entry)
    throw (FMS_CPF_PrivateException)
{
    bool result = false;

    if(m_first)
    {
    	m_first = false;

		if((dirp = ACE_OS::opendir(m_path.c_str())) == 0)
		{
			char errorText[512] = {'\0'};
			std::string errorDetail(strerror_r(errno, errorText, 511));
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
		}
    }

    if((direntp = ACE_OS::readdir(dirp)) != 0)
    {
       entry = direntp->d_name;
       result = true;
    }

    return result;
}

void CPF_ImportFile_Request::FileSearch::close()
{
	ACE_OS::closedir(dirp);
}

/*============================================================================
	ROUTINE: CPF_ImportFile_Request
 ============================================================================ */
CPF_ImportFile_Request::CPF_ImportFile_Request(const importFileData& importInfo)
: m_ImportInfo(importInfo),
  m_ExitCode(0),
  m_ExitMessage(),
  m_ComCliExitMessage(),
  cpf_ImportFileTrace( new (std::nothrow) ACS_TRA_trace("CPF_ImportFile_Request"))
{

}

/*============================================================================
	ROUTINE: executeImport
 ============================================================================ */
bool CPF_ImportFile_Request::executeImport()
{
	TRACE(cpf_ImportFileTrace, "%s", "Entering in executeImport()");
	bool result = false;
	FMS_CPF_PrivateException resultOK(FMS_CPF_Exception::OK);
	FileReference dstFileReference;
	try{

		bool isSingleFile;
		bool isZippedFile;
		std::list<std::string> subFilesToCreate;
		std::list<std::string> srcFileList;

		FMS_CPF_FileId dstFileId(m_ImportInfo.dstFile);

		isSingleFile = checkSourceFolder(m_ImportInfo.srcPath, srcFileList, isZippedFile);

		dstFileReference = (DirectoryStructureMgr::instance())->open(dstFileId.file(), FMS_CPF_Types::XR_XW_, m_ImportInfo.cpName.c_str());

		FMS_CPF_Attribute dstFileAttribute = dstFileReference->getAttribute();

		if(dstFileAttribute.type() == FMS_CPF_Types::ft_INFINITE)
		{
			TRACE(cpf_ImportFileTrace, "%s", "executeImport(), destination file infinite not allowed");
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::TYPEERROR);
		}

		m_ImportInfo.dstVolume = dstFileReference->getVolume();

		if(dstFileId.subfileAndGeneration().empty() && dstFileAttribute.composite() )
		{
			TRACE(cpf_ImportFileTrace, "%s", "executeImport(), import a folder to composite file");
			TRACE(cpf_ImportFileTrace, "volume = %s", m_ImportInfo.dstVolume.c_str());

			// set the composite physical path
			m_ImportInfo.dstPath = dstFileReference->getPath();

			// import to a composite file
			if(!checkSourceFileName(srcFileList))
			{
				TRACE(cpf_ImportFileTrace, "%s", "executeImport(), source file name not valid");
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INVALIDREF);
			}

			// List of subfiles of composite file
			std::list<FMS_CPF_FileId> dstFileList;
			DirectoryStructureMgr::instance()->getListFileId(dstFileId, dstFileList, m_ImportInfo.cpName.c_str());

			// Assemble the list of subfile to create
			makeFilesList(srcFileList, dstFileList, subFilesToCreate);

			DirectoryStructureMgr::instance()->close(dstFileReference, m_ImportInfo.cpName.c_str());

			importToCompositeFile(srcFileList, subFilesToCreate, isSingleFile, isZippedFile);

			TRACE(cpf_ImportFileTrace, "%s", "executeImport(), files imported to composite file");
			// CP files Copied
			result = true;
		}
		else
		{
			TRACE(cpf_ImportFileTrace, "%s", "executeImport(), import a file to subfile/simple file");
			if(!isSingleFile)
			{
				TRACE(cpf_ImportFileTrace, "%s", "executeImport(), import a folder to a subfile not allowed");
			    std::string errorDetail(m_ImportInfo.relativeSrcPath);
			    errorDetail.append(errorText::isAFolder);
				throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
			}

			// Check error import to subfile of simple file
			if(!dstFileId.subfileAndGeneration().empty() && !dstFileAttribute.composite())
			{
				TRACE(cpf_ImportFileTrace, "%s", "executeImport(), simple file have not subfile");
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::NOTCOMPOSITE, dstFileId.file());
			}

			// check if the subfile exist
			bool file_exists = DirectoryStructureMgr::instance()->exists(dstFileId, m_ImportInfo.cpName.c_str());

			// check copy mode if destination file exist
			if(file_exists && (FMS_CPF_Types::cm_NORMAL == m_ImportInfo.importOption ) )
			{
				TRACE(cpf_ImportFileTrace, "%s", "executeImport(), simple/subfile file exists and copy mode normal");
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILEEXISTS, m_ImportInfo.dstFile);
			}

			DirectoryStructureMgr::instance()->close(dstFileReference, m_ImportInfo.cpName.c_str());

			if(!file_exists)
			{
				std::string subFileName = dstFileId.subfileAndGeneration();
				TRACE(cpf_ImportFileTrace, "executeImport(), subfile=<%s> must be created", subFileName.c_str() );
				subFilesToCreate.push_back(subFileName);
				// create dst subfile
				if( !createSubFiles(subFilesToCreate) )
				{
					TRACE(cpf_ImportFileTrace, "%s", "executeImport(), error on dst subfile creation");
					throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::GENERAL_FAULT);
				}
			}

			importToSubFile();

			TRACE(cpf_ImportFileTrace, "%s", "executeImport(), files imported to subfile/simple");
			// CP files Copied
			result = true;
		}
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		m_ExitCode = static_cast<int>(ex.errorCode());
		m_ExitMessage = ex.errorText();
		m_ExitMessage.append(ex.detailInfo());

		m_ComCliExitMessage = m_ExitMessage;

		// Close dst file
		DirectoryStructureMgr::instance()->closeExceptionLess(dstFileReference, m_ImportInfo.cpName.c_str() );
		TRACE(cpf_ImportFileTrace, "executeImport() exception=<%s%s> on import", ex.errorText(), ex.detailInfo().c_str());
	}

	TRACE(cpf_ImportFileTrace, "%s", "Leaving executeImport()");
	return result;
}

/*============================================================================
	ROUTINE: importToCompositeFile
 ============================================================================ */
void CPF_ImportFile_Request::importToSubFile()
throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_ImportFileTrace, "%s", "Entering in importToSubFile()");

	//std::fstream::openmode oflag;  
	int oflag;               // HW79785
	FileReference dstFileReference;
	// Open destination file
	dstFileReference = DirectoryStructureMgr::instance()->open(m_ImportInfo.dstFile, FMS_CPF_Types::XR_XW_, m_ImportInfo.cpName.c_str());

	// set the subfile physical path
	m_ImportInfo.dstPath = dstFileReference->getPath();

	try
	{
		switch(m_ImportInfo.importOption)
		{
			case FMS_CPF_Types::cm_NORMAL:
			case FMS_CPF_Types::cm_CLEAR:
			case FMS_CPF_Types::cm_OVERWRITE:
				{
					// set the open flag of physical file
					// oflag = fstream::binary | fstream::trunc;
					oflag = O_BINARY | O_TRUNC; //HW79785
				}
				break;

			case FMS_CPF_Types::cm_APPEND:
				{
					// oflag = fstream::binary | fstream::app;
					oflag = O_BINARY | O_APPEND; //HW79785
				}
				break;

			default: // Never reached
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
		}
		TRACE(cpf_ImportFileTrace, "importToSubFile(), source file=<%s>", m_ImportInfo.srcPath.c_str());

		copyFile(m_ImportInfo.srcPath, m_ImportInfo.dstPath, oflag);

	}
	catch(FMS_CPF_PrivateException& ex)
	{
		TRACE(cpf_ImportFileTrace, "%s", "importToSubFile(), catched an exception");
		DirectoryStructureMgr::instance()->closeExceptionLess(dstFileReference, m_ImportInfo.cpName.c_str());
		throw;
	}

	DirectoryStructureMgr::instance()->closeExceptionLess(dstFileReference, m_ImportInfo.cpName.c_str());

	TRACE(cpf_ImportFileTrace, "%s", "Leaving importToSubFile()");
}

/*============================================================================
	ROUTINE: importToCompositeFile
 ============================================================================ */
void CPF_ImportFile_Request::importToCompositeFile(const std::list<std::string>& importFileList, const std::list<std::string>& fileCreateList, bool isSingleFile, bool isZipped)
throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_ImportFileTrace, "%s", "Entering in importToCompositeFile()");
	// std::fstream::openmode oflag;
	int oflag;         // HW79785
    FileReference dstFileReference;

	switch(m_ImportInfo.importOption)
	{
		case FMS_CPF_Types::cm_NORMAL:
		case FMS_CPF_Types::cm_CLEAR:
		case FMS_CPF_Types::cm_OVERWRITE:
			{
				// set the open flag of physical file
				// oflag = fstream::binary | fstream::trunc;
				oflag = O_TRUNC | O_BINARY; //HW79785
			}
			break;

		case FMS_CPF_Types::cm_APPEND:
			{
				if(isZipped)
				{
					TRACE(cpf_ImportFileTrace, "%s", "importToCompositeFile(), mode append with zip file not allowed");
					throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::UNREASONABLE, errorText::ModeNotAllowed);
				}
				// oflag = fstream::binary | fstream::app;
				oflag = O_BINARY | O_APPEND; //HW79785
			}
			break;

		default:
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
	}

	if(!removeSubFiles())
	{
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
	}
	try
	{
		std::list<std::string>::const_iterator importIterator;

		if(createSubFiles(fileCreateList))
		{
			dstFileReference = DirectoryStructureMgr::instance()->open(m_ImportInfo.dstFile, FMS_CPF_Types::XR_XW_, m_ImportInfo.cpName.c_str());

			if(isZipped)
			{
				TRACE(cpf_ImportFileTrace, "%s", "importToCompositeFile(), unzipping files");
				// import from a zip file, unzip into the cp-file folder
				int unZipResult = exctractZippedFile(m_ImportInfo.srcPath, m_ImportInfo.dstPath);
				if( SUCCESS != unZipResult)
				{
					char errorText[256] = {0};
					snprintf(errorText, 255, "%s(), unzip:<%s> fails, error=<%i>", __func__, m_ImportInfo.srcPath.c_str(), unZipResult );
					CPF_Log.Write(errorText, LOG_LEVEL_ERROR);

					std::string detail(errorText::unzipFail);
					detail.append(m_ImportInfo.relativeSrcPath);

					// An error happens on extraction
					throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, detail);
				}

				checkLowerCaseName(importFileList, m_ImportInfo.dstPath);
			}
			else
			{
				// Import from a folder
				if(isSingleFile)
				{
					importIterator = importFileList.begin();
					std::string dstSubFileName = *importIterator;
					ACS_APGCC::toUpper(dstSubFileName);
					std::string dstSubFilePath = m_ImportInfo.dstPath + DirDelim + dstSubFileName;
					copyFile(m_ImportInfo.srcPath, dstSubFilePath, oflag);
				}
				else
				{
					// copy each src file to the subfile
					for(importIterator = importFileList.begin(); importIterator != importFileList.end(); ++importIterator)
					{
						// set the src subfile name
						std::string dstSubFileName = *importIterator;
						ACS_APGCC::toUpper(dstSubFileName);
						std::string srcSubFilePath = m_ImportInfo.srcPath + DirDelim + (*importIterator);
						std::string dstSubFilePath = m_ImportInfo.dstPath + DirDelim + dstSubFileName;
						copyFile(srcSubFilePath, dstSubFilePath, oflag);
					}
				}
			}
			DirectoryStructureMgr::instance()->closeExceptionLess(dstFileReference, m_ImportInfo.cpName.c_str());
		}
		else
		{
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
		}
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		TRACE(cpf_ImportFileTrace, "importToCompositeFile(), exception=<%s>", ex.errorText());
		DirectoryStructureMgr::instance()->closeExceptionLess(dstFileReference, m_ImportInfo.cpName.c_str());
		throw;
	}

	TRACE(cpf_ImportFileTrace, "%s", "Leaving importToCompositeFile()");
}
/* HW79785 BEGIN
 * 
 */
/*============================================================================
	ROUTINE: copyfile
 ============================================================================ */
void CPF_ImportFile_Request::copyFile(const std::string& srcFilePath, const std::string& dstFilePath, int oflag /*std::fstream::openmode oflag*/)
throw (FMS_CPF_PrivateException)
{
	const int BUFFSIZE = 4096;
	char buf[BUFFSIZE];
	char errorText[512] = {'\0'};
	ACE_HANDLE srcFileHandle;
	ACE_HANDLE dstFileHandle;
	ssize_t numOfByte;
	unsigned int count = 0;
	bool b_isEnvGEP5EGEMandMaxSize = false;

	TRACE(cpf_ImportFileTrace, "%s", "Entering in copyFile()");
	snprintf(errorText, 511, "%s() begin", __func__ );
	CPF_Log.Write(errorText, LOG_LEVEL_TRACE);

	if((srcFileHandle = ACE_OS::open(srcFilePath.c_str(), O_RDONLY | O_BINARY)) == -1)
	{
		TRACE(cpf_ImportFileTrace, "copyFile(), error to open src file=<%s>", srcFilePath.c_str());
		std::string errorDetail(strerror_r(errno, errorText, 511));
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
	}

	TRACE(cpf_ImportFileTrace, "copyFile(), src file=<%s> opened", srcFilePath.c_str());
	snprintf(errorText, 511,"copyFile(), src file=<%s> opened", srcFilePath.c_str());
	CPF_Log.Write(errorText, LOG_LEVEL_TRACE);

	if ((dstFileHandle = ACE_OS::open(dstFilePath.c_str(), O_WRONLY | O_BINARY | oflag )) == -1)
	{
		TRACE(cpf_ImportFileTrace, "copyFile(), error to open dst file=<%s>", dstFilePath.c_str());
		std::string errorDetail(strerror_r(errno, errorText, 511));
		ACE_OS::close(srcFileHandle);
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
	}

	TRACE(cpf_ImportFileTrace, "copyFile(), dst file=<%s> opened", dstFilePath.c_str());
	snprintf(errorText, 511, "copyFile(), dst file=<%s> opened", dstFilePath.c_str());
	CPF_Log.Write(errorText, LOG_LEVEL_TRACE);

	if ( (checkFileSize (srcFilePath)))       // check whether source file size => 100MB
	{
		if ((checkNodeArch()) && (checkHWVersionInfo()))  // check if node architecure is EGEM2 and hw version is GEP5
		{
			b_isEnvGEP5EGEMandMaxSize = true;
			TRACE(cpf_ImportFileTrace, " src file size >= 100mb :: <%d>", b_isEnvGEP5EGEMandMaxSize);
			snprintf(errorText, 511, "CPF_ExportFile_Request::copyFile, src file size >= 100mb :: <%d>::<%s>", b_isEnvGEP5EGEMandMaxSize ,srcFilePath.c_str());
			CPF_Log.Write(errorText, LOG_LEVEL_WARN);
		}
	}

	do{
		numOfByte = ACE_OS::read(srcFileHandle, &buf, BUFFSIZE);

		if(numOfByte > 0)
		{
			if(ACE_OS::write(dstFileHandle, &buf, numOfByte) == -1)
			{
				TRACE(cpf_ImportFileTrace, "copyFile(), error to write dst file=<%s>", dstFilePath.c_str());
				std::string errorDetail(strerror_r(errno, errorText, 511));
				ACE_OS::close(srcFileHandle);
				ACE_OS::close(dstFileHandle);
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
			}

			if(b_isEnvGEP5EGEMandMaxSize)
			{
				++count;                                           
				if(count == 10) 
				{
					TRACE(cpf_ImportFileTrace, "copyFile(), count reached=<%d> , sleep for 1000 micro seconds", count);
					count = 0; 
					// reset the counter to zero
					usleep(1000);
				}
			}
		}
		else if(numOfByte < 0)
		{
			TRACE(cpf_ImportFileTrace, "copyFile(), error to read src file=<%s>", srcFilePath.c_str());
			std::string errorDetail(strerror_r(errno, errorText, 511));

			ACE_OS::close(srcFileHandle);
			ACE_OS::close(dstFileHandle);

			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail );	
		}
	}while(numOfByte > 0);

	ACE_OS::close(srcFileHandle);
	ACE_OS::close(dstFileHandle);

	TRACE(cpf_ImportFileTrace, "%s", "Leaving copyFile()");
	snprintf(errorText, 511, "%s() Leaving", __func__ );
	CPF_Log.Write(errorText, LOG_LEVEL_TRACE);
	/*
	TRACE(cpf_ImportFileTrace, "%s", "Entering in copyfile()");
	char errorText[512] = {'\0'};
	std::ifstream srcFile(srcFilePath.c_str(), fstream::binary);

	if(srcFile.fail())
	{
		TRACE(cpf_ImportFileTrace, "copyfile() on import, error to open src file=<%s>", srcFilePath.c_str());
		std::string errorDetail(strerror_r(errno, errorText, 511));
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
	}

	std::ofstream dstFile(dstFilePath.c_str(), oflag);

	if(dstFile.fail())
	{
		std::string errorDetail(strerror_r(errno, errorText, 511));
		srcFile.close();
		TRACE(cpf_ImportFileTrace, "copyfile() on import, error to open dst file=<%s>", dstFilePath.c_str());
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
	}
	// Get file size to skip copy of zero file size,
	// since always we have failbit set on destination stream

	// get pointer to associated buffer object
	std::filebuf* pbufferInput = srcFile.rdbuf();

	// get file size using buffer's members
	std::streamoff fileSize = pbufferInput->pubseekoff(0, srcFile.end, srcFile.in);
	pbufferInput->pubseekpos(0, srcFile.in);

	// check the file size
	if( fileSize > 0L )
	{
		dstFile << srcFile.rdbuf();
		dstFile.flush();

		if( dstFile.bad() || dstFile.fail())
		{
			TRACE(cpf_ImportFileTrace, "copyfile() on import <%s> error", srcFilePath.c_str());
			std::string errorDetail(strerror_r(errno, errorText, 511));
			srcFile.close();
			dstFile.close();
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
		}
	}
	srcFile.close();
	dstFile.close();

	TRACE(cpf_ImportFileTrace, "%s", "Leaving copyfile()"); */
}
/* HW79785 BEGIN
 * 
 */
/*============================================================================
	ROUTINE: createSubFiles
 ============================================================================ */
bool CPF_ImportFile_Request::createSubFiles(const std::list<std::string>& subFileNameList)
{
	TRACE(cpf_ImportFileTrace, "%s", "Entering in createSubFiles()");
	bool result = true;

	if(!subFileNameList.empty())
	{
		int retCode = 0;
		FMS_CPF_FileId dstFileId(m_ImportInfo.dstFile);
		FMS_CPF_File fileToCreate;
		retCode = fileToCreate.createInternalSubFiles(dstFileId.file(), subFileNameList, m_ImportInfo.dstVolume, m_ImportInfo.cpName );

		if(0 != retCode)
		{
			TRACE(cpf_ImportFileTrace, "%s", "createSubFiles(), error during subfiles creation");
			result= false;
		}
	}
	TRACE(cpf_ImportFileTrace, "%s", "Leaving createSubFiles()");
	return result;

}

/*============================================================================
	ROUTINE: checkSourceFileName
 ============================================================================ */
bool CPF_ImportFile_Request::checkSourceFileName(const std::list<std::string>& fileList)
{
	TRACE(cpf_ImportFileTrace, "%s", "Entering in checkSourceFileName()");
	bool result = true;
	std::list<std::string>::const_iterator srcIterator;
	FMS_CPF_FileId dstFileId(m_ImportInfo.dstFile);

	for(srcIterator = fileList.begin(); srcIterator != fileList.end(); ++srcIterator)
	{
		std::string subFileName= dstFileId.file() + "-" + (*srcIterator);
		ACS_APGCC::toUpper(subFileName);
		FMS_CPF_FileId subDstFileId(subFileName);
		if(!subDstFileId.isValid())
		{
			result = false;
			TRACE(cpf_ImportFileTrace, "checkSourceFileName(), source file name=<%s> not valid", (*srcIterator).c_str());
			break;
		}
	}
	TRACE(cpf_ImportFileTrace, "%s", "Leaving checkSourceFileName()");
	return result;
}

/*============================================================================
	ROUTINE: makeFilesList
 ============================================================================ */
void CPF_ImportFile_Request::makeFilesList(const std::list<std::string>& srcFileList, const std::list<FMS_CPF_FileId>& dstFileList, std::list<std::string>& subFilesToCreate)
		throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_ImportFileTrace, "%s", "Entering in makeFilesList()");

	std::list<std::string>::const_iterator srcIterator;
	std::list<FMS_CPF_FileId>::const_iterator dstIterator;

	FMS_CPF_FileId dstSubFileid;
	FMS_CPF_FileId dstFileId(m_ImportInfo.dstFile);

	TRACE(cpf_ImportFileTrace, "makeFilesList(), num=<%zd> of dst subFiles", dstFileList.size());

	if(FMS_CPF_Types::cm_CLEAR == m_ImportInfo.importOption)
	{
		// All srcfiles must be created
		for(srcIterator = srcFileList.begin(); srcIterator != srcFileList.end(); ++srcIterator)
		{
			std::string srcSubFileName = *srcIterator;
			ACS_APGCC::toUpper(srcSubFileName);
			subFilesToCreate.push_back(srcSubFileName);
			TRACE(cpf_ImportFileTrace, "makeFilesList(), subFile=%s must be created", srcSubFileName.c_str() );
		}
	}
	else
	{
		bool fileFound;
		// compare each src subfile name with the dst sub file name
		for(srcIterator = srcFileList.begin(); srcIterator != srcFileList.end(); ++srcIterator)
		{
			// set the src subfile name
			std::string srcSubFileName = *srcIterator;
			ACS_APGCC::toUpper(srcSubFileName);
			fileFound = false;
			// compare the src subfile with eacg dst subfile name
			for(dstIterator = dstFileList.begin(); dstIterator != dstFileList.end(); ++dstIterator)
			{
				dstSubFileid = *dstIterator;
				TRACE(cpf_ImportFileTrace, "makeFilesList(), s1:<%s>, s2:<%s>", srcSubFileName.c_str(), dstSubFileid.subfileAndGeneration().c_str());
				//compare the src subfile name with dst sub file name
				if( srcSubFileName.compare(dstSubFileid.subfileAndGeneration()) == 0 )
				{
					// dst subfile found
					fileFound = true;
					break;
				}
			}

			if(!fileFound)
			{
				// subfile not present in the dst composite file
				// insert in the creation list
				subFilesToCreate.push_back(srcSubFileName);
				TRACE(cpf_ImportFileTrace, "makeFilesList(), subFile=%s must be created", srcSubFileName.c_str() );
			}
			else if(FMS_CPF_Types::cm_NORMAL == m_ImportInfo.importOption )
			{
				// subfile already present in the destination composite file
				// copy mode could not be normal
				subFilesToCreate.clear();
				TRACE(cpf_ImportFileTrace, "%s", "makeFilesList(), dst subfile exist and copy mode is normal");
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILEEXISTS, dstSubFileid.data() );
			}
		}
	}
	TRACE(cpf_ImportFileTrace, "Leaving makeFilesToCreateList(), num=<%zd> of subFiles to create", subFilesToCreate.size());
}

/*============================================================================
	ROUTINE: removeSubFiles
 ============================================================================ */
bool CPF_ImportFile_Request::removeSubFiles()
{
	TRACE(cpf_ImportFileTrace, "%s", "Entering in removeSubFiles()");
	bool result = true;
	int retCode = 0;

	if(FMS_CPF_Types::cm_CLEAR == m_ImportInfo.importOption)
	{
		FMS_CPF_File fileToDelete;
		retCode = fileToDelete.deleteInternalSubFiles(m_ImportInfo.dstFile, m_ImportInfo.dstVolume, m_ImportInfo.cpName );
	}

	if(0!= retCode)
	{
		TRACE(cpf_ImportFileTrace, "%s", "removeSubFiles(), error during subfiles removing");
		result= false;
	}
	TRACE(cpf_ImportFileTrace, "%s", "Leaving removeSubFiles()");
	return result;
}

/*============================================================================
	ROUTINE: checkSourceFolder
 ============================================================================ */
bool CPF_ImportFile_Request::checkSourceFolder(const std::string& srcPath, std::list<std::string>& fileList, bool& isZipped)
throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_ImportFileTrace, "%s", "Entering in checkSourceFolder()");
	bool isSingleFile;
	isZipped = false;
	ACE_stat statInfo;
	std::string srcFileName;

	if(checkZippedSource(srcPath, fileList))
	{
		isZipped = true;
		// A zip file is compared to a folder
		isSingleFile = false;
	}
	else
	{
		int statResult = ACE_OS::stat(srcPath.c_str(), &statInfo);

		if(-1 == statResult)
		{
			TRACE(cpf_ImportFileTrace, "checkSourceFolder(), invalid path=<%s>", srcPath.c_str());
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INVALIDPATH);
		}

		if(S_ISDIR(statInfo.st_mode))
		{
			TRACE(cpf_ImportFileTrace, "%s", "checkSourceFolder(), import of a folder");
			FileSearch directoryPtr(srcPath);

			// Open source directory
			if( !directoryPtr.open() )
			{
				std::string errorDetail(errorText::sourceFail);
				errorDetail.append(m_ImportInfo.relativeSrcPath);
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
			}

			isSingleFile = false;
			// Get all file into the folder
			while(directoryPtr.find(srcFileName) )
			{
				if(srcFileName == ".") continue;
				if(srcFileName == "..") continue;

				std::string dirEntry = srcPath + '/' + srcFileName;

				statResult = ACE_OS::stat(dirEntry.c_str(), &statInfo);

				if (S_ISDIR(statInfo.st_mode))
				{
					// Nested subdirectories not allowed.
					TRACE(cpf_ImportFileTrace, "%s", "checkSourceFolder(), error source folder with nested subfolder");
					directoryPtr.close();
					throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INVALIDPATH);
				}

				// insert the foud file in the list of files to import
				fileList.push_back(srcFileName);
				TRACE(cpf_ImportFileTrace, "checkSourceFolder(), found file=<%s> to import", srcFileName.c_str() );
			}

			// Close source folder
			directoryPtr.close();

			//  check if the directory is empty
			if(fileList.size() == 0U)
			{
				TRACE(cpf_ImportFileTrace, "%s", "checkSourceFolder(), source folder empty");

				std::string errorDetail = m_ImportInfo.relativeSrcPath + errorText::EmptyFolder;

				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
			}
		}
		else
		{
			//Not a directory
			TRACE(cpf_ImportFileTrace, "%s", "checkSourceFolder(), import of a single file");

			std::string::size_type fileNameStart;

			fileNameStart = srcPath.find_last_of(DirDelim);
			if(std::string::npos != fileNameStart)
			{
				srcFileName = srcPath.substr( fileNameStart+1, (srcPath.length() - fileNameStart) );
				fileList.push_back(srcFileName);
			}
			isSingleFile = true;
		}
	}
	TRACE(cpf_ImportFileTrace, "%s", "Leaving checkSourceFolder()");
	return isSingleFile;
}

/*============================================================================
	ROUTINE: checkZippedSource
 ============================================================================ */
bool CPF_ImportFile_Request::checkZippedSource(const std::string& srcPath, std::list<std::string>& fileList)
{
	TRACE(cpf_ImportFileTrace, "%s", "Entering checkZippedSource()");
	bool sourceZipped = false;

	std::string::size_type extPos;
	extPos = srcPath.find_last_of('.');

	if(std::string::npos == extPos)
	{
		// File without any extension the command will try with .zip and .ZIP
		// check if a folder skip it
		size_t slashPos = srcPath.find_last_of(DirDelim);
		if(slashPos != (srcPath.length() - 1))
			sourceZipped = getZippedFileList(srcPath, fileList);
	}
	else
	{
		// Check file extension
		std::string fileExt(srcPath.substr(extPos + 1));
		ACS_APGCC::toUpper(fileExt);
		// Check if a zip extension
		if(fileExt.compare(unzipInfo::extensionZIP) == 0)
		{
			sourceZipped = getZippedFileList(srcPath, fileList);
		}
	}

	TRACE(cpf_ImportFileTrace, "Leaving checkZippedSource(), result:<%s>", (sourceZipped ? "ZIPPED" : "NOT ZIPPED"));
	return sourceZipped;
}

/*============================================================================
	ROUTINE: getZippedFileList
 ============================================================================ */
bool CPF_ImportFile_Request::getZippedFileList(const std::string& zipFile, std::list<std::string>& fileList)
{
	TRACE(cpf_ImportFileTrace, "%s", "Entering getZippedFileList()");
	bool result = false;

	std::string cmdUnzip(unzipInfo::zipCmd);
	cmdUnzip += unzipInfo::listOption;
	cmdUnzip += unzipInfo::optionSpace;
	cmdUnzip += zipFile;

	TRACE(cpf_ImportFileTrace, "getZippedFileList(), run cmd:<%s>", cmdUnzip.c_str());

	FILE* pipe = popen(cmdUnzip.c_str(), "r");
	if(NULL != pipe)
	{
		char buffer[128]={'\0'};

		while(!feof(pipe))
		{
			// get the cmd output
			if(fgets(buffer, 127, pipe) != NULL)
			{
				size_t len = strlen(buffer);
				// remove the newline
				if( buffer[len-1] == '\n' ) buffer[len-1] = 0;

				TRACE(cpf_ImportFileTrace, "getZippedFileList(),found file:<%s>", buffer);
				fileList.push_back(buffer);
			}
		}
		// wait cmd termination
		int exitCode = pclose(pipe);
		// get the exit code from the exit status
		result = (WEXITSTATUS(exitCode) == 0);
	}
	TRACE(cpf_ImportFileTrace, "Leaving getZippedFileList(), result:<%s>", (result ? "TRUE" : "FALSE") );
	return result;
}

/*============================================================================
	ROUTINE: exctractZippedFile
 ============================================================================ */
int CPF_ImportFile_Request::exctractZippedFile(const std::string& zipFile, const std::string& dstFolder)
{
	TRACE(cpf_ImportFileTrace, "%s", "Entering in exctractZippedFile()");
	int result = -1;
	// Assemble zip cmd to run
	std::string cmdUnzip(unzipInfo::zipCmd);
	cmdUnzip += unzipInfo::extractOption;
	cmdUnzip += dstFolder;
	cmdUnzip += unzipInfo::optionSpace;
	cmdUnzip += zipFile;

	TRACE(cpf_ImportFileTrace, "exctractZippedFile(), run cmd:<%s>", cmdUnzip.c_str());
	FILE* pipe = popen(cmdUnzip.c_str(), "r");
	if(NULL != pipe)
	{
		char buffer[128]={'\0'};

		while(!feof(pipe))
		{
			// get the cmd output
			if(fgets(buffer, 127, pipe) != NULL)
			{
				//
				TRACE(cpf_ImportFileTrace, "exctractZippedFile(): <%s>", buffer);
			}
		}
		// wait cmd termination
		int exitCode = pclose(pipe);
		// get the exit code from the exit status
		result = WEXITSTATUS(exitCode);
	}
	TRACE(cpf_ImportFileTrace, "Leaving exctractZippedFile(), result:<%d>", result );
	return result;
}

/*============================================================================
	ROUTINE: checkLowerCaseName
 ============================================================================ */
void CPF_ImportFile_Request::checkLowerCaseName(const std::list<std::string>& fileList, const std::string& dstPath)
{
	TRACE(cpf_ImportFileTrace, "%s", "Entering in checkLowerCaseName()");
	std::list<std::string>::const_iterator fileIterator;
	boost::system::error_code renameResult;

	std::string subFilePath = dstPath + DirDelim;

	// check each file extracted
	for(fileIterator = fileList.begin(); fileIterator != fileList.end(); ++fileIterator)
	{
		std::string subFileName = (*fileIterator);
		ACS_APGCC::toUpper(subFileName);

		std::string newFileName = subFilePath + subFileName;

		// Compare with upper case version
		if(subFileName.compare(*fileIterator) != 0)
		{
			TRACE(cpf_ImportFileTrace, "checkLowerCaseName(), file:<%s> in lower case", (*fileIterator).c_str());
			std::string fileToRename = subFilePath + (*fileIterator);

			// rename to upper case
			boost::filesystem::rename(fileToRename, newFileName, renameResult );

			// check rename operation
			if(SUCCESS != renameResult.value() )
			{
				char logBuffer[512] = {'\0'};
				snprintf(logBuffer, 511,"CPF_ImportFile_Request::checkLowerCaseName(), make upper file name:<%s> failed with error:<%d>", fileToRename.c_str(), renameResult.value() );
				CPF_Log.Write(logBuffer, LOG_LEVEL_WARN);
			}
		}

		// Set permission 744 (RWXR--R--) on file
		// extracted from the zip file
		mode_t mode = S_IRWXU | S_IRGRP | S_IROTH;
		if(SUCCESS != chmod(newFileName.c_str(), mode) )
		{
			char errorText[256] = {0};
			std::string errorDetail(strerror_r(errno, errorText, 255));
			char errorMsg[512] = {0};
			snprintf(errorMsg, 511, "CPF_ImportFile_Request::checkLowerCaseName(), failed to set permission on subfile:<%s> error=<%s>", newFileName.c_str(), errorDetail.c_str() );
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		}
	}

	TRACE(cpf_ImportFileTrace, "%s", "Leaving checkLowerCaseName()");
}

/*============================================================================
	ROUTINE: ~CPF_ImportFile_Request
 ============================================================================ */
CPF_ImportFile_Request::~CPF_ImportFile_Request()
{
	if(NULL != cpf_ImportFileTrace)
			delete cpf_ImportFileTrace;
}
