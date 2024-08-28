/*
 * * @file fms_cpf_filelock.h
 *	@brief
 *	Header file for FMS_CPF_FileLock class.
 *  This module contains the declaration of the class FMS_CPF_FileLock.
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
 *	| 1.0.0  | 2011-11-18 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 *
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_FILELOCK_H_
#define FMS_CPF_FILELOCK_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include <ace/Thread_Semaphore.h>

#include <string>

class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class FileLock {
 public:

	/**
	 * 	@brief	Constructor of FMS_CPF_FileLock class
	*/
	FileLock(const std::string& afileId, const std::string& cpName);

	/**
	 * 	@brief	Destructor of FMS_CPF_FileLock class
	*/
	virtual ~FileLock();

	/**
	 * 	@brief	This method lock the file access
	*/
	void lock();

	/**
	 * 	@brief	This method unlock the file access
	*/
	void unlock();

	/**
	 * 	@brief	This method increases of one the number of users of the file
	*/
	void increaseUsers() { m_Users++; };

	/**
	 * 	@brief	This method decreases of one the number of users of the file
	*/
	void decreaseUsers(){ m_Users--; };

	/**
	 * 	@brief	This method returns the number of users of the file
	*/
	int getNumOfUsers() const {return m_Users; };

	/**
	 * 	@brief	This method returns the file name
	*/
	const char* getFileName() const {return m_FileName.c_str(); };

	/**
	 * 	@brief	This method returns the CP name
	*/
	const char* getCpName() const {return m_CpName.c_str(); };


 private:

	/**
	   @brief  	m_Users, number of users
	*/
	int m_Users;


	/**
	   @brief  	m_FileId, the file name
	*/
	std::string m_FileName;

	/**
	   @brief  	m_CpName, the CP name
	*/
	std::string m_CpName;

	/**
	   @brief  	m_lock, internal lock structure
	*/
	ACE_Thread_Semaphore m_lock;

	ACS_TRA_trace* m_trace;

	// Disallow copying and assignment.
	FileLock(const FileLock&);
	void operator= (const FileLock&);

};


#endif /* FMS_CPF_FILELOCK_H_ */
