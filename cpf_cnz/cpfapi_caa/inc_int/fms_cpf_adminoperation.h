/*
 * *@file fms_cpf_admin_operation.h
 *	This module contains the declaration of the class FMS_CPF_AdminOperation.
 *  This class handles the Async Action Operation
 *
 *  Created on: Oct 20, 2011
 *      Author: enungai
 *
 *
 *	COPYRIGHT Ericsson AB, 2010
 *	All rights reserved.
 *
 *	The information in this document is the property of Ericsson.
 *	Except as specifically authorized in writing by Ericsson, the receiver of
 *	this document shall keep the information contained herein confidential and
 *	shall protect the same in whole or in part from disclosure and dissemination
 *	to third parties. Disclosure and dissemination to the receivers employees
 *	shall only be made on a strict need to know basis.
 *
 *	REVISION INFO
 *	+========+============+==============+=====================================+
 *	| REV    | DATE       | AUTHOR       | DESCRIPTION                         |
 *	+========+============+==============+=====================================+
 *	| 1.0.0  | 2011-10-20 | enungai      | File created.                       |
  *	| 1.1.0  | 2012-05-10 | qvincon      | Adaption to the new APGCC class.    |
 *	+========+============+==============+=====================================+*/

#ifndef FMS_CPF_ADMINOPERATION_H_
#define FMS_CPF_ADMINOPERATION_H_

#include "acs_apgcc_adminoperationasync_V2.h"
#include <ace/Task.h>

#include <string>

class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_AdminOperation: public ACE_Task_Base,
							  public acs_apgcc_adminoperationasync_V2
{
 public:

	/** @brief Constructor method
	*/
	FMS_CPF_AdminOperation();

	/** @brief Destructor method
	*/
	virtual ~FMS_CPF_AdminOperation();

	/** @brief sendAsyncActionToServer method the Admin Operation Asynchronous
	 *
	 *	This method
	 *	- initialize the object
	 *	- send AdminOperation Asynchronous
	 * 	- wait for the call-back
	 *
	 *	@param p_objName: DN related to the object on which the Administrative Operation is called
	 *	@param operationId: The identifier of the Administrative operation.
	 *	@param paramVector: Vector of acs_apgcc_adminoperationParamType  to be used as parameters for the Administrative
     * 		   operation.
	*/
	int sendAsyncActionToServer(const std::string& p_objName, ACS_APGCC_AdminOperationIdType operationId, std::vector<ACS_APGCC_AdminOperationParamType>& paramVector);

	/**
   	   @brief  Internal thread to wait callback call
	*/
	virtual int svc(void);

	/** @brief objectManagerAdminOperationCallback method
	 *
	 *	This method This is a virtual method to be implemented by the Designer when extending
	 *  the base class.
	 *  This method will be called as a callback to notify an asynchronous administrative operation execution.
	 *
	 * @param invocation : the invocation Id used by the application calling the Admin Operation Asynchronous
	 *
	 * @param returnVal : the return value. This value is meaningful only if the error param is has value 1 (IMM OP is OK), in this case
	 * returnVal value indicates the value returned by the Admin Operation Implementer for the Admin Operation requested.
	 * this value is specific to the AdminOperation being performed and is valid only if error param is has value 1.
	 *
	 * @param error: indicates if the IMM Service succeed or not to invoke the admin Operation implementer.
	 *
	 */
	virtual void objectManagerAdminOperationCallback( ACS_APGCC_InvocationType invocation, int returnVal , int error, ACS_APGCC_AdminOperationParamType** outParamVector );

	/** @brief getOIError method
	 *
	 *	This method returns the error from the Object Implementer
	 *
	 *	Return the error from the Object Implementer
	*/
	int getOIError() { return m_InternalError; };

	/** @brief getExitCode method
	 *
	 *	This method returns the operation exit code
	 *
	*/
	int getExitCode() { return m_ActionExitCode; };

	/** @brief getErrorDetail method
	 *
	 *	This method returns the error details when an operation error occurs
	 *
	*/
	void getErrorDetail(std::string& errorDetail){ errorDetail = m_ActionErrorDetail; };

 private:

	/** @brief stopInternalThread method
	 *
	 *	This method signals to the internal thread to terminate
	 *
	*/
	void stopInternalThread();

	/**
		@brief	m_ActionExitCode: the exit code returned from the server
	*/
	int m_ActionExitCode;

	/**
		@brief	m_ActionErrorDetail: the error detail returned from the server
	*/
	std::string m_ActionErrorDetail;

	/**
		@brief	m_InternalError: internal error code
	*/
	int m_InternalError;

	/**
		@brief	m_StopEvent: to signal to the internal thread to terminate
	*/
	int m_StopEvent;

	/**
		@brief	fmsCpfAsyncAction: trace object
	*/
	ACS_TRA_trace* fmsCpfAsyncAction;
};

#endif /* FMS_CPF_ADMINOPERATION_H_ */
