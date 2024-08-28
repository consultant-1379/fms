/*
 * * @file fms_cpf_fileaccess.h
 *	@brief
 *	Header file for FileAccess class.
 *  This module contains the declaration of the class FileAccess.
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
#ifndef FILEACCESS_H
#define FILEACCESS_H

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_types.h"
#include "fms_cpf_privateexception.h"
#include "fms_cpf_fileid.h"

//#include <cstdlib>
#include<list>

const unsigned short MAXUSERS = (unsigned short)255;  

class File;
class FileDescriptor;
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FileAccess
{
	friend class File;
	friend class FileTable;
	friend class FileDescriptor;

 public:

	bool operator== (const FileAccess& faccess) const;
	bool operator== (const FileAccess * faccess) const;

 private:

	FileAccess(const FMS_CPF_FileId& fileid);

	FileAccess(const FMS_CPF_FileId& fileid, File* filep);

	~FileAccess(void);

	FileDescriptor* create(FMS_CPF_Types::accessType access, bool inf)
							throw (FMS_CPF_PrivateException);

	void remove(FileDescriptor* fd);

	bool checkAccess(FMS_CPF_Types::accessType access)
					  throw (FMS_CPF_PrivateException);

	bool isLockedforDelete();

	void setAccess(FMS_CPF_Types::accessType access, bool inf)
					throw (FMS_CPF_PrivateException);

	void unsetAccess(FMS_CPF_Types::accessType access, bool inf)
					  throw (FMS_CPF_PrivateException);

	FMS_CPF_FileId fileid_;

	File* filep_;

	FMS_CPF_Types::userType users_;

	unsigned short icount_;

	bool changeSubFileOnCommand; // TODO removed it?

	typedef std::list<FileDescriptor*, std::allocator<FileDescriptor*> >FDList;

	FDList fdList_;

	/**		@brief	fms_cpf_FileAccess
	*/
	ACS_TRA_trace* fms_cpf_FileAccess;

};

#endif
