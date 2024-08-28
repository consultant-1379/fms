/*
 * @file fms_cpf_oc_buffermgr.h
 *	@brief
 *	Header file for FMS_CPF_OC_BufferMgr class.
 *  This module contains the declaration of the class FMS_CPF_OC_BufferMgr.
 *  Created on: Nov 14, 2011
 *      Author: enungai
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
 *	| 1.0.0  | 2011-11-14 | enungai      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_OC_BUFFERMGR_H_
#define FMS_CPF_OC_BUFFERMGR_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include <ace/Singleton.h>

#include <map>
#include <string>
#include <ace/Thread_Mutex.h>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class ACS_TRA_trace;
class ExOcBuffer;
class SbOcBuffer;
class FMS_CPF_CPMsg;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FMS_CPF_OC_BufferMgr
{
	typedef std::map<std::string, ExOcBuffer*> ocMapType;
	typedef std::map<std::string, SbOcBuffer*> sbocMapType;

    private:
		/**
		 * 	@brief	Construct of the FMS_CPF_OC_BufferMgr class
		*/
		FMS_CPF_OC_BufferMgr();

		/**
		 * 	@brief	Destructor of the FMS_CPF_OC_BufferMgr class
		*/
		virtual ~FMS_CPF_OC_BufferMgr();

		/** @brief removeCPBuffers method
		 *
		 *	This method removes all CP buffers
		 *
		*/
		void removeCPBuffers();

		//It holds OCBuffer instance per cpnmame
		ocMapType ocTableMap;

		//It holds SBOCBuffer instance per cpname
		sbocMapType	sbocTableMap;

		/**
		 * 	@brief	fms_cpf_ocBuffMgrTrace: trace object
		*/
		ACS_TRA_trace* fms_cpf_ocBuffMgrTrace;

		ACE_Thread_Mutex m_exBuffersLock;

		ACE_Thread_Mutex m_sbBuffersLock;

    public:

		friend class ACE_Singleton<FMS_CPF_OC_BufferMgr, ACE_Recursive_Thread_Mutex>;

		/** @brief getOCBufferForCP method
		 *
		 *	This method returns a OCBuffer for the EX side given the CP name
		 *
		 *	@param cpName : the CP name
		 *
		*/
		ExOcBuffer* getOCBufferForCP(std::string cpname);

		/** @brief getSBOCBufferForCP method
		 *
		 *	This method returns a OCBuffer for the SB side given the CP name
		 *
		 *	@param cpName : the CP name
		 *
		*/
		SbOcBuffer* getSBOCBufferForCP(std::string cpname);

		/** @brief pushInOCBuf method
		 *
		 *	This method inserts a CP message in the OCBuffer related to the CP name
		 *
		 *	@param cpName : the CP name
		 *	@param cpmsg  : message from the CP
		*/
		void pushInOCBuf(std::string cpname, FMS_CPF_CPMsg *cpmsg);


		/** @brief pushInSBOCBuf method
		 *
		 *	This method inserts a CP message in the SB OCBuffer related to the CP name
		 *
		 *	@param cpName : the CP name
		 *	@param cpmsg  : message from the CP
		 *
		*/
		void pushInSBOCBuf(std::string cpname, FMS_CPF_CPMsg *cpmsg);


		/** @brief OCBsync method
		 *
		 *	This method sends a sync message to the OCBuffer related to the CP name
		 *
		 *	@param cpName : the CP name
		 *
		*/
		void OCBsync(std::string cpname);


		/** @brief SBOCBsync method
		 *
		 *	This method sends a sync message to the SB OCBuffer related to the CP name
		 *
		 *	@param cpName : the CP name
		 *
		*/
		void SBOCBsync(std::string cpname);


		/** @brief clearFileRefs method
		 *
		 *	This method sends clears all the file references associated to the CP name
		 *
		 *	@param cpName : the CP name
		 *
		 *	@param cpSide : the CP side
		 *
		*/
		void clearFileRefs(std::string cpname, bool cpSide);


		/** @brief shutdown method
		 *
		 *	This method performs the shutdown closing all the OC Buffers
		 *
		*/
		void shutdown();

};

typedef ACE_Singleton <FMS_CPF_OC_BufferMgr, ACE_Recursive_Thread_Mutex> OcBufferMgr;
#endif /* FMS_CPF_OC_BUFFERMGR_H_ */
