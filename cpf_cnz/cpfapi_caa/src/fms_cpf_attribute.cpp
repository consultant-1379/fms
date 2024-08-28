//**********************************************************************
//
// NAME
//      FMS_CPF_Attribute.C
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
// See FMS_CPF_Attribute.H
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
//                            added                   SIO_I12
//            010618  qablake INGO3 DROP1 Add pMode in saveFile
//						::open, needed when doing create
//			  020506	qabtjer	Removed datalink code
//				20020917 QABDAPA Added trace.
//							esalves
// SEE ALSO
//   FMS_CPF_BaseAttribute, FMS_CPF_InfiniteAttribute and FMS_CPF_RegularAttribute
//
//**********************************************************************

#include "fms_cpf_attribute.h"
#include "fms_cpf_baseattribute.h"
#include "fms_cpf_regularattribute.h"
#include "fms_cpf_infiniteattribute.h"
#include "fms_cpf_fileid.h"
#include "fms_cpf_eventhandler.h"
#include "fms_cpf_api_trace.h"

#include <ace/ACE.h>
#include <fstream>
#include <fcntl.h>

#include "ACS_TRA_trace.h"

using namespace std;
using namespace ACE_OS;

const std::string ATTRIBUTE = ".attribute";
#define NEWATTRIBUTE


//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_Attribute::FMS_CPF_Attribute() :
attrp_(NULL)
{
	cpfAttributeTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_Attribute");
}

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_Attribute::FMS_CPF_Attribute(FMS_CPF_Types::fileType ftype)
throw (FMS_CPF_PrivateException) :
attrp_(0)
{
	cpfAttributeTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_Attribute");

	switch (ftype)
	{
	    case FMS_CPF_Types::ft_REGULAR:
	    	attrp_ = new FMS_CPF_RegularAttribute();
	    break;

	    case FMS_CPF_Types::ft_INFINITE:
	    	attrp_ = new FMS_CPF_InfiniteAttribute();
	    break;
	    default:
	    	throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "Undefined file type");
   }
   attrp_->refcount_ = 1;
}

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_Attribute::FMS_CPF_Attribute(FMS_CPF_Types::fileAttributes fileattr)
throw(FMS_CPF_PrivateException) :
attrp_(NULL)
{
	cpfAttributeTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_Attribute");

	switch (fileattr.ftype)
	{
    	case FMS_CPF_Types::ft_REGULAR:
    		attrp_ = new FMS_CPF_RegularAttribute(fileattr.regular);
        break;

    	case FMS_CPF_Types::ft_INFINITE:
    		attrp_ = new FMS_CPF_InfiniteAttribute(fileattr.infinite);
        break;
     default:
       throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::TYPEERROR);
   }
   attrp_->refcount_ = 1;
}

//------------------------------------------------------------------------------
//      Copy constructor
//------------------------------------------------------------------------------
FMS_CPF_Attribute::FMS_CPF_Attribute(const FMS_CPF_Attribute& attribute)
{
	cpfAttributeTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_Attribute");
    attrp_ = attribute.attrp_;

    if(NULL != attrp_)
    {
    	// use gcc build-in thread safe function
    	__sync_fetch_and_add(&attrp_->refcount_, 1);
    }

    TRACE(cpfAttributeTrace, "attrp copied, ref:<%d>", attrp_->refcount_);
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FMS_CPF_Attribute::~FMS_CPF_Attribute()
{
	if(NULL != attrp_)
    {

		// use gcc build-in thread safe function
		if( __sync_sub_and_fetch(&attrp_->refcount_, 1) <= 0)
		{
			delete attrp_;
			attrp_ = NULL;
			TRACE(cpfAttributeTrace, "%s", "DCTOR attrp_ deleted!");
		}
    }

	if (NULL != cpfAttributeTrace)
	   delete cpfAttributeTrace;
}

//------------------------------------------------------------------------------
//      Assignment operator
//------------------------------------------------------------------------------
FMS_CPF_Attribute& FMS_CPF_Attribute::operator= (const FMS_CPF_Attribute& attribute)
{
	TRACE(cpfAttributeTrace, "%s", "FMS_CPF_Attribute::operator()");

	if(NULL != attrp_)
	{
		// use gcc build-in thread safe function
		if( __sync_sub_and_fetch(&attrp_->refcount_, 1) <= 0)
		{
			delete attrp_;
			TRACE(cpfAttributeTrace, "%s", "operator= attrp_ deleted!");
		}
	}

	attrp_ = attribute.attrp_;

	if(NULL != attrp_)
	{
		// use gcc build-in thread safe function
		__sync_fetch_and_add(&attrp_->refcount_, 1);
	}

	TRACE(cpfAttributeTrace, "attrp assigned, ref:<%d>", attrp_->refcount_);

    return *this;
}

//------------------------------------------------------------------------------
//      Conversion operator
//------------------------------------------------------------------------------
FMS_CPF_Attribute::operator FMS_CPF_Types::fileAttributes() const
{
   return attrp_->operator FMS_CPF_Types::fileAttributes();
}

//------------------------------------------------------------------------------
//      Calculate the size of the attributes
//------------------------------------------------------------------------------
int FMS_CPF_Attribute::binaryStoreSize() const
{
   int count;

   count = sizeof (a);
   count += attrp_->binaryStoreSize ();
   TRACE(cpfAttributeTrace, "binaryStoreSize()\n count = %d", count);
   return count;
}

//------------------------------------------------------------------------------
//      Read file type
//------------------------------------------------------------------------------
FMS_CPF_Types::fileType FMS_CPF_Attribute::type() const
{
   TRACE(cpfAttributeTrace, "%s", "type()");
   return attrp_->type ();
}

//------------------------------------------------------------------------------
//      Read file class
//------------------------------------------------------------------------------
bool FMS_CPF_Attribute::composite() const
{
   TRACE(cpfAttributeTrace, "%s", "composite()");
   return attrp_->composite ();
}

//------------------------------------------------------------------------------
//      update attributes
//------------------------------------------------------------------------------
void FMS_CPF_Attribute::update()
{
   TRACE(cpfAttributeTrace, "%s", "update()");
   attrp_->update ();
}

//------------------------------------------------------------------------------
//      Read recordlength
//------------------------------------------------------------------------------
unsigned int FMS_CPF_Attribute::getRecordLength() const
{
   TRACE(cpfAttributeTrace, "getRecordLength()\n record length = %d", attrp_->getRecordLength() );
   return attrp_->getRecordLength ();
}

//------------------------------------------------------------------------------
//      Read deletefiletimer 
//------------------------------------------------------------------------------
int FMS_CPF_Attribute::getDeleteFileTimer()
{
  TRACE(cpfAttributeTrace, "getDeleteFileTimer\n record length = %d", attrp_->getDeleteFileTimer());
  return attrp_->getDeleteFileTimer ();
}

//------------------------------------------------------------------------------
//      Change attributes
//------------------------------------------------------------------------------
void FMS_CPF_Attribute::setAttribute(const FMS_CPF_Attribute& attribute)
throw (FMS_CPF_PrivateException)
{
   TRACE(cpfAttributeTrace, "%s", "setAttribute()");
   if (attribute.type () == FMS_CPF_Types::ft_TEXT) {
      TRACE(cpfAttributeTrace, "%s", "setAttribute()\n Not an modifiable file type");
      throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::TYPEERROR);
   }
   attrp_->setAttribute (attribute.attrp_);
}

//------------------------------------------------------------------------------
//      Change active subfile
//------------------------------------------------------------------------------
void FMS_CPF_Attribute::changeActiveSubfile() throw(FMS_CPF_PrivateException)
{
   TRACE(cpfAttributeTrace, "%s", "changeActiveSubfile()");
   attrp_->changeActiveSubfile ();
}

//------------------------------------------------------------------------------
//      Change active subfile to the previous one
//------------------------------------------------------------------------------
void FMS_CPF_Attribute::rollbackActiveSubfile() throw(FMS_CPF_PrivateException)
{
  TRACE(cpfAttributeTrace, "%s", "rollbackActiveSubfile()");
  attrp_->rollbackActiveSubfile ();
}

//------------------------------------------------------------------------------
//      Get active subfile number
//------------------------------------------------------------------------------
unsigned long FMS_CPF_Attribute::getActiveSubfile() const
throw (FMS_CPF_PrivateException)
{
   TRACE(cpfAttributeTrace, "getActiveSubfile()\n Subfile = %10u", attrp_->getActiveSubfile());
   return attrp_->getActiveSubfile ();
}

//------------------------------------------------------------------------------
//      Get active subfile number as string
//------------------------------------------------------------------------------
void FMS_CPF_Attribute::getActiveSubfile(std::string &str)
throw (FMS_CPF_PrivateException)
{
   char subfile[SUB_LENGTH - 1];
   unsigned long active = attrp_->getActiveSubfile ();
   ACE_OS::snprintf (subfile, SUB_LENGTH, "%.10u", active);
   str.assign(subfile);
   TRACE(cpfAttributeTrace, "getActiveSubfile()\n Subfile = %s", str.c_str());
}

//------------------------------------------------------------------------------
//      Set change time for the subfile
//------------------------------------------------------------------------------
void FMS_CPF_Attribute::setChangeTime() throw(FMS_CPF_PrivateException)
{
   TRACE(cpfAttributeTrace, "%s", "setChangeTime()");
   attrp_->setChangeTime ();
}

//------------------------------------------------------------------------------
//      Get change time for the subfile
//------------------------------------------------------------------------------
time_t FMS_CPF_Attribute::getChangeTime() const
{
   return attrp_->getChangeTime ();
}

//------------------------------------------------------------------------------
//      Get changeAfterClose status for the subfile
//------------------------------------------------------------------------------
bool FMS_CPF_Attribute::getChangeAfterClose() const
throw (FMS_CPF_PrivateException)
{
   TRACE(cpfAttributeTrace, "getChangeAfterClose()\n Returns %s",attrp_->getChangeAfterClose()?"true":"false");
   return attrp_->getChangeAfterClose ();
}

//------------------------------------------------------------------------------
//      Set changeAfterClose status for the subfile
//------------------------------------------------------------------------------
void FMS_CPF_Attribute::setChangeAfterClose(bool changeAfterClose)
throw (FMS_CPF_PrivateException)
{

   TRACE(cpfAttributeTrace, "setChangeAfterClose()\n Change = %s",
	         changeAfterClose?"true":"false");
   attrp_->setChangeAfterClose (changeAfterClose);
}

//------------------------------------------------------------------------------
//      Extract common attributes
//------------------------------------------------------------------------------
void FMS_CPF_Attribute::extractAttributes(FMS_CPF_Types::fileType & ftype,
                                      bool& compositeFile, unsigned int &recordLength, int &deleteFileTimer)
{
   ftype = type();      // type
   compositeFile = composite();
   recordLength = getRecordLength();
   deleteFileTimer = getDeleteFileTimer();
   TRACE(cpfAttributeTrace, "extractAttributes()\n Composite file = %s, recordLength = %d, deleteFileTimer = %d", compositeFile?"true":"false", recordLength,deleteFileTimer);
}

//------------------------------------------------------------------------------
//      extractExtAttributes()
//------------------------------------------------------------------------------
void FMS_CPF_Attribute::extractExtAttributes(unsigned long &maxsize,
                                         unsigned long& maxtime,
                                         bool& release,
                                         unsigned long& active,
                                         unsigned long& lastSent,
                                         FMS_CPF_Types::transferMode &mode,
                                         string& transferqueue,
                                         FMS_CPF_Types::transferMode &inittransfermode)
{
   if (type()==FMS_CPF_Types::ft_INFINITE)
   {
      FMS_CPF_InfiniteAttribute* in_afttrp = dynamic_cast<FMS_CPF_InfiniteAttribute*>(attrp_);
      // dynamic_cast can return NULL
      if(NULL != in_afttrp)
    	  in_afttrp->extractExtAttributes(maxsize, maxtime, release, active, lastSent, mode, transferqueue, inittransfermode);
   }
   else
   {
      maxsize = 0;
      maxtime = 0;
      release = 0;
      active = 0;
      lastSent = 0;
      transferqueue = "";
      mode = FMS_CPF_Types::tm_UNDEF;
      inittransfermode = FMS_CPF_Types::tm_UNDEF;
   }
   TRACE(cpfAttributeTrace, "extractExtAttributes(), maxSize:<%u>, maxTime:<%u>, release:<%s>, active:<%u>, transferqueue:<%s>, mode:<%d>, inittransfermode:<%d>",
	         maxsize,
	         maxtime,
	         (release ? "true":"false"),
	         active,
	         transferqueue.c_str(),
	         mode,
	         inittransfermode);

}

//------------------------------------------------------------------------------
//      Read attributes from file
//------------------------------------------------------------------------------
void
FMS_CPF_Attribute::readFile(std::string fileName)
throw (FMS_CPF_PrivateException)

{
   TRACE(cpfAttributeTrace, "readFile()\n Filename = %s", fileName.c_str());

#ifdef NEWATTRIBUTE
   string fName = fileName + ATTRIBUTE;
   long fileSize = 0;
  // int fileSize = 0; //Mapping ACE
   int rStatus = 0;
 //  size_t rStatus = 0;
   int sStatus = 0;
   int cStatus = 0;
   int extStatus = 0;
   unsigned long calcChSum = 0;

   char* bufPtr;
   unsigned long attrSize = 0;

   // to avoid warning
   attrSize++;

   int fd = ::open (fName.c_str(), O_RDONLY | O_BINARY);
   //FILE *fd = ACE_OS::fopen(fName.c_str(),"r"); //Mapping ACE
 if(fd != -1){

	   TRACE(cpfAttributeTrace, "readFile() open Filename = %s", fName.c_str());
	   // store size of file
        fileSize = ::lseek(fd, 0, SEEK_END);

      if(fileSize != -1L)
	//   if(fileSize > 0)
	   {
		 TRACE(cpfAttributeTrace, "readFile() fseek Filename = %s", fName.c_str());
		 // set file pointer to beginning of file
         sStatus = ::lseek(fd, 0, SEEK_SET);
		  //sStatus = ACE_OS::fseek(fd, 0, SEEK_SET);
         if(sStatus != -1L) {
			 TRACE(cpfAttributeTrace, "readFile() sstatus Filename = %s", fName.c_str());
			 bufPtr = new char [fileSize];
             rStatus = ::read(fd,bufPtr,fileSize);
		//	   rStatus = ACE_OS::fread(bufPtr, sizeof(char)*fileSize, 1, fd);
             if(rStatus != -1L) {
				   TRACE(cpfAttributeTrace, "readFile() rStatus Filename = %s", fName.c_str());
				   ACE_OS::memcpy((char*)&a,bufPtr,sizeof(a));
				   //  checksum check of file
				   for(int i = sizeof(a.chSum); i < fileSize; i++) {
					   calcChSum += bufPtr[i];
				   }
				   if(calcChSum == a.chSum) {
					   switch (a.type) {
					   	   case FMS_CPF_Types::ft_REGULAR:
					   		   attrp_ = new FMS_CPF_RegularAttribute;
					   		   break;
					   	   case FMS_CPF_Types::ft_INFINITE:
					   		   attrp_ = new FMS_CPF_InfiniteAttribute;
					   		   break;
					   	   default:
					   		   delete [] bufPtr;
					   		   cStatus = ::close (fd);
					   		  // cStatus = ACE_OS::fclose(fd);
					   		   FMS_CPF_PrivateException ex (FMS_CPF_PrivateException::INTERNALERROR,
					   				   "Undefined file type");
					   		   FMS_CPF_EventHandler::instance()->event(ex);
					   		   throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR,
					   				   	   	   	   	   	   	   "Undefined file type");
					   }
                  attrp_->refcount_ = 1;
                  // read specific attrbutes
                  extStatus = attrp_->read2 (bufPtr,sizeof(a),fileSize);
                  TRACE(cpfAttributeTrace, "readFile() read2 Filename = %s", fName.c_str());

                  if(extStatus == -1) {
                     delete attrp_;
                  }
               }
               else {
                  delete [] bufPtr;
                cStatus = ::close (fd);
               //   cStatus = ACE_OS::fclose(fd);
                  FMS_CPF_PrivateException ex (FMS_CPF_PrivateException::INTERNALERROR,
                     "Attribute file checksum error");
                  FMS_CPF_EventHandler::instance()->event(ex);
                  throw  FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR,
                          	  	  	  	  	  	  "Attribute file checksum error");
               }
            }
            delete [] bufPtr;
         }
      }
   }
   if((fd == -1) || (sStatus == -1L) || (rStatus == -1L) ||
      (cStatus == -1L) || (extStatus == -1)) {
//   if((fd == NULL) || (sStatus == 0) || (rStatus < 1) ||
//         (cStatus == EOF) || (extStatus == -1)) {
         FMS_CPF_PrivateException ex (FMS_CPF_PrivateException::INTERNALERROR,
            "Attribute file error");
         FMS_CPF_EventHandler::instance()->event(ex);
         throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::INTERNALERROR, "Attribute file error");
      }

   cStatus = ::close (fd);
  // cStatus = ACE_OS::fclose(fd);

#else
   string fName = fileName + ATTRIBUTE;
   ifstream vistr (fName.c_str(), ios::in | ios::binary);
   if (vistr.good ()) {
      unsigned long ftype;
      vistr.read((char*)&ftype,4);
      switch (ftype) {
      case FMS_CPF_Types::ft_REGULAR:
         attrp_ = new FMS_CPF_RegularAttribute;
         break;
      case FMS_CPF_Types::ft_INFINITE:
         attrp_ = new FMS_CPF_InfiniteAttribute;
         break;
      default:
         FMS_CPF_PrivateException ex (FMS_CPF_PrivateException::INTERNALERROR,
            "Undefined file type");
         FMS_CPF_EventHandler::instance()->event(ex);
         throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR,
        		 	 	 	 	 	 	 "Undefined file type");
      }
      attrp_->refcount_ = 1;
      attrp_->read (vistr);
   }
   else {

      throw  FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR,
    		  	  	  	  	  	  	  "Corrupt or missing attribute file for file");
   }
#endif
}


//------------------------------------------------------------------------------
//      Write attributes to file
//------------------------------------------------------------------------------
void
FMS_CPF_Attribute::saveFile(std::string fileName)
throw (FMS_CPF_PrivateException)
{
   TRACE(cpfAttributeTrace, "saveFile()\n Filename = %s", fileName.c_str());

#ifdef NEWATTRIBUTE
   string fName = fileName + ATTRIBUTE;
   unsigned long bufSize = binaryStoreSize();
   char* bufPtr = new char [bufSize];
   int wStatus = 0;
   int cStatus = 0;
   a.size = bufSize;
   a.type = attrp_->type();
   a.version = 0x100;
   a.chSum = 0;
   ACE_OS::memcpy(bufPtr, (char*)&a, sizeof(a));
   attrp_->write2 (bufPtr,sizeof(a));
   
   // calculate checksum and store in buffer
   for(unsigned long i = sizeof(a.chSum); i < bufSize; i++) {
      a.chSum += bufPtr[i];
   }
   ACE_OS::memcpy(bufPtr, (char*)&a, sizeof(a));
   int fd = ::open(fName.c_str(), O_CREAT | O_WRONLY | _O_BINARY, S_IREAD | S_IWRITE);
   //int fd;
   //fd  = ::open(fName.c_str(), ACE_TEXT("w"));
   if(fd != -1) {
      wStatus = ::write(fd,bufPtr,bufSize);
      cStatus = ::close (fd);
   }

   delete [] bufPtr;

//   if((fd == -1) || (wStatus == -1) || (cStatus == -1)) {
//      FMS_CPF_PrivateException ex (FMS_CPF_PrivateException::PHYSICALERROR);
//      ex << "Corrupt or missing attribute file for file '"
//         << fName.c_str() << "'";
//      throw ex;
//   }

#else
   string fName = fileName + ATTRIBUTE;

//#ifdef WIN32
//   fstream vostr (fName.c_str(),ios::out | ios::binary);
//#else
   fstream vostr (fName.c_str(),ios::out);
//#endif
   if (vostr.good ()) {
      unsigned long type = attrp_->type();
      vostr.write((char*)&type,4);

      attrp_->write (vostr);
   }
   else {
      throw  FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR,
    		  	  	  	  	  	  	  "corrupt or missing attribute file for file '");
   }
#endif
}

//------------------------------------------------------------------------------
//      setLastReportedSubfile()
//------------------------------------------------------------------------------
void FMS_CPF_Attribute::setLastReportedSubfile(std::string subfileNumber)
{
   TRACE(cpfAttributeTrace, "setLastReportedSubfile()\n subfile = %s",
	         subfileNumber.c_str());
   attrp_->setLastReportedSubfile(subfileNumber);
}

void FMS_CPF_Attribute::setLastReportedSubfile(unsigned long subFile)
{
	TRACE(cpfAttributeTrace, "setLastReportedSubfile() subfile:<%lu>", subFile);
	attrp_->setLastReportedSubfile(subFile);
}

//------------------------------------------------------------------------------
//      getLastReportedSubfile()
//------------------------------------------------------------------------------
void
FMS_CPF_Attribute::getLastReportedSubfile(unsigned long &lastFile)
{
   attrp_->getLastReportedSubfile(lastFile);
   TRACE(cpfAttributeTrace, "getLastReportedSubfile()\n Last reported = %u", lastFile);
}

//------------------------------------------------------------------------------
//      getTQMode()
//------------------------------------------------------------------------------
void
FMS_CPF_Attribute::getTQmode(FMS_CPF_Types::transferMode& mode)
{
   attrp_->getTQmode(mode);
   TRACE(cpfAttributeTrace, "getTQmode()\n Mode = %d", mode);
}

//------------------------------------------------------------------------------
//      getTQName()
//------------------------------------------------------------------------------
void
FMS_CPF_Attribute::getTQname(string& name)
{
   attrp_->getTQname(name);
   TRACE(cpfAttributeTrace, "getTQname()\n TQ name = %s", name.c_str());
}

//------------------------------------------------------------------------------
//      getInitTransferMode()
//------------------------------------------------------------------------------
void
FMS_CPF_Attribute::getInitTransfermode(FMS_CPF_Types::transferMode& mode)
{
   attrp_->getInitTransfermode(mode);
   TRACE(cpfAttributeTrace, "getInitTransfermode()\n initmode = %d", mode);
}

