/*
 * * @file fms_cpf_client.h
 *	@brief
 *	Header file for FMS_CPF_Client class.
 *  This module contains the declaration of the class FMS_CPF_Client.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-08-17
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
 *	| 1.0.0  | 2011-08-17 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_CLIENT_H
#define FMS_CPF_CLIENT_H

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_exception.h"

class ACS_APGCC_Command;
class ACS_APGCC_DSD_Stream;
class ACS_APGCC_DSD_Connector;
class ACS_APGCC_DSD_Addr;

class ACS_TRA_trace;
/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FMS_CPF_Client 
{
 public:

	/**
		@brief	Constructor of FMS_CPF_Client class
	*/
    FMS_CPF_Client();

    /**
   		@brief	Destructor of FMS_CPF_Client class
   	*/
    ~FMS_CPF_Client();

    /**
       	@brief	execute method
       	This method sends the command execution request to the CPF server by DSD interface
    */
    int execute(ACS_APGCC_Command& cmd) throw(FMS_CPF_Exception);

 private:

    /**
        @brief	ICClient class definition
        This is  an inner class to handle DSD communication
    */
    class ICClient{

      public:
    	ICClient(const char* service_name, ACS_TRA_trace* clientTrace);
    	~ICClient();
    	int open();
	    int execute(ACS_APGCC_Command& cmd);
	    int sendCommand(ACS_APGCC_Command& cmd);
	    int close();

   	  private:
	    ACS_APGCC_DSD_Stream*   m_DSD_stream;
	    ACS_APGCC_DSD_Connector* m_DSD_client;
	    ACS_APGCC_DSD_Addr* m_DSD_address;
	    ACS_TRA_trace* cpfICClientTrace;
	    bool connected;
    };

    /**
        @brief	init method
        This method initialize the DSD objects
    */
    void init() throw(FMS_CPF_Exception);

    /**
        @brief	CPFClient: inner class object
    */
    ICClient*  CPFClient;

    ACS_TRA_trace* cpfClientTrace;

};

#endif //FMS_CPF_CLIENT_H
