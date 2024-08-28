/*
 * * @file fms_cpf_clientcmd_request.cpp
 *	@brief
 *	Class method implementation for CPF_ClientCmd_Request.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_clientcmd_request.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-23
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
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_clientcmd_request.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_common.h"

#include "fms_cpf_apdcommonfile.h"
#include "fms_cpf_privateexception.h"
#include "fms_cpf_fileid.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	ROUTINE: CPF_ClientCmd_Request
 ============================================================================ */
CPF_ClientCmd_Request::CPF_ClientCmd_Request(int& stopEvent):
m_ConState(EXIT),
m_StopEvent(stopEvent)
{
	cpf_clientcmdTrace = new ACS_TRA_trace("CPF_ClientCmd_Request");
}

/*============================================================================
	ROUTINE: close
 ============================================================================ */
int CPF_ClientCmd_Request::close(u_long flags)
{
	// auto destroyed
	TRACE(cpf_clientcmdTrace, "%s", "close(), client cmd object deletion");
	UNUSED(flags);
	delete this;
	return 0;
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
int CPF_ClientCmd_Request::open(void *args)
{
	TRACE(cpf_clientcmdTrace, "%s", "Entering open()");

	// To avoid warning about unused parameter
	UNUSED(args);
	int result;
	m_ConState = CONTINUE;

	// start event loop by svc thread
	result = activate( (THR_NEW_LWP| THR_DETACHED | THR_INHERIT_SCHED));

	// Check if the svc thread is started
	if(0 != result)
	{
		CPF_Log.Write("CPF_ClientCmd_Request::open,  error on start svc thread", LOG_LEVEL_ERROR);
		TRACE(cpf_clientcmdTrace, "%s", "open(), error on start svc thread");
	}

	TRACE(cpf_clientcmdTrace, "%s", "Leaving open()");

	return result;
}

/*============================================================================
	ROUTINE: checkConnectionData
 ============================================================================ */
bool CPF_ClientCmd_Request::checkConnectionData()
{
	TRACE(cpf_clientcmdTrace, "%s", "Entering checkConnectionData()");
	bool result= false;

	// Read data on dsd stream
	if( m_cmdObj.recv(m_cmdStreamIO) == 0)
	{
		TRACE(cpf_clientcmdTrace, "checkConnectionData(), checks protocol values cmd code= %i, data size=%i", m_cmdObj.cmdCode, m_cmdObj.data.size());
		if( m_cmdObj.cmdCode == CPF_API_Protocol::INIT_Session && m_cmdObj.data.size() >= 2)
		{
			if( m_cmdObj.data[0] ==  CPF_API_Protocol::MAGIC_NUMBER &&
				m_cmdObj.data[1] ==  CPF_API_Protocol::VERSION )
			{
				TRACE(cpf_clientcmdTrace, "%s", "checkConnectionData(), initial protocol values are valid");
				result = true;
				m_cmdObj.result = FMS_CPF_Exception::OK;
			}
			else
			{
				TRACE(cpf_clientcmdTrace, "%s", "checkConnectionData(), initial protocol values are NOT valid");
				FMS_CPF_PrivateException ex(FMS_CPF_PrivateException::PROTOCOLERROR, "Wrong magic or version number");
				m_cmdObj.data[0] = ex.errorText();
				m_cmdObj.data[1] = ex.detailInfo();
				m_cmdObj.result =  ex.errorCode();
			}

			if ( m_cmdObj.send(m_cmdStreamIO) != 0)
			{
				TRACE(cpf_clientcmdTrace, "%s", "checkConnectionData(), failed to send data on dsd stream");
				result = false;
			}
		}
	}
	else
	{
		TRACE(cpf_clientcmdTrace, "%s", "checkConnectionData(), error on recv protocol values");
	}

	TRACE(cpf_clientcmdTrace, "%s", "Leaving checkConnectionData()");
	return result;
}

/*============================================================================
	ROUTINE: svc
 ============================================================================ */
int CPF_ClientCmd_Request::svc()
{
	TRACE(cpf_clientcmdTrace, "%s", "Entering svc()");
	int result = 0;
	acs_dsd::HANDLE dsdHandle;
	int dsdHandleCount = 1;

	const nfds_t nfds = 2;
	struct pollfd fds[nfds];
	ACE_INT32 ret;

	// Initialize the pollfd structure
	ACE_OS::memset(fds, 0 , sizeof(fds));

	m_cmdStreamIO.get_handles(&dsdHandle, dsdHandleCount);

	TRACE(cpf_clientcmdTrace, "svc, get <%i> handle from dsd stream", dsdHandleCount);

	fds[0].fd = m_StopEvent;
	fds[0].events = POLLIN;

	fds[1].fd = dsdHandle;
	fds[1].events = POLLIN;

	while(CONTINUE == m_ConState )
	{
		ret = ACE_OS::poll(fds, nfds);

		if( -1 == ret )
		{
			if(errno == EINTR)
			{
				continue;
			}
			TRACE(cpf_clientcmdTrace, "%s","Leaving svc, exit after poll error");
			break;
		}

		// Server shutdown request
		if(fds[0].revents & POLLIN)
		{
			// Received a stop request from server
			TRACE(cpf_clientcmdTrace, "%s","Leaving svc, received a stop request from server");
			m_ConState = EXIT;
			// exit from the while loop
			break;
		}

		// Data ready to read
		if(fds[1].revents & POLLIN)
		{
			// Received a stop request from server
			TRACE(cpf_clientcmdTrace, "%s","received a data on dsd stream");
			if( m_cmdObj.recv(m_cmdStreamIO) == 0)
			{
				m_ConState = (STATUS) process_AP_CmdRequest();
			}
			else
			{
				TRACE(cpf_clientcmdTrace, "%s", "svc(), error on recv cmd data on dsd stream");
				CPF_Log.Write("CPF_ClientCmd_Request::svc, error on recv cmd data on dsd stream", LOG_LEVEL_ERROR);
				m_ConState = EXIT;
			}
		}
	}

	TRACE(cpf_clientcmdTrace, "%s", "Leaving svc()");
	return result;
}

/*============================================================================
	ROUTINE: process_AP_CmdRequest
 ============================================================================ */
CPF_ClientCmd_Request::STATUS CPF_ClientCmd_Request::process_AP_CmdRequest()
{
	TRACE(cpf_clientcmdTrace, "%s", "Entering process_AP_CmdRequest()");
	STATUS result = CONTINUE;
	try
	{
		switch(m_cmdObj.cmdCode)
		{

			case CPF_API_Protocol::OPEN_ :
				{
					TRACE(cpf_clientcmdTrace, "%s", "process_AP_CmdRequest(), OPEN_ request");
					openFile();
					break;
				}

			case CPF_API_Protocol::CLOSE_ :
				{
					TRACE(cpf_clientcmdTrace, "%s", "process_AP_CmdRequest(), CLOSE_ request");
					closeFile();
					break;
				}

			case CPF_API_Protocol::EXISTS_ :
				{
					TRACE(cpf_clientcmdTrace, "%s", "process_AP_CmdRequest(), EXISTS_ request");
					exists();
					break;
				}

			case CPF_API_Protocol::GET_PATH_ :
				{
					TRACE(cpf_clientcmdTrace, "%s", "process_AP_CmdRequest(), GET_PATH_ request");
					getCPFilePath();
					break;
				}

			case CPF_API_Protocol::GET_FILEINFO_ :
				{
					TRACE(cpf_clientcmdTrace, "%s", "process_AP_CmdRequest(), GET_FILEINFO_ request");
					getFileInfo();
					break;
				}

			case CPF_API_Protocol::GET_USERS :
				{
					TRACE(cpf_clientcmdTrace, "%s", "process_AP_CmdRequest(), GET_USERS request");
					getSubFileInfo();
					break;
				}

			case CPF_API_Protocol::GET_SUBFILESLIST :
				{
					TRACE(cpf_clientcmdTrace, "%s", "process_AP_CmdRequest(), GET_USERS request");
					getInfiniteSubFilesList();
					break;
				}

			case CPF_API_Protocol::REMOVE_INFINITESUBFILE :
				{
					TRACE(cpf_clientcmdTrace, "%s", "process_AP_CmdRequest(), REMOVE_INFINITESUBFILE request");
					removeInfiniteSubFile();
					break;
				}

			case CPF_API_Protocol::EXIT_ :
				{
					TRACE(cpf_clientcmdTrace, "%s", "process_AP_CmdRequest(), EXIT_ request");
					result = EXIT;
					break;
				}
			default:

				TRACE(cpf_clientcmdTrace, "%s", "process_AP_CmdRequest(), Unknow request");
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PROTOCOLERROR, "Unknown function code");
		}
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		TRACE(cpf_clientcmdTrace, "%s", "process_AP_CmdRequest(), catched an exception");
		m_cmdObj.data[0] = ex.errorText();
		m_cmdObj.data[1] = ex.detailInfo();
		m_cmdObj.result =  ex.errorCode();
	}

	TRACE(cpf_clientcmdTrace, "%s", "process_AP_CmdRequest() send reply");
	// skip send on exit request
	if (result == CONTINUE && m_cmdObj.send(m_cmdStreamIO) != 0)
	{
		TRACE(cpf_clientcmdTrace, "%s", "process_AP_CmdRequest(), failed to send data on dsd stream");
		result = EXIT;
	}

	TRACE(cpf_clientcmdTrace, "%s", "Leaving process_AP_CmdRequest()");
	return result;
}

/*============================================================================
	ROUTINE: exists
 ============================================================================ */
void CPF_ClientCmd_Request::exists() throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_clientcmdTrace, "%s", "Entering exists()");

	FMS_CPF_FileId fileId(m_cmdObj.data[0]);

	std::string strCPName(m_cmdObj.data[1]);

    bool exists = DirectoryStructureMgr::instance()->exists(fileId, strCPName.c_str());

	m_cmdObj.data[0] = exists;
	m_cmdObj.result =  FMS_CPF_Exception::OK;

	TRACE(cpf_clientcmdTrace, "%s", "Leaving exists()");
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
void CPF_ClientCmd_Request::openFile() throw(FMS_CPF_PrivateException)
{
	TRACE(cpf_clientcmdTrace, "%s", "Entering openFile()");

	FileReference reference;
	FMS_CPF_FileId fileid( m_cmdObj.data[0] );
	unsigned long access = m_cmdObj.data[1];
	std::string	CpName(m_cmdObj.data[2]);

	reference = DirectoryStructureMgr::instance()->open(fileid, (FMS_CPF_Types::accessType)access, CpName.c_str());

	// insert here just to avoid an not closure of the file
	fileReferenceList_.push_back(make_pair(reference, CpName));

	TRACE(cpf_clientcmdTrace, "openFile(), file= %s opened", fileid.file().c_str());

	m_cmdObj.clear();
	m_cmdObj.data[0] = reference;
	m_cmdObj.result =  FMS_CPF_Exception::OK;

	TRACE(cpf_clientcmdTrace, "%s", "Leaving openFile()");
}

/*============================================================================
	ROUTINE: getFileInfo
 ============================================================================ */
void CPF_ClientCmd_Request::getFileInfo() throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_clientcmdTrace, "%s", "Entering getFileInfo()");

	FileReference reference;
	FMS_CPF_FileId fileid( m_cmdObj.data[0] );
	std::string	CpName(m_cmdObj.data[1]);

	TRACE(cpf_clientcmdTrace, "getFileInfo(), file=%s, cpName=%s", (fileid.file()).c_str(), CpName.c_str() );
	// reserve file data structure
	reference = DirectoryStructureMgr::instance()->open(fileid, FMS_CPF_Types::NONE_, CpName.c_str());

	m_cmdObj.clear();
	m_cmdObj.data[0] = (reference->getVolume()).c_str();
	m_cmdObj.result =  FMS_CPF_Exception::OK;

	// unreserve file data structure
	DirectoryStructureMgr::instance()->close(reference, CpName.c_str() );

	TRACE(cpf_clientcmdTrace, "%s", "Leaving getFileInfo()");
}

/*============================================================================
	ROUTINE: getSubFileInfo
 ============================================================================ */
void CPF_ClientCmd_Request::getSubFileInfo() throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_clientcmdTrace, "Entering in %s", __func__);

	FileReference reference;
	FMS_CPF_FileId fileid( m_cmdObj.data[0] );
	std::string	cpName(m_cmdObj.data[1]);

	TRACE(cpf_clientcmdTrace, "%s, file:<%s>, cpName:<%s>", __func__, (fileid.file()).c_str(), cpName.c_str() );
	// reserve file data structure
	reference = DirectoryStructureMgr::instance()->open(fileid, FMS_CPF_Types::NONE_, cpName.c_str());

	// Get the user structure with user access info
	FMS_CPF_Types::userType users = reference->getUsers();

	m_cmdObj.clear();
	m_cmdObj.result =  FMS_CPF_Exception::OK;

	// set num of readers on the file
	m_cmdObj.data[0] = (FMS_CPF_EXCLUSIVE != users.rcount) ? static_cast<int>(users.rcount) : 1;

	// set num of writers on the file
	m_cmdObj.data[1] = (FMS_CPF_EXCLUSIVE != users.wcount) ? static_cast<int>(users.wcount) : 1;

	// set exclusive access mode
	m_cmdObj.data[2] = ((FMS_CPF_EXCLUSIVE == users.wcount) || (FMS_CPF_EXCLUSIVE == users.rcount)) ? 1 : 0;

	// set the size of file in terms of records
	m_cmdObj.data[3] = static_cast<int>(reference->getSize());

	DirectoryStructureMgr::instance()->closeExceptionLess(reference, cpName.c_str());

	TRACE(cpf_clientcmdTrace, "Leaving %s", __func__);
}

/*============================================================================
	ROUTINE: getInfiniteSubFilesList
 ============================================================================ */
void CPF_ClientCmd_Request::getInfiniteSubFilesList() throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_clientcmdTrace, "Entering in %s", __func__);

	std::string	infiniteFileName(m_cmdObj.data[0]);
	std::string	cpName(m_cmdObj.data[1]);
	std::set<std::string> subFileList;

	TRACE(cpf_clientcmdTrace, "%s, get subfiles of file:<%s> CP:<%s> ", __func__, infiniteFileName.c_str(), cpName.c_str() );

	if( DirectoryStructureMgr::instance()->getListOfInfiniteSubFile(infiniteFileName, cpName, subFileList) )
	{
		m_cmdObj.clear();
		m_cmdObj.result =  FMS_CPF_Exception::OK;
		std::set<std::string>::const_iterator element;
		int numOfFile = subFileList.size();
		m_cmdObj.data[0] = numOfFile;
		int idx = 1;
		TRACE(cpf_clientcmdTrace, "%s, found <%d> subfiles", __func__, numOfFile);
		for(element = subFileList.begin(); element != subFileList.end(); ++element)
		{
			m_cmdObj.data[idx++] = (*element);
		}
	}
	else
	{
		m_cmdObj.clear();
		m_cmdObj.result =  FMS_CPF_Exception::GENERAL_FAULT;
		TRACE(cpf_clientcmdTrace, "%s, error on get subfiles list", __func__);
	}

	TRACE(cpf_clientcmdTrace, "Leaving %s", __func__);
}

/*============================================================================
	ROUTINE: removeInfiniteSubFile
 ============================================================================ */
void CPF_ClientCmd_Request::removeInfiniteSubFile() throw (FMS_CPF_PrivateException)
{
	TRACE(cpf_clientcmdTrace, "Entering in %s", __func__);

	FMS_CPF_FileId subFileid(m_cmdObj.data[0]);
	FMS_CPF_FileId mainFileid(subFileid.file());
	std::string	cpName(m_cmdObj.data[1]);

	TRACE(cpf_clientcmdTrace, "%s, remove subfile:<%s>, Cp:<%s>", __func__, subFileid.file().c_str(), cpName.c_str());
	FileReference mainFileReference = DirectoryStructureMgr::instance()->open(mainFileid, FMS_CPF_Types::NONE_, cpName.c_str());
	// Check if defined to transfer queue
	FMS_CPF_Attribute fileAttribute = mainFileReference->getAttribute();
	FMS_CPF_Types::transferMode TQMode;
	// Get the current transfer mode
	fileAttribute.getTQmode(TQMode);

	DirectoryStructureMgr::instance()->closeExceptionLess(mainFileReference, cpName.c_str() );

	if( FMS_CPF_Types::tm_NONE != TQMode )
	{
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILEISPROT, errorText::FileAttachedToTQ);
	}

	// Open logical subfile in exclusive access
	FileReference subFileReference = DirectoryStructureMgr::instance()->open(subFileid, FMS_CPF_Types::DELETE_, cpName.c_str());
	try
	{
		DirectoryStructureMgr::instance()->remove(subFileReference, cpName.c_str());
		subFileReference = FileReference::NOREFERENCE;
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		DirectoryStructureMgr::instance()->closeExceptionLess(subFileReference, cpName.c_str());
		char errorMsg[256] = {0};
		snprintf(errorMsg, 255, "%s, infinite subfile:<%s> delete fails, error:<%s>", __func__, subFileid.file().c_str(), ex.errorText());
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(cpf_clientcmdTrace, "%s", errorMsg);
		// re-trow the same exception
		throw;
	}

	m_cmdObj.clear();
	m_cmdObj.result =  FMS_CPF_Exception::OK;

	TRACE(cpf_clientcmdTrace, "Leaving %s", __func__);
}

/*============================================================================
	ROUTINE: getCPFilePath
 ============================================================================ */
void CPF_ClientCmd_Request::getCPFilePath() throw(FMS_CPF_PrivateException)
{
	TRACE(cpf_clientcmdTrace, "%s", "Entering in getCPFilePath()");

	unsigned long numRef =  static_cast<unsigned long>(m_cmdObj.data[0]);

	FileReference reference(numRef);

	std::string	filePath = reference->getPath();
	TRACE(cpf_clientcmdTrace, "getCPFilePath(), path=<%s>", filePath.c_str());

	m_cmdObj.clear();
	m_cmdObj.data[0] = filePath.c_str();
	m_cmdObj.result =  FMS_CPF_Exception::OK;

	TRACE(cpf_clientcmdTrace, "%s", "Leaving getCPFilePath()");
}

/*============================================================================
	ROUTINE: closeFile
 ============================================================================ */
void CPF_ClientCmd_Request::closeFile() throw (FMS_CPF_PrivateException)
{

	TRACE(cpf_clientcmdTrace, "%s", "Entering closeFile()");
	unsigned long numRef =  static_cast<unsigned long>(m_cmdObj.data[0]);

	FileReference reference(numRef);

	std::string	CpName( m_cmdObj.data[1] );

	// Close logical file
	DirectoryStructureMgr::instance()->close(reference, CpName.c_str() );
	fileReferenceList_.remove(make_pair(reference, CpName) );

	m_cmdObj.result =  FMS_CPF_Exception::OK;

	TRACE(cpf_clientcmdTrace, "%s", "Leaving closeFile()");
 }

/*============================================================================
	ROUTINE: ~CPF_ClientCmd_Request
 ============================================================================ */
CPF_ClientCmd_Request::~CPF_ClientCmd_Request()
{
	// Close DSD stream
	m_cmdStreamIO.close();

	std::list< std::pair<FileReference,std::string> >::iterator index;

	// Close references which were left open
	for(index = fileReferenceList_.begin(); index != fileReferenceList_.end() ; ++index )
	{
		try
		{
			DirectoryStructureMgr::instance()->close(index->first, (index->second).c_str());
		}
		catch(FMS_CPF_PrivateException& ex)
		{
			// just ship it
		}
	}

	fileReferenceList_.clear();

	if(NULL != cpf_clientcmdTrace)
			delete cpf_clientcmdTrace;
}
