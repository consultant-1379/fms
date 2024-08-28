//
/** @file fms_cpf_unknowncpmsg.cpp
 *	@brief
 *	Class method implementation for fms_cpf_unknowncpmsg.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_unknowncpmsg.h module
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

#include "fms_cpf_unknowncpmsg.h"
#include "fms_cpf_common.h"
#include "fms_cpf_eventalarmhndl.h"

#include <sstream>

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

//------------------------------------------------------------------------------
//      constructor
//------------------------------------------------------------------------------
FMS_CPF_UnknownCPMsg::FMS_CPF_UnknownCPMsg(const char* aInBuffer, unsigned int aInBufferSize, FMS_CPF_CpChannel *CPChan) :
FMS_CPF_CPMsg(aInBuffer, aInBufferSize, CPChan, FMS_CPF_CPMsg::MT_UNKNOW_MSG)
{
  fms_cpf_UnknownCPMsg = new (std::nothrow) ACS_TRA_trace("FMS_CPF_UnknownCPMsg");
  unpack();
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FMS_CPF_UnknownCPMsg::~FMS_CPF_UnknownCPMsg()
{
	  if(NULL != fms_cpf_UnknownCPMsg)
		  delete fms_cpf_UnknownCPMsg;
}

//------------------------------------------------------------------------------
//      unpack
//------------------------------------------------------------------------------
void FMS_CPF_UnknownCPMsg::unpack()
{
	TRACE(fms_cpf_UnknownCPMsg, "%s", "Entering in unpack()");
	try
	{
		m_ReplySizeMsg = cpMsgReplySize::UnknownCPMsgSize;

		inFunctionCode = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgFcCodeMask;
		protocolVersion = get_ushort_cp(cpMsgInOffset::cpMsgInFcCodePos) & cpMsgConst::cpMsgVersionMask;

		if(protocolVersion >= cpMsgConst::cpMsgVersion2)
		{
            cp_session_ptr = get_ulong_cp(cpMsgInOffset::cpMsgInSessPtrPos);
		}
		else
		{
			m_ReplySizeMsg = 10;
		}

		std::stringstream errorDetail;
		errorDetail << "Unknown message, inFunctionCode= " << inFunctionCode << std::ends;
    	EventReport::instance()->reportException(errorDetail, FMS_CPF_PrivateException::INTERNALERROR);
	}
	catch (FMS_CPF_PrivateException& ex)
	{
		char errorMsg[1024]={0};
		snprintf(errorMsg, 1023,"FMS_CPF_UnknownCPMsg::unpack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);

		logError(ex);
	}
	TRACE(fms_cpf_UnknownCPMsg, "%s", "Leaving unpack()");
}

//------------------------------------------------------------------------------
//      work
//------------------------------------------------------------------------------
bool FMS_CPF_UnknownCPMsg::work()
{
	TRACE(fms_cpf_UnknownCPMsg, "%s", "work(), return TRUE");
  // Do nothing, this is an illegal signal.
	return true; 
}

//------------------------------------------------------------------------------
//      pack
//------------------------------------------------------------------------------
void FMS_CPF_UnknownCPMsg::pack()
{
	TRACE(fms_cpf_UnknownCPMsg, "%s", "Entering in pack()");
	try
	{
		createOutBuffer(m_ReplySizeMsg);

		// Pack out buffer
		set_ushort_cp(cpMsgOutOffset::cpMsgOutFcCodePos,  cpMsgReplyCode::UnknownCPMsgCode + protocolVersion);
		set_ushort_cp(cpMsgOutOffset::cpMsgOutResCodePos, resultCode());
		if(protocolVersion >= cpMsgConst::cpMsgVersion2)
		{
			set_ulong_cp(cpMsgOutOffset::cpMsgOutSessPtrPos, cp_session_ptr);
		}
	}
	catch (FMS_CPF_PrivateException & ex)
	{
		char errorMsg[1024]={0};
		snprintf(errorMsg, 1023,"FMS_CPF_UnknownCPMsg::pack(), %s, %s", ex.errorText(), ex.detailInfo().c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
		logError(ex);
	}
	TRACE(fms_cpf_UnknownCPMsg, "%s", "Leaving pack()");
}
