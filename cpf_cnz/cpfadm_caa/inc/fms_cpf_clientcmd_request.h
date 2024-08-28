/*
 * * @file fms_cpf_clientcmd_request.h
 *	@brief
 *	Header file for CPF_ClientCmd_Request class.
 *  This module contains the declaration of the class CPF_ClientCmd_Request.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-08-08
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
 *	| 1.0.0  | 2011-08-08 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_CLIENTCMD_REQUEST_H_
#define FMS_CPF_CLIENTCMD_REQUEST_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_filereference.h"

#include <ACS_APGCC_DSD.H>
#include <ACS_APGCC_Command.H>

#include <ace/Task.h>
#include <utility>
#include <list>


class ACS_TRA_trace;
/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_ClientCmd_Request: public ACE_Task_Base
{
 public:

	/**
		@brief	Constructor of CPF_ClientCmd_Request class
	*/
	CPF_ClientCmd_Request(int& stopEvent);

	/**
		@brief	Destructor of CPF_ClientCmd_Request class
	*/
	virtual ~CPF_ClientCmd_Request();

	/**
   	   @brief  Run by a daemon thread
	*/
	virtual int svc(void);

	/**
	   @brief  This method initializes a task and prepare it for execution
	*/
	virtual int open (void *args = 0);

	/**
	   @brief  This method is called by ACE_Thread_Exit, as hook, on svc termination
	*/
	virtual int close(u_long flags = 0);

	/**
		@brief	This method verifies the protocol version used by the client
	*/
	bool checkConnectionData();

	/**
		@brief	This method gets the DSD stream
	*/
	ACS_APGCC_DSD_Stream& getStream() { return m_cmdStreamIO;}

 private:

	enum STATUS{
				EXIT = 0,
				CONTINUE,
	};


	/**
		@brief	This method gets a volume of a cp file by the fileName
	*/
	void getFileInfo() throw(FMS_CPF_PrivateException);

	/**
		@brief	This method gets user info on subfile
	*/
	void getSubFileInfo() throw(FMS_CPF_PrivateException);

	/**
		@brief	This method gets infinite subfiles list
	*/
	void getInfiniteSubFilesList() throw(FMS_CPF_PrivateException);

	/**
		@brief	This method removes an infinite subfile
	*/
	void removeInfiniteSubFile() throw(FMS_CPF_PrivateException);

	/**
		@brief	This method checks if a cp file exists
	*/
	void exists() throw(FMS_CPF_PrivateException);

	/**
		@brief	This method gets the cp file path
	*/
	void getCPFilePath() throw(FMS_CPF_PrivateException);


	/**
		@brief	This method opens a cp file
	*/
	void openFile() throw(FMS_CPF_PrivateException);

	/**
		@brief	This method closes a cp file
	*/
	void closeFile() throw(FMS_CPF_PrivateException);

	/**
		@brief	This method implements the action requested
	*/
	STATUS process_AP_CmdRequest();


	/**
	    @brief  m_ConState : flag to indicate connection state
	*/
	STATUS m_ConState;

	std::list< std::pair<FileReference,std::string> > fileReferenceList_;

	/**
	   @brief  	m_StopEvent, used to signal to every cmd client to terminate
	*/
	int m_StopEvent;

	/**
	    @brief  m_cmdStreamIO : DSD stream object
	*/
	ACS_APGCC_DSD_Stream m_cmdStreamIO;

	/**
	    @brief  m_cmdObj : APGCC_Command object
	*/
	ACS_APGCC_Command m_cmdObj;

	ACS_TRA_trace* cpf_clientcmdTrace;

	// Disallow copying and assignment.
	CPF_ClientCmd_Request(const CPF_ClientCmd_Request &);
	void operator= (const CPF_ClientCmd_Request &);


};

#endif /* FMS_CPF_CLIENTCMD_REQUEST_H_ */
