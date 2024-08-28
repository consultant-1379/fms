/*
 * * @file fms_cpf_cpmsgfactory.h
 *	@brief
 *	Header file for CPMsgFactory class.
 *  This module contains the declaration of the class CPMsgFactory.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-15
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
 *	| 1.0.0  | 2011-11-15 | qvincon      | File imported/updated.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_CPMSGFACTORY_H_
#define FMS_CPF_CPMSGFACTORY_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include <ace/Singleton.h>
#include <ace/Synch.h>

#include <stdint.h>
/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class FMS_CPF_CpChannel;
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FMS_CPF_CPMsgFactory
{
 public:

	friend class ACE_Singleton<FMS_CPF_CPMsgFactory, ACE_Recursive_Thread_Mutex>;

	/** @brief handleMsg method
	 *
	 *	This method creates an object of appropriate type for the given buffer
	 *
	 *  @param aInBuffer: message buffer.
	 *
	 *  @param aInBufferSize: message buffer size.
	 *
	 *  @param CpChannel: Pointer of the FMS_CPF_CpChannel object.
	*/
	bool handleMsg(const char* aInBuffer, const unsigned int aInBufferSize, FMS_CPF_CpChannel* CpChannel);

 private:

	/**
	 * 	@brief	getRequestCode method
	 *
	 *	This method gets the request message code into the buffer
	*/
	void getRequestCode(const char* aInBuffer, const unsigned int aInBufferSize, uint16_t& requestCode);

	/**
	 * 	@brief	Constructor of FMS_CPF_CPMsgFactory class
	*/
	FMS_CPF_CPMsgFactory();

	/**
	 * 	@brief	Destructor of FMS_CPF_CPMsgFactory class
	*/
	virtual ~FMS_CPF_CPMsgFactory();

	/**
	 * 	@brief	fms_cpf_MsgFactoryTrace: trace object
	*/
	ACS_TRA_trace* fms_cpf_MsgFactoryTrace;
};

typedef ACE_Singleton<FMS_CPF_CPMsgFactory, ACE_Recursive_Thread_Mutex> CPMsgFactory;

#endif /* FMS_CPF_CPMSGFACTORY_H_ */

