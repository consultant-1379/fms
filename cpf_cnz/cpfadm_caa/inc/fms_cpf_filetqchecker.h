/*
 * * @file fms_cpf_filetqchecker.h
 *	@brief
 *	Header file for FMS_CPF_FileTQChecker class.
 *  This module contains the declaration of the class FMS_CPF_FileTQChecker.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-03-13
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
 *	| 1.0.0  | 2012-03-13 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_FILETQCHECKER_H_
#define FMS_CPF_FILETQCHECKER_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "aes_ohi_extfilehandler2.h"

#include <ace/RW_Thread_Mutex.h>
#include <string>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_FileTQChecker : public AES_OHI_ExtFileHandler2
{
 public:

	/**
	 * 	@brief	Constructor of FMS_CPF_FileTQChecker class
	*/
	FMS_CPF_FileTQChecker();

	/**
	 * 	@brief	Destructor of FMS_CPF_FileTQChecker class
	*/
	virtual ~FMS_CPF_FileTQChecker();

	/**
	 * 	@brief	Call back method to handle the AFP event
	*/
	virtual unsigned int handleEvent(AES_OHI_Eventcodes eventCode);

	/** @brief validateTQ method
	 *
	 * 	This method validate a file TQ
	 *
	 * 	@param tqName : Transfer queue name
	 *
	 *  @param errorCode : error code returned from OHI
	 *
	 *  @return bool : true on File TQ defined, otherwise false.
	 *
	 *  @remarks Remarks
	*/
	bool validateTQ(const std::string& tqName, unsigned int& errorCode);

	/** @brief validateTQ method
	 *
	 * 	This method validate a file TQ
	 *
	 * 	@param tqName : Transfer queue name
	 *
	 * 	@param tqDN : Transfer queue DN
	 *
	 *  @param errorCode : error code returned from OHI
	 *
	 *  @return bool : true on File TQ defined, otherwise false.
	 *
	 *  @remarks Remarks
	*/
	bool validateTQ(const std::string& tqName, std::string& tqDN, unsigned int& errorCode);

	/** @brief removeSentFile method
	 *
	 * 	This method removes a sent file
	 *
	 *  @param tqName :  The name of the file transfer queue
	 *
     *  @param fileName :  The file to be removed.
     *
	 *  @return unsigned int : error code returned from OHI
	 *
	 *  @remarks Remarks
	*/
	unsigned int removeSentFile(const std::string& tqName, const std::string& fileName);

 private:

	/**
	* 	@brief	m_mutex
	*
	* 	Mutex for internal sync
	*/
	ACE_RW_Thread_Mutex m_mutex;

	/**
	 * 	@brief	m_connectionStatus: Connection status to OHI
	*/
	bool m_connectionStatus;

	/**
	 * 	@brief	m_trace: trace object
	*/
	ACS_TRA_trace* m_trace;
};

#endif /* FMS_CPF_FILETQCHECKER_H_ */
