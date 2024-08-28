/*
 * * @file fms_cpf_jtp_connectionhndl.h
 *	@brief
 *	Header file for FMS_CPF_JTPConnectionHndl class.
 *  This module contains the declaration of the class FMS_CPF_JTPConnectionHndl.
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
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_JTPCONNECTIONHNDL_H_
#define FMS_CPF_JTPCONNECTIONHNDL_H_
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include <ace/Task.h>
#include "ACS_JTP.h"
#include <map>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */

class FMS_CPF_JTPCpMsg;
class ACS_TRA_trace;

typedef std::map<ACE_INT64, std::string> TimeSortedMap;  //ROH

typedef std::pair<ACE_INT64, std::string> TimeSortedMapPair;

typedef std::map<std::string, TimeSortedMap*> CompositeFileMap;

typedef std::map<std::string, ACE_INT32> DeleteFileTimerMap;
/*=====================================================================
						CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_JTPConnectionHndl: public ACE_Task_Base
{
 public:

	/**
		@brief	Constructor of FMS_CPF_JTP_ConnectionsMgr class
	*/
	FMS_CPF_JTPConnectionHndl(ACS_JTP_Conversation* jtpSession, bool systemType);

	/**
		@brief	Destructor of FMS_CPF_JTP_ConnectionsMgr class
	*/
	virtual ~FMS_CPF_JTPConnectionHndl();

	/**
		@brief  This method initializes a task and prepare it for execution
	*/
	virtual int open(void *args = 0);

	/**
		@brief  Run by a daemon thread
	*/
	virtual int svc(void);

	/**
		@brief  This method is called by ACE_Thread_Exit, as hook, on svc termination
	*/
	virtual int close(u_long flags = 0);

	/**
		@brief  This method return thread execution state
	*/
	bool isConnectionStateOn() const { return m_ThreadState; };

	static void updateDeleteFileTimer(const std::string& compositeFile, ACE_INT32 deleteFileTimer); //ROH

	static void removeDeleteFileTimer(const std::string& compositeFile);

	static void removeCompositeFile(const std::string& compositeFile);
	/**
                @brief  This method returns the deleteFileTimer of the CompositeFile
        */

	static ACE_INT32 getDeleteFileTimer(const std::string& compositeFile);
	/**
                @brief  This method deletes the files when delete file timer is expired
        */

	static void deleteFiles();		
	 /**
                @brief  This method deletes the files when delete file timer is expired
        */

	static bool readyForDelete(ACE_UINT64  absoluteDeletionTime, ACE_INT32 removeDelay);
	
	/**
                @brief  This method returns the time in seconds from 1970 to present
        */

	static unsigned long getAbsTime();

	static void shutDown();

	static void initFromFile(const std::string& compositeFile);

	static void removeFromFile(const std::string& compositeFile,const std::string& fileName);

	static void writeToFile(const unsigned long& creationDate, const std::string& compositeFile, const std::string& fileName);

	static void cleanUp();

 private:

	/**
		@brief  This method handles the Cp request and replies to it
	*/
	void handlingCpRequest();

	/**
		@brief  This method removes a sent file
	*/
	unsigned int removeSentFile();

	/**
		@brief  This method sends a file to the TQ
	*/
	unsigned int sendFile();

	/**
		@brief  This method sets a file transfered as archived
	*/
	unsigned int setFileArchived();

	/**
		@brief  This method gets the status info of  a file transfered
	*/
	unsigned int getSentFileStatus();

	/**
		@brief  This method gets the main file path
	*/
	static void getFilePath(std::string& filePath, const char* fileName, const char* cpName = ""); 

	/**
		@brief  This method gets the subfile size
	*/
	uint32_t getSubFileSize() const;

	/**
		@brief  This method remove subfile object from IMM on MCP system
	*/
	static void removeSubFileFromImm(const char* fileName, const char* subFileName, std::string cpName = "");

	/**
	 * 	JTP conversation object
	*/
	ACS_JTP_Conversation* m_JTPSession;

	/**
	 * 	JTP message object
	*/
	FMS_CPF_JTPCpMsg* m_JTPMsg;

	/**
	 * 	@brief	m_IsMultiCP
	 *
	 * 	Flag of system type (SCP/MCP)
	*/
	bool m_IsMultiCP;

	/**
	 * 	Flag of connection thread state
	*/
	bool m_ThreadState;

	/**
	 * 	Name of the Cp connected by JTP
	*/
	std::string m_CpName;

	/**	@brief	trautil object trace
	*/
	static ACS_TRA_trace m_trace;

	static DeleteFileTimerMap theDeleteFileTimerMap; //ROH

	static TimeSortedMapPair theTimeSortedMapPair;

	static CompositeFileMap theCompositeFileMap;

	static bool m_isShutdownSignaled;

	static ACE_Recursive_Thread_Mutex theMutex;

};

#endif /* FMS_CPF_JTPCONNECTIONHNDL_H_ */
