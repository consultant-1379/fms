//
/** @file fms_cpf_opencpmsg.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_openCPMsg.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_opencpmsg.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-11
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
 *	| 1.0.0  | 2011-11-11 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_opencpmsg.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_regularcpdfile.h"
#include "fms_cpf_infinitecpdfile.h"
#include "fms_cpf_cpdopenfilesmgr.h"
#include "fms_cpf_cpdfile.h"
#include "fms_cpf_cpdfilethrd.h"
#include "fms_cpf_common.h"
#include "fms_cpf_eventalarmhndl.h"

#include "fms_cpf_privateexception.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_OpenCPMsg::FMS_CPF_OpenCPMsg(const char* aInBuffer, unsigned int aInBufferSize, FMS_CPF_CpChannel *CPChan) :
FMS_CPF_CPMsg(aInBuffer, aInBufferSize, CPChan, FMS_CPF_CPMsg::MT_OPEN_MSG)
{
  fms_cpf_OpenCPMsg = new (std::nothrow) ACS_TRA_trace("FMS_CPF_OpenCPMsg");
  outFileReferenceUlong	= 0;
  m_CloseThrd = false;
  unpack();
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FMS_CPF_OpenCPMsg::~FMS_CPF_OpenCPMsg()
{
	ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, ;);
	// Failed to open file
	if( m_CloseThrd)
		fThrd->shutDown(false);

	if(NULL != fms_cpf_OpenCPMsg)
	{
		delete fms_cpf_OpenCPMsg;
		fms_cpf_OpenCPMsg = NULL;
	}
}

//------------------------------------------------------------------------------
//      unpack
//------------------------------------------------------------------------------
void FMS_CPF_OpenCPMsg::unpack()
{
	TRACE(fms_cpf_OpenCPMsg, "%s", "Entering in unpack()");
	try
	{
		m_ReplySizeMsg = cpMsgReplySize::OpenCPMsgSize;

		// If not possible to unpack, out-params shall be 0.
		outFileReferenceUlong = 0;
		outRecordLength		= 0;
		outFileSize			= 0;
		outFileType			= 0;
		inSBOCSequence = 0;
		inOCSequence = 0;

		// Unpack in parameters
		unsigned short offset=0;

		inFunctionCode	= get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgFcCodeMask;
		protocolVersion = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgVersionMask;
		APFMIversion    = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgTwobuffers;
		msgIn   = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgSBSide;

		inPriority = get_ushort_cp(cpMsgInOffset::cpMsgInPrioPos);
		checkPriority(inPriority);

		inSequenceNumber = get_ushort_cp(cpMsgInOffset::cpMsgInSeqNrPos);

		if(protocolVersion >= cpMsgConst::cpMsgVersion2)
		{
			if(cpMsgConst::cpMsgTwobuffers == APFMIversion )
			{
				APFMIdoubleBuffers = true;
				if (msgIn == cpMsgConst::cpMsgSBSide)
				{
					inSBOCSequence = get_ushort_cp(cpMsgInOffset::cpMsgInSBOCSeqPos);
					isEXMsg = false;
				}
				else
				{
					inOCSequence = get_ushort_cp(cpMsgInOffset::cpMsgInOCSeqPos);
					isEXMsg = true;
				}
			}
			else
			{
				inOCSequence = get_ushort_cp(cpMsgInOffset::cpMsgInOCSeqPos);
				APFMIdoubleBuffers = false;
				isEXMsg = true;
			}
			cp_session_ptr = get_ulong_cp(cpMsgInOffset::cpMsgInSessPtrPos);
		}

		inFileName = get_string_cp(cpMsgInOffset::cpMsgInBufferPos);

		offset = cpMsgInOffset::cpMsgInBufferPos + 1 + inFileName.length();
		inSubfileName = get_string_cp(offset);

		offset += 1 + inSubfileName.length();
		inGenerationName = get_string_cp(offset);

		offset += 1 + inGenerationName.length();
		inAccessType = get_uchar_cp(offset);
		checkAccessType(inAccessType);

		// You can not trust CP applications to send subfileOption
		// and subfileSize in all cases. Only read the attributes
		// when they are needed.
		if (inSubfileName.length() > 0)
		{
			  // Composite file
			  offset += 1;
			  inSubfileOption = get_uchar_cp(offset);

			  checkSubfileOption(inSubfileOption);

			  if (inSubfileOption == 1)
			  {
				offset += 1;
				inSubfileSize = get_ulong_cp(offset);
			  }
			  else
			  {
				inSubfileSize = 0;
			  }
		}
		else
		{
			  // Simple file
			  inSubfileOption = 0;
			  inSubfileSize = 0;
		}

		// Construct other values
		std::string tmpFileId;
		tmpFileId = inFileName;

		if(!inSubfileName.empty())
		{
			tmpFileId += "-" + inSubfileName;
		}

		if(!inGenerationName.empty())
		{
			tmpFileId += "-" + inGenerationName;
		}

		inFileId = tmpFileId;

		if(!inFileId.isValid())
		{
			char errorLog[128] = {'\0'};
			ACE_OS::snprintf(errorLog, 127, "invalid file<%s>", inFileId.data() );
			throw FMS_CPF_PrivateException(FMS_CPF_Exception::PARAMERROR, errorLog);
		}
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		char errorMsg[1024]={0};
		snprintf(errorMsg, 1023,"FMS_CPF_OpenCPMsg::unpack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		logError(ex);
		outFileReferenceUlong = 0; // Make sure not usable...
	}
	TRACE(fms_cpf_OpenCPMsg, "%s", "Leaving unpack()");
}

bool FMS_CPF_OpenCPMsg::incSeqNrAllowed() const
{
	TRACE(fms_cpf_OpenCPMsg, "%s", "incSeqNrAllowed() return FALSE");
	return false;
}

//------------------------------------------------------------------------------
//      go
//------------------------------------------------------------------------------
unsigned short FMS_CPF_OpenCPMsg::go(bool &reply)
{
	TRACE(fms_cpf_OpenCPMsg, "Entering in go(), msgId<%d>", inSequenceNumber);
	ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, ;);
	if (errorCode() == 0)
    {
        // Do not work if exception during unpack
        if (!work()) {
            pack();
            // If work did not do well, answer with error
            reply = true;
        }
        else
            reply = false;
    }
    else
    {
        pack();
        reply = true;
    }
    TRACE(fms_cpf_OpenCPMsg, "Leaving go(), msgId<%d>", inSequenceNumber);
    return errorCode();
}

//------------------------------------------------------------------------------
//      thGo
//------------------------------------------------------------------------------
unsigned short FMS_CPF_OpenCPMsg::thGo()
{
	TRACE(fms_cpf_OpenCPMsg, "Entering thGo(), msgId<%d>", inSequenceNumber);
	if (errorCode() == 0)
	{
		thWork();
	}
	pack();
	TRACE(fms_cpf_OpenCPMsg, "Leaving thGo(), msgId<%d>", inSequenceNumber);
	return errorCode();
}

//------------------------------------------------------------------------------
//      work
//------------------------------------------------------------------------------
bool FMS_CPF_OpenCPMsg::work()   
{
  TRACE(fms_cpf_OpenCPMsg, "%s", "Entering in work()");
  bool result = true;
  try 
  {
	  // Create the thread
	  fThrd = new (std::nothrow) CPDFileThrd();

	  if(NULL != fThrd)
	  {
		  // Set Cp Name that opened the file
		  fThrd->setCpName(getCpName());

		  // Set messages sequence number
		  fThrd->setNextSeqNr(inSequenceNumber);

		  if(protocolVersion >= cpMsgConst::cpMsgVersion2) fThrd->setOnMultiWrite();

		  // start cpdfile thread that will handle the other messages
		  if( FAILURE == fThrd->open() )
		  {
			  delete fThrd;
			  throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "Failed to open cpdfile thread object");
		  }

		  // Send this open message to it
		  if( FAILURE == fThrd->putq(this) )
		  {
			  m_CloseThrd = true;
			  throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "Failed to put open message to cpdfile thread");
		  }
	  }
	  else
	  {
		  throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "Failed cpdfile object allocation");
	  }
  }
  catch(FMS_CPF_PrivateException& ex)
  {
	  char errorLog[256] = {'\0'};
	  ACE_OS::snprintf(errorLog, 255, "FMS_CPF_OpenCPMsg::work(), catched exception:<%d>, error<%s>", ex.errorCode(), ex.detailInfo().c_str() );
	  CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
	  TRACE(fms_cpf_OpenCPMsg, "%s", errorLog);
	  EventReport::instance()->reportException(&ex);
	  logError(ex);
	  result = false;
  }

  TRACE(fms_cpf_OpenCPMsg, "%s", "Leaving work()");
  return result;
}

//------------------------------------------------------------------------------
//      thWork
//------------------------------------------------------------------------------
void FMS_CPF_OpenCPMsg::thWork()
{
  CPDFile* cpdfile = NULL;
  bool infiniteFileOpened = false; 
  bool infiniteSubFileOpen = false;

  TRACE(fms_cpf_OpenCPMsg, "%s", "Entering in thWork()");
  //DBG CPF_Log.Write("MS_CPF_OpenCPMsg::thWork, IN", LOG_LEVEL_INFO);
  try 
  {
	  TRACE(fms_cpf_OpenCPMsg, "thWork(), File<%s>", inFileId.data() );
	  // What type of file is it?
	  FMS_CPF_Types::fileType fileType = DirectoryStructureMgr::instance()->getFileType(inFileId.file(), m_CpName);
    
	  // Create an instance of the appropriate type.
	  switch(fileType)
	  {
	  	  case FMS_CPF_Types::ft_REGULAR:
	  	  {
	  		  TRACE(fms_cpf_OpenCPMsg, "%s", "thwork(), regular file handling");
	  		  cpdfile = new (std::nothrow) RegularCPDFile(m_CpName);
	  	  }
	  	  break;
      
	  	  case FMS_CPF_Types::ft_INFINITE:
	  	  {
	  		  infiniteSubFileOpen = true;

	  		  if( inFileId.subfile().empty())
	  		  {
	  			  TRACE(fms_cpf_OpenCPMsg, "%s", "thwork(), infinite file handling");
	  			  cpdfile = new (std::nothrow) InfiniteCPDFile(m_CpName);
	  			  infiniteFileOpened = true;
	  		  }
	  		  else
	  		  {
	  			  TRACE(fms_cpf_OpenCPMsg, "%s", "thwork(), infinite SubFile handling");
	  			  cpdfile = new (std::nothrow) RegularCPDFile(m_CpName);
	  		  }
	  	  }
	  	  break;
     
          default:
          {
        	  char errorLog[256] = {'\0'};
        	  ACE_OS::snprintf(errorLog, 255, "FMS_CPF_OpenCPMsg::thWork(), File<%s> not found/Attribute file corrupt or missing, Blade<%s>", inFileId.data(), m_CpName.c_str());
        	  CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
        	  TRACE(fms_cpf_OpenCPMsg, "%s", errorLog);
        	  throw FMS_CPF_PrivateException(FMS_CPF_Exception::INTERNALERROR, errorLog);
          }

	  }// end switch
  }
  catch(FMS_CPF_PrivateException& ex)
  {
	  EventReport::instance()->reportException(&ex);
	  logError(ex);

	  if(NULL != cpdfile)
    	  delete cpdfile;

	  m_CloseThrd = true;
	  // Make sure not usable...
      outFileReferenceUlong = 0;
      TRACE(fms_cpf_OpenCPMsg, "%s", "thWork(), exception caught");
  }

  if( errorCode() == 0)
  {
	  FileLock* openLock = NULL;

	  TRACE(fms_cpf_OpenCPMsg,"thWork(), lock file<%s> request", inFileId.data());
	  //DBG CPF_Log.Write("MS_CPF_OpenCPMsg::thWork, lock file request with errorCode=0", LOG_LEVEL_INFO);

	  openLock = CPDOpenFilesMgr::instance()->lockOpen(inFileId.data(), m_CpName);

	  try
	  {
		  TRACE(fms_cpf_OpenCPMsg, "%s" ,"thwork(), open cpdfile object");

		  cpdfile->open(inFileId,
					    inAccessType,
						inSubfileOption,
						inSubfileSize,
						outFileReferenceUlong,
						outRecordLength,
						outFileSize,
						outFileType,
						infiniteSubFileOpen
					   );
	  }
	  catch(FMS_CPF_PrivateException& ex)
	  {
		  char errorLog[256] = {'\0'};
		  ACE_OS::snprintf(errorLog, 255, "FMS_CPF_OpenCPMsg::thWork(), on open file<%s> catched exception<%d>, detail<%s>", inFileId.data(), ex.errorCode(), ex.detailInfo().c_str() );
		  CPF_Log.Write(errorLog, LOG_LEVEL_ERROR);

		  EventReport::instance()->reportException(&ex);

		  logError(ex);
		  delete cpdfile;
		  cpdfile = NULL;
		  // Make sure not usable...
		  outFileReferenceUlong = 0;
	  }

	  TRACE(fms_cpf_OpenCPMsg,"thWork(), unlock file<%s> request", inFileId.data());

	  CPDOpenFilesMgr::instance()->unlockOpen( openLock, m_CpName);

	  if(errorCode() == 0)
	  {
		  //put cpdfile into thread object
		  fThrd->putFileRef(outFileReferenceUlong, cpdfile, infiniteFileOpened);

		  try
		  {
			  TRACE(fms_cpf_OpenCPMsg, "%s" ,"thWork(), insert the new opened file");
		   	  CPDOpenFilesMgr::instance()->insert(outFileReferenceUlong, fThrd, m_CpName, isEXMsg);
		  }
		  catch(FMS_CPF_PrivateException& ex)
		  {
			  TRACE(fms_cpf_OpenCPMsg, "%s" ,"thwork(), catched execption");
			  EventReport::instance()->reportException(&ex);
		      logError(ex);
		      m_CloseThrd = true;
		      outFileReferenceUlong = 0;
		  }
	  }
	  else
	  {
		  m_CloseThrd = true;
	  }
  }

  TRACE(fms_cpf_OpenCPMsg, "Leaving thwork(), close thread <%s>", m_CloseThrd ? "TRUE": "FALSE");
  //DBG CPF_Log.Write("MS_CPF_OpenCPMsg::thWork, OUT", LOG_LEVEL_INFO);
}

//------------------------------------------------------------------------------
//      pack
//        Build cp replay message
//------------------------------------------------------------------------------
void FMS_CPF_OpenCPMsg::pack()
{
  TRACE(fms_cpf_OpenCPMsg, "%s", "Entering in pack()");
  try
  {
		createOutBuffer(m_ReplySizeMsg);
		// Pack out buffer
		set_ushort_cp(cpMsgOutOffset::cpMsgOutFcCodePos, cpMsgReplyCode::OpenCPMsgCode + protocolVersion);
		set_ushort_cp(cpMsgOutOffset::cpMsgOutSeqNrPos, inSequenceNumber);
		set_ushort_cp(cpMsgOutOffset::cpMsgOutResCodePos, resultCode());
		set_ulong_cp(cpMsgOutOffset::cpMsgOutFileRefPos, outFileReferenceUlong);

		if(protocolVersion >= cpMsgConst::cpMsgVersion2)
		{
			if (APFMIdoubleBuffers)
			{
				if (isEXMsg)
				{
					set_ushort_cp(cpMsgOutOffset::cpMsgOutOCSeqPos, inOCSequence);
				}
				else
				{
					set_ushort_cp(cpMsgOutOffset::cpMsgOutSBOCSeqPos, inSBOCSequence);
				}
			}
			else
			{
				set_ushort_cp(cpMsgOutOffset::cpMsgOutOCSeqPos, inOCSequence);
			}
			set_ulong_cp(cpMsgOutOffset::cpMsgOutSessPtrPos, cp_session_ptr);
		}

		set_ushort_cp(cpMsgOutOffset::cpMsgOutRecLenPos, outRecordLength);
		set_ulong_cp(cpMsgOutOffset::cpMsgOutFsizePos, outFileSize);
		set_uchar_cp(cpMsgOutOffset::cpMsgOutOreplyPos, 3);
  }
  catch(FMS_CPF_PrivateException& ex)
  {
	  char errorMsg[1024]={0};
	  snprintf(errorMsg, 1023, "FMS_CPF_OpenCPMsg::pack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
	  CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
	  logError(ex);
  }
  TRACE(fms_cpf_OpenCPMsg, "%s", "Leaving pack()");
}

//------------------------------------------------------------------------------
//      Return the sequence number for Open messages
//------------------------------------------------------------------------------
uint16_t FMS_CPF_OpenCPMsg::getOCSeq() const
{

	TRACE(fms_cpf_OpenCPMsg, "getOCSeq(), EX-OC seq<%d>", inOCSequence);
    return inOCSequence;
}

//------------------------------------------------------------------------------
//      Return the sequence number for Open messages in SB side
//------------------------------------------------------------------------------
uint16_t FMS_CPF_OpenCPMsg::getSBOCSeq() const
{
	TRACE(fms_cpf_OpenCPMsg,"getSBOCSeq() SB-OC seq<%d>", inSBOCSequence);
    return inSBOCSequence;
}
