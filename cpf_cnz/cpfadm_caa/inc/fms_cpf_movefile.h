/*
 * * @file fms_cpf_movefile.h
 *	@brief
 *	Header file for CPF_CopyFile_Request class.
 *  This module contains the declaration of the class CPF_MoveFile_Request.
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
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_MOVEFILE_H_
#define FMS_CPF_MOVEFILE_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_filereference.h"

#include "fms_cpf_types.h"
#include "fms_cpf_privateexception.h"

#include <string>
#include <list>

class ACS_TRA_trace;
/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_MoveFile_Request
{
 public:

	struct moveFileData{
					std::string srcFile;
					std::string dstVolume;
					std::string cpName;
	};

	/**
		@brief	Constructor of CPF_MoveFile_Request class
	*/
	CPF_MoveFile_Request(const moveFileData& moveInfo);

	/**
		@brief	Destructor of CPF_MoveFile_Request class
	*/
	virtual ~CPF_MoveFile_Request();

	/**
		@brief	This method implements the action requested
	*/
	bool executeMove();

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
	 	@brief	This method assembles the lists of subfiles to create and delete
	*/
	void makeSubFilesList(std::list<std::string>& subFilesToCreate) throw (FMS_CPF_PrivateException);

	/**
	 	@brief	This method assembles the destination file path
	*/
	void assemblePath(const std::string& srcVolume, std::string& dstPath) throw (FMS_CPF_PrivateException);

	/**
	 	@brief	This method copy files from src to dst
	*/
	void copyFile(const std::string& srcFilePath, const std::string& dstFilePath)  throw (FMS_CPF_PrivateException);

	/**
	 	@brief	This method remove an ISF
	*/
	void removeISF(std::string& mainFilePath);

	/**
	 	@brief	This method swaps the file
	*/
	void swapFiles(FileReference srcFileReference, bool recursive);

	/**
		@brief	This method creates the file in the new volume
	*/
	void createFile(const FMS_CPF_Types::fileAttributes& attribute, FileReference& srcFileReference)
		throw (FMS_CPF_PrivateException);

	/**
	 	@brief	This method checks if the destination volume exists
	*/
	bool volumeExist();

	/**
 		@brief	The file move parameters structure
 	*/
 	moveFileData m_MoveInfo;

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

   	ACS_TRA_trace* cpf_MoveFileTrace;

};

#endif /* FMS_CPF_MOVEFILE_H_ */
