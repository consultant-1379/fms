/*
 * * @file fms_cpf_exportfile.h
 *	@brief
 *	Header file for CPF_ExportFile_Request class.
 *  This module contains the declaration of the class CPF_ExportFile_Request.
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

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_EXPORTFILE_H_
#define FMS_CPF_EXPORTFILE_H_
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_types.h"
#include "fms_cpf_privateexception.h"
#include "fms_cpf_fileid.h"

#include <string>
#include <list>

class ACS_TRA_trace;
/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_ExportFile_Request {

 public:

	struct exportFileData{
						std::string cpName;
						std::string srcFile;
						std::string dstPath;
						std::string relativeDstPath;
						std::string srcPath;
						FMS_CPF_Types::copyMode exportOption;
						bool zipData;
	};

	/**
		@brief	Constructor of CPF_ExportFile_Request class
	*/
	CPF_ExportFile_Request(const exportFileData& exportInfo);

	/**
			@brief	Destructor of CPF_ExportFile_Request class
	*/
	virtual ~CPF_ExportFile_Request();

	/**
		@brief	This method implements the action requested
	*/
	bool executeExport();

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
		@brief	This method exports a CP composite file to a AP directory
	*/
	void exportCompositeFile() throw (FMS_CPF_PrivateException);

	/**
		@brief	This method exports a CP file to a Zip File
	*/
	void exportFileToZip(bool allFiles = false) throw (FMS_CPF_PrivateException);

	/**
	 * @brief	This method checks if the zip file already exist
	 *
	 * @param 	zipFile: the zip archive to check
	 *
	 * @return 	True if the zip exists, false otherwise
	*/
	bool zipExist(std::string& zipFile);


	/**
		@brief	This method exports a CP subfile or a simple file to an AP file
	*/
	void exportSubFile() throw (FMS_CPF_PrivateException);

	/**
		@brief	This method checks the dst folder
	*/
	void checkDstFolder(const std::list<FMS_CPF_FileId>& exportFileList) throw (FMS_CPF_PrivateException);

	/**
	   	@brief	This method copies physical files
	*/
  	void copyFile(const std::string& srcFilePath, const std::string& dstFilePath, int oflag)
	  		throw (FMS_CPF_PrivateException);

	/**
		@brief	The file import parameters structure
	*/
	exportFileData m_ExportInfo;

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

	ACS_TRA_trace* cpf_ExportFileTrace;


};

#endif /* FMS_CPF_EXPORTFILE_H_ */
