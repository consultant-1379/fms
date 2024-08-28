/*
 * * @file fms_cpf_copyfile.h
 *	@brief
 *	Header file for CPF_CopyFile_Request class.
 *  This module contains the declaration of the class CPF_CopyFile_Request.
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

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF__COPYFILE_H_
#define FMS_CPF__COPYFILE_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_filereference.h"

#include "fms_cpf_types.h"
#include "fms_cpf_privateexception.h"
#include "fms_cpf_fileid.h"
#include "fms_cpf_attribute.h"

#include <string>
#include <list>
#include <fstream>

class ACS_TRA_trace;
/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class CPF_CopyFile_Request
{
 public:

	struct copyFileData{
				std::string cpName;
				std::string srcFile;
				std::string dstFile;
				std::string dstVolume;
				FMS_CPF_Types::copyMode copyOption;
	};

	/**
		@brief	Constructor of CPF_CopyFile_Request class
	*/
	CPF_CopyFile_Request(const copyFileData& copyInfo );

	/**
		@brief	Destructor of CPF_CopyFile_Request class
	*/
	virtual ~CPF_CopyFile_Request();

	/**
		@brief	This method implements the action requested
	*/
	bool executeCopy();

	/**
		@brief	This method gets the error code
	*/
	int getErrorCode() const { return m_ExitCode; };

	/**
		@brief	This method gets the error message
	*/
	const char* getErrorMessage() const { return m_ExitMessage.c_str(); };

	/**
		@brief	This method gets the com-cli error message
	*/
	const char* getComCliErrorMessage() const { return m_ComCliExitMessage.c_str(); };

 private:

  	/**
  		@brief	This method checks if the the src CP file is associated to a TQ
  	*/
  	bool checkFileProtection(FileReference srcFileReference);

  	/**
  	  	@brief	This method checks if the the dst CP file is valid
  	*/
  	int checkDestinationFile(FMS_CPF_Attribute& fileAttribute, std::string& error);

  	/**
  	  	@brief	This method copies composite CP files
  	*/
  	void copyCompositeFile(const std::string& srcFilePath)
  		throw (FMS_CPF_PrivateException);

  	/**
		@brief	This method copies SubFiles
	*/
	void copySubFile(const std::string& srcFilePath)
		throw (FMS_CPF_PrivateException);

  	/**
  	   	@brief	This method copies physical files
  	*/
  	void copyFile(const std::string& srcFilePath, const std::string& dstFilePath, std::fstream::openmode oflag)
  		throw (FMS_CPF_PrivateException);

  	/**
  	   	@brief	This method assembles the lists of subfiles to create and delete
  	*/
  	void makeFilesList(std::list<FMS_CPF_FileId>& srcFileList, std::list<std::string>& subFilesToCreate)
  		throw (FMS_CPF_PrivateException);

  	/**
  	   	@brief	This method removes the subfiles in the destination file
  	*/
  	bool removeSubFiles();

  	/**
  	  	@brief	This method creates the subfiles in the list
  	*/
  	bool createSubFiles(const std::list<std::string>& subFileNameList);

  	/**
  		@brief	The file copy parameters structure
  	*/
  	copyFileData m_CopyInfo;

  	/**
		@brief	The action exit code
	*/
	int m_ExitCode;

	/**
		@brief	The action exit message
	*/
	std::string m_ExitMessage;

	/**
		@brief	The action COM-CLI exit message
	*/
	std::string m_ComCliExitMessage;

  	ACS_TRA_trace* cpf_CopyFileTrace;

  	// Disallow copying and assignment.
  	CPF_CopyFile_Request(const CPF_CopyFile_Request &);
    void operator= (const CPF_CopyFile_Request &);
};

#endif /* FMS_CPF__COPYFILE_H_ */
