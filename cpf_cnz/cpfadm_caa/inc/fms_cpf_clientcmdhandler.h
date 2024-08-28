/*
 * * @file fms_cpf_clientcmdhandler.h
 *	@brief
 *	Header file for FMS_CPF_ClientCmdHandler class.
 *  This module contains the declaration of the class FMS_CPF_ClientCmdHandler.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-08-04
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
 *	| 1.0.0  | 2011-08-04 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_CLIENTCMDHANDLER_H_
#define FMS_CPF_CLIENTCMDHANDLER_H_
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include <ACS_APGCC_DSD.H>

#include <ace/Event_Handler.h>

class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FMS_CPF_ClientCmdHandler : public ACE_Event_Handler
{
 public:

	/**
		@brief	Constructor of FMS_CPF_ClientCmdHandler class
	*/
	FMS_CPF_ClientCmdHandler();

	/**
		@brief	Destructor of FMS_CPF_ClientCmdHandler class
	*/
	virtual ~FMS_CPF_ClientCmdHandler();

	/** @brief handle_input method
	 *
	 *	This method is called by reactor when input events occur
	 *
	 *	@param fd file descriptor
	*/
	virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);

	/** @brief handle_close method
	 *
	 *	This method is called when a <handle_*()> method returns -1 or when the
	 *	<remove_handler> method is called on an ACE_Reactor
	 *
	 *	@param handle file descriptor
	 *	@param close_mask indicates which event has triggered
	*/
	virtual int handle_close(ACE_HANDLE handle,
	                            ACE_Reactor_Mask close_mask);

	/** @brief get_handle method
	 *
	 *	This method gets the I/O handle.
	 *
	 *	@param index of the handle to get
	*/
	ACE_HANDLE get_handle(int index = 0){ return m_CmdServerAcceptor.get_handle(index); };

	/** @brief initDSDServer method
	 *
	 *	This method initialize DSD server.
	 *
	 *	return true on success, false otherwise.
	*/
	bool initDSDServer();


	/** @brief stopDSDServer method
	 *
	 *	This method close DSD server.
	 *
	*/
	void stopDSDServer();

 private:

	bool stopCmdClient();
	/**
	   @brief  	m_StopClientEvent, used to signal to every cmd client to terminate
	*/
	int m_StopClientEvent;

	/**
	   @brief  cmdServerAcceptor : DSD server object
	*/
	ACS_APGCC_DSD_Acceptor m_CmdServerAcceptor;


	/**
	   @brief  serverOnLine : indicates that DSD server is open
	*/
	bool m_serverOnLine;

	ACS_TRA_trace* fms_cpf_clientcmdTrace;
};

#endif /* FMS_CPF_CLIENTCMDHANDLER_H_ */
