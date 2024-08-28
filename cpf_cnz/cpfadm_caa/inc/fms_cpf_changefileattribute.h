/*
 * * @file fms_cpf_changefileattribute.h
 *	@brief
 *	Header file for CPF_ChangeFileAttribute_Request class.
 *  This module contains the declaration of the class CPF_ChangeFileAttribute_Request.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-08-27
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
 *	| 1.0.0  | 2011-08-27 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_CHANGEFILEATTRIBUTE_H_
#define FMS_CPF_CHANGEFILEATTRIBUTE_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_filereference.h"
#include "fms_cpf_privateexception.h"
#include "fms_cpf_types.h"

#include <ace/Method_Request.h>
#include <ace/Future.h>
#include <string>

class ACS_TRA_trace;

namespace changeSet{

    const unsigned short COMPRESSION_CHANGE = 0x01;
    const unsigned short MAXTIME_CHANGE = 0x02;
    const unsigned short MAXSIZE_CHANGE = 0x04;
    const unsigned short RELCOND_CHANGE = 0x08;
    const unsigned short FILETQ_CHANGE = 0x10;
    const unsigned short BLOCKTQ_CHANGE = 0x20;
    const unsigned short FILENAME_CHANGE = 0x40;
    const unsigned short DELETEFILETIMER_CHANGE = 0x80;
}

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_ChangeFileAttribute_Request: public ACE_Method_Request
{
	public:

		struct cpFileModData{
			        FMS_CPF_Types::fileType fileType;
					std::string currentFileName;
					std::string newFileName;
					std::string cpName;
					unsigned int newMaxSize;
					unsigned int newMaxTime;
					bool newReleaseCondition;
					bool newCompression;
					std::string newTransferQueue;
					std::string oldTransferQueue;
					FMS_CPF_Types::transferMode newTQMode;
					FMS_CPF_Types::transferMode oldTQMode;
					unsigned short changeFlags;
                                         //HY46076
                                        long int newdeleteFileTimer;
		};

		/**
			@brief	Constructor of CPF_ChangeFileAttribute_Request class
		*/
		CPF_ChangeFileAttribute_Request(ACE_Future<FMS_CPF_PrivateException>& result, cpFileModData& fileInfo);

		/**
			@brief	Destructor of CPF_ChangeFileAttribute_Request class
		*/
		virtual ~CPF_ChangeFileAttribute_Request();

		/**
			@brief	This method implements the action requested
		*/
		virtual int call();

	 private:

		ACE_Future<FMS_CPF_PrivateException> m_result;

		/**
			@brief	The file parameters structure
		*/
		cpFileModData& m_FileInfo;

		ACS_TRA_trace* cpf_ChangeAttrTrace;

		// Disallow copying and assignment.
		CPF_ChangeFileAttribute_Request (const CPF_ChangeFileAttribute_Request &);
		void operator= (const CPF_ChangeFileAttribute_Request &);
};

#endif /* FMS_CPF_CHANGEFILEATTRIBUTE_H_ */
