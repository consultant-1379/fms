/*
 * * @file fms_cpf_filedescriptor.h
 *	@brief
 *	Header file for File class.
 *  This module contains the declaration of the class FileDescriptor.
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

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FILEDESCRIPTOR_H
#define FILEDESCRIPTOR_H

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_types.h"
#include "fms_cpf_privateexception.h"
#include "fms_cpf_fileid.h"
#include "fms_cpf_attribute.h"
#include <string>

class File;
class FileAccess;
class FileTable;
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FileDescriptor
{
	friend class File;
	friend class FileAccess;
	friend class FileTable;
	friend class CPF_MoveFile_Request;
	friend class FMS_CPF_BlockSender;

public:

	~FileDescriptor();
	bool operator==(const FileDescriptor& fd) const;

	void rename(const FMS_CPF_FileId& fileid, const std::string& newFileDN) throw (FMS_CPF_PrivateException);

	FMS_CPF_FileId getFileid() const;

	std::string getVolume() const;

	void setVolume(const std::string& volume) throw (FMS_CPF_PrivateException);

	FMS_CPF_Attribute getAttribute() const;

	void setAttribute(const FMS_CPF_Attribute& attribute) throw (FMS_CPF_PrivateException);

	unsigned short getRecordLength() const;

	FMS_CPF_Types::accessType getAccess() const;

	FMS_CPF_Types::userType getUsers() const;

	size_t getSize() const throw (FMS_CPF_PrivateException);

	std::string getPath() const throw (FMS_CPF_PrivateException);

	unsigned long getLastSentSubfile() const throw (FMS_CPF_PrivateException);

	unsigned long getActiveSubfile() const throw (FMS_CPF_PrivateException);

	void getActiveSubfile(std::string &str) throw (FMS_CPF_PrivateException);

	// Get the TQ DN
	void getTransferQueueDN(std::string& tqDN) throw(FMS_CPF_PrivateException);

	// Clear TQ DN
	void clearTransferQueueDN() throw(FMS_CPF_PrivateException);

	unsigned long changeActiveSubfile() throw (FMS_CPF_PrivateException);

	void updateAttributes();

	off64_t phyOpenWriteApp(const char* filePath, int &writeFd);

	off64_t phyOpenRW(const char* filePath, int &readFd, int &writeFd);

	int phyClose(int& readFD, int& writeFD);

	int phyExist(const char* path);

	ssize_t phyPwrite(int fildes, const char* buf,	size_t nbyte, unsigned long	aRecordNumber);

	ssize_t phyPread(int fildes, char* buf, size_t nbyte, off64_t offset);

	off64_t phyGetFileSize(const char* path);

	long phyUnifyFileSize (const char* path, unsigned short aRecordLength);

	ssize_t phyWrite( int handle, const char* buf, unsigned long aRecordsToWrite, unsigned int count, unsigned long& aLastRecordWritten);

	ssize_t phyRead(int handle, void* buf, unsigned int count);

	int phyFExpand(const char* filePath, off64_t aSubfileSize);

	off64_t phyRewrite(const char* path, int& readFD, int& writeFD);

	off64_t phyLseek(int readFD, off64_t toPos);

	void changeActiveSubfileOnCommand() throw(FMS_CPF_PrivateException);

	bool changeSubFileOnCommand(void) throw(FMS_CPF_PrivateException);

private:

	void setTmpName(const std::string& name);

	FileDescriptor(FileAccess* faccessp, FMS_CPF_Types::accessType access, bool inf);

	void setLastSentSubfile(const unsigned long& lastSentSubfile) throw(FMS_CPF_PrivateException);

	FileAccess* faccessp_;

	FMS_CPF_Types::accessType access_;

	bool inf_;

	bool secure;

	unsigned short m_fileRecordLength;

	bool m_firstRun;

	/**		@brief	fms_cpf_FileDspt
	*/
	ACS_TRA_trace* fms_cpf_FileDspt;

};
#endif
