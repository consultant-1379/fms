/*
 * * @file fms_cpf_createvolume.h
 *	@brief
 *	Header file for CPF_CreateVolume_Request class.
 *  This module contains the declaration of the class CPF_CreateVolume_Request.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-23
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
 *	| 1.0.0  | 2011-06-23 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_CREATEVOLUME_H_
#define FMS_CPF_CREATEVOLUME_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_privateexception.h"

#include <ace/Method_Request.h>
#include <ace/Future.h>
#include <string>
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_CreateVolume_Request: public ACE_Method_Request {
 public:

	struct systemData{
			std::string volumeName;
			std::string cpName;
			bool isMultiCP;
	};

	/**
		@brief	Constructor of CPF_CreateVolume_Request class
	*/
	CPF_CreateVolume_Request(ACE_Future<FMS_CPF_PrivateException>& result, const systemData& vInfo);

	/**
		@brief	Destructor of CPF_CreateVolume_Request class
	*/
	virtual ~CPF_CreateVolume_Request();

	/**
		@brief	This method implements the action requested
	*/
	virtual int call();


 private:

	ACE_Future<FMS_CPF_PrivateException> m_result;
	systemData m_SystemInfo;

	ACS_TRA_trace* cpf_CreateVolumeTrace;
 	// Disallow copying and assignment.
	CPF_CreateVolume_Request (const CPF_CreateVolume_Request &);
  	void operator= (const CPF_CreateVolume_Request &);
};

#endif /* FMS_CPF_CREATEVOLUME_H_ */
