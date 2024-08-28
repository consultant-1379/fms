/*
 * * @file fms_cpf_immhandler.h
 *	@brief
 *	Header file for FMS_CPF_ImmHandler class.
 *  This module contains the declaration of the class FMS_CPF_ImmHandler.
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
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_IMMHANDLER_H_
#define FMS_CPF_IMMHANDLER_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include <ace/Task.h>
#include <acs_apgcc_oihandler_V3.h>

#include <map>
#include <list>
#include <string>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class FMS_CPF_OI_CPFRoot;
class FMS_CPF_OI_CpVolume;
class FMS_CPF_OI_CompositeFile;
class FMS_CPF_OI_CompositeSubFile;
class FMS_CPF_OI_InfiniteFile;
class FMS_CPF_OI_SimpleFile;

class FMS_CPF_CmdHandler;
class FMS_CPF_ConfigReader;
class ACE_Barrier;
class ACS_TRA_trace;

typedef std::map<std::string, short> map_CPnameId;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FMS_CPF_ImmHandler: public ACE_Task_Base {

 public:

	/**
		@brief	Constructor of FMS_CPF_ImmHandler class
	*/
	FMS_CPF_ImmHandler(FMS_CPF_CmdHandler* cmdHandler);

	/**
		@brief	Destructor of FMS_CPF_ImmHandler class
	*/
	virtual ~FMS_CPF_ImmHandler();

	/**
	   @brief  	Run by a daemon thread
	*/
	virtual int svc(void);

	/**
	   @brief  	This method initializes a task and prepare it for execution
	*/
	virtual int open(void *args = 0);

	/**
	   @brief  	This method get the stop handle to terminate the svc thread
	*/
	void getStopHandle(int& stopFD){stopFD = m_StopEvent;};
	
	/**
	   @brief       This method set the stop signal when the service is going down
	*/
	void setStopSignal(bool status){m_stopSignalReceived = status;}; //HX99576

	/**
	   @brief  	This method get the svc thread termination handle
	*/
	int& getSvcEndHandle() {return m_svcTerminated;};

	/**
	   @brief  	This method get the svc thread state
	*/
	bool getSvcState() const {return svc_run;};

	/**
	   @brief  	This method retrieves system parameters from the FMS_CPF_ConfigReader object
	*/
	void setSystemParameters(FMS_CPF_ConfigReader* sysConf);

	/**
	 * 	@brief	fms_cpf_immhandlerTrace: trace object
	*/
	ACS_TRA_trace* fms_cpf_immhandlerTrace;

	/**
		   @brief  	m_oi_CpVolume: CpVolume OI
	*/
	FMS_CPF_OI_CpVolume* m_oi_CpVolume;

	ACE_Barrier* m_ThreadsSyncShutdown;

 private:

	enum { N_THREADS = 2 };

	/**
		@brief		This method implement a thread to handle the action request Cp File System

		@return		ACE_THR_FUNC_RETURN

		@exception	none
	*/
	static ACE_THR_FUNC_RETURN actionCallbackHandler(void* ptrParam);

	/**
	   @brief 	This method signal the svc thread termination
	*/
	void signalSvcTermination();

	/**
	   @brief  		This method register the OI
	*/
	bool registerImmOI();

	/**
	   @brief  	m_oiHandler
	*/
	acs_apgcc_oihandler_V3 m_oiHandler;

	/**
	   @brief  	m_oi_CpFileSystemRoot: CpFileSystemRoot OI
	*/
	FMS_CPF_OI_CPFRoot* m_oi_CpFileSystemRoot;

	/**
	   @brief  	m_oi_CompositeFile: Composite File OI
	*/
	FMS_CPF_OI_CompositeFile* m_oi_CompositeFile;

	/**
	   @brief  	m_oi_CompositeSubFile: Composite SubFile OI
	*/
	FMS_CPF_OI_CompositeSubFile* m_oi_CompositeSubFile;

	/**
	   @brief  	m_oi_InfiniteFile: Infinite File OI
	*/
	FMS_CPF_OI_InfiniteFile* m_oi_InfiniteFile;

	/**
	   @brief  	m_oi_SimpleFile: Simple File OI
	*/
	FMS_CPF_OI_SimpleFile* m_oi_SimpleFile;

	/**
	   @brief  	m_StopEvent
	*/
	int m_StopEvent;

	/**
	   @brief  	svc_run: svc state flag
	*/
	bool svc_run;

	/**
	   @brief  	m_svcTerminated: to signal out-side the svc termination
	*/
	int m_svcTerminated;

	/**
		@brief	m_IsMultiCP
	*/
	bool m_IsMultiCP;
	/**
		@brief stopSignalReceived
	*/
	bool m_stopSignalReceived; //HX56277

	/**
		@brief	m_mapCpNameId
	*/
	map_CPnameId m_mapCpNameId;

	/**
		@brief	m_CpList : list of CP name
    */
	std::list<std::string> m_CpList;

	/**
	    @brief	m_cmdHandler :
    */
	FMS_CPF_CmdHandler* m_cmdHandler;

};

#endif /* FMS_CPF_IMMHANDLER_H_ */
