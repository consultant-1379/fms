//
/** @file fms_cpf_readrandcpmsg.cpp
 *	@brief
 *	Class method implementation for fms_cpf_readrandcpmsg.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_readrandcpmsg.h module
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
#include "fms_cpf_readrandcpmsg.h"
#include "fms_cpf_cpdfile.h"
#include "fms_cpf_cpdfilethrd.h"
#include "fms_cpf_cpdopenfilesmgr.h"
#include "fms_cpf_common.h"

#include "fms_cpf_privateexception.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

/*===============================================================================
	ROUTINE: constructor
 =============================================================================== */
FMS_CPF_ReadrandCPMsg::FMS_CPF_ReadrandCPMsg(const char* aInBuffer, unsigned int aInBufferSize, FMS_CPF_CpChannel *CPChan) :
FMS_CPF_CPMsg(aInBuffer, aInBufferSize, CPChan, FMS_CPF_CPMsg::MT_READAND_MSG)
{
  fms_cpf_ReadrandCPMsg = new (std::nothrow) ACS_TRA_trace("FMS_CPF_ReadrandCPMsg");
  unpack();
}

/*===============================================================================
	ROUTINE: destructor
 =============================================================================== */
FMS_CPF_ReadrandCPMsg::~FMS_CPF_ReadrandCPMsg()
{
	ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, ;);
	if(NULL != fms_cpf_ReadrandCPMsg)
	{
		delete fms_cpf_ReadrandCPMsg;
		fms_cpf_ReadrandCPMsg = NULL;
	}
}

/*===============================================================================
	ROUTINE: unpack
 =============================================================================== */
void FMS_CPF_ReadrandCPMsg::unpack()
{
	TRACE(fms_cpf_ReadrandCPMsg, "%s", "Entering in unpack()");
	try
	{
		m_ReplySizeMsg = cpMsgReplySize::ReadrandCPMsgSize;

		inFunctionCode	= get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgFcCodeMask;
		protocolVersion = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgVersionMask;
 
		inPriority = get_ushort_cp(cpMsgInOffset::cpMsgInPrioPos);
		inSequenceNumber = get_ushort_cp(cpMsgInOffset::cpMsgInSeqNrPos);
		inFileReferenceUlong = get_ulong_cp(cpMsgInOffset::cpMsgInFileRefPos);
		inRecordNumber = get_ulong_cp(cpMsgInOffset::cpMsgInRecNrPos);

		if(protocolVersion >= cpMsgConst::cpMsgVersion2)
		{
			cp_session_ptr = get_ulong_cp(cpMsgInOffset::cpMsgInSessPtrPos);
		}

		checkPriority(inPriority);

		TRACE(fms_cpf_ReadrandCPMsg, "unpack(), inFileReferenceUlong = %lu - inRecordNumber = %d - inSequenceNumber = %d", inFileReferenceUlong, inRecordNumber, inSequenceNumber);
	}
	catch (FMS_CPF_PrivateException & ex)
	{
		char errorMsg[1024]={0};
		snprintf(errorMsg, 1023,"FMS_CPF_ReadrandCPMsg::unpack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		logError(ex);
	}
  	TRACE(fms_cpf_ReadrandCPMsg, "%s", "Leaving unpack()");
}
/*===============================================================================
	ROUTINE: go
 =============================================================================== */
unsigned short FMS_CPF_ReadrandCPMsg::go(bool& reply)
{
	TRACE(fms_cpf_ReadrandCPMsg, "Entering in go(), msgId<%d>", inSequenceNumber);
	ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, ;);
	if (errorCode() == 0) {
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
    TRACE(fms_cpf_ReadrandCPMsg, "Leaving go(), msgId<%d>", inSequenceNumber);
    return errorCode();
}

/*===============================================================================
	ROUTINE: thGo
 =============================================================================== */
unsigned short FMS_CPF_ReadrandCPMsg::thGo()
{
	TRACE(fms_cpf_ReadrandCPMsg, "Entering in thGo(), msgId<%d>", inSequenceNumber);
	if (errorCode() == 0)
	{
		thWork();
	}
	pack();
	TRACE(fms_cpf_ReadrandCPMsg, "Leaving thGo(), msgId<%d>", inSequenceNumber);
	return errorCode();
}


/*===============================================================================
	ROUTINE: work
 =============================================================================== */
bool FMS_CPF_ReadrandCPMsg::work()
{
	TRACE(fms_cpf_ReadrandCPMsg, "%s", "Entering in work()");
	bool result =true;
	try
	{
		fThrd = CPDOpenFilesMgr::instance()->lookup(inFileReferenceUlong, m_CpName);

		if( FAILURE == fThrd->putq(this) )
		{
			CPF_Log.Write("FMS_CPF_ReadrandCPMsg::work(), failed putq to cpdfile thread", LOG_LEVEL_WARN);
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "FMS_CPF_ReadrandCPMsg::work(), failed putq to cpdfile thread");
		}
	}
	catch (FMS_CPF_PrivateException& ex)
	{
		logError(ex);
		result = false;
	}
	TRACE(fms_cpf_ReadrandCPMsg, "Leaving work, result=<%s>", (result ? "TRUE" : "FALSE") );
	return result;
}

/*===============================================================================
	ROUTINE: thWork
 =============================================================================== */
void FMS_CPF_ReadrandCPMsg::thWork()
{
  TRACE(fms_cpf_ReadrandCPMsg,"%s","ReadrandCPMsg::thWork() entering");
  try 
  {
	  CPDFile * cpdfile = fThrd->getFileRef();
	  unsigned short recordLength = cpdfile->getRecordLength();
	  createOutBuffer(m_ReplySizeMsg + recordLength);
	  char* buffer = get_buffer_out(m_ReplySizeMsg);

  	  cpdfile->readrand(inRecordNumber, buffer);
  }
  catch (FMS_CPF_PrivateException & ex)
  {
    logError(ex);
  }
  TRACE(fms_cpf_ReadrandCPMsg,"%s","ReadrandCPMsg::thWork() leaving");
}


/*===============================================================================
	ROUTINE: pack
 =============================================================================== */
void FMS_CPF_ReadrandCPMsg::pack()
{
  TRACE(fms_cpf_ReadrandCPMsg,"%s","ReadrandCPMsg::pack entering");
  try
  {
    // If exception in work, use this smaller size instead.
    createOutBuffer(m_ReplySizeMsg);

    // Pack out buffer
    set_ushort_cp(cpMsgOutOffset::cpMsgOutFcCodePos, cpMsgReplyCode::ReadrandCPMsgCode + protocolVersion);
    set_ushort_cp(cpMsgOutOffset::cpMsgOutSeqNrPos, inSequenceNumber);
    set_ushort_cp(cpMsgOutOffset::cpMsgOutResCodePos, resultCode());
     
	if(protocolVersion >= cpMsgConst::cpMsgVersion2)
	{
       set_ulong_cp(cpMsgOutOffset::cpMsgOutSessPtrPos, cp_session_ptr);
	}
  }
  catch(FMS_CPF_PrivateException& ex)
  {
	  char errorMsg[1024]={0};
	  snprintf(errorMsg, 1023,"FMS_CPF_ReadrandCPMsg::pack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
	  CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
	  logError(ex);
  }
  TRACE(fms_cpf_ReadrandCPMsg,"%s","ReadrandCPMsg::pack leaving");
}
