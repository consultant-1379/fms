/*
 * * @file fms_cpf_tqchecker.h
 *	@brief
 *	Header file for FMS_CPF_TQChecker class.
 *  This module contains the declaration of the class FMS_CPF_TQChecker.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-03-09
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
 *	| 1.0.0  | 2012-03-09 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_TQCHECKER_H_
#define FMS_CPF_TQCHECKER_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include <ace/Singleton.h>
#include <ace/Synch.h>
#include <ace/RW_Thread_Mutex.h>

#include <string>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */

class ACS_TRA_trace;
class FMS_CPF_FileTQChecker;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_TQChecker
{
 public:

	friend class ACE_Singleton<FMS_CPF_TQChecker, ACE_Recursive_Thread_Mutex>;

	/** @brief validateFileTQ method
	 *
	 * 	This method validate a block TQ
	 *
	 * 	@param tqName : Transfer queue name
	 *
	 *  @param errorCode : error code returned from OHI
	 *
	 *  @return bool : true on block TQ defined, otherwise false.
	 *
	 *  @remarks Remarks
	*/
	bool validateBlockTQ(const std::string& tqName, int& errorCode);

	/** @brief validateFileTQ method
	 *
	 * 	This method validate a block TQ
	 *
	 * 	@param tqName : Transfer queue name
	 *
	 * 	@param tqDN : Transfer queue DN
	 *
	 *  @param errorCode : error code returned from OHI
	 *
	 *  @return bool : true on block TQ defined, otherwise false.
	 *
	 *  @remarks Remarks
	*/
	bool validateBlockTQ(const std::string& tqName, std::string& tqDN, int& errorCode);

	/** @brief validateFileTQ method
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
	bool validateFileTQ(const std::string& tqName, int& errorCode);

	/** @brief validateFileTQ method
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
	bool validateFileTQ(const std::string& tqName, std::string& tqDN, int& errorCode);

	/** @brief removeSentFile
	 *
	 * 	This method remove a file from a given file destination.
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

	/** @brief checkTQName method
	 *
	 * 	This method checks a TQ name
	 *
	 * 	@param tqName : Transfer queue name
	 *
	 *  @return bool : true on valid TQ name, otherwise false.
	 *
	 *  @remarks Remarks
	*/
	bool checkTQName(const std::string& tqName);

	/** @brief createTQFolder method
	 *
	 *	This method creates the folder of TQ
	 *
	 *	@param tqName : Transfer queue name
	 *
	 *  @return bool : true on success, otherwise false.
	 *
	*/
	bool createTQFolder(const std::string& tqName );

	/** @brief createTQFolder method
	 *
	 *	This method creates the folder of TQ
	 *
	 *	@param tqName : Transfer queue name
	 *	@param folderPath : TQ folder path
	 *
	 *  @return bool : true on success, otherwise false.
	 *
	*/
	bool createTQFolder(const std::string& tqName, std::string& folderPath );

 private:

	/**
	 * 	@brief	Constructor of FMS_CPF_TQChecker class
	*/
	FMS_CPF_TQChecker();

	/**
	 * 	@brief	Destructor of FMS_CPF_TQChecker class
	*/
	virtual ~FMS_CPF_TQChecker();

	/** @brief createFolder method
	 *
	 *	This method creates a folder
	 *
	 *	@param fileTQRoot : root of the file TQ
	 *
	 *  @return bool : true on success, otherwise false.
	 *
	*/
	bool getFileMTQPath(std::string& fileTQRoot);

	/** @brief createFolder method
	 *
	 *	This method creates a folder
	 *
	 *	@param folderPath : path and folder to create
	 *
	 *  @return bool : true on success, otherwise false.
	 *
	*/
	bool createFolder(const std::string& folderPath);

	/**
	 * 	@brief	m_fileTQChk : file transfer queue object handler for check
	*/
	FMS_CPF_FileTQChecker* m_fileTQChk;

	/**
	 * 	@brief	m_fileTQRoot : root of the file TQ
	*/
	std::string m_fileTQRoot;

	/**
	 * 	@brief	m_mutex
	 *
	 * 	Mutex for internal sync
	*/
	ACE_RW_Thread_Mutex m_mutex;

	/**
	 * 	@brief	m_trace: trace object
	*/
	ACS_TRA_trace* m_trace;
};

typedef ACE_Singleton<FMS_CPF_TQChecker, ACE_Recursive_Thread_Mutex> TQChecker;

#endif /* FMS_CPF_TQCHECKER_H_ */
