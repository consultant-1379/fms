/*
 * * @file fms_cpf_infinitecpdfile.h
 *	@brief
 *	Header file for InfiniteCPDFile class.
 *  This module contains the declaration of the class InfiniteCPDFile.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-22
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
 *	| 1.0.0  | 2011-11-22 | qvincon      | File imported.                      |
 *	+========+============+==============+=====================================+
 */
/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_INFINITECPDFILE_H_
#define FMS_CPF_INFINITECPDFILE_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_cpdfile.h"
#include "fms_cpf_privateexception.h"
#include "fms_cpf_filereference.h"

#include "fms_cpf_types.h"

#include <string>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class InfiniteCPDFile : public CPDFile
{
 public:

	InfiniteCPDFile(std::string& cpname);

	virtual ~InfiniteCPDFile();

	virtual void open(const FMS_CPF_FileId& aFileId,
					  unsigned char aAccessType,
					  unsigned char aSubfileOption,
					  off64_t aSubfileSize,
					  unsigned long& outFileReferenceUlong,
					  unsigned short& outRecordLength,
					  unsigned long& outFileSize,
					  unsigned char& outFileType,
					  bool isInfiniteSubFile
					 ) throw(FMS_CPF_PrivateException);

	virtual void close(unsigned char aSubfileOption,
			  	  	   bool noSubFileChange = false,
			  	  	   bool syncReceived = false
			  	  	  ) throw(FMS_CPF_PrivateException);

	virtual void writerand(unsigned long aRecordNumber,
			  	  	  	   char* aDataRecord,
			  	  	  	   unsigned long aDataRecordSize
			  	  	  	  ) throw(FMS_CPF_PrivateException);

	virtual void readrand(unsigned long	aRecordNumber,
			  	  	  	  char* aDataRecord
			  	  	  	 ) throw(FMS_CPF_PrivateException);

	virtual void writenext(unsigned long aRecordsToWrite,
			  	  	  	   char* aDataRecords,
			  	  	  	   unsigned long aDataRecordsSize,
			  	  	  	   unsigned long& aLastRecordWritten
			  	  	  	  ) throw(FMS_CPF_PrivateException);

	virtual void readnext(unsigned long	aRecordsToRead,
			  	  	  	  char* aDataRecords,
			  	  	  	  unsigned short& aRecordsRead
			  	  	  	 ) throw(FMS_CPF_PrivateException);

	virtual void rewrite() throw(FMS_CPF_PrivateException);

	virtual void reset(unsigned long aRecordNumber) throw(FMS_CPF_PrivateException);

	virtual void checkSwitchSubFile(bool onTime, bool fileClosed, bool& noMoreTimeSwitch);

 private:

	void openSubfile() throw(FMS_CPF_PrivateException);

	void closeSubfile() throw(FMS_CPF_PrivateException);

	void writerandSubfile(unsigned long aRecordNumber,
						  char* aBuffer,
						  unsigned long aBufferSize
		  	  	  	   	 ) throw(FMS_CPF_PrivateException);


	int mustChangeSubfile( bool& secure) throw(FMS_CPF_PrivateException);

	FMS_CPF_Types::infiniteType getAttributes();

	void activeSubfilePath(std::string& path) throw(FMS_CPF_PrivateException);

	void ulong2subfile(unsigned long value, std::string& strValue);

	bool existSubfile();

	unsigned long findRecord();

	bool checkRelease();

	void switchSubFile();

	bool checkAttributes();

	unsigned long m_CurrentRecord;

	int m_WriteFd;

	unsigned long m_ActiveSubFile;

	FileReference m_SubFileReference;

	FMS_CPF_Types::accessType m_AccessMode;

	std::string m_VolumeName;

	std::string m_MainFileName;

	ACS_TRA_trace* fms_cpf_InfCPDFileTrace;
};

#endif //FMS_CPF_INFINITECPDFILE_H_
