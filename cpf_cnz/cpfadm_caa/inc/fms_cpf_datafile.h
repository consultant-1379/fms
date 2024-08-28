/*
 * * @file fms_cpf_datafile.h
 *	@brief
 *	Header file for CPF_DataFile_Request class.
 *  This module contains the declaration of the class CPF_DataFile_Request.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-09
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
 *	| 1.0.0  | 2011-11-09 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_DATAFILE_H_
#define FMS_CPF_DATAFILE_H_

#include "fms_cpf_privateexception.h"

#include <ace/Future.h>
#include <ace/Method_Request.h>

#include <string>

class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_DataFile_Request: public ACE_Method_Request {

 public:
	struct dataFile{
						std::string mainFileName;
						std::string subFileName;
						std::string cpName;
						unsigned int fileSize;
						unsigned int numOfReaders;
						unsigned int numOfWriters;
						int exclusiveAccess;
						unsigned int lastSubFileActive;
						unsigned int lastSubFileSent;
						std::string transferQueueDn;

						// active infinite subfile info
						unsigned int activeSubfileNumOfReaders;
						unsigned int activeSubfileNumOfWriters;
						unsigned int activeSubfileSize;
						int activeSubfileExclusiveAccess;


	};
	/**
		@brief	Constructor of CPF_DataFile_Request class
	*/
	CPF_DataFile_Request(ACE_Future<FMS_CPF_PrivateException>& result, dataFile& fileInfo, bool isMainFileInfo = false, bool skipSize = false);

	/**
		@brief	Constructor of CPF_DataFile_Request class
	*/
	virtual ~CPF_DataFile_Request();

	/**
		@brief	This method implements the action requested
	*/
	virtual int call();

 private:

	void getActiveSubfileInfo();

   	ACE_Future<FMS_CPF_PrivateException> m_result;

   	/**
   		@brief	The file parameters structure
   	*/
   	dataFile& m_FileInfo;

   	bool m_ExtraInfiniteFileInfo ;

   	bool m_SkipSize;

   	ACS_TRA_trace* cpf_DataFileTrace;

   	// Disallow copying and assignment.
   	CPF_DataFile_Request(const CPF_DataFile_Request& );
    void operator= (const CPF_DataFile_Request& );
};

#endif /* FMS_CPF_DATAFILE_H_ */
