/*
 * * @file fms_cpf_oi_baseobject.h
 *	@brief
 *	Header file for FMS_CPF_OI_BaseObject class.
 *  This module contains the declaration of the class FMS_CPF_OI_BaseObject.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-30
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
 *	| 1.0.0  | 2011-06-30 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_OI_BASEOBJECT_H_
#define FMS_CPF_OI_BASEOBJECT_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_cmdhandler.h"

#include <string>
#include <map>
#include <vector>

typedef std::map<std::string, short> map_CPname_Id;

class ACS_TRA_trace;
class CPF_DeleteFile_Request;

/*===================================================================
                        CLASS DECLARATION SECTION
=================================================================== */
class FMS_CPF_OI_BaseObject
{
 public:

	/**		@brief	constructor of FMS_CPF_OI_BaseObject class
	 *
	*/
	FMS_CPF_OI_BaseObject(FMS_CPF_CmdHandler* cmdHandler, std::string ImmClassName);

	/**		@brief	destructor of FMS_CPF_OI_BaseObject class
	 *
	*/
	virtual ~FMS_CPF_OI_BaseObject();

	/** @brief getClassName method
	 *
	 * 	This method return the name of the IMM class.
	 *
	 *  @return const char pointer of the IMM class name
	 *
	 *  @remarks Remarks
	 */
	const char* getIMMClassName() const {return m_ImmClassName.c_str();};

	/** @brief setSystemParameters method
	 *
	 * 	This method copies the system parameters
	 *
	 * 	@param systemType : Flag of system type (SCP/MCP)
	 *
	 * 	@param mapConfig : a map of defined CP/BC name and their Id
	 *
	 *  @remarks Remarks
	 */
	void setSystemParameters(const bool systemType, const map_CPname_Id& mapConfig );

	/** @brief	getDNbyTag
	 *
	 *  This method gets the DN of a object by its RDN
	 *
	 *  @param fullDN : A full DN
	 *
	 *  @param tagOfDN : The RDN
	 *
	 *  @param outDN :  The DN of the object
	 *
	 *  @return true on success, otherwise false
	 *
	 *  @remarks Remarks
	 */
	bool getDNbyTag(const std::string& fullDN, const char* tagOfDN, std::string& outDN);

	/** @brief	checkCpName
	 *
	 *  This method validate the Cp name
	 *
	 *  @param cpName : The CP/BC name
	 *
	 *  @return SUCCESS if the CP/BC name is defined, otherwise error code
	 *
	 *  @remarks Remarks
	 */
	int checkCpName(const std::string& cpName);

	/** @brief	getVolumeName
	 *
	 *  This method extracts the volume name from the DN
	 *
	 *  @param parentName : the DN of the parent object
	 *
	 *  @param volName : the volume name
	 *
	 *  @return true on success, false otherwise
	 *
	 *  @remarks Remarks
	 */
	bool getVolumeName(std::string parentName,std::string& volName);

	/** @brief	getCpName
	 *
	 *  This method extracts the CP/BC name from the DN
	 *
	 *  @param parentName : the DN of the parent object
	 *
	 *  @param cpName : the Cp name in a MCP system, empty string in a SCP system
	 *
	 *  @return true on success, false otherwise
	 *
	 *  @remarks Remarks
	 */
	bool getCpName(std::string parentName, std::string& cpName);

	/** @brief	getSimpleFilename
	 *
	 *  This method extracts the simple file name from the DN
	 *
	 *  @param parentName : the DN of the parent object
	 *
	 *  @param simpleFileName : the simple File name
	 *
	 *  @return true on success, false otherwise
	 *
	 *  @remarks Remarks
	 */
	bool getSimpleFileName(std::string objDN, std::string& simpleFileName);

	/** @brief	getInfiniteFilename
	 *
	 *  This method extracts the infinite file name from the DN
	 *
	 *  @param parentName : the DN of the parent object
	 *
	 *  @param infiniteFileName : the infinite File name
	 *
	 *  @return true on success, false otherwise
	 *
	 *  @remarks Remarks
	 */
	bool getInfiniteFileName(std::string parentName, std::string& infiniteFileName);

	/** @brief	getInfiniteFilename
	 *
	 *  This method extracts the infinite subfile name from the DN
	 *
	 *  @param parentName : the DN of the parent object
	 *
	 *  @param infiniteSubFileName : the infinite SubFile name
	 *
	 *  @return true on success, false otherwise
	 *
	 *  @remarks Remarks
	 */
	bool getSubInfiniteFileName(std::string parentName, std::string& infiniteSubFileName);

	/** @brief	getCompositeFilename
	 *
	 *  This method extracts the composite file name from the DN
	 *
	 *  @param parentName : the DN of the parent object
	 *
	 *  @param compositeFileName : the composite File name
	 *
	 *  @return true on success, false otherwise
	 *
	 *  @remarks Remarks
	 */
	bool getCompositeFileName(std::string parentName, std::string& compositeFileName);

	/** @brief	getSubCompositeFileName
	 *
	 *  This method extracts the sub composite file name from the DN
	 *
	 *  @param objDN : the DN of the object
	 *
	 *  @param subCompositeFileName : the sub composite File name
	 *
	 *  @return true on success, false otherwise
	 *
	 *  @remarks Remarks
	 */
	bool getSubCompositeFileName(std::string objDN, std::string& subCompositeFileName);

	/** @brief	getLastFieldValue
	 *
	 *  This method extracts the last field value of a DN
	 *
	 *  @param objDN : the DN of the object
	 *
	 *  @param value : the field value
	 *
	 *  @return true on success, false otherwise
	 *
	 *  @remarks Remarks
	 */
	bool getLastFieldValue(const std::string& objDN, std::string& value, bool toUpperValue = true);

	/** @brief	getCpId
	 *
	 *  This method retrieve the CpId from the CpName
	 *
	 *  @param cpName : the CP Name
	 *
	 *  @return the CP Id
	 *
	 *  @remarks Remarks
	 */
	short getCpId(const std::string& cpName);

	/** @brief	checkVolumeName
	 *
	 *  This method validate the volume name
	 *
	 *  @param vName : the volume name
	 *
	 *  @return true if the name is valid, otherwise false
	 *
	 *  @remarks Remarks
	 */
	bool checkVolumeName(const std::string& vName);

	/** @brief	checkFileName
	 *
	 *  This method validate the file name
	 *
	 *  @param fName : the file name
	 *
	 *  @return true if the name is valid, otherwise false
	 *
	 *  @remarks Remarks
	 */
	bool checkFileName(const std::string& fName);

	/** @brief	checkSubFileName
	 *
	 *  This method validate the file name
	 *
	 *  @param fName : the file name
	 *
	 *  @return true if the name is valid, otherwise false
	 *
	 *  @remarks Remarks
	 */
	bool checkSubFileName(const std::string& fName);

	/** @brief	checkRecordLength
	 *
	 *  This method validate the file record length
	 *
	 *  @param recordLength : the record length
	 *
	 *  @return true if the record length is valid, otherwise false
	 *
	 *  @remarks Remarks
	 */
	bool checkRecordLength(const int& recordLength);

	enum {
		RequestFailure = -1,
		RequestSuccess = 0
	};

	enum ImmAction {
		Create = 1,
		Delete,
		Modify
	};
 protected:

	/** @brief	getNumberOfChildren
	 *
	 *  This method gets the number of children of an object
	 *
	 *  @param objectDN : the object DN
	 *
	 *  @return -1 on error, otherwise number of children
	 *
	 *  @remarks Remarks
	*/
	long getNumberOfChildren(const char* objectDN);

	/** @brief m_deleteOperationList
	 *
	 * List of enqueued delete operation
	 *
	*/
	std::vector<CPF_DeleteFile_Request*> m_deleteOperationList;

	/** @brief m_ImmClassName
	 *
	 * The name of the IMM class.
	 *
	 */
	std::string m_ImmClassName;

	/**
	 * 	@brief	m_IsMultiCP
	 *
	 * 	Flag of system type (SCP/MCP)
	*/
	bool m_IsMultiCP;

	/**	@brief	m_mapCpNameId
	 *
	 * A map of defined CP/BC name and their Id
	*/
	map_CPname_Id m_mapCpNameId;

	/**	@brief	m_CmdHandler
	 *
	 * Pointer of the command request handler
	*/
	FMS_CPF_CmdHandler* m_CmdHandler;

 private:

	/** @brief	getStringAttributeValue
	 *
	 *  This method extracts an attribute value of an object from the DN
	 *
	 *  @param objectDN : the DN of the object
	 *
	 *  @param attributeName : the attribute name
	 *
	 *  @param attributeValue: the attribute value retrieved
	 *
	 *  @return true on success, false otherwise
	 *
	 *  @remarks Remarks
	 */
	bool getStringAttributeValue(const std::string& objectDN, char* attributeName, std::string& attributeValue);

	/**		@brief	fms_cpf_oi_compFileTrace
	*/
	ACS_TRA_trace* fms_cpf_oi_baseObjTrace;
};

#endif /* FMS_CPF_OI_BASEOBJECT_H_ */
