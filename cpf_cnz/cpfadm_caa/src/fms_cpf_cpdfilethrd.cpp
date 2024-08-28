/*
 * * @file fms_cpf_cpdfilethrd.cpp
 *	@brief
 *	Class method implementation for CPDFileThrd.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_cpdfilethrd.h module
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
 *	| 1.0.0  | 2011-11-15 | qvincon      | File imported.                      |
 *	+========+============+==============+=====================================+
 *	| 1.1.0  | 2011-11-15 | qvincon      | ACE introduction                    |
 *	+========+============+==============+=====================================+
 */
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_cpdfilethrd.h"
#include "fms_cpf_cpopenchannels.h"
#include "fms_cpf_cpdopenfilesmgr.h"
#include "fms_cpf_infinitefileopenlist.h"
#include "fms_cpf_cpdfile.h"
#include "fms_cpf_cpmsg.h"
#include "fms_cpf_message.h"
#include "fms_cpf_common.h"

#include <ace/Time_Value_T.h>

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include <sys/types.h>
#include <sys/syscall.h>


extern ACS_TRA_Logging CPF_Log;

namespace FileParameter {

	const int FileThrdDelay = 1000;                // ms before timeout in msg wait loop
	const int FileThrdSubFileSwitchDelay = 30000;  // ms between check infinite subfile switch
	const int MsgOutOfSeqExitDelay = 10000;        // ms before file thread exit after no msg
}

/*============================================================================
	ROUTINE: CPDFileThrd
 ============================================================================ */
CPDFileThrd::CPDFileThrd():
m_CpdFile(NULL),
m_FileReference(0),
m_NextSeqNr(0),
m_FileClosed(true),
m_FSWLoopCnt(0),
m_MultiWriteOn(false),
m_FileInList(false),
m_OpenLock(NULL),
m_SyncReceived(false),
m_ServiceShutDown(false)
{
	fms_cpf_CpFileTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_CPDFileThrd");
}

/*============================================================================
	ROUTINE: ~CPDFileThrd
 ============================================================================ */
CPDFileThrd::~CPDFileThrd()
{
	if(NULL != fms_cpf_CpFileTrace)
	{
		delete fms_cpf_CpFileTrace;
		fms_cpf_CpFileTrace = NULL;
	}
}

/*============================================================================
	ROUTINE: shutDown
 ============================================================================ */
void CPDFileThrd::shutDown(bool isServiceShutDown)
{
	TRACE(fms_cpf_CpFileTrace, "%s","Entering in shutDown()");
	m_ServiceShutDown = isServiceShutDown;

	// Service lock
	if(m_ServiceShutDown)
	{
		// Deactive the queue as forced closure
		msg_queue_->deactivate();
	}
	else
	{
		// Handling close message
		CPF_Close_Msg* internalMsg = new (std::nothrow) CPF_Close_Msg();
		if( NULL != internalMsg)
		{
			TRACE(fms_cpf_CpFileTrace, "%s","Leaving shutDown(), enqueue close message to cpdfile thread");

			int putResult = putq(internalMsg);

			if(FAILURE == putResult)
			{
				TRACE(fms_cpf_CpFileTrace, "%s","shutDown(), Failed to enqueue close message");
				internalMsg->release();
				// Deactive the queue as forced closure
				msg_queue_->deactivate();
				CPF_Log.Write("CPDFileThrd::shutDown(), Failed to enqueue close message", LOG_LEVEL_WARN);
			}
		}
		else
		{
			CPF_Log.Write("CPDFileThrd::shutDown(), Failed to create close message", LOG_LEVEL_WARN);
			// Deactive the queue as forced closure
			msg_queue_->deactivate();
		}
	}
}
/*============================================================================
	ROUTINE: removeFileRef
 ============================================================================ */
void CPDFileThrd::removeFileRef()
{
	TRACE(fms_cpf_CpFileTrace, "%s","Entering in removeFileRef()");

	if( (NULL != m_CpdFile) && !m_FileInList)
	{
		TRACE(fms_cpf_CpFileTrace, "%s","removeFileRef(), delete cpdfile object");
		delete m_CpdFile;
		m_CpdFile = NULL;
	}

	TRACE(fms_cpf_CpFileTrace, "%s","Leaving removeFileRef()");
}

/*============================================================================
	ROUTINE: putFileRef
 ============================================================================ */
bool CPDFileThrd::putFileRef(uint32_t aFileRefUlong, CPDFile* aCPDFile, bool infiniteFileOpen )
{
	TRACE(fms_cpf_CpFileTrace, "%s","Entering in putFileRef()");
	bool result;

	// Set the file reference
	m_FileReference = aFileRefUlong;

	// Check the current value
	if(NULL == m_CpdFile )
    {
    	TRACE(fms_cpf_CpFileTrace, "%s","putFileRef(), set CpdFile object");
    	// Set the cpdfile handler
    	m_CpdFile = aCPDFile;

    	// Check the new assigned value
    	if(NULL != m_CpdFile)
    	{
    		TRACE(fms_cpf_CpFileTrace, "%s","putFileRef(), CpdFile object setted");
    		// The file is open
    		m_FileClosed = false;
    		if(infiniteFileOpen)
    		{
    			TRACE(fms_cpf_CpFileTrace, "%s","putFileRef(), file handled is infinite");
    			// add file to list
    			m_fileIdInList = m_CpdFile->getFileId();
    			InfiniteFileOpenList::instance()->add(m_CpName, m_fileIdInList);  //HT89089
    			m_FileInList = true;
    		}
    	}
    	result = true;
    }
	else
	{
		result = false;
		TRACE(fms_cpf_CpFileTrace, "%s","putFileRef(), CpdFile already set");
	}

	TRACE(fms_cpf_CpFileTrace, "%s","Leaving putFileRef()");
	return result;
}

/*============================================================================
	ROUTINE: insertCpMsg
 ============================================================================ */
void CPDFileThrd::insertCpMsg(const uint16_t seqNr, FMS_CPF_CPMsg* aMsg)
{
	TRACE(fms_cpf_CpFileTrace, "%s","Entering in insertCpMsg()");

	std::pair<cpMsgMap::iterator, bool> result;
	// Insert a message into internal wait map
	result = m_CpMsgQueue.insert(cpMsgMap::value_type(seqNr, aMsg));
  
	if(!result.second)
	{
		TRACE(fms_cpf_CpFileTrace, "%s","insertCpMsg(), error to insert a new msg, duplicate keys");
		CPF_Log.Write("CPDFileThrd::insertCpMsg(), error to insert a new msg, duplicate keys", LOG_LEVEL_WARN);
	}

    TRACE(fms_cpf_CpFileTrace, "%s","Leaving insertCpMsg()");
}
/*============================================================================
	ROUTINE: fetchCpMsg
 ============================================================================ */
FMS_CPF_CPMsg* CPDFileThrd::fetchCpMsg(const uint16_t aSeqNr)
{
	TRACE(fms_cpf_CpFileTrace, "Entering in fetchCpMsg(seqNr=<%d>)", aSeqNr);
	FMS_CPF_CPMsg* cp_msg = NULL;
	cpMsgMap::iterator mapPairValue;

	mapPairValue = m_CpMsgQueue.find(aSeqNr);

    if(m_CpMsgQueue.end() != mapPairValue )
    {
    	TRACE(fms_cpf_CpFileTrace, "fetchCpMsg(), found a message with seqNr=<%d>", aSeqNr);
    	cp_msg = (*mapPairValue).second;
    	m_CpMsgQueue.erase(mapPairValue);
    }

    TRACE(fms_cpf_CpFileTrace, "Leaving fetchCpMsg(seqNr=<%d>)", aSeqNr);

    return cp_msg;
}

/*============================================================================
	ROUTINE: fetchCpMsg
 ============================================================================ */
FMS_CPF_CPMsg* CPDFileThrd::fetchCpMsg()
{
	TRACE(fms_cpf_CpFileTrace, "%s", "Entering in fetchCpMsg()");
	FMS_CPF_CPMsg* cp_msg = NULL;
	cpMsgMap::iterator mapPairValue;

	mapPairValue = m_CpMsgQueue.begin();

	if(mapPairValue != m_CpMsgQueue.end())
	{
		TRACE(fms_cpf_CpFileTrace, "fetchCpMsg(), get first message seqNr=<%d>", (*mapPairValue).first);
		cp_msg = (*mapPairValue).second;
		m_CpMsgQueue.erase(mapPairValue);
	}
	TRACE(fms_cpf_CpFileTrace, "%s", "Leaving fetchCpMsg()");
	return cp_msg;
}

/*============================================================================
	ROUTINE: checkSubFileSwitch
 ============================================================================ */
void CPDFileThrd::checkSubFileSwitch()
{
	TRACE(fms_cpf_CpFileTrace, "%s","Entering in checkSubFileSwitch()");

	if(!m_FileClosed)
	{
		bool onTime = false;
		bool noMoreTimeSwitch = false;
		TRACE(fms_cpf_CpFileTrace, "%s","checkSubFileSwitch(), file is open");
		// Check if time to change active subfile on time
		int maxLoop = FileParameter::FileThrdSubFileSwitchDelay / FileParameter::FileThrdDelay;

		if( (m_FSWLoopCnt++) > maxLoop )
		{
			TRACE(fms_cpf_CpFileTrace, "%s","checkSubFileSwitch(), reset m_FSWLoopCnt");
			m_FSWLoopCnt = 0;
			onTime = true;
		}

		if(NULL != m_CpdFile)
		{
			try
			{
				m_CpdFile->checkSwitchSubFile(onTime, false, noMoreTimeSwitch);
			}
			catch(FMS_CPF_PrivateException& ex)
			{
				TRACE(fms_cpf_CpFileTrace, "checkSubFileSwitch(), catched exception:<%d>, <%s>", ex.errorCode(), ex.detailInfo().c_str());
			}
		}
		else
		{
		    TRACE(fms_cpf_CpFileTrace, "%s","checkSubFileSwitch(), cpdfile object is NULL");
			CPF_Log.Write("CPDFileThrd::checkSubFileSwitch, cpdfile object is NULL", LOG_LEVEL_WARN);
		}
	}
	TRACE(fms_cpf_CpFileTrace, "%s","Leaving in checkSubFileSwitch()");
}

/*============================================================================
	ROUTINE: checkSubFileSwitch
 ============================================================================ */
void CPDFileThrd::waitForMaxTimeSwitch()
{
	TRACE(fms_cpf_CpFileTrace, "%s","Entering in waitForMaxTimeSwitch()");

    // Special use of subfileOption. HD96346
    unsigned char subfileOption = 255;
    off64_t subfileSize = 0;
    unsigned long fileReference = 0;
    unsigned short outRecordLength = 0;
    unsigned long outFileSize = 0;
    unsigned char outFileType = 0;

    int fileThrdSubFileSwitchDelaySec = static_cast<int>(FileParameter::FileThrdSubFileSwitchDelay / 1000);
    int waitOneSec = 1;

    // loop control
    bool noMoreTimeSwitch = false;

    // Wait until switch has been done or subfile is empty or maxtime is zero
    // or file failed to open
    while(!noMoreTimeSwitch)
    {
    	// wait in each cycle
        int maxWaitSec = 0;
        TRACE(fms_cpf_CpFileTrace, "waitForMaxTimeSwitch(), shutDown<%s>", (m_ServiceShutDown ? "TRUE" : "FALSE") );
        // wait 1 sec and check shutdown
        while(!m_ServiceShutDown && (maxWaitSec < fileThrdSubFileSwitchDelaySec))
        {
        	ACE_OS::sleep(waitOneSec);
        	maxWaitSec++;
        }

    	if(m_ServiceShutDown)
    	{
    		// Service shutdown, remove file open info and exit
    		InfiniteFileOpenList::instance()->remove(m_CpName, m_fileIdInList);  //HT89089
    		break;
    	}

        // If another infinite file has been opened remove from list
        // and terminate thread
        if( !InfiniteFileOpenList::instance()->removeIfMoreThanOneLeft(m_CpName, m_fileIdInList) )   //HT89089
        {
        	if(NULL == m_CpdFile)
           	{
            	noMoreTimeSwitch = true;
           	}
            else
            {
            	TRACE(fms_cpf_CpFileTrace, "waitForMaxTimeSwitch(), open file<%s>", m_fileIdInList.data() );
				try
				{
					m_CpdFile->open(m_fileIdInList,
								static_cast<unsigned char>(FMS_CPF_Types::NONE_),
								subfileOption,
								subfileSize,
								fileReference,
								outRecordLength,
								outFileSize,
								outFileType,
								m_FileInList);
				}
				catch(FMS_CPF_PrivateException& ex)
				{
					noMoreTimeSwitch = true;
					TRACE(fms_cpf_CpFileTrace, "%s","waitForMaxTimeSwitch(), exception on cpdfile object when open file");
					CPF_Log.Write("CPDFileThrd::waitForMaxTimeSwitch, exception on cpdfile object when open file", LOG_LEVEL_WARN);
				}

				// Check loop condition
				if(!noMoreTimeSwitch )
				{
					try
					{
						// check switch, noMoreTimeSwitch will be set
						m_CpdFile->checkSwitchSubFile(true, true, noMoreTimeSwitch);
					}
					catch(FMS_CPF_PrivateException& ex)
					{
						TRACE(fms_cpf_CpFileTrace, "%s", "waitForMaxTimeSwitch(), exception on cpdfile when checkSwitchSubFile");
						CPF_Log.Write("CPDFileThrd::waitForMaxTimeSwitch, exception on cpdfile when checkSwitchSubFile", LOG_LEVEL_WARN);
					}
				}
				// Try close in any case
				try
				{
					// close file but do not switch subfile
					m_CpdFile->close(subfileOption, true);
				}
				catch(FMS_CPF_PrivateException& ex)
				{
					TRACE(fms_cpf_CpFileTrace, "%s","waitForMaxTimeSwitch(), exception on cpdfile when close");
					CPF_Log.Write("CPDFileThrd::waitForMaxTimeSwitch, exception on cpdfile when close", LOG_LEVEL_WARN);
				}
            }

            if(noMoreTimeSwitch)
            {
            	InfiniteFileOpenList::instance()->remove(m_CpName, m_fileIdInList);   //HT89089
            }
        }
        else
        {
        	// Break the loop
            noMoreTimeSwitch = true;
        }
    }

    TRACE(fms_cpf_CpFileTrace, "%s","Leaving in waitForMaxTimeSwitch()");
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
int CPDFileThrd::close(u_long flags)
{
	TRACE(fms_cpf_CpFileTrace, "%s", "Entering close()");
	//To avoid unused warning
	UNUSED(flags);
	// Erase all queued messages
	msg_queue_->flush();

	// Erase the messages into internal map
	FMS_CPF_CPMsg* cpEnqueMsg;
	while( (cpEnqueMsg=fetchCpMsg()) != NULL )
	{
		// Reclaim the allocated memory
		cpEnqueMsg->release();
	}

	if( (NULL != m_CpdFile))
	{
		TRACE(fms_cpf_CpFileTrace, "%s","close, delete cpdfile object");
		delete m_CpdFile;
		m_CpdFile = NULL;
	}

	TRACE(fms_cpf_CpFileTrace, "%s", "close(), delete me");
	delete this;
	return SUCCESS;
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
int CPDFileThrd::open(void *args)
{
	TRACE(fms_cpf_CpFileTrace, "%s", "Entering open()");
	//To avoid unused warning
	UNUSED(args);
	int result;

	// Start the worker thread
	result = activate(THR_DETACHED);

	TRACE(fms_cpf_CpFileTrace, "%s", "Leaving open()");
	return result;
}
/*============================================================================
	ROUTINE: svc
 ============================================================================ */
int CPDFileThrd::svc()
{
	TRACE(fms_cpf_CpFileTrace, "%s", "Entering in svc()");
	pid_t threadId = syscall(SYS_gettid);

	int getResult;
	ACE_Message_Block* base_msg;
	FMS_CPF_CPMsg* cp_msg;

	bool svcRun = true;

	// To handle getq failure
	int MAX_RETRY = 5;
	int num_of_retry = 0;

	bool increaseSeqNum = false;
	// To handle out sync messages
	int msgOutOfSeqCnt = 0;
    int MOSQLoopCnt = 0;
    int maxNumOfOutSyncMsgs = static_cast<int>(FileParameter::MsgOutOfSeqExitDelay / FileParameter::FileThrdDelay);

    bool putResult;

    // timeout in sec
    int secOnWait = static_cast<int>(FileParameter::FileThrdDelay / 1000);

    ACE_Time_Value_T<ACE_Monotonic_Time_Policy> timeout(msg_queue_->gettimeofday());
    // Thread work loop
    while(svcRun)
    {
    	 timeout += ACE_Time_Value(secOnWait);

    	 getResult = getq(base_msg, &timeout);

    	 if(getResult >= 0)
    	 {
    		 TRACE(fms_cpf_CpFileTrace, "svc(), get of a new message thread ID<%d>, CP:<%s>", threadId, m_CpName.c_str()); //HT89089
    		 // Successful message get
			 num_of_retry = 0;

			 // Check if a internal shutdown request
			 if(base_msg->msg_type() == CPF_Message_Base::MT_EXIT)
			 {
				 svcRun = false;
				 base_msg->release();
				 continue;
			 }

			 cp_msg = reinterpret_cast<FMS_CPF_CPMsg*>(base_msg);

			 cp_msg->setReadyToSend(false);

			 msgOutOfSeqCnt = 0;

			 TRACE(fms_cpf_CpFileTrace, "svc(), msg SeqNum=<%d> m_NextSeqNr=<%d>, multiwrite:<%s>, forceClose:<%s>,  thread ID<%d>, CP:<%s>",
					 cp_msg->getInSequenceNumber(), m_NextSeqNr, (m_MultiWriteOn? "ON":"OFF"), (cp_msg->forcedClose()?"TRUE":"FALSE"), threadId,
					 m_CpName.c_str());  //HT89089

			 if((!m_MultiWriteOn) || (cp_msg->getInSequenceNumber() == m_NextSeqNr ) || cp_msg->forcedClose() )
			 {
				 TRACE(fms_cpf_CpFileTrace, "%s", "svc(), before loop()");
				 do
				 {
					 MOSQLoopCnt = 0;

					 // execute msg
					 cp_msg->thGo();

					 if(cp_msg->msg_type() ==  FMS_CPF_CPMsg::MT_CLOSE_MSG)
					 {
						 // After the handling of this message the thread must be stopped
						 svcRun = false;
					 }

					 if(cp_msg->forcedClose())
					 {
						 TRACE(fms_cpf_CpFileTrace, "%s", "svc(), forced closure!");
						 // We've received a forced close
						 // throw away messages in queue
						 FMS_CPF_CPMsg* cpEnqueMsg;
						 while( (cpEnqueMsg=fetchCpMsg()) != NULL )
						 {
							 // Reclaim the allocated memory
							 cpEnqueMsg->release();
						 }
					 }

					 cp_msg->setReadyToSend(true);

					 increaseSeqNum = cp_msg->incSeqNrAllowed();

					 // Puts the message in the CpChannel
					 putResult = CpChannelsList::instance()->putMsgToCpChannel(cp_msg->cpChannel(), cp_msg);

					 //Check operation result
					 if(!putResult)
					 {
						 // message not created
						 CPF_Log.Write("CPDFileThrd::svc, error to put message into CpChannel", LOG_LEVEL_ERROR);
						 TRACE(fms_cpf_CpFileTrace, "%s", "svc, error to put message into CpChannel");

						 bool isOpenMsg = (cp_msg->msg_type() ==  FMS_CPF_CPMsg::MT_OPEN_MSG);
						 // Reclaim the allocated memory
						 cp_msg->release();

						 if(isOpenMsg)
						 {
							 CPF_Log.Write("CPDFileThrd::svc, closing svc()", LOG_LEVEL_ERROR);
							 //to avoid missing SYNC message from CP
							 svcRun = false;

							 //Exit from while loop
							 break;
						 }
					}

					 // avoid to use again
					 cp_msg = NULL;

					 // Check if possible to increase sequence number
					 if(increaseSeqNum)
					 {
						 // increase message seq. number
						 m_NextSeqNr++;

						 // check if next msg already in queue
						 cp_msg = fetchCpMsg(m_NextSeqNr);
					 }

					 TRACE(fms_cpf_CpFileTrace, "svc(), end loop m_NextSeqNr=<%d>, thread id<%d>, CP:<%s>",
							 m_NextSeqNr, threadId, m_CpName.c_str() );  //HT89089

				 } while( NULL != cp_msg );
			 }
			 else
			 {
				 // Received message is out of sync
				 // message seq number is greaten than m_NextSeqNr
				 // insert it into internal map for next scheduling
				 insertCpMsg(cp_msg->getInSequenceNumber(), cp_msg);
				 msgOutOfSeqCnt++;
				 TRACE(fms_cpf_CpFileTrace, "svc, messages out of sync are <%d>, thread id<%d>, CP:<%s>",
						 msgOutOfSeqCnt, threadId, m_CpName.c_str());  //HT89089
			 }

			 checkSubFileSwitch();
    	 }
    	 else
    	 {
    		 if(errno != EWOULDBLOCK)
			 {
    			 // getq failure
				 num_of_retry++;
				 if(MAX_RETRY == num_of_retry)
				 {
					 CPF_Log.Write("CPDFileThrd::svc, reached the max number of getq failed", LOG_LEVEL_ERROR);
					 svcRun = false;
				 }
				 continue;
			 }

			 // Timeout is elapsed without receive a message
			 checkSubFileSwitch();
			 if( msgOutOfSeqCnt > 0) MOSQLoopCnt++;
		 }

    	 if( (msgOutOfSeqCnt > 0) && ( MOSQLoopCnt > maxNumOfOutSyncMsgs ) )
         {
    		 // One or more messages are missing.   (only when multiwrite on)
    		 TRACE(fms_cpf_CpFileTrace, "%s", "svc, one or more messages are missing");

    		 // block further messages
    		 CPDOpenFilesMgr::instance()->remove(m_FileReference, true, m_CpName);


    		 // Reply error on all messages in queue, and close file thread
    		 while((cp_msg = fetchCpMsg()) != NULL )
    		 {
    			 cp_msg->forceErrorCode(FMS_CPF_PrivateException::PARAMERROR);

    			 // execute msg to get an answer produced
    			 cp_msg->thGo();

    			 // Puts the message in the CpChannel
				 putResult = CpChannelsList::instance()->putMsgToCpChannel(cp_msg->cpChannel(), cp_msg);

				 //Check operation result
				 if(!putResult)
				 {
					 // message not created
					 CPF_Log.Write("CPDFileThrd::svc, error to put message into CpChannel", LOG_LEVEL_ERROR);
					 TRACE(fms_cpf_CpFileTrace, "%s", "svc, error to put message into CpChannel");
					 // Reclaim the allocated memory
					 cp_msg->release();
				 }
    		 }
    		 CPF_Log.Write("CPDFileThrd::svc, closing because one or more messages are missing", LOG_LEVEL_WARN);
    		 svcRun = false;
         }
    }

    TRACE(fms_cpf_CpFileTrace, "svc(), work loop of thread ID<%d>, CP:<%s> terminated",
    		threadId, m_CpName.c_str());  //HT89089

    if( !m_FileClosed && (NULL != m_CpdFile) )
    {
    	TRACE(fms_cpf_CpFileTrace, "svc(), close file by cpdfile object, thread ID<%d>, CP:<%s>",
    			threadId, m_CpName.c_str());  //HT89089
    	try
    	{
    		// close file if not already closed
    		if(m_SyncReceived)
    		{
    			// Change close-remove order sync-sync
    			m_CpdFile->close(0, false, true);
    		}
    		else
    		{
    			m_CpdFile->close(0);
    		}
    	}
    	catch(FMS_CPF_PrivateException& ex)
    	{
    		TRACE(fms_cpf_CpFileTrace, "svc(), cpdfile close exception:<%d>, thread ID<%d>, CP:<%s>",
    				ex.errorCode(), threadId, m_CpName.c_str());  //HT89089
    	}
    }

	TRACE(fms_cpf_CpFileTrace, "svc(), unlock file, thread ID<%d>",threadId);

	// Check if release FileLock
	if(NULL != m_OpenLock)
	{
		CPDOpenFilesMgr::instance()->unlockOpen(m_OpenLock, m_CpName);
	}

	m_OpenLock = NULL;

	if(m_FileInList &&
			!InfiniteFileOpenList::instance()->removeIfMoreThanOneLeft(m_CpName, m_fileIdInList))  //HT89089
		waitForMaxTimeSwitch();

	TRACE(fms_cpf_CpFileTrace, "svc(), remove file reference, thread ID<%d>, CP:<%s>",
			threadId, m_CpName.c_str());  //HT89089
	// block further messages
	CPDOpenFilesMgr::instance()->remove(m_FileReference, true, m_CpName);

	TRACE(fms_cpf_CpFileTrace, "Leaving svc(), thread ID<%d>, CP:<%s> exit", threadId, m_CpName.c_str() ); //HT89089
	return SUCCESS;
}


