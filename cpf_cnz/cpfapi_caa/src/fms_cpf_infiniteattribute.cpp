//**********************************************************************
//
// NAME
//      FMS_CPF_InfiniteAttribute.C
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
// See FMS_CPF_InfiniteAttribute.H
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
//							esalves
//
// SEE ALSO
//
//
//**********************************************************************

#include "fms_cpf_infiniteattribute.h"
#include "fms_cpf_api_trace.h"
#include "ACS_TRA_trace.h"

#include <ace/ACE.h>

#include <sys/timeb.h>

namespace{
	std::string fms_cpf_infiniteAttr_name = "FMS_CPF_InfiniteAttribute";
}

const unsigned long infiniteAttributeVersion = 0x0100;
//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_InfiniteAttribute::FMS_CPF_InfiniteAttribute()
{
    fms_cpf_infiniteAttr_trace = new (std::nothrow) ACS_TRA_trace(fms_cpf_infiniteAttr_name);

    TRACE(fms_cpf_infiniteAttr_trace, "%s", "Default constructor");

	a.version_ = infiniteAttributeVersion;
	a.rlength_ = 0;
	a.maxsize_ = 0;
	a.maxtime_ = 0;
	a.release_  = false;
	a.active_ = 0;
	a.changetime_ = 0;
	a.changeAfterClose_ = false;
	a.nsub_= 0;
	a.lastReportedSubfile_ = 0;
	a.inittransfermode_ = FMS_CPF_Types::tm_UNDEF;
	a.transfermode_ = FMS_CPF_Types::tm_UNDEF;
	ACE_OS::memset(a.tqname_, '\0', sizeof(a.tqname_)/sizeof(a.tqname_[0]));
	ACE_OS::memset(ch_a.tqname_, '\0', sizeof(ch_a.tqname_)/sizeof(ch_a.tqname_[0]));
	attrIsChanged = false;
	// copy attributes
	ch_a = a;

}

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_InfiniteAttribute::FMS_CPF_InfiniteAttribute(const FMS_CPF_Types::infiniteType& infinite)
throw (FMS_CPF_PrivateException)
{
	fms_cpf_infiniteAttr_trace = new (std::nothrow) ACS_TRA_trace(fms_cpf_infiniteAttr_name);
	TRACE(fms_cpf_infiniteAttr_trace, "%s", "Constructor by infinite type");

	a.version_ = infiniteAttributeVersion;
	a.rlength_ = infinite.rlength;

	if(a.rlength_ == 0)
	{
		TRACE(fms_cpf_infiniteAttr_trace, "%s", "Constructor(), Illegal value of record length");
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::ILLVALUE, "Record length");
	}

	a.active_ = infinite.active;
	a.changetime_ = 0;
	a.changeAfterClose_ = false;
	a.nsub_= 0;
	a.lastReportedSubfile_ = infinite.lastReportedSubfile;
	a.inittransfermode_ = FMS_CPF_Types::tm_UNDEF;
	a.transfermode_ = FMS_CPF_Types::tm_UNDEF;

	ACE_OS::memset(a.tqname_, '\0', sizeof(a.tqname_)/sizeof(a.tqname_[0]));
	ACE_OS::memset(ch_a.tqname_, '\0', sizeof(ch_a.tqname_)/sizeof(ch_a.tqname_[0]));

	attrIsChanged = false;

	a.maxsize_ = infinite.maxsize;

	if(a.maxsize_ > MAXSIZELIMIT)
	{
		TRACE(fms_cpf_infiniteAttr_trace, "%s", "Constructor(), Illegal value of  max size");
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::ILLVALUE, "Max size");
	}

	a.maxtime_ = infinite.maxtime;

	if(a.maxtime_ > MAXTIMELIMIT)
	{
		TRACE(fms_cpf_infiniteAttr_trace, "%s", "Constructor(), Illegal value of  max time");
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::ILLVALUE, "Max time");
	}

	a.release_ = infinite.release;
	a.transfermode_ = infinite.mode;
	a.inittransfermode_ = infinite.inittransfermode;
	ACE_OS::strncpy(a.tqname_, infinite.transferQueue, sizeof(a.tqname_)/sizeof(a.tqname_[0]));
	// copy attributes
	ch_a = a;
	TRACE(fms_cpf_infiniteAttr_trace, "%s", "Leaving Constructor()");
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FMS_CPF_InfiniteAttribute::~FMS_CPF_InfiniteAttribute()
{
   if (NULL != fms_cpf_infiniteAttr_trace)
	   delete fms_cpf_infiniteAttr_trace;
}

//------------------------------------------------------------------------------
//      Assignment operator
//------------------------------------------------------------------------------
const FMS_CPF_InfiniteAttribute& FMS_CPF_InfiniteAttribute::operator= (const FMS_CPF_Types::infiniteType& infinite)
throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_infiniteAttr_trace, "%s", "Entering in Operator=()");

	a.maxsize_ = infinite.maxsize;

	if(a.maxsize_ > MAXSIZELIMIT)
	{
		TRACE(fms_cpf_infiniteAttr_trace, "%s", "Constructor(), Illegal value of  max size");
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::ILLVALUE, "Max size");
	}

	a.maxtime_ = infinite.maxtime;

	if(a.maxtime_ > MAXTIMELIMIT)
	{
		TRACE(fms_cpf_infiniteAttr_trace, "%s", "Constructor(), Illegal value of  max time");
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::ILLVALUE, "Max time");
	}

	a.release_ = infinite.release;
	a.transfermode_ = infinite.mode;
	a.inittransfermode_ = infinite.inittransfermode;
	ACE_OS::strncpy(a.tqname_, infinite.transferQueue, sizeof(a.tqname_)/sizeof(a.tqname_[0]));

	TRACE(fms_cpf_infiniteAttr_trace, "%s", "Leaving Operator=()");
	return *this;
}

//------------------------------------------------------------------------------
//      Conversion operator
//------------------------------------------------------------------------------
FMS_CPF_InfiniteAttribute::operator FMS_CPF_Types::fileAttributes () const
{
  FMS_CPF_Types::fileAttributes attributes;
  attributes.ftype = FMS_CPF_Types::ft_INFINITE;
  attributes.infinite.rlength = a.rlength_;
  attributes.infinite.maxsize = a.maxsize_;
  attributes.infinite.maxtime = a.maxtime_;
  attributes.infinite.release = a.release_ ? true: false;
  attributes.infinite.active = a.active_;
  attributes.infinite.lastReportedSubfile = a.lastReportedSubfile_;
  attributes.infinite.inittransfermode = a.inittransfermode_;
  attributes.infinite.mode = a.transfermode_;
  ACE_OS::strncpy(attributes.infinite.transferQueue, a.tqname_, sizeof(a.tqname_)/sizeof(a.tqname_[0]) );

  return attributes;
}

//------------------------------------------------------------------------------
//      Calculate the size of the attributes
//------------------------------------------------------------------------------
int FMS_CPF_InfiniteAttribute::binaryStoreSize() const
{
  return sizeof(a);
}

//------------------------------------------------------------------------------
//      Read file type
//------------------------------------------------------------------------------
FMS_CPF_Types::fileType FMS_CPF_InfiniteAttribute::type() const
{
  return FMS_CPF_Types::ft_INFINITE;
}

//------------------------------------------------------------------------------
//      Read file class
//------------------------------------------------------------------------------
bool FMS_CPF_InfiniteAttribute::composite() const
{
  return true;
}

int FMS_CPF_InfiniteAttribute::getDeleteFileTimer()
{
  return -1;
}

//------------------------------------------------------------------------------
//      Update attributes
//------------------------------------------------------------------------------
void FMS_CPF_InfiniteAttribute::update()
{
	TRACE(fms_cpf_infiniteAttr_trace, "%s", "Entering in update()");
	a.maxsize_ = ch_a.maxsize_;
	a.maxtime_ = ch_a.maxtime_;
	a.release_ = ch_a.release_;
	if((a.inittransfermode_ == FMS_CPF_Types::tm_UNDEF) || (a.inittransfermode_ == FMS_CPF_Types::tm_NONE))
	{
		a.inittransfermode_ = ch_a.inittransfermode_;
	}
	a.transfermode_ = ch_a.transfermode_;
	ACE_OS::strncpy(a.tqname_, ch_a.tqname_, sizeof(a.tqname_)/sizeof(a.tqname_[0]));

	TRACE(fms_cpf_infiniteAttr_trace, "%s", "Leaving update()");
}

//------------------------------------------------------------------------------
//      Read record length
//------------------------------------------------------------------------------
unsigned int FMS_CPF_InfiniteAttribute::getRecordLength() const
{
  return a.rlength_;
}

//------------------------------------------------------------------------------
//      Change attributes
//------------------------------------------------------------------------------
void FMS_CPF_InfiniteAttribute::setAttribute(FMS_CPF_BaseAttribute* attrp)
throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_infiniteAttr_trace, "%s", "Entering in setAttributes()");

	FMS_CPF_InfiniteAttribute* attribute = dynamic_cast<FMS_CPF_InfiniteAttribute*>(attrp);

	// dynamic_cast can return NULL
	if(NULL != attribute)
	{
		a.active_ = attribute->a.active_;
		a.lastReportedSubfile_ = attribute->a.lastReportedSubfile_;

		ch_a.maxsize_ = attribute->a.maxsize_;
		ch_a.maxtime_ = attribute->a.maxtime_;
		ch_a.release_ = attribute->a.release_;
		ch_a.inittransfermode_ = attribute->a.inittransfermode_;
		ch_a.transfermode_ = attribute->a.transfermode_;
		ACE_OS::strncpy(ch_a.tqname_, attribute->a.tqname_, sizeof(ch_a.tqname_)/sizeof(ch_a.tqname_[0]));

		attrIsChanged = true;
	}
    TRACE(fms_cpf_infiniteAttr_trace, "%s", "Leaving setAttributes()");
}

//------------------------------------------------------------------------------
//      Get extended attributes
//------------------------------------------------------------------------------
void FMS_CPF_InfiniteAttribute::extractExtAttributes(unsigned long& maxsize, unsigned long& maxtime,
													  bool& release,unsigned long& active, unsigned long& lastSent, FMS_CPF_Types::transferMode& mode,
													  std::string& tqname, FMS_CPF_Types::transferMode& inittransfermode)
{
	TRACE(fms_cpf_infiniteAttr_trace, "%s", "Entering in extractExtAttributes()");

	maxsize = a.maxsize_;
	maxtime = a.maxtime_;
	release = a.release_ > 0;
	active = a.active_;
	lastSent = a.lastReportedSubfile_;
	mode   = a.transfermode_;
	tqname = a.tqname_;
	inittransfermode = a.inittransfermode_;
    TRACE(fms_cpf_infiniteAttr_trace, "%s", "Leaving extractExtAttributes()");
}

//------------------------------------------------------------------------------
//      Change active subfile
//------------------------------------------------------------------------------
void FMS_CPF_InfiniteAttribute::changeActiveSubfile() throw(FMS_CPF_PrivateException)
{
	a.active_++;
	TRACE(fms_cpf_infiniteAttr_trace, "changeActiveSubfile(), new active subfile<%lu>", a.active_);
}

//------------------------------------------------------------------------------
//      Change active subfile to the previous one
//------------------------------------------------------------------------------
void FMS_CPF_InfiniteAttribute::rollbackActiveSubfile() throw(FMS_CPF_PrivateException)
{
	a.active_--;
	TRACE(fms_cpf_infiniteAttr_trace, "rollbackActiveSubfile(), new active subfile<%lu>", a.active_);
}

//------------------------------------------------------------------------------
//      Get active subfile number
//------------------------------------------------------------------------------
unsigned long FMS_CPF_InfiniteAttribute::getActiveSubfile() const throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_infiniteAttr_trace, "getActiveSubfile(), active subfile<%lu>", a.active_);
	return a.active_;
}

//------------------------------------------------------------------------------
//      Set change time for subfile
//------------------------------------------------------------------------------
void FMS_CPF_InfiniteAttribute::setChangeTime() throw(FMS_CPF_PrivateException)
{
	struct timeb timebuffer;
 	ftime(&timebuffer);

	a.changetime_ = timebuffer.time;
	TRACE(fms_cpf_infiniteAttr_trace, "setChangeTime(), time=<%d>", a.changetime_);
}

//------------------------------------------------------------------------------
//      Get change time for subfile
//------------------------------------------------------------------------------
time_t FMS_CPF_InfiniteAttribute::getChangeTime() const
{
	TRACE(fms_cpf_infiniteAttr_trace, "%s", "getChangeTime()");
	return a.maxtime_ ? (a.changetime_ + a.maxtime_) : 0;
}

//------------------------------------------------------------------------------
//      Get changeAfterClose status for the subfile
//------------------------------------------------------------------------------
bool FMS_CPF_InfiniteAttribute::getChangeAfterClose() const throw(FMS_CPF_PrivateException)
{
	bool result = (a.changeAfterClose_ > 0);
	TRACE(fms_cpf_infiniteAttr_trace, "getChangeAfterClose(), is <%s>", (result ? "TRUE" : "FALSE"));
	return result;
}

//------------------------------------------------------------------------------
//      Set changeAfterClose status for the subfile
//------------------------------------------------------------------------------
void FMS_CPF_InfiniteAttribute::setChangeAfterClose(bool changeAfterClose) throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_infiniteAttr_trace, "setChangeAfterClose(), to <%s>", (changeAfterClose ? "TRUE" : "FALSE"));
	a.changeAfterClose_ = changeAfterClose;
}

//------------------------------------------------------------------------------
//      Read attributes from istream
//------------------------------------------------------------------------------
std::istream& FMS_CPF_InfiniteAttribute::read(std::istream& vistr)
{
	TRACE(fms_cpf_infiniteAttr_trace,"%s", "Entering in read()");
	unsigned char majorVersion_;
	unsigned char minorVersion_;
	unsigned long notUsed = 0;

	int changeAfterClose_tmp = false;
	int release_tmp = false;
	vistr.read((char*)&majorVersion_,1);
	vistr.read((char*)&minorVersion_,1);

	if((majorVersion_ == 1) && (minorVersion_ == 1))
	{
		TRACE(fms_cpf_infiniteAttr_trace,"%s", "read(), new version");
		vistr.read((char*)&a.rlength_,2);
		vistr.read((char*)&a.maxsize_,4);
		vistr.read((char*)&a.maxtime_,4);
		vistr.read((char*)&release_tmp,4);
		vistr.read((char*)&a.active_,4);
		vistr.read((char*)&a.changetime_,4);
		vistr.read((char*)&changeAfterClose_tmp,4);
		vistr.read((char*)&a.nsub_,4);
		vistr.read((char*)&a.lastReportedSubfile_,4);
		vistr.read((char*)&a.inittransfermode_,4);
		vistr.read((char*)&a.transfermode_,4);
		vistr.read((char*)&a.tqname_,4);
		vistr.read((char*)&notUsed,4);
	}
	else
	{
		TRACE(fms_cpf_infiniteAttr_trace,"%s", "read(), old version");
		a.rlength_ = (minorVersion_ << 8) + majorVersion_;
		vistr.read((char*)&a.maxsize_,4);
		vistr.read((char*)&a.maxtime_,4);
		vistr.read((char*)&release_tmp,4);
		vistr.read((char*)&a.active_,4);
		vistr.read((char*)&a.changetime_,4);
		vistr.read((char*)&changeAfterClose_tmp,4);
		a.nsub_ = 0;
	}

	a.changeAfterClose_ = (changeAfterClose_tmp > 0);
	a.release_ = (release_tmp > 0);
	TRACE(fms_cpf_infiniteAttr_trace,"%s", "Leaving read()");
	return vistr;
}

int FMS_CPF_InfiniteAttribute::read2(char* bufPtr, int offset, int size)
{
	TRACE(fms_cpf_infiniteAttr_trace,"%s", "Entering in read2()");

	// abort if filesize larger than size of attributes
	if((unsigned)(size - offset) > sizeof(a))
	    return -1;

	int bytesToCopy = sizeof(a);

	if((unsigned)(size - offset) < sizeof(a))
	{
	    bytesToCopy = size - offset;
	    ACE_OS::memset((char*)&a, 0, sizeof(a));
	}

	ACE_OS::memcpy((char*)&a, bufPtr + offset, bytesToCopy);
	switch (a.version_ )
	{
		case 0x100L:
			break;
		default:
		{
			// non handled version
			ACE_OS::memset((char*)&a, 0, sizeof(a));
			return -1;
		}
	}
	// copy attributes
	ch_a = a;
	TRACE(fms_cpf_infiniteAttr_trace,"%s", "Leaving read2()");

	return 0;
}

//------------------------------------------------------------------------------
//      Write attributes to ostream
//------------------------------------------------------------------------------

std::iostream&
FMS_CPF_InfiniteAttribute::write(std::iostream& vostr)
{
	TRACE(fms_cpf_infiniteAttr_trace,"%s", "Entering in write()");

	unsigned char majorVersion_ = 1;
	unsigned char minorVersion_ = 1;
	unsigned long notUsed = 0;
	int release_tmp = (int)a.release_;
	int changeAfterClose_tmp = (int)a.changeAfterClose_;

	vostr.write((char*)&majorVersion_,1);
	vostr.write((char*)&minorVersion_,1);
	vostr.write((char*)&a.rlength_,2);
	vostr.write((char*)&a.maxsize_,4);
	vostr.write((char*)&a.maxtime_,4);
	vostr.write((char*)&release_tmp,4);
	vostr.write((char*)&a.active_,4);
	vostr.write((char*)&a.changetime_,4);
	vostr.write((char*)&changeAfterClose_tmp,4);
	vostr.write((char*)&a.nsub_,4);
	vostr.write((char*)&a.lastReportedSubfile_,4);
	vostr.write((char*)&a.inittransfermode_,4);
	vostr.write((char*)&a.transfermode_,4);
	vostr.write((char*)&a.tqname_,4);
	vostr.write((char*)&notUsed,4);

	TRACE(fms_cpf_infiniteAttr_trace,"%s", "Leaving write()");

	return vostr;
}

void FMS_CPF_InfiniteAttribute::write2(char* bufPtr, int offset)
{
	TRACE(fms_cpf_infiniteAttr_trace,"%s", "Entering in write2()");
	const int tqLength = 33;
	unsigned long tmp_maxsize = ch_a.maxsize_;
	unsigned long tmp_maxtime = ch_a.maxtime_;
	unsigned long tmp_release = ch_a.release_;
	FMS_CPF_Types::transferMode tmp_mode = ch_a.transfermode_;
	FMS_CPF_Types::transferMode tmp_inittransfermode = ch_a.inittransfermode_;
	char tmp_tqname[tqLength] = {'\0'};

	ACE_OS::strncpy(tmp_tqname, ch_a.tqname_, tqLength);

	ch_a = a;

	if(attrIsChanged)
	{
		ch_a.maxsize_ = tmp_maxsize;
		ch_a.maxtime_ = tmp_maxtime;
		ch_a.release_ = tmp_release;
		ch_a.transfermode_ = tmp_mode;
		ACE_OS::strncpy(ch_a.tqname_, tmp_tqname, tqLength);
		ch_a.inittransfermode_ = tmp_inittransfermode;
		attrIsChanged = false;
	}
	ACE_OS::memcpy(bufPtr + offset,(char*)&ch_a, sizeof(ch_a));

	TRACE(fms_cpf_infiniteAttr_trace,"%s", "Leaving write2()");
}

unsigned int FMS_CPF_InfiniteAttribute::getNumOfRecords() const
{
	return 0;
}

void FMS_CPF_InfiniteAttribute::setNumOfRecords(int numOfRecords)
{
	numOfRecords = 0;
}

//------------------------------------------------------------------------------
//      Update with the last written subfile to GOH
//------------------------------------------------------------------------------
void FMS_CPF_InfiniteAttribute::setLastReportedSubfile(std::string subfileNumber)
{
	TRACE(fms_cpf_infiniteAttr_trace, "setLastReportedSubfile(), last reported subfile<%s>", subfileNumber.c_str());
	int subfilenumber = ACE_OS::atoi(subfileNumber.c_str());
	a.lastReportedSubfile_ = subfilenumber;
	TRACE(fms_cpf_infiniteAttr_trace, "%s", "Leaving setLastReportedSubfile()");
}

//------------------------------------------------------------------------------
//      Update with the last written subfile to GOH
//------------------------------------------------------------------------------
void FMS_CPF_InfiniteAttribute::setLastReportedSubfile(unsigned long subFile)
{
	a.lastReportedSubfile_ = subFile;
}

//------------------------------------------------------------------------------
//      Get the last written subfile to GOH
//------------------------------------------------------------------------------
void FMS_CPF_InfiniteAttribute::getLastReportedSubfile(unsigned long& lastFile)
{
	lastFile = a.lastReportedSubfile_;
	TRACE(fms_cpf_infiniteAttr_trace, "getLastReportedSubfile(), last subfile<%lu>", lastFile);
}

//------------------------------------------------------------------------------
//      Get TQ name for the file in CPF
//------------------------------------------------------------------------------
void FMS_CPF_InfiniteAttribute::getTQname(std::string& TQname)
{
    TQname = a.tqname_;
    TRACE(fms_cpf_infiniteAttr_trace, "getTQname(), TQ name<%s>", TQname.c_str());
}

//------------------------------------------------------------------------------
//      Get transfer mode for the file in CPF
//------------------------------------------------------------------------------
void FMS_CPF_InfiniteAttribute::getTQmode(FMS_CPF_Types::transferMode& mode)
{
    mode = a.transfermode_;
    TRACE(fms_cpf_infiniteAttr_trace, "getTQmode(), Transfer mode<%d>", mode);
}

//------------------------------------------------------------------------------
//      Get initial transfer mode for the file in CPF
//------------------------------------------------------------------------------
void FMS_CPF_InfiniteAttribute::getInitTransfermode(FMS_CPF_Types::transferMode& mode)
{
    mode = a.inittransfermode_;
    TRACE(fms_cpf_infiniteAttr_trace, "getInitTransfermode(),init Transfer mode<%d>", mode);
}

