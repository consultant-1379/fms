///**********************************************************************
//
// NAME
//      FMS_CPF_BaseAttribute.H
//
// COPYRIGHT Ericsson Telecom AB, Sweden 1999.
// All rights reserved.
//
// The Copyright to the computer program(s) herein is the property of
// Ericsson Telecom AB, Sweden.
// The program(s) may be used and/or copied only with the written
// permission from Ericsson Telecom AB or in accordance with
// the terms and conditions stipulated in the agreement/contract under
// which the program(s) have been supplied.

// DESCRIPTION
//
// This is the base class used for accessing the attributes of a file.
// This class is inherited by all different attribute type classes.

// DOCUMENT NO
//

// AUTHOR
//

// REVISION
//
//

// CHANGES
//
//	RELEASE REVISION HISTORY
//
//	REV NO		DATE		NAME 		DESCRIPTION
//            99              Ported from UNIX to NT  SIO_I4 - SIO_I7
//            990620  qabhall rwtool cleaning  SIO_I8
//            991011  qabhall File access of attribute file
//                            changed                   SIO_I11B
//						esalves
// SEE ALSO
//   FMS_CPF_Attribute
//
//**********************************************************************
#ifndef FMS_CPF_BASEATTRIBUTE_H
#define FMS_CPF_BASEATTRIBUTE_H

#include "fms_cpf_privateexception.h"
#include "fms_cpf_types.h"

const unsigned long MAXSIZELIMIT = 9999999;
const unsigned long MAXTIMELIMIT = 599999940;

class FMS_CPF_Attribute;

class FMS_CPF_BaseAttribute
{
  friend class FMS_CPF_Attribute;

public:
	FMS_CPF_BaseAttribute ();

	virtual ~FMS_CPF_BaseAttribute ();

	virtual std::istream& read (std::istream& vistr) = 0;

	virtual std::iostream& write (std::iostream& vostr) = 0;

	virtual int read2 (char* bufPtr, int offset, int size) = 0;

	virtual void write2 (char* bufPtr, int offset) = 0;

protected:
	virtual operator FMS_CPF_Types::fileAttributes () const = 0;

	virtual FMS_CPF_Types::fileType type () const = 0;

	virtual int binaryStoreSize () const = 0;

	virtual bool composite () const = 0;

	virtual unsigned int getRecordLength () const = 0;

	virtual int getDeleteFileTimer() = 0;

	virtual void setAttribute(FMS_CPF_BaseAttribute* attrp) throw(FMS_CPF_PrivateException) = 0;

	virtual void changeActiveSubfile() throw(FMS_CPF_PrivateException) = 0;

	virtual void rollbackActiveSubfile() throw(FMS_CPF_PrivateException) = 0;

	virtual void update() = 0;

	virtual unsigned long getActiveSubfile() const throw(FMS_CPF_PrivateException) = 0;

	virtual void setChangeTime() throw(FMS_CPF_PrivateException) = 0;

	virtual time_t getChangeTime() const = 0;

	virtual bool getChangeAfterClose() const throw(FMS_CPF_PrivateException) = 0;

	virtual void setChangeAfterClose(bool changeAfterClose) throw(FMS_CPF_PrivateException) = 0;

	virtual unsigned int getNumOfRecords () const = 0;

	virtual void setNumOfRecords (int numOfRecords) = 0;

	virtual void getLastReportedSubfile (unsigned long &lastFile) = 0;

	virtual void setLastReportedSubfile (std::string subfileNumber) = 0;

    virtual void setLastReportedSubfile(unsigned long subFile) = 0;

	virtual void getTQmode(FMS_CPF_Types::transferMode& mode) = 0;

	virtual void getTQname(std::string& TQname) = 0; //030813

	virtual void getInitTransfermode(FMS_CPF_Types::transferMode &mode) = 0; //030821

	int refcount_;                // Reference counter
};

#endif
