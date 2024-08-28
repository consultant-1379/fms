/*
 * * @file fms_cpf_eventalarmhndl.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_EventAlarmHndl.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_eventalarmhndl.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-04-27
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

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_eventalarmhndl.h"
#include "fms_cpf_common.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "acs_aeh_evreport.h"

#include "ace/TP_Reactor.h"
#include "ace/Reactor.h"

#include <sstream>

extern ACS_TRA_Logging CPF_Log;

namespace eventInfo
{
	const int TimeIntervalSec = 300;
	const int TimerScheduling = 30;
	const long specificProblemOffset = 23000;
	const char probableCause[] = "AP INTERNAL FAULT";
	const char severity[] = "EVENT";
	const char objReferenceClass[] = "APZ";
	const char objReference[] = "CPF SERVER";
	const std::string commonProblemText("It will not be repeated if it occurs again within 5 minutes.");
}

namespace eventText
{
	const std::string internalError("A program error has occurred. ");
	const std::string tqNotFound("The specified transfer queue does not exist. ");
	const std::string physicalError("An error has occurred in the physical file system.");
	const std::string fileNotFound("The file was not found in the CP file system. ");
	const std::string invalidReference("The file reference is not associated with any opened files. ");
	const std::string accessError("An operation conflicts with other users of the file. ");
	const std::string fileExist("The file already exists in CP file system. ");
	const std::string attachFailed("Connection lost to transfer queue.  ");  //HV61830
	const std::string illiOption("Illegal attribute configuration in this system. ");
}

namespace alarmInfo
{
	const char probableCause[] = "DATA OUTPUT, AP TRANSMISSION FAULT";
	const char category[] = "A2";
	const char ceaseText[] = "CEASING";
	const char problemTextFile[] = "\nCAUSE\nCP FILESYSTEM, FILE TRANSFER QUEUE\n \nFILE NAME\n-\n \nTRANSFER QUEUE\ntransferQueueId=";
	const char problemTextBlock[] = "\nCAUSE\nCP FILESYSTEM, BLOCK TRANSFER QUEUE\n \nFILE NAME\n-\n \nTRANSFER QUEUE\ntransferQueueId=";
	const char problemText2[] = "DESTINATION SET\n-\n \nDESTINATION\n-\n";
	const long specificProblem = 23200;
	const char objReferenceClass[] = "APZ";
	const char problemData[] = "Connection lost to transfer queue";
}

/*============================================================================
	ROUTINE: FMS_CPF_EventAlarmHndl
 ============================================================================ */
FMS_CPF_EventAlarmHndl::FMS_CPF_EventAlarmHndl() :
 m_EnableToRaiseAlarm(true),
 m_threadTimerOn(false),
 m_timerId(-1),
 m_IsMultiCP(false),
 theThrMutex() //HY73946
{
	// assemble process name
	pid_t processPid = getpid();
	std::stringstream procName;
	procName << FMS_CPF_DAEMON_NAME << ":" << processPid << std::ends;
	m_ProcessName = procName.str();

	// Instance a Reactor to handle timeout events
	ACE_TP_Reactor* tp_reactor_impl = new ACE_TP_Reactor();

	// the reactor will delete the implementation on destruction
	m_TimerReactor = new ACE_Reactor(tp_reactor_impl, true);

	m_EventReportObj = new acs_aeh_evreport();
	m_Trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_EventAlarmHndl");
	populateTextMap();
}


/*============================================================================
	ROUTINE: handle_timeout
 ============================================================================ */
int FMS_CPF_EventAlarmHndl::handle_timeout(const ACE_Time_Value&, const void* arg)
{
	TRACE(m_Trace, "%s", "Entering in handle_timeout()");
	//To avoid warning
	UNUSED(arg);
        theThrMutex.acquire(); //HY73946

	eventMap::iterator eventElement;
	eventElement = m_EventMap.begin();

	while( eventElement != m_EventMap.end() )
	{
		eventElement->second.eventTimeLife--;
		if(eventElement->second.eventTimeLife < 0 )
		{
			// The post increment increments the iterator but returns the
			// original value for use by erase
			m_EventMap.erase(eventElement++);
		}
		else
		{
			//pre-increment is more efficient
			++eventElement;
		}
	}
	// Check if there isn't any events
	if(m_EventMap.empty())
	{
		TRACE(m_Trace, "%s", "handle_timeout(), map events empty cancel the timer");
		//stop Timer, it will be restarted when an event is again reported
		m_TimerReactor->cancel_timer(m_timerId);
		m_timerId = -1;
	}
	theThrMutex.release();  //HY73946
	TRACE(m_Trace, "%s", "Leaving in handle_timeout()");
	return SUCCESS;
}

/*============================================================================
	ROUTINE: reportEvent
 ============================================================================ */
int FMS_CPF_EventAlarmHndl::reportException(FMS_CPF_PrivateException* exception)
{
	TRACE(m_Trace, "%s", "Entering in reportException()");
	int result;
	std::string problemData;
	exception->getSlogan(problemData);
	int errorType = static_cast<int>(exception->errorCode());
	TRACE(m_Trace, "reportException(), errorType:<%d>", errorType);
	result = reportEvent(problemData, errorType);
	TRACE(m_Trace, "%s", "Leaving in reportException()");
	return result;
}

/*============================================================================
	ROUTINE: reportException
 ============================================================================ */
int FMS_CPF_EventAlarmHndl::reportException(const std::string& detailInfo, const FMS_CPF_PrivateException::errorType errType)
{
	TRACE(m_Trace, "%s", "Entering in reportException1()");
	int result;
	FMS_CPF_PrivateException exception(errType, detailInfo);
	result = reportException(&exception);
	TRACE(m_Trace, "%s", "Leaving reportException1()");
	return result;
}

/*============================================================================
	ROUTINE: reportException
 ============================================================================ */
int FMS_CPF_EventAlarmHndl::reportException(const std::stringstream& detailInfo, const FMS_CPF_PrivateException::errorType errType)
{
	TRACE(m_Trace, "%s", "Entering in reportException2()");
	int result;
	std::string tmpString = detailInfo.str();
	FMS_CPF_PrivateException exception(errType, tmpString);
	result = reportException(&exception);
	TRACE(m_Trace, "%s", "Leaving reportException2()");
	return result;
}

/*============================================================================
	ROUTINE: reportException
 ============================================================================ */
int FMS_CPF_EventAlarmHndl::reportException(const char* detailInfo, const FMS_CPF_PrivateException::errorType errType)
{
	TRACE(m_Trace, "%s", "Entering in reportException3()");
	int result;
	FMS_CPF_PrivateException exception(errType, detailInfo);
	result = reportException(&exception);
	TRACE(m_Trace, "%s", "Leaving reportException3()");
	return result;
}

/*============================================================================
	ROUTINE: reportEvent
 ============================================================================ */
int FMS_CPF_EventAlarmHndl::reportEvent(const std::string& eventProblemData, const int errorType)
{
	TRACE(m_Trace, "%s", "Entering in reportEvent()");
	int result = SUCCESS;
	theThrMutex.acquire(); //HY73946

	// Check if it possible report event
	if(m_EnableToRaiseAlarm)
	{
		if(!m_threadTimerOn)
		{
			// start event loop by svc thread
			result = activate();
			if( SUCCESS != result)
			{
				CPF_Log.Write("FMS_CPF_EventAlarmHndl::reportEvent(),  error on start timer thread", LOG_LEVEL_ERROR);
				TRACE(m_Trace, "%s", "reportEvent(), error on start timer thread");
			}
			else
			{
				TRACE(m_Trace, "%s", "reportEvent(), timer thread started");
				m_threadTimerOn = true;
			}
		}

		// Check if already reported
		if(!findEvent(eventProblemData, errorType))
		{
			TRACE(m_Trace, "%s", "reportEvent(), event to report");
			std::string problemText;
			getProblemText(errorType, problemText);
			int aehResult = sendToAEH(eventProblemData, errorType, problemText);
			if(SUCCESS == aehResult)
			{
				// Assemble the new event object
				eventData newEvent;
				// set the event life time to 300 sec, after thar it will be removed from the map
				// and a same event could be reported again
				newEvent.eventTimeLife = static_cast<int>(eventInfo::TimeIntervalSec / eventInfo::TimerScheduling);
				newEvent.problemData = eventProblemData;

				// Add a new event to the events map
				m_EventMap.insert(eventMap::value_type(errorType, newEvent));

				if(m_timerId == -1)
				{
					// schedule a timer to avoid to report the same event with a frequency less than 5 minutes
					ACE_Time_Value interval(eventInfo::TimerScheduling);
					m_timerId = m_TimerReactor->schedule_timer(this, 0, interval, interval);
				}
			}
		}
	}
	theThrMutex.release(); //HY73946 
	TRACE(m_Trace, "%s", "Leaving reportEvent()");
	return result;
}

/*============================================================================
	ROUTINE: raiseFileTQAlarm
 ============================================================================ */
void FMS_CPF_EventAlarmHndl::raiseFileTQAlarm(const std::string& tqName)
{
	TRACE(m_Trace, "Entering in %s()", __func__);

	// New text alarm to report
	std::stringstream alarmText;
	alarmText << alarmInfo::problemTextFile << tqName << "\n \n" << alarmInfo::problemText2 << std::ends;

	// report the alarm to AEH
	raiseAlarm(tqName, alarmText.str());

	TRACE(m_Trace, "Leaving %s()", __func__);
}

/*============================================================================
	ROUTINE: raiseFileTQAlarm
 ============================================================================ */
void FMS_CPF_EventAlarmHndl::raiseBlockTQAlarm(const std::string& tqName)
{
	TRACE(m_Trace, "Entering in %s()", __func__);

	// New text alarm to report
	std::stringstream alarmText;
	alarmText << alarmInfo::problemTextBlock << tqName << "\n \n" << alarmInfo::problemText2 << std::ends;

	// report the alarm to AEH
	raiseAlarm(tqName, alarmText.str());

	TRACE(m_Trace, "Leaving %s()", __func__);
}

/*============================================================================
	ROUTINE: raiseAlarm
 ============================================================================ */
void FMS_CPF_EventAlarmHndl::raiseAlarm(const std::string& tqName, const std::string& alarmText)
{
	TRACE(m_Trace, "Entering in %s()", __func__);

	theThrMutex.acquire();  //HY73946

	// Check if it possible raise alarm
	if(m_EnableToRaiseAlarm)
	{
		// check if already raised
		std::map<std::string, std::string>::const_iterator alarmElement;

		alarmElement = m_AlarmList.find(tqName);

		if( m_AlarmList.end() == alarmElement)
		{
			char logBuffer[256] = {0};
			// New alarm to raise
			ACS_AEH_ReturnType aehResult = m_EventReportObj->sendEventMessage(m_ProcessName.c_str(),
																			   alarmInfo::specificProblem,
																			   alarmInfo::category,
																			   alarmInfo::probableCause,
																			   alarmInfo::objReferenceClass,
																			   alarmInfo::objReferenceClass,
																			   alarmInfo::problemData,
																			   alarmText.c_str(),
																			   false
																			   );

			if(ACS_AEH_ok != aehResult)
			{
				// error on alarm raise
				snprintf(logBuffer, 255, "FMS_CPF_EventAlarmHndl::raiseAlarm(), raised alarm for TQ:<%s> failed, error:<%s>", tqName.c_str(), m_EventReportObj->getErrorText() );
				CPF_Log.Write(logBuffer, LOG_LEVEL_ERROR);
			}
			else
			{
				// insert the raised alarm into the alrm list
				m_AlarmList.insert(std::make_pair(tqName, alarmText));
				snprintf(logBuffer, 255, "FMS_CPF_EventAlarmHndl::raiseAlarm(), raised alarm for TQ:<%s>, active alarms:<%zd>", tqName.c_str(), m_AlarmList.size());
				CPF_Log.Write(logBuffer, LOG_LEVEL_INFO);
			}

			TRACE(m_Trace, "%s", logBuffer);
		}
	}
	theThrMutex.release();   //HY73946
	TRACE(m_Trace, "Leaving %s()", __func__);
}

/*============================================================================
	ROUTINE: ceaseAlarm
 ============================================================================ */
void FMS_CPF_EventAlarmHndl::ceaseAlarm(const std::string& tqName)
{
	TRACE(m_Trace, "Entering in %s()", __func__);

	// search the alarm into the list
	std::map<std::string, std::string>::iterator alarmElement;

	theThrMutex.acquire();  //HY73946

	alarmElement = m_AlarmList.find(tqName);

	if( m_AlarmList.end() != alarmElement)
	{
		char logBuffer[256] = {0};
		// alarm found, cease it
		ACS_AEH_ReturnType aehResult = m_EventReportObj->sendEventMessage(m_ProcessName.c_str(),
																		   alarmInfo::specificProblem,
																		   alarmInfo::ceaseText,
																		   alarmInfo::probableCause,
																		   alarmInfo::objReferenceClass,
																		   alarmInfo::objReferenceClass,
																		   alarmInfo::problemData,
																		   alarmElement->second.c_str(),
																		   false
																		   );

		if(ACS_AEH_ok != aehResult)
		{
			// error on alarm raise
			snprintf(logBuffer, 255, "FMS_CPF_EventAlarmHndl::ceaseAlarm(), ceased alarm for TQ:<%s> failed, error:<%s>", tqName.c_str(), m_EventReportObj->getErrorText() );
			CPF_Log.Write(logBuffer, LOG_LEVEL_ERROR);
		}
		else
		{
			// insert the raised alarm into the alrm list
			m_AlarmList.erase(alarmElement);
			snprintf(logBuffer, 255, "FMS_CPF_EventAlarmHndl::ceaseAlarm(), ceased alarm for TQ:<%s>, active alarms:<%zd>", tqName.c_str(), m_AlarmList.size() );
			CPF_Log.Write(logBuffer, LOG_LEVEL_INFO);
		}

		TRACE(m_Trace, "%s", logBuffer);
	}
	theThrMutex.release();  //HY73946

	TRACE(m_Trace, "Leaving %s()", __func__);
}

/*============================================================================
	ROUTINE: shutdown
 ============================================================================ */
void FMS_CPF_EventAlarmHndl::shutdown()
{
	TRACE(m_Trace, "%s", "Entering in shutdown()");
	// disable raising of new alarm
        theThrMutex.acquire(); //HY73946
	m_EnableToRaiseAlarm = false;
        theThrMutex.release(); //HY73946

	if(m_threadTimerOn)
	{
		if(m_timerId != -1)
		{
			TRACE(m_Trace, "%s", "shutdown(), cancel timer");
			m_TimerReactor->cancel_timer(m_timerId);
			m_timerId = -1;
		}

		TRACE(m_Trace, "%s", "shutdown(), stop timer thread");
		// stop timer thread
		m_TimerReactor->end_reactor_event_loop();

		// wait on svc termination
		wait();
		m_threadTimerOn = false;
	}
        theThrMutex.acquire(); //HY73946

	// clear all reported events from internal map
	m_EventMap.clear();

	// cease all raised alarms
	ceaseAllAlarms();
        theThrMutex.release();   //HY73946

	TRACE(m_Trace, "%s", "Leaving shutdown()");
}

/*============================================================================
	ROUTINE: setSystemType
 ============================================================================ */
void FMS_CPF_EventAlarmHndl::setSystemType(bool isMultiCp)
{
	m_IsMultiCP = isMultiCp;
	// Enable alarm raising
	m_EnableToRaiseAlarm = true;
}

/*============================================================================
	ROUTINE: svc
 ============================================================================ */
bool FMS_CPF_EventAlarmHndl::findEvent(const std::string& eventProblemData, const int errorType)
{
	TRACE(m_Trace, "%s", "Entering in findEvent()");
	bool result = false;
	eventMap::iterator eventElement;
	std::pair<eventMap::iterator, eventMap::iterator> eventRange;

	eventRange = m_EventMap.equal_range(errorType);

	for(eventElement = eventRange.first; eventElement != eventRange.second; ++eventElement)
	{
		std::string problemData = (*eventElement).second.problemData;
		if(eventProblemData.compare(problemData) == 0)
		{
			TRACE(m_Trace, "%s", "findEvent(), event already reported");
			result = true;
			break;
		}
	}
	TRACE(m_Trace, "%s", "Leaving findEvent()");
	return result;
}

/*============================================================================
	ROUTINE: svc
 ============================================================================ */
int FMS_CPF_EventAlarmHndl::svc()
{
	TRACE(m_Trace, "%s", "Entering in svc()");

	// Initialize the ACE_Reactor
	m_TimerReactor->open(1);

	TRACE(m_Trace, "%s", "svc(), starting reactor event loop");
	// start timer dispatching
	m_TimerReactor->run_reactor_event_loop();

	m_TimerReactor->close();

	TRACE(m_Trace, "%s", "Leaving svc()");
	return SUCCESS;
}

/*============================================================================
	ROUTINE: ~FMS_CPF_EventAlarmHndl
 ============================================================================ */
int FMS_CPF_EventAlarmHndl::sendToAEH(const std::string& eventProblemData, const int errorType, const std::string& eventProblemText)
{
	TRACE(m_Trace, "%s", "Entering in sendToAEH()");
	int result = SUCCESS;
	ACS_AEH_ReturnType aehResult;

	acs_aeh_specificProblem eventSpecificProblem = eventInfo::specificProblemOffset + errorType;

	std::string problemText(eventProblemText);
	problemText.append(eventInfo::commonProblemText);

	aehResult = m_EventReportObj->sendEventMessage(m_ProcessName.c_str(),
												   eventSpecificProblem,
												   eventInfo::severity,
												   eventInfo::probableCause,
												   eventInfo::objReferenceClass,
												   eventInfo::objReference,
												   eventProblemData.c_str(),
												   problemText.c_str()
												   );

	if(ACS_AEH_ok != aehResult)
	{
		TRACE(m_Trace, "%s", "sendToAEH(), send of event message failed");
		CPF_Log.Write("FMS_CPF_EventAlarmHndl::sendToAEH(), send of event message failed", LOG_LEVEL_ERROR);
		result = FAILURE;
	}

	TRACE(m_Trace, "%s", "Leaving sendToAEH()");
	return result;
}

/*============================================================================
	ROUTINE: populateTextMap
 ============================================================================ */
void FMS_CPF_EventAlarmHndl::populateTextMap()
{
	TRACE(m_Trace, "%s", "Entering in populateTextMap()");
	m_TextMap.clear();
	m_TextMap.insert(std::map<int, std::string>::value_type(FMS_CPF_PrivateException::INTERNALERROR, eventText::internalError));
	m_TextMap.insert(std::map<int, std::string>::value_type(FMS_CPF_PrivateException::TQNOTFOUND, eventText::tqNotFound));
	m_TextMap.insert(std::map<int, std::string>::value_type(FMS_CPF_PrivateException::PHYSICALERROR, eventText::physicalError));
	m_TextMap.insert(std::map<int, std::string>::value_type(FMS_CPF_PrivateException::FILENOTFOUND, eventText::fileNotFound));
	m_TextMap.insert(std::map<int, std::string>::value_type(FMS_CPF_PrivateException::INVALIDREF, eventText::invalidReference));
	m_TextMap.insert(std::map<int, std::string>::value_type(FMS_CPF_PrivateException::ACCESSERROR, eventText::accessError));
	m_TextMap.insert(std::map<int, std::string>::value_type(FMS_CPF_PrivateException::FILEEXISTS, eventText::fileExist));
	m_TextMap.insert(std::map<int, std::string>::value_type(FMS_CPF_PrivateException::ATTACHFAILED, eventText::attachFailed)); //HV61830
	m_TextMap.insert(std::map<int, std::string>::value_type(FMS_CPF_PrivateException::ILLOPTION, eventText::illiOption));
	TRACE(m_Trace, "%s", "Leaving populateTextMap()");
}

/*============================================================================
	ROUTINE: getProblemText
 ============================================================================ */
void FMS_CPF_EventAlarmHndl::getProblemText(const int errorType, std::string& problemText)
{
	TRACE(m_Trace, "%s", "Entering in getProblemText()");
	std::map<int, std::string>::const_iterator textElement;
	textElement = m_TextMap.find(errorType);

	if(m_TextMap.end() != textElement)
	{
		problemText = textElement->second;
	}

	TRACE(m_Trace, "%s", "Leaving getProblemText()");
}

/*============================================================================
	ROUTINE: ceaseAllAlarms
 ============================================================================ */
void FMS_CPF_EventAlarmHndl::ceaseAllAlarms()
{
	TRACE(m_Trace, "%Entering in %s()", __func__);
	// search the alarm into the list
	std::map<std::string, std::string>::iterator alarmElement;
	// cease all the alarm at shutdown
	for(alarmElement = m_AlarmList.begin(); alarmElement != m_AlarmList.end(); ++alarmElement)
	{
		m_EventReportObj->sendEventMessage(m_ProcessName.c_str(),
										   alarmInfo::specificProblem,
										   alarmInfo::ceaseText,
										   alarmInfo::probableCause,
										   alarmInfo::objReferenceClass,
										   alarmInfo::objReferenceClass,
										   alarmInfo::problemData,
										   alarmElement->second.c_str(),
										   false
										   );
	}
	CPF_Log.Write("FMS_CPF_EventAlarmHndl::ceaseAllAlarms(), ceased all alarms", LOG_LEVEL_INFO);
	m_AlarmList.clear();

	TRACE(m_Trace, "Leaving %s()", __func__);
}

/*============================================================================
	ROUTINE: ~FMS_CPF_EventAlarmHndl
 ============================================================================ */
FMS_CPF_EventAlarmHndl::~FMS_CPF_EventAlarmHndl()
{
	if(NULL != m_EventReportObj)
		delete m_EventReportObj;

	if(NULL != m_TimerReactor)
		delete m_TimerReactor;

	if(NULL != m_Trace)
		delete m_Trace;
	
	theThrMutex.remove(); //HY73946
}
