/*
 * * @file fms_cpf_cpmsgfactory.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_CPMsgFactory.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_cpmsgfactory.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-15
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
 *	| 1.0.0  | 2011-11-15 | qvincon      | File imported/updated.                       |
 *	+========+============+==============+=====================================+
 */
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_cpmsgfactory.h"
#include "fms_cpf_cpmsg.h"
#include "fms_cpf_cpopenchannels.h"
#include "fms_cpf_cpchannel.h"
#include "fms_cpf_common.h"

#include "fms_cpf_unknowncpmsg.h"
#include "fms_cpf_synccpmsg.h"
#include "fms_cpf_echocpmsg.h"
#include "fms_cpf_opencpmsg.h"
#include "fms_cpf_closecpmsg.h"
#include "fms_cpf_readnextcpmsg.h"
#include "fms_cpf_writenextcpmsg.h"
#include "fms_cpf_readrandcpmsg.h"
#include "fms_cpf_writerandcpmsg.h"
#include "fms_cpf_resetcpmsg.h"
#include "fms_cpf_rewritecpmsg.h"
#include "fms_cpf_connectcpmsg.h"

#include "fms_cpf_oc_buffermgr.h"


#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

namespace CpProcol{
	const uint16_t requestCodeSize = 2;
}
/*============================================================================
	ROUTINE: FMS_CPF_CPMsgFactory
 ============================================================================ */
FMS_CPF_CPMsgFactory::FMS_CPF_CPMsgFactory()
{
	fms_cpf_MsgFactoryTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_CPMsgFactory");
}

/*============================================================================
	ROUTINE: getRequestCode
 ============================================================================ */
void FMS_CPF_CPMsgFactory::getRequestCode(const char* aInBuffer, const unsigned int aInBufferSize, uint16_t& requestCode)
{
	word16_t reqCode;
	requestCode = 0;

	if(aInBufferSize >= CpProcol::requestCodeSize )
	{
		// Get first 2 bytes
		reqCode.byte[0] = aInBuffer[0];
		reqCode.byte[1] = aInBuffer[1];
		// Mask version info (highest 2 bits) 000104
		requestCode = (reqCode.word16 & cpMsgConst::cpMsgFcCodeMask);
	}

}

/*============================================================================
	ROUTINE: FMS_CPF_CPMsgFactory
 ============================================================================ */
FMS_CPF_CPMsgFactory::~FMS_CPF_CPMsgFactory()
{
	if(NULL != fms_cpf_MsgFactoryTrace)
		delete fms_cpf_MsgFactoryTrace;
}

/*============================================================================
	ROUTINE: handleMsg
 ============================================================================ */
bool FMS_CPF_CPMsgFactory::handleMsg(const char* aInBuffer, const unsigned int aInBufferSize, FMS_CPF_CpChannel* CpChannel)
{
	uint16_t requestCode;
	FMS_CPF_CPMsg* cpmsg = NULL;
	bool replyNow = false;
	std::string cpName;

    // get the message type
    getRequestCode(aInBuffer, aInBufferSize, requestCode);

    TRACE(fms_cpf_MsgFactoryTrace, "handleMsg requestCode<%d>, buffer size<%d>", requestCode, aInBufferSize);

	switch(requestCode)
	{
		case cpMsgFuncCodes::SyncCPMsgFcCode:
		{
			cpmsg = new (std::nothrow) FMS_CPF_SyncCPMsg(aInBuffer, aInBufferSize, CpChannel);
			break;
		}
		case cpMsgFuncCodes::EchoCPMsgFcCode:
		{
			cpmsg = new (std::nothrow) FMS_CPF_EchoCPMsg(aInBuffer, aInBufferSize, CpChannel);
			break;
		}
		case cpMsgFuncCodes::OpenCPMsgFcCode:
		{
			cpmsg = new (std::nothrow) FMS_CPF_OpenCPMsg(aInBuffer, aInBufferSize, CpChannel);
			break;
		}
		case cpMsgFuncCodes::CloseCPMsgFcCode:
		{
			cpmsg = new (std::nothrow) FMS_CPF_CloseCPMsg(aInBuffer, aInBufferSize, CpChannel);
			break;
		}
		case cpMsgFuncCodes::WriterandCPMsgFcCode:
		{
			cpmsg = new (std::nothrow) FMS_CPF_WriterandCPMsg(aInBuffer, aInBufferSize, CpChannel);
			break;
		}
		case cpMsgFuncCodes::ReadrandCPMsgFcCode:
		{
			cpmsg = new (std::nothrow) FMS_CPF_ReadrandCPMsg(aInBuffer, aInBufferSize, CpChannel);
			break;
		}
		case cpMsgFuncCodes::WritenextCPMsgFcCode:
		{
			cpmsg = new (std::nothrow) FMS_CPF_WritenextCPMsg(aInBuffer, aInBufferSize, CpChannel);
			break;
		}
		case cpMsgFuncCodes::ReadnextCPMsgFcCode:
		{
			cpmsg = new (std::nothrow) FMS_CPF_ReadnextCPMsg(aInBuffer, aInBufferSize, CpChannel);
			break;
		}
		case cpMsgFuncCodes::RewriteCPMsgFcCode:
		{
		 	cpmsg = new (std::nothrow) FMS_CPF_RewriteCPMsg(aInBuffer, aInBufferSize, CpChannel);
		 	break;
		}
		case cpMsgFuncCodes::ResetCPMsgFcCode:
		{
			cpmsg = new (std::nothrow) FMS_CPF_ResetCPMsg(aInBuffer, aInBufferSize, CpChannel);
			break;
		}
		case cpMsgFuncCodes::ConnectCPMsgFcCode:
		{
			cpmsg = new (std::nothrow) FMS_CPF_ConnectCPMsg(aInBuffer, aInBufferSize, CpChannel);
			break;
		}
		default:
			cpmsg = new (std::nothrow) FMS_CPF_UnknownCPMsg(aInBuffer, aInBufferSize, CpChannel);
	}

	//Check the message
	if(NULL == cpmsg)
	{
		// message not created, memory problem
		CPF_Log.Write("FMS_CPF_CPMsgFactory::handleMsg, error on message memory allocation", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_MsgFactoryTrace, "%s", "handleMsg(), error on message memory allocation");
		return false;
	}

    CpChannel->getDefaultCpName(cpName);
    cpmsg->setCpName(cpName);

	// Check if the message should be run or stuffed into OCBuffer
    if( (cpmsg->protoVersion() >= cpMsgConst::cpMsgVersion2) &&
        (requestCode == cpMsgFuncCodes::OpenCPMsgFcCode || requestCode == cpMsgFuncCodes::CloseCPMsgFcCode))
	{
    	// insert the message into OCBuffer
    	// Check if a message from CP side EX or SB
		if(cpmsg->messageIsFromEX())
		{
			// Message from EXcutive CP side
			TRACE(fms_cpf_MsgFactoryTrace, "handleMsg(), message from CP<%s, EX>", CpChannel->getConnectedCpName() );
			OcBufferMgr::instance()->pushInOCBuf( cpName, cpmsg);
		}
		else
		{
			// Message from StandBay CP side
			TRACE(fms_cpf_MsgFactoryTrace, "handleMsg(), message from CP<%s, SB>", CpChannel->getConnectedCpName() );
			OcBufferMgr::instance()->pushInSBOCBuf(cpName, cpmsg);
		}
	}
    else
	{
    	// Run the message
    	cpmsg->go(replyNow);
	}
	
    // Check if reply immediately
	if(replyNow)
	{
		// Puts the message in the CpChannel
		bool putResult = CpChannelsList::instance()->putMsgToCpChannel(CpChannel, cpmsg);

		//Check operation result
		if(!putResult)
		{
			// message not created
			CPF_Log.Write("FMS_CPF_CPMsgFactory::handleMsg, error to put message into CpChannel", LOG_LEVEL_ERROR);
			TRACE(fms_cpf_MsgFactoryTrace, "%s", "handleMsg(), error to put message into CpChannel");
			// Reclaim the allocated memory
			cpmsg->release();
		}
    }

	return true;
}
