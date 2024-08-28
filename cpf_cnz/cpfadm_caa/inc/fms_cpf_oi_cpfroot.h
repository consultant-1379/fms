/* * @file fms_cpf_oi_cpfroot.h
 *	@brief
 *	Header file for FMS_CPF_OI_CPFRoot class.
 *  This module contains the declaration of the class FMS_CPF_OI_CPFRoot.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-10-03
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
 *	| 1.0.0  | 2012-10-03 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_OI_CPFROOT_H_
#define FMS_CPF_OI_CPFROOT_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_oi_baseobject.h"

#include "fms_cpf_types.h"

#include "acs_apgcc_objectimplementerinterface_V3.h"

#include <vector>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class ACS_TRA_trace;

/*===================================================================
                        CLASS DECLARATION SECTION
=================================================================== */

class FMS_CPF_OI_CPFRoot : public FMS_CPF_OI_BaseObject, public acs_apgcc_objectimplementerinterface_V3
{
 public:

	/**		@brief	constructor of FMS_CPF_OI_CPFRoot class
	*/
	FMS_CPF_OI_CPFRoot(FMS_CPF_CmdHandler* cmdHandler);

	/**		@brief	destructor of FMS_CPF_OI_CPFRoot class
	*/
	virtual ~FMS_CPF_OI_CPFRoot();

	/** @brief create method
	 *
	 *	This method will be called as a callback when an Object is created as instance of a Class CpFileSystemM
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
	 *	This method will be called as a callback when deleting a CpFileSystemM Object
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
	 *	This method will be called as a callback when modifying a CpFileSystemM Object
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
	 *	regarding a CpFileSystemM Object
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
	 *	This method will be called as a callback when a Configuration Change Bundle, regarding a CpFileSystemM Object,
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
	 *	This method will be called as a callback when a Configuration Change Bundle, regarding a CpFileSystemM Object, is complete and can be applied.
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
	 * CpFileSystemM object.
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

	 /** @brief
	 *
	 *  Enum of defined operations on the CpFileSystem class element
	 *
	 */
	enum definedOperations{
		importCpFile = 1,
		exportCpFile,
		copyCpFile,
		moveCpFile,
		renameCpFile,
		importCpClusterFile,
		exportCpClusterFile,
		copyCpClusterFile,
		moveCpClusterFile,
		renameCpClusterFile
	};

	/**
	 * @brief getOperationParameter method
	 * getOperationParameter method: This method will be called to retrieve an action parameter
	 *
	 * @param  parValue: bool parameter value
	 *
	 * @param  parName : parameter name to find
	 *
	 * @param paramList: a null terminated array of pointers to operation params elements. each element of the list
	 * is a pointer toACS_APGCC_AdminOperationParamType element holding the params provided to the Administretive operation..
	 *
	 * @return a bool. On success true on Failure false
	 */
	bool getOperationParameter(std::string& parValue, const char* parName, ACS_APGCC_AdminOperationParamType** paramList);

	/**
	 * @brief getOperationParameter method
	 * getOperationParameter method: This method will be called to retrieve an action parameter
	 *
	 * @param  parValue: string parameter value
	 *
	 * @param  parName : parameter name to find
	 *
	 * @param paramList: a null terminated array of pointers to operation params elements. each element of the list
	 * is a pointer toACS_APGCC_AdminOperationParamType element holding the params provided to the Administretive operation..
	 *
	 * @return a bool. On success true on Failure false
	 */
	bool getOperationParameter(bool& parValue, const char* parName, ACS_APGCC_AdminOperationParamType** paramList);

	/**
	 * @brief getOperationParameter method
	 * getOperationParameter method: This method will be called to retrieve an action parameter
	 *
	 * @param  parValue: FMS_CPF_Types::copyMode parameter value
	 *
	 * @param  parName : parameter name to find
	 *
	 * @param paramList: a null terminated array of pointers to operation params elements. each element of the list
	 * is a pointer toACS_APGCC_AdminOperationParamType element holding the params provided to the Administretive operation..
	 *
	 */
	void getOperationParameter(FMS_CPF_Types::copyMode& parValue, const char* parName, ACS_APGCC_AdminOperationParamType** paramList);

	/**
	 * @brief replyToActionCall method
	 * replyToActionCall method: This method will be called to reply to the action callback
	 *
	 * @param  oiHandle : input parameter,ACS_APGCC_OiHandle this value has to be used for returning the callback
	 *  result to IMM.
	 *
	 * @param invocation: input parameter,the invocation id used to match the invocation of the callback with the invocation
	 * of result function
	 *
	 * @param  errorCode: action result value as integer
	 *
	 * @param  errorMsg: action error message as string
	 *
	 * @param  errorComCliMsg: action error message to show as string
	 *
	*/
	void replyToActionCall(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_InvocationType invocation, const int errorCode, const char* errorMsg, const char* errorComCliMsg);


	/**
	 * @brief replyToActionCall method
	 * replyToActionCall method: This method will be called to reply to the action callback
	 *
	 * @param  oiHandle : input parameter,ACS_APGCC_OiHandle this value has to be used for returning the callback
	 *  result to IMM.
	 *
	 * @param invocation: input parameter,the invocation id used to match the invocation of the callback with the invocation
	 * of result function
	 *
	 * @param  errorCode: action result value as int
	 *
	*/
	void replyToActionCall(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_InvocationType invocation, const int errorCode);

	/**
	 * @brief addActionResult method
	 * addActionResult method: This method will be called to add an string result to the action output parameters
	 *
	 * @param  attrName: attribute name
	 *
	 * @param  message: message string value
	 *
	 * @param  outParameteres : vector of parameters result
	 *
	*/
	void addActionResult(char* attrName, char* message, std::vector<ACS_APGCC_AdminOperationParamType>& outParameteres);

	/**
	 * @brief getFileMCpFilePath method
	 * getFileMCpFilePath method: This method will be called to retrieve the Cp files path
	 *
	 * @param  path: FileM defined Cp files area
	 *
	 */
	void getFileMCpFilePath(std::string& path);

	/**
	 * @brief getAbsolutePath method
	 * getAbsolutePath method: This method will be called to assemble the absolute import/export path
	 *
	 * @param  relativePath: relative path
	 *
	 * @param  absolutePath: the full path
	 *
	 */
	bool getAbsolutePath(const std::string& relativePath, std::string& absolutePath);

	/** @brief	m_ImportExportPath
	 *
	 *  Import/Export area defined in FileM
	 */
	std::string m_ImportExportPath;

	/**	@brief	m_trace
	*/
	ACS_TRA_trace* m_trace;

};

#endif /* FMS_CPF_OI_CPFROOT_H_ */
