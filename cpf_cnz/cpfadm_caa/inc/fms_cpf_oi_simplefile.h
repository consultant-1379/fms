/*
 * * @file fms_cpf_oi_compositefile.h
 *	@brief
 *	Header file for FMS_CPF_OI_SimpleFile class.
 *  This module contains the declaration of the class FMS_CPF_OI_SimpleFile.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-02-13
 *	@version 1.0.0
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
 *	| 1.0.0  | 2012-01-13 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_OI_SIMPLEFILE_H_
#define FMS_CPF_OI_SIMPLEFILE_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_oi_baseobject.h"

#include "acs_apgcc_objectimplementerinterface_V3.h"

/*===================================================================
                        CLASS FORWARD SECTION
=================================================================== */
class ACS_TRA_trace;

/*===================================================================
                        CLASS DECLARATION SECTION
=================================================================== */
class FMS_CPF_OI_SimpleFile : public FMS_CPF_OI_BaseObject, public acs_apgcc_objectimplementerinterface_V3
{
 public:

		/**		@brief	constructor of FMS_CPF_OI_SimpleFile class
		*/
		FMS_CPF_OI_SimpleFile(FMS_CPF_CmdHandler* cmdHandler);

		/**		@brief	destructor of FMS_CPF_OI_SimpleFile class
		*/
		virtual ~FMS_CPF_OI_SimpleFile();

		/** @brief create method
		 *
		 *	This method will be called as a callback when an Object is created as instance of a Class SimpleFile
		 *	All input parameters are input provided by IMMSV Application and have to be used by the implementer to perform
		 *	proper actions.
		 *
		 *  @param oiHandle: the object implementer handle. This is an Input Parameter provided by IMMSV Application.
		 *
		 *	@param ccbId: the ID for the Configuration Change Bundle in which the creation of the Object is contained.
		 *	This is an Input Parameter provided by IMMSV Application.
		 *
		 *	@param className: the name of the class. When an object is created as instance of this class this method is
		 *	called if the application has registered as class implementer. This is an Input Parameter provided by IMMSV Application.
		 *
		 *	@param parentname: the name of the parent object for the object now creating.
		 *	This is an Input Parameter provided by IMMSV Application.
		 *
		 *	@param attr: a pointer to a null terminated array of ACS_APGCC_AttrValues element pointers each one containing
		 *	the info about the attributes belonging to the now creating class.
		 *	This is an Input Parameter provided by IMMSV Application.
		 *
		 *  @return ACS_CC_ReturnType On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE.
		 */
		virtual ACS_CC_ReturnType create(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *className, const char* parentname, ACS_APGCC_AttrValues **attr);

		/** @brief deleted method
		 *
		 *	This method will be called as a callback when deleting a SimpleFile Object
		 *	Object Implementer. All input parameters are input provided by IMMSV Application and have to be used by
		 *	the implementer to perform proper actions.
		 *
		 *  @param oiHandle: the object implementer handle. This is an Input Parameter provided by IMMSV Application.
		 *
		 *	@param ccbId: the ID for the Configuration Change Bundle in which the deletion of the Object is contained.
		 *	This is an Input Parameter provided by IMMSV Application.
		 *
		 *	@param objName: the Distinguished name of the object that has to be deleted.
		 *	This is an Input Parameter provided by IMMSV Application.
		 *
		 *  @return ACS_CC_ReturnType On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE.
		 *
		 *	@remarks Remarks
		 */
		virtual ACS_CC_ReturnType deleted(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName);

		/** @brief modify method
		 *
		 *	This method will be called as a callback when modifying a SimpleFile Object
		 *	All input parameters are input provided by IMMSV Application and have to be used by the implementer to perform
		 *	proper actions.
		 *
		 *  @param oiHandle: the object implementer handle. This is an Input Parameter provided by IMMSV Application.
		 *
		 *	@param ccbId: the ID for the Configuration Change Bundle in which the modify of the Object is contained.
		 *	This is an Input Parameter provided by IMMSV Application.
		 *
		 *	@param objName: the Distinguished name of the object that has to be modified.
		 *	This is an Input Parameter provided by IMMSV Application.
		 *
		 *	@param attrMods: a NULL terminated array of pointers to ACS_APGCC_AttrModification elements containing
		 *	the information about the modify to perform. This is an Input Parameter provided by IMMSV Application.
		 *
		 *  @return ACS_CC_ReturnType On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE.
		 *
		 *	@remarks Remarks
		 */
		virtual ACS_CC_ReturnType modify(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName, ACS_APGCC_AttrModification **attrMods);

		/** @brief complete method
		 *
		 *	This method will be called as a callback when a Configuration Change Bundle is complete and can be applied
		 *	regarding a SimpleFile Object
		 *
		 *  @param oiHandle: the object implementer handle. This is an Input Parameter provided by IMMSV Application.
		 *
		 *	@param ccbId: the ID for the Configuration Change Bundle in which the modify of the Object is contained.
		 *	This is an Input Parameter provided by IMMSV Application.
		 *
		 *  @return ACS_CC_ReturnType On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE.
		 *
		 *	@remarks Remarks
		 */
		virtual ACS_CC_ReturnType complete(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId);

		/** @brief abort method
		 *
		 *	This method will be called as a callback when a Configuration Change Bundle, regarding a SimpleFile Object,
		 *	has aborted. This method is called only if at least one complete method failed.
		 *
		 *  @param oiHandle: the object implementer handle. This is an Input Parameter provided by IMMSV Application.
		 *
		 *	@param ccbId: the ID for the Configuration Change Bundle containing actions on Objects for which the Application
		 *	registered as Object Implementer. This is an Input Parameter provided by IMMSV Application.
		 *
		 *  @return ACS_CC_ReturnType On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE.
		 *
		 *	@remarks Remarks
		 */
		virtual void abort(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId);

		/** @brief apply method
		 *
		 *	This method will be called as a callback when a Configuration Change Bundle, regarding a SimpleFile Object, is complete and can be applied.
		 *	This method is called only if all the complete method have been successfully executed.
		 *
		 *  @param oiHandle: the object implementer handle. This is an Input Parameter provided by IMMSV Application.
		 *
		 *	@param ccbId: the ID for the Configuration Change Bundle containing actions on Objects for which the Application
		 *	registered as Object Implementer. This is an Input Parameter provided by IMMSV Application.
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
		 * 	All input parameters are input provided by IMMSV Application and have to be used by the implementer
		 * 	to perform proper actions.
		 *
		 *  @param p_objName: the Distinguished name of the object that has to be modified.
		 *  				  This is an Input Parameter provided by IMMSV Application.
		 *
		 *  @param p_attrName: the name of attribute that has to be modified.
		 *  				   This is an Input Parameter provided by IMMSV Application.
		 *
		 *  @return ACS_CC_ReturnType On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE.
		 *
		 *	@remarks Remarks
		*/
		virtual ACS_CC_ReturnType updateRuntime(const char* p_objName, const char** p_attrName);

		/**
		 * @brief adminOperationCallback method
		 * adminOperationCallback method: This method will be called as a callback to manage an administrative operation invoked, on the
		 * Simple CP File object.
		 *
		 * @param  p_objName:	the distinguished name of the object for which the administrative operation
		 * has to be managed.
		 *
		 * @param  oiHandle : input parameter,ACS_APGCC_OiHandle this value has to be used for returning the callback
		 *  result to IMM.
		 *
		 * @param invocation: input parameter,the invocation id used to match the invocation of the callback with the invocation
		 * of result function
		 *
		 * @param  p_objName: input parameter,the name of the object
		 *
		 * @param  operationId: input parameter, the administrative operation identifier
		 *
		 * @param paramList: a null terminated array of pointers to operation params elements. each element of the list
		 * is a pointer toACS_APGCC_AdminOperationParamType element holding the params provided to the Administretive operation..
		 *
		 * @return ACS_CC_ReturnType. On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE
		*/
		virtual void adminOperationCallback(ACS_APGCC_OiHandle oiHandle,ACS_APGCC_InvocationType invocation, const char* p_objName, ACS_APGCC_AdminOperationIdType operationId,  ACS_APGCC_AdminOperationParamType** paramList);

 private:

		/** @brief	compositeFileInfo
		 *
		 *  Struct of IMM simpleFileInfo class elements
		 *
		*/
		struct simpleFileInfo{
				std::string cpName;
				std::string volumeName;
				std::string fileName;
				std::string fileDN;
				int recordLength;
				bool compression;
				bool completed;
				ImmAction action;
				bool newCompressionValue;
				std::string newFileNameValue;
		};

		typedef std::multimap<ACS_APGCC_CcbId, simpleFileInfo> operationTable;

		/** @brief	m_simpleFileOperationTable
		 *
		 *  Map of pending SimpleFile Operations
		 *
		 */
		operationTable m_simpleFileOperationTable;

		/**		@brief	fms_cpf_oi_simpleFileTrace
		*/
		ACS_TRA_trace* fms_cpf_oi_simpleFileTrace;
};

#endif /* FMS_CPF_OI_SIMPLEFILE_H_ */
