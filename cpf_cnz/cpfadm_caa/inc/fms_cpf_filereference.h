/*
 * * @file fms_cpf_filereference.h
 *	@brief
 *	Header file for FileReference class.
 *  This module contains the declaration of the class FileReference.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-07-06
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
 *	| 1.0.0  | 2011-07-06 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FILEREFERENCE_H
#define FILEREFERENCE_H

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_privateexception.h"

#include <ace/Recursive_Thread_Mutex.h>

class FileDescriptor;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FileReference
{ 
public:

	/*
	 * Friend classes
	 */
	friend class File;
	friend class FileTable;
	/**
		@brief	Constructor of FileReference class
	*/
	FileReference();

	/**
		@brief	Constructor of FileReference class
	*/
	explicit FileReference(const unsigned long ref);

	/**
		@brief	Destructor of FileReference class
	*/
	~FileReference();

	/*
	 * @brief operator -> overload
	 */
	FileDescriptor* operator->() throw(FMS_CPF_PrivateException);

	/*
	 * @brief operator () overload
	 */
	operator unsigned long() const;

	/*
	 * @brief getReferenceValue get the numeric value
	 */
	unsigned long getReferenceValue() const { return reference_.l; };

	/*
	 * @brief operator != overload
	 */
	bool operator!=(const FileReference& other) const;

	/*
	 * @brief operator == overload
	 */
	bool operator==(const FileReference& other) const;

	/*
	 * @brief operator = overload
	 */
	FileReference& operator=(const FileReference& rhs);

	/*
	 * @brief operator >> overload
	 */
	friend std::istream& operator>> (std::istream& istr, FileReference& fd);

	/*
	 * @brief operator >> overload
	 */
	friend std::ostream& operator<< (std::ostream& ostr, const FileReference& fd);

	/* @brief	isValid method
	 *
	 * This method verifies if a file reference is valid
	 *
	 * @return true if the file reference is valid, otherwise false
	 *
	 * @remarks Remarks
	*/
	bool isValid();

	/* @brief	first method
	 *
	 * This method
	 *
	 * @remarks Remarks
	*/
	static void first();

	/* @brief	next method
	 *
	 * This method retrieves the next file reference
	 *
	 * @return a file reference
	 *
	 * @remarks Remarks
	*/
	static FileReference next();

	static const FileReference NOREFERENCE;

private:

	static unsigned short prand();

	static FileReference insert(FileDescriptor* fd) throw (FMS_CPF_PrivateException);

	static FileDescriptor* find(FileReference ref);

	static bool remove(FileReference ref);

	static void resize(unsigned long);

	union
	{
		struct
		{
			unsigned short key;  
			unsigned short index;
		} s;
		unsigned long l;  
	} reference_;

	static const unsigned short EXPANSION;
	static FileDescriptor** fdList_;
	static unsigned short* keyList_;  
	static unsigned short* idleList_;
	static unsigned short firstIdle_;
	static unsigned short listSize_;
	static unsigned short random_;
	static unsigned short index_;	// Used for iterating

	static ACE_Recursive_Thread_Mutex m_mutex;
};

#endif
