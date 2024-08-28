/*
 * * @file fms_cpf_filereference.cpp
 *	@brief
 *	Class method implementation for FileReference.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_filereference.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-07-05
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

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_filereference.h"
#include "fms_cpf_filedescriptor.h"

#include "ace/Guard_T.h"

#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

const FileReference FileReference::NOREFERENCE;
const unsigned short FileReference::EXPANSION = 100;
FileDescriptor **FileReference::fdList_ = 0;
unsigned short* FileReference::keyList_ = 0;
unsigned short* FileReference::idleList_ = 0;
unsigned short FileReference::firstIdle_ = 0;
unsigned short FileReference::listSize_ = 0;
unsigned short FileReference::random_ = 0;
unsigned short FileReference::index_ = 0;	// Used for iterating

ACE_Recursive_Thread_Mutex FileReference::m_mutex;

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FileReference::FileReference()
{
    reference_.l = 0;
}

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FileReference::FileReference(const unsigned long ref)
{
    reference_.l = ref;
}

//------------------------------------------------------------------------------
//      Conversion operator
//------------------------------------------------------------------------------
FileReference::operator unsigned long() const
{
	return reference_.l;
}

bool FileReference::operator!=(const FileReference& other) const
{
	return (this->reference_.l != other.getReferenceValue() );
}

bool FileReference::operator==(const FileReference& other) const
{
	return (this->reference_.l == other.getReferenceValue() );
}

FileReference& FileReference::operator=(const FileReference& rhs)
{
	this->reference_.l = rhs.getReferenceValue();
	return *this;
}


//------------------------------------------------------------------------------
//      Arrow operator
//------------------------------------------------------------------------------
FileDescriptor* FileReference::operator-> ()
throw (FMS_CPF_PrivateException)
{
	FileDescriptor* fd = FileReference::find(*this);
	if( NULL == fd )
	{
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::INVALIDREF);
	} 
	return fd;
}

//------------------------------------------------------------------------------
//      Check if file reference is valid
//------------------------------------------------------------------------------
bool FileReference::isValid ()
{
	return find(*this) ? true: false;
}

//------------------------------------------------------------------------------
//      Generate a pseudo random number
//------------------------------------------------------------------------------
unsigned short FileReference::prand ()
{
	unsigned short randomTemp = 0;
	//Critical section
	{
		ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex, 0);
		random_ = (4101 * random_ + 1) % 0x8000;
		randomTemp = random_ | 0x8000;
	}
	return randomTemp;
}
 

//------------------------------------------------------------------------------
//      Insert a file descriptor
//------------------------------------------------------------------------------
FileReference FileReference::insert(FileDescriptor *fd) throw(FMS_CPF_PrivateException)
{
	FileReference ref;
	// Critical section
	{
		ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex, NOREFERENCE);

		if (firstIdle_ == listSize_)
		{
			if (listSize_ < 0x10000 - EXPANSION)
			{
				resize(listSize_ + EXPANSION);
			}
			else
			{
				CPF_Log.Write("FileReference::insert(), Cannot allocate file reference", LOG_LEVEL_ERROR);
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "Cannot allocate file reference");
			}  
		}
		ref.reference_.s.index = idleList_[firstIdle_];
		firstIdle_++;
		fdList_[ref.reference_.s.index] = fd;
		keyList_[ref.reference_.s.index] = ref.reference_.s.key = prand ();
	}
	return ref;
}


//------------------------------------------------------------------------------
//      Find a file descriptor
//------------------------------------------------------------------------------
FileDescriptor* FileReference::find(FileReference ref)
{
	FileDescriptor* myReturnPtr = NULL;
	// Critical section
	{
		ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex, NULL);

		if(ref.reference_.s.index < listSize_)
		{
			if(fdList_[ref.reference_.s.index])
			{
				if(ref.reference_.s.key == keyList_[ref.reference_.s.index])
				{
					myReturnPtr = fdList_[ref.reference_.s.index];
				}
			}
		}
	}
	return myReturnPtr;
}

//------------------------------------------------------------------------------
//      Remove a file descriptor
//------------------------------------------------------------------------------
bool FileReference::remove(FileReference ref)
{
	bool result = false;
	// Critical section
	{
		ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex, result);
		if(ref.reference_.s.index < listSize_)
		{
			if(fdList_[ref.reference_.s.index])
			{
				if(ref.reference_.s.key == keyList_ [ref.reference_.s.index])
				{
					fdList_[ref.reference_.s.index] = 0;
					firstIdle_--;
					idleList_[firstIdle_] = ref.reference_.s.index;
					result = true;
				}
			}
		}
	}
	return result;
}

//------------------------------------------------------------------------------
//      Set index to first file descriptor
//------------------------------------------------------------------------------
void FileReference::first()
{
	ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, m_mutex);
	index_ = 0;
}

//------------------------------------------------------------------------------
//      Set index to next file descriptor
//------------------------------------------------------------------------------
FileReference FileReference::next()
{
	FileReference ref;
	{
		ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex, NOREFERENCE);

		while(index_ < listSize_)
		{
			if (fdList_ [index_]) 
			{
				ref.reference_.s.index = index_;
				ref.reference_.s.key = keyList_ [index_];
				index_++;
				return ref;
			}  
			else
			{
				index_++;
			}
		}
	}
	return ref;
}

//------------------------------------------------------------------------------
//      Resize the table of file descriptors
//------------------------------------------------------------------------------
void FileReference::resize(unsigned long size)
{
	// Critical section
	ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, m_mutex);

	unsigned short index;
	unsigned short* idleList;
	FileDescriptor** fdList;
	unsigned short* keyList;

	idleList = new unsigned short[size];
	fdList = new FileDescriptor* [size];
	keyList = new unsigned short[size];
	if (size > listSize_)
	{
		for (index = 0; index < listSize_; index++)
		{
			idleList [index] = idleList_ [index];
			fdList [index] = fdList_ [index];
			keyList [index] = keyList_ [index];
		}
		for (index = listSize_; index < size; index++)
		{
			idleList [index] = index;
			fdList [index] = 0;
			keyList [index] = 0;
		}
	}
	else
	{
		for (index = 0; index < size; index++)
		{
			idleList [index] = index;
			fdList [index] = 0;
			keyList [index] = 0;
		}
	}
	if (listSize_ > 0)
	{
		delete[] idleList_;
		delete[] fdList_;
		delete[] keyList_;
	}
	idleList_ = idleList;
	fdList_ = fdList;
	keyList_ = keyList;
	listSize_ = size;
}

//------------------------------------------------------------------------------
//      Read file reference from istream
//------------------------------------------------------------------------------
istream& operator>> (istream& istr, FileReference& ref)
{
	istr >> ref.reference_.l;
	return istr;
}


//------------------------------------------------------------------------------
//      Write file reference to ostream
//------------------------------------------------------------------------------
ostream& operator<< (ostream& ostr, const FileReference& ref)
{
	ostr << ref.reference_.l;
	return ostr;
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FileReference::~FileReference ()
{
}

