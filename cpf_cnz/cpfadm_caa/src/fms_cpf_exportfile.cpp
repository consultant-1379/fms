/*
 * * @file fms_cpf_exportfile.cpp
 *	@brief
 *	Class method implementation for CPF_ExportFile_Request.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_exportfile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-09-27
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
 *	| 1.0.0  | 2011-09-27 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_exportfile.h"
#include "fms_cpf_common.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filereference.h"
#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_attribute.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"

#include "ace/ACE.h"

extern ACS_TRA_Logging CPF_Log;

namespace zipInfo{
		std::string zipCmd("/usr/bin/zip");
		std::string zipOption(" -jD ");
		std::string extensionZIP(".zip");
		const char allFolderFile = '*';
		const char optionSpace = ' ';
}

namespace zipErrorCode{
		const int emptySource = 12;
		const int relPathNotExist = 15;
}

/*============================================================================
	ROUTINE: CPF_ExportFile_Request
 ============================================================================ */
CPF_ExportFile_Request::CPF_ExportFile_Request(const exportFileData& exportInfo)
: m_ExportInfo(exportInfo),
  m_ExitCode(0),
  m_ExitMessage(),
  m_ComCliExitMessage(),
  cpf_ExportFileTrace( new (std::nothrow) ACS_TRA_trace("CPF_ExportFile_Request"))
{

}

/*============================================================================
	ROUTINE: executeExport
 ============================================================================ */
bool CPF_ExportFile_Request::executeExport()
{
	TRACE(cpf_ExportFileTrace, "%s", "Entering in executeExport()");
	bool result = false;

	FileReference srcFileReference;
	FMS_CPF_FileId srcFileId(m_ExportInfo.srcFile);

	try{

		srcFileReference = DirectoryStructureMgr::instance()->open(srcFileId, FMS_CPF_Types::R_XW_, m_ExportInfo.cpName.c_str());

		FMS_CPF_Attribute srcFileAttribute = srcFileReference->getAttribute();

		if(srcFileAttribute.type() == FMS_CPF_Types::ft_INFINITE)
		{
			 FMS_CPF_Types::transferMode tqmode;
			 srcFileAttribute.getTQmode(tqmode);

			 if( FMS_CPF_Types::tm_NONE != tqmode )
			 {
				 // Export of Infinite file connected to a TQ not allowed
				 throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILEISPROT, errorText::FileAttachedToTQ);
			 }
		}

		m_ExportInfo.srcPath = srcFileReference->getPath();

		if(srcFileId.subfileAndGeneration().empty()  && srcFileAttribute.composite() )
		{
			// check for zip file
			if(m_ExportInfo.zipData)
			{
				// export a composite file to a zip file
				exportFileToZip(true);
			}
			else
			{
				// export a composite file
				exportCompositeFile();
			}

			TRACE(cpf_ExportFileTrace, "%s", "executeExport(), composite file exported");
			// CP files exported
			result = true;
		}
		else
		{
			// check for zip file
			if(m_ExportInfo.zipData)
			{
				// export a simple file or subfile to a zip file
				exportFileToZip();
			}
			else
			{
				// export a simple file or subfile
				exportSubFile();
			}

			TRACE(cpf_ExportFileTrace, "%s", "executeExport(), subfile/simple file exported");
			// CP files exported
			result = true;

		}

	}
	catch(FMS_CPF_PrivateException& ex)
	{
		m_ExitCode = static_cast<int>(ex.errorCode());
		m_ExitMessage = ex.errorText();
		m_ExitMessage.append(ex.detailInfo());

		m_ComCliExitMessage = m_ExitMessage;

		TRACE(cpf_ExportFileTrace, "executeExport(), exception=<%d, %s> on export", m_ExitCode, m_ExitMessage.c_str() );
	}

	// Close the opened file
	DirectoryStructureMgr::instance()->closeExceptionLess(srcFileReference, m_ExportInfo.cpName.c_str());

	TRACE(cpf_ExportFileTrace, "%s", "Leaving executeExport()");
	return result;
}

/*============================================================================
	ROUTINE: exportFileToZip
 ============================================================================ */
void CPF_ExportFile_Request::exportFileToZip(bool allFiles) throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_ExportFileTrace, "%s", "Entering in exportFileToZip()");

	std::string srcFolder(m_ExportInfo.srcPath);
	std::string fullName(m_ExportInfo.dstPath);

	switch(m_ExportInfo.exportOption)
	{
		case FMS_CPF_Types::cm_NORMAL:
		{
			// Check if the zip file already exist
			if( zipExist(fullName) )
			{
				// error the zip file exist
				std::string errorDetail(m_ExportInfo.relativeDstPath);
				errorDetail.append(errorText::AlredyExist);
				TRACE(cpf_ExportFileTrace, "exportFileToZip(),error:<%s>", errorDetail.c_str());
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail );
			}
		}
		break;

		case FMS_CPF_Types::cm_OVERWRITE:
		{
			TRACE(cpf_ExportFileTrace, "%s", "exportFileToZip(), overwrite the zip");
			// Check if the zip file already exist
			if( zipExist(fullName) )
			{
				// Remove the zip file
				if(SUCCESS != remove(fullName.c_str()) )
				{
					char errorText[64] = {0};
					std::string errorDetail(strerror_r(errno, errorText, 63));
					errorDetail.append(errorText::SpaceOnSpace);
					errorDetail.append(m_ExportInfo.relativeDstPath);
					TRACE(cpf_ExportFileTrace, "exportFileToZip(),error:<%s>", errorDetail.c_str());
					throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail );
				}
			}
		}
		break;

		case FMS_CPF_Types::cm_APPEND:
		{
			// only to check extension
			zipExist(fullName);
			TRACE(cpf_ExportFileTrace, "%s", "exportFileToZip(), append to the zip");
		}
		break;

		default:
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PARAMERROR, errorText::exportModeError);
	}

	if(allFiles)
	{
		srcFolder += DirDelim;
		srcFolder += zipInfo::allFolderFile;
	}

	// Assemble zip cmd to run
	std::string cmdZip(zipInfo::zipCmd);
	cmdZip += zipInfo::zipOption;
	cmdZip += fullName;
	cmdZip += zipInfo::optionSpace;
	cmdZip += srcFolder;

	TRACE(cpf_ExportFileTrace, "exportFileToZip(), run cmd:<%s>", cmdZip.c_str());
	FILE* pipe = popen(cmdZip.c_str(), "r");
	if(NULL != pipe)
	{
		char buffer[128]={'\0'};

		while(!feof(pipe))
		{
			// get the cmd output
			if(fgets(buffer, 127, pipe) != NULL)
			{
				// show cmd output
				TRACE(cpf_ExportFileTrace, "exportFileToZip(): <%s>", buffer);
			}
		}
		// wait cmd termination
		int exitCode = pclose(pipe);
		// get the exit code from the exit status
		int result = WEXITSTATUS(exitCode);
		if(SUCCESS != result)
		{
			char errorText[256] = {0};
			snprintf(errorText, 255, "%s(), zip:<%s> fails, error=<%i>", __func__, m_ExportInfo.dstPath.c_str(), result );
			CPF_Log.Write(errorText, LOG_LEVEL_ERROR);
			TRACE(cpf_ExportFileTrace, "%s", errorText);

			std::string detail;
			if(zipErrorCode::emptySource == result)
			{
				// Empty source folder
				detail.append(errorText::zipFailEmptySource);
			}
			else if(zipErrorCode::relPathNotExist == result )
			{
				// Relative path not exists
				detail.append(errorText::zipFailPathError);
			}
			else
			{
				detail.append(errorText::zipFail);
				detail.append(m_ExportInfo.relativeDstPath);
			}
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, detail);
		}
	}
	TRACE(cpf_ExportFileTrace, "%s", "Leaving exportFileToZip()");
}

/*============================================================================
	ROUTINE: zipExist
 ============================================================================ */
bool CPF_ExportFile_Request::zipExist(std::string& zipFile)
{
	TRACE(cpf_ExportFileTrace, "%s", "Entering in zipExist()");
	bool result= false;
	std::string::size_type extPos;
	extPos = zipFile.find_last_of(parseSymbol::dot);

	// Append the zip extension if not present
	if(std::string::npos == extPos)
	{
		zipFile += zipInfo::extensionZIP;
	}
	else
	{
		// found a dot, check extension part
		std::string fileExtension = zipFile.substr(extPos);
		ACS_APGCC::toLower(fileExtension);

		if(fileExtension.compare(zipInfo::extensionZIP) != 0)
			zipFile += zipInfo::extensionZIP;
	}

	result = (SUCCESS == ACE_OS::access(zipFile.c_str(), F_OK));

	TRACE(cpf_ExportFileTrace, "Leaving zipExist(), file:<%s> %s", zipFile.c_str(), (result ? "EXIST" : "NOT EXIST") );
	return result;
}


/*============================================================================
	ROUTINE: exportCompositeFile
 ============================================================================ */
void CPF_ExportFile_Request::exportCompositeFile() throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_ExportFileTrace, "%s", "Entering in exportCompositeFile()");
	int oflag;
	char errorText[512];
	ACE_stat statInfo;
	std::list<FMS_CPF_FileId> exportFileList;
	std::list<FMS_CPF_FileId>::iterator exportIterator;

	// get the list of subfiles
	DirectoryStructureMgr::instance()->getListFileId(m_ExportInfo.srcFile, exportFileList, m_ExportInfo.cpName.c_str() );

	int statResult = ACE_OS::stat(m_ExportInfo.dstPath.c_str(), &statInfo);

	if( 0 == statResult )
	{
		// check if the dst path is a directory
		if( !(S_ISDIR(statInfo.st_mode)))
		{
			// dst path is not a directory
			std::string errorDetail(m_ExportInfo.relativeDstPath);
			errorDetail.append(errorText::NeedDirectory);
			TRACE(cpf_ExportFileTrace, "exportCompositeFile(), dst path<%s> is not a folder", m_ExportInfo.dstPath.c_str());
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
		}

		TRACE(cpf_ExportFileTrace, "%s","exportCompositeFile(), dst path is a folder");

		switch(m_ExportInfo.exportOption)
		{
			case FMS_CPF_Types::cm_NORMAL:
			{
				// Check if destination file already exists
				checkDstFolder(exportFileList);
				oflag = O_CREAT | O_EXCL;
			}
			break;

			case FMS_CPF_Types::cm_OVERWRITE:
			{
				oflag = O_CREAT | O_TRUNC;
			}
			break;

			case FMS_CPF_Types::cm_APPEND:
			{
				oflag = O_CREAT | O_APPEND;
			}
			break;

			default:
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PARAMERROR, errorText::exportModeError);
		}
	}
	else if(errno == ENOENT)
	{
		TRACE(cpf_ExportFileTrace, "%s", "exportCompositeFile(), dst folder must be created");
		// dst directory not exist
		switch(m_ExportInfo.exportOption)
		{
			case FMS_CPF_Types::cm_NORMAL:
			case FMS_CPF_Types::cm_OVERWRITE:
			case FMS_CPF_Types::cm_APPEND:
			{
				oflag = O_CREAT;
			}
			break;

			default:
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PARAMERROR, errorText::exportModeError);
		}

		// Set permission 775 (RWXRWXR-X) on folder
		mode_t folderPermission = S_IRWXU |S_IRWXG | S_IROTH | S_IXOTH;
		ACE_OS::umask(0);

		if( ACE_OS::mkdir(m_ExportInfo.dstPath.c_str(), folderPermission) == -1)
		{
			std::string errorDetail(strerror_r(errno, errorText, 511));

			TRACE(cpf_ExportFileTrace, "exportCompositeFile(), error on mkdir=<%s>", errorDetail.c_str());
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
		}
		TRACE(cpf_ExportFileTrace, "%s", "exportCompositeFile(), dst folder has been created");
	}
	else
	{
		std::string errorDetail(strerror_r(errno, errorText, 511));
		TRACE(cpf_ExportFileTrace, "exportCompositeFile(), error on access to <%s>", errorDetail.c_str());

		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
	}

	TRACE(cpf_ExportFileTrace, "exportCompositeFile(), starting copy of <%zd> files", exportFileList.size());

	for(exportIterator = exportFileList.begin(); exportIterator != exportFileList.end(); ++exportIterator)
	{
		std::string subFilePath = m_ExportInfo.srcPath + DirDelim + (*exportIterator).subfileAndGeneration();
		std::string filePath = m_ExportInfo.dstPath + DirDelim + (*exportIterator).subfileAndGeneration();
		copyFile(subFilePath, filePath, oflag);
	}

	TRACE(cpf_ExportFileTrace, "%s", "Leaving exportCompositeFile()");
}

/*============================================================================
	ROUTINE: exportSubFile
 ============================================================================ */
void CPF_ExportFile_Request::exportSubFile() throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_ExportFileTrace, "%s", "Entering in exportSubFile()");
	int oflag;
	char errorText[512] = {0};
	ACE_stat statInfo;
	std::string filePath = m_ExportInfo.dstPath;
	std::string folderPath;
	std::string::size_type startFile;

	int statResult = ACE_OS::stat(filePath.c_str(), &statInfo);

	// check if dst path is a directory
	if( 0 == statResult && S_ISDIR(statInfo.st_mode) )
	{
		TRACE(cpf_ExportFileTrace, "%s", "exportSubFile(), not allowed a folder as destination");
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INVALIDPATH);
	}

	startFile = m_ExportInfo.dstPath.find_last_of(DirDelim);

	if(std::string::npos != startFile)
		folderPath = filePath.substr(0, startFile);
	else
		folderPath = filePath;

	statResult = ACE_OS::stat(folderPath.c_str(), &statInfo);

	if( 0 != statResult)
	{
		std::string errorDetail(strerror_r(errno, errorText, 511));
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
	}
	switch(m_ExportInfo.exportOption)
	{
		case FMS_CPF_Types::cm_NORMAL:
		{
			int accessResult = ACE_OS::access(filePath.c_str(), F_OK);
			// verify the dst file
			if( 0 == accessResult)
			{
				// dst file exist
				std::string erroMsg(m_ExportInfo.relativeDstPath);
				erroMsg.append(errorText::AlredyExist);

				TRACE(cpf_ExportFileTrace, "exportSubFile(), file<%s> already exist", filePath.c_str());
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, erroMsg );
			}

			oflag = O_CREAT | O_EXCL;
		}
		break;

		case FMS_CPF_Types::cm_OVERWRITE:
		{
			oflag = O_CREAT | O_TRUNC;
		}
		break;

		case FMS_CPF_Types::cm_APPEND:
		{
			oflag = O_CREAT | O_APPEND;
		}
		break;

		default:
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PARAMERROR, errorText::exportModeError);
	}

	copyFile(m_ExportInfo.srcPath, filePath, oflag);

	TRACE(cpf_ExportFileTrace, "%s", "Leaving exportSubFile()");
}
/* HW79785 BEGIN
 * 
 */
/*============================================================================
	ROUTINE: copyfile
 ============================================================================ */
void CPF_ExportFile_Request::copyFile(const std::string& srcFilePath, const std::string& dstFilePath, int oflag)
throw (FMS_CPF_PrivateException)
{
	const int BUFFSIZE = 4096;
	char buf[BUFFSIZE];
	char errorText[512] = {0};
	ACE_HANDLE srcFileHandle;
	ACE_HANDLE dstFileHandle;
	ssize_t numOfByte;
	unsigned int count = 0;
	bool b_isEnvGEP5EGEMandMaxSize = false;
	TRACE(cpf_ExportFileTrace, "%s", "Entering in copyFile()");

	snprintf(errorText, 511, "%s() begin", __func__ );
	CPF_Log.Write(errorText, LOG_LEVEL_TRACE);

	if((srcFileHandle = ACE_OS::open(srcFilePath.c_str(), O_RDONLY | O_BINARY)) == -1)
	{
		TRACE(cpf_ExportFileTrace, "copyFile(), error to open src file=<%s>", srcFilePath.c_str());
		std::string errorDetail(strerror_r(errno, errorText, 511));
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
	}

	TRACE(cpf_ExportFileTrace, "copyFile(), src file=<%s> opened", srcFilePath.c_str());
	snprintf(errorText, 511, "copyFile(), src file=<%s> opened",srcFilePath.c_str());
	CPF_Log.Write(errorText, LOG_LEVEL_TRACE);

	// Set permission 744 (RWXR--R--) on destination file
	mode_t filePermission = S_IRWXU | S_IRGRP | S_IROTH;
	ACE_OS::umask(0);

	if ((dstFileHandle = ACE_OS::open(dstFilePath.c_str(), O_WRONLY | O_BINARY | oflag, filePermission)) == -1)
	{
		TRACE(cpf_ExportFileTrace, "copyFile(), error to open dst file=<%s>", dstFilePath.c_str());
		std::string errorDetail(strerror_r(errno, errorText, 511));
		ACE_OS::close(srcFileHandle);
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
	}

	TRACE(cpf_ExportFileTrace, "copyFile(), dst file=<%s> opened", dstFilePath.c_str());
	snprintf(errorText, 511,"copyFile(), dst file=<%s> opened", dstFilePath.c_str());
	CPF_Log.Write(errorText, LOG_LEVEL_TRACE);

	if ((checkFileSize (srcFilePath)))  // check if source file >= 100MB
	{
		if ((checkNodeArch()) && (checkHWVersionInfo())) // check if node arch is EGEM2 and hw veriosn is GEP5
		{
			b_isEnvGEP5EGEMandMaxSize = true;
			TRACE(cpf_ExportFileTrace, " src file size >= 100mb :: <%d>", b_isEnvGEP5EGEMandMaxSize);
			snprintf(errorText, 511, "CPF_ExportFile_Request::copyFile, src file size >= 100mb :: <%d> :: <%s>", b_isEnvGEP5EGEMandMaxSize,srcFilePath.c_str());
			CPF_Log.Write(errorText, LOG_LEVEL_WARN);
		}
	}

	do{
		numOfByte = ACE_OS::read(srcFileHandle, &buf, BUFFSIZE);

		if(numOfByte > 0)
		{
			if(ACE_OS::write(dstFileHandle, &buf, numOfByte) == -1)
			{
				TRACE(cpf_ExportFileTrace, "copyFile(), error to write dst file=<%s>", dstFilePath.c_str());
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
					TRACE(cpf_ExportFileTrace, "copyFile(), count reached=<%d> , sleep for 1000 micro seconds", count);
					count = 0; 
					// reset the counter to zero
					usleep(1000);
				}
			}
		}
		else if(numOfByte < 0)
		{
			TRACE(cpf_ExportFileTrace, "copyFile(), error to read src file=<%s>", srcFilePath.c_str());
			std::string errorDetail(strerror_r(errno, errorText, 511));
			ACE_OS::close(srcFileHandle);
			ACE_OS::close(dstFileHandle);
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail );
		}
	}while(numOfByte > 0);

	ACE_OS::close(srcFileHandle);
	ACE_OS::close(dstFileHandle);

	TRACE(cpf_ExportFileTrace, "%s", "Leaving copyFile()");
	snprintf(errorText, 511, "%s() Leaving", __func__ );
	CPF_Log.Write(errorText, LOG_LEVEL_TRACE);
}
/* HW79785 BEGIN
 * 
 */
/*===========================================================================
	ROUTINE: checkDstFolder
 ============================================================================ */
void CPF_ExportFile_Request::checkDstFolder(const std::list<FMS_CPF_FileId>& exportFileList) throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_ExportFileTrace, "%s", "Entering in checkDstFolder()");
	std::list<FMS_CPF_FileId>::const_iterator exportIterator;
	char errorBuffer[512];
	int accessResult;

	for(exportIterator = exportFileList.begin(); exportIterator != exportFileList.end(); ++exportIterator)
	{
		// assemble the dst file path
		std::string filePath = m_ExportInfo.dstPath + DirDelim + (*exportIterator).subfileAndGeneration();

		accessResult = ACE_OS::access(filePath.c_str(), F_OK);
		// verify the dst file
		if( 0 == accessResult)
		{
			// dst file exist
			std::string errorDetail = m_ExportInfo.relativeDstPath + DirDelim + (*exportIterator).subfileAndGeneration();
			errorDetail.append(errorText::AlredyExist);
			TRACE(cpf_ExportFileTrace, "checkDstFolder(), file<%s>", errorDetail.c_str());
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail );
		}
		else if(errno != ENOENT)
		{
			// file not exist but an error is occurred
			std::string errorDetail(strerror_r(errno, errorBuffer, 511));
			TRACE(cpf_ExportFileTrace, "checkDstFolder(), error on file=<%s>", errorDetail.c_str());
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail );
		}
	}

	TRACE(cpf_ExportFileTrace, "%s", "Leaving checkDstFolder()");
}

/*============================================================================
	ROUTINE: ~CPF_ExportFile_Request
 ============================================================================ */
CPF_ExportFile_Request::~CPF_ExportFile_Request()
{
	if(NULL != cpf_ExportFileTrace)
		delete cpf_ExportFileTrace;
}
