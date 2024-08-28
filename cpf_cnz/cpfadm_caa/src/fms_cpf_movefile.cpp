/*
 * * @file fms_cpf_movefile.cpp
 *	@brief
 *	Class method implementation for CPF_MoveFile_Request.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_movefile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-01-23
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
 *	| 1.0.0  | 2012-01-23 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 	| 1.1.0  | 2014-04-29 | qvincon      | Infinite subfiles removed from IMM  |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_movefile.h"
#include "fms_cpf_common.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filereference.h"
#include "fms_cpf_filedescriptor.h"

#include "fms_cpf_file.h"
#include "fms_cpf_types.h"
#include "fms_cpf_fileid.h"
#include "fms_cpf_attribute.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include <list>
#include <fstream>

#include <ace/Condition_T.h>

#include "fms_cpf_oi_infinitefile.h"

extern ACE_Thread_Mutex mutex_lock;
extern ACE_Condition<ACE_Thread_Mutex> cond_var;

extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	ROUTINE: CPF_MoveFile_Request
 ============================================================================ */
CPF_MoveFile_Request::CPF_MoveFile_Request(const moveFileData& moveInfo)
: m_MoveInfo(moveInfo),
  m_ExitCode(0),
  m_ExitMessage(),
  m_ComCliExitMessage(),
  cpf_MoveFileTrace( new (std::nothrow) ACS_TRA_trace("CPF_MoveFile_Request"))
{

}

/*============================================================================
	ROUTINE: executeMove
 ============================================================================ */
bool CPF_MoveFile_Request::executeMove()
{
	TRACE(cpf_MoveFileTrace, "%s", "Entering in executeMove()");
	bool result = false;

	FMS_CPF_FileId srcFileId(m_MoveInfo.srcFile);
	FileReference srcFileReference;

	//Check the destination volume, it must already exist
	if(!volumeExist() )
	{
		FMS_CPF_PrivateException errorMsg(FMS_CPF_PrivateException::VOLUMENOTFOUND, m_MoveInfo.dstVolume);
		m_ExitCode = static_cast<int>(errorMsg.errorCode());
		m_ExitMessage = errorMsg.errorText();
		m_ExitMessage.append(errorMsg.detailInfo());

		m_ComCliExitMessage = m_ExitMessage;
		TRACE(cpf_MoveFileTrace, "%s", "Leaving executeMove(), invalid volume name");
		return result;
	}

	//Check the file type, subfile cannot be moved
	if(!srcFileId.subfileAndGeneration().empty() )
	{
		FMS_CPF_PrivateException errorMsg(FMS_CPF_PrivateException::NOTBASEFILE, m_MoveInfo.srcFile);
		m_ExitCode = static_cast<int>(errorMsg.errorCode());
		m_ExitMessage = errorMsg.errorText();
		m_ExitMessage.append(errorMsg.detailInfo());

		m_ComCliExitMessage = m_ExitMessage;

		TRACE(cpf_MoveFileTrace, "%s", "Leaving executeMove(), invalid file name");
		return result;
	}

	try
	{
		// Open logical src file
		srcFileReference = DirectoryStructureMgr::instance()->open(srcFileId, FMS_CPF_Types::XR_XW_, m_MoveInfo.cpName.c_str());

		std::string srcVolume( srcFileReference->getVolume());

		// Move operations
		if(m_MoveInfo.dstVolume.compare(srcVolume) != 0)
		{
			FMS_CPF_Types::fileType ftype;
			bool compositeFile;
			unsigned int recordLength;
			int deleteFileTimer;
			std::list<std::string> moveSubFilesList;

			std::string srcFilePath = srcFileReference->getPath();
			// Prepare the full destination path
			std::string dstFilePath(srcFilePath);
			assemblePath(srcVolume, dstFilePath);

			TRACE(cpf_MoveFileTrace, "executeMove(), move to path:<%s>", dstFilePath.c_str() );
			FMS_CPF_Attribute srcFileAttribute = srcFileReference->getAttribute();
			srcFileAttribute.extractAttributes( ftype, compositeFile, recordLength, deleteFileTimer);

			FMS_CPF_Types::fileAttributes attributes;

			if( FMS_CPF_Types::ft_REGULAR == ftype )
			{
				// Move of regular file
				attributes.ftype = FMS_CPF_Types::ft_REGULAR;
				attributes.regular.rlength = recordLength;
				attributes.regular.composite = compositeFile;

				if(compositeFile)
				{
					TRACE(cpf_MoveFileTrace, "%s", "executeMove(), move a regular composite file");
					attributes.regular.deleteFileTimer = deleteFileTimer;
					// Move of a regular composite file
					bool recursive = false;
					// assemble the list of subfiles
					makeSubFilesList(moveSubFilesList);

					// create the new file
					createFile(attributes, srcFileReference);

					TRACE(cpf_MoveFileTrace, "%s", "executeMove(), composite file created");
					if( !moveSubFilesList.empty() )
					{
						// create and move subfiles
						recursive = true;
						FMS_CPF_File subFileToCreate;
						// Create subfiles
						if( SUCCESS == subFileToCreate.createInternalSubFiles(m_MoveInfo.srcFile, moveSubFilesList, m_MoveInfo.dstVolume, m_MoveInfo.cpName ) )
						{
							TRACE(cpf_MoveFileTrace, "executeMove(), <%zd> composite subfiles are been created", moveSubFilesList.size());
							//copy subfiles
							std::list<std::string>::iterator subFileIterator;
							for(subFileIterator = moveSubFilesList.begin(); subFileIterator != moveSubFilesList.end(); ++subFileIterator)
							{
								std::string srcSubFilePath = srcFilePath + DirDelim + (*subFileIterator);
								std::string dstSubFilePath = dstFilePath + DirDelim + (*subFileIterator);
								copyFile(srcSubFilePath, dstSubFilePath);
							}
						}
						else
						{
							TRACE(cpf_MoveFileTrace, "%s", "executeMove(), failed composite subfiles creation");
							throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
						}
					}
					// remove the moved file
					swapFiles(srcFileReference, recursive);
				}
				else
				{
					TRACE(cpf_MoveFileTrace, "%s", "executeMove(), move a regular simple file");
					// Move of a regular simple file
					// create the new file
					createFile(attributes, srcFileReference);
					// Physical file copy
					TRACE(cpf_MoveFileTrace, "executeMove(), move simple file<%s> to <%s>", srcFilePath.c_str(), dstFilePath.c_str() );
					// Physical file copy
					copyFile(srcFilePath, dstFilePath);
					// remove the moved file
					swapFiles(srcFileReference, false);
				}
			}
			else
			{
				TRACE(cpf_MoveFileTrace, "%s", "executeMove(), move a infinite file");
				//Move of Infinite file

				attributes.ftype = FMS_CPF_Types::ft_INFINITE;
				attributes.infinite.rlength = recordLength;
				std::string transferQueue;

				srcFileAttribute.extractExtAttributes( attributes.infinite.maxsize,
													   attributes.infinite.maxtime,
													   attributes.infinite.release,
													   attributes.infinite.active,
													   attributes.infinite.lastReportedSubfile,
													   attributes.infinite.mode,
													   transferQueue,
													   attributes.infinite.inittransfermode
													  );

				if( FMS_CPF_Types::tm_NONE != attributes.infinite.mode )
				{
					throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILEISPROT, errorText::FileAttachedToTQ);
				}

				ACE_OS::strncpy(attributes.infinite.transferQueue, transferQueue.c_str(), FMS_CPF_TQMAXLENGTH);

				// assemble the list of subfiles
				makeSubFilesList(moveSubFilesList);

				// create the new file
				createFile(attributes, srcFileReference);

				TRACE(cpf_MoveFileTrace, "%s", "executeMove(), infinite file created");

				// remove the first subfile automatically created
				removeISF(dstFilePath);

				if( !moveSubFilesList.empty() )
				{
					TRACE(cpf_MoveFileTrace, "executeMove(), <%zd> infinite subfiles move", moveSubFilesList.size());
					//copy subfiles
					FileReference subFileReference;
					std::list<std::string>::iterator subFileIterator;

					for(subFileIterator = moveSubFilesList.begin(); subFileIterator != moveSubFilesList.end(); ++subFileIterator)
					{
						std::string subFileId(m_MoveInfo.srcFile);
						// complete subfile name
						subFileId += SubFileSep + (*subFileIterator);

						subFileReference = DirectoryStructureMgr::instance()->create(subFileId, FMS_CPF_Types::NONE_, m_MoveInfo.cpName.c_str());

						std::string srcSubFilePath = srcFilePath + DirDelim + (*subFileIterator);
						std::string dstSubFilePath = dstFilePath + DirDelim + (*subFileIterator);
						// Physical file copy
						copyFile(srcSubFilePath, dstSubFilePath);
						DirectoryStructureMgr::instance()->closeExceptionLess(subFileReference, m_MoveInfo.cpName.c_str());
					}
				}

				// remove the moved file
				swapFiles(srcFileReference, true);
			}

			// CP file moved
			result = true;
		}
		else
		{
			TRACE(cpf_MoveFileTrace, "%s", "executeMove(), exception on file move src and dst volume are equal");
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::SAMEVOLUME, m_MoveInfo.dstVolume.c_str());
		}
	}
	catch(const FMS_CPF_PrivateException& ex)
	{
		m_ExitCode = static_cast<int>(ex.errorCode());
		m_ExitMessage = ex.errorText();
		m_ExitMessage.append(ex.detailInfo());

		m_ComCliExitMessage = m_ExitMessage;

		char errorMsg[256] = {'\0'};
		snprintf(errorMsg, 255,"%s(), file<%s> move failed, error:<%s>", __func__, m_MoveInfo.srcFile.c_str(), m_ExitMessage.c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(cpf_MoveFileTrace, "%s", errorMsg);
	}

	DirectoryStructureMgr::instance()->closeExceptionLess(srcFileReference, m_MoveInfo.cpName.c_str());
	TRACE(cpf_MoveFileTrace, "%s", "Leaving executeMove()");
	return result;

}

/*============================================================================
	ROUTINE: createFile
 ============================================================================ */
void CPF_MoveFile_Request::createFile(const FMS_CPF_Types::fileAttributes& attribute, FileReference& srcFileReference)
throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_MoveFileTrace, "%s", "Entering in swapFiles()");
	std::string emptyName;
	// Hidden the original file in order to create the same file into another volume
	srcFileReference->setTmpName(emptyName);

	try
	{
		// create the new file
		FMS_CPF_File file(m_MoveInfo.srcFile.c_str(), m_MoveInfo.cpName.c_str());
		file.create(attribute, m_MoveInfo.dstVolume.c_str());
	}
	catch(const FMS_CPF_Exception& ex)
	{
		// An internal API call could generate this type of exception

		char errorMsg[256] = {'\0'};
		snprintf(errorMsg, 255, "CPF_MoveFile_Request::createFile(), move file:<%s>, step create failed, error:<%s>", m_MoveInfo.srcFile.c_str(), ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(cpf_MoveFileTrace, "%s", errorMsg);
		// restore before return with error
		srcFileReference->setTmpName(m_MoveInfo.srcFile);
		throw FMS_CPF_PrivateException(FMS_CPF_Exception::GENERAL_FAULT);
	}
	TRACE(cpf_MoveFileTrace, "%s", "Leaving createFile()");
}

/*============================================================================
	ROUTINE: swapFiles
 ============================================================================ */
void CPF_MoveFile_Request::swapFiles(FileReference srcFileReference, bool recursive)
{
	TRACE(cpf_MoveFileTrace, "%s", "Entering in swapFiles()");
	//HZ17581
	FMS_CPF_Types::fileType ftype;
                       bool compositeFile;
                       unsigned int recordLength;
                       int deleteFileTimer;
        FMS_CPF_Attribute srcFileAttribute = srcFileReference->getAttribute();
        srcFileAttribute.extractAttributes( ftype, compositeFile, recordLength,deleteFileTimer);
	//HZ17581
	// Remove Cp file from the previous volume
	FileReference dstFileReference;

	FMS_CPF_FileId srcFileId(m_MoveInfo.srcFile);

	// Open logical dst file
	dstFileReference = DirectoryStructureMgr::instance()->open(srcFileId, FMS_CPF_Types::XR_XW_, m_MoveInfo.cpName.c_str());

	std::string fileDN;
	DirectoryStructureMgr::instance()->getFileDN(m_MoveInfo.srcFile, m_MoveInfo.cpName, fileDN);
	TRACE(cpf_MoveFileTrace, "swapFiles(), fileDN:<%s>", fileDN.c_str());

	std::string oldVolume = srcFileReference->getVolume();

	dstFileReference->setVolume(oldVolume);

	DirectoryStructureMgr::instance()->closeExceptionLess(dstFileReference, m_MoveInfo.cpName.c_str());

	TRACE(cpf_MoveFileTrace, "%s", "swapFiles(), remove Cp file form the previous volume");
	try
	{
		FMS_CPF_File file(m_MoveInfo.srcFile.c_str(), m_MoveInfo.cpName.c_str());
		// remove old file
		TRACE(cpf_MoveFileTrace, "%s", "swapFiles(), Before conditional wait");
		ACE_GUARD(ACE_Thread_Mutex, guard, mutex_lock);	
		if( FMS_CPF_Types::ft_INFINITE == ftype )
                {
                 FMS_CPF_OI_InfiniteFile::setMoveCpFileOpStatus(true);
                }
		file.deleteFile(recursive);
		//HZ17581
		if( FMS_CPF_Types::ft_INFINITE == ftype )
		cond_var.wait();
		TRACE(cpf_MoveFileTrace, "%s", "swapFiles(), After conditional wait");
	}
	catch(const FMS_CPF_Exception& ex)
	{
		// An internal API call could generate this type of exception
		char errorMsg[256] = {'\0'};
		snprintf(errorMsg, 255, "CPF_MoveFile_Request::swapFiles(), move file:<%s>, step remove failed, error:<%s>", m_MoveInfo.srcFile.c_str(), ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(cpf_MoveFileTrace, "%s", errorMsg);

		DirectoryStructureMgr::instance()->closeExceptionLess(dstFileReference, m_MoveInfo.cpName.c_str());
		throw FMS_CPF_PrivateException(FMS_CPF_Exception::GENERAL_FAULT);
	}

	// set the correct name on the moved file and close it
	srcFileReference->setVolume(m_MoveInfo.dstVolume);
	srcFileReference->setTmpName(m_MoveInfo.srcFile);

	// update the new file DN
	DirectoryStructureMgr::instance()->setFileDN(m_MoveInfo.srcFile, m_MoveInfo.cpName, fileDN);
	TRACE(cpf_MoveFileTrace, "%s", "Leaving swapFiles()");
}

/*============================================================================
	ROUTINE: removeISF
 ============================================================================ */
void CPF_MoveFile_Request::removeISF(std::string& mainFilePath)
{
	TRACE(cpf_MoveFileTrace, "%s", "Entering in removeISF()");
	char strSubFile[12]={'\0'};
	unsigned int subFileValue = 1;
	ACE_OS::snprintf(strSubFile, 11, "%.10i", subFileValue);

	std::string subFilePath = mainFilePath + DirDelim;
	subFilePath += strSubFile;
	TRACE(cpf_MoveFileTrace, "removeISF() first subfile<%s> to remove", subFilePath.c_str());

	if(SUCCESS != ::remove(subFilePath.c_str()) )
	{
		// Error on file remove log it
		char errorText[256]={0};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		char errorMsg[1024];
		snprintf(errorMsg, 1023,"CPF_MoveFile_Request::removeISF(), file=<%s> remove file failed, error=<%s>", subFilePath.c_str(), errorDetail.c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(cpf_MoveFileTrace, "%s", "removeISF(), error to remove infinite subfile for move");
	}

	TRACE(cpf_MoveFileTrace, "%s", "Leaving in removeISF()");
}



/*============================================================================
	ROUTINE: copyFile
 ============================================================================ */
void CPF_MoveFile_Request::copyFile(const std::string& srcFilePath, const std::string& dstFilePath)
	throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_MoveFileTrace, "%s", "Entering in copyFile()");
	char errorText[512] = {'\0'};
	std::ifstream srcFile(srcFilePath.c_str(), fstream::binary);

	if(srcFile.fail())
	{
		TRACE(cpf_MoveFileTrace, "copyfile() on move, error to open src file=<%s>", srcFilePath.c_str());
		std::string errorDetail(strerror_r(errno, errorText, 511));
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
	}

	std::ofstream dstFile(dstFilePath.c_str(), fstream::binary | fstream::trunc);

	if(dstFile.fail())
	{
		TRACE(cpf_MoveFileTrace, "copyfile() on move, error to open dst file=<%s>", dstFilePath.c_str());
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
		// Move should be always allowed, also in case of quota exceeded
		dstFile << srcFile.rdbuf();
		dstFile.flush();

		if( dstFile.bad() || dstFile.fail())
		{
			TRACE(cpf_MoveFileTrace, "copyfile() on move <%s> error", srcFilePath.c_str());
			std::string errorDetail(strerror_r(errno, errorText, 511));
			srcFile.close();
			dstFile.close();
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
		}
	}
	srcFile.close();
	dstFile.close();
	TRACE(cpf_MoveFileTrace, "%s", "Leaving copyFile()");
}

/*============================================================================
	ROUTINE: assemblePath
 ============================================================================ */
void CPF_MoveFile_Request::assemblePath(const std::string& srcVolume, std::string& dstPath)
throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_MoveFileTrace, "%s", "Entering in assemblePath()");
	size_t startPos;
	// find the volume position in the src path
	startPos = dstPath.find(srcVolume);

	if(startPos != std::string::npos)
	{
		// replace srcVolume with dstVolume
		dstPath.replace(startPos, srcVolume.length(), m_MoveInfo.dstVolume.c_str(), m_MoveInfo.dstVolume.length());
	}
	else
	{
		TRACE(cpf_MoveFileTrace, "%s", "assemblePath(), src Volume not found in the src path");
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::GENERAL_FAULT);
	}
	TRACE(cpf_MoveFileTrace, "Leaving assemblePath(), destination path:<%s>", dstPath.c_str());
}

/*============================================================================
	ROUTINE: volumeExist
 ============================================================================ */
bool CPF_MoveFile_Request::volumeExist()
{
	TRACE(cpf_MoveFileTrace, "%s", "Entering in volumeExist()");
	struct stat statbuf;
	std::string volumePath;
	bool result = false;

	volumePath = ParameterHndl::instance()->getCPFroot(m_MoveInfo.cpName.c_str());
	// Assemble the full path
	volumePath += DirDelim + m_MoveInfo.dstVolume;

	if((::stat(volumePath.c_str(), &statbuf) != FAILURE) && S_ISDIR(statbuf.st_mode))
	{
       result = true;
	}

	TRACE(cpf_MoveFileTrace, "Leaving volumeExist(), volume<%s> exists is %s", m_MoveInfo.dstVolume.c_str(), (result ? "TRUE" : "FALSE") );
	return result;
}

/*============================================================================
	ROUTINE: executeMove
============================================================================ */
void CPF_MoveFile_Request::makeSubFilesList(std::list<std::string>& subFilesToCreate)
throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_MoveFileTrace, "%s", "Entering in makeSubFilesList()");

	std::list<FMS_CPF_FileId>::iterator subFileIterator;
	std::list<FMS_CPF_FileId> subFilesList;
	FMS_CPF_FileId subFileid;

	FMS_CPF_FileId srcFileId(m_MoveInfo.srcFile);

	DirectoryStructureMgr::instance()->getListFileId(srcFileId, subFilesList, m_MoveInfo.cpName.c_str());

	TRACE(cpf_MoveFileTrace, "makeSubFilesList(), num=<%zd> subFiles", subFilesList.size());

	for(subFileIterator = subFilesList.begin(); subFileIterator != subFilesList.end(); ++subFileIterator)
	{
		subFileid = *subFileIterator;
		std::string subFileName = subFileid.subfileAndGeneration();
		// Insert subfile in the list of subfiles to create
		subFilesToCreate.push_back(subFileName);
		TRACE(cpf_MoveFileTrace, "makeSubFilesList(), subFile=<%s> must be created", subFileName.c_str() );
	}

	TRACE(cpf_MoveFileTrace, "%s", "Leaving makeSubFilesList()");
}
/*============================================================================
	ROUTINE: ~CPF_MoveFile_Request
 ============================================================================ */
CPF_MoveFile_Request::~CPF_MoveFile_Request()
{
	if(NULL != cpf_MoveFileTrace)
		delete cpf_MoveFileTrace;
}
