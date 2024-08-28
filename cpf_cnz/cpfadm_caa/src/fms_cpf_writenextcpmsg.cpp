//
/** @file fms_cpf_WritenextCPMsg.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_WritenextCPMsg.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_echocpmsg.h module
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
#include "fms_cpf_writenextcpmsg.h"
#include "fms_cpf_cpdfile.h"
#include "fms_cpf_cpdfilethrd.h"
#include "fms_cpf_cpdopenfilesmgr.h"
#include "fms_cpf_common.h"
#include "fms_cpf_eventalarmhndl.h"
#include "fms_cpf_privateexception.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_WritenextCPMsg::FMS_CPF_WritenextCPMsg(const char* aInBuffer, unsigned int aInBufferSize, FMS_CPF_CpChannel* CPChan) :
FMS_CPF_CPMsg(aInBuffer, aInBufferSize, CPChan, FMS_CPF_CPMsg::MT_WRITENEXT_MSG)
{
  fms_cpf_WritenextCPMsg = new (std::nothrow) ACS_TRA_trace("FMS_CPF_WritenextCPMsg");
  unpack ();
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FMS_CPF_WritenextCPMsg::~FMS_CPF_WritenextCPMsg ()
{
	ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, ;);
	if(NULL != fms_cpf_WritenextCPMsg)
	{
	    delete fms_cpf_WritenextCPMsg;
	    fms_cpf_WritenextCPMsg = NULL;
	}
}

//------------------------------------------------------------------------------
//      unpack
//------------------------------------------------------------------------------
void FMS_CPF_WritenextCPMsg::unpack ()
{
	TRACE(fms_cpf_WritenextCPMsg, "%s", "Entering in unpack()");
	try
	{
		m_ReplySizeMsg = cpMsgReplySize::WritenextCPMsgSize;
		// If not possible to unpack, out-params shall be 0.
		lastRecordWritten = 0;

		inFunctionCode	= get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgFcCodeMask; //000104
		protocolVersion = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgVersionMask; //000104

		inPriority = get_ushort_cp(cpMsgInOffset::cpMsgInPrioPos);
		inSequenceNumber = get_ushort_cp(cpMsgInOffset::cpMsgInSeqNrPos);
		inFileReferenceUlong = get_ulong_cp(cpMsgInOffset::cpMsgInFileRefPos);
		inNumberOfRecords = get_ushort_cp(cpMsgInOffset::cpMsgInNrOfRecPos);

		if(protocolVersion >= cpMsgConst::cpMsgVersion2)
		{
			cp_session_ptr = get_ulong_cp(cpMsgInOffset::cpMsgInSessPtrPos);
		}
		else
		{
			m_ReplySizeMsg = 14;
		}

		checkPriority(inPriority);

		TRACE(fms_cpf_WritenextCPMsg, "unpack(), inFileReferenceUlong = %lu - inSequenceNumber = %d - inNumberOfRecords = %d", inFileReferenceUlong, inSequenceNumber, inNumberOfRecords);
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		char errorMsg[1024]={0};
		snprintf(errorMsg, 1023,"FMS_CPF_WritenextCPMsg::unpack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);

		logError (ex);
	}
	TRACE(fms_cpf_WritenextCPMsg, "%s", "Leaving in unpack()");
}

//------------------------------------------------------------------------------
//      go
//------------------------------------------------------------------------------
unsigned short FMS_CPF_WritenextCPMsg::go(bool& reply)
{
	TRACE(fms_cpf_WritenextCPMsg, "Entering in go(), msgId<%d>", inSequenceNumber);
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
    TRACE(fms_cpf_WritenextCPMsg, "Leaving go(), msgId<%d>", inSequenceNumber);
    return errorCode();
}

//------------------------------------------------------------------------------
//      thGo
//------------------------------------------------------------------------------
unsigned short FMS_CPF_WritenextCPMsg::thGo()
{
	TRACE(fms_cpf_WritenextCPMsg, "Entering in thGo(), msgId<%d>", inSequenceNumber);
	if (errorCode() == 0)
	{
		thWork();
	}
	pack();
	TRACE(fms_cpf_WritenextCPMsg, "Leaving thGo(), msgId<%d>", inSequenceNumber);
	return errorCode();
}


//------------------------------------------------------------------------------
//      work
//------------------------------------------------------------------------------
bool FMS_CPF_WritenextCPMsg::work() 
{
	TRACE(fms_cpf_WritenextCPMsg, "%s", "Entering in work()");
	bool result= true;
	try
	{
		fThrd = CPDOpenFilesMgr::instance()->lookup(inFileReferenceUlong, m_CpName);
		if( FAILURE == fThrd->putq(this) )
		{
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "failed putq to cpdfile thread");
		}
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		char errorMsg[1024]={0};
		snprintf(errorMsg, 1023,"FMS_CPF_WritenextCPMsg::work(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		TRACE(fms_cpf_WritenextCPMsg, "work(), catched exception:<%s>", errorMsg );
		logError(ex);
		result = false;
	}
	TRACE(fms_cpf_WritenextCPMsg, "Leaving work(), result=<%s>", (result ? "TRUE" : "FALSE") );
	return result;
}

//------------------------------------------------------------------------------
//      thWork
//------------------------------------------------------------------------------
void FMS_CPF_WritenextCPMsg::thWork()
{
  TRACE(fms_cpf_WritenextCPMsg, "%s", "Entering in thWork()");
  try 
  {
	  // Let this be the first statement in the try block to catch
	  // invalid reference first of all.
	  CPDFile* cpdfile = fThrd->getFileRef();

	  char* buffer = const_cast<char*>(get_buffer_in(cpMsgInOffset::cpMsgInBufferPos));
	  unsigned long bufferSize = get_buffer_size_in(cpMsgInOffset::cpMsgInBufferPos);

	  TRACE(fms_cpf_WritenextCPMsg, "thWork(), buffer size:<%lu>", bufferSize);
	  cpdfile->writenext(inNumberOfRecords,
			  	  	  	 buffer,
			  	  	  	 bufferSize,
			  	  	  	 lastRecordWritten
                       	);
   }
  catch (FMS_CPF_PrivateException& ex)
  {
	  char errorMsg[1024]={0};
	  snprintf(errorMsg, 1023,"FMS_CPF_WritenextCPMsg::thWork(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
	  CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
	  TRACE(fms_cpf_WritenextCPMsg, "thWork(), catched exception:<%s>", errorMsg );
	  logError (ex);
   }
   TRACE(fms_cpf_WritenextCPMsg, "%s", "Leaving thWork()");
}

//------------------------------------------------------------------------------
//      pack
//------------------------------------------------------------------------------
void FMS_CPF_WritenextCPMsg::pack()
{
	TRACE(fms_cpf_WritenextCPMsg, "%s", "Entering in pack()");
	try
	{
		createOutBuffer(m_ReplySizeMsg);
		// Pack out buffer
		set_ushort_cp(cpMsgOutOffset::cpMsgOutFcCodePos, cpMsgReplyCode::WritenextCPMsgCode + protocolVersion);
		set_ushort_cp(cpMsgOutOffset::cpMsgOutSeqNrPos, inSequenceNumber);
		set_ushort_cp(cpMsgOutOffset::cpMsgOutResCodePos, resultCode());
		set_ulong_cp(cpMsgOutOffset::cpMsgOutLstRecWrPos, lastRecordWritten);

		if(protocolVersion >= cpMsgConst::cpMsgVersion2)
		{
			set_ulong_cp(cpMsgOutOffset::cpMsgOutSessPtrPos, cp_session_ptr);
		}
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		char errorMsg[1024]={0};
		snprintf(errorMsg, 1023, "FMS_CPF_WritenextCPMsg::pack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);

		logError(ex);
	}
	TRACE(fms_cpf_WritenextCPMsg, "%s", "Leaving pack()");
}
