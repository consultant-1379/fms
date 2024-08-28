//**********************************************************************
//
// NAME
//      fms_cpf_attribute.h
//
// COPYRIGHT Ericsson Telecom AB, Sweden 2004.
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
// This is the class used for accessing the attributes of a file.
// The class includes methods for getting and setting attributes,
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
//            991011  qabhall Check of attribute file
//                            added                   SIO_I11B
//            20040615 dapa   Updated for win2003 server.
//						esalves
//
// SEE ALSO
//   FMS_CPF_BaseAttribute, FMS_CPF_InfiniteAttribute and FMS_CPF_RegularAttribute
//
//**********************************************************************

#ifndef FMS_CPF_ATTRIUTE_H
#define FMS_CPF_ATTRIUTE_H

#include "fms_cpf_privateexception.h"
#include "fms_cpf_types.h"

class FMS_CPF_BaseAttribute;
class ACS_TRA_trace;

class FMS_CPF_Attribute {
public:

	void readFile(std::string fileName)
		throw (FMS_CPF_PrivateException);

	void saveFile(std::string fileName)
		throw (FMS_CPF_PrivateException);

	FMS_CPF_Attribute ();

	FMS_CPF_Attribute (FMS_CPF_Types::fileType ftype)
		throw (FMS_CPF_PrivateException);

	FMS_CPF_Attribute (FMS_CPF_Types::fileAttributes attributes)
		throw (FMS_CPF_PrivateException);

	FMS_CPF_Attribute (const FMS_CPF_Attribute& attribute);

	~FMS_CPF_Attribute ();

	FMS_CPF_Attribute& operator= (const FMS_CPF_Attribute& attribute);

	operator FMS_CPF_Types::fileAttributes () const;

	FMS_CPF_Types::fileType type () const;

	int binaryStoreSize () const;

	bool composite () const;

	void update ();

	unsigned int getRecordLength () const;

	int getDeleteFileTimer ();

	void extractAttributes(FMS_CPF_Types::fileType & ftype, bool& composite, unsigned int &recordLength, int &deleteFileTimer);

	void extractExtAttributes(unsigned long &maxsize,
                             unsigned long& maxtime,
                             bool& release,
                             unsigned long& active,
                             unsigned long& lastSent,
                             FMS_CPF_Types::transferMode &mode,
                             std::string& transferqueue,
                             FMS_CPF_Types::transferMode &inittransfermode);

	void setAttribute (const FMS_CPF_Attribute& attribute)
		throw (FMS_CPF_PrivateException);

	void changeActiveSubfile ()
		throw (FMS_CPF_PrivateException);

	void rollbackActiveSubfile ()
		throw (FMS_CPF_PrivateException);

	unsigned long getActiveSubfile () const
		throw (FMS_CPF_PrivateException);

	void getActiveSubfile (std::string &str)
		throw (FMS_CPF_PrivateException);

	void setChangeTime ()
		throw (FMS_CPF_PrivateException);

	time_t getChangeTime () const;

	bool getChangeAfterClose () const
		throw (FMS_CPF_PrivateException);

	void setChangeAfterClose (bool changeAfterClose)
		throw (FMS_CPF_PrivateException);

	unsigned int getNumOfRecords () const;

	void setNumOfRecords (int numOfRecords);

	void getLastReportedSubfile(unsigned long &lastFile);

	void setLastReportedSubfile(std::string subfileNumber);

	void setLastReportedSubfile(unsigned long subFile);

	void getTQmode(FMS_CPF_Types::transferMode& mode);

	void getTQname(std::string& TQname);

	void getInitTransfermode(FMS_CPF_Types::transferMode &mode);

private:
   FMS_CPF_BaseAttribute* attrp_;

   // NOTE! Changes in the following struct must be done with care
   // since it is written to file.
   // All members must be of a size that does not lead to uninitialized
   // characters in the written file, due to byte alignment.
   // All members has also to be initialized by ALL constructors.
   struct attrFileStruct
   {
      unsigned long chSum;
      unsigned long size;
      unsigned long version;
      unsigned long type;
   };
   attrFileStruct a;
   ACS_TRA_trace* cpfAttributeTrace;
};


#endif /* FMS_CPF_ATTRIUTE_H */

