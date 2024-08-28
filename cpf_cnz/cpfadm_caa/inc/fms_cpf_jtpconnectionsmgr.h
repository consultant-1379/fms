 /*
 * * @file fms_cpf_jtpconnectionsmgr.h
 *	@brief
 *	Header file for FMS_CPF_JTPConnectionsMgr class.
 *  This module contains the declaration of the class FMS_CPF_JTPConnectionsMgr.
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

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_JTPCONNECTIONSMGR_H_
#define FMS_CPF_JTPCONNECTIONSMGR_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "ACS_JTP.h"
#include <ace/Task.h>

#include <string>
#include <list>
#include <map> //HU59831
#include <ace/Event.h>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class FMS_CPF_JTPConnectionHndl;

class ACS_TRA_trace;

/*=====================================================================
						CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_JTPConnectionsMgr: public ACE_Task_Base
{
 public:

	/**
		@brief	Constructor of FMS_CPF_JTPConnectionsMgr class
	*/
	FMS_CPF_JTPConnectionsMgr();

	/**
		@brief	Destructor of FMS_CPF_JTPConnectionsMgr class
	*/
	virtual ~FMS_CPF_JTPConnectionsMgr();

	/**
	    @brief 	Run by a daemon thread
	*/
	virtual int svc(void);


	/**
	    @brief 	This method initializes a task and prepare it for execution
	*/
	virtual int open(void *args = 0);

	/** @brief setSystemType method
	 *
	 *	This method sets the system type Single CP / Multi CP
	 *
	*/
	inline void setSystemType(bool sysType) { m_IsMultiCP = sysType; };

	/** @brief stopJTPConnectionsMgr method
	 *
	 *	This method close connections manager, and wait for a graceful closure of all threads
	 *
	*/
	void stopJTPConnectionsMgr();

	bool startDeleteFileThread();

	static void* deleteExpiredFiles(void * arg);

 private:

	/** @brief publishJTPServer method
	 *
	 *	This method initialize and publish the CPF server towards JTP
	 *
	 *	return true on success, false otherwise.
	*/
	bool publishJTPServer();

	/** @brief clientConnectionHandling method
	 *
	 *	This method handling a jtp client connection
	 *
	*/
	void clientConnectionHandling();

	/** @brief cleanCloseSession method
	 *
	 *	This method deallocates disconnected client handler
	 *
	*/
	void cleanCloseSession(bool shutDown = false);

	/** @brief handlingFolderChange method
	 *
	 *	This method handles file or subfile deletion
	 *
	*/
	void handlingFolderChange();

	/** @brief removeSubFileFromImm method
	 *
	 *	This method remove subfile object from IMM
	 *
	*/
	void removeSubFileFromImm(const std::string& subFileName);

	/** @brief	getLastFieldValue
	 *
	 *  This method returns the value of last field of a DN
	 *
	 *  @remarks Remarks
	 */
	void getLastFieldValue(const std::string& fileDN, std::string& value);

	/** @brief	getLastFieldValue
	 *
	 *  This method adds a watcher on the RELCMDHDF file if it exists
	 *
	 *  @remarks Remarks
	 */
	void addWatcherOnLogFile();

	/**
	 * 	@brief	m_IsMultiCP
	 *
	 * 	Flag of system type (SCP/MCP)
	*/
	bool m_IsMultiCP;

	/**
		@brief	m_JTPServer : JTP server object
	*/
	ACS_JTP_Service* m_JTPServer;

	/**
	 	 @brief m_InotifyFD, inotify file descriptor
	*/
	int m_InotifyFD;

	/**
	 	 @brief m_watcherID, active watch descriptor
	*/
	int m_watcherID;

	/**
	    @brief m_ConnectionList, list of active connections
	*/
	std::list<FMS_CPF_JTPConnectionHndl*> m_ConnectionList;

	/**
		@brief	m_StopEvent : signal to internal thread to exit
	*/
	int m_StopEvent;

	/**	@brief	trautil object trace
	*/
	ACS_TRA_trace* m_trace;
	std::map<std::string, std::string> m_renameMap; //HU59831

	ACE_hthread_t m_deleteThreadHandle;
	bool m_isShutdownSignaled;
	ACE_Event m_deleteFilesEv;

};

#endif /* FMS_CPF_JTPCONNECTIONSMGR_H_ */
