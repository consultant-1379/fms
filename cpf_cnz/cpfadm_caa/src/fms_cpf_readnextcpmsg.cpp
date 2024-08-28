//
/** @file fms_cpf_readnextcpmsg.cpp
 *	@brief
 *	Class method implementation for fms_cpf_readnextcpmsg.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_readnextcpmsg.h module
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

#include "fms_cpf_readnextcpmsg.h"
#include "fms_cpf_cpdfile.h"
#include "fms_cpf_cpdfilethrd.h"
#include "fms_cpf_cpdopenfilesmgr.h"
#include "fms_cpf_common.h"

#include "fms_cpf_privateexception.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_ReadnextCPMsg::FMS_CPF_ReadnextCPMsg(const char* aInBuffer, unsigned int aInBufferSize, FMS_CPF_CpChannel *CPChan) :
FMS_CPF_CPMsg(aInBuffer, aInBufferSize, CPChan, FMS_CPF_CPMsg::MT_READNEXT_MSG)
{
  fms_cpf_ReadnextCPMsg = new (std::nothrow) ACS_TRA_trace("FMS_CPF_ReadnextCPMsg");
  unpack();
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FMS_CPF_ReadnextCPMsg::~FMS_CPF_ReadnextCPMsg()
{
	// critical section
	ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, ;);
	if(NULL != fms_cpf_ReadnextCPMsg)
	{
		delete fms_cpf_ReadnextCPMsg;
		fms_cpf_ReadnextCPMsg = NULL;
	}
}

//------------------------------------------------------------------------------
//      unpack
//------------------------------------------------------------------------------
void FMS_CPF_ReadnextCPMsg::unpack()
{
	TRACE(fms_cpf_ReadnextCPMsg, "%s", "Entering in unpack()");
	try
	{
		m_ReplySizeMsg = cpMsgReplySize::ReadnextCPMsgSize;
		// If not possible to unpack, out-params shall be 0.
		outRecordsRead = 0;

		inFunctionCode = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgFcCodeMask;
		protocolVersion = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgVersionMask;

		inPriority = get_ushort_cp(cpMsgInOffset::cpMsgInPrioPos);
		inSequenceNumber = get_ushort_cp(cpMsgInOffset::cpMsgInSeqNrPos);
		inFileReferenceUlong = get_ulong_cp(cpMsgInOffset::cpMsgInFileRefPos);
		inNumberOfRecords = get_ushort_cp(cpMsgInOffset::cpMsgInNrOfRecPos);
    
		if(protocolVersion >= cpMsgConst::cpMsgVersion2)
		{
			cp_session_ptr = get_ulong_cp(cpMsgInOffset::cpMsgInSessPtrPos);
		}

		checkPriority(inPriority);

		TRACE(fms_cpf_ReadnextCPMsg, "unpack(), inFileReferenceUlong = %d - inSequenceNumber = %d - inNumberOfRecords = %d", inFileReferenceUlong, inSequenceNumber, inNumberOfRecords);
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		char errorMsg[1024]={0};
		snprintf(errorMsg, 1023, "FMS_CPF_ReadnextCPMsg::unpack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		logError(ex);
	}
	TRACE(fms_cpf_ReadnextCPMsg, "%s", "Leaving unpack()");
}

//------------------------------------------------------------------------------
//      go
//------------------------------------------------------------------------------
unsigned short FMS_CPF_ReadnextCPMsg::go(bool& reply)
{
	TRACE(fms_cpf_ReadnextCPMsg, "Entering in go(), msgId<%d>", inSequenceNumber);
	ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, ;);

	if(errorCode() == 0)
	{
        // Do not work if exception during unpack
        if(!work())
        {
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
    TRACE(fms_cpf_ReadnextCPMsg, "Leaving go(), msgId<%d>", inSequenceNumber);
    return errorCode();
}

//------------------------------------------------------------------------------
//      thGo
//------------------------------------------------------------------------------
unsigned short FMS_CPF_ReadnextCPMsg::thGo()
{
	TRACE(fms_cpf_ReadnextCPMsg, "Entering in thGo() , msgId<%d>", inSequenceNumber);

	if (errorCode() == 0)
	{
		thWork();
	}
	pack();
	TRACE(fms_cpf_ReadnextCPMsg, "Leaving thGo(), msgId<%d>", inSequenceNumber);
	return errorCode();
}

//------------------------------------------------------------------------------
//      work
//------------------------------------------------------------------------------
bool FMS_CPF_ReadnextCPMsg::work()
{
	TRACE(fms_cpf_ReadnextCPMsg, "Entering in work(), msgId<%d>", inSequenceNumber);
	bool result = true;
	try
	{
		fThrd = CPDOpenFilesMgr::instance()->lookup(inFileReferenceUlong, m_CpName);

		if( FAILURE == fThrd->putq(this) )
		{
			CPF_Log.Write("FMS_CPF_ReadnextCPMsg::work(), failed putq to cpdfile thread", LOG_LEVEL_WARN);
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "FMS_CPF_ReadnextCPMsg::work(), failed putq to cpdfile thread");
		}
	}
	catch (FMS_CPF_PrivateException & ex)
	{
		logError(ex);
		result = false;
	}
	
	TRACE(fms_cpf_ReadnextCPMsg, "Leaving work, result=<%s>, msgId<%d>", (result ? "TRUE" : "FALSE"), inSequenceNumber );
	return result;
}

//------------------------------------------------------------------------------
//      thWork
//------------------------------------------------------------------------------
void FMS_CPF_ReadnextCPMsg::thWork()
{
	TRACE(fms_cpf_ReadnextCPMsg, "Entering in thWork(), msgId<%d>", inSequenceNumber);
	try
	{
		CPDFile* cpdfile = fThrd->getFileRef();
		unsigned int size = cpdfile->getRecordLength() * inNumberOfRecords;
		createOutBuffer(m_ReplySizeMsg + size);
		char* buffer = get_buffer_out(m_ReplySizeMsg);

		TRACE(fms_cpf_ReadnextCPMsg, "thWork(), read data, msgId<%d>", inSequenceNumber);
		cpdfile->readnext(inNumberOfRecords,
                      	  buffer,
                      	  outRecordsRead
                      	 );
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		TRACE(fms_cpf_ReadnextCPMsg, "thWork(), catched exception:<%d>, error:<%s>", ex.errorCode(), ex.detailInfo().c_str() );
		logError(ex);
	}
	TRACE(fms_cpf_ReadnextCPMsg, "Leaving thWork(), msgId<%d>", inSequenceNumber);
}

//------------------------------------------------------------------------------
//      pack
//------------------------------------------------------------------------------
void FMS_CPF_ReadnextCPMsg::pack()
{
	TRACE(fms_cpf_ReadnextCPMsg, "%s", "Entering in pack()");
	try
	{
		// If exception, use smallest possible size.
		createOutBuffer(m_ReplySizeMsg);

		// Pack out buffer
		set_ushort_cp(cpMsgOutOffset::cpMsgOutFcCodePos, cpMsgReplyCode::ReadnextCPMsgCode + protocolVersion);
		set_ushort_cp(cpMsgOutOffset::cpMsgOutSeqNrPos, inSequenceNumber);
		set_ushort_cp(cpMsgOutOffset::cpMsgOutResCodePos, resultCode());
		set_ushort_cp(cpMsgOutOffset::cpMsgOutRecsReadPos, outRecordsRead);
    
		if(protocolVersion >= cpMsgConst::cpMsgVersion2)
		{
			set_ulong_cp(cpMsgOutOffset::cpMsgOutSessPtrPos, cp_session_ptr);
		}
	}
	catch (FMS_CPF_PrivateException& ex)
	{
		char errorMsg[1024]={0};
		snprintf(errorMsg, 1023, "FMS_CPF_ReadnextCPMsg::pack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		logError(ex);
	}

	TRACE(fms_cpf_ReadnextCPMsg, "%s", "Leaving pack()");
}
