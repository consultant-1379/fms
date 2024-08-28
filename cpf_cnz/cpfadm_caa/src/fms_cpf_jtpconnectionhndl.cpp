/*
 * * @file fms_cpf_jtp_connectionhndl.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_JTP_ConnectionHndl.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_jtp_connectionhndl.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-08-06
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
 *	| 1.0.0  | 2012-08-06 | qvincon      | File created.                       |
  *	+========+============+==============+=====================================+
 *	| 1.0.2  | 2014-09-03 | qvincon      | Multi CP changes                    |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_jtpconnectionhndl.h"
#include "fms_cpf_jtpconnectionsmgr.h"
#include "fms_cpf_jtpcpmsg.h"
#include "fms_cpf_tqchecker.h"
#include "fms_cpf_filetransferhndl.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filereference.h"
#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_common.h"
#include "fms_cpf_fileid.h"
#include "fms_cpf_eventalarmhndl.h"
#include "aes_ohi_filehandler.h"
#include "fms_cpf_parameterhandler.h"
#include "acs_apgcc_omhandler.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include <ace/Dirent.h>
#include <fstream>

extern ACS_TRA_Logging CPF_Log;
DeleteFileTimerMap FMS_CPF_JTPConnectionHndl::theDeleteFileTimerMap;
CompositeFileMap FMS_CPF_JTPConnectionHndl::theCompositeFileMap;
TimeSortedMapPair FMS_CPF_JTPConnectionHndl::theTimeSortedMapPair;
bool FMS_CPF_JTPConnectionHndl::m_isShutdownSignaled = false;
ACE_Recursive_Thread_Mutex FMS_CPF_JTPConnectionHndl::theMutex;
ACS_TRA_trace FMS_CPF_JTPConnectionHndl::m_trace("FMS_CPF_JTPConnectionHndl");

namespace {
const char CREATION_TIME[] = "CREATIONDATE";
const char FILE_NAME[] = "FILENAME";
}

/*============================================================================
	ROUTINE: FMS_CPF_JTPConnectionHndl
 ============================================================================ */
FMS_CPF_JTPConnectionHndl::FMS_CPF_JTPConnectionHndl(ACS_JTP_Conversation* jtpSession, bool systemType)
: m_JTPSession(jtpSession),
  m_JTPMsg(NULL),
  m_IsMultiCP(systemType),
  m_ThreadState(false)
{
}

/*============================================================================
	ROUTINE: ~FMS_CPF_JTPConnectionHndl
 ============================================================================ */
FMS_CPF_JTPConnectionHndl::~FMS_CPF_JTPConnectionHndl()
{
	if(NULL != m_JTPSession)
		delete m_JTPSession;

	if(NULL != m_JTPMsg)
		delete m_JTPMsg;
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
int FMS_CPF_JTPConnectionHndl::open(void *args)
{
	ACS_TRA_trace* trace = &m_trace;
	TRACE(trace, "%s", "Entering open()");
	// To avoid warning about unused parameter
	UNUSED(args);
	// start event loop by svc thread
	// wait on it is required to deallocate all resources
	int result = activate();

	// Check if the svc thread is started
	if(SUCCESS != result)
	{
		CPF_Log.Write("FMS_CPF_JTPConnectionHndl::open(), error on start svc thread", LOG_LEVEL_ERROR);
		TRACE(trace, "%s", "open(), error on start svc thread");
	}
	else
		m_ThreadState = true;

	TRACE(trace, "%s", "Leaving open()");
	return result;
}

/*============================================================================
	ROUTINE: svc
 ============================================================================ */
int FMS_CPF_JTPConnectionHndl::svc()
{
	ACS_TRA_trace* trace = &m_trace;
	TRACE(trace, "%s", "Entering svc()");
	int result = SUCCESS;

	ACS_JTP_Conversation::JTP_Node jtpNode;
	uint16_t userData1 = 0;
	uint16_t userData2 = 0;
	do
	{
		// JTP conversation initiation indication
		if( !m_JTPSession->jexinitind(jtpNode, userData1, userData2) )
		{
			CPF_Log.Write("FMS_CPF_JTPConnectionHndl::svc(), error on call <jexinitind()> method of ACS_JTP_Conversation object", LOG_LEVEL_ERROR);
			TRACE(trace, "%s", "svc(), error on call <jexinitind()> method");
			break;
		}

		TRACE(trace, "svc(), jtp node info: id:<%i>, name:<%s>, state:<%i>", jtpNode.system_id, jtpNode.system_name, jtpNode.node_state);

		// set the CP name
		if(m_IsMultiCP)
		{
			m_CpName = jtpNode.system_name;
		}

		// reply with conversation accepted
		uint16_t replyCode = 0;
		// JTP conversation initiation response
		if( !m_JTPSession->jexinitrsp(userData1, userData2, replyCode) )
		{
			CPF_Log.Write("FMS_CPF_JTPConnectionHndl::svc(), error on call <jexinitrsp()> method of ACS_JTP_Conversation object", LOG_LEVEL_ERROR);
			TRACE(trace, "%s", "svc(), error on call <jexinitrsp()> method");
			break;
		}

		uint16_t msgBufferLength = 0;
		char* msgBuffer = NULL;
		// Retrieve message data
		if( !m_JTPSession->jexdataind(userData1, userData2, msgBufferLength, msgBuffer) )
		{
			CPF_Log.Write("FMS_CPF_JTPConnectionHndl::svc(), error on call <jexdataind()> method of ACS_JTP_Conversation object", LOG_LEVEL_ERROR);
			TRACE(trace, "%s", "svc(), error on call <jexdataind()> method");

			// Reply with a fault code
			if( !m_JTPSession->jexdiscreq(userData1, userData2, CpReplyCode::CP_OTHERFAULT) )
			{
				CPF_Log.Write("FMS_CPF_JTPConnectionHndl::svc(), error on call <jexdiscreq()> method of ACS_JTP_Conversation object", LOG_LEVEL_ERROR);
				TRACE(trace, "%s", "svc(), error on call <jexdiscreq()> method");
			}
			break;
		}

		TRACE(trace, "svc(), retrieved msg lenght:<%i>", msgBufferLength);

		m_JTPMsg = new (std::nothrow) FMS_CPF_JTPCpMsg(msgBuffer, msgBufferLength);

		if(NULL != m_JTPMsg)
		{
			// decode retrieved Cp data
			m_JTPMsg->decode();

			// handling the Cp request
			handlingCpRequest();

			// encode the info to send to the Cp
			m_JTPMsg->encode();

			TRACE(trace, "%s", "svc(), send response to CP");
			// send data to CP
			if(!m_JTPSession->jexdatareq(userData1, userData2, m_JTPMsg->getOutputBufferSize(), m_JTPMsg->getOutputBuffer()) )
			{
				CPF_Log.Write("FMS_CPF_JTPConnectionHndl::svc(), error on call <jexdatareq()> method of ACS_JTP_Conversation object", LOG_LEVEL_ERROR);
				TRACE(trace, "%s", "svc(), error send data to Cp when call <jexdatareq()> method");
			}

			TRACE(trace, "%s", "svc(), wait for Cp to close connection");

			uint16_t disconnectReason = CpReplyCode::CP_OK;

			// Wait for CP to close connection
			if(!m_JTPSession->jexdiscind(userData1, userData2, disconnectReason) )
			{
				CPF_Log.Write("FMS_CPF_JTPConnectionHndl::svc(), error on call <jexdiscind()> method of ACS_JTP_Conversation object", LOG_LEVEL_ERROR);
				TRACE(trace, "%s", "svc(), error on disconnect wait when call <jexdiscind()> method");
			}

			TRACE(trace, "%s", "svc(), JTP connection closed");
		}

	}while(false);

	TRACE(trace, "%s", "Leaving open()");
	return result;
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
int FMS_CPF_JTPConnectionHndl::close(u_long flags)
{
        ACS_TRA_trace* trace = &m_trace;
	// To avoid warning about unused parameter
	UNUSED(flags);
	TRACE(trace, "%s", "close");
	m_ThreadState = false;
	return SUCCESS;
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
void FMS_CPF_JTPConnectionHndl::handlingCpRequest()
{ 
	ACS_TRA_trace* trace = &m_trace;
	TRACE(trace, "%s", "Entering handlingCpRequest()");

	unsigned int ohiResult = AES_OHI_EXECUTEERROR;

	switch( m_JTPMsg->getFunctionCode() )
	{
		case FMS_CPF_JTPCpMsg::SUBFILEREM :
		{
			TRACE(trace, "%s", "handlingCpRequest(), remove subfile");
			ohiResult = removeSentFile();
			break;
		}

		case FMS_CPF_JTPCpMsg::SUBFILEINIT :
		{
			TRACE(trace, "%s", "handlingCpRequest(), report subfile");
			ohiResult = sendFile();
			break;
		}

		case FMS_CPF_JTPCpMsg::SETSFSTAT :
		{
			TRACE(trace, "%s", "handlingCpRequest(), set status ARCHIVE for a subfile");
			ohiResult = setFileArchived();
			break;
		}

		case FMS_CPF_JTPCpMsg::FUFILELIST :
		{
			TRACE(trace, "%s", "handlingCpRequest(), listing status for a subfile");
			ohiResult = getSentFileStatus();
			break;
		}

		default:
		{
			char msgBuf[256] = {0};
			snprintf(msgBuf, 255, "FMS_CPF_JTPConnectionHndl::handlingCpRequest(), unknown Cp function code:<%i>", m_JTPMsg->getFunctionCode());
			CPF_Log.Write(msgBuf, LOG_LEVEL_ERROR);
			TRACE(trace, "%s", msgBuf);
		}
	}

	m_JTPMsg->setCpResultCode(ohiResult);

	TRACE(trace, "%s", "Leaving handlingCpRequest()");
}

/*============================================================================
	ROUTINE: removeSentFile
 ============================================================================ */
unsigned int FMS_CPF_JTPConnectionHndl::removeSentFile()
{
	ACS_TRA_trace* trace = &m_trace;
	TRACE(trace, "Entering %s()", __func__);

	std::string tQName = m_JTPMsg->getFileName();
	std::string fullFileName;

	if(m_IsMultiCP)
	{
		fullFileName = m_CpName;
		fullFileName.push_back(parseSymbol::plus);
		fullFileName.append(m_JTPMsg->getFileName());
		fullFileName.push_back(parseSymbol::minus);
		fullFileName.append(m_JTPMsg->getSubFileName());
		TRACE(trace, "%s(), Multi-CP file:<%s>", __func__, fullFileName.c_str());
	}
	else
	{
		fullFileName = m_JTPMsg->getSubFileName();
		TRACE(trace, "%s(), Single-CP file:<%s>", __func__, fullFileName.c_str());
	}

	unsigned int ohiResult = TQChecker::instance()->removeSentFile(tQName, fullFileName);

	TRACE(trace, "Leaving %s(), resultCode:<%d>", __func__, ohiResult);
	return ohiResult;
}

/*============================================================================
	ROUTINE: sendFile
 ============================================================================ */
unsigned int FMS_CPF_JTPConnectionHndl::sendFile()
{
	ACS_TRA_trace* trace = &m_trace;
	TRACE(trace, "%s", "Entering sendFile()");
	std::string filePath;
	const char* fileName = m_JTPMsg->getFileName();
	const char* subFileName = m_JTPMsg->getSubFileName();

	unsigned int ohiResult = AES_OHI_EXECUTEERROR;

	getFilePath(filePath, fileName, m_CpName.c_str());

	if(!filePath.empty())
	{
		// Check system type
		if(m_IsMultiCP)
		{
			// MCP
			TRACE(trace, "%s, jtp order from CP:<%s>", __func__, m_CpName.c_str());

			FMS_CPF_FileTransferHndl fileTQHandler(fileName, filePath, m_CpName);

			// Attach to the File TQ, it must be defined with the same name of the CP file
			if( SUCCESS == fileTQHandler.setFileTQ(fileName) )
			{
				// move subfile to the TQ folder
				if( SUCCESS == fileTQHandler.moveFileToTransfer(subFileName) )
				{
					//Report the file to GOH
					if( fileTQHandler.sendCurrentFile() )
					{
						TRACE(trace, "%s, file reported", __func__);
						// Subfile reported to GOH
						// remove it from IMM
						removeSubFileFromImm(fileName, subFileName, m_CpName);
					}
					else
					{
						// OHI send fails
						TRACE(trace, "%s, failed to report the file towards OHI", __func__);
					}

					// set OHI result in both case
					ohiResult = fileTQHandler.getOhiError();
				}
				else
				{
					// Move file to TQ folder error
					TRACE(trace, "%s, move of subfile:<%s> failed, order coming from CP:<%s>", __func__, subFileName, m_CpName.c_str());
				}
			}
			else
			{
				// Attach to TQ failed
				TRACE(trace, "%s, attach to the File TQ:<%s> failed, order coming from CP:<%s>", __func__, fileName, m_CpName.c_str());
			}
		}
		else
                {
                        // SCP
                        TRACE(trace, "%s, jtp order from CP:<%s>", __func__, m_CpName.c_str());

                        FMS_CPF_FileTransferHndl fileTQHandler(fileName, filePath, m_CpName);

                        // Attach to the File TQ, it must be defined with the same name of the CP file
                        if( SUCCESS == fileTQHandler.setFileTQ(fileName) )
                        {
                                // move subfile to the TQ folder
                                if( SUCCESS == fileTQHandler.linkFileToTransfer(subFileName))
                                {
                                        //sleep(200);
                                        //Report the file to GOH
                                        if( fileTQHandler.sendCurrentFile(true) )
                                        {
                                                // Subfile reported to GOH
                                                TRACE(trace, "%s, file reported", __func__);
						TRACE(trace, "%s, file <%s> subfile <%s>", __func__, fileName, subFileName);
						TimeSortedMap* timeSortedMapPtr = NULL; 
						theMutex.acquire();
						CompositeFileMap::iterator myCompositeFileItr = theCompositeFileMap.find(fileName);
						if(myCompositeFileItr != theCompositeFileMap.end())
						{
							timeSortedMapPtr = myCompositeFileItr->second;
						}
						else
						{
							timeSortedMapPtr =  new TimeSortedMap();
							if(timeSortedMapPtr != NULL)
							{
								theCompositeFileMap[fileName] = timeSortedMapPtr;
							}
						}
						
						if(timeSortedMapPtr != NULL)
						{
							theTimeSortedMapPair.first = getAbsTime();
							theTimeSortedMapPair.second = subFileName;
							std::pair<TimeSortedMap::iterator, bool> result = timeSortedMapPtr->insert(theTimeSortedMapPair);
							if(result.second == false)
							{
								TRACE(trace, "%s(), inserting in map failed for file<%s>", __func__, subFileName);
							}
							TRACE(trace, "%s, inserting file into the Map and count <%d>", __func__, timeSortedMapPtr->size());
						}
						theMutex.release();
						writeToFile(theTimeSortedMapPair.first, fileName, subFileName);
                                        }
                                        else
                                        {
                                                // OHI send fails
                                                TRACE(trace, "%s, failed to report the file towards OHI", __func__);
                                        }

                                        // set OHI result in both case
                                        ohiResult = fileTQHandler.getOhiError();
                                }
                                else
                                {
                                        // Move file to TQ folder error
                                        TRACE(trace, "%s, move of subfile:<%s> failed, order coming from CP:<%s>", __func__, subFileName, m_CpName.c_str());
                                }
                        }
                        else
                        {
                                // Attach to TQ failed
                                TRACE(trace, "%s, attach to the File TQ:<%s> failed, order coming from CP:<%s>", __func__, fileName, m_CpName.c_str());
                        }
                }
	}
	else
	{
		// main file does not exist
		ohiResult = AES_OHI_FILENOTFOUND;
		char errorMsg[128] = {0};
		snprintf(errorMsg, 127, "FMS_CPF_JTPConnectionHndl::sendFile(), main file=<%s>, does not exist", fileName);
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(trace, "%s", errorMsg);
	}


	TRACE(trace, "Leaving %s, resultCode:<%d>", __func__, ohiResult);
	return ohiResult;
}

/*============================================================================
	ROUTINE: setFileArchived
 ============================================================================ */
unsigned int FMS_CPF_JTPConnectionHndl::setFileArchived()
{
	ACS_TRA_trace* trace = &m_trace;
	TRACE(trace, "Entering %s()", __func__);

	std::string fileSentName;
	if(m_IsMultiCP)
	{
		// MCP, file name e.g: CP1+RELCMDHDF-000020
		fileSentName = m_CpName;
		fileSentName.push_back(parseSymbol::plus);
		fileSentName.append(m_JTPMsg->getFileName());
		fileSentName.push_back(parseSymbol::minus);
		fileSentName.append(m_JTPMsg->getSubFileName());

		TRACE(trace, "%s(), Multi-CP file:<%s>", __func__, fileSentName.c_str());
	}
	else
	{
		// SCP
		fileSentName = m_JTPMsg->getSubFileName();
		TRACE(trace, "%s(), Single-CP file:<%s>", __func__, fileSentName.c_str());
	}

	AES_OHI_FileHandler gohFileHandler(OHI_USERSPACE::SUBSYS,
									   OHI_USERSPACE::APPNAME,
									   m_JTPMsg->getFileName() );

	unsigned int ohiResult = gohFileHandler.attach();
	if(AES_OHI_NOERRORCODE == ohiResult )
	{
		ohiResult = gohFileHandler.setTransferState(fileSentName.c_str(), m_JTPMsg->getDestinationName(), AES_OHI_FSARCHIVE);

		TRACE(trace, "%s(), file:<:%s> sent to destination:<%s>, ohi result:<%d>", __func__, fileSentName.c_str(), m_JTPMsg->getDestinationName(), ohiResult);

		gohFileHandler.detach();
	}

	TRACE(trace, "Leaving %s(), resultCode:<%d>", __func__, ohiResult);
	return ohiResult;
}

/*============================================================================
	ROUTINE: getSentFileStatus
 ============================================================================ */
unsigned int FMS_CPF_JTPConnectionHndl::getSentFileStatus()
{
	ACS_TRA_trace* trace = &m_trace;
	TRACE(trace, "%s", "Entering getSentFileStatus()");

	std::string fileSentName;
	if(m_IsMultiCP)
	{
		// MCP
		fileSentName = m_CpName ;
		fileSentName.push_back(parseSymbol::plus);
		fileSentName.append(m_JTPMsg->getFileName());
		fileSentName.push_back(parseSymbol::minus);
		fileSentName.append(m_JTPMsg->getSubFileName());
		TRACE(trace, "%s(), Multi-CP file:<%s>", __func__, fileSentName.c_str());
	}
	else
	{
		// SCP
		fileSentName = m_JTPMsg->getSubFileName();
		TRACE(trace, "%s(), Single-CP file:<%s>", __func__, fileSentName.c_str());
	}

	AES_OHI_FileHandler gohFileHandler(OHI_USERSPACE::SUBSYS,
									   OHI_USERSPACE::APPNAME,
									   m_JTPMsg->getFileName() );

	unsigned int ohiResult = gohFileHandler.attach();

	if(AES_OHI_NOERRORCODE == ohiResult )
	{
		int nameLength = 256; //HV37177
		char tmpFileName[nameLength];
		snprintf(tmpFileName, nameLength, fileSentName.c_str() );

		char tmpDestinationName[64]={0};
		snprintf(tmpDestinationName, 64, m_JTPMsg->getDestinationName() );

		FMS_CPF_JTPCpMsg::statusInfo sentFileStatus = FMS_CPF_JTPCpMsg::statusInfo();
		bool isDirectory;
		AES_OHI_Filestates gohStatus;

		// Reason why automatic transfer failed
		int reasonForFailure;
		// retrieve file status info by GOH
		ohiResult = gohFileHandler.getTransferStateEx(tmpFileName,
													  gohStatus,
													  tmpDestinationName,
													  m_JTPMsg->getFileOrder(),
													  sentFileStatus.dateAutoSendStart,
													  sentFileStatus.dateAutoSendEnd,
													  sentFileStatus.dateManSendStart,
													  sentFileStatus.dateManSendEnd,
													  sentFileStatus.dateAutoSendFail,
													  sentFileStatus.dateDumpedOnPrimary,
													  reasonForFailure,
													  isDirectory);

		TRACE(trace, "getSentFileStatus(), file:<%s> sent to destination:<%s>, ohi result:<%d>", tmpFileName, tmpDestinationName, ohiResult);
		//HV37177 Start
		if(m_IsMultiCP)
		{
			//MCP
			std::string receivedFileName = tmpFileName;
			size_t prefix = receivedFileName.find(fileSentName.c_str());
			if (std::string::npos != prefix)
			{
				// Received subfile name is from the requested BLADE
				std::string part="";
				size_t size  = receivedFileName.length();
				size_t start = 0;
				size_t pend = receivedFileName.find_first_of(parseSymbol::minus, start);
				if(size > 0)
				{
					start = pend + 1;
					pend = size;
					part = receivedFileName.substr(start, pend - start);
				}
				tmpFileName[0] = '\0';
				snprintf(tmpFileName, sizeof(part.c_str()), part.c_str() );
				TRACE(trace, "getSentFileStatus(), After truncating the prefix, subfile file name is :<%s>", tmpFileName);
			}
			else
			{
				// Received subfile name is not from the requested BLADE
				TRACE(trace, "%s", "getSentFileStatus() Received subfile name is not for the requested CP");
				ohiResult = AES_OHI_FILENOTFOUND; //Update OHI error code with AES_OHI_FILENOTFOUND
				tmpFileName[0]='\0'; //Empty the char array
				TRACE(trace, "getSentFileStatus(), Subfile name is set to empty :<%s>", tmpFileName);
			}

		}
		//HV37177 End
		m_JTPMsg->setSubFileName(tmpFileName);	//HU30480
		gohFileHandler.detach();

		// Set other file info
		sentFileStatus.reasonForFailure = static_cast<uint8_t>(reasonForFailure);

		sentFileStatus.subFileSize = getSubFileSize();

		// set retrieved info for Cp reply message
		m_JTPMsg->setFileStatusInfo(sentFileStatus);

		if(AES_OHI_FILENOTFOUND == ohiResult)
			ohiResult = AES_OHI_SUBFILENOTFOUND;

	}

	TRACE(trace, "Leaving getSentFileStatus(), resultCode:<%d>", ohiResult);
	return ohiResult;
}

/*============================================================================
	ROUTINE: getFilePath
 ============================================================================ */
void FMS_CPF_JTPConnectionHndl::getFilePath(std::string& filePath, const char* fileName, const char* cpName)
{
	ACS_TRA_trace* trace = &m_trace;
	TRACE(trace, "%s", "Entering getFilePath()");

	FMS_CPF_FileId fileid(fileName);
	FileReference reference;

	// Set the access mode required for the attribute get
	FMS_CPF_Types::accessType reserveMode = FMS_CPF_Types::NONE_;

	try
	{
		// Open logical file
		reference = DirectoryStructureMgr::instance()->open(fileid, reserveMode, cpName);

		// get main file path
		filePath = reference->getPath();

		// close the opened file
		DirectoryStructureMgr::instance()->closeExceptionLess(reference, cpName);
	}
	catch(const FMS_CPF_PrivateException& ex)
	{
		char errorMsg[512] = {0};
		snprintf(errorMsg, 511, "FMS_CPF_JTPConnectionHndl::getFilePath(), operation failed on file=<%s>, error=<%s>", fileName, ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(trace, "%s", errorMsg);
	}

	TRACE(trace, "Leaving getFilePath(), path:<%s>", filePath.c_str());
}

/*============================================================================
	ROUTINE: getSubFileSize
 ============================================================================ */
uint32_t FMS_CPF_JTPConnectionHndl::getSubFileSize() const
{
	ACS_TRA_trace* trace = &m_trace;
	TRACE(trace, "%s", "Entering getSubFileSize()");
	uint32_t subFileSize = 0;

	// On single CP the subfile is not moved outside the CP file system,
	// so, it is possible to retrieved the subfile size when it is not already deleted by GOH
	if(!m_IsMultiCP && !m_JTPMsg->isSubFileEmpty() )
	{
		// max file size=15 + '-' + max subfile size = 12
		const unsigned short maxLength = 30;
		char subFileName[maxLength] = {0};

		snprintf(subFileName, maxLength, "%s-%s", m_JTPMsg->getFileName(), m_JTPMsg->getSubFileName() );

		FMS_CPF_FileId fileId( subFileName );
		FileReference reference;

		// Set the access mode required for the attribute get
		FMS_CPF_Types::accessType reserveMode = FMS_CPF_Types::NONE_;

		try
		{
			// Open logical file
			reference = DirectoryStructureMgr::instance()->open(fileId, reserveMode, m_CpName.c_str());

			// get main file path
			subFileSize = reference->getSize();

			// close the opened file
			DirectoryStructureMgr::instance()->closeExceptionLess(reference, m_CpName.c_str());
		}
		catch(const FMS_CPF_PrivateException& ex)
		{
			char errorMsg[512] = {0};
			snprintf(errorMsg, 511, "FMS_CPF_JTPConnectionHndl::getSubFileSize(), operation failed on file=<%s>, error=<%s>",subFileName , ex.errorText() );
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
			TRACE(trace, "%s", errorMsg);
		}
	}

	TRACE(trace, "Leaving getSubFileSize(), size:<%d>", subFileSize);
	return subFileSize;
}

/*============================================================================
	ROUTINE: removeSubFileFromImm
 ============================================================================ */
void FMS_CPF_JTPConnectionHndl::removeSubFileFromImm(const char* fileName, const char* subFileName, std::string cpName)
{
	ACS_TRA_trace* trace = &m_trace;
	TRACE(trace, "Entering in %s", __func__);

	OmHandler objManager;
	std::string fileDN;

	// Retrieve the file DN and init OM handler
	if( DirectoryStructureMgr::instance()->getFileDN(fileName, cpName, fileDN)
		 && (objManager.Init() != ACS_CC_FAILURE ))
	{
		const unsigned int MAXBUFFER_LENGTH = 512U;
		char subFileDN[MAXBUFFER_LENGTH] = {0};

		// Assemble subfile DN
		snprintf(subFileDN, MAXBUFFER_LENGTH-1, "%s=%s,%s", cpf_imm::CompositeSubFileKey, subFileName, fileDN.c_str());

		TRACE(trace, "%s, delete subfile:<%s> object", __func__, subFileDN );
		// file found, remove it from IMM
		if(ACS_CC_SUCCESS != objManager.deleteObject(subFileDN) )
		{
			char errorMsg[MAXBUFFER_LENGTH] = {0};
			snprintf(errorMsg, MAXBUFFER_LENGTH-1, "%s, error=<%d> on subfile:<%s> delete", __func__, objManager.getInternalLastError(), subFileDN );
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
			TRACE(trace, "%s", errorMsg);
		}

		objManager.Finalize();
	}
	else
	{
		char errorMsg[512] = {0};
		snprintf(errorMsg, 511, "%s, file not found:<%s>", __func__, fileName);
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(trace, "%s", errorMsg);
	}

	TRACE(trace, "Leaving %s", __func__);
}

/*============================================================================
	ROUTINE: getAbsTime
 ============================================================================ */
unsigned long FMS_CPF_JTPConnectionHndl::getAbsTime ()
{
  time_t ltime;
  ACE_OS::time(&ltime);
  return ltime;
}

/*============================================================================
	ROUTINE: deleteFiles
 ============================================================================ */
void FMS_CPF_JTPConnectionHndl::deleteFiles()
{
	ACS_TRA_trace* trace = &m_trace;
	TRACE(trace, "Entering in %s", __func__);
	TimeSortedMap* myTimeSortedMapPtr;
	TimeSortedMap::iterator myTimeSortedItr;
	std::string filePath;
	theMutex.acquire();
	CompositeFileMap::iterator myCompositeFileItr = theCompositeFileMap.begin();
	while((!m_isShutdownSignaled) && (myCompositeFileItr != theCompositeFileMap.end()))
	{
		int removeDelay = getDeleteFileTimer(myCompositeFileItr->first);
                TRACE(trace, "CompositeFile:<%s>", myCompositeFileItr->first.c_str());
                TRACE(trace, "deleteFileTimer:<%d>", removeDelay);
		if (removeDelay == -1)
		{
			myCompositeFileItr++;
			continue;
		}
		
		myTimeSortedMapPtr = myCompositeFileItr->second;
		getFilePath(filePath, myCompositeFileItr->first.c_str());
		
		if((!filePath.empty()) && (myTimeSortedMapPtr != NULL))
		{
			myTimeSortedItr = myTimeSortedMapPtr->begin();
			while((!m_isShutdownSignaled) && (myTimeSortedItr != myTimeSortedMapPtr->end()))
			{
				bool result = readyForDelete(myTimeSortedItr->first, removeDelay);
				if (result)
				{
					removeSubFileFromImm(myCompositeFileItr->first.c_str(), myTimeSortedItr->second.c_str());
					std::string sourceFile(filePath);
					sourceFile.push_back(DirDelim);
					sourceFile += myTimeSortedItr->second;
					boost::system::error_code delResult;
					boost::filesystem::remove(sourceFile, delResult);
					// Check remove result
				        if(SUCCESS !=  delResult.value())
        				{
                				// remove error log it
                				char errorLog[256] = {0};
                				ACE_OS::snprintf(errorLog, 255, "%s(), error:<%d> on remove sourcefile<%s>", __func__, delResult.value(),sourceFile.c_str());
                				CPF_Log.Write(errorLog, LOG_LEVEL_ERROR);
        				}

					removeFromFile(myCompositeFileItr->first, myTimeSortedItr->second);
					myTimeSortedMapPtr->erase(myTimeSortedItr);
				}
				myTimeSortedItr++;
			}
		}
		myCompositeFileItr++;
	}
	theMutex.release();
	TRACE(trace, "Leaving %s", __func__);	
}

/*============================================================================
	ROUTINE: readyForDelete
 ============================================================================ */
bool FMS_CPF_JTPConnectionHndl::readyForDelete(ACE_UINT64  absoluteDeletionTime, ACE_INT32 removeDelay)
{
	ACS_TRA_trace* trace = &m_trace;
	TRACE(trace, "Entering in %s", __func__);
	ACE_UINT64 deleteSeconds(0);
	ACE_UINT64 todaySeconds(0);
	ACE_INT64 calcTime(0);
	bool deleteFile(false);
	
	todaySeconds = getAbsTime();
	deleteSeconds = absoluteDeletionTime;
	deleteSeconds = (removeDelay * 60) + deleteSeconds;
	calcTime = (deleteSeconds - todaySeconds);
	if (calcTime <= 0)
		deleteFile = true;

        TRACE(trace, "calcTime:<%d>",calcTime);
        TRACE(trace, "deleteSeconds:<%d>",deleteSeconds);
        TRACE(trace, "todaySeconds:<%d>",todaySeconds);
	TRACE(trace, "Leaving %s", __func__);
	return deleteFile;
}

/*============================================================================
	ROUTINE: getDeleteFileTimer
 ============================================================================ */
ACE_INT32 FMS_CPF_JTPConnectionHndl::getDeleteFileTimer(const std::string& compositeFile)
{
	ACE_INT32 removeDelay = -1;
	DeleteFileTimerMap::iterator myItr = theDeleteFileTimerMap.find(compositeFile);
	if(myItr != theDeleteFileTimerMap.end())
	{
		removeDelay = myItr->second;
	}
	return removeDelay;
}

/*============================================================================
	ROUTINE: updateDeleteFileTimer
 ============================================================================ */
void FMS_CPF_JTPConnectionHndl::updateDeleteFileTimer(const std::string& compositeFile, ACE_INT32 deleteFileTimer)
{
	ACS_TRA_trace* trace = &m_trace;
	TRACE(trace, "Entering in %s", __func__);
	TRACE(trace, "%s(),compositeFile<%s>, deleteFileTimer<%d>",__func__,compositeFile.c_str(), deleteFileTimer);
	DeleteFileTimerMap::iterator myItr = theDeleteFileTimerMap.find(compositeFile);
	if(myItr != theDeleteFileTimerMap.end())
	{
		myItr->second = deleteFileTimer;
	}
	else
	{
		theDeleteFileTimerMap[compositeFile] = deleteFileTimer;
	}
	TRACE(trace, "Leaving %s", __func__);
}

/*============================================================================
        ROUTINE: removeDeleteFileTimer
 ============================================================================ */
void FMS_CPF_JTPConnectionHndl::removeDeleteFileTimer(const std::string& compositeFile)
{
	theMutex.acquire();
	DeleteFileTimerMap::iterator myItr = theDeleteFileTimerMap.find(compositeFile);
	if(myItr != theDeleteFileTimerMap.end())
        {
                theDeleteFileTimerMap.erase(myItr);
        }
	theMutex.release();
}

/*============================================================================
        ROUTINE: removeCompositeFile 
 ============================================================================ */
void FMS_CPF_JTPConnectionHndl::removeCompositeFile(const std::string& compositeFile)
{
	theMutex.acquire();
	CompositeFileMap::iterator myCompositeFileItr = theCompositeFileMap.find(compositeFile);
	if(myCompositeFileItr != theCompositeFileMap.end())
	{
		TimeSortedMap* timeSortedMapPtr = myCompositeFileItr->second;
		if(timeSortedMapPtr != NULL)
		{
			timeSortedMapPtr->clear();
			delete timeSortedMapPtr;
		}
		theCompositeFileMap.erase(myCompositeFileItr);
	}
	theMutex.release();
}

/*============================================================================
        ROUTINE: shutDown
 ============================================================================ */
void FMS_CPF_JTPConnectionHndl::shutDown()
{
	m_isShutdownSignaled = true;
}

/*============================================================================
        ROUTINE: initFromFile
 ============================================================================ */
void FMS_CPF_JTPConnectionHndl::initFromFile(const std::string& compositeFile)
{
	ACS_TRA_trace* trace = &m_trace;
	TRACE(trace, "Entering in %s", __func__);
	std::string compFile = ParameterHndl::instance()->getDataDiskRoot();
	compFile.push_back(DirDelim);
	compFile.append(cpf_imm::CompositeFileObjectName);
	ACE_stat fstat;

	if(!(ACE_OS::stat(compFile.c_str(), &fstat) == 0))  // check for composite directory incase of upgrade
	{
		// composite file creation
        	// Set permission 755 (RWXR-XR-X) on folder
        	mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
        	ACE_OS::umask(0);
	
		if((ACE_OS::mkdir(compFile.c_str(), mode) == FAILURE) && (errno != EEXIST))
		{
        		char errorText[256] = {0};
        		std::string errorDetail(strerror_r(errno, errorText, 255));
        		char errorMsg[512] = {0};
        		snprintf(errorMsg, 511, "initFromFile(), compositeFile dir creation to the path:<%s> failed, error=<%s>", compositeFile.c_str(), errorDetail.c_str());
        		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		}
	}

        compFile.push_back(DirDelim);
        compFile.append(compositeFile);
	std::string file;
        TRACE(trace, "%s(),compFile = <%s>",__func__,compFile.c_str());
	if(ACE_OS::stat(compFile.c_str(), &fstat) == 0) // check for compositeFile directory
	{ 
		if(fstat.st_mode & S_IFDIR) // check if it is dir
		{
			ACE_Dirent Dir(compFile.c_str());
			ACE_DIRENT* dirEntry;
			TimeSortedMap* timeSortedMapPtr = NULL;
			while((dirEntry = Dir.read()))
			{
				if (!strcmp(dirEntry->d_name,".") ||!strcmp(dirEntry->d_name,".."))
					continue;
				file.clear();
				file = compFile + string(ACE_DIRECTORY_SEPARATOR_STR_A) + dirEntry->d_name;
				string textline = "";
				TRACE(trace,"compositeFile dir file = %s", file.c_str());
				ifstream infile(file.c_str());
				if(infile)
				{
					ACE_INT64 creationDate = -1;
					std::string subFileName = "";
					while(getline(infile,textline))
					{
						if(textline.find(CREATION_TIME) != string::npos)
						{
							size_t create_index = textline.find("=");
							if(create_index != string::npos)
							{
								stringstream tmp; 
								tmp << textline.substr(create_index+1);
								tmp >> creationDate;
							}	
						}
						else if(textline.find(FILE_NAME) != string::npos)
						{
							size_t file_index = textline.find("=");
							if(file_index != string::npos)
							{
								subFileName = textline.substr(file_index+1);
							}
						}
					}
					
					if((creationDate != -1) && (!subFileName.empty()))
					{
						if(timeSortedMapPtr == NULL)
							timeSortedMapPtr =  new TimeSortedMap();
					
						if(timeSortedMapPtr != NULL)
						{
							TRACE(trace, "%s(),file creationDate <%ld> and file <%s>",__func__, creationDate, subFileName.c_str());
							theTimeSortedMapPair.first = creationDate;
							theTimeSortedMapPair.second = subFileName;
							std::pair<TimeSortedMap::iterator, bool> result = timeSortedMapPtr->insert(theTimeSortedMapPair);
							if(result.second == false)
							{
								TRACE(trace, "%s(), inserting in map failed for file<%s>", __func__, subFileName.c_str());
							}
						}
					
						if(timeSortedMapPtr != NULL)
							theCompositeFileMap[compositeFile] = timeSortedMapPtr;
					}
				}	
			}
		}
		else
		{
			TRACE(trace, "%s(),  compositeFile is not a Directory %s",__func__, compFile.c_str() );
		}
	}
	else
	{
		TRACE(trace, "%s(),  compositeFile Dir %s not found", __func__, compFile.c_str());
	
		// Set permission 755 (RWXR-XR-X) on folder
                mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
                ACE_OS::umask(0);

		if((ACE_OS::mkdir(compFile.c_str(), mode) == FAILURE) && (errno != EEXIST))
		{
        		char errorText[256] = {0};
        		std::string errorDetail(strerror_r(errno, errorText, 255));
        		char errorMsg[512] = {0};
        		snprintf(errorMsg, 511, "initFromFile(), compositeFile dir creation to the path:<%s> failed, error=<%s>", compositeFile.c_str(), errorDetail.c_str());
        		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		}
	}
}

/*============================================================================
        ROUTINE: writeToFile
 ============================================================================ */
void FMS_CPF_JTPConnectionHndl::writeToFile(const unsigned long& creationDate, const std::string& compositeFile, const std::string& fileName)
{
	ACS_TRA_trace* trace = &m_trace;
	std::string subFile = ParameterHndl::instance()->getDataDiskRoot();
	subFile.push_back(DirDelim);
	subFile.append(cpf_imm::CompositeFileObjectName);
        subFile.push_back(DirDelim);
        subFile.append(compositeFile);
	ACE_stat fstat;
	if(ACE_OS::stat(subFile.c_str(), &fstat) == 0) // check for compositeFile directory
	{
		if(fstat.st_mode & S_IFDIR) // check if it is dir 
		{
			ACE_HANDLE fHandle = ACE_INVALID_HANDLE;
			subFile.push_back(DirDelim);
			subFile.append(fileName);
			//Create the file.
			fHandle = ACE_OS::open(subFile.c_str(), O_WRONLY|O_CREAT);
			if(fHandle == ACE_INVALID_HANDLE)
			{
                                char errorText[256] = {0};
                                std::string errorDetail(strerror_r(errno, errorText, 255));
                                char errorMsg[512] = {0};
                                snprintf(errorMsg, 511, "Create(), subfile creation to the path:<%s> failed, error=<%s>", subFile.c_str(), errorDetail.c_str() );
                                CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
                                EventReport::instance()->reportException(errorDetail, FMS_CPF_PrivateException::PHYSICALERROR);
			}
			else
			{
				stringstream tmp; 
				tmp << creationDate;
				std::string toFile;
				toFile = CREATION_TIME;
				toFile.append("=");
				toFile.append(tmp.str());
				toFile.append("\n");
				toFile.append(FILE_NAME);
				toFile.append("=");
				toFile.append(fileName);
				toFile.append("\n");
				
				const size_t fileSize = toFile.size()+1;
				char buf[fileSize];
				memset(buf, 0, fileSize);
				memcpy(buf,toFile.c_str(), toFile.size());

				if(ACE_OS::write(fHandle, buf , fileSize) < 0)
				{
                                        char errorLog[512]={'\0'};
                                        char errorText[256]={'\0'};
                                        std::string errorDetail(strerror_r(errno, errorText, 255));
                                        ACE_OS::snprintf(errorLog, 511, "%s(), write to file failed, error=<%s>", __func__, errorDetail.c_str());
                                        CPF_Log.Write(errorLog, LOG_LEVEL_WARN);
                                        TRACE(trace, "%s", errorLog);
				}
				
				ACE_OS::close(fHandle);
			}
		}
		else
		{
			TRACE(trace, "%s(),  compositeFile is not a Directory %s ", __func__, subFile.c_str() );
		}
	}
	else
	{
		TRACE(trace, "%s(),  compositeFile Dir %s not found", __func__, subFile.c_str() );
	}
}

/*============================================================================
        ROUTINE: removeFromFile 
 ============================================================================ */
void FMS_CPF_JTPConnectionHndl::removeFromFile(const std::string& compositeFile,const std::string& fileName)
{
	ACS_TRA_trace* trace = &m_trace;
	std::string subFile = ParameterHndl::instance()->getDataDiskRoot();
	subFile.push_back(DirDelim);
	subFile.append(cpf_imm::CompositeFileObjectName);
	subFile.push_back(DirDelim);
	subFile.append(compositeFile);
	ACE_stat fstat;
	if(ACE_OS::stat(subFile.c_str(), &fstat) == 0) // check for compositeFile directory
	{
		if(fstat.st_mode & S_IFDIR) // check if it is dir
		{
			subFile.push_back(DirDelim);
			subFile.append(fileName);
			if(ACE_OS::stat(subFile.c_str(),&fstat) == 0)
			{
				if(ACE_OS::unlink(subFile.c_str()) != 0)
				{
					TRACE(trace,"%s(),Not able to delete the file %s",__func__ ,subFile.c_str());
				}
			}
		}
	}
	else
	{
		TRACE(trace, "%s(),  compositeFile Dir %s not found", __func__, subFile.c_str());
	}
}

/*============================================================================
        ROUTINE: cleanUp 
 ============================================================================ */
void FMS_CPF_JTPConnectionHndl::cleanUp()
{
	TimeSortedMap* myTimeSortedMapPtr;
        CompositeFileMap::iterator myCompositeFileItr = theCompositeFileMap.begin();
        while(myCompositeFileItr != theCompositeFileMap.end())
        {
        	myTimeSortedMapPtr = myCompositeFileItr->second;
                if(myTimeSortedMapPtr != NULL)
                {
                	myTimeSortedMapPtr->clear();
                	delete(myTimeSortedMapPtr);
                }
                theCompositeFileMap.erase(myCompositeFileItr++);
        }
        theDeleteFileTimerMap.clear();
}
