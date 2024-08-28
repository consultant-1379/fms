/*
 * * @file fms_cpf_cpdopenfilesmgr.h
 *	@brief
 *	Header file for FMS_CPF_CpdOpenFilesMgr class.
 *  This module contains the declaration of the class FMS_CPF_CpdOpenFilesMgr.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-18
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
 *	| 1.0.0  | 2011-11-18 | qvincon      | File imported.                      |
 *	+========+============+==============+=====================================+
 *	| 1.1.0  | 2011-11-18 | qvincon      | ACE introduced.                     |
 *	+========+============+==============+=====================================+
 */
/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_CPDOPENFILESMGR_H_
#define FMS_CPF_CPDOPENFILESMGR_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_fileid.h"

#include <ace/Thread_Mutex.h>
#include <ace/Singleton.h>

#include <string>
#include <map>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class CPDOpenFiles;
class CPDFileThrd;
class FileLock;
class ACS_TRA_trace;
/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_CpdOpenFilesMgr
{
 public:

	friend class ACE_Singleton<FMS_CPF_CpdOpenFilesMgr, ACE_Recursive_Thread_Mutex>;

	/** @brief lookup method
	 *
	 *	This method returns a CPDFileThrd object given a file reference and the CP name
	 *
	 *	@param fileRef : the file reference
	 *
	 *	@param cpName : the CP name
	 *
	*/
	CPDFileThrd* lookup(const uint32_t fileRef, const std::string& cpName);

	/** @brief lockOpen method
	 *
	 *	This method returns a FileLock object given a file ID and the CP name
	 *
	 *	@param fileId : the file ID
	 *
	 *	@param cpName : the CP name
	 *
	*/
	FileLock* lockOpen(const std::string fileId, const std::string& cpName);

	/** @brief unlockOpen method
	 *
	 *	This method decrease the lock users of a FileLock object
	 *
	 *	@param openLock : Pointer to the FileLock object
	 *
	 *	@param cpName : the CP name
	 *
	*/
	void unlockOpen(FileLock* openLock, const std::string& cpName);

	/** @brief insert method
	 *
	 *	This method inserts a new entry in the open files
	 *
	 *	@param fileRef : the file reference
	 *	@param fThrd : Pointer to the CPDFileThrd object
	 *	@param cpName : the CP name
	 *	@param cpSide : the CP side EX/SB
	 *
	*/
	void insert(const uint32_t fileRef, CPDFileThrd* fThrd, const std::string& cpName, bool cpSide);

	/** @brief remove method
	 *
	 *	This method removes a entry from the open files
	 *
	 *	@param fileRef : the file reference
	 *	@param removeReference : indicates if remove or not the file reference
	 *	@param cpName : the CP name
	 *
	*/
	void remove(const uint32_t fileRef, bool notRemoveReference, const std::string& cpName);

	/** @brief remove method
	 *
	 *	This method removes a all entry from the open files
	 *
	*	@param cpSide : the CP side EX/SB
	 *	@param cpName : the CP name
	 *
	*/
	void removeAll(bool cpSide, const std::string& cpName);

	/**
	 *	@brief shutDown
	 *
	 *	This method terminate all cp file threads
	*/
	void shutDown();

 private:

	/**
	 * 	@brief	Constructor of FMS_CPF_CpdOpenFilesMgr class
	*/
	FMS_CPF_CpdOpenFilesMgr();

	/**
	 * 	@brief	Destructor of FMS_CPF_CpdOpenFilesMgr class
	*/
	virtual ~FMS_CPF_CpdOpenFilesMgr();

	/** @brief getCPDOpenFilesForCP method
	 *
	 *	This method returns a CPDOpenFiles object given a CP name
	 *
	 *	@param cpName : the CP name
	 *
	*/
	CPDOpenFiles* getCPDOpenFilesForCP(const std::string cpName);

	typedef std::map<std::string, CPDOpenFiles*> cpdOpenFilesMapType;

	/**
	 * 	@brief	m_CpdOpenFilesTable, It holds CPDOpenFile instance per CP name
	*/
	cpdOpenFilesMapType	m_CpdOpenFilesTable;

	/**
	 * 	@brief	m_mutex
	 *
	 * 	Mutex for internal map access
	*/
	ACE_Thread_Mutex m_mutex;

	/**
	 * 	@brief	fms_cpf_OpensMgrTrace: trace object
	*/
	ACS_TRA_trace* fms_cpf_OpensMgrTrace;

};

typedef ACE_Singleton<FMS_CPF_CpdOpenFilesMgr, ACE_Recursive_Thread_Mutex> CPDOpenFilesMgr;

#endif //FMS_CPF_CPDOPENFILESMGR_H_
