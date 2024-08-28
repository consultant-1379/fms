/*
 * * @file fms_cpf_jtpconnectionsmgr.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_JTPConnectionsMgr.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_jtpconnectionsmgr.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-08-02
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
 *	| 1.0.0  | 2012-08-02 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_jtpconnectionsmgr.h"
#include "fms_cpf_jtpconnectionhndl.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_filereference.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_common.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"

#include "acs_apgcc_omhandler.h"

#include "ace/OS_NS_poll.h"
#include <sys/eventfd.h>
#include <sys/inotify.h>

extern ACS_TRA_Logging CPF_Log;

namespace CPF_JTP {
		char serviceName[] = "FMS_AFP";
		char relCmdHdh [] = "RELCMDHDF";
}

namespace Inotify_Size{
	const size_t EVENT_SIZE = sizeof( struct inotify_event );
	const size_t EVENT_BUF_LEN = ( 64U * ( EVENT_SIZE  ) );
}

/*============================================================================
	ROUTINE: FMS_CPF_JTP_ConnectionsMgr
 ============================================================================ */
FMS_CPF_JTPConnectionsMgr::FMS_CPF_JTPConnectionsMgr()
: m_IsMultiCP(false),
  m_JTPServer(NULL),
  m_InotifyFD(INVALID),
  m_watcherID(INVALID),
  m_deleteThreadHandle(0), 
  m_isShutdownSignaled(false)
{
	// create the file descriptor to signal stop
	m_StopEvent = eventfd(0,0);
	m_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_JTP_ConnectionsMgr");
}

/*============================================================================
	ROUTINE: ~FMS_CPF_JTP_ConnectionsMgr
 ============================================================================ */
FMS_CPF_JTPConnectionsMgr::~FMS_CPF_JTPConnectionsMgr()
{
	ACE_OS::close(m_StopEvent);

	if(NULL != m_JTPServer)
		delete m_JTPServer;

	if(NULL != m_trace)
		delete m_trace;
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
int FMS_CPF_JTPConnectionsMgr::open(void *args)
{
	TRACE(m_trace, "%s", "Entering open()");
	// To avoid warning about unused parameter
	UNUSED(args);
	int result = FAILURE;

	// Initialize inotify object
	m_InotifyFD = inotify_init();

	if( INVALID == m_InotifyFD )
	{
		char errorText[256] = {0};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		char errorMsg[512] = {0};
		snprintf(errorMsg, 511, "FMS_CPF_JTP_ConnectionsMgr::open()(), inotify_init() failed, error=<%s>", errorDetail.c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", errorMsg);
	}
	else
	{
		// start event loop by svc thread
		// wait on it is required to deallocate all resources
		result = activate();

		// Check if the svc thread is started
		if(SUCCESS != result)
		{
			// Close the INOTIFY instance
			ACE_OS::close( m_InotifyFD );

			CPF_Log.Write("FMS_CPF_JTP_ConnectionsMgr::open(),  error on start svc thread", LOG_LEVEL_ERROR);
			TRACE(m_trace, "%s", "open(), error on start svc thread");
		}
		else if(!m_IsMultiCP) 
		{
			if(!startDeleteFileThread()) 
			{
				result = FAILURE;	
				CPF_Log.Write("FMS_CPF_JTP_ConnectionsMgr::open(),  error on startDeleteFileThread thread", LOG_LEVEL_ERROR);
				TRACE(m_trace, "%s", "open(), error on start svc thread");
			}	
		}
	}
	TRACE(m_trace, "%s", "Leaving open()");
	return result;
}

/*============================================================================
	ROUTINE: svc
 ============================================================================ */
int FMS_CPF_JTPConnectionsMgr::svc()
{
	TRACE(m_trace, "%s", "Entering svc()");

	int result = SUCCESS;

	if(publishJTPServer())
	{
		int pollResult;
		int jtpHandleCount = 0;
		JTP_HANDLE* jtpHandles = NULL;

		TRACE(m_trace, "%s", "svc(), get number of jtp handles");

		// To get the number of handles
		m_JTPServer->getHandles(jtpHandleCount, jtpHandles);

		TRACE(m_trace, "svc(), retrieved <%i> jtp handles", jtpHandleCount);

		// Now jtpHandleCount has the correct number of handles to retrieve
		jtpHandles = new JTP_HANDLE[jtpHandleCount];

		// get jtp handles
		m_JTPServer->getHandles(jtpHandleCount, jtpHandles);

		TRACE(m_trace, "%s", "svc(), retrieved jtp handles");

		// Create a fd to wait for request from JTP client
		const nfds_t nfds = jtpHandleCount + 2;
		struct pollfd fds[nfds];

		// Initialize the pollfd structure
		ACE_OS::memset(fds, 0, sizeof(fds));

		// Set JTP handles
		for(int handleIdx = 0; handleIdx < jtpHandleCount; ++handleIdx)
		{
			fds[handleIdx].fd = jtpHandles[handleIdx];
			fds[handleIdx].events = POLLIN;
		}

		// Set folder watcher
		fds[jtpHandleCount].fd = m_InotifyFD;
		fds[jtpHandleCount].events = POLLIN;

		// Set shutdown handle
		fds[jtpHandleCount+1].fd = m_StopEvent;
		fds[jtpHandleCount+1].events = POLLIN;

		// check if reactive a watcher
		addWatcherOnLogFile();

		// timeout to start cleanup procedure
		ACE_Time_Value timeout;

		__time_t secs = 300;
		__suseconds_t usecs = 0;
		timeout.set(secs, usecs);

		// waiting for JTP requests or stop
		while(true)
		{
			TRACE(m_trace, "%s", "svc(), start waiting for JTP client connection");

			pollResult = ACE_OS::poll(fds, nfds, &timeout);

			if( FAILURE == pollResult )
			{
				char msgBuffer[256] = {0};
				snprintf(msgBuffer, 255, "FMS_CPF_JTP_ConnectionsMgr::svc(), error:<%s> on poll", strerror(errno) );
				CPF_Log.Write(msgBuffer, LOG_LEVEL_ERROR);
				TRACE(m_trace, "%s", msgBuffer);
				continue;
			}

			// Time out happened
			if( 0 == pollResult )
			{
				TRACE(m_trace, "%s", "svc(), time-out expired");
				// deallocate closed connection handlers
				cleanCloseSession();
				continue;
			}

			// checks for jtp handle signal
			for(int handleIdx = 0; handleIdx < jtpHandleCount; ++handleIdx)
			{
				if(fds[handleIdx].revents & POLLIN)
				{
					TRACE(m_trace, "%s","svc(), jtp connection manager receive client request");
					// Handling client connection
					clientConnectionHandling();

					addWatcherOnLogFile();
					// exit from the "for" loop on handles
					break;
				}
			}

			// check for shutdown request
			if(fds[jtpHandleCount].revents & POLLIN)
			{
				TRACE(m_trace, "%s", "svc(), received folder change notification");
				handlingFolderChange();
				continue;
			}

			// check for shutdown request
			if(fds[jtpHandleCount + 1].revents & POLLIN)
			{
				TRACE(m_trace, "%s", "svc(), jtp connection manager received shutdown request");
				// exit from "while" loop
				break;
			}
		}

		// deallocate handles memory
		delete[] jtpHandles;
	}

	// wait and deallocate all connection handlers
	cleanCloseSession(true);

	// remove folder watcher if defined
	if(INVALID != m_watcherID)
		inotify_rm_watch( m_InotifyFD, m_watcherID );

	// Close the INOTIFY instance
	ACE_OS::close( m_InotifyFD );

	TRACE(m_trace, "%s", "Leaving open()");
	return result;
}

/*============================================================================
	ROUTINE: stopJTPConnectionsMgr
 ============================================================================ */
void FMS_CPF_JTPConnectionsMgr::stopJTPConnectionsMgr()
{
	TRACE(m_trace, "%s", "Entering stopJTPConnectionsMgr()");

	m_isShutdownSignaled = true; 
	ACE_UINT64 stopEvent=1;
	ssize_t numByte;

	// Signal to internal thread to stop
	numByte = ::write(m_StopEvent, &stopEvent, sizeof(ACE_UINT64));

	if(sizeof(ACE_UINT64) != numByte)
	{
		CPF_Log.Write("stopJTPConnectionsMgr(), error on stop internal thread", LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", "stopJTPConnectionsMgr(), error on stop internal thread");
	}
	else
	{
		TRACE(m_trace, "%s", "stopJTPConnectionsMgr(), waiting on svc end");
		// wait svc termination
		wait();
	}

	TRACE(m_trace, "%s", "Leaving stopJTPConnectionsMgr()");
}

/*============================================================================
	ROUTINE: publishJTPServer
 ============================================================================ */
bool FMS_CPF_JTPConnectionsMgr::publishJTPServer()
{
	TRACE(m_trace, "%s", "Entering in publishJTPServer()");

	bool published = false;

	// try until OK or shutdown happens
	do{

		//create JTP server object
		if(NULL == m_JTPServer)
			m_JTPServer = new (std::nothrow) ACS_JTP_Service(CPF_JTP::serviceName);

		// publish the server
		if(NULL != m_JTPServer)
		{
			TRACE(m_trace, "%s", "publishJTPServer(), call jtp publish");
			published = m_JTPServer->jidrepreq();
		}

		// check publish result
		if(!published)
		{
			// Some error occurs wait 1 sec and retry, exit on shutdown request
			struct pollfd fds[1];
			nfds_t nfd = 1;

			// Initialize the pollfd structure
			ACE_OS::memset(fds, 0, sizeof(fds));

			ACE_Time_Value timeout;

			__time_t secs = 1;
			__suseconds_t usecs = 0;
			timeout.set(secs, usecs);

			fds[0].fd = m_StopEvent;
			fds[0].events = POLLIN;

			int pollResult = ACE_OS::poll(fds, nfd, &timeout);

			// Error on poll
			if( FAILURE == pollResult )
			{
				char msgBuff[256] = {0};
				::snprintf(msgBuff, 255, "publishJTPServer(), error=<%s> on poll", ::strerror(errno) );
				CPF_Log.Write(msgBuff,	LOG_LEVEL_ERROR);
				TRACE(m_trace, "%s", msgBuff);
				continue;
			}

			// Time out happened
			if( 0 == pollResult )
			{
				TRACE(m_trace, "%s", "publishJTPServer(), time-out expired");
				continue;
			}

			// Received shutdown signal
			if(fds[0].revents & POLLIN)
			{
				TRACE(m_trace, "%s", "publishJTPServer(), received shutdown signal");
				break;
			}
		}

	}while(!published);

	TRACE(m_trace, "%s", "Leaving publishJTPServer()");
	return published;
}

/*============================================================================
	ROUTINE: clientConnectionHandling
 ============================================================================ */
void FMS_CPF_JTPConnectionsMgr::clientConnectionHandling()
{
	TRACE(m_trace, "%s", "Entering in clientConnectionHandling()");

	// Allocate jtp conversation object
	ACS_JTP_Conversation* newConversation = new (std::nothrow) ACS_JTP_Conversation();

	if(NULL != newConversation)
	{
		// Accept new connection request
		if( m_JTPServer->accept(NULL, newConversation) )
		{
			TRACE(m_trace, "%s", "clientConnectionHandling(), new JTP connection accepted");

			//Check connection state
			if( ACS_JTP_Conversation::StateConnected == newConversation->State())
			{
				TRACE(m_trace, "%s", "clientConnectionHandling(), connection state is connected");

				// Allocate the connection handler object
				FMS_CPF_JTPConnectionHndl* newSession = new (std::nothrow) FMS_CPF_JTPConnectionHndl( newConversation, m_IsMultiCP);

				if(NULL != newSession)
				{
					// Start the connection handler thread
					if( SUCCESS == newSession->open() )
					{
						TRACE(m_trace, "%s", "clientConnectionHandling(), new connection handler started");
						m_ConnectionList.push_back(newSession);
					}
					else
					{
						CPF_Log.Write("clientConnectionHandling(), error on start FMS_CPF_JTPConnectionHndl thread", LOG_LEVEL_ERROR);
						TRACE(m_trace, "%s", "clientConnectionHandling(), error on start FMS_CPF_JTPConnectionHndl thread");

						// deallocate session object
						// it also will deallocated the jtp conversation object
						delete newSession;
					}
				}
				else
				{
					CPF_Log.Write("clientConnectionHandling(), error on FMS_CPF_JTPConnectionHndl object allocation", LOG_LEVEL_ERROR);
					TRACE(m_trace, "%s", "clientConnectionHandling(), error on FMS_CPF_JTPConnectionHndl object allocation");

					//Deallocate jtp conversation object
					delete newConversation;
			    }
			}
			else
			{
				CPF_Log.Write("clientConnectionHandling(), error on ACS_JTP_Conversation state is not connected", LOG_LEVEL_ERROR);
				TRACE(m_trace, "%s", "clientConnectionHandling(), error on ACS_JTP_Conversation state is not connected");

				//Deallocate jtp conversation object
				delete newConversation;
			}
		}
		else
		{
			CPF_Log.Write("clientConnectionHandling(), error on ACS_JTP_Server accept", LOG_LEVEL_ERROR);
			TRACE(m_trace, "%s", "clientConnectionHandling(), error on ACS_JTP_Server accept");

			//Deallocate jtp conversation object
			delete newConversation;
		}
	}
	else
	{
		CPF_Log.Write("clientConnectionHandling(), error on ACS_JTP_Conversation object allocation", LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", "clientConnectionHandling(), error on ACS_JTP_Conversation object allocation");
	}

	TRACE(m_trace, "%s", "Leaving clientConnectionHandling()");
}

/*============================================================================
	ROUTINE: clientConnectionHandling
 ============================================================================ */
void FMS_CPF_JTPConnectionsMgr::cleanCloseSession(bool shutDown)
{
	TRACE(m_trace, "%s", "Entering in cleanCloseSession()");

	// only for trace scope
	short int activeConnection = 0;
	short int closeConnection = 0;

	std::list<FMS_CPF_JTPConnectionHndl*>::iterator closeSession;

	closeSession = m_ConnectionList.begin();

	// scan connection handler list
	while(closeSession != m_ConnectionList.end())
	{
		// Check connection state if not shutDown
		if( shutDown || !(*closeSession)->isConnectionStateOn() )
		{
			// Connection closed or shutdown, deallocate the handler
			// wait handler svc end, Needed to allow deallocating of all SO resources
			(*closeSession)->wait();

			delete (*closeSession);

			// erase return the iterator to next element
			closeSession = m_ConnectionList.erase(closeSession);

			// only for trace scope
			++closeConnection;
		}
		else
		{
			// Connection active ship it
		    ++closeSession;

		    ++activeConnection;
		}
	}

	if(shutDown && (!m_IsMultiCP)) 
        {
                FMS_CPF_JTPConnectionHndl::shutDown();

                m_deleteFilesEv.signal();

                if(0 != m_deleteThreadHandle)
                {
                        ACE_Thread::join(m_deleteThreadHandle);
                }

		FMS_CPF_JTPConnectionHndl::cleanUp();
        }

	TRACE(m_trace, "Leaving cleanCloseSession(), connections: active:<%d>, close:<%d>", activeConnection, closeConnection);
}

/*============================================================================
	ROUTINE: handlingFolderChange
 ============================================================================ */
void FMS_CPF_JTPConnectionsMgr::handlingFolderChange()
{
	TRACE(m_trace, "%s", "Entering in handlingFolderChange()");

	char buffer[Inotify_Size::EVENT_BUF_LEN] = {0};
	std::string origSubFileName;//HU59831
	std::string renameSubFileName; //HU59831

	// read the list of change events happens
	ssize_t length = read( m_InotifyFD, buffer, Inotify_Size::EVENT_BUF_LEN );

	if( length > 0)
	{
		TRACE(m_trace, "handlingFolderChange(), received <%d> bytes", length);

		ssize_t eventIdx = 0;

		while( eventIdx < length )
		{
			// read the change event one by one and process it accordingly
			struct inotify_event* event = (struct inotify_event*)( &buffer[eventIdx]);

			if( event->len && (event->mask & IN_DELETE) && !(event->mask & IN_ISDIR) )
			{
				TRACE(m_trace, "%s", "handlingFolderChange(), subfile removed");

				// File deleted from folder
				std::string subFileName(event->name);

				// Search in the map for the rename
				std::map<std::string, std::string>::iterator mapItr = m_renameMap.find(subFileName);
				if (m_renameMap.end() != mapItr)
				{
					// The original name has been changed by AFP because the renaming is active on the transfer queue
					TRACE(m_trace, "handlingFolderChange(), deleting the IMM entry file = %s", mapItr->second.c_str());
					subFileName.assign(mapItr->second);
					m_renameMap.erase(mapItr);
				}

				removeSubFileFromImm(subFileName);
			}
			else if( event->mask & IN_DELETE_SELF )
			{
				TRACE(m_trace, "%s", "handlingFolderChange(), watched folder removed");
				m_watcherID = INVALID;
			}
			else if ( event->mask & IN_MOVED_FROM)
			{
				origSubFileName.assign(event->name);
			}
			else if ( event->mask & IN_MOVED_TO)
			{
				renameSubFileName.assign(event->name);
			}

			// When both are not empty we can fill the map
			if(!origSubFileName.empty() && !renameSubFileName.empty())
			{
				m_renameMap.insert(std::pair<std::string, std::string> (renameSubFileName, origSubFileName));
				origSubFileName.clear();
				renameSubFileName.clear();
			}

			eventIdx += Inotify_Size::EVENT_SIZE + event->len;
		}
	}
	else
	{
		// handling read error
		char errorText[256] = {0};
		std::string errorDetail(strerror_r(errno, errorText, 255));
		char errorMsg[512] = {0};
		snprintf(errorMsg, 511, "FMS_CPF_JTP_ConnectionsMgr::handlingFolderChange()(), read events failed, error=<%s>", errorDetail.c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", errorMsg);
	}

	TRACE(m_trace, "%s", "Leaving handlingFolderChange()");
}


/*============================================================================
	ROUTINE: removeSubFileFromImm
 ============================================================================ */
void FMS_CPF_JTPConnectionsMgr::removeSubFileFromImm(const std::string& subFileName)
{
	TRACE(m_trace, "Entering in removeSubFileFromImm(<%s>)", subFileName.c_str());

	std::string fileName(CPF_JTP::relCmdHdh);
	std::string cpName;
	std::string fileDN;
	OmHandler objManager;

	// Retrieve the file DN and init OM handler
	if( DirectoryStructureMgr::instance()->getFileDN(fileName, cpName, fileDN)
		 && (objManager.Init() != ACS_CC_FAILURE ))
	{
		std::vector<std::string> subFileDNList;

		// Get all subfile of this file
		ACS_CC_ReturnType getResult = objManager.getChildren(fileDN.c_str(), ACS_APGCC_SUBLEVEL, &subFileDNList);

		if(ACS_CC_SUCCESS == getResult)
		{
			std::vector<std::string>::const_iterator subFileDN;
			// check each subfile
			for(subFileDN = subFileDNList.begin(); subFileDN != subFileDNList.end(); ++subFileDN)
			{
				// Get the filename
				std::string name;
				getLastFieldValue((*subFileDN), name);

				// check the file name
				if( subFileName.compare(name) == 0)
				{
					TRACE(m_trace, "removeSubFileFromImm(), delete subfile:<%s> object", (*subFileDN).c_str());
					// file found, remove it from IMM
					if(ACS_CC_SUCCESS != objManager.deleteObject((*subFileDN).c_str()) )
					{
						char errorMsg[512] = {0};
						snprintf(errorMsg, 511, "FMS_CPF_JTP_ConnectionsMgr::removeSubFileFromImm(), error=<%d> on subfile:<%s> delete", objManager.getInternalLastError(), (*subFileDN).c_str() );
						CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
						TRACE(m_trace, "%s", errorMsg);
					}
					break;
				}
			}
		}

		objManager.Finalize();
	}
	else
	{
		char errorMsg[512] = {0};
		snprintf(errorMsg, 511, "FMS_CPF_JTP_ConnectionsMgr::removeSubFileFromImm(), file not found:<%s>", fileName.c_str() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", errorMsg);
	}

	TRACE(m_trace, "%s", "Leaving removeSubFileFromImm()");
}

/*============================================================================
	ROUTINE: getLastFieldValue
 ============================================================================ */
void FMS_CPF_JTPConnectionsMgr::getLastFieldValue(const std::string& fileDN, std::string& value)
{
	TRACE(m_trace, "%s", "Entering in getLastFieldValue()");

	value.clear();
	// Get the file name from DN
	// Split the field in RDN and Value
	size_t equalPos = fileDN.find_first_of(parseSymbol::equal);
	size_t commaPos = fileDN.find_first_of(parseSymbol::comma);

	// Check if some error happens
	if( (std::string::npos != equalPos) )
	{
		// check for a single field case
		if( std::string::npos == commaPos )
			value = fileDN.substr(equalPos + 1);
		else
			value = fileDN.substr(equalPos + 1, (commaPos - equalPos - 1) );

		// make the value in upper case
		ACS_APGCC::toUpper(value);
	}

	TRACE(m_trace, "Leaving getLastFieldValue(), value:<%s>", value.c_str());
}

/*============================================================================
	ROUTINE: addWatcheOnLogFile
 ============================================================================ */
void FMS_CPF_JTPConnectionsMgr::addWatcherOnLogFile()
{
	TRACE(m_trace, "%s", "Entering in addWatcheOnLogFile()");

	// Only in SCP is required a watcher
	if(!m_IsMultiCP && (INVALID == m_watcherID))
	{
		try
		{
			std::string filePath;
			std::string cpName;

			FMS_CPF_FileId fileId(CPF_JTP::relCmdHdh);
			// check if the file is defined
			if(DirectoryStructureMgr::instance()->exists(fileId, cpName.c_str()))
			{
				// Open logical file to get phisycal path
				FileReference reference = DirectoryStructureMgr::instance()->open(fileId, FMS_CPF_Types::NONE_, cpName.c_str());

				filePath = reference->getPath();

				// add a watcher on folder
				m_watcherID = inotify_add_watch( m_InotifyFD, filePath.c_str(), IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM | IN_MOVED_TO); //HU59831

				TRACE(m_trace, "addWatcheOnLogFile(), add watcher:<%d> on folder:<%s>", m_watcherID, filePath.c_str());

				DirectoryStructureMgr::instance()->closeExceptionLess(reference);
			}
		}
		catch(FMS_CPF_PrivateException& ex)
		{
			char errorMsg[256] = {0};
			snprintf(errorMsg, 255, "FMS_CPF_JTPConnectionsMgr::addWatcheOnLogFile(), catched an exception:<%d> on file=<%s> open", ex.errorCode(), CPF_JTP::relCmdHdh);
			CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
			TRACE(m_trace, "%s", errorMsg);
		}
	}
	TRACE(m_trace, "%s", "Leaving addWatcheOnLogFile()");
}

/*============================================================================
	ROUTINE: startDeleteFileThread
 ============================================================================ */
bool FMS_CPF_JTPConnectionsMgr::startDeleteFileThread()
{
	ACE_thread_t tId = 0;
	int ret = -1;
	bool result = false;
	//spawn deleteExpiredFiles as JOINABLE thread
	ret = ACE_Thread::spawn((ACE_THR_FUNC)deleteExpiredFiles, (void*)this, THR_NEW_LWP|THR_JOINABLE, &tId, &m_deleteThreadHandle);
	if(0 == ret)
	{
		TRACE(m_trace, "%s", "Thread is spawned for delete operation !!");
		CPF_Log.Write("FMS_CPF_JTP_ConnectionsMgr::startDeleteFileThread(),deleteExpiredFiles Thread is spawned !!", LOG_LEVEL_WARN);
		result = true;
	}
	else
	{
		TRACE(m_trace, "%s", "Spawning a separate thread for delete operation is failed !!");
		CPF_Log.Write("FMS_CPF_JTP_ConnectionsMgr::startDeleteFileThread(),Spawning a separate thread for delete operation is failed !!", LOG_LEVEL_ERROR);
	}
	return result;
}

/*============================================================================
	ROUTINE: deleteExpiredFiles
 ============================================================================ */
void* FMS_CPF_JTPConnectionsMgr::deleteExpiredFiles(void * arg)
{
	FMS_CPF_JTPConnectionsMgr * thisJtpMgr = static_cast<FMS_CPF_JTPConnectionsMgr*>(arg);
	ACE_Time_Value tv;
	tv.set(10,0);
	TRACE(thisJtpMgr->m_trace, "%s", "deleteExpiredFiles(), Entering");
	while((!thisJtpMgr->m_isShutdownSignaled) && (thisJtpMgr->m_deleteFilesEv.wait(&tv,0)))
	{
		TRACE(thisJtpMgr->m_trace, "%s", "deleteExpiredFiles(), Checking expired files and MO to remove");
		FMS_CPF_JTPConnectionHndl::deleteFiles();
	}
	return (void*)0;
}

