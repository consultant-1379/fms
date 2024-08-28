/*
 * * @file fms_cpf_oi_backuprestore.h
 *	@brief
 *	Header file for FMS_CPF_OI_BackupRestore class.
 *  This module contains the declaration of the class FMS_CPF_OI_BackupRestore.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-06-27
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
 *	| 1.0.0  | 2012-06-27 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_OI_BACKUPRESTORE_H_
#define FMS_CPF_OI_BACKUPRESTORE_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include <acs_apgcc_objectimplementerinterface_V3.h>

#include <ace/Singleton.h>
#include <ace/Synch.h>


/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */

class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_OI_BackupRestore : public acs_apgcc_objectimplementerinterface_V3
{
 public:

	friend class ACE_Singleton<FMS_CPF_OI_BackupRestore, ACE_Recursive_Thread_Mutex>;

	/** @brief create method
	 *
	 *	This method will be called as a callback when an Object is created under the backup object.
	 *
	 *  @return ACS_CC_ReturnType On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE.
	 */
	virtual ACS_CC_ReturnType create(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *className, const char* parentname, ACS_APGCC_AttrValues **attr);

	/** @brief deleted method
	 *
	 *	This method will be called as a callback when deleting the backup object
	 *
	 *  @return ACS_CC_ReturnType On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE.
	 *
	 *	@remarks Remarks
	 */
	virtual ACS_CC_ReturnType deleted(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName);

	/** @brief modify method
	 *
	 *	This method will be called as a callback when modifying the backup object
	 *
	 *  @return ACS_CC_ReturnType On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE.
	 *
	 *	@remarks Remarks
	 */
	virtual ACS_CC_ReturnType modify(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName, ACS_APGCC_AttrModification **attrMods);

	/** @brief complete method
	 *
	 *	This method will be called as a callback when a Configuration Change Bundle is complete and can be applied
	 *
	 *  @return ACS_CC_ReturnType On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE.
	 *
	 *	@remarks Remarks
	 */
	virtual ACS_CC_ReturnType complete(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId);

	/** @brief abort method
	 *
	 *	This method will be called as a callback when a Configuration Change Bundle
	 *	has aborted. This method is called only if at least one complete method failed.
	 *
	 *
	 *  @return ACS_CC_ReturnType On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE.
	 *
	 *	@remarks Remarks
	 */
	virtual void abort(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId);

	/** @brief apply method
	 *
	 *	This method will be called as a callback when a Configuration Change Bundle is complete and can be applied.
	 *	This method is called only if all the complete method have been successfully executed.
	 *
	 *  @return ACS_CC_ReturnType On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE.
	 *
	 *	@remarks Remarks
	 */
	virtual void apply(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId);

	/** @brief updateRuntime method
	 *
	 * 	This method will be called as a callback when modifying a runtime not-cached attribute of a configuration Object
	 * 	for which the Application has registered as Object Implementer.
	 *
	 *  @return ACS_CC_ReturnType On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE.
	 *
	 *	@remarks Remarks
	*/
	virtual ACS_CC_ReturnType updateRuntime(const char* p_objName, const char** p_attrName);

	/**
	 * @brief adminOperationCallback method
	 * adminOperationCallback method: This method will be called as a callback to manage an administrative operation invoked, on the
	 * backup object.
	 *
	*/
	virtual void adminOperationCallback(ACS_APGCC_OiHandle oiHandle,ACS_APGCC_InvocationType invocation, const char* p_objName, ACS_APGCC_AdminOperationIdType operationId,  ACS_APGCC_AdminOperationParamType**paramList);

	/** @brief
	 *
	 * 	This method returns backup state.
	 *
	 *  @return true when a backup is ongoing, otherwise false
	 *
	 *  @remarks
	 */
	bool isBackupOnGoing() { return m_BackupState; };

 private:

	/** @brief	backupAction
	 *
	 *  Enum of defined operations on the CpVolume class element
	 *
	 */
	enum backupAction{
		// new BRF values
		PERMIT_BACKUP2 = 0,
		PREPARE_BACKUP2 = 1,
		COMMIT_BACKUP2 = 3,
		CANCEL_BACKUP2 = 4,
	};

	/**
	 * 	@brief	Constructor of FMS_CPF_OI_BackupRestore class
	*/
	FMS_CPF_OI_BackupRestore();

	/**
	 * 	@brief	Destructor of FMS_CPF_OI_BackupRestore class
	*/
	virtual ~FMS_CPF_OI_BackupRestore();

	/**
	 * 	@brief	replyToBRF
	 *  This method replies to the BRF callback
	*/
	void replyToBRF(unsigned long long& requestId, backupAction operationID);
	/**
	 * 	@brief	Flag to store the backup state
	*/
	bool m_BackupState;

	/**		@brief	trautil object trace
	*/
	ACS_TRA_trace* m_trace;

};

typedef ACE_Singleton<FMS_CPF_OI_BackupRestore, ACE_Recursive_Thread_Mutex> BackupHndl;

#endif /* FMS_CPF_OI_BACKUPRESTORE_H_ */
