/*
 * * @file fms_cpf_immhandler.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_ImmHandler.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_service.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-14
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
 *	| 1.0.0  | 2011-06-14 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 *	| 1.1.0  | 2011-09-24 | qvincon      | File updated with action handling.  |                       |
 *	+========+============+==============+=====================================+
 *	| 1.2.0  | 2011-10-13 | qvincon      | File updated with infinite file OI  |                       |
 *	+========+============+==============+=====================================+
 *	| 1.2.0  | 2012-02-16 | qvincon      | File updated with simple file OI    |
 *	+========+============+==============+=====================================+
 *
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_immhandler.h"
#include "fms_cpf_oi_cpfroot.h"
#include "fms_cpf_oi_cpvolume.h"
#include "fms_cpf_oi_compositefile.h"
#include "fms_cpf_oi_compositesubfile.h"
#include "fms_cpf_oi_infinitefile.h"
#include "fms_cpf_oi_simplefile.h"
#include "fms_cpf_configreader.h"
#include "fms_cpf_cmdhandler.h"
#include "fms_cpf_common.h"


#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include "ace/OS_NS_poll.h"
#include <ace/Barrier.h>
#include <sys/eventfd.h>
#include <poll.h>
#include <iostream>
#include <string>
#include <list>

extern ACS_TRA_Logging CPF_Log;

// dummy info for Cluster as CP name
namespace SSICluster{
	const char clusterName[] = "CLUSTER";
	const int clusterId = 5001;
}

/*============================================================================
	ROUTINE: cpVolumeCallbackHnd
 ============================================================================ */
ACE_THR_FUNC_RETURN FMS_CPF_ImmHandler::actionCallbackHandler(void* ptrParam)
{
	FMS_CPF_ImmHandler* m_this = (FMS_CPF_ImmHandler*)ptrParam;
	int stopEventHandle;
	const nfds_t nfds = 2;
	struct pollfd fds[nfds];

	ACE_INT32 ret;

	char msg_buff[256]={'\0'};
	ACS_CC_ReturnType result;

	// Initialize the pollfd structure
	ACE_OS::memset(fds, 0 , sizeof(fds));

	m_this->getStopHandle(stopEventHandle);

	fds[0].fd = stopEventHandle;
	fds[0].events = POLLIN;

	// CpVolume callback handle
	fds[1].fd = m_this->m_oi_CpFileSystemRoot->getSelObj();
	fds[1].events = POLLIN;

	// waiting for IMM requests or stop
	while(m_this->getSvcState())
	{
		ret = ACE_OS::poll(fds, nfds);

		if( -1 == ret )
		{
			if(errno == EINTR)
			{
				continue;
			}
			snprintf(msg_buff, sizeof(msg_buff)-1, "actionCallbackHandler(), exit after error=<%s>", strerror(errno) );
			CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
			TRACE(m_this->fms_cpf_immhandlerTrace, "%s", "actionCallbackHandler(), exit after poll error");
			break;
		}

		if(fds[0].revents & POLLIN)
		{
			// Received a stop request from server
			TRACE(m_this->fms_cpf_immhandlerTrace, "%s", "actionCallbackHandler(), received a stop request from server");
			break;
		}

		if(fds[1].revents & POLLIN)
		{
			// Received a IMM request on a CpVolume
			TRACE(m_this->fms_cpf_immhandlerTrace, "%s","actionCallbackHandler(), received IMM request on a CpVolume");
			result = m_this->m_oi_CpFileSystemRoot->dispatch(ACS_APGCC_DISPATCH_ONE);

			if(ACS_CC_SUCCESS != result)
			{
				snprintf(msg_buff, sizeof(msg_buff)-1, "actionCallbackHandler(), error on Cp File System Root dispatch event" );
				CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
				TRACE(m_this->fms_cpf_immhandlerTrace, "%s", "actionCallbackHandler(), error on Cp File System Root dispatch event");
			}
			continue;
		}
	}

	// Threads shutdown barrier, Block the caller until all N_THREADS count threads have called N_THREADS wait and
	// then allow all the caller threads to continue in parallel.
	m_this->m_ThreadsSyncShutdown->wait();
	return 0;
}

/*============================================================================
	ROUTINE: FMS_CPF_ImmHandler
 ============================================================================ */
FMS_CPF_ImmHandler::FMS_CPF_ImmHandler(FMS_CPF_CmdHandler* cmdHandler)
{
	// Create objct to sync thread termination
	m_ThreadsSyncShutdown = new ACE_Barrier(FMS_CPF_ImmHandler::N_THREADS);

	// create the file descriptor to signal stop
	m_StopEvent = eventfd(0,0);

	// create the file descriptor to signal svc terminated
	m_svcTerminated = eventfd(0,0);

	// create the Cp File System Root OI
	m_oi_CpFileSystemRoot = new FMS_CPF_OI_CPFRoot(cmdHandler);

	// create CpVolume OI
	m_oi_CpVolume = new FMS_CPF_OI_CpVolume(cmdHandler);

	// create CompositeFile OI
	m_oi_CompositeFile = new FMS_CPF_OI_CompositeFile(cmdHandler);

	// create CompositeSubFile OI
	m_oi_CompositeSubFile = new FMS_CPF_OI_CompositeSubFile(cmdHandler);

	// create InfiniteFile OI
	m_oi_InfiniteFile = new FMS_CPF_OI_InfiniteFile(cmdHandler);

	// create SimpleFile OI
	m_oi_SimpleFile = new FMS_CPF_OI_SimpleFile(cmdHandler);

	// Initialize the svc state flag
	svc_run = false;

	// to avoid warning
	m_IsMultiCP = false;

	m_stopSignalReceived = false; //HX56277

	m_cmdHandler = cmdHandler;

	fms_cpf_immhandlerTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_ImmHandler");
}

/*============================================================================
	ROUTINE: ~FMS_CPF_ImmHandler
 ============================================================================ */
FMS_CPF_ImmHandler::~FMS_CPF_ImmHandler()
{
	ACE_OS::close(m_StopEvent);
	ACE_OS::close(m_svcTerminated);
	//HY38505
        OmHandler objManager;
         char errMsg[512] = {0};


        for (int i =0; i < 10; ++i)
            {
              if( m_oiHandler.removeClassImpl(m_oi_CpFileSystemRoot, m_oi_CpFileSystemRoot->getIMMClassName()) == ACS_CC_FAILURE )
                  {
                     int intErr = objManager.getInternalLastError();
                       if( intErr == -6)
                            {
                              for(int j=0; j< 100; ++j) ; //do nothing loop to wait for sometime, better than sleeping
                                  if ( i >= 10)
                                      {
                                       ACE_OS::snprintf(errMsg, 511,"Error occurred while removing class implementer after waitin in destg, ErrCode:<%d>",objManager.getInternalLastError());
                                       CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
                                       TRACE(fms_cpf_immhandlerTrace, "%s", "Error occurred while removing class implementer after waitingin dest, ErrCode= %d.", objManager.getInternalLastError());                                                        break;
                                      }
                                        else
                                                continue;
                            }
                                else
				{
                                     ACE_OS::snprintf(errMsg, 511,"Error occurred while removing class implemente in destr, ErrCode:<%d>",objManager.getInternalLastError());
                                     CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
                                     TRACE(fms_cpf_immhandlerTrace, "%s", "Error occurred while removing class implementerin destructor, ErrCode= %d.", objManager.getInternalLastError());
                                        break;
                                }
                  
		}
			else
                        {
                                ACE_OS::snprintf(errMsg, 511,"no error occurred while removing class implementerin destructor, ErrCode:<%d>",objManager.getInternalLastError());

				CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
                                TRACE(fms_cpf_immhandlerTrace, "%s", "removing class implementer Success");
                                       break;
                        }
                }
// end of HY38505



	if(NULL != m_oi_CpFileSystemRoot)
	{
		// remove Cp File System root OI definition
		m_oiHandler.removeClassImpl(m_oi_CpFileSystemRoot, m_oi_CpFileSystemRoot->getIMMClassName());
		delete m_oi_CpFileSystemRoot;
	}

	if(NULL != m_oi_CpVolume)
	{
		// remove CpVolume OI definition
		m_oiHandler.removeClassImpl(m_oi_CpVolume, m_oi_CpVolume->getIMMClassName());
		delete m_oi_CpVolume;
	}

	if(NULL != m_oi_InfiniteFile)
	{
		// remove InfiniteFile OI definition
		m_oiHandler.removeClassImpl(m_oi_InfiniteFile, m_oi_InfiniteFile->getIMMClassName());
		delete m_oi_InfiniteFile;
	}

	if(NULL != m_oi_CompositeFile)
	{
		// remove CompositeFile OI definition
		m_oiHandler.removeClassImpl(m_oi_CompositeFile, m_oi_CompositeFile->getIMMClassName());
		delete m_oi_CompositeFile;
	}

	if(NULL != m_oi_CompositeSubFile)
	{
		// remove CompositeFile OI definition
		m_oiHandler.removeClassImpl(m_oi_CompositeSubFile, m_oi_CompositeSubFile->getIMMClassName());
		delete m_oi_CompositeSubFile;
	}

	if(NULL != m_oi_SimpleFile)
	{
		// remove SimpleFile OI definition
		m_oiHandler.removeClassImpl(m_oi_SimpleFile, m_oi_SimpleFile->getIMMClassName());
		delete m_oi_SimpleFile;
	}

	if(NULL != fms_cpf_immhandlerTrace)
		delete fms_cpf_immhandlerTrace;

	if(NULL != m_ThreadsSyncShutdown)
		delete m_ThreadsSyncShutdown;
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
int FMS_CPF_ImmHandler::open(void *args)
{
	TRACE(fms_cpf_immhandlerTrace, "%s", "Entering open()");

	// To avoid warning about unused parameter
	UNUSED(args);


	// Initialize IMM and set Implementers in the map
	if(!registerImmOI())
	{
		CPF_Log.Write("FMS_CPF_ImmHandler::open(), error on Implementer set", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_immhandlerTrace, "%s", "Leaving open(), error on Implementer set");
		return FAILURE;
	}

	// Set system configuration parameters
	m_oi_CpFileSystemRoot->setSystemParameters(m_IsMultiCP, m_mapCpNameId);
	m_oi_CpVolume->setSystemParameters(m_IsMultiCP, m_mapCpNameId);
	m_oi_CompositeFile->setSystemParameters(m_IsMultiCP, m_mapCpNameId);
	m_oi_CompositeSubFile->setSystemParameters(m_IsMultiCP, m_mapCpNameId);
	m_oi_InfiniteFile->setSystemParameters(m_IsMultiCP, m_mapCpNameId);
	m_oi_SimpleFile->setSystemParameters(m_IsMultiCP, m_mapCpNameId);

	TRACE(fms_cpf_immhandlerTrace, "%s", "open, Start thread to handler IMM request");
	// Start the thread to handler IMM request
	if( activate() == FAILURE )
	{
		m_oiHandler.removeClassImpl(m_oi_CpFileSystemRoot, m_oi_CpFileSystemRoot->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_CpVolume, m_oi_CpVolume->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_CompositeFile, m_oi_CompositeFile->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_CompositeSubFile, m_oi_CompositeSubFile->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_InfiniteFile, m_oi_InfiniteFile->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_SimpleFile, m_oi_SimpleFile->getIMMClassName());

		CPF_Log.Write("FMS_CPF_ImmHandler::open,  error on start svc thread", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_immhandlerTrace, "%s", "Leaving open(), error on start svc thread");
		return FAILURE;
	}
	TRACE(fms_cpf_immhandlerTrace, "%s", "Leaving open()");
	return SUCCESS;
}

/*============================================================================
	ROUTINE: registerImmOI
 ============================================================================ */
bool FMS_CPF_ImmHandler::registerImmOI()
{
        TRACE(fms_cpf_immhandlerTrace, "%s", "Entering registerImmOI()");
        ACS_CC_ReturnType result;
         // HY38505
         OmHandler objManager;
         char errMsg[512] = {0};
          //START of HX56277
        uint8_t maxRetry=0;
        do
        {  //HY38505
           for (int i =0; i < 10; ++i)
                {
                        if( m_oiHandler.removeClassImpl(m_oi_CpFileSystemRoot, m_oi_CpFileSystemRoot->getIMMClassName()) == ACS_CC_FAILURE )
                        {
                               int intErr = objManager.getInternalLastError();
                                if( intErr == -6)
                                {
                                     for(int j=0; j< 100; ++j) ; //do nothing loop to wait for sometime, better than sleeping
                                        if ( i >= 10)
                                        {
                                         ACE_OS::snprintf(errMsg, 511,"Error occurred while removing class implementer after waiting, ErrCode:<%d>",objManager.getInternalLastError());
                                           CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
                                            TRACE(fms_cpf_immhandlerTrace, "%s", "Error occurred while removing class implementer after waiting, ErrCode= %d.", objManager.getInternalLastError());                                                        break;
                                    }
                                        else
                                                continue;
                                }
                                else
{
                                     ACE_OS::snprintf(errMsg, 511,"Error occurred while removing class implementer, ErrCode:<%d>",objManager.getInternalLastError());
                                     CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
                                       TRACE(fms_cpf_immhandlerTrace, "%s", "Error occurred while removing class implementer, ErrCode= %d.", objManager.getInternalLastError());
                                        break;
                                }
                        }
                        else
                        {
                                ACE_OS::snprintf(errMsg, 511,"no error occurred while removing class implementer, ErrCode:<%d>",objManager.getInternalLastError());
                                     CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
                                        TRACE(fms_cpf_immhandlerTrace, "%s", "removing class implementer Success");
                                break;
                        }
                }//HY38505 end
                       

	
		// Register the OI of CpVolume class
		result = m_oiHandler.addClassImpl(m_oi_CpFileSystemRoot, m_oi_CpFileSystemRoot->getIMMClassName());

		if(ACS_CC_FAILURE == result)
		{
			//sleeping for 10 seconds - JIRA CC-13975-retry mechanism
			if (m_stopSignalReceived)
			{
				return false;
			}
			
                        sleep(10);
                        maxRetry++;
                        if(maxRetry==3)
                        {

				CPF_Log.Write("FMS_CPF_ImmHandler::open,Register with IMM failed after 3 retries for the CpFileSystemRoot Implementer", LOG_LEVEL_ERROR);

				TRACE(fms_cpf_immhandlerTrace, "%s", "Leaving registerImmOI(), error on set CpFileSystemRoot Implementer");
				return false;
			}
		}
		else
		{
			break;
		}
	}while(!m_stopSignalReceived || maxRetry<3);
	//END of HX56277

	// Register the OI of CpVolume class
	result = m_oiHandler.addClassImpl(m_oi_CpVolume, m_oi_CpVolume->getIMMClassName());

	if(ACS_CC_FAILURE == result)
	{
		m_oiHandler.removeClassImpl(m_oi_CpFileSystemRoot, m_oi_CpFileSystemRoot->getIMMClassName());
		CPF_Log.Write("FMS_CPF_ImmHandler::open, error on set CpVolume Implementer", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_immhandlerTrace, "%s", "Leaving registerImmOI(), error on set CpVolume Implementer");
		return false;
	}

	// Register the OI of CompositeFile class
	result = m_oiHandler.addClassImpl(m_oi_CompositeFile, m_oi_CompositeFile->getIMMClassName());

	if(ACS_CC_FAILURE == result)
	{
		m_oiHandler.removeClassImpl(m_oi_CpFileSystemRoot, m_oi_CpFileSystemRoot->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_CpVolume, m_oi_CpVolume->getIMMClassName());

		CPF_Log.Write("FMS_CPF_ImmHandler::open, error on set CompositeFile Implementer", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_immhandlerTrace, "%s", "Leaving registerImmOI(), error on set CompositeFile Implementer");
		return false;
	}

	// Register the OI of CompositeSubFile class
	result = m_oiHandler.addClassImpl(m_oi_CompositeSubFile, m_oi_CompositeSubFile->getIMMClassName());

	if(ACS_CC_FAILURE == result)
	{
		m_oiHandler.removeClassImpl(m_oi_CpFileSystemRoot, m_oi_CpFileSystemRoot->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_CpVolume, m_oi_CpVolume->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_CompositeFile, m_oi_CompositeFile->getIMMClassName());

		CPF_Log.Write("FMS_CPF_ImmHandler::open, error on set CompositeSubFile Implementer", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_immhandlerTrace, "%s", "Leaving registerImmOI(), error on set CompositeSubFile Implementer");
		return false;
	}

	// Register the OI of InfiniteFile class
	result = m_oiHandler.addClassImpl(m_oi_InfiniteFile, m_oi_InfiniteFile->getIMMClassName());

	if(ACS_CC_FAILURE == result)
	{
		m_oiHandler.removeClassImpl(m_oi_CpFileSystemRoot, m_oi_CpFileSystemRoot->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_CpVolume, m_oi_CpVolume->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_CompositeFile, m_oi_CompositeFile->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_CompositeSubFile, m_oi_CompositeSubFile->getIMMClassName());

		CPF_Log.Write("FMS_CPF_ImmHandler::open, error on set InfiniteFile Implementer", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_immhandlerTrace, "%s", "Leaving registerImmOI(), error on set InfiniteFile Implementer");
		return false;
	}

	// Register the OI of SimpleFile class
	result = m_oiHandler.addClassImpl(m_oi_SimpleFile, m_oi_SimpleFile->getIMMClassName());

	if(ACS_CC_FAILURE == result)
	{
		m_oiHandler.removeClassImpl(m_oi_CpFileSystemRoot, m_oi_CpFileSystemRoot->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_CpVolume, m_oi_CpVolume->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_CompositeFile, m_oi_CompositeFile->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_CompositeSubFile, m_oi_CompositeSubFile->getIMMClassName());
		m_oiHandler.removeClassImpl(m_oi_InfiniteFile, m_oi_InfiniteFile->getIMMClassName());
		CPF_Log.Write("FMS_CPF_ImmHandler::open, error on set InfiniteFile Implementer", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_immhandlerTrace, "%s", "Leaving registerImmOI(), error on set InfiniteFile Implementer");
		return false;
	}

	TRACE(fms_cpf_immhandlerTrace, "%s", "Leaving registerImmOI()");
	return true;
}

/*============================================================================
	ROUTINE: setSystemParameters
 ============================================================================ */
void FMS_CPF_ImmHandler::setSystemParameters(FMS_CPF_ConfigReader* sysConf)
{
	TRACE(fms_cpf_immhandlerTrace, "%s", "Entering setSystemParameters()");

	m_IsMultiCP = sysConf->IsBladeCluster();
	m_oi_CompositeFile->isMulticp(m_IsMultiCP);

	if( m_IsMultiCP )
	{
		TRACE(fms_cpf_immhandlerTrace, "%s", "setSystemParameters(), Multi CP System");
		// create a map with cpname and cpid
		std::list<short> cpIdList = sysConf->getCP_IdList();
		std::list<short>::iterator idx;
		for( idx = cpIdList.begin(); idx != cpIdList.end(); ++idx)
		{
			// get the CPname from the CPid
			std::string tmpCpName = sysConf->cs_getDefaultCPName((*idx));
			m_mapCpNameId.insert(pair<std::string,short>(tmpCpName, *idx));
			m_CpList.push_back(tmpCpName);
		}
	}
	else
	{
		TRACE(fms_cpf_immhandlerTrace, "%s", "setSystemParameters(), Single CP System");
	}
	TRACE(fms_cpf_immhandlerTrace, "%s", "Leaving setSystemParameters()");
}

/*============================================================================
	ROUTINE: svc
 ============================================================================ */
int FMS_CPF_ImmHandler::svc()
{
	TRACE(fms_cpf_immhandlerTrace, "%s", "Entering svc() thread");
	// Create a fd to wait for request from IMM
	const nfds_t nfds = 6;

	struct pollfd fds[nfds];

	const size_t msgLength = 255;
	char msg_buff[msgLength+1]={'\0'};

	ACE_INT32 poolResult;
	ACS_CC_ReturnType result;

	// Set the svc thread state to on
	svc_run = true;

	// Initialize the pollfd structure
	ACE_OS::memset(fds, 0 , sizeof(fds));

	fds[0].fd = m_StopEvent;
	fds[0].events = POLLIN;

	fds[1].fd = m_oi_CpVolume->getSelObj();
	fds[1].events = POLLIN;

	fds[2].fd = m_oi_CompositeFile->getSelObj();
	fds[2].events = POLLIN;

	fds[3].fd = m_oi_CompositeSubFile->getSelObj();
	fds[3].events = POLLIN;

	fds[4].fd = m_oi_InfiniteFile->getSelObj();
	fds[4].events = POLLIN;

	fds[5].fd = m_oi_SimpleFile->getSelObj();
	fds[5].events = POLLIN;

	// Start a different thread for CpVolume class in order to handle the Actions
	ACE_Thread::spawn(actionCallbackHandler, (void*)this);
	TRACE(fms_cpf_immhandlerTrace, "%s", "svc thread, waiting for IMM request or service stop");

	// waiting for IMM requests or stop
	while(svc_run)
	{
		poolResult = ACE_OS::poll(fds, nfds);

		if( FAILURE == poolResult )
		{
			if(errno == EINTR)
			{
				continue;
			}
			snprintf(msg_buff, msgLength, "FMS_CPF_ImmHandler::svc(), exit after error=%s", strerror(errno) );
			CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
			TRACE(fms_cpf_immhandlerTrace, "%s", "Leaving svc(), exit after poll error");
			break;
		}

		if(fds[0].revents & POLLIN)
		{
			// Received a stop request from server
			TRACE(fms_cpf_immhandlerTrace, "%s", "Leaving svc(), received a stop request from server");
			break;
		}

		//Start of TR. HX99576
		if (!m_stopSignalReceived)
		{

			if(fds[1].revents & POLLIN)
			{
				// Received a IMM request on a CpVolume
				TRACE(fms_cpf_immhandlerTrace, "%s", "svc(), received IMM request on a CpVolume");
				result = m_oi_CpVolume->dispatch(ACS_APGCC_DISPATCH_ONE);

				if(ACS_CC_SUCCESS != result)
				{
					snprintf(msg_buff, msgLength, "FMS_CPF_ImmHandler::svc, error on CpVolume dispatch event" );
					CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
					TRACE(fms_cpf_immhandlerTrace, "%s", msg_buff);
				}
				continue;
			}

			if(fds[2].revents & POLLIN)
			{
				// Received a IMM request on a CompositeFile
				TRACE(fms_cpf_immhandlerTrace, "%s", "svc(), received IMM request on a CompositeFile");
				result = m_oi_CompositeFile->dispatch(ACS_APGCC_DISPATCH_ONE);

				if(ACS_CC_SUCCESS != result)
				{
					snprintf(msg_buff, msgLength, "FMS_CPF_ImmHandler::svc, error on CompositeFile dispatch event" );
					CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
					TRACE(fms_cpf_immhandlerTrace, "%s", msg_buff);
				}
				continue;
			}

			if(fds[3].revents & POLLIN)
			{
				// Received a IMM request on a CompositeSubFile
				TRACE(fms_cpf_immhandlerTrace, "%s", "svc(), received IMM request on a CompositeSubFile");
				result = m_oi_CompositeSubFile->dispatch(ACS_APGCC_DISPATCH_ONE);
	
				if(ACS_CC_SUCCESS != result)
				{
					snprintf(msg_buff, msgLength, "FMS_CPF_ImmHandler::svc(), error on CompositeSubFile dispatch event" );
					CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
					TRACE(fms_cpf_immhandlerTrace, "%s", msg_buff);
				}
				continue;
			}

			if(fds[4].revents & POLLIN)
			{
				// Received a IMM request on a InfiniteFile
				TRACE(fms_cpf_immhandlerTrace, "%s", "svc(), received IMM request on a InfiniteFile");
				result = m_oi_InfiniteFile->dispatch(ACS_APGCC_DISPATCH_ONE);
	
				if(ACS_CC_SUCCESS != result)
				{
					snprintf(msg_buff, msgLength, "FMS_CPF_ImmHandler::svc(), error on InfiniteFile dispatch event" );
					CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
					TRACE(fms_cpf_immhandlerTrace, "%s", msg_buff);
				}
				continue;
			}

			if(fds[5].revents & POLLIN)
			{
				// Received a IMM request on a SimpleFile
				TRACE(fms_cpf_immhandlerTrace, "%s","svc, received IMM request on a SimpleFile");
				result = m_oi_SimpleFile->dispatch(ACS_APGCC_DISPATCH_ONE);

				if(ACS_CC_SUCCESS != result)
				{
					snprintf(msg_buff, msgLength, "FMS_CPF_ImmHandler::svc(), error on SimpleFile dispatch event" );
					CPF_Log.Write(msg_buff,	LOG_LEVEL_ERROR);
					TRACE(fms_cpf_immhandlerTrace, "%s", msg_buff);
				}
				continue;
			}
		}  //End of TR. HX99576
	}
	// Threads shutdown barrier, Block the caller until all N_THREADS count threads have called N_THREADS wait and
	// then allow all the caller threads to continue in parallel.
	m_ThreadsSyncShutdown->wait();

	// Signal svc termination
	signalSvcTermination();

	// Set the svc thread state to off
	svc_run = false;
	TRACE(fms_cpf_immhandlerTrace, "%s","Leaving svc() thread");
	return SUCCESS;
}

/*============================================================================
	ROUTINE: signalSvcTermination
 ============================================================================ */
void FMS_CPF_ImmHandler::signalSvcTermination()
{
	TRACE(fms_cpf_immhandlerTrace, "%s", "Entering signalSvcTermination()");
	ACE_UINT64 stopEvent=1;
	ssize_t numByte;

	// Signal to IMM thread to stop
	numByte = write(m_svcTerminated, &stopEvent, sizeof(ACE_UINT64));

	if(sizeof(ACE_UINT64) != numByte)
	{
		CPF_Log.Write("FMS_CPF_ImmHandler::signalSvcTermination, error on signal svc termination", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_immhandlerTrace, "%s", "signalSvcTermination, error on signal svc termination");
	}
	TRACE(fms_cpf_immhandlerTrace, "%s", "signalSvcTermination()");
}
