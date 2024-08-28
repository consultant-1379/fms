 /*
 * * @file fms_cpf_eventalarmhndl.h
 *	@brief
 *	Header file for FMS_CPF_EventAlarmHndl class.
 *  This module contains the declaration of the class FMS_CPF_EventAlarmHndl.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-04-27
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
 *	| 1.0.0  | 2012-04-27 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_EVENTALARMHNDL_H_
#define FMS_CPF_EVENTALARMHNDL_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_privateexception.h"

#include <string>
#include <sstream>
#include <map>

#include <ace/Singleton.h>
#include <ace/Task_T.h>
#include <ace/Synch.h>
#include <ace/RW_Thread_Mutex.h>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */

class ACS_TRA_trace;
class acs_aeh_evreport;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_EventAlarmHndl: public ACE_Task<ACE_MT_SYNCH>
{
 public:

	friend class ACE_Singleton<FMS_CPF_EventAlarmHndl, ACE_Recursive_Thread_Mutex>;

	/**
	 * 	@brief  Run by a daemon thread
	*/
	virtual int svc(void);

	/** @brief handle_close method
	 *
	 *	This method is called when a <handle_*()> method returns -1 or when the
	 *	<remove_handler> method is called on an ACE_Reactor
	 *
	 *	@param handle file descriptor
	 *	@param close_mask indicates which event has triggered
	*/
	virtual int handle_timeout(const ACE_Time_Value& tv, const void*);

	/** @brief reportException method
	 *
	 *	This method reports an event to AEH if the event is not already reported in the
	 *	last five minutes
	 *
	 *	@param exception the exception  to report
	*/
	int reportException(FMS_CPF_PrivateException* exception);

	/** @brief reportException method
	 *
	 *	This method reports an event to AEH if the event is not already reported in the
	 *	last five minutes
	 *
	 *	@param detailInfo the event problem text to report
	 *	@param errorType the specific problem
	*/
	int reportException(const std::string& detailInfo, const FMS_CPF_PrivateException::errorType errType);

	/** @brief reportException method
	 *
	 *	This method reports an event to AEH if the event is not already reported in the
	 *	last five minutes
	 *
	 *	@param detailInfo the event problem text to report
	 *	@param errorType the specific problem
	*/
	int reportException(const std::stringstream& detailInfo, const FMS_CPF_PrivateException::errorType errType);

	/** @brief reportException method
	 *
	 *	This method reports an event to AEH if the event is not already reported in the
	 *	last five minutes
	 *
	 *	@param detailInfo the event problem text to report
	 *	@param errorType the specific problem
	*/
	int reportException(const char* detailInfo, const FMS_CPF_PrivateException::errorType errType);

	/** @brief reportEvent method
	 *
	 *	This method reports an event to AEH if the event is not already reported in the
	 *	last five minutes
	 *
	 *	@param eventProblemTxt the event problem text to report
	 *	@param errorType the specific problem
	*/
	int reportEvent(const std::string& eventProblemData, const int errorType);

	/** @brief raiseAlarm method
	 *
	 *	This method raises an alarm by AEH, if it is not already reported
	 *
	 *	@param tqName: the file transfer queue name
	*/
	void raiseFileTQAlarm(const std::string& tqName);

	/** @brief raiseAlarm method
	 *
	 *	This method raises an alarm by AEH, if it is not already reported
	 *
	 *	@param tqName: the transfer queue name
	*/
	void raiseBlockTQAlarm(const std::string& tqName);


	/** @brief ceaseAlarm method
	 *
	 *	This method ceases an alarm by AEH
	 *
	 *	@param tqName: the transfer queue name
	*/
	void ceaseAlarm(const std::string& tqName);

	/**
	 * 	@brief	This method stops and waits timer thread termination
	*/
	void shutdown();

	/**
	 * 	@brief	This method set the system type
	*/
	void setSystemType(bool isMultiCp);

 private:

	typedef struct {
		int eventTimeLife;
		std::string problemData;
	}eventData;

	/**
	 * 	@brief	Constructor of FMS_CPF_EventAlarmHndl class
	*/
	FMS_CPF_EventAlarmHndl();

	/**
	 * 	@brief	Destructor of FMS_CPF_EventAlarmHndl class
	*/
	virtual ~FMS_CPF_EventAlarmHndl();

	/** @brief findEvent method
	 *
	 *  This method checks if the event is already reported
	 *
	 * return 0 on success
	*/
	bool findEvent(const std::string& eventProblemData, const int errorType);

	/** @brief sendToAEH method
	 *
	 *  This method reports an event to AEH
	 *
	 * return 0 on success
	*/
	int sendToAEH(const std::string& eventProblemData, const int errorType, const std::string& eventProblemText);

	/** @brief populateTextMap method
	 *
	 *  This method inserts constants text into internal map
	 *
	*/
	void populateTextMap();

	/** @brief getProblemText method
	 *
	 *  This method gets problem text from the internal map
	 *
	*/
	void getProblemText(const int errorType, std::string& problemText);

	/** @brief raiseAlarm method
	 *
	 *	This method raises an alarm by AEH, if it is not already reported
	 *
	 *	@param tqName: the transfer queue name
	*/
	void raiseAlarm(const std::string& tqName, const std::string& alarmText);

	/** @brief ceaseAllAlarms method
	 *
	 *	This method ceases all raised alarms
	 *
	*/
	void ceaseAllAlarms();

	typedef std::multimap<int, eventData> eventMap;

	/**
	 * 	@brief	Enable(true)/Disable(false) alarm raising
	*/
	bool m_EnableToRaiseAlarm;

	/**
	 * 	@brief	m_EventMap
	*/
	eventMap m_EventMap;

	/**
	 * 	@brief	List of raised alarms
	*/
	std::map<std::string, std::string> m_AlarmList;

	/**
	 * 	@brief	m_TextMap, constants text map
	*/
	std::map<int, std::string> m_TextMap;

	/**
	 * 	@brief	m_threadTimerOn
	*/
	bool m_threadTimerOn;

	/**
	 * 	@brief	m_timerId
	*/
	long m_timerId;

	/**
	 * 	@brief	m_IsMultiCP
	 *
	 * 	Flag of system type (SCP/MCP)
	*/
	bool m_IsMultiCP;

	/**
	 * 	@brief	m_ProcessName
	*/
	std::string m_ProcessName;

	/**
	 * 	@brief	m_EventReportObj
	 *
	*/
	acs_aeh_evreport* m_EventReportObj;

	/**
	 * 	@brief	m_mutex
	 *
	 * 	Mutex for internal map access
	*/
	ACE_RW_Thread_Mutex m_Mutex;

	/**
	 *  @brief  m_TimerReactor
	*/
	ACE_Reactor* m_TimerReactor;

	/**
	 * 	@brief	m_Trace: trace object
	*/
	ACS_TRA_trace* m_Trace;
	//HY73946
        ACE_Recursive_Thread_Mutex  theThrMutex;
};

typedef ACE_Singleton<FMS_CPF_EventAlarmHndl, ACE_Recursive_Thread_Mutex> EventReport;

#endif /* FMS_CPF_EVENTALARMHNDL_H_ */
