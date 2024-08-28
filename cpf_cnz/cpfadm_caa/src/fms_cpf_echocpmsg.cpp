//
/** @file fms_cpf_echocpmsg.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_EchoCPMsg.
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
#include "fms_cpf_echocpmsg.h"

#include "fms_cpf_cpdfile.h"
#include "fms_cpf_privateexception.h"
#include "fms_cpf_common.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

/*===============================================================================
	ROUTINE: EchoCPMsg
 =============================================================================== */
FMS_CPF_EchoCPMsg::FMS_CPF_EchoCPMsg(const char* aInBuffer, unsigned int aInBufferSize, FMS_CPF_CpChannel *CPChan) :
FMS_CPF_CPMsg(aInBuffer, aInBufferSize, CPChan, FMS_CPF_CPMsg::MT_ECHO_MSG)
{
  fms_cpf_EchoCP = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EchoCP");
  unpack();
}

/*===============================================================================
	ROUTINE: ~EchoCPMsg
 =============================================================================== */
FMS_CPF_EchoCPMsg::~FMS_CPF_EchoCPMsg()
{
	if(NULL != fms_cpf_EchoCP)
	  delete fms_cpf_EchoCP;
}

/*===============================================================================
	ROUTINE: unpack
 =============================================================================== */
void FMS_CPF_EchoCPMsg::unpack()
{
	try
	{
		m_ReplySizeMsg = cpMsgReplySize::EchoCPMsgSize;
		inFunctionCode = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgFcCodeMask;
		protocolVersion = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgVersionMask;

		if(protocolVersion >= cpMsgConst::cpMsgVersion2)
		{
			cp_session_ptr = get_ulong_cp(cpMsgInOffset::cpMsgInSessPtrPos);
		}
		else
		{
			m_ReplySizeMsg = 6;
		}
		TRACE(fms_cpf_EchoCP, "unpack(),  protocolVersion = %d", protocolVersion);
	}
	catch (FMS_CPF_PrivateException& ex)
	{
		char errorMsg[1024]={0};
		snprintf(errorMsg, 1023,"FMS_CPF_EchoCPMsg::unpack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		logError(ex);
	}
}

/*===============================================================================
	ROUTINE: work
 =============================================================================== */
bool FMS_CPF_EchoCPMsg::work()
{
  // Do nothing, the echo signal is sent on an established
  // service channel every 10:th second.
	return true;
}

/*===============================================================================
	ROUTINE: pack
 =============================================================================== */
void FMS_CPF_EchoCPMsg::pack()
{
	try
    {
		createOutBuffer(m_ReplySizeMsg);
    
		// Pack out buffer
		set_ushort_cp(0, cpMsgReplyCode::EchoCPMsgCode + protocolVersion);
	
		if(protocolVersion >= cpMsgConst::cpMsgVersion2)
		{
			set_ulong_cp(cpMsgOutOffset::cpMsgOutSessPtrPos, cp_session_ptr);
			TRACE(fms_cpf_EchoCP, "pack(),  cp_session_ptr = %lu", cp_session_ptr);
		}
    }
	catch(FMS_CPF_PrivateException& ex)
	{
		char errorMsg[1024]={0};
		snprintf(errorMsg, 1023,"FMS_CPF_EchoCPMsg::pack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		logError(ex);
	}
}
