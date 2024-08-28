/*
 * * @file fms_cpf_cpopenfiles.h
 *	@brief
 *	Header file for CPDOpenFiles class.
 *  This module contains the declaration of the class CPDOpenFiles.
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
#ifndef FMS_CPF_CPDOPENFILES_H_
#define FMS_CPF_CPDOPENFILES_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_fileid.h"
#include "fms_cpf_privateexception.h"

#include <ace/Thread_Mutex.h>

#include <map>
#include <string>
#include <list>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */

class FMS_CPF_CpdOpenFilesMgr;
class CPDFileThrd;
class FileLock;
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPDOpenFiles
{
 public:

	friend class FMS_CPF_CpdOpenFilesMgr;

	/** @brief lookup method
	 *
	 *	This method lookup one item in the map
	 *
	 *	@param fileRef : the file reference
	 *
	*/
	CPDFileThrd* lookup(const uint32_t fileRef) throw(FMS_CPF_PrivateException);

	/** @brief lockOpen method
	 *
	 *	This method returns a FileLock object given a file ID and the CP name.
	 *	Only one file thread should enter open and close on the same file.
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
	*/
	void unlockOpen(FileLock* openLock);

	/** @brief insert method
	 *
	 *	This method inserts one item in the map
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
	 *	This method removes one item in the map
	 *
	 *	@param fileRef : the file reference
	 *	@param notRemoveReference : indicates if remove or not the file reference
	 *
	*/
	void remove(const uint32_t fileRef, bool notRemoveReference);

	/** @brief removeAll method
	 *
	 *	This method remove all items in the map
	 *
	 *	@param cpSide : the CP side EX/SB
	 *
	*/
	void removeAll(bool cpSide);

	/**
	 *	@brief shutDown
	 *
	 *	This method terminate all cp file threads
	*/
	void shutDown();

 private:

	/**
	 * 	@brief	Constructor of FMS_CPF_CpOpenFiles class
	*/
	CPDOpenFiles();

	/**
	 * 	@brief	Destructor of FMS_CPF_CpOpenFiles class
	*/
	virtual ~CPDOpenFiles();

	/**
	 * 	Internal structure to store CPDFileThrd that handles the opened file
	*/
	struct MsgSide
	{
		CPDFileThrd* fileThrd;
		//EXside = true or SBside  = false
		bool cpSide;
	};

    typedef std::map<uint32_t, MsgSide> FileMapType;

    /**
	 * 	@brief	m_OpenFiles
	 *
	 * 	Internal map of the opened files
	*/
    FileMapType m_OpenFiles;

    typedef std::list<FileLock* > FileLockListType;

    /**
	 * 	@brief	m_OpenLocks
	 *
	 * 	Internal list of the FileLock objects
	*/
    FileLockListType m_OpenLocks;

    /**
	 * 	@brief	m_mutex
	 *
	 * 	Mutex for internal map access
	*/
	ACE_Thread_Mutex m_mutex;

	/**
	 * 	@brief	fms_cpf_OpenFilesTrace: trace object
	*/
	ACS_TRA_trace* fms_cpf_OpenFilesTrace;

};

#endif //FMS_CPF_CPDOPENFILES_H_
