/*
 * fms_cpf_sb_ocbuffer.cpp
 *
 *  Created on: Nov 14, 2011
 *      Author: enungai
 */

#include "fms_cpf_sb_ocbuffer.h"
#include "fms_cpf_common.h"
#include "fms_cpf_message.h"
extern ACS_TRA_Logging CPF_Log;


SbOcBuffer::SbOcBuffer(std::string cpname) {

	fms_cpf_sbbuffer_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_SB_OCBuffer");
	syncReceived = false;
	openSyncReceived = false;
	m_firstCycle = true;
	cleanMessages = false;
	m_cpname = cpname;
	_nextSeqNr = 1;//HX68002
}

SbOcBuffer::~SbOcBuffer() {

	if (fms_cpf_sbbuffer_trace)
		delete fms_cpf_sbbuffer_trace;

}


//------------------------------------------------------------------------------
//      Synchronize sequence numbers with CP
//------------------------------------------------------------------------------
void SbOcBuffer::sync()
{
	TRACE(fms_cpf_sbbuffer_trace, "%s", "set sync()");
	syncReceived = true;
}

//------------------------------------------------------------------------------
//      Make sure that all file references are removed before handling first open message
//------------------------------------------------------------------------------
void SbOcBuffer::clearFileRefs()
{
	TRACE(fms_cpf_sbbuffer_trace, "%s", "Entering in clearFileRefs()");
	//Start of fix for HW38429

	if(syncReceived)
	{
		TRACE(fms_cpf_sbbuffer_trace, "clearFileRefs(), remove All opened file of cp:<%s>", m_cpname.c_str());
		syncReceived = false;
		//END of fix for HW38429
		CPDOpenFilesMgr::instance()->removeAll(false, m_cpname);

	}
	TRACE(fms_cpf_sbbuffer_trace, "%s", "Leaving in clearFileRefs()");
}

int SbOcBuffer::svc(void)
{
	TRACE(fms_cpf_sbbuffer_trace, "%s", "Entering in svc");
	bool svc_run = true;
	int MAX_RETRY = 5;
	int num_of_retry = 0;
	int getResult;
	int msg_type;
	ACE_Message_Block* base_msg;
	FMS_CPF_CPMsg* cpmsg;

	TRACE(fms_cpf_sbbuffer_trace, "%s", "svc thread, waiting for cp messages");

	while(svc_run)
	{
		getResult = getq(base_msg);

		if(FAILURE == getResult)
		{
			CPF_Log.Write("SbOcBuffer::svc(), getq of a message failed", LOG_LEVEL_ERROR);
			TRACE(fms_cpf_sbbuffer_trace, "%s", "svc(), getq of a message failed");

			if(MAX_RETRY == num_of_retry)
			{
				CPF_Log.Write("SbOcBuffer::svc(), reached the max number of getq failed", LOG_LEVEL_ERROR);
				svc_run = false;
			}
			num_of_retry++;
			continue;
		}

		num_of_retry = 0;

		msg_type = base_msg->msg_type();

		 // Dispatch on message type
		switch(msg_type)
		{
			case CPF_Message_Base::MT_EXIT :
			{
				// Received the termination message
				TRACE(fms_cpf_sbbuffer_trace, "%s", "svc, received EXIT message");
				clearFileRefs();
				// release the message memory
				base_msg->release();
				svc_run = false;
			}
			break;

			case CPF_Message_Base::MT_SYNC :
			{
				// Received the termination message
				TRACE(fms_cpf_sbbuffer_trace, "%s", "svc, received SYNC message");
				sync();
				// release the message memory
				base_msg->release();

			}
			break;

			case CPF_Message_Base::MT_CLEAR_FILES :
			{
				// Received the termination message
				TRACE(fms_cpf_sbbuffer_trace, "%s", "svc, received CLEAR_FILES message");
				clearFileRefs();
				// release the message memory
				base_msg->release();

			}
			break;

			case FMS_CPF_CPMsg::MT_OPEN_MSG :
			case FMS_CPF_CPMsg::MT_CLOSE_MSG:
			{
				TRACE(fms_cpf_sbbuffer_trace, "%s", "svc, received OPEN/CLOSE message");
				cpmsg = reinterpret_cast<FMS_CPF_CPMsg*>(base_msg);
				handleCpMsg(cpmsg);

		 	}
			break;

			default:
			{
				CPF_Log.Write("SbOcBuffer::svc(), received an unknown message", LOG_LEVEL_ERROR);
				TRACE(fms_cpf_sbbuffer_trace, "%s", "svc, received an unknown message");
			}
			break;
		}
		TRACE(fms_cpf_sbbuffer_trace, "%s", "svc, received message handled");
	}
	TRACE(fms_cpf_sbbuffer_trace, "%s", "Leaving the svc()");

	return SUCCESS;

}

int SbOcBuffer::open(void *args)
{
	UNUSED(args);
	int result;

	// Start the thread to handler Infinite SubFile
	result = activate();
	if( FAILURE == result)
	{
		CPF_Log.Write("SbOcBuffer::open, error on start svc thread", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_sbbuffer_trace, "%s", "Leaving open(), error on start svc thread");
	}
	return result;
}


void SbOcBuffer::insertCpMsg(unsigned short seqNr, FMS_CPF_CPMsg* aMsg)
{
	TRACE(fms_cpf_sbbuffer_trace, "%s", "insertCpMsg");
	PAIR_IF inserted =
	myCpMsgQueue.insert(cpMsgMap::value_type(seqNr, aMsg));

	if(! inserted.second)
	{
	   TRACE(fms_cpf_sbbuffer_trace, "%s", "insertCpMsg(): duplicate key");
	   aMsg->release();
	}
}

FMS_CPF_CPMsg* SbOcBuffer::fetchCpMsg(unsigned short aSeqNr)
{
	TRACE(fms_cpf_sbbuffer_trace, "%s", "fetchCpMsg");
	FMS_CPF_CPMsg* aCpMsg = NULL;
	myCpMsgPtr = myCpMsgQueue.find(aSeqNr);
	if (myCpMsgPtr != myCpMsgQueue.end())
	{
	    aCpMsg = (*myCpMsgPtr).second;
	    myCpMsgQueue.erase(aSeqNr);
	}
	return aCpMsg;
}
void SbOcBuffer::cleanMsgQue()
{
	TRACE(fms_cpf_sbbuffer_trace, "%s", "cleanMsgQue()");
	myCpMsgPtr = myCpMsgQueue.begin();
	while (myCpMsgPtr != myCpMsgQueue.end())
	{
		FMS_CPF_CPMsg* aCpMsg = (*myCpMsgPtr).second;
		aCpMsg->release();
		myCpMsgQueue.erase(myCpMsgPtr);
		myCpMsgPtr = myCpMsgQueue.begin();
	}
}

void SbOcBuffer::handleCpMsg(FMS_CPF_CPMsg* cpmsg)
{
	int seq = cpmsg->getSBOCSeq();

	TRACE(fms_cpf_sbbuffer_trace, "handleCpMsg(), received Msg-Seq:<%d>, expected Msg-Seq:<%d>", seq ,_nextSeqNr);

	if (syncReceived)
	{
	    // Clear number to 1
	    _nextSeqNr = 1;
		syncReceived = false;
		if (m_firstCycle)
		{
			m_firstCycle = false;
		}
		else
		{
			openSyncReceived = true;
		}
	    cleanMessages = true;
	}

	if (cleanMessages)
	{
		cleanMsgQue(); // remove old msg
	    cleanMessages = false;
	}

	// Check if it is next in sequence
	// seq == 0 equals old APFMI
	if (seq == _nextSeqNr || seq == 0)
	{
		do
		{
			TRACE(fms_cpf_sbbuffer_trace,"%s", "handleCpMsg() before to go() call");
			// Do the work for this message
			bool replyNow = false;
			cpmsg->go(replyNow);
			if (replyNow) {
				FMS_CPF_CpChannel *cpChannel = cpmsg->cpChannel();
				bool putResult = CpChannelsList::instance()->putMsgToCpChannel(cpChannel, cpmsg);

				//Check operation result
				if(!putResult)
				{
					// message not created
					CPF_Log.Write("FMS_CPF_CPMsgFactory::handleMsg, error to put message into CpChannel", LOG_LEVEL_ERROR);
					TRACE(fms_cpf_sbbuffer_trace, "%s", "handleMsg(), error to put message into CpChannel");
					// Reclaim the allocated memory
					cpmsg->release();
				}
			}

			if (syncReceived)
			{
				_nextSeqNr = 1;
				syncReceived = false;
				if (m_firstCycle)
				{
					m_firstCycle = false;
				}
				else
				{
					openSyncReceived = true; //040501
				}
				cleanMessages = true;
			}
			else
			{
				 // Increase the counter for next message
				 _nextSeqNr++;
			}

			cpmsg = fetchCpMsg(_nextSeqNr);

		} while (cpmsg != NULL);
	}
	else
	{
		TRACE(fms_cpf_sbbuffer_trace,"%s", "handleCpMsg(), out of sequence");
		// Stuff it into the internal queue
		insertCpMsg(seq, cpmsg);
	}

}
