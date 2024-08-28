//**********************************************************************
//
// NAME
//      FMS_CPF_InfiniteAttribute.H
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
// This class handles the attributes for infinite files.
//

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
//            99              Ported from UNIX        SIO_I4 - SIOI7
//  	        990622  qabhall rwtool cleaned out      SIO_I8
//            990920  qabhall Version handling and nsub field in
//                            attribute file impl.    SIO_I11
//						esalves
//
// SEE ALSO
//
//
//**********************************************************************

#ifndef FMS_CPF_INFINITEATTRIBUTE_H
#define FMS_CPF_INFINITEATTRIBUTE_H

#include "fms_cpf_baseattribute.h"

class ACS_TRA_trace;
//----------------------------------------------------------------------
// Attributes for infinite file type
//----------------------------------------------------------------------

class FMS_CPF_InfiniteAttribute: public FMS_CPF_BaseAttribute
{
	friend class FMS_CPF_Attribute;

 public:
	//------------------------------------------------------------------------------
	//      Constructor
	//------------------------------------------------------------------------------
	FMS_CPF_InfiniteAttribute ();

	//------------------------------------------------------------------------------
	//      Constructor
	//------------------------------------------------------------------------------
	FMS_CPF_InfiniteAttribute (const FMS_CPF_Types::infiniteType& infinite)
		throw (FMS_CPF_PrivateException);

	//------------------------------------------------------------------------------
	//      destructor
	//------------------------------------------------------------------------------
	virtual ~FMS_CPF_InfiniteAttribute ();

 private:

	ACS_TRA_trace *fms_cpf_infiniteAttr_trace;

	//------------------------------------------------------------------------------
	//      Assignment operator
	//------------------------------------------------------------------------------
	const FMS_CPF_InfiniteAttribute& operator= (const FMS_CPF_Types::infiniteType& infinite)
		throw (FMS_CPF_PrivateException);

	//------------------------------------------------------------------------------
	//      Conversion operator
	//------------------------------------------------------------------------------
	operator FMS_CPF_Types::fileAttributes() const;

	//------------------------------------------------------------------------------
	//      Fetch binary store size
	//------------------------------------------------------------------------------
	int binaryStoreSize() const;

	//------------------------------------------------------------------------------
	//      Read file type
	//------------------------------------------------------------------------------
	FMS_CPF_Types::fileType type() const;

	//------------------------------------------------------------------------------
	//      Read file class
	//------------------------------------------------------------------------------
	bool composite() const;

	int getDeleteFileTimer();
	//------------------------------------------------------------------------------
	//      update attributes
	//------------------------------------------------------------------------------
	void update();

	//------------------------------------------------------------------------------
	//      Read record length
	//------------------------------------------------------------------------------
	unsigned int getRecordLength() const;


	//------------------------------------------------------------------------------
	//      Get extended attributes
	//------------------------------------------------------------------------------
	void extractExtAttributes(unsigned long &maxsize,unsigned long& maxtime,
							  bool& release,unsigned long& active, unsigned long& lastSent,
							  FMS_CPF_Types::transferMode &mode, std::string &tqname,
							  FMS_CPF_Types::transferMode &inittransfermode);

	//------------------------------------------------------------------------------
	//      Change attributes
	//------------------------------------------------------------------------------
	void setAttribute(FMS_CPF_BaseAttribute* attrp) throw(FMS_CPF_PrivateException);

	//------------------------------------------------------------------------------
	//      Change active subfile
	void changeActiveSubfile() throw(FMS_CPF_PrivateException);

	//------------------------------------------------------------------------------
	//      Change active subfile to the previous one
	//------------------------------------------------------------------------------
	void rollbackActiveSubfile() throw(FMS_CPF_PrivateException);


	//------------------------------------------------------------------------------
	//      Get active subfile number
	//------------------------------------------------------------------------------
	unsigned long getActiveSubfile() const throw(FMS_CPF_PrivateException);

	//------------------------------------------------------------------------------
	//      Set change time for subfile
	//------------------------------------------------------------------------------
	void setChangeTime() throw(FMS_CPF_PrivateException);

	//------------------------------------------------------------------------------
	//      Get change time for subfile
	//------------------------------------------------------------------------------
	time_t getChangeTime() const;

	//------------------------------------------------------------------------------
	//      Get changeAfterClose status for the subfile
	//------------------------------------------------------------------------------
	bool getChangeAfterClose() const throw(FMS_CPF_PrivateException);

	//------------------------------------------------------------------------------
	//      Set changeAfterClose status for the subfile
	//------------------------------------------------------------------------------
	void setChangeAfterClose(bool changeAfterClose) throw(FMS_CPF_PrivateException);

	//------------------------------------------------------------------------------
	//      Read attributes from istream
	//------------------------------------------------------------------------------
	std::istream& read(std::istream& vistr);

	int read2(char* bufPtr, int offset, int size);

	//------------------------------------------------------------------------------
	//      Write attributes to iostream
	//------------------------------------------------------------------------------
	std::iostream& write(std::iostream& vostr);

	void write2(char* bufPtr, int offset);

	unsigned int getNumOfRecords() const;

	void setNumOfRecords(int numOfRecords);

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
  struct infAttrStruct
  {
    unsigned long   version_;     // version
    unsigned long   rlength_;     // Record length
    unsigned long   maxsize_;     // Max size before release
    unsigned long   maxtime_;     // Max time before release
    unsigned long   release_;     // Release at close
    unsigned long   active_;      // Active subfile
    time_t          changetime_;	// Time when the subfile was changed
    unsigned long   changeAfterClose_;// Indicates that changetime_ is set
				                              // so no update is allowed
    unsigned long   nsub_;            // Max number of subfiles
    unsigned long   lastReportedSubfile_;  //The last subfile reported to GOH   drop 1 INGO3
    FMS_CPF_Types::transferMode  inittransfermode_;   //The first transfermode set
    FMS_CPF_Types::transferMode   transfermode_;     // Transfer mode NONE, BLOCK or FILE     //030813
    char             tqname_[33];             //030813
  };

  infAttrStruct a;     // attributes
  infAttrStruct ch_a;  // copy of attributes. Holds changes by command until nex subfileswitch
  bool attrIsChanged;  // Set if attributes changed by command
};

#endif
