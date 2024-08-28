//
/** @file fms_cpf_synccpmsg.cpp
 *	@brief
 *	Class method implementation for fms_cpf_synccpmsg.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_synccpmsg.h module
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
#include "fms_cpf_synccpmsg.h"
#include "fms_cpf_cpdopenfilesmgr.h"
#include "fms_cpf_oc_buffermgr.h"
#include "fms_cpf_common.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

/*===============================================================================
	ROUTINE: constructor
 =============================================================================== */
FMS_CPF_SyncCPMsg::FMS_CPF_SyncCPMsg(const char* aInBuffer, unsigned int aInBufferSize, FMS_CPF_CpChannel *CPChan) :
FMS_CPF_CPMsg(aInBuffer, aInBufferSize, CPChan, FMS_CPF_CPMsg::MT_SYNC_MSG)
{
  fms_cpf_SyncCPMsg = new (std::nothrow) ACS_TRA_trace("FMS_CPF_SyncCPMsg");
  unpack();
}

/*===============================================================================
	ROUTINE: destructor
 =============================================================================== */
FMS_CPF_SyncCPMsg::~FMS_CPF_SyncCPMsg()
{
	if(NULL != fms_cpf_SyncCPMsg)
	{
		delete fms_cpf_SyncCPMsg;
		fms_cpf_SyncCPMsg = NULL;
	}
}

/*===============================================================================
	ROUTINE: unpack
 =============================================================================== */
void FMS_CPF_SyncCPMsg::unpack()
{
	TRACE(fms_cpf_SyncCPMsg, "%s", "Entering in unpack()");
	try
	{
		m_ReplySizeMsg = cpMsgReplySize::SyncCPMsgSize;

		inFunctionCode = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgFcCodeMask;
		protocolVersion = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgVersionMask;

		APFMIversion = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgTwobuffers;
		msgIn = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgSBSide;

		TRACE(fms_cpf_SyncCPMsg,"unpack(), - APFMIversion = %d", APFMIversion);

		if(protocolVersion >= cpMsgConst::cpMsgVersion2)
		{
			isEXMsg = true;
			if( (APFMIversion == cpMsgConst::cpMsgTwobuffers) && (msgIn == cpMsgConst::cpMsgSBSide) )
			{
				isEXMsg = false;
			}
			cp_session_ptr  = get_ulong_cp(cpMsgInOffset::cpMsgInSessPtrPos);
			TRACE(fms_cpf_SyncCPMsg, "unpack(), isEXMsg is <%s>", (isEXMsg ? "TRUE" : "FALSE") );
		}
		else
		{
			m_ReplySizeMsg = 6;
		}
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		char errorMsg[1024]={0};
		snprintf(errorMsg, 1023,"FMS_CPF_SyncCPMsg::unpack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		logError(ex);
	}
	TRACE(fms_cpf_SyncCPMsg, "%s", "Leaving in unpack()");
}

/*===============================================================================
	ROUTINE: work
 =============================================================================== */
bool FMS_CPF_SyncCPMsg::work() 
{
  TRACE(fms_cpf_SyncCPMsg, "%s", "Entering in work()");
  bool result = true;
  try 
  {	 

    // This signal is sent on both service channels after a CP restart
    // or when  both service channels have been disconnected.
    // When CPF receives this signal, all open files shall be closed on that side where the
	// sync signal came from.
	if(isEXMsg)
	{
		 TRACE(fms_cpf_SyncCPMsg, "work(), received sync from <%s>EX", m_CpName.c_str());
		 //Sync message from EX side
		 CPDOpenFilesMgr::instance()->removeAll(true, m_CpName );
		 // Set sync in EXOC Buffer
		 OcBufferMgr::instance()->OCBsync(m_CpName);
	}
	else
	{
		TRACE(fms_cpf_SyncCPMsg, "work(), received sync from <%s>SB", m_CpName.c_str());
		//Sync message from SB side
		CPDOpenFilesMgr::instance()->removeAll(false, m_CpName);
		// Set sync in SBOC Buffer
		OcBufferMgr::instance()->SBOCBsync(m_CpName);
	}

  }
  catch(FMS_CPF_PrivateException& ex)
  {
	  TRACE(fms_cpf_SyncCPMsg, "work(), catched exception:<%d>, error:<%s>", ex.errorCode(), ex.detailInfo().c_str() );
	  logError(ex);
	  result = false;
  }
  TRACE(fms_cpf_SyncCPMsg, "%s", "Leaving work()");
  return result;
}

/*===============================================================================
	ROUTINE: pack
 =============================================================================== */
void FMS_CPF_SyncCPMsg::pack()
{
	TRACE(fms_cpf_SyncCPMsg, "%s", "Entering in pack()");
	try
	{
		createOutBuffer(m_ReplySizeMsg);

		set_ushort_cp(cpMsgOutOffset::cpMsgOutFcCodePos, cpMsgReplyCode::SyncCPMsgCode + protocolVersion);
		if(protocolVersion >= cpMsgConst::cpMsgVersion2)
		{
			set_ulong_cp(cpMsgOutOffset::cpMsgOutSessPtrPos, cp_session_ptr);
		}
	}
	catch (FMS_CPF_PrivateException & ex)
	{
		char errorMsg[1024]={0};
		snprintf(errorMsg, 1023, "FMS_CPF_SyncCPMsg::pack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		logError(ex);
	}
    TRACE(fms_cpf_SyncCPMsg, "%s", "Leaving pack()");
}
