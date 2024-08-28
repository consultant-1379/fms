/*
 * * @file fms_cpf_infinitefileopenlist.h
 *	@brief
 *	Header file for FMS_CPF_InfiniteFileOpenList class.
 *  This module contains the declaration of the class FMS_CPF_InfiniteFileOpenList.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-16
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
 *	| 1.0.0  | 2011-11-16 | qvincon      | File imported.                      |
 *	+========+============+==============+=====================================+
 *	| 1.1.0  | 2011-11-16 | qvincon      | ACE introduced.                     |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_InfiniteOpenFilesList_H_
#define FMS_CPF_InfiniteOpenFilesList_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_fileid.h"

#include <ace/Recursive_Thread_Mutex.h>
#include <ace/Singleton.h>

#include <list>
#include <map>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class  FMS_CPF_InfiniteFileOpenList
{
 public:

	friend class ACE_Singleton<FMS_CPF_InfiniteFileOpenList, ACE_Recursive_Thread_Mutex>;

	/** @brief add method
	 *
	 *	This method inserts a new infinite file opened into the list
	 *
	 *	@param FMS_CPF_FileId: Infinite file opened.
	*/
	void add(const std::string& cpName, const FMS_CPF_FileId& aFileId); //HT89089

	/** @brief add method
	 *
	 *	This method removes a infinite file opened from the list
	 *
	 *	@param FMS_CPF_FileId: Infinite file to remove.
	*/
	void remove(const std::string& cpName, const FMS_CPF_FileId& aFileId);  //HT89089

	bool removeIfMoreThanOneLeft(const std::string& cpName, const FMS_CPF_FileId& aFileId);   //HT89089

 private:

	/**
	 * 	@brief	Constructor of FMS_CPF_InfiniteFileOpenList class
	*/
	FMS_CPF_InfiniteFileOpenList();

	/**
	 * 	@brief	Destructor of FMS_CPF_InfiniteFileOpenList class
	*/
	virtual ~FMS_CPF_InfiniteFileOpenList();

	/**
	 * 	@brief	m_InfiniteFileOpenMap
	 *
	 * 	Map of opened infinite files
	*/
	std::map<std::string, std::list<FMS_CPF_FileId> > m_InfiniteFileOpenMap; //HT89089

	/**
	 * 	@brief	m_mutex
	 *
	 * 	Mutex for internal list access
	*/
	ACE_Recursive_Thread_Mutex m_mutex;

	/**
	 * 	@brief	fms_cpf_IFOListTrace: trace object
	*/
	ACS_TRA_trace* fms_cpf_IFOListTrace;
};

typedef ACE_Singleton<FMS_CPF_InfiniteFileOpenList, ACE_Recursive_Thread_Mutex> InfiniteFileOpenList;

#endif //FMS_CPF_InfiniteOpenFilesList_H_
