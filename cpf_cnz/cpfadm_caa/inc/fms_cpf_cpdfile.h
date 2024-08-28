/*
 * * @file fms_cpf_cpdfile.h
 *	@brief
 *	Header file for CPDFile class.
 *  This module contains the declaration of the class CPDFile.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-21
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
 *	| 1.0.0  | 2011-11-21 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */
/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_CPDFILE_H_
#define FMS_CPF_CPDFILE_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_fileid.h"
#include "fms_cpf_filereference.h"
#include "fms_cpf_privateexception.h"

#include <string>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class ACS_TRA_trace;
/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPDFile
{
 public:

	CPDFile(std::string& cpname);

	virtual ~CPDFile();

	unsigned short getRecordLength() throw(FMS_CPF_PrivateException);

	off64_t getFileSize(const char* path) throw(FMS_CPF_PrivateException);

	FMS_CPF_FileId getFileId() { return m_FileReference->getFileid(); };

	std::string getFileName();

	std::string  getCPName() const { return m_CpName; };

	virtual void open(const FMS_CPF_FileId& aFileId,
					  unsigned char aAccessType,
					  unsigned char aSubfileOption,
					  off64_t aSubfileSize,
					  unsigned long& outFileReferenceUlong,
					  unsigned short& outRecordLength,
					  unsigned long& outFileSize,
					  unsigned char& outFileType,
					  bool isInfiniteSubFile
					 ) throw(FMS_CPF_PrivateException) = 0;

	virtual void close(unsigned char aSubfileOption,
					   bool noSubFileChange = false,
					   bool syncReceived = false
					  ) throw(FMS_CPF_PrivateException) = 0;

	virtual void writerand(unsigned long aRecordNumber,
						   char* aDataRecord,
						   unsigned long aDataRecordSize
						  ) throw(FMS_CPF_PrivateException) = 0;

	virtual void readrand(unsigned long	aRecordNumber,
						  char* aDataRecord
						 ) throw(FMS_CPF_PrivateException) = 0;

	virtual void writenext(unsigned long aRecordsToWrite,
						   char* aDataRecords,
						   unsigned long aDataRecordsSize,
						   unsigned long& aLastRecordWritten
						  ) throw(FMS_CPF_PrivateException) = 0;

	virtual void readnext(unsigned long	aRecordsToRead,
						  char* aDataRecords,
						  unsigned short& aRecordsRead
						 ) throw(FMS_CPF_PrivateException) = 0;

	virtual void rewrite() throw(FMS_CPF_PrivateException) = 0;

	virtual void reset(unsigned long aRecordNumber) throw(FMS_CPF_PrivateException) = 0;

	virtual void checkSwitchSubFile(bool onTime, bool fileClosed, bool& noMoreTimeSwitch) = 0;

 protected:

	FileReference getFileReference() const { return m_FileReference; };

	void setFileReference(const FileReference& aFileReference);

	bool isCompositeClass()  throw(FMS_CPF_PrivateException);

	bool isSubFile();

	bool isSubFile(const FMS_CPF_FileId& fileId);

	void checkFileClass() throw(FMS_CPF_PrivateException);

	void checkReadAccess() throw(FMS_CPF_PrivateException);

	void checkWriteAccess() throw(FMS_CPF_PrivateException);

	long unifyFileSize(
			const std::string& aFilePath,
			unsigned short aRecordLength) throw(FMS_CPF_PrivateException);

	void closeWithoutException(const FileReference& aFileReference) const;

	std::string m_CpName;

 private:

	FileReference m_FileReference;

	ACS_TRA_trace* fms_cpf_CPDFileTrace;
};

#endif //FMS_CPF_CPDFILE_H_
