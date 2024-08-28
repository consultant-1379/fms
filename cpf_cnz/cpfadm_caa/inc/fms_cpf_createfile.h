/*
 * * @file fms_cpf_createfile.h
 *	@brief
 *	Header file for CPF_CreateFile_Request class.
 *  This module contains the declaration of the class CPF_CreateFile_Request.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-07-04
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
 *	| 1.0.0  | 2011-07-04 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_CREATEFILE_H_
#define FMS_CPF_CREATEFILE_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_types.h"
#include "fms_cpf_privateexception.h"

#include <ace/Method_Request.h>
#include <ace/Future.h>
#include <string>
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_CreateFile_Request: public ACE_Method_Request {
 public:

	struct cpFileData{
		        FMS_CPF_Types::fileType fileType;
				std::string volumeName;
				std::string fileName;
				std::string fileDN;
				std::string subFileName;
				std::string cpName;
				std::string transferQueue;
				FMS_CPF_Types::transferMode tqMode;
				unsigned int recordLength;
                                 //HY46076
                                long int deleteFileTimer;
				unsigned int maxSize;
				unsigned int maxTime;
				bool releaseCondition;
				bool composite;
	};

	/**
		@brief	Constructor of CPF_CreateFile_Request class
	*/
	CPF_CreateFile_Request(ACE_Future<FMS_CPF_PrivateException>& result, const cpFileData& fileInfo);

	/**
		@brief	Destructor of CPF_CreateFile_Request class
	*/
	virtual ~CPF_CreateFile_Request();

	/**
		@brief	This method implements the action requested
	*/
	virtual int call();

 private:

 	ACE_Future<FMS_CPF_PrivateException> m_result;

 	/**
 		@brief	The file parameters structure
 	*/
 	cpFileData m_FileInfo;

 	ACS_TRA_trace* cpf_CreateFileTrace;
  	// Disallow copying and assignment.
 	CPF_CreateFile_Request (const CPF_CreateFile_Request &);
   	void operator= (const CPF_CreateFile_Request &);
};

#endif /* FMS_CPF_CREATEFILE_H_ */
