//
/** @file fms_cpf_service.h
 *	@brief
 *	Header file for FMS_CPF_Service class.
 *  This module contains the declaration of the class FMS_CPF_Service.
 *
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

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_Service_H
#define FMS_CPF_Service_H

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "ACS_APGCC_ApplicationManager.h"

#include <ace/Task.h>

class FMS_CPF_Server;
class ACS_TRA_trace;

/*===================================================================
                        CLASS DECLARATION SECTION
=================================================================== */
class  FMS_CPF_Service : public ACS_APGCC_ApplicationManager
{
	private:

		enum WORKER_THREAD_STATE{
				STOPPED = 0,
				RUNNING = 1,
				NOTSTARTED = 2
		};

		enum{
			READ_PIPE = 0,
			WRITE_PIPE = 1
		};

		/**
			@brief	svc_run
		*/
	    bool svc_run;

		/**
			@brief	readWritePipe
		*/
		int readWritePipe[2];

		/**
			@brief	workerThreadState
		*/
		WORKER_THREAD_STATE workerThreadState;

		/**
			@brief	isWorkerThreadOn
		*/
		bool isWorkerThreadOn;

		/**
			@brief	applicationThreadId
		*/
		ACE_thread_t applicationThreadId;

		/**
			@brief	fms_cpf_serviceTrace
		*/
		ACS_TRA_trace* fms_cpf_serviceTrace;

		/**
			@brief	fms_cpf_serviceTrace
		*/
		FMS_CPF_Server* cpfServer;

		/**
			@brief	This method signal to the worker thread to terminate
		*/
		ACS_APGCC_ReturnType stopWorkerThread();

   public:

		/**
			@brief		constructor of FMS_CPF_Service class

			@param		daemon_name

			@param		username
		*/
		FMS_CPF_Service(const char* daemon_name, const char* username);

		/**
			@brief		destructor of FMS_CPF_Service class

		*/
		virtual ~FMS_CPF_Service();

		/**
			@brief		This method implement the main service thread

			@return		ACE_THR_FUNC_RETURN

			@exception	none
		*/
		static ACE_THR_FUNC_RETURN cpf_service_thread(void* ptrParam);

		/**
			@brief		This method is the application worker thread

			@return		ACS_APGCC_ReturnType

			@exception	none
		*/
		ACS_APGCC_ReturnType cpf_svc();

		/**
			@brief		isWorkerThreadOn

			@return 	True if the Worker thread state is stopped , otherwise False

			@exception	none
		*/
		bool isWorkerThreadOff() { return (STOPPED == workerThreadState); };

		/**
			@brief		setWorkerThreadState

			@return 	Set Worker thread state

			@exception	none
		*/
		void setWorkerThreadState(WORKER_THREAD_STATE newState) { workerThreadState = newState; } ;

		/**
			@brief		performStateTransitionToActiveJobs

			@param		previousHAState

			@return		ACS_APGCC_ReturnType

			@exception	none
		*/
		ACS_APGCC_ReturnType performStateTransitionToActiveJobs(ACS_APGCC_AMF_HA_StateT previousHAState);

		/**
			@brief		performStateTransitionToPassiveJobs

			@param		previousHAState

			@return 	ACS_APGCC_ReturnType

			@exception	none
		*/
		ACS_APGCC_ReturnType performStateTransitionToPassiveJobs(ACS_APGCC_AMF_HA_StateT previousHAState);

		/**
			@brief		performStateTransitionToQueisingJobs

			@param		previousHAState

			@return 	ACS_APGCC_ReturnType

			@exception	none
		*/
		ACS_APGCC_ReturnType performStateTransitionToQueisingJobs(ACS_APGCC_AMF_HA_StateT previousHAState);

		/**
			@brief		performStateTransitionToQueiscedJobs

			@param		previousHAState

			@return		ACS_APGCC_ReturnType

			@exception	none

		*/
		ACS_APGCC_ReturnType performStateTransitionToQuiescedJobs(ACS_APGCC_AMF_HA_StateT previousHAState);

		/**

			@brief		performComponentHealthCheck

			@return		ACS_APGCC_ReturnType

			@exception	none

		*/
		ACS_APGCC_ReturnType performComponentHealthCheck(void);

		/**
			@brief		performComponentTerminateJobs

			@return 	ACS_APGCC_ReturnType

			@exception	none
		*/
		ACS_APGCC_ReturnType performComponentTerminateJobs(void);

		/**
			@brief		performComponentRemoveJobs

			@return		ACS_APGCC_ReturnType

			@exception	none

		*/
		ACS_APGCC_ReturnType performComponentRemoveJobs (void);

		/**
			@brief		performApplicationShutdownJobs

			@return		ACS_APGCC_ReturnType

			@exception	none
		*/
		ACS_APGCC_ReturnType performApplicationShutdownJobs(void);

};
#endif /* FMS_CPF_Service_H */

