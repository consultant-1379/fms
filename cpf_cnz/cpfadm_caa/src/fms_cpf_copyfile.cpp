/*
 * * @file fms_cpf_copyfile.cpp
 *	@brief
 *	Class method implementation for CPF_CopyFile_Request.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_copyfile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-09-12
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
 *	| 1.0.0  | 2011-09-12 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_copyfile.h"
#include "fms_cpf_common.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filedescriptor.h"

#include "fms_cpf_file.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include <ace/ACE.h>


extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	ROUTINE: CPF_CopyFile_Request
 ============================================================================ */
CPF_CopyFile_Request::CPF_CopyFile_Request(const copyFileData& copyInfo)
: m_CopyInfo(copyInfo),
  m_ExitCode(0),
  m_ExitMessage(),
  m_ComCliExitMessage(),
  cpf_CopyFileTrace(new (std::nothrow) ACS_TRA_trace("CPF_CopyFile_Request"))
{

}

/*============================================================================
	ROUTINE: call
 ============================================================================ */
bool CPF_CopyFile_Request::executeCopy()
{
	TRACE(cpf_CopyFileTrace, "%s", "Entering in executeCopy()");
	bool result = false;
	FMS_CPF_FileId srcFileId(m_CopyInfo.srcFile);
	FileReference srcFileReference;

	FMS_CPF_FileId dstFileId(m_CopyInfo.dstFile);

	//Check the dst CP file name
	if( !dstFileId.isValid() )
	{
		FMS_CPF_PrivateException errorMsg(FMS_CPF_Exception::INVALIDFILE);
		m_ExitCode = static_cast<int>(errorMsg.errorCode());
		m_ExitMessage = errorMsg.errorText();
		m_ExitMessage.append(errorMsg.detailInfo());

		m_ComCliExitMessage = m_ExitMessage;

		TRACE(cpf_CopyFileTrace, "%s", "Leaving executeCopy(), invalid dstFile name");
		return result;
	}

	try
	{
		// Open logical src file
		srcFileReference = DirectoryStructureMgr::instance()->open(srcFileId, FMS_CPF_Types::R_XW_, m_CopyInfo.cpName.c_str());
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		m_ExitCode = static_cast<int>(ex.errorCode());
		m_ExitMessage = ex.errorText();
		m_ExitMessage.append(ex.detailInfo());

		m_ComCliExitMessage = m_ExitMessage;

		char errorMsg[256];
		snprintf(errorMsg, 255, "%s(), open file:<%s> failed, error:<%s>", __func__, m_CopyInfo.srcFile.c_str(), m_ExitMessage.c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(cpf_CopyFileTrace, "%s", errorMsg);

		TRACE(cpf_CopyFileTrace, "%s", "Leaving executeCopy()");
		return result;
	}

	if( !checkFileProtection(srcFileReference) )
	{
		FMS_CPF_PrivateException errorMsg(FMS_CPF_PrivateException::FILEISPROT, errorText::FileAttachedToTQ);
		m_ExitCode = static_cast<int>(errorMsg.errorCode());
		m_ExitMessage = errorMsg.errorText();
		m_ExitMessage.append(errorMsg.detailInfo());

		m_ComCliExitMessage = m_ExitMessage;
	}
	else
	{
		// Check if the destination file is connected to a TQ
		FMS_CPF_Attribute dstFileAttribute;
		std::string errorDetail;
		FMS_CPF_PrivateException::errorType errCode = (FMS_CPF_PrivateException::errorType) checkDestinationFile(dstFileAttribute, errorDetail);

		if(FMS_CPF_PrivateException::OK != errCode)
		{
			FMS_CPF_PrivateException errorMsg(errCode, errorDetail);
			m_ExitCode = static_cast<int>(errorMsg.errorCode());
			m_ExitMessage = errorMsg.errorText();
			m_ExitMessage.append(errorMsg.detailInfo());

			m_ComCliExitMessage = m_ExitMessage;
		}
		else
		{
			TRACE(cpf_CopyFileTrace, "%s", "executeCopy(), cp files checked");
			FMS_CPF_Attribute srcFileAttribute = srcFileReference->getAttribute();
			// Check the type of file to copy
			if( srcFileId.subfileAndGeneration().empty() && srcFileAttribute.composite() )
			{
				TRACE(cpf_CopyFileTrace, "%s", "executeCopy(), composite file copy");
				try{
					// Composite main file
					if (!(dstFileId.subfileAndGeneration().empty()) || !(dstFileAttribute.composite()) )
					{
						TRACE(cpf_CopyFileTrace, "%s", "executeCopy(), destination file not a composite file");
						throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::NOTCOMPOSITE, dstFileId.data() );
					}

					// Copy CP files
					copyCompositeFile(srcFileReference->getPath());

					TRACE(cpf_CopyFileTrace, "%s", "executeCopy(), composite file copied");
					// CP files Copied
					result = true;
				}
				catch(FMS_CPF_PrivateException& ex)
				{
					m_ExitCode = static_cast<int>(ex.errorCode());
					m_ExitMessage = ex.errorText();
					m_ExitMessage.append(ex.detailInfo());

					m_ComCliExitMessage = m_ExitMessage;
					TRACE(cpf_CopyFileTrace, "%s(), exception<%s> on copy of a Composite File", __func__, m_ExitMessage.c_str());
				}
			}
			else
			{
				TRACE(cpf_CopyFileTrace, "%s", "executeCopy(), copy a subfile or simple file");
				bool file_exists = true;
				try
				{
					// Simple file or subfile
					if(dstFileId.subfileAndGeneration().empty())
					{
						if(dstFileAttribute.composite())
						{
							TRACE(cpf_CopyFileTrace, "%s", "executeCopy(), destination file is a composite file");
							throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::NOTSIMPLE, dstFileId.data() );
						}
						TRACE(cpf_CopyFileTrace, "%s", "executeCopy(), copy to a simple file");
					}
					else
					{
						TRACE(cpf_CopyFileTrace, "%s", "executeCopy(), copy to a subfile");
						file_exists = DirectoryStructureMgr::instance()->exists(dstFileId, m_CopyInfo.cpName.c_str());
						if(file_exists)
						{
							// check for subfile to subfile copy
							if(FMS_CPF_Types::cm_NORMAL == m_CopyInfo.copyOption )
							{
								TRACE(cpf_CopyFileTrace, "%s", "executeCopy(), subfile exists and copy mode normal");
								throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::FILEEXISTS, dstFileId.data());
							}
						}
						else
						{
							// dst subfile not exists
							std::list<std::string> subFileToCreate;
							subFileToCreate.push_back(dstFileId.subfileAndGeneration());

							// create dst subfile
							if( !createSubFiles(subFileToCreate) )
							{
								TRACE(cpf_CopyFileTrace, "%s", "executeCopy(), error on dst subfile creation");
								throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
							}
						}
					}

					// check for simple file copy
					if((FMS_CPF_Types::cm_NORMAL == m_CopyInfo.copyOption ) && file_exists)
					{
						TRACE(cpf_CopyFileTrace, "%s", "executeCopy(), simple file exists and copy mode normal");
						throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILEEXISTS, dstFileId.data());
					}

					// Copy subFiles
					copySubFile(srcFileReference->getPath());

					TRACE(cpf_CopyFileTrace, "%s", "executeCopy(), subfile copied");
					// Subfiles Copied
					result = true;

				}
				catch(FMS_CPF_PrivateException& ex)
				{
					m_ExitCode = static_cast<int>(ex.errorCode());
					m_ExitMessage = ex.errorText();
					m_ExitMessage.append(ex.detailInfo());

					m_ComCliExitMessage = m_ExitMessage;

					TRACE(cpf_CopyFileTrace, "%s(), exception:<%s> on subFiles copy", m_ExitMessage.c_str());
				}
			}
		}
	}

	// Close src file
	DirectoryStructureMgr::instance()->closeExceptionLess(srcFileReference, m_CopyInfo.cpName.c_str());

	TRACE(cpf_CopyFileTrace, "%s", "Leaving executeCopy()");
	return result;
}

/*============================================================================
	ROUTINE: copySubFile
 ============================================================================ */
void CPF_CopyFile_Request::copySubFile(const std::string& srcFilePath) throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_CopyFileTrace, "%s", "Entering in copySubFile()");

	FMS_CPF_FileId dstFileId(m_CopyInfo.dstFile);

	FileReference dstFileReference;
	// Open destination file
	dstFileReference = DirectoryStructureMgr::instance()->open(dstFileId, FMS_CPF_Types::XR_XW_, m_CopyInfo.cpName.c_str());
	TRACE(cpf_CopyFileTrace, "copySubFile(), dst file=<%s> opened", dstFileId.data() );
	try
	{
		std::string dstFilePath = dstFileReference->getPath();
		std::fstream::openmode oflag;
		switch(m_CopyInfo.copyOption)
		{
			case FMS_CPF_Types::cm_NORMAL:
			case FMS_CPF_Types::cm_CLEAR:
			case FMS_CPF_Types::cm_OVERWRITE:
				{
					oflag = fstream::binary | fstream::trunc;
				}
				break;
			case FMS_CPF_Types::cm_APPEND:
				{
					oflag = fstream::binary | fstream::app;
				}
				break;
			default:
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
		}

		copyFile(srcFilePath, dstFilePath, oflag);
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		TRACE(cpf_CopyFileTrace, "copySubFile(), exception=<%s> on copy", ex.errorText() );
		DirectoryStructureMgr::instance()->closeExceptionLess(dstFileReference, m_CopyInfo.cpName.c_str());
		// re-throw the same caught exception
		throw;
	}

	DirectoryStructureMgr::instance()->closeExceptionLess(dstFileReference, m_CopyInfo.cpName.c_str());

	TRACE(cpf_CopyFileTrace, "%s", "Leaving copySubFile()");
}

/*============================================================================
	ROUTINE: copyCompositeFile
 ============================================================================ */
void CPF_CopyFile_Request::copyCompositeFile(const std::string& srcFilePath)
throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_CopyFileTrace, "%s", "Entering in copyCompositeFile()");

	FMS_CPF_FileId srcFileId(m_CopyInfo.srcFile);
	FMS_CPF_FileId dstFileId(m_CopyInfo.dstFile);

	FileReference dstFileReference;
	// Open destination file
	dstFileReference = DirectoryStructureMgr::instance()->open(dstFileId, FMS_CPF_Types::XR_XW_, m_CopyInfo.cpName.c_str());

	std::list<std::string> subFilesToCreate;
	std::list<FMS_CPF_FileId> copyFileList;
	std::fstream::openmode oflag;

	try
	{
		DirectoryStructureMgr::instance()->getListFileId(srcFileId, copyFileList, m_CopyInfo.cpName.c_str());

		makeFilesList(copyFileList, subFilesToCreate);
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		DirectoryStructureMgr::instance()->closeExceptionLess(dstFileReference, m_CopyInfo.cpName.c_str());
		// re-throw the same caught exception
		throw;
	}

	switch(m_CopyInfo.copyOption)
	{
		case FMS_CPF_Types::cm_NORMAL:
		case FMS_CPF_Types::cm_CLEAR:
		case FMS_CPF_Types::cm_OVERWRITE:
			{
				// set the open flag of physical file
				oflag = fstream::binary | fstream::trunc;
			}
			break;

		case FMS_CPF_Types::cm_APPEND:
			{
				oflag = fstream::binary | fstream::app;;
			}
			break;

		default:
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
	}

	m_CopyInfo.dstVolume = dstFileReference->getVolume();
	std::string dstFilePath = dstFileReference->getPath();

	DirectoryStructureMgr::instance()->close(dstFileReference, m_CopyInfo.cpName.c_str());
	bool dstFileOpened = false;
	if(!removeSubFiles())
	{
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
	}
	try
	{

		FMS_CPF_FileId srcSubFileid;
		FMS_CPF_FileId dstSubFileid;

		std::list<FMS_CPF_FileId>::iterator copyIterator;
		std::list<std::string>::iterator dstIterator;

		if(createSubFiles(subFilesToCreate))
		{
			dstFileReference = DirectoryStructureMgr::instance()->open(dstFileId, FMS_CPF_Types::XR_XW_, m_CopyInfo.cpName.c_str());
			dstFileOpened = true;
			// compare each src subfile name with the dst sub file name
			for(copyIterator = copyFileList.begin(); copyIterator != copyFileList.end(); ++copyIterator)
			{
				srcSubFileid = *copyIterator;
				// set the src subfile name
				std::string srcSubFileName = srcSubFileid.subfileAndGeneration();

				std::string srcSubFilePath = srcFilePath + "/" + srcSubFileName;
				std::string dstSubFilePath = dstFilePath + "/" + srcSubFileName;
				copyFile(srcSubFilePath, dstSubFilePath, oflag);
			}
			dstFileOpened = false;
			DirectoryStructureMgr::instance()->close(dstFileReference, m_CopyInfo.cpName.c_str());
		}
		else
		{
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
		}
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		TRACE(cpf_CopyFileTrace, "copyCompositeFile(), exception=<%s>", ex.errorText());

		if( dstFileOpened)
			DirectoryStructureMgr::instance()->closeExceptionLess(dstFileReference, m_CopyInfo.cpName.c_str());

		// re-throw the same caught exception
		throw;
	}

	TRACE(cpf_CopyFileTrace, "%s", "Leaving copyCompositeFile()");
}

/*============================================================================
	ROUTINE: copyfile
 ============================================================================ */
void CPF_CopyFile_Request::copyFile(const std::string& srcFilePath, const std::string& dstFilePath, fstream::openmode oflag)
throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_CopyFileTrace, "%s", "Entering in copyfile()");
	char errorText[512] = {'\0'};

	std::ifstream srcFile(srcFilePath.c_str(), fstream::binary);

	if(srcFile.fail())
	{
		TRACE(cpf_CopyFileTrace, "copyfile() on copy, error to open src file=<%s>", srcFilePath.c_str());
	    std::string errorDetail(strerror_r(errno, errorText, 511));
	    throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
	}

	std::ofstream dstFile(dstFilePath.c_str(), oflag);

	if(dstFile.fail())
	{
		TRACE(cpf_CopyFileTrace, "copyfile() on copy, error to open dst file=<%s>", dstFilePath.c_str());
		std::string errorDetail(strerror_r(errno, errorText, 511));
		srcFile.close();
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

		if( dstFile.bad() || dstFile.fail() )
		{
			TRACE(cpf_CopyFileTrace, "copyfile() on copy <%s> error", srcFilePath.c_str());

			std::string errorDetail(strerror_r(errno, errorText, 511));
			srcFile.close();
			dstFile.close();
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
		}
	}

	srcFile.close();
	dstFile.close();

	TRACE(cpf_CopyFileTrace, "%s", "Leaving copyfile()");
}

/*============================================================================
	ROUTINE: removeSubFiles
 ============================================================================ */
bool CPF_CopyFile_Request::removeSubFiles()
{
	TRACE(cpf_CopyFileTrace, "%s", "Entering in removeSubFiles()");
	bool result = true;
	int retCode = 0;

	if(FMS_CPF_Types::cm_CLEAR == m_CopyInfo.copyOption)
	{
		FMS_CPF_File fileToDelete;
		retCode = fileToDelete.deleteInternalSubFiles(m_CopyInfo.dstFile, m_CopyInfo.dstVolume, m_CopyInfo.cpName );
	}

	if(0!= retCode)
	{
		TRACE(cpf_CopyFileTrace, "%s", "removeSubFiles(), error during subfiles removing");
		result= false;
	}
	TRACE(cpf_CopyFileTrace, "%s", "Leaving removeSubFiles()");
	return result;
}

/*============================================================================
	ROUTINE: createSubFiles
 ============================================================================ */
bool CPF_CopyFile_Request::createSubFiles(const std::list<std::string>& subFileNameList)
{
	TRACE(cpf_CopyFileTrace, "%s", "Entering in createSubFiles()");
	bool result = true;

	if( !subFileNameList.empty() )
	{
		int retCode = 0;
		FMS_CPF_File fileToCreate;
		retCode = fileToCreate.createInternalSubFiles(m_CopyInfo.dstFile, subFileNameList, m_CopyInfo.dstVolume, m_CopyInfo.cpName );

		if(0 != retCode)
		{
			TRACE(cpf_CopyFileTrace, "%s", "createSubFiles(), error during subfiles creation");
			result= false;
		}
	}
	TRACE(cpf_CopyFileTrace, "%s", "Leaving createSubFiles()");
	return result;

}

/*============================================================================
	ROUTINE: makeFilesList
 ============================================================================ */
void  CPF_CopyFile_Request::makeFilesList(std::list<FMS_CPF_FileId>& srcFileList, std::list<std::string>& subFilesToCreate)
throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_CopyFileTrace, "%s", "Entering in makeFilesList()");

	std::list<FMS_CPF_FileId>::iterator srcIterator;
	std::list<FMS_CPF_FileId>::iterator dstIterator;
	std::list<FMS_CPF_FileId> dstFileList;
	FMS_CPF_FileId srcSubFileid;
	FMS_CPF_FileId dstSubFileid;

	FMS_CPF_FileId dstFileId(m_CopyInfo.dstFile);

	DirectoryStructureMgr::instance()->getListFileId(dstFileId, dstFileList, m_CopyInfo.cpName.c_str());
	TRACE(cpf_CopyFileTrace, "makeFilesList(), num=<%zd>  of dst subFiles", dstFileList.size());

	if(FMS_CPF_Types::cm_CLEAR == m_CopyInfo.copyOption)
	{
		for(srcIterator = srcFileList.begin(); srcIterator != srcFileList.end(); ++srcIterator)
		{
			srcSubFileid = *srcIterator;
			std::string subFileName = srcSubFileid.subfileAndGeneration();
			// Insert srcSubfile in the list of subfiles to create
			subFilesToCreate.push_back(subFileName);
			TRACE(cpf_CopyFileTrace, "makeFilesList(), subFile=%s must be created", subFileName.c_str() );
		}
	}
	else
	{
		bool fileFound;
		// compare each src subfile name with the dst sub file name
		for(srcIterator = srcFileList.begin(); srcIterator != srcFileList.end(); ++srcIterator)
		{
			srcSubFileid = *srcIterator;
			// set the src subfile name
			std::string srcSubFileName = srcSubFileid.subfileAndGeneration();
			fileFound = false;
			// compare the src subfile with eacg dst subfile name
			for(dstIterator = dstFileList.begin(); dstIterator != dstFileList.end(); ++dstIterator)
			{
				dstSubFileid = *dstIterator;
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
				TRACE(cpf_CopyFileTrace, "makeFilesList(), subFile=%s must be created", srcSubFileName.c_str() );
			}
			else if(FMS_CPF_Types::cm_NORMAL == m_CopyInfo.copyOption )
			{
				// subfile already present in the destination composite file
				// copy mode could not be normal
				subFilesToCreate.clear();
				TRACE(cpf_CopyFileTrace, "%s", "makeFilesList(), dst subfile exist and copymode is normal");
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILEEXISTS, dstSubFileid.data() );
			}
		}
	}
	TRACE(cpf_CopyFileTrace, "makeFilesList(), num=<%zd> of subFiles to create", subFilesToCreate.size());
	TRACE(cpf_CopyFileTrace, "%s", "Leaving makeFilesList()");
}

/*============================================================================
	ROUTINE: checkDestinationFile
 ============================================================================ */
int CPF_CopyFile_Request::checkDestinationFile(FMS_CPF_Attribute& fileAttribute, std::string& error)
{
	TRACE(cpf_CopyFileTrace, "%s", "Entering in checkDestinationFile()");
	bool fileExist = false;
	FMS_CPF_PrivateException::errorType errCode = FMS_CPF_PrivateException::OK;
	FMS_CPF_FileId fileId(m_CopyInfo.dstFile);

	try
	{
		// Checks if the file exists
		fileExist = DirectoryStructureMgr::instance()->exists(fileId, m_CopyInfo.cpName.c_str() );
	}
	catch (FMS_CPF_PrivateException& ex)
	{
		TRACE(cpf_CopyFileTrace, "%s", "checkDestinationFile(), exception on exists");
		error = ex.detailInfo();
		errCode = ex.errorCode();
		TRACE(cpf_CopyFileTrace, "%s", "Leaving checkDestinationFile()");
		return (int)errCode;
	}

	// check the file type if it exists
	if(fileExist)
    {
		bool fileOpened = false;
		FileReference fileRef;
		try
    	{
			TRACE(cpf_CopyFileTrace, "checkDestinationFile(), open file=%s", fileId.file().c_str() );
    		// Open destination main file
			fileRef =DirectoryStructureMgr::instance()->open(fileId.file(), FMS_CPF_Types::NONE_, m_CopyInfo.cpName.c_str());

    		fileOpened = true;
    		// get the file attribute
			fileAttribute = fileRef->getAttribute();
			// Checks the file type
			if( FMS_CPF_Types::ft_INFINITE == fileAttribute.type() )
			{
				TRACE(cpf_CopyFileTrace, "%s", "checkDestinationFile(), destination file infinite is not allowed");
				// infinite file not accepted as destination file
				errCode = FMS_CPF_PrivateException::TYPEERROR;
			}
    	}
        catch (FMS_CPF_PrivateException& ex)
        {
        	TRACE(cpf_CopyFileTrace, "%s", "checkDestinationFile(), exception on open");
        	error = ex.detailInfo();
        	errCode = ex.errorCode();
        }
        // Close the file is open
        if(fileOpened)
        {
        	try
        	{
        		DirectoryStructureMgr::instance()->close(fileRef, m_CopyInfo.cpName.c_str() );
        	}
        	catch (FMS_CPF_PrivateException&ex)
        	{
        		TRACE(cpf_CopyFileTrace, "%s", "checkDestinationFile(), exception on close");
        	}
        }
    }
	else
	{
		error = fileId.data();
	    errCode = FMS_CPF_PrivateException::FILENOTFOUND;
		TRACE(cpf_CopyFileTrace, "%s", "checkDestinationFile(), file not exists");
	}

	TRACE(cpf_CopyFileTrace, "%s", "Leaving checkDestinationFile()");
	return (int)errCode;
}

/*============================================================================
	ROUTINE: checkFileProtection
 ============================================================================ */
bool CPF_CopyFile_Request::checkFileProtection(FileReference srcFileReference)
{
	TRACE(cpf_CopyFileTrace, "%s", "Entering in checkFileProtection()");
    bool result = true;
	FMS_CPF_Attribute fileAttribute = srcFileReference->getAttribute();

    if(fileAttribute.type() == FMS_CPF_Types::ft_INFINITE)
	{
    	// Check if connected to a TQ
        FMS_CPF_Types::transferMode tqmode;

 	   	fileAttribute.getTQmode(tqmode);

		if(tqmode != FMS_CPF_Types::tm_NONE)
		{
			result = false;
			TRACE(cpf_CopyFileTrace, "%s", "checkFileProtection(), File is protected. File is defined to GOH");
			return result;
		}

	}
    TRACE(cpf_CopyFileTrace, "%s", "Leaving checkFileProtection()");
    return result;
}

/*============================================================================
	ROUTINE: ~CPF_CopyFile_Request
 ============================================================================ */

CPF_CopyFile_Request::~CPF_CopyFile_Request()
{
	if(NULL != cpf_CopyFileTrace)
			delete cpf_CopyFileTrace;
}
