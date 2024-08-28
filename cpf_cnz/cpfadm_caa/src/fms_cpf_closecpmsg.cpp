//
/** @file fms_cpf_closecpmsg.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_CloseCPMsg.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_closecpmsg.h module
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

#include "fms_cpf_closecpmsg.h"
#include "fms_cpf_cpchannel.h"
#include "fms_cpf_cpdfile.h"
#include "fms_cpf_cpdfilethrd.h"
#include "fms_cpf_cpdopenfilesmgr.h"
#include "fms_cpf_filelock.h"
#include "fms_cpf_eventalarmhndl.h"
#include "fms_cpf_common.h"

#include "fms_cpf_privateexception.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_CloseCPMsg::FMS_CPF_CloseCPMsg(const char* aInBuffer, unsigned int aInBufferSize, FMS_CPF_CpChannel *CPChan) :
FMS_CPF_CPMsg(aInBuffer, aInBufferSize, CPChan, FMS_CPF_CPMsg::MT_CLOSE_MSG)
{ 
  fms_cpf_CloseCPMsg = new (std::nothrow) ACS_TRA_trace("FMS_CPF_CloseCPMsg");
  unpack();
  fThrd = NULL;
  m_FileLock = NULL;
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FMS_CPF_CloseCPMsg::~FMS_CPF_CloseCPMsg()
{
	ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, ;);
	if(NULL != fms_cpf_CloseCPMsg)
	{
		delete fms_cpf_CloseCPMsg;
		fms_cpf_CloseCPMsg = NULL;
	}
}


//------------------------------------------------------------------------------
//      unpack
//------------------------------------------------------------------------------
void FMS_CPF_CloseCPMsg::unpack()
{
	TRACE(fms_cpf_CloseCPMsg, "%s", "Entering in unpack()");
	try
	{
		m_ReplySizeMsg = cpMsgReplySize::CloseCPMsgSize;
		inFunctionCode = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgFcCodeMask;
		protocolVersion = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgVersionMask;

		APFMIversion = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgTwobuffers;
		msgIn = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgSBSide;
		inPriority = get_ushort_cp(cpMsgInOffset::cpMsgInPrioPos);
		inSequenceNumber = get_ushort_cp(cpMsgInOffset::cpMsgInSeqNrPos);
		inFileReferenceUlong = get_ulong_cp (cpMsgInOffset::cpMsgInFileRefPos);
		inSubfileOption = (unsigned char)get_ushort_cp(cpMsgInOffset::cpMsgInSubFilOptPos);

		if(protocolVersion >= cpMsgConst::cpMsgVersion2)
		{
			  TRACE(fms_cpf_CloseCPMsg,"CloseCPMsg::unpack() protocolVersion = %d - APFMIversion = %d - msgIn = %d", protocolVersion, APFMIversion, msgIn);
			  if (APFMIversion == cpMsgConst::cpMsgTwobuffers)
			  {
				APFMIdoubleBuffers = true;
				if (msgIn == cpMsgConst::cpMsgSBSide)
				{
					inSBOCSequence = get_ushort_cp(cpMsgInOffset::cpMsgInSBOCSeqPos);
					cp_session_ptr = get_ulong_cp(cpMsgInOffset::cpMsgInSessPtrPos);
					isEXMsg = false;
				}
				else
				{
					inOCSequence = get_ushort_cp(cpMsgInOffset::cpMsgInOCSeqPos);
					cp_session_ptr = get_ulong_cp(cpMsgInOffset::cpMsgInSessPtrPos);
					isEXMsg = true;
				}
			 }
			 else
			 {
				inOCSequence = get_ushort_cp(cpMsgInOffset::cpMsgInOCSeqPos);
				cp_session_ptr = get_ulong_cp(cpMsgInOffset::cpMsgInSessPtrPos);
				APFMIdoubleBuffers = false;
				isEXMsg = true;
			 }
		}
		else
		{
			m_ReplySizeMsg = 10;
		}

		checkPriority(inPriority);
		checkSubfileOption(inSubfileOption);

		TRACE(fms_cpf_CloseCPMsg, "unpack(), inFileReferenceUlong = %lu - inSequenceNumber = %d - inSubfileOption = %d", inFileReferenceUlong, inSequenceNumber, inSubfileOption);
  }
  catch (FMS_CPF_PrivateException& ex)
  {
	  char errorMsg[1024]={0};
	  snprintf(errorMsg, 1023,"FMS_CPF_CloseCPMsg::unpack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
	  CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
	  logError(ex);
  }
  TRACE(fms_cpf_CloseCPMsg,"%s","unpack() leaving");
}

//------------------------------------------------------------------------------
//  forcedClose              Check if message is a forced close
//------------------------------------------------------------------------------
bool FMS_CPF_CloseCPMsg::forcedClose() const
{
	bool result = (inSequenceNumber == 0);
	TRACE(fms_cpf_CloseCPMsg,"forcedClose(), forcedClose=<%s>", (result ? "TRUE": "FALSE") );
	return result;
}

//------------------------------------------------------------------------------
//      go
//------------------------------------------------------------------------------
unsigned short FMS_CPF_CloseCPMsg::go(bool &reply)
{
  TRACE(fms_cpf_CloseCPMsg, "Entering in go(), msgId<%d>", inSequenceNumber);
  ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, ;);
  if (errorCode() == 0)
  {
	  // Do not work if exception during unpack
	  if(!work())
	  {
		  pack();
		  //if work did not do well, answer with error
		  reply = true;
      }
	  else
	  {
		  reply = false;
	  }
  }
  else
  {
	  pack();
	  reply = true;
  }
  TRACE(fms_cpf_CloseCPMsg, "Leaving go(), msgId<%d>", inSequenceNumber);
  return errorCode();
}

//------------------------------------------------------------------------------
//      thGo
//------------------------------------------------------------------------------
unsigned short FMS_CPF_CloseCPMsg::thGo()
{
	TRACE(fms_cpf_CloseCPMsg, "Entering in thGo(), msgId<%d>", inSequenceNumber);
	if(errorCode() == 0)
	{
		thWork();
	}
	pack();
	TRACE(fms_cpf_CloseCPMsg, "Leaving thGo(), msgId<%d>", inSequenceNumber);
	return errorCode();
}


//------------------------------------------------------------------------------
//      work
//------------------------------------------------------------------------------
bool FMS_CPF_CloseCPMsg::work()
{
	bool result = true;
	try
	{
		fThrd = CPDOpenFilesMgr::instance()->lookup(inFileReferenceUlong, m_CpName);

		if(NULL != fThrd)
		{
			CPDFile* cpdfile = fThrd->getFileRef();

			if(NULL == cpdfile)
			{
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "FMS_CPF_CloseCPMsg::work(), cpdfile object is NULL");
			}
			TRACE(fms_cpf_CloseCPMsg," work() lock file<%s> request", cpdfile->getFileName().c_str());

			m_FileLock = CPDOpenFilesMgr::instance()->lockOpen(cpdfile->getFileName(), m_CpName);

			fThrd->saveOpenLock(m_FileLock);
		
			// Send a message to it
			if( FAILURE == fThrd->putq(this) )
			{
				CPDOpenFilesMgr::instance()->unlockOpen(m_FileLock, m_CpName);
				fThrd->saveOpenLock(NULL);
				CPF_Log.Write("FMS_CPF_CloseCPMsg::work(), failed putq to cpdfile thread", LOG_LEVEL_WARN);
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "FMS_CPF_CloseCPMsg::work(), failed putq to cpdfile thread");
			}
		}
		else
		{
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "FMS_CPF_CloseCPMsg::work(), cpdfile thread is NULL");
		}
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		char errorLog[256] = {'\0'};
		ACE_OS::snprintf(errorLog, 255, "FMS_CPF_CloseCPMsg::work(), catched exception:<%d>, error<%s>", ex.errorCode(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		TRACE(fms_cpf_CloseCPMsg, "%s", errorLog);
		EventReport::instance()->reportException(&ex);

		logError(ex);

		if(NULL != fThrd)
			fThrd->shutDown(false);

		result = false;
  }
  TRACE(fms_cpf_CloseCPMsg, "Leaving work(), result<%s> leaving", (result ? "TRUE" : "FALSE"));
  return result;
}


//------------------------------------------------------------------------------
//      thWork
//------------------------------------------------------------------------------
void FMS_CPF_CloseCPMsg::thWork()
{
	TRACE(fms_cpf_CloseCPMsg, "%s", "Entering thWork()");
	
	try
	{
		//CPDOpenFilesMgr::instance()->remove(inFileReferenceUlong, true, m_CpName);

		CPDFile* cpdfile = fThrd->getFileRef();

		if(NULL == cpdfile)
		{
			// The file now is closed
			fThrd->setFileIsClosed();
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "FMS_CPF_CloseCPMsg::thWork(), cpdfile object is NULL");
		}

		cpdfile->close(inSubfileOption);

		// The file now is closed
		fThrd->setFileIsClosed();
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		char errorLog[256] = {'\0'};
		ACE_OS::snprintf(errorLog, 255, "FMS_CPF_CloseCPMsg::thWork(), catched exception:<%d>, error<%s>", ex.errorCode(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
		TRACE(fms_cpf_CloseCPMsg, "%s", errorLog);

		EventReport::instance()->reportException(&ex);
		logError(ex);
	}

	CPDOpenFilesMgr::instance()->unlockOpen(m_FileLock, m_CpName);

	fThrd->saveOpenLock(NULL);

	TRACE(fms_cpf_CloseCPMsg, "%s", "Leaving thWork()");
}

//------------------------------------------------------------------------------
//      pack
//------------------------------------------------------------------------------
void FMS_CPF_CloseCPMsg::pack()
{
	TRACE(fms_cpf_CloseCPMsg, "%s", "Entering in pack()");
	try
	{
		createOutBuffer(m_ReplySizeMsg);

		// Pack out buffer
		set_ushort_cp(cpMsgOutOffset::cpMsgOutFcCodePos, cpMsgReplyCode::CloseCPMsgCode + protocolVersion);
		set_ushort_cp(cpMsgOutOffset::cpMsgOutSeqNrPos, inSequenceNumber);
		set_ushort_cp(cpMsgOutOffset::cpMsgOutResCodePos, resultCode());

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
	  }
	  catch (FMS_CPF_PrivateException& ex)
	  {
		  char errorMsg[1024]={0};
		  snprintf(errorMsg, 1023,"FMS_CPF_CloseCPMsg::pack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		  CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		  logError(ex);
	  }
	  TRACE(fms_cpf_CloseCPMsg, "%s", "Leaving pack()");
}


//------------------------------------------------------------------------------
//      Return the sequence number for Close messages
//------------------------------------------------------------------------------
uint16_t FMS_CPF_CloseCPMsg::getOCSeq() const
{
	TRACE(fms_cpf_CloseCPMsg, "getOCSeq(), EX-OC seq<%d>", inOCSequence);
    return inOCSequence;
}


//------------------------------------------------------------------------------
//      Return the sequence number for Close messages in SB side
//------------------------------------------------------------------------------
uint16_t FMS_CPF_CloseCPMsg::getSBOCSeq() const
{
	TRACE(fms_cpf_CloseCPMsg, "getSBOCSeq() SB-OC seq<%d>", inSBOCSequence);
    return inSBOCSequence;
}

bool FMS_CPF_CloseCPMsg::messageIsFromEX()
{
	TRACE(fms_cpf_CloseCPMsg, "messageIsFromEX() result<%s>", ( isEXMsg ? "TRUE" : "FALSE"));
	return isEXMsg;
}

