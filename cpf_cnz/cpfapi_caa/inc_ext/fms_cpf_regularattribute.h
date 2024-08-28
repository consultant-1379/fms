//**********************************************************************
//
// NAME
//      FMS_CPF_RegularAttribute.H
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
// This the class holds the attributes of a regular file.
// It includes methods for getting and setting attributes,
// and for read/write the attibutes from/to file.

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
//							esalves
// SEE ALSO
//   FMS_CPF_BaseAttribute and FMS_CPF_Attribute
//
//**********************************************************************
#ifndef FMS_CPF_REGULARATTRIBUTE_H
#define FMS_CPF_REGULARATTRIBUTE_H

#include "fms_cpf_baseattribute.h"

class ACS_TRA_trace;

class FMS_CPF_RegularAttribute: public FMS_CPF_BaseAttribute
{
	friend class FMS_CPF_Attribute;

public:

  FMS_CPF_RegularAttribute ();

  FMS_CPF_RegularAttribute (const FMS_CPF_Types::regularType& regular)
  throw (FMS_CPF_PrivateException);

  virtual ~FMS_CPF_RegularAttribute ();

private:

  ACS_TRA_trace *fms_cpf_regularAttr_trace;

  const FMS_CPF_RegularAttribute& operator= (
			const FMS_CPF_Types::regularType& regular)
  throw (FMS_CPF_PrivateException);

  operator FMS_CPF_Types::fileAttributes () const;

  int binaryStoreSize () const;

  FMS_CPF_Types::fileType type () const;


  bool composite () const;

  void update();

  unsigned int getRecordLength () const;

  int getDeleteFileTimer();

  int setRecordLength(unsigned int recordLength) const;

  void setAttribute(FMS_CPF_BaseAttribute* attrp) throw(FMS_CPF_PrivateException);

  void changeActiveSubfile()  throw(FMS_CPF_PrivateException);

  void rollbackActiveSubfile() throw(FMS_CPF_PrivateException);

  unsigned long getActiveSubfile() const throw(FMS_CPF_PrivateException);

  void setChangeTime() throw(FMS_CPF_PrivateException);

  time_t getChangeTime() const;

  bool getChangeAfterClose() const throw(FMS_CPF_PrivateException);

  void setChangeAfterClose(bool changeAfterClose) throw(FMS_CPF_PrivateException);

  std::istream& read (std::istream& vistr);
  int read2 (char* bufPtr, int offset, int size);

  std::iostream& write (std::iostream& vostr);
  void write2 (char* bufPtr, int offset);

 unsigned int getNumOfRecords () const;

 void setNumOfRecords (int numOfRecords);

 void setLastReportedSubfile(std::string subfileNumber);

 void setLastReportedSubfile(unsigned long subFile);

 void getLastReportedSubfile(unsigned long &lastFile);

 void getTQmode(FMS_CPF_Types::transferMode& mode);

 void getTQname(std::string& TQname);

 void getInitTransfermode(FMS_CPF_Types::transferMode& mode);

  // NOTE! Changes in the following struct must be done with care
  // since it is written to file.
  // All members must be of a size that does not lead to uninitialized
  // characters in the written file, due to byte alignment.
  // All members has also to be initialized by ALL constructors.
  // To be able to read older versions, the size of the struct must not
  // be reduced, and new members added last in struct.
  struct regAttrStruct
  {
    unsigned long  version_;
    unsigned long  rlength_;     // Record length
    unsigned long  composite_;   // True if composite file
    long int deleteFileTimer_;
    unsigned long  notUsed2;
  };

  regAttrStruct a;
  regAttrStruct ch_a;
  bool attrIsChanged;

};

#endif
