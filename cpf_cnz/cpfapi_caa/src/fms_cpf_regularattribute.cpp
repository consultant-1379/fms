//**********************************************************************
//
// NAME
//      FMS_CPF_RegularAttribute.C
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
// See FMS_CPF_RegularAttribute.H
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
//            99              Ported from UNIX to NT  SIO_I4 - SIO_I7
//  	        990622          rwtool cleaned out      SIO_I8
//            991018  qabhall Attributes stored in a struct.
//                            Handling of file read and write
//                            modified. See FMS_CPF_Attributes.c SIO_I12
//							esalves
// SEE ALSO
//
//
//**********************************************************************
#include "fms_cpf_regularattribute.h"
#include "fms_cpf_api_trace.h"
#include "ACS_TRA_trace.h"

#include <ace/ACE.h>

namespace{
	std::string fms_cpf_regularAttr_name = "FMS_CPF_RegularAttribute";
}

const unsigned long regularAttributeVersion = 0x0100;

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------

FMS_CPF_RegularAttribute::FMS_CPF_RegularAttribute()
{
  fms_cpf_regularAttr_trace = new (std::nothrow) ACS_TRA_trace(fms_cpf_regularAttr_name);
  a.version_ = regularAttributeVersion;
  a.rlength_ = 0;
  a.composite_ = false;
  a.deleteFileTimer_ = -1;
  a.notUsed2 = 0;
  attrIsChanged = false;
  ch_a = a;

}


//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------

FMS_CPF_RegularAttribute::FMS_CPF_RegularAttribute(const FMS_CPF_Types::regularType& regular)
throw (FMS_CPF_PrivateException)
{
  fms_cpf_regularAttr_trace = new (std::nothrow) ACS_TRA_trace(fms_cpf_regularAttr_name);
  a.version_ = regularAttributeVersion;
  a.rlength_ = regular.rlength;
  a.composite_ = regular.composite;
  a.deleteFileTimer_ = regular.deleteFileTimer;
  a.notUsed2 = 0;
  attrIsChanged = false;
  if (a.rlength_ == 0)
  {
	  TRACE(fms_cpf_regularAttr_trace, "%s", "Constructor(), Illegal value of record length");
	  throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::ILLVALUE, "Record length");
  }
  ch_a = a;
}


//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------

FMS_CPF_RegularAttribute::~FMS_CPF_RegularAttribute()
{
	if (NULL != fms_cpf_regularAttr_trace)
		delete fms_cpf_regularAttr_trace;
}


//------------------------------------------------------------------------------
//      Assignment operator
//------------------------------------------------------------------------------

const FMS_CPF_RegularAttribute& FMS_CPF_RegularAttribute::operator= (const FMS_CPF_Types::regularType& regular)
throw (FMS_CPF_PrivateException)
{
  TRACE(fms_cpf_regularAttr_trace, "%s", "Entering in Operator=()");
  a.deleteFileTimer_ = regular.deleteFileTimer;
  TRACE(fms_cpf_regularAttr_trace, "%s", "Leaving Operator=()"); 
  //UNUSED(regular);  // ??????
  return *this;
}


//------------------------------------------------------------------------------
//      Conversion operator
//------------------------------------------------------------------------------

FMS_CPF_RegularAttribute::operator FMS_CPF_Types::fileAttributes () const
{
  TRACE(fms_cpf_regularAttr_trace, "%s", "Entering in fileAttibutes=()"); 
  FMS_CPF_Types::fileAttributes attributes;
  attributes.ftype = FMS_CPF_Types::ft_REGULAR;
  attributes.regular.rlength = a.rlength_;
  attributes.regular.composite = a.composite_ ? true: false;
  attributes.regular.deleteFileTimer = a.deleteFileTimer_;
  TRACE(fms_cpf_regularAttr_trace, "%s", "leaving fileAttributes()");
  return attributes;
}


//------------------------------------------------------------------------------
//      Calculate the size of the attributes
//------------------------------------------------------------------------------
int FMS_CPF_RegularAttribute::binaryStoreSize() const
{
  return sizeof(a);
}


//------------------------------------------------------------------------------
//      Read file type
//------------------------------------------------------------------------------
FMS_CPF_Types::fileType FMS_CPF_RegularAttribute::type() const
{
  return FMS_CPF_Types::ft_REGULAR;
}


//------------------------------------------------------------------------------
//      Read file class
//------------------------------------------------------------------------------
bool FMS_CPF_RegularAttribute::composite() const
{
  return (a.composite_ > 0);
}

//------------------------------------------------------------------------------
//      Read record length
//------------------------------------------------------------------------------
unsigned int FMS_CPF_RegularAttribute::getRecordLength() const
{
	TRACE(fms_cpf_regularAttr_trace, "getRecordLength(), record length <%d>", a.rlength_);
	return a.rlength_;
}

//------------------------------------------------------------------------------
//      Read deletefile timer 
//------------------------------------------------------------------------------
int FMS_CPF_RegularAttribute::getDeleteFileTimer()
{
	TRACE(fms_cpf_regularAttr_trace, "getDeleteFileTimer(), deletefiletimer <%d>", a.deleteFileTimer_);
	return a.deleteFileTimer_;
}

//------------------------------------------------------------------------------
//      Change attributes
//------------------------------------------------------------------------------
void FMS_CPF_RegularAttribute::setAttribute(FMS_CPF_BaseAttribute* attrp)
throw (FMS_CPF_PrivateException)
{
  TRACE(fms_cpf_regularAttr_trace, "%s", "Entering in setAttributes()");
  FMS_CPF_RegularAttribute* attribute = dynamic_cast<FMS_CPF_RegularAttribute*>(attrp);
  // dynamic_cast can return NULL
  if(NULL != attribute)
  {
  	ch_a.deleteFileTimer_ = attribute->a.deleteFileTimer_; 
        TRACE(fms_cpf_regularAttr_trace,"setattribute deletimervalue change to :<%d>",ch_a.deleteFileTimer_);
        attrIsChanged = true;
  }
  TRACE(fms_cpf_regularAttr_trace, "%s", "Leaving setAttributes()");
  //UNUSED(attrp);
  //throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::TYPEERROR);
}

//------------------------------------------------------------------------------
//      Change active subfile
//------------------------------------------------------------------------------
void FMS_CPF_RegularAttribute::changeActiveSubfile()
throw (FMS_CPF_PrivateException)
{
  throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::TYPEERROR);
}


//------------------------------------------------------------------------------
//      Change active subfile to the previous one
//------------------------------------------------------------------------------
void FMS_CPF_RegularAttribute::rollbackActiveSubfile()
throw (FMS_CPF_PrivateException)
{
  throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::TYPEERROR);
}

//------------------------------------------------------------------------------
//      Get active subfile number
//------------------------------------------------------------------------------

unsigned long FMS_CPF_RegularAttribute::getActiveSubfile() const
throw (FMS_CPF_PrivateException)
{
  throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::TYPEERROR);
}


//------------------------------------------------------------------------------
//      Set change time for the subfile
//------------------------------------------------------------------------------
void FMS_CPF_RegularAttribute::setChangeTime ()
throw (FMS_CPF_PrivateException)
{
  throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::TYPEERROR);
}

//------------------------------------------------------------------------------
//      Update attributes
//------------------------------------------------------------------------------
void FMS_CPF_RegularAttribute::update()
{
 TRACE(fms_cpf_regularAttr_trace, "%s", "Entering in update()");
 a.deleteFileTimer_= ch_a.deleteFileTimer_;
 TRACE(fms_cpf_regularAttr_trace,"update function deleteFileTimervalue change to :<%d>",a.deleteFileTimer_);
 TRACE(fms_cpf_regularAttr_trace, "%s", "Leaving update()");
 
}

//------------------------------------------------------------------------------
//      Get change time for the subfile
//------------------------------------------------------------------------------
time_t FMS_CPF_RegularAttribute::getChangeTime() const
{
  return 0;
}

//------------------------------------------------------------------------------
//      Get changeAfterClose status for the subfile
//------------------------------------------------------------------------------
bool FMS_CPF_RegularAttribute::getChangeAfterClose() const
throw(FMS_CPF_PrivateException)
{
  throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::TYPEERROR);
}


//------------------------------------------------------------------------------
//      Set changeAfterClose status for the subfile
//------------------------------------------------------------------------------
void FMS_CPF_RegularAttribute::setChangeAfterClose(bool changeAfterClose)
throw(FMS_CPF_PrivateException)
{
	UNUSED(changeAfterClose);
	throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::TYPEERROR);
}


//------------------------------------------------------------------------------
//      Read attributes from RWvistream
//------------------------------------------------------------------------------
istream& FMS_CPF_RegularAttribute::read (istream& vistr) //this method shall be replaced with read2 SIO_I12
{
    int composite_tmp = false;
    vistr.read((char*)&a.rlength_,2);
    vistr.read((char*)&composite_tmp,4);

    a.composite_  = (composite_tmp > 0);
    return vistr;
}

int FMS_CPF_RegularAttribute::read2 (char* bufPtr, int offset, int size)  //SIO_I12
{
  TRACE(fms_cpf_regularAttr_trace, "%s", "Entering in read2()");

  if((unsigned)(size - offset) > sizeof(a))// abort if filesize larger than size of attributes
    return -1;
  int bytesToCopy = sizeof(a);

  if((unsigned)(size - offset) < sizeof(a))
  {
    bytesToCopy = size - offset;
    ACE_OS::memset((char*)&a, 0, sizeof(a));
  }


  ACE_OS::memcpy((char*)&a,bufPtr + offset,bytesToCopy);

  switch (a.version_ )
  {
    case 0x100L: break;
    default:
    {
      // non handled version
    	ACE_OS::memset((char*)&a, 0, sizeof(a));
      return -1;
    }
  }
  ch_a = a;
  TRACE(fms_cpf_regularAttr_trace, "%s", "Leaving read2()");
  return 0;
}

//------------------------------------------------------------------------------
//      Write attributes to iostream
//------------------------------------------------------------------------------

iostream& FMS_CPF_RegularAttribute::write (iostream& vostr)  //this method shall be replaced with write2 SIO_I12
{
   int composite_tmp = (int)a.composite_;
   vostr.write((char*)&a.rlength_,2);
   vostr.write((char*)&composite_tmp,4);

   return vostr;
}

void FMS_CPF_RegularAttribute::write2(char* bufPtr, int offset)
{
	TRACE(fms_cpf_regularAttr_trace, "%s", "Entering in write2()");
        long int tmp_deleteFileTimer = ch_a.deleteFileTimer_;
        ch_a = a;
        if(attrIsChanged)
	{
           ch_a.deleteFileTimer_ = tmp_deleteFileTimer ;
        }
    	ACE_OS::memcpy(bufPtr + offset,(char*)&ch_a,sizeof(ch_a));
    	TRACE(fms_cpf_regularAttr_trace,"write2  deleteFileTimervalue change to :<%d>",ch_a.deleteFileTimer_);
    	TRACE(fms_cpf_regularAttr_trace, "%s", "Leaving write2()");
}

unsigned int FMS_CPF_RegularAttribute::getNumOfRecords() const
{
	return 0;
}

void FMS_CPF_RegularAttribute::setNumOfRecords(int numOfRecords)
{
	UNUSED(numOfRecords);
}


//------------------------------------------------------------------------------
//      Update with the last written subfile to GOH
//------------------------------------------------------------------------------
void FMS_CPF_RegularAttribute::setLastReportedSubfile(std::string subfileNumber)
{
  UNUSED(subfileNumber);
}

//------------------------------------------------------------------------------
//      Update with the last written subfile to GOH
//------------------------------------------------------------------------------
void FMS_CPF_RegularAttribute::setLastReportedSubfile(unsigned long subFile)
{
	 UNUSED(subFile);
}

//------------------------------------------------------------------------------
//      Get the last written subfile to GOH
//------------------------------------------------------------------------------
void FMS_CPF_RegularAttribute::getLastReportedSubfile(unsigned long &lastFile)
{
	lastFile = 0;
}

//------------------------------------------------------------------------------
//      Get TQ name for the file in CPF
//------------------------------------------------------------------------------
void FMS_CPF_RegularAttribute::getTQname(std::string& TQname)  //030813
{
  UNUSED(TQname);
}

//------------------------------------------------------------------------------
//      Get transfer mode for the file in CPF
//------------------------------------------------------------------------------
void FMS_CPF_RegularAttribute::getTQmode(FMS_CPF_Types::transferMode& mode)    //030813
{
	UNUSED(mode);
}

void FMS_CPF_RegularAttribute::getInitTransfermode(FMS_CPF_Types::transferMode& mode)    //030821
{
	UNUSED(mode);
}

