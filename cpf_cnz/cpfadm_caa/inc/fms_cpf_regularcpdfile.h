/*
 * * @file fms_cpf_regularcpdfile.h
 *	@brief
 *	Class method implementation for InfiniteCPDFile.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_infinitecpdfile.h module
 *
 *	@author enungai (Nunziante Gaito)
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
 *	| 1.0.0  | 2011-11-21 | enungai      | File imported.                      |
 *	+========+============+==============+=====================================+
 */



#ifndef REGULARCPDFILE_H
#define REGULARCPDFILE_H

#include "fms_cpf_cpdfile.h"
class ACS_TRA_trace;

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

class RegularCPDFile: public CPDFile
{
public:
  RegularCPDFile(std::string& cpname);
  virtual ~RegularCPDFile();

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
		  	  	  	) throw (FMS_CPF_PrivateException);

  virtual void writerand(unsigned long aRecordNumber,
		  	  	  	  	 char* aDataRecord,
		  	  	  	  	 unsigned long aDataRecordSize
		  	  	  	  	) throw(FMS_CPF_PrivateException);

  virtual void readrand(unsigned long aRecordNumber,
		  	  	  	  	char* aDataRecord
		  	  	  	   ) throw(FMS_CPF_PrivateException);

  virtual void writenext(unsigned long aRecordsToWrite,
		  	  	  	  	 char* aDataRecords,
		  	  	  	  	 unsigned long aDataRecordsSize,
		  	  	  	  	 unsigned long& aLastRecordWritten
		  	  	  	  	) throw(FMS_CPF_PrivateException);

  virtual void readnext(unsigned long aRecordsToRead,
		  	  	  	  	char* aDataRecords,
		  	  	  	  	unsigned short& aRecordsRead
		  	  	  	   ) throw(FMS_CPF_PrivateException);

  virtual void rewrite() throw(FMS_CPF_PrivateException);

  virtual void reset(unsigned long aRecordNumber) throw(FMS_CPF_PrivateException);

  void checkSwitchSubFile(bool onTime, bool fileClosed, bool& noMoreTimeSwitch);

private:

  int unixReadFd;
  int unixWriteFd;
  ACS_TRA_trace* fms_cpf_regfileTrace;

};

#endif
