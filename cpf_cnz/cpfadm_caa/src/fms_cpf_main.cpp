//
/** @file fms_cpf_main.cpp
 *	@brief
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-08
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
 *	| 1.0.0  | 2011-06-08 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

//#include <ace/Log_Msg.h>
//#include <ace/Handle_Set.h>

#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <syslog.h>

#include "fms_cpf_service.h"
#include "fms_cpf_server.h"
#include "fms_cpf_common.h"

/*===================================================================
                        PROTOTYPES DECLARATION SECTION
=================================================================== */
void sighandler(int signum);
int setupSignalHandler(const struct sigaction* sigAction);
void printusage();

FMS_CPF_Service* g_ptr_CPFAppl = 0;
FMS_CPF_Server* g_ptr_CPFServer = 0;
bool g_InteractiveMode = false;


/*=================================================================
	ROUTINE: Main
=================================================================== */

ACE_INT32 ACE_TMAIN( ACE_INT32 argc , ACE_TCHAR* argv[])
{
	ACE_INT32 result = 0;

	struct sigaction sa;
	sa.sa_handler = sighandler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask );

	// Open system event log
	openlog(FMS_CPF_DAEMON_NAME, LOG_PID, 0);
	syslog(LOG_INFO, "service main started");
	// set the signal handler for the main
	result = setupSignalHandler(&sa);

	if(0 == result )
	{
		if( argc > 1)
		{
			// fms_cpfserverd started by command line
			if( argc == 2 && ( 0 == strcmp(argv[1],"-d") ) )
			{
				g_InteractiveMode = true;

				std::cout << "fms_cpfserverd main..."<< std::endl;
				// check if there's another CPF Server instance running
				int fdlock = open(CPF_SERVER_LOCKFILE, O_CREAT | O_WRONLY | O_APPEND, 0664);
				if(fdlock < 0)
					return (ACE_INT32) -2;

				if(flock(fdlock, LOCK_EX | LOCK_NB) < 0)
				{
					close(fdlock);
					if(errno == EWOULDBLOCK)
					{
						std::cout << "Another CPF Server instance already running" << std::endl;
						return (ACE_INT32) 1;
					}
					return (ACE_INT32) -3;
				}

				g_ptr_CPFServer = new (std::nothrow) FMS_CPF_Server() ;

				if( NULL == g_ptr_CPFServer )
				{
					std::cout << "Memory allocated failed for FMS_CPF_Server" << std::endl;
					result = -1;
				}
				else
				{
					std::cout << "fms_cpfserverd started in debug mode..." << std::endl;

					g_ptr_CPFServer->startWorkerThreads();

					// wait 5s before start the wait on termination
					ACE_OS::sleep(5);

					std::cout << "fms_cpfserverd wait for termination..." << std::endl;
					g_ptr_CPFServer->waitOnShotdown();

					std::cout << "fms_cpfserverd stopped in debug mode..." << std::endl;
					delete g_ptr_CPFServer;

					// delete lock file
					if(unlink(CPF_SERVER_LOCKFILE) < 0)
					{
						std::cout << "fms_cpfserverd unlock file error " << std::endl;
					}

					close(fdlock);
				}
			}
			else
			{
				printusage();
				result = -1;
			}
		}
		else
		{
			g_InteractiveMode = false;

			ACS_APGCC_HA_ReturnType errorCode = ACS_APGCC_HA_SUCCESS;

			g_ptr_CPFAppl = new (std::nothrow) FMS_CPF_Service(FMS_CPF_DAEMON_NAME, FMS_CPF_DAEMON_USER);

			if( NULL != g_ptr_CPFAppl )
			{
				errorCode = g_ptr_CPFAppl->activate();

				if (errorCode == ACS_APGCC_HA_FAILURE)
				{
					syslog(LOG_INFO, "HA Activation Failed for fms_cpfd");
					delete g_ptr_CPFAppl;
					result = -1;
				}
				else if( errorCode == ACS_APGCC_HA_FAILURE_CLOSE)
				{
					syslog(LOG_INFO, "HA Application Failed to close gracefully for fms_cpfd");

					delete g_ptr_CPFAppl;
					result = -1;
				}
				else if (errorCode == ACS_APGCC_HA_SUCCESS)
				{
					syslog(LOG_INFO, "HA Application Gracefully close for fms_cpfd");
					delete g_ptr_CPFAppl;
				}
				else
				{
					syslog(LOG_INFO, "error occurred while starting fms_cpfd with HA");

					delete g_ptr_CPFAppl;
					result = -1;
				}
			}
			else
			{
				syslog(LOG_INFO, "unable to allocate memory for FMS_CPF_Service object");

				result = -1;
			}
		}
	}

	syslog(LOG_INFO, "main program terminated.");
	// Close system event log
	closelog();
	return result;
} //End of MAIN

/*==========================================================================
	ROUTINE: sighandler
========================================================================== */
void sighandler(int signum)
{
	if( signum == SIGTERM || signum == SIGINT || signum == SIGTSTP )
	{
		if(!g_InteractiveMode)
		{
			if(g_ptr_CPFAppl != 0)
			{
				g_ptr_CPFAppl->performStateTransitionToQuiescedJobs(ACS_APGCC_AMF_HA_UNDEFINED);
			}
		}
		else
		{
			if(g_ptr_CPFServer != 0)
			{
				g_ptr_CPFServer->stopWorkerThreads();
			}
		}
	}
	else
	{
		if( signum == SIGPIPE )
		{
			syslog(LOG_INFO, "Handled SIGPIPE in fms_cpfd");
		}
	}
}
/*===============================================================
	ROUTINE: setupSignalHandler()
================================================================ */
int setupSignalHandler(const struct sigaction* sigAction)
{
	if( sigaction(SIGINT, sigAction, NULL ) == -1)
	{
		syslog(LOG_INFO, "Error occurred while handling SIGINT in fms_cpfd");
		return -1;
	}

	if( sigaction(SIGTERM, sigAction, NULL ) == -1)
	{
		syslog(LOG_INFO, "Error occurred while handling SIGTERM in fms_cpfd");
		return -1;
	}

	if( sigaction(SIGTSTP, sigAction, NULL ) == -1)
	{
		syslog(LOG_INFO, "Error occurred while handling SIGTSTP in fms_cpfd");
		return -1;
	}

	if( sigaction(SIGPIPE, sigAction, NULL ) == -1)
	{
		syslog(LOG_INFO, "Error occurred while handling SIGPIPE in fms_cpfd");
		return -1;
	}
    return 0;
}

/*===============================================================
	ROUTINE: printusage()
================================================================ */
void printusage()
{
	std::cout<<"Usage:\nfms_cpfd -d for debug mode."<< std::endl;
}
