/*
 * * @file fms_cpf_server.h
 *	@brief
 *	Header file for FMS_CPF_Server class.
 *  This module contains the declaration of the class FMS_CPF_Server.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-13
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
 *	| 1.0.0  | 2011-06-13 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_SERVER_H_
#define FMS_CPF_SERVER_H_

class ACS_TRA_trace;

class FMS_CPF_ConfigReader;
class FMS_CPF_ImmHandler;
class FMS_CPF_CmdHandler;
class FMS_CPF_CmdListener;
class FMS_CPF_CpChannelMgr;
class FMS_CPF_JTPConnectionsMgr;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_Server {

 public:

	/**
		@brief	constructor of FMS_CPF_Service class
	*/
	FMS_CPF_Server();

	/**
		@brief	destructor of FMS_CPF_Service class
	*/
	virtual ~FMS_CPF_Server();

	/**
		@brief	This method starts all worker threads to handle IMM callbacks
	*/
	bool startWorkerThreads();

	/**
		@brief	This method stops all worker threads
	*/
	bool stopWorkerThreads();

	/**
		@brief	This method waits that all worker threads are terminated
	*/
	bool waitOnShotdown();

	/**
		@brief	This method returns the shutdown handle
	*/
	int& getStopHandle() { return m_StopEvent; };

 private:

	/**
		@brief	waitBeforeRetry
		Wait 5s or stop signal before return
	*/
	int waitBeforeRetry() const;

	/**
			@brief	systemDiscovery
	*/
	bool systemDiscovery();

	/**
		@brief	stopImmHandler
		stop the Command handler thread
	*/
	bool stopCmdHandler();

	/**
		@brief	stopImmHandler
		stop the IMM handler thread
	*/
	bool stopImmHandler();

	/**
		@brief	Handler of internal command
	*/
	FMS_CPF_CmdHandler* m_CmdHandle;

	/**
		@brief	Handler to IMM objects
	*/
	FMS_CPF_ImmHandler* IMM_Handler;

	/**
		@brief	Handler of CP-AP communication
	*/
	FMS_CPF_CpChannelMgr* m_CpComHandler;

	/**
		@brief	systemConfig
		system configuration handler
	*/
	FMS_CPF_ConfigReader* m_systemConfig;

	/**
		@brief	Handler of cpf command
	*/
	FMS_CPF_CmdListener* m_APICmdHandler;

	/**
		@brief	Handler of jtp connection
	*/
	FMS_CPF_JTPConnectionsMgr* m_JtpConHandler;

	/**
		@brief	m_StopEvent : signal to internal thread to exit
	*/
	int m_StopEvent;

	/**
		@brief	m_SysDiscoveryOn : query to CS to configuration discovery
	*/
	bool m_SysDiscoveryOn;

	/**
		@brief	fms_cpf_serverTrace
	*/
	ACS_TRA_trace* fms_cpf_serverTrace;

};

#endif /* FMS_CPF_SERVER_H_ */
