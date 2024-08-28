/*
 * * @file fms_cpf_client.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_Client.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_client.h module
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
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_client.h"
#include "fms_cpf_apdcommonfile.h"
#include "fms_cpf_api_trace.h"

#include <ACS_APGCC_DSD.H>
#include <ACS_APGCC_Command.H>
#include <acs_prc_api.h>

#include "ACS_TRA_trace.h"

/*===================================================================
                        INNER CLASS IMPLEMENTATION
=================================================================== */
FMS_CPF_Client::ICClient::ICClient(const char* service_name, ACS_TRA_trace* clientTrace)
{
	connected = false;

	// create the DSD objects
	m_DSD_address = new ACS_APGCC_DSD_Addr(service_name);
	m_DSD_stream = new ACS_APGCC_DSD_Stream();
	m_DSD_client =  new ACS_APGCC_DSD_Connector();

	// Use the same trace object of FMS_CPF_Client
	cpfICClientTrace = clientTrace;
}

FMS_CPF_Client::ICClient::~ICClient()
{
	delete m_DSD_address;
	delete m_DSD_stream;
	delete m_DSD_client;
}

int FMS_CPF_Client::ICClient::open()
{
	int result = -1;
	TRACE(cpfICClientTrace, "%s", "Entering in ICClient open()");
	int conResult = m_DSD_client->connect((*m_DSD_stream), (*m_DSD_address) );

	if( conResult != 0 )
	{
		TRACE(cpfICClientTrace, "ICClient open(), DSD Connection error=%i", conResult);
		return result;
    }
	result = 0;
	connected = true;
	TRACE(cpfICClientTrace, "%s", "ICClient open() leaving");
    return result;
}

int FMS_CPF_Client::ICClient::execute(ACS_APGCC_Command& cmd)
{
	int result = -1;
    cmd.result = -1;
    TRACE(cpfICClientTrace, "%s", "Entering in ICClient execute()");
    // Checks if the DSD client is connected
    if(connected)
    {
    	//Sends the command request
    	if(cmd.send(*m_DSD_stream) < 0)
    	{
    		TRACE(cpfICClientTrace, "%s", "ICClient execute() data send error");
    		return result;
    	}

    	// Receives the command response
    	if(cmd.recv(*m_DSD_stream) < 0)
    	{
    		TRACE(cpfICClientTrace, "%s", "ICClient execute() data receive error");
    		return result;
    	}

    	// Operation success
    	result = 0;
    }
    TRACE(cpfICClientTrace, "Leaving ICClient execute(), result=%i", result);
    return result;
 }

int FMS_CPF_Client::ICClient::sendCommand(ACS_APGCC_Command& cmd)
{
	TRACE(cpfICClientTrace, "%s", "Entering in ICClient sendCommand()");
	int result = -1;
    cmd.result = -1;

    // checks if connected
    if(connected)
    {
        if(cmd.send(*m_DSD_stream) == 0)
        {
        	// Data are send
        	result = 0;
        }
    }
    TRACE(cpfICClientTrace, "Leaving ICClient sendCommand(), result=%i", result);
    return result;
}
  
int FMS_CPF_Client::ICClient::close()
{
	TRACE(cpfICClientTrace, "%s", "Entering in ICClient close()");
    if(connected)
    {
    	m_DSD_stream->close();
    	m_DSD_client->close();
    }
    connected = false; 
    TRACE(cpfICClientTrace, "%s", "Leaving ICClient close()");
    return 0;
}


//------------------------------------------------------------------------------
//      Constructor   
//------------------------------------------------------------------------------
FMS_CPF_Client::FMS_CPF_Client()
{
	CPFClient = NULL;
	cpfClientTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_Client");
}


//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FMS_CPF_Client::~FMS_CPF_Client ()
{
  if( NULL != CPFClient )
  {
	  // send the exit to the CPF server
	  ACS_APGCC_Command cmd;
	  cmd.cmdCode = CPF_API_Protocol::EXIT_;
	  cmd.data[0] = 0;
	  CPFClient->sendCommand(cmd);

	  // close dsd objects
	  CPFClient->close();

	  delete CPFClient;
	  CPFClient = NULL;
  }

  if( NULL != cpfClientTrace )
	  delete cpfClientTrace;
}

//------------------------------------------------------------------------------
//      execute 
//------------------------------------------------------------------------------
int FMS_CPF_Client::execute(ACS_APGCC_Command& cmd)  throw(FMS_CPF_Exception)
{
	TRACE(cpfClientTrace, "%s", "Entering in execute()");

	if(NULL == CPFClient )
	{
		try
		{
		  init();
		}
		catch(FMS_CPF_Exception& ex)
		{
		  TRACE(cpfClientTrace, "%s", "execute(), exception in init");
		  if( NULL != CPFClient )
		  {
			  CPFClient->close();
			  delete CPFClient;
			  CPFClient = NULL;
		  }
		  // Re-throw the caught exception
		  throw;
		}
	}

  int result = CPFClient->execute(cmd);

  TRACE(cpfClientTrace, "%s", "Leaving execute()");
  return result;
}   


//------------------------------------------------------------------------------
//      Initialize communication towards server
//------------------------------------------------------------------------------
void FMS_CPF_Client::init() throw(FMS_CPF_Exception)
{
	TRACE(cpfClientTrace, "%s", "Entering in init()");
	int result;
	ACS_APGCC_Command cmd;
	//Instantiate the DSD client wrapper object
	CPFClient = new ICClient(CPF_API_Protocol::CPF_DSD_Address, cpfClientTrace);

    cmd.cmdCode = CPF_API_Protocol::INIT_Session;
	cmd.data[0] = CPF_API_Protocol::MAGIC_NUMBER;
	cmd.data[1] = CPF_API_Protocol::VERSION;

    //HF54685 begin
	ACS_PRC_API prcApi;
    int nodeState = prcApi.askForNodeState();

    // nodeState = -1 Undefined, 0 Passive, 1 Active.
    if (nodeState == 0) // Passive
    {
    	TRACE(cpfClientTrace, "%s", "init(), error node is Passive");
    	FMS_CPF_Exception exErr(FMS_CPF_Exception::NODEISPASSIVE);
    	std::string errMsg(exErr.errorText());
    	errMsg += "Node is Passive";
    	throw FMS_CPF_Exception(FMS_CPF_Exception::NODEISPASSIVE, errMsg);
    }

    // Open connection
    result = CPFClient->open();
  
    if (result < 0)
    {
    	TRACE(cpfClientTrace, "%s", "init(), Unable to connect to server");
    	throw FMS_CPF_Exception(FMS_CPF_Exception::UNABLECONNECT);
    }

    //execute command (send to server)
    result = CPFClient->execute(cmd);

    if (result < 0)
    {
    	TRACE(cpfClientTrace, "%s", "init(), cannot open stream socket");
    	FMS_CPF_Exception exErr(FMS_CPF_Exception::SOCKETERROR);
    	std::string errMsg(exErr.errorText());
    	errMsg += "Cannot open stream socket";
    	throw FMS_CPF_Exception (FMS_CPF_Exception::SOCKETERROR, errMsg);
    }

    TRACE(cpfClientTrace, "%s", "Leaving init()");
}
