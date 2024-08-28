/*
 * * @file fms_cpf_cpdfilethrd.h
 *	@brief
 *	Header file for CPDFileThrd class.
 *  This module contains the declaration of the class CPDFileThrd.
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
 *	| 1.0.0  | 2011-11-15 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 *	| 1.1.0  | 2011-11-15 | qvincon      | ACE introduction.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_CPDFILETHRD_H_
#define FMS_CPF_CPDFILETHRD_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_fileid.h"

#include <ace/Task_T.h>
#include <ace/Synch.h>
#include <ace/Monotonic_Time_Policy.h>

#include <map>
#include <string>
#include <stdint.h>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
 class CPDFile;
 class FMS_CPF_CPMsg;
 class FileLock;
/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPDFileThrd : public ACE_Task<ACE_MT_SYNCH, ACE_Monotonic_Time_Policy >
{
 public:

	/**
	 * 	@brief	Constructor of CPDFileThrd class
	*/
	CPDFileThrd();

	/**
	 * 	@brief	Destructor of CPDFileThrd class
	*/
	virtual ~CPDFileThrd();

	/**
	 * 	@brief  Run by a daemon thread, execution part
	*/
	virtual int svc(void);

	/**
	 * 	@brief	This method initializes a task and prepare it for execution
	*/
	virtual int open(void *args = 0);

	/**
	 * 	@brief	This method called from ACE_Thread_Exit when thread exit
	*/
	virtual int close(u_long flags = 0);

	/**
	 *	@brief putFileRef
	 *
	 *	This method saves our file reference
	*/
	bool putFileRef(uint32_t aFileRefUlong, CPDFile* aCPDFile, bool infiniteFileOpen = false );

	/**
	 *	@brief removeFileRef
	 *
	 *	This method inhibits further messages to this file reference
	*/
    void removeFileRef();
  
    /**
	 *	@brief getFileRef
	 *
	 *	This method fetch own file reference
	*/
    inline CPDFile* getFileRef() const {return m_CpdFile; };
  
    /**
	 *	@brief setMultiWrite
	 *
	 *	This method sets multiwrite support on, ie. allow sorting of cp messages
	*/
    inline void setOnMultiWrite(){ m_MultiWriteOn = true; };
  
    /**
	 *	@brief setNextSeqNr
	 *
	 *	This method sets sequence number for next message
	*/
    inline void setNextSeqNr(uint16_t nextNr) { m_NextSeqNr = nextNr; };

    /**
	 *	@brief fileIsClosed
	 *
	 *	This method inhibit furher access to file
	*/
    inline void setFileIsClosed() { m_FileClosed = true; };

    /**
	 *	@brief saveOpenLock
	 *
	 *	This method saves open/close Lock reference
	*/
    inline void saveOpenLock(FileLock* lock) {m_OpenLock = lock; };

    /**
	 *	@brief setOnSync
	 *
	 *	This method notifies the thread about the sync message
	*/
    inline void setOnSync(){ m_SyncReceived = true; };

    /**
   	 *	@brief setCpName
   	 *
   	 *	This method sets the cp Name
   	*/
    inline void setCpName(const char* cpName) { m_CpName = cpName;} ;

    /**
	 *	@brief setCpName
	 *
	 *	This method gets the cp Name
	*/
    inline const char* getCpName() const { return m_CpName.c_str(); };

    /**
	 *	@brief shutDown
	 *
	 *	This method stop the worker thread
	 *	@param isServiceShutDown: indicates a service shutdown order or not
	*/
    void shutDown(bool isServiceShutDown = false);

 private:

    /**
	 *	@brief insertCpMsg
	 *
	 *	This method inserts CP msg in the wait queue
	*/
    void insertCpMsg(const uint16_t seqNr, FMS_CPF_CPMsg* aMsg);

    /**
  	 *	@brief
  	 *
  	 *	This method fetch a CP msg with specific seq.no from wait queue
  	*/
    FMS_CPF_CPMsg* fetchCpMsg(const uint16_t aSeqNr);

    /**
  	 *	@brief
  	 *
  	 *	This method fetch a CP msg from in queue.
  	*/

    FMS_CPF_CPMsg* fetchCpMsg();

    /**
  	 *	@brief
  	 *
  	 *	This method checks for a switch due to max time after an infinite file has been closed
  	*/
    void checkSubFileSwitch();
  
    /**
  	 *	@brief waitForMaxTimeSwitch
  	 *
  	 *	This method waits for a switch due to max time after an infinite file has been closed
  	*/
    void waitForMaxTimeSwitch();

    /* sorted CP msg wait queue */
    typedef std::map<uint16_t, FMS_CPF_CPMsg*> cpMsgMap;

    cpMsgMap m_CpMsgQueue;

    CPDFile* m_CpdFile;

    uint32_t m_FileReference;

    uint16_t m_NextSeqNr;

    bool m_FileClosed;

    int m_FSWLoopCnt;

    bool m_MultiWriteOn;

    bool m_FileInList;

    FMS_CPF_FileId m_fileIdInList;

    FileLock* m_OpenLock;

    bool m_SyncReceived;

    bool m_ServiceShutDown;

    std::string m_CpName;

    ACS_TRA_trace* fms_cpf_CpFileTrace;
};

#endif //FMS_CPF_CPDFILETHRD_H_
