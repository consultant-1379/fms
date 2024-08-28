/*
 * * @file fms_cpf_releasesubfile.h
 *	@brief
 *	Header file for CPF_ReleaseISF_Request class.
 *  This module contains the declaration of the class CPF_ReleaseISF_Request.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-01-12
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
 *	| 1.0.0  | 2012-01-11 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_RELEASESUBFILE_H_
#define FMS_CPF_RELEASESUBFILE_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_privateexception.h"
#include <string>

/*=====================================================================
					FORWARD DECLARATION SECTION
==================================================================== */
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class CPF_ReleaseISF_Request
{
 public:

	struct fileData{
				   std::string cpName;
				   std::string fileName;
	};

	/**
		@brief	Constructor of CPF_ReleaseInfiniteSubFile_Request class
	*/
	CPF_ReleaseISF_Request(const fileData& fileInfo);

	/**
		@brief	Destructor of CPF_ReleaseInfiniteSubFile_Request class
	*/
	virtual ~CPF_ReleaseISF_Request();

	/**
		@brief	This method implements the action requested
	*/
	bool executeReleaseISF(FMS_CPF_PrivateException& switchResult);

 private:

	/**
		@brief	The file import parameters structure
	*/
	fileData m_FileInfo;

	ACS_TRA_trace* cpf_ReleaseISFTrace;

};

#endif /* FMS_CPF_RELEASESUBFILE_H_ */
