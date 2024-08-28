/*
 * * @file fms_cpf_fileaccess.cpp
 *	@brief
 *	Class method implementation for FileAccess.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_fileaccess.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-07-07
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
 *	| 1.0.0  | 2011-07-07 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_fileaccess.h"
#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_common.h"
#include "fms_cpf_eventalarmhndl.h"

#include "ACS_TRA_trace.h"

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FileAccess::FileAccess(const FMS_CPF_FileId& fileid) :
fileid_ (fileid),
filep_ (0),
fdList_ ()
{
  users_.ucount = 0;
  users_.rcount = 0;
  users_.wcount = 0;
  icount_ = 0;
  changeSubFileOnCommand = false;
  fms_cpf_FileAccess = new (std::nothrow) ACS_TRA_trace("FMS_CPF_FileAccess");
}


//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FileAccess::FileAccess(const FMS_CPF_FileId& fileid, File* filep) :
fileid_ (fileid),
filep_ (filep),
fdList_ ()
{
  users_.ucount = 0;
  users_.rcount = 0;
  users_.wcount = 0;
  icount_ = 0;
  changeSubFileOnCommand = false;
  fms_cpf_FileAccess = new (std::nothrow) ACS_TRA_trace("FMS_CPF_FileAccess");
}


//------------------------------------------------------------------------------
//      Equality operator
//------------------------------------------------------------------------------
bool FileAccess::operator== (const FileAccess& faccess) const
{
  return faccess.fileid_ == fileid_;
}

bool FileAccess::operator== (const FileAccess * faccess) const
{
  return faccess->fileid_ == fileid_;
}

//------------------------------------------------------------------------------
//      Create file descriptor
//------------------------------------------------------------------------------
FileDescriptor* FileAccess::create (FMS_CPF_Types::accessType access, bool inf)
throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileAccess, "%s", "Entering in Create");

	if( !fdList_.empty() )
	{
		if(checkAccess(access) == false)
		{
			throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::ACCESSERROR);
		}
	}

	setAccess(access, inf);

	// Create the file descriptor
	FileDescriptor* fd = new FileDescriptor(this, access, inf);

	// add the new file descriptor in the list
	fdList_.push_back(fd);

	TRACE(fms_cpf_FileAccess, "%s", "Leaving Create");
	return fd;
}

//------------------------------------------------------------------------------
//      Remove file descriptor
//------------------------------------------------------------------------------
void FileAccess::remove(FileDescriptor* fd)
{
	TRACE(fms_cpf_FileAccess, "%s", "Entering in remove()");

	bool fdfound = false;
	FDList::iterator listPtr;

	if( !fdList_.empty() )
	{
		for(listPtr = fdList_.begin(); !fdfound && listPtr != fdList_.end(); ++listPtr)
		{
			FileDescriptor* next = *listPtr;
			if(next == fd)
			{
				fdfound = true;
			}
		}
	}

	if(fdfound)
	{
		unsetAccess(fd->access_, fd->inf_);

		// remove the file descriptor from the list
		fdList_.remove(fd);

		delete fd;

		TRACE(fms_cpf_FileAccess, "%s", "Remove(), file descriptor erased");
	}
	else
	{
		TRACE(fms_cpf_FileAccess, "%s", "remove(), file descriptor not found");
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "remove(), file descriptor not found");
	}

	TRACE(fms_cpf_FileAccess, "%s", "Leaving remove()");
}

//------------------------------------------------------------------------------
//      Check the file access
//------------------------------------------------------------------------------
bool FileAccess::checkAccess(FMS_CPF_Types::accessType access)
throw (FMS_CPF_PrivateException)
{
  TRACE(fms_cpf_FileAccess, "%s", "Entering checkAccess()");

  bool result = true;

  TRACE(fms_cpf_FileAccess, "checkAccess(), access = %d", access);
  switch (access)
  {
    case FMS_CPF_Types::NONE_:
      if( users_.rcount == (FMS_CPF_EXCLUSIVE+1) ) result = false;
      break;

    case FMS_CPF_Types::R_:
      if (users_.rcount >= FMS_CPF_EXCLUSIVE) result = false;
      break;

    case FMS_CPF_Types::XR_:
      if (users_.rcount > 0) result = false;
      break;

    case FMS_CPF_Types::W_:
      if (users_.wcount >= FMS_CPF_EXCLUSIVE) result = false;
      break;

    case FMS_CPF_Types::XW_:
      if (users_.wcount > 0) result = false;
      break;

    case FMS_CPF_Types::R_W_:
      if( (users_.rcount >= FMS_CPF_EXCLUSIVE) || (users_.wcount >= FMS_CPF_EXCLUSIVE) )
    	  result = false;
      break;

    case FMS_CPF_Types::XR_W_:
      if( (users_.rcount > 0 ) || (users_.wcount == FMS_CPF_EXCLUSIVE )) result = false;
      break;

    case FMS_CPF_Types::R_XW_:
      if( (users_.rcount == FMS_CPF_EXCLUSIVE) || (users_.wcount > 0) ) result = false;
      break;

    case FMS_CPF_Types::XR_XW_:
      if( (users_.rcount > 0) || (users_.wcount > 0) ) result = false;
      break;

    case FMS_CPF_Types::DELETE_:
      if( ((users_.rcount != 0) && (users_.rcount != (FMS_CPF_EXCLUSIVE+1))) ||
          ((users_.wcount != 0) && (users_.wcount != (FMS_CPF_EXCLUSIVE+1)))  ) result = false;
      break;

    default:
        throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "Unknown access mode");
  }

  TRACE(fms_cpf_FileAccess, "Leaving checkAccess(), result=%s", (result ? "TRUE" : "FALSE") );
  return result;
}

//------------------------------------------------------------------------------
//      Check the file access
//------------------------------------------------------------------------------
bool FileAccess::isLockedforDelete()
{
	bool isLocked = ((FMS_CPF_EXCLUSIVE + 1) == users_.rcount ) && ( (FMS_CPF_EXCLUSIVE + 1) == users_.wcount);

	TRACE(fms_cpf_FileAccess, "Leaving isLockedforDelete(), isLocked=<%s>", (isLocked ? "TRUE" : "FALSE") );
	return isLocked;
}

//------------------------------------------------------------------------------
//      Set the file access
//------------------------------------------------------------------------------
void FileAccess::setAccess(FMS_CPF_Types::accessType access, bool inf)
throw (FMS_CPF_PrivateException)
{
  TRACE(fms_cpf_FileAccess, "%s", "Entering setAccess()");

  if (users_.ucount == MAXUSERS)
  {
	  throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::ACCESSERROR);
  }

  users_.ucount++;

  TRACE(fms_cpf_FileAccess, "setAccess(), access = %d", access);

  switch (access)
  {
    case FMS_CPF_Types::NONE_:
      break;

    case FMS_CPF_Types::R_:
      users_.rcount++;
      break;

    case FMS_CPF_Types::XR_:
      users_.rcount = FMS_CPF_EXCLUSIVE;
      break;

    case FMS_CPF_Types::W_:
      users_.wcount++;
      break;

    case FMS_CPF_Types::XW_:
      users_.wcount = FMS_CPF_EXCLUSIVE;
      break;

    case FMS_CPF_Types::R_W_:
      users_.rcount++;
      users_.wcount++;
      break;

    case FMS_CPF_Types::XR_W_:
      users_.rcount = FMS_CPF_EXCLUSIVE;
      users_.wcount++;
      break;

    case FMS_CPF_Types::R_XW_:
      users_.rcount++;
      users_.wcount = FMS_CPF_EXCLUSIVE;
      break;

    case FMS_CPF_Types::XR_XW_:
      users_.rcount = FMS_CPF_EXCLUSIVE;
      users_.wcount = FMS_CPF_EXCLUSIVE;
      break;

    case FMS_CPF_Types::DELETE_:
	  users_.rcount = FMS_CPF_EXCLUSIVE + 1;
	  users_.wcount = FMS_CPF_EXCLUSIVE + 1;
	  break;

    default:   
      users_.ucount--;
      throw  FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "Unknown access mode");
  }
  (inf == true) ? icount_++ : 0;
  TRACE(fms_cpf_FileAccess, "Leaving setAccess(), ucount=%i, rcount=%i, wcount=%i, icount_=%i",
		  users_.ucount, users_.rcount, users_.wcount, icount_
	    );
}

//------------------------------------------------------------------------------
//      Unset the file access
//------------------------------------------------------------------------------
void FileAccess::unsetAccess (FMS_CPF_Types::accessType access, bool inf)
throw (FMS_CPF_PrivateException)
{
  TRACE(fms_cpf_FileAccess, "Entering unsetAccess(), access = %d", access);

  users_.ucount--;

  switch (access)
  {
    case FMS_CPF_Types::NONE_:
      break;

    case FMS_CPF_Types::R_:
      users_.rcount--;
      break;

    case FMS_CPF_Types::XR_:
      users_.rcount = 0;
      break;

    case FMS_CPF_Types::W_:
      users_.wcount--;
      break;

    case FMS_CPF_Types::XW_:
      users_.wcount = 0;
      break;

    case FMS_CPF_Types::R_W_:
      users_.rcount--;
      users_.wcount--;
      break;

    case FMS_CPF_Types::XR_W_:
      users_.rcount = 0;
      users_.wcount--;
      break;

    case FMS_CPF_Types::R_XW_:
      users_.rcount--;
      users_.wcount = 0;
      break;

    case FMS_CPF_Types::XR_XW_:
    case FMS_CPF_Types::DELETE_:
      users_.rcount = 0;
      users_.wcount = 0;
      break;

    default: 
      users_.ucount++;
      throw   FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "Unknown access mode");
  }
  (inf == true) ? icount_--: 0;

  TRACE(fms_cpf_FileAccess, "Leaving unsetAccess(), ucount=%i, rcount=%i, wcount=%i, icount_=%i",
		  users_.ucount, users_.rcount, users_.wcount, icount_
	    );
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FileAccess::~FileAccess ()
{
	if(NULL != fms_cpf_FileAccess)
			delete fms_cpf_FileAccess;
}
