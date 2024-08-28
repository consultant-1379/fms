/*
 * * @file fms_cpf_importfile.h
 *	@brief
 *	Header file for CPF_ImportFile_Request class.
 *  This module contains the declaration of the class CPF_ImportFile_Request.
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

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_IMPORTFILE_H_
#define FMS_CPF_IMPORTFILE_H_
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_types.h"
#include "fms_cpf_privateexception.h"
#include "fms_cpf_fileid.h"

#include "ace/OS.h"
#include <string>
#include <list>
#include <fstream>

class ACS_TRA_trace;
/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_ImportFile_Request {
 public:

	struct importFileData{
					std::string cpName;
					std::string srcPath;
					std::string relativeSrcPath;
					std::string dstFile;
					std::string dstVolume;
					std::string dstPath;
					FMS_CPF_Types::copyMode importOption;
	};

	/**
		@brief	Constructor of CPF_ImportFile_Request class
	*/
	CPF_ImportFile_Request(const importFileData& importInfo );

	/**
	   	@brief	This method valids source file name
	*/
	bool checkSourceFileName(const std::list<std::string>& fileList);

	/**
	   	@brief	This method removes the subfiles in the destination file
	*/
	bool removeSubFiles();

	/**
		@brief	Destructor of CPF_ImportFile_Request class
	*/
	virtual ~CPF_ImportFile_Request();

	/**
		@brief	This method implements the action requested
	*/
	bool executeImport();

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
		@brief	This method checks the source directory is valid
	*/
	bool checkSourceFolder(const std::string& srcPath, std::list<std::string>& fileList, bool& isZipped) throw (FMS_CPF_PrivateException);

	/**
		@brief	This method checks for a zipped source
	*/
	bool checkZippedSource(const std::string& srcPath, std::list<std::string>& fileList);

	/**
		@brief	This method gets the list of files into the zip file
	*/
	bool getZippedFileList(const std::string& zipFile, std::list<std::string>& fileList);

	/**
		@brief	This method extracts files to destination folder
	*/
	int exctractZippedFile(const std::string& zipFile, const std::string& dstFolder);

	/**
		@brief	This method converts extracted files name in lower case to upper case
	*/
	void checkLowerCaseName(const std::list<std::string>& fileList, const std::string& dstPath);

	/**
		@brief	This method imports files to a composite file
	*/
	void importToCompositeFile(const std::list<std::string>& importFileList, const std::list<std::string>& fileCreateList, bool isSingleFile, bool isZipped) throw (FMS_CPF_PrivateException);

	/**
		@brief	This method  imports a file to a subfile or a simple file
	*/
	void importToSubFile() throw (FMS_CPF_PrivateException);

	/**
  	   	@brief	This method assembles the lists of subfiles to create and delete
  	*/
  	void makeFilesList(const std::list<std::string>& srcFileList, const std::list<FMS_CPF_FileId>& dstFileList, std::list<std::string>& subFilesToCreate)
  		throw (FMS_CPF_PrivateException);

  	/**
  	  	@brief	This method creates the subfiles in the list
  	*/
  	bool createSubFiles(const std::list<std::string>& subFileNameList);

	/**
  	   	@brief	This method copies physical files
  	*/
  	void copyFile(const std::string& srcFilePath, const std::string& dstFilePath, int oflag /*std::fstream::openmode oflag*/)
  		throw (FMS_CPF_PrivateException);      // HW79785 BEGIN

	/**
		@brief	The file import parameters structure
	*/
	importFileData m_ImportInfo;

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

	ACS_TRA_trace* cpf_ImportFileTrace;

	class FileSearch
	{
	  public:

		  FileSearch();
		  FileSearch(const std::string& path );
		  bool open();
		  bool find( std::string& entry ) throw(FMS_CPF_PrivateException);
		  void close();

	  private:

		  bool m_first;
		  ACE_DIR* dirp;
		  ACE_DIRENT* direntp;
		  std::string m_path;
	};

	// Disallow copying and assignment.
	CPF_ImportFile_Request(const CPF_ImportFile_Request &);
	void operator=(const CPF_ImportFile_Request &);
};

#endif /* FMS_CPF_IMPORTFILE_H_ */
