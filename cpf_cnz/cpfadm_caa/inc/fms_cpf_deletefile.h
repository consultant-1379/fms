/*
 * * @file fms_cpf_deletefile.h
 *	@brief
 *	Header file for CPF_DeleteFile_Request class.
 *  This module contains the declaration of the class CPF_DeleteFile_Request.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-08-24
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
 *	| 1.0.0  | 2011-08-44 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_DELETEFILE_H_
#define FMS_CPF_DELETEFILE_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_filereference.h"

#include "fms_cpf_types.h"
#include "fms_cpf_privateexception.h"
#include <string>

class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_DeleteFile_Request
{
 public:
	struct deleteFileData{
					FMS_CPF_Types::fileType fileType;
					std::string fileName;
					std::string cpName;
					bool isMultiCP;
					bool composite;
	};

	/**
		@brief	Constructor of CPF_DeleteFile_Request class
	*/
	CPF_DeleteFile_Request(const deleteFileData& fileInfo);

	bool acquireFileLock(FMS_CPF_PrivateException& result);

	/**
		@brief	Destructor of CPF_DeleteFile_Request class
	*/
	virtual ~CPF_DeleteFile_Request();

	/**
		@brief	This method implements the action requested
	*/
	bool deleteFile();
	//HY46076
	const std::string& getFileName();

 private:

  	/**
  		@brief	The file parameters structure
  	*/
  	deleteFileData m_FileInfo;

  	/**
  		@brief	The reference to the file to delete
  	*/
  	FileReference m_FileReference;

  	/**
  		@brief	Indicates a recursive deletion of composite file
  	*/
	bool m_RecursiveDelete;

  	ACS_TRA_trace* cpf_DeleteFileTrace;

  	// Disallow copying and assignment.
  	CPF_DeleteFile_Request (const CPF_DeleteFile_Request &);
    void operator= (const CPF_DeleteFile_Request &);

};

#endif /* FMS_CPF_DELETEFILE_H_ */
