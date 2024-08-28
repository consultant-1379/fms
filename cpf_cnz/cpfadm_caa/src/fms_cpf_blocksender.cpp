/*
 * * @file fms_cpf_blocksender.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_BlockSender.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_blocksender.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2013-06-21
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
 *	| 1.0.0  | 2013-06-21 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
  *	| 1.1.0  | 2014-04-30 | qvincon      | Infinite subfiles removed from IMM  |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_blocksender.h"
#include "fms_cpf_blocksender_logger.h"
#include "fms_cpf_common.h"
#include "fms_cpf_eventalarmhndl.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_filereference.h"

#include "fms_cpf_fileid.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"


#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

#include <inttypes.h>

extern ACS_TRA_Logging CPF_Log;

namespace BLOCK_TRANSFER_PARAMETER {

	uint32_t FIXED_DELAYMS = 20U; // 50 ms
	uint32_t SINGLE_BLOCK_DELAY = 15U; // 50 ms
	uint32_t MAX_COMMIT_DELAY = 300U; //  1000ms -> 1sec
	uint32_t STARTING_COMMIT_DELAY = 10U; //  1000ms -> 1sec

	uint32_t GETLASTBLOCK_DELAY = 50U; //  1000ms -> 1sec

	uint32_t SEC_2_MS = 1000U;
	uint32_t MS_2_NS = 1000000U;


};

/*============================================================================
	METHOD: FMS_CPF_BlockSender
 ============================================================================ */
FMS_CPF_BlockSender::FMS_CPF_BlockSender(const std::string& fileName, const std::string& volumeName, const std::string& cpName, const std::string& subFilePath, const int blockSize, bool systemType)
: m_FileName(fileName),
  m_VolumeName(volumeName),
  m_TqName(fileName),
  m_CpName(cpName),
  m_IsMultiCP(systemType),
  m_TqStreamId(),
  m_BlockSize(blockSize),
  m_activeSubFile(1),
  m_subFileToSend(1),
  m_readFD(-1),
  m_RemoveState(false),
  m_ohiBlockHandler(NULL),
  m_lastOhiError(AES_OHI_NOERRORCODE),
  m_LogFileHandler(new FMS_CPF_BlockSender_Logger(fileName, cpName, subFilePath)),
  m_mutex(),
  m_trace( new (std::nothrow) ACS_TRA_trace("FMS_CPF_BlockSender") )
{
	// Init the log file or read last info from it
	m_LogFileHandler->initLogFile();
	// set current active subfile and subfile to send
	m_activeSubFile = m_LogFileHandler->subFileStringToInt(subFilePath);
	m_subFileToSend = m_LogFileHandler->getSubFileToSendAsNumber();

	if(m_IsMultiCP)
		m_TqStreamId = m_CpName;
}

/*============================================================================
	METHOD: ~FMS_CPF_BlockSender
 ============================================================================ */
FMS_CPF_BlockSender::~FMS_CPF_BlockSender()
{
	detachToBlockTQ();

	//cease any alarm on this TQ
	EventReport::instance()->ceaseAlarm(m_TqName);

	if( NULL != m_ohiBlockHandler )
		delete m_ohiBlockHandler;

	if( NULL != m_LogFileHandler)
		delete m_LogFileHandler;

	if(NULL != m_trace)
		delete m_trace;
}

/*============================================================================
	METHOD: sendData
 ============================================================================ */
FMS_CPF_BlockSender::sendResult FMS_CPF_BlockSender::sendDataOnSCP(volatile bool& stopRequest)
{
	TRACE(m_trace, "Entering in %s()", __func__);
	FMS_CPF_BlockSender::sendResult result = NOMOREDATAEXIST;

	m_subFileToSend = m_LogFileHandler->getSubFileToSendAsNumber();

	if( m_subFileToSend < m_activeSubFile )
	{
		TRACE(m_trace, "%s(), start sending blocks from subfile:<%d>", __func__, m_subFileToSend);
		if( attachToBlockTQ())
		{
			do
			{
				// send data
				result = sendSubFileBlocks();

			}while( !stopRequest && (FMS_CPF_BlockSender::DATAEXIST == result) );

			TRACE(m_trace, "%s(), next subfile:<%d> should be send", __func__, m_subFileToSend);

			// Detach for the TQ
			detachToBlockTQ();
		}
		else
		{
			TRACE(m_trace, "%s(), attach failed", __func__);
			result = TQERROR;
		}
	}

	TRACE(m_trace, "Leaving %s(), result:<%d>", __func__, result);
	return result;
}

/*============================================================================
	METHOD: sendData
 ============================================================================ */
void FMS_CPF_BlockSender::sendData(const long& timeSlice)
{
	TRACE(m_trace, "Entering in %s(), CP:<%s>", __func__, m_CpName.c_str());

	m_subFileToSend = m_LogFileHandler->getSubFileToSendAsNumber();

	if( m_subFileToSend < m_activeSubFile )
	{
		// attach to the Block TQ
		if( attachToBlockTQ())
		{
			TRACE(m_trace, "%s(), start sending blocks from subfile:<%d>, CP:<%s>", __func__, m_subFileToSend, m_CpName.c_str());

			struct timespec startTime;
			//Get start time
			clock_gettime(CLOCK_MONOTONIC, &startTime);

			//Get start in milliseconds
			long int startTimeMs = (startTime.tv_sec * stdValue::MilliSecondsInSecond) + (static_cast<long int>(startTime.tv_nsec / stdValue::NanoSecondsInMilliSecond));

			// Start sending loop
			bool sendOtherData = true;
			FMS_CPF_BlockSender::sendResult result = NOMOREDATAEXIST;

			do
			{
				// try to send available data
				result = sendSubFileBlocks();

				if(FMS_CPF_BlockSender::DATAEXIST == result )
				{
					// there are other data to send check time slice
					struct timespec endTime;
					//Get end time
					clock_gettime(CLOCK_MONOTONIC, &endTime);

					// end time in milliseconds
					long int endTimeMs = (endTime.tv_sec * stdValue::MilliSecondsInSecond) + (static_cast<long int>(endTime.tv_nsec / stdValue::NanoSecondsInMilliSecond));

					// Calculate elapsed time in milliseconds
					long int elapsedTimeMs = endTimeMs - startTimeMs;

					// Checks how many time is elapsed
					sendOtherData = (elapsedTimeMs < timeSlice);

					TRACE(m_trace, "%s(), elapsed Time:<%lu>, CP Slice Time:<%lu>", __func__, elapsedTimeMs, timeSlice);
				}
				else
				{
					// Some error happened or there are not more data to send
					// skip to the next CP
					sendOtherData = false;
					TRACE(m_trace, "%s(), last send result:<%d>", __func__, result);
				}

			}while(sendOtherData);

			TRACE(m_trace, "%s(), next subfile:<%d> should be send, CP:<%s>", __func__, m_subFileToSend, m_CpName.c_str());

			// Detach for the TQ
			detachToBlockTQ();
		}
		else
		{
			TRACE(m_trace, "%s(), attach failed, CP:<%s>", __func__, m_CpName.c_str());
		}
	}

	TRACE(m_trace, "Leaving %s(), CP:<%s>", __func__, m_CpName.c_str());
}

/*============================================================================
	METHOD: getRemoveState
 ============================================================================ */
void FMS_CPF_BlockSender::getRemoveState(bool& isToRemove)
{
	TRACE(m_trace, "Entering in %s()", __func__);

	{
		ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, (isToRemove = m_RemoveState));

		// critical section
		//ACE_GUARD(ACE_RW_Thread_Mutex, guard, m_mutex);
		isToRemove = m_RemoveState;
	}

	TRACE(m_trace, "Leaving %s(), isToRemove:<%s>", __func__, (isToRemove ? "TRUE":"FALSE" ));
}

/*============================================================================
	METHOD: getRemoveState
 ============================================================================ */
void FMS_CPF_BlockSender::setRemoveState()
{
	TRACE(m_trace, "Entering in %s()", __func__);

	{
		// critical section
		ACE_GUARD_REACTION(ACE_RW_Thread_Mutex, guard, m_mutex, (m_RemoveState = true));
		m_RemoveState = true;
	}

	TRACE(m_trace, "Leaving %s()", __func__);
}

/*============================================================================
	METHOD: attachToBlockTQ
 ============================================================================ */
bool FMS_CPF_BlockSender::attachToBlockTQ()
{
	TRACE(m_trace, "Entering in %s()", __func__);
	bool result = false;

	// Create the OHI object
	if( createBlockTQHandler() )
	{
		// OHI object created, attach the block TQ
		m_lastOhiError = m_ohiBlockHandler->attach();

		if(AES_OHI_NOERRORCODE != m_lastOhiError)
		{
			//Start of fix for HV61830
			char errorMsg[512] = {'\0'};
			//raise alarm TQ error
			if ( (AES_OHI_TQNOTFOUND == m_lastOhiError) || (AES_OHI_NODESTINATION == m_lastOhiError))
			{
				ACE_OS::snprintf(errorMsg, 511, "%s(), attach to block TQ:<%s> failed with OHI error:<%d>, CpName:<%s>", __func__, m_TqName.c_str(), m_lastOhiError, m_CpName.c_str() );
				TRACE(m_trace, "%s", errorMsg);
				CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
				TRACE(m_trace, "%s(), Block TQ:<%s> not found for CP:<%s>",  __func__, m_TqName.c_str(), m_CpName.c_str());
				EventReport::instance()->raiseBlockTQAlarm(m_TqName);
			}
			else
			{
				ACE_OS::snprintf(errorMsg, 511, "%s(), attach to block TQ:<%s> failed with OHI error:<%d>, CpName:<%s>", __func__, m_TqName.c_str(), m_lastOhiError, m_CpName.c_str() );
				TRACE(m_trace, "%s", errorMsg);
				CPF_Log.Write(errorMsg, LOG_LEVEL_WARN);
				std::stringstream errorlog;
				errorlog<<"Unable to connect block transfer queue"<<std::ends;
				EventReport::instance()->reportException(errorlog,FMS_CPF_PrivateException::ATTACHFAILED);
			}
			//End of fix for HV61830
		}
		else
		{
			TRACE(m_trace, "%s(), attached to TQ:<%s>, CP:<%s>",  __func__, m_TqName.c_str(), m_CpName.c_str());
			//cease any alarm on this TQ
			EventReport::instance()->ceaseAlarm(m_TqName);

			// wait a min fix delay before try again to get last committed block
			timespec fixDelay;
			fixDelay.tv_sec = 0U;
			fixDelay.tv_nsec = BLOCK_TRANSFER_PARAMETER::GETLASTBLOCK_DELAY * BLOCK_TRANSFER_PARAMETER::MS_2_NS;
			nanosleep(&fixDelay, NULL);

			// get last committed block from remote side
			unsigned int lastCommitedBlock = 0U;

			// Get the last committed block, it is needed before begin a transaction
			m_lastOhiError = m_ohiBlockHandler->getLastCommitedBlockNo(lastCommitedBlock);

			TRACE(m_trace, "%s(), last committed block:<%d>, OHI error:<%d>", __func__, lastCommitedBlock, m_lastOhiError);

			if( (AES_OHI_NOERRORCODE == m_lastOhiError) ||
				(AES_OHI_BLOCKNRFROMGOHLIST == m_lastOhiError) ||
				(AES_OHI_BLOCKNRNOTAVAILABLE == m_lastOhiError) )
			{
				result = true;

				if( (m_LogFileHandler->getLastBlockCommited() != lastCommitedBlock) )
				{
					char errorMsg[512] = {'\0'};
					ACE_OS::snprintf(errorMsg, 511, "%s(), OHI returns lastCommitedBlock:<%d> mismatch with CPF Sender lastCommitBlock:<%d>, OHI error code:<%d>  file:<%s> of CpName:<%s>",
												  __func__, lastCommitedBlock, m_LogFileHandler->getLastBlockCommited(), m_lastOhiError, m_FileName.c_str(), m_CpName.c_str() );
					TRACE(m_trace, "%s", errorMsg);
					CPF_Log.Write(errorMsg,	LOG_LEVEL_ERROR);

					// Update internal info with OHI returned info
					m_LogFileHandler->setLastBlockCommited(lastCommitedBlock);

					m_LogFileHandler->setBlockNumberOffset(lastCommitedBlock);
				}
			}
			else
			{
				// OHI getLastCommitedBlockNo() failed
				char errorMsg[512] = {'\0'};
				ACE_OS::snprintf(errorMsg, 511, "%s(), getLastCommitedBlockNo() of block TQ:<%s> failed with OHI error:<%d>, CpName:<%s>", __func__, m_TqName.c_str(), m_lastOhiError, m_CpName.c_str() );
				TRACE(m_trace, "%s", errorMsg);
				CPF_Log.Write(errorMsg,	LOG_LEVEL_ERROR);

				detachToBlockTQ();
			}
		}
	}

	TRACE(m_trace, "Leaving %s()", __func__);
	return result;
}

/*============================================================================
	METHOD: detachToBlockTQ
 ============================================================================ */
void FMS_CPF_BlockSender::detachToBlockTQ()
{
	TRACE(m_trace, "Entering in %s()", __func__);
	if(NULL != m_ohiBlockHandler && m_ohiBlockHandler->isConnected() )
	{
		m_lastOhiError = m_ohiBlockHandler->detach();

		if(AES_OHI_NOERRORCODE != m_lastOhiError)
		{
			char errorMsg[512] = {'\0'};
			ACE_OS::snprintf(errorMsg, 511, "%s(), detach to block TQ:<%s> failed with OHI error:<%d>, CpName:<%s>", __func__, m_TqName.c_str(), m_lastOhiError, m_CpName.c_str() );
			TRACE(m_trace, "%s", errorMsg);
			CPF_Log.Write(errorMsg,	LOG_LEVEL_ERROR);
		}
	}
	TRACE(m_trace, "Leaving %s()", __func__);
}

/*============================================================================
	METHOD: sendSubFileBlocks
 ============================================================================ */
FMS_CPF_BlockSender::sendResult FMS_CPF_BlockSender::sendSubFileBlocks()
{
	TRACE(m_trace, "Entering in %s()", __func__);
	FMS_CPF_BlockSender::sendResult result = TQERROR;

	// begin new transaction
	if( blockTransfer_TranscationBegin() )
	{
		TRACE(m_trace, "%s(), transaction begin", __func__);
		result = DATAEXIST;

		int readFD;
		uint32_t currentBlockSent = m_LogFileHandler->getLastBlockCommited();

		TRACE(m_trace, "%s(), currentBlockSent:<%d>", __func__, currentBlockSent);
		// Open current file to transfer
		if(openSubFileToSend(readFD) )
		{
			// read block and send it
			TRACE(m_trace, "%s(), file open", __func__);
			char bufferTosend[m_BlockSize+1];
			int readResult;
			bool continueToSend = true;
			bool endOfFile = false;

			do
			{
				// Init buffer to zero
				memset(bufferTosend, 0, m_BlockSize+1);
				// Read block to send
				readResult = read(readFD, bufferTosend, m_BlockSize);

				if(readResult > 0)
				{
					// send block
					continueToSend = sendBlockToTQ(bufferTosend, currentBlockSent);
				}
				else if( 0 == readResult )
				{
					// EOF, all blocks read and send
					continueToSend = false;
					endOfFile = true;
				}
				else
				{
					continueToSend = false;
					// Error on buffer read
					char errorLog[512]={'\0'};
					char errorText[64]={'\0'};
					std::string errorDetail(strerror_r(errno, errorText, 64));
					ACE_OS::snprintf(errorLog, 511, "%s(), read buffer on file:<%s> failed, error=<%s>, cpName:<%s>",
												__func__, m_LogFileHandler->getSubFileToSend(), errorDetail.c_str(), m_CpName.c_str());
					CPF_Log.Write(errorLog, LOG_LEVEL_ERROR);
					TRACE(m_trace, "%s", errorLog);
				}

			} while(continueToSend);

			// Close opened subfile
			close(readFD);

			TRACE(m_trace, "%s(), blocks sent, currentBlockSent:<%d>, EOF:<%s>", __func__, currentBlockSent, ( endOfFile ? "TRUE" : "FALSE") );

			// check no block send case
			if(m_LogFileHandler->getLastBlockCommited() == currentBlockSent)
			{
				TRACE(m_trace, "%s(), no blocks sent", __func__ );

				// No block sent
				if(endOfFile)
				{
					// The current file has size zero
					// remove subfile and update attribute
					updateInfiniteFileState();

					char errorLog[256]={0};
					ACE_OS::snprintf(errorLog, 255, "%s(), zero size subfile:<%s> found or all block already sent, cpName:<%s>",
																__func__, m_LogFileHandler->getSubFileToSend(), m_CpName.c_str());
					CPF_Log.Write(errorLog, LOG_LEVEL_ERROR);
					TRACE(m_trace, "%s", errorLog);

					// Set the next subfile to sent
					++m_subFileToSend;
					// Update the log file
					m_LogFileHandler->updateLogFile(m_subFileToSend, currentBlockSent, currentBlockSent);
				}

				// terminate the begun transaction, since no block sent
				m_ohiBlockHandler->transactionTerminate();
			}
			else
			{
				// Complete the current transaction
				blockTransfer_TranscationEnd(currentBlockSent, endOfFile);
			}

			result = (m_subFileToSend < m_activeSubFile) ? DATAEXIST : NOMOREDATAEXIST;
		}
		else
		{
			// Open file failed
			// terminate the begun transaction
			m_ohiBlockHandler->transactionTerminate();
		}
	}

	TRACE(m_trace, "Leaving %s()", __func__);
	return result;
}

/*============================================================================
	METHOD: blockTransfer_TranscationBegin
 ============================================================================ */
bool FMS_CPF_BlockSender::blockTransfer_TranscationBegin()
{
	TRACE(m_trace, "Entering in %s()", __func__);
	bool result = false;

	if(NULL != m_ohiBlockHandler)
	{
			// New transaction begin
			m_lastOhiError = m_ohiBlockHandler->transactionBegin();

			if( (AES_OHI_NOERRORCODE == m_lastOhiError) )
			{
				TRACE(m_trace, "%s(), new transaction begin", __func__);
				result = true;
			}
			else
			{
				// OHI transactionBegin() failed
				char errorMsg[512] = {'\0'};
				ACE_OS::snprintf(errorMsg, 511, "%s(), transactionBegin() of block TQ:<%s> failed with OHI error:<%d>, CpName:<%s>", __func__, m_TqName.c_str(), m_lastOhiError, m_CpName.c_str() );
				TRACE(m_trace, "%s", errorMsg);
				CPF_Log.Write(errorMsg,	LOG_LEVEL_ERROR);

				// Even if the result code from transactionBegin is negative
				// the application cannot repeat the call for transactionBegin.
				// The next operation to call is the send or transactionTerminate
				m_ohiBlockHandler->transactionTerminate();
			}
	}

	TRACE(m_trace, "Leaving %s()", __func__);
	return result;
}

/*============================================================================
	METHOD: sendBlockToTQ
 ============================================================================ */
bool FMS_CPF_BlockSender::sendBlockToTQ(const char* buffer, uint32_t& blockSentNumber)
{
	TRACE(m_trace, "Entering in %s()", __func__);
	bool result = true;
	++blockSentNumber;
	m_lastOhiError = m_ohiBlockHandler->send(buffer, m_BlockSize, blockSentNumber);

	if( AES_OHI_NOERRORCODE != m_lastOhiError)
	{
		result = false;
		char errorMsg[512] = {'\0'};
		ACE_OS::snprintf(errorMsg, 511, "%s(), block send failed, OHI error:<%d> file:<%s>, cpName:<%s>",
									__func__, m_lastOhiError, m_FileName.c_str(), m_CpName.c_str());
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", errorMsg);

		if( AES_OHI_NOTINSYNCH == m_lastOhiError )
		{
			m_ohiBlockHandler->transactionTerminate();
			uint32_t lastCommitedBlock;
			if( m_ohiBlockHandler->getLastCommitedBlockNo(lastCommitedBlock) == AES_OHI_NOERRORCODE)
			{
				TRACE(m_trace, "%s(), getLastCommitedBlockNo:<%d>", __func__, lastCommitedBlock);
				m_LogFileHandler->setLastBlockCommited(lastCommitedBlock);
			}
			detachToBlockTQ();
		}
		else if(AES_OHI_BUFFERFULL == m_lastOhiError)
		{
			// Buffer full, in order to avoid re-send wait a moment and try commit
			sleep(1);
		}
		else
		{
			m_ohiBlockHandler->transactionTerminate();
			detachToBlockTQ();
		}
	}
	TRACE(m_trace, "Leaving %s()", __func__);
	return result;
}

/*============================================================================
	METHOD: blockTransfer_TranscationEnd
 ============================================================================ */
void FMS_CPF_BlockSender::blockTransfer_TranscationEnd(const uint32_t& currentSentBlock, bool endOfFile)
{
	TRACE(m_trace, "Entering in %s()", __func__);
	// Checks if there is something to commit
	if(NULL != m_ohiBlockHandler && m_ohiBlockHandler->isConnected())
	{
		// wait a min fix delay before transaction end
		timespec fixDelay;
		fixDelay.tv_sec = 0U;
		fixDelay.tv_nsec = BLOCK_TRANSFER_PARAMETER::FIXED_DELAYMS * BLOCK_TRANSFER_PARAMETER::MS_2_NS * 4; //HW36722 changes;
		nanosleep(&fixDelay, NULL);

		uint32_t handledBlockByOHI;
		m_lastOhiError = m_ohiBlockHandler->transactionEnd(handledBlockByOHI);

		TRACE(m_trace, "%s(), transactionEnd() returns blockNr:<%u>, OHI error:<%d> ", __func__, handledBlockByOHI, m_lastOhiError);

		if(AES_OHI_NOERRORCODE == m_lastOhiError)
		{
			if(handledBlockByOHI > currentSentBlock)
			{
				TRACE(m_trace, "%s(), last block handled by OHI:<%u> is more big!", __func__, handledBlockByOHI );
				char errorLog[512]={'\0'};
				ACE_OS::snprintf(errorLog, 511, "%s(), transactionEnd() returns a last commited block:<%u> more big of blockNrSent:<%u>, OHI error=<%d>, file:<%s> cpName:<%s>",
												__func__, handledBlockByOHI, currentSentBlock, m_lastOhiError, m_LogFileHandler->getSubFileToSend(), m_CpName.c_str());

				CPF_Log.Write(errorLog, LOG_LEVEL_ERROR);
				TRACE(m_trace, "%s", errorLog);
			}
			else if(handledBlockByOHI < currentSentBlock)
			{
				//Start of TR HW36722 changes
				uint32_t minCommitDelay = 0U;
				uint32_t missingBlocks = 0U;
				uint32_t commitDelay = 0U;
				int32_t commitDelayEv = 0U;
				uint32_t sentBlocks = (handledBlockByOHI - m_LogFileHandler->getLastBlockCommited()) + 1U;

				if(sentBlocks == 0U) // Safe check to avoid division by 0
				{

					char errorMsg[512] = {'\0'};
					ACE_OS::snprintf(errorMsg,511,"%s(), ****SENTBLOCK IS 0**** HandledblockbyOHI:<%u>, currentSentBlock:<%u>, logFileCommitedBlock:<%u>, CpName:<%s>", __func__, handledBlockByOHI, currentSentBlock, m_LogFileHandler->getLastBlockCommited(), m_CpName.c_str());
					CPF_Log.Write(errorMsg,LOG_LEVEL_ERROR);
					sentBlocks = 1U;
				}
				// "Wait a moment" (as reported into SI of block transfer) in order to avoid block re-send
				missingBlocks = currentSentBlock - handledBlockByOHI;

				commitDelayEv = (missingBlocks * static_cast<uint32_t>(BLOCK_TRANSFER_PARAMETER::FIXED_DELAYMS / sentBlocks));

				commitDelay = (missingBlocks * BLOCK_TRANSFER_PARAMETER::SINGLE_BLOCK_DELAY / sentBlocks);

				minCommitDelay = std::min(commitDelay, BLOCK_TRANSFER_PARAMETER::MAX_COMMIT_DELAY);
				//End of TR HW36722 changes

				timespec delay;
				delay.tv_sec = static_cast<int> (minCommitDelay / BLOCK_TRANSFER_PARAMETER::SEC_2_MS);
				delay.tv_nsec = ( minCommitDelay % BLOCK_TRANSFER_PARAMETER::SEC_2_MS ) * BLOCK_TRANSFER_PARAMETER::MS_2_NS;

				TRACE(m_trace, "%s(), sentBlocks: <%u> last block handled by OHI:<%u> is more little!, blocks to send:<%d>, wait:<%d> msec before commit", __func__, sentBlocks, handledBlockByOHI, missingBlocks, minCommitDelay );
				TRACE(m_trace, "%s(), COMMIT DELAY FIX:<%d> EVA:<%d>", __func__, commitDelay, commitDelayEv);
				nanosleep(&delay, NULL);
			}

			m_lastOhiError = m_ohiBlockHandler->transactionCommit(handledBlockByOHI);

			TRACE(m_trace, "%s(), transactionCommit() returns blockCommitedNr:<%u>, OHI error:<%d> ", __func__, handledBlockByOHI, m_lastOhiError);

			if(AES_OHI_NOERRORCODE == m_lastOhiError)
			{
				if(handledBlockByOHI > currentSentBlock)
				{
					TRACE(m_trace, "%s(), last block handled by OHI:<%d> is more big!", __func__, handledBlockByOHI );
					char errorLog[512]={'\0'};
					ACE_OS::snprintf(errorLog, 511, "%s(), transactionEnd() returns a last commited block:<%u> more big of blockNrSent:<%u>, OHI error=<%d>, file:<%s> cpName:<%s>",
										__func__, handledBlockByOHI, currentSentBlock, m_lastOhiError, m_LogFileHandler->getSubFileToSend(), m_CpName.c_str());
					CPF_Log.Write(errorLog, LOG_LEVEL_ERROR);
					TRACE(m_trace, "%s", errorLog);
				}
				else if( (handledBlockByOHI == currentSentBlock) && endOfFile)
				{
					// Check if file sent is completed
					TRACE(m_trace, "%s(), all subfile blocks are been sent, last committed block:<%u>", __func__, handledBlockByOHI );

					// remove subfile sent and update IMM
					updateInfiniteFileState();

					// Set the next subfile to sent
					++m_subFileToSend;
					// Update the log file
					m_LogFileHandler->updateLogFile(m_subFileToSend, handledBlockByOHI, handledBlockByOHI);
				}
				else
				{
					// There are other block to sent
					TRACE(m_trace, "%s(),  there are other subfile blocks to send, last committed block:<%d>", __func__, handledBlockByOHI);
					// Update last committed block into the log file
					m_LogFileHandler->updateLogFile(handledBlockByOHI);
				}
			}
			else
			{
				char errorLog[512]={'\0'};
				ACE_OS::snprintf(errorLog, 511, "%s(), transactionCommit() on file:<%s> failed, OHI error=<%d>, cpName:<%s>",
															__func__, m_LogFileHandler->getSubFileToSend(), m_lastOhiError, m_CpName.c_str());
				CPF_Log.Write(errorLog, LOG_LEVEL_ERROR);
				TRACE(m_trace, "%s", errorLog);
			}
		}
		else
		{
			char errorLog[512]={'\0'};
			ACE_OS::snprintf(errorLog, 511, "%s(), transactionEnd() on file:<%s> failed, OHI error=<%d>, cpName:<%s>",
														__func__, m_LogFileHandler->getSubFileToSend(), m_lastOhiError, m_CpName.c_str());
			CPF_Log.Write(errorLog, LOG_LEVEL_ERROR);
			TRACE(m_trace, "%s", errorLog);
			// Error on transaction end
			m_ohiBlockHandler->transactionTerminate();
		}
	}
	TRACE(m_trace, "Leaving %s()", __func__);
}

/*============================================================================
	METHOD: createBlockTQHandler
 ============================================================================ */
bool FMS_CPF_BlockSender::createBlockTQHandler()
{
	TRACE(m_trace, "Entering in %s()", __func__);
	bool result = true;

	if(NULL == m_ohiBlockHandler)
	{
		TRACE(m_trace, "%s(), create block TQ:<%s> handler, stream ID:<%s>", __func__, m_TqName.c_str(), m_TqStreamId.c_str());
		m_ohiBlockHandler = new (std::nothrow) AES_OHI_BlockHandler2(OHI_USERSPACE::SUBSYS,
																	 OHI_USERSPACE::APPNAME,
																	 m_TqName.c_str(),
																	 OHI_USERSPACE::eventText,
																	 m_TqStreamId.c_str());
		if(NULL == m_ohiBlockHandler)
		{
			result = false;
			char errorMsg[512] = {'\0'};
			ACE_OS::snprintf(errorMsg, 511, "%s(), block TQ:<%s> allocation failed, Cp:<%s>", __func__, m_TqName.c_str(), m_CpName.c_str() );
			TRACE(m_trace, "%s", errorMsg);
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		}
	}

	TRACE(m_trace, "Leaving %s()", __func__);
	return result;
}

/*============================================================================
	METHOD: openSubFileToSend
 ============================================================================ */
bool FMS_CPF_BlockSender::openSubFileToSend(int& readFD)
{
	TRACE(m_trace, "Entering in %s()", __func__);
	bool result = false;
	// Open in read mode
	readFD = ::open(m_LogFileHandler->getSubFileToSend(), O_RDONLY | O_BINARY);

	if( FAILURE == readFD )
	{
		// get the open error
		int openError = errno;

		char errorLog[512]={'\0'};
		char errorText[64]={'\0'};
		std::string errorDetail(strerror_r(openError, errorText, 64));

		ACE_OS::snprintf(errorLog, 511, "%s(), opening file:<%s> in read mode failed, error=< %d -> %s>", __func__, m_LogFileHandler->getSubFileToSend(), openError, errorDetail.c_str());
		CPF_Log.Write(errorLog, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", errorLog);
		// Check if exist the source file
		if(ENOENT == openError)
		{
			// The source subfile doesn't exist.
			// Switch to the next one, since the problem is not recoverable

			// Set the next subfile to sent
			++m_subFileToSend;
			// Update the log file
			m_LogFileHandler->updateSubFileToLogFile(m_subFileToSend);
		}
	}
	else
	{
		// Subfile opened
		off64_t offSetPos = m_BlockSize * ( m_LogFileHandler->getLastBlockCommited() - m_LogFileHandler->getBlockNumberOffset() );

		TRACE(m_trace, "%s(), subfile opened, seek to pos:<%lld>", __func__, offSetPos);

		if( FAILURE == lseek64(readFD, offSetPos, SEEK_SET))
		{
			char errorLog[512]={'\0'};
			char errorText[64]={'\0'};
			std::string errorDetail(strerror_r(errno, errorText, 64));
			ACE_OS::snprintf(errorLog, 511, "%s(), lseek of bytes:<%zd> on file:<%s> failed, error:<%s>, commited block:<%u> offset block:<%u>",
												__func__, offSetPos, m_LogFileHandler->getSubFileToSend(), errorDetail.c_str(), m_LogFileHandler->getLastBlockCommited(), m_LogFileHandler->getBlockNumberOffset() );
			CPF_Log.Write(errorLog, LOG_LEVEL_ERROR);
			TRACE(m_trace, "%s", errorLog);
		}

		// In any case, also if seek fails, send all records
		result = true;
	}
	TRACE(m_trace, "Leaving %s()", __func__);
	return result;
}

/*============================================================================
	METHOD: updateInfiniteFileState
 ============================================================================ */
void FMS_CPF_BlockSender::updateInfiniteFileState()
{
	TRACE(m_trace, "Entering in %s()", __func__);

	boost::system::error_code delResult;
	boost::filesystem::remove(m_LogFileHandler->getSubFileToSend(), delResult);

	// Check remove result
	if(SUCCESS !=  delResult.value())
	{
		// remove error log it
		char errorLog[256] = {0};
		ACE_OS::snprintf(errorLog, 255, "%s(), error:<%d> on remove subfile<%s> file:<%s>, Cp Name:<%s>", __func__, delResult.value(),
													m_LogFileHandler->getSubFileToSend(), m_FileName.c_str(), m_CpName.c_str() );
		CPF_Log.Write(errorLog,	LOG_LEVEL_ERROR);
	}

	// Update last sent subfile
	FMS_CPF_FileId fileId(m_FileName);
	FileReference reference;

	// Open the CP file
	try
	{
		// Open logical file
		reference = DirectoryStructureMgr::instance()->open(fileId, FMS_CPF_Types::NONE_, m_CpName.c_str());
		// update last sent subfile into file attribute
		reference->setLastSentSubfile(m_subFileToSend);
	}
	catch(const FMS_CPF_PrivateException& ex)
	{
		char errorMsg[256] = {0};
		snprintf(errorMsg, 255, "%s, exception on file=<%s> attribute update, error=<%s>",__func__, m_FileName.c_str(), ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", errorMsg);
	}

	// Close CP file
	DirectoryStructureMgr::instance()->closeExceptionLess(reference, m_CpName.c_str());

	TRACE(m_trace, "Leaving %s()", __func__);
}
