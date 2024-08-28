//
/** @file fms_cpf_connectcpmsg.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_ConnectCPMsg.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_Connectcpmsg.h module
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

#include "fms_cpf_connectcpmsg.h"
#include "fms_cpf_common.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_ConnectCPMsg::FMS_CPF_ConnectCPMsg(const char* aInBuffer, unsigned int aInBufferSize, FMS_CPF_CpChannel *CPChan) :
FMS_CPF_CPMsg(aInBuffer, aInBufferSize, CPChan, FMS_CPF_CPMsg::MT_CONNECT_MSG)
{
  fms_cpf_ConnectCPMsg = new (std::nothrow) ACS_TRA_trace("FMS_CPF_ConnectCPMsg");
  unpack();
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FMS_CPF_ConnectCPMsg::~FMS_CPF_ConnectCPMsg()
{
	if(NULL != fms_cpf_ConnectCPMsg)
	{
	  delete fms_cpf_ConnectCPMsg;
	  fms_cpf_ConnectCPMsg = NULL;
	}
}

//------------------------------------------------------------------------------
//      unpack
//------------------------------------------------------------------------------
void FMS_CPF_ConnectCPMsg::unpack()
{
	TRACE(fms_cpf_ConnectCPMsg, "%s", "Entering in unpack()");
	try
	{
		m_ReplySizeMsg = cpMsgReplySize::ConnectCPMsgSize;

		inFunctionCode = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgFcCodeMask;
		protocolVersion = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgVersionMask;
		APFMItwoBuffers = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgTwobuffers;

		if(inBufferSize > 10)
		{
			protocolVersion = cpMsgConst::cpMsgVersion2;
		}

		unsigned int answer = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos);
	
		inSubnetId = get_ushort_cp(cpMsgInOffset::cpMsgInSubNetIdPos);
		if(protocolVersion == cpMsgConst::cpMsgVersion2)
		{
            cp_session_ptr    = get_ulong_cp (cpMsgInOffset::cpMsgInSessPtrPos);  // 001005
		}
		else
		{
			m_ReplySizeMsg = 6;
		}

		TRACE(fms_cpf_ConnectCPMsg,"unpack(), APFMItwoBuffers = %d - protocolVersion = %d - answer = %d", APFMItwoBuffers, protocolVersion, answer);
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		char errorMsg[1024]={0};
	    snprintf(errorMsg, 1023,"FMS_CPF_ConnectCPMsg::unpack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		logError(ex);
	}
	TRACE(fms_cpf_ConnectCPMsg, "%s", "Leaving in unpack()");
}

//------------------------------------------------------------------------------
//      pack
//------------------------------------------------------------------------------
void FMS_CPF_ConnectCPMsg::pack()
{
	TRACE(fms_cpf_ConnectCPMsg, "%s", "Entering in pack()");
	unsigned short twoBuf = 0;
	try
	{
		createOutBuffer(m_ReplySizeMsg);

		if(protocolVersion == cpMsgConst::cpMsgVersion2)
		{
			set_ulong_cp(cpMsgOutOffset::cpMsgOutSessPtrPos, cp_session_ptr);
			if(APFMItwoBuffers == cpMsgConst::cpMsgTwobuffers)
			{
				twoBuf = cpMsgConst::cpMsgTwobuffers;
			}
		}

		unsigned short answer = cpMsgReplyCode::ConnectCPMsgCode + protocolVersion + twoBuf;
		set_ushort_cp(cpMsgOutOffset::cpMsgOutFcCodePos, answer);
		TRACE(fms_cpf_ConnectCPMsg, "pack(), answer = %d", answer);
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		char errorMsg[1024]={0};
		snprintf(errorMsg, 1023,"FMS_CPF_ConnectCPMsg::pack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		logError(ex);
	}
    TRACE(fms_cpf_ConnectCPMsg, "%s", "Leaving pack()");
}
