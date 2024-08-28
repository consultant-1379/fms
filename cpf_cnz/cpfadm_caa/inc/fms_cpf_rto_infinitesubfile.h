/*
 * * @file fms_cpf_rto_infinitesubfile.h
 *	@brief
 *	Header file for FMS_CPF_RTO_InfiniteSubFile class.
 *  This module contains the declaration of the class FMS_CPF_RTO_InfiniteSubFile.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-10-15
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
 *	| 1.0.0  | 2011-10-15 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_RTO_INFINITESUBFILE_H_
#define FMS_CPF_RTO_INFINITESUBFILE_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_oi_baseobject.h"
#include "ACS_APGCC_RuntimeOwner_V2.h"

#include <ace/Thread_Mutex.h>

#include <string>
#include <map>
#include <set>


/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class FMS_CPF_CmdHandler;
class OmHandler;
class ACS_TRA_trace;


/*===================================================================
                        CLASS DECLARATION SECTION
=================================================================== */
class FMS_CPF_RTO_InfiniteSubFile: public FMS_CPF_OI_BaseObject, public ACS_APGCC_RuntimeOwner_V2
{
 public:

	enum operationResult
	{
		ResetAndRetry = -1,
		OK = 0,
		Retry = 1
	};

	friend class FMS_CPF_InfiniteSubFiles_Manager;

	/**	@brief	constructor of FMS_CPF_RTO_InfiniteSubFile class
	 */
    FMS_CPF_RTO_InfiniteSubFile(FMS_CPF_CmdHandler* m_CmdHandler);

	/**	@brief	destructor of FMS_CPF_RTO_InfiniteSubFile class
	 */
    virtual ~FMS_CPF_RTO_InfiniteSubFile();

    /** @brief createISF method
	 *
	 * 	This method will be called to create a runtime infinite subfile object.
	 *
	 *  @param implName: input parameter, runtime owner name.
	 *
	 *  @return bool. On success true on Failure false.
	 *
	 */
    bool registerToImm(bool systemType, const std::string& implName);

    /** @brief updateInitialState method
	 *
	 * 	This method will be called at start-up to check consistency between
	 *  IMM data and physical files on data disk.
	 *
	 *  @param cpName: Cp file system to check.
	 *
	 */
    void updateInitialState(const std::string& cpName);

    /** @brief updateAttribute method
     *
     * 	This method will be called at start-up on restore to check consistency between
     *  physical files on data disk and file attribute value
     *
     *  @param cpName: Cp file system to check.
     *  @param fileName: Cp file to check.
     *  @param firstSubFile: First physical subfile.
     *  @param lastSubFile: Last physical subfile.
     *
     */
    void updateAttribute(const std::string& cpName, const std::string& fileName, const std::string& firstSubFile, const std::string& lastSubFile);

    /** @brief updateRuntime method
	 *
	 * 	This method will be called as a callback when a runtime not-cached attribute will be retrieved
	 * 	All input parameters are input provided by IMMSV Application and have to be used by the implementer
	 * 	to perform proper actions.
	 *
	 *  @param p_objName: the Distinguished name of the object that has to be modified.
	 *  				  This is an Input Parameter provided by IMMSV Application.
	 *
	 *  @param p_attrName: pointer to a null terminate array of attribute name for which values must be updated.
	 *
	 *  @return ACS_CC_ReturnType On success ACS_CC_SUCCESS on Failure ACS_CC_FAILURE.
	 *
	 *	@remarks Remarks
	 */
	ACS_CC_ReturnType updateCallback(const char* p_objName, const char** p_attrNames);

	/**
	 * @brief adminOperationCallback method
	 * adminOperationCallback method: This method will be called as a callback to manage an administrative operation invoked, on the
	 * infinite subFile object.
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
	void adminOperationCallback(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_InvocationType invocation, const char* p_objName, ACS_APGCC_AdminOperationIdType operationId,  ACS_APGCC_AdminOperationParamType**paramList);

 private:

	/** @brief getListOfISF method
	 *
	 * 	This method will be called to create a runtime infinite subfile object.
	 *
	 *  @param fileDN: input parameter, infinite file DN name
	 *
	 *  @param subFileList: output parameter, infinite subfile list
	 *
	 */
	void getListOfISF(const std::string& fileDN, std::set<std::string>& subFileList);

	/** @brief createISF method
	 *
	 * 	This method will be called to create a runtime infinite subfile object.
	 *
	 *  @param subFileValue: input parameter, infinite subfile name
	 *
	 *  @param fileName: input parameter, infinite file name
	 *
	 *  @param volume: input parameter, volume name
	 *
	 *  @param cpName: input parameter, cp name
	 */
	int createISF(const unsigned int& subFileValue, const std::string& fileName, const std::string& volume, const std::string& cpName);

        /** @brief deleteISF method
	 *
	 * 	This method will be called to delete a runtime infinite subfile object.
	 *
	 *  @param subFileValue: input parameter, infinite subfile name
	 *
	 *  @param fileName: input parameter, infinite file name
	 *
	 *  @param volume: input parameter, volume name
	 *
	 *  @param cpName: input parameter, cp name
	 */
	int deleteISF(const unsigned int& subFileValue, const std::string& fileName, const std::string& volume, const std::string& cpName);

	/** @brief deleteISF method
         *
         *      This method will be called to delete a runtime infinite subfile object.
         *
         *  @param subFileDN: input parameter, infinite subfile DN
         *
         */
	int deleteISF(const std::string& subFileDN);

    /** @brief getMapEntry method
	 *
	 * 	This method gets the DN of an infinite file.
	 *
	 *  @param fileName: input parameter, infinite file name
	 *
	 *  @param cpName: input parameter, cp name
	 *
	 *  @param fileDN: output parameter, DN of the infinite file
	 */
    bool getMapEntry(const std::string& fileName, const std::string& cpName, std::string& fileDN);

    /** @brief insertMapEntry method
	 *
	 * 	This method adds a DN of an infinite file in the internal DNs map.
	 *
	 *  @param fileName: input parameter, infinite file name
	 *
	 *  @param cpName: input parameter, cp name
	 *
	 *  @param fileDN: input parameter, DN of the infinite file
	 */
	void insertMapEntry(const std::string& fileName, const std::string& cpName, const std::string& fileDN);

	 /** @brief updateMapEntry method
	 *
	 * 	This method updates a entry  of internal DNs map.
	 *
	 *  @param fileName: input parameter, the old infinite file name
	 *
	 *  @param cpName: input parameter, cp name
	 *
	 *  @param newFileName: input parameter, the new infinite file name
	 *
	 *  @param fileDN: input parameter, DN of the infinite file
	 */
	void updateMapEntry(const std::string& fileName, const std::string& cpName, const std::string& newFileName, const std::string& fileDN);

	/** @brief removeMapEntry method
	 *
	 * 	This method removes a entry of internal DNs map.
	 *
	 *  @param fileName: input parameter, infinite file name
	 *
	 *  @param cpName: input parameter, cp name
	*/
	void removeMapEntry(const std::string& fileName, const std::string& cpName);

	/** @brief clearMap method
	 *
	 * 	This method removes all entry of internal DNs map.
	 *
	*/
	void clearMap();

	/** @brief getParentDN method
	 *
	 * 	This method retrieves the DN of the infinite file object.
	 *
	 *  @param fileName: input parameter, infinite file name
	 *
	 *  @param volume: input parameter, volume name
	 *
	 *  @param cpName: input parameter, cp name
	 *
	 *  @param parentDN: output parameter, parent DN
	 *
	 *  @return bool. On success true on Failure false
	 */
	bool getParentDN(const std::string& fileName, const std::string& volume, const std::string& cpName, std::string& parentDN);

	/** @brief getParentDN method
	 *
	 * 	This method assemble the DN of a cpVolume object.
	 *
	 *  @param volume: input parameter, volume name
	 *
	 *  @param cpName: input parameter, cp name
	 *
	 *  @param rootDN: output parameter, root DN
	 *
	 */
	void assembleRootDN(const std::string& volume, const std::string& cpName, std::string& rootDN);

	/** @brief getParentDN method
	 *
	 * 	This method assemble the DN of a cpVolume object.
	 *
	 *  @param fileName: input parameter, infinite file name
	 *
	 *  @param rootDN: input parameter, cp name
	 *
	 *  @param fileDN: output parameter, infinite file DN
	 *
	 */
	bool getInfiniteFileDN(const std::string& fileName, const std::string& rootDN, std::string& fileDN);

	typedef pair<std::string, std::string> mapKeyType;

	/**
	 * 	@brief	m_FileDNMap
	 *
	 * 	Internal map to handles the DN of infinite file
	 *
	 * 	map key : pair<fileName, CpName>
	 *
	 * 	map value : DN of infinite file
	*/
	std::map<mapKeyType, std::string> m_FileDNMap;

	/**
	 * 	@brief	m_mutex
	 *
	 * 	Mutex for internal map access
	*/
	ACE_Thread_Mutex m_mutex;

	/**
	 * 	@brief	m_implementerName : RT owner name
	*/
	std::string m_implementerName;

	/**
	 * 	@brief	OmHandler: Object Manager
	*/
	OmHandler* m_objManager;

	/**
	 * 	@brief	fms_cpf_roManTrace: trace object
	*/
	ACS_TRA_trace* fms_cpf_RTOTrace;

};

#endif /* FMS_CPF_RTO_INFINITESUBFILE_H_ */
