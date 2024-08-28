/*
 * fms_cpf_oc_buffermgr.cpp
 *
 *  Created on: Nov 14, 2011
 *      Author: enungai
 */

#include "fms_cpf_oc_buffermgr.h"
#include "fms_cpf_message.h"
#include "fms_cpf_ex_ocbuffer.h"
#include "fms_cpf_sb_ocbuffer.h"
#include "fms_cpf_cpmsg.h"
#include "fms_cpf_common.h"
#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;


/*============================================================================
	ROUTINE: FMS_CPF_OC_BufferMgr
 ============================================================================ */
FMS_CPF_OC_BufferMgr::FMS_CPF_OC_BufferMgr()
{
	fms_cpf_ocBuffMgrTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_OC_BufferMgr");
}

/*============================================================================
	ROUTINE: ~FMS_CPF_OC_BufferMgr
 ============================================================================ */
FMS_CPF_OC_BufferMgr::~FMS_CPF_OC_BufferMgr()
{
	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "destructor() Begin");

    //Send the close msg to all exBuffers
	ExOcBuffer* ocBuffer = NULL;

	ACE_Guard<ACE_Thread_Mutex> guard(m_exBuffersLock);

	ocMapType::iterator it1;

	for (it1 = ocTableMap.begin(); it1 != ocTableMap.end(); it1++) {
		ocBuffer = (*it1).second; // get pointer
		delete ocBuffer;
	}

    //Send the close msg to all sbBuffers
	SbOcBuffer* sbocBuffer = NULL;

	ACE_Guard<ACE_Thread_Mutex> guard1(m_sbBuffersLock);

	sbocMapType::iterator it2;

	for (it2 = sbocTableMap.begin(); it2 != sbocTableMap.end(); it2++) {
		sbocBuffer = (*it2).second; // get pointer
		delete sbocBuffer;
	}
	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "destructor() End");

	if(NULL != fms_cpf_ocBuffMgrTrace)
			delete fms_cpf_ocBuffMgrTrace;
}

void FMS_CPF_OC_BufferMgr::removeCPBuffers()
{
	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "Entering in removeCPBuffers()");
	{
		ACE_Guard<ACE_Thread_Mutex> guard(m_exBuffersLock);
		TRACE(fms_cpf_ocBuffMgrTrace, "%s", "removeCPBuffers(), remove CP Ex buffers");
		ocMapType::iterator cpExBuffer;

		for (cpExBuffer = ocTableMap.begin(); cpExBuffer != ocTableMap.end(); ++cpExBuffer)
		{
			delete (*cpExBuffer).second;
		}
		ocTableMap.clear();
	}

	{
		ACE_Guard<ACE_Thread_Mutex> guard1(m_sbBuffersLock);
		TRACE(fms_cpf_ocBuffMgrTrace, "%s", "removeCPBuffers(), remove CP Sb buffers");
		sbocMapType::iterator cpSbBuffer;

		for (cpSbBuffer = sbocTableMap.begin(); cpSbBuffer != sbocTableMap.end(); ++cpSbBuffer)
		{
			delete (*cpSbBuffer).second;;
		}
		sbocTableMap.clear();
	}
	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "Leaving removeCPBuffers()");
}

/*============================================================================
	ROUTINE: getOCBufferForCP
 ============================================================================ */
ExOcBuffer* FMS_CPF_OC_BufferMgr::getOCBufferForCP(std::string cpname)
{
	TRACE(fms_cpf_ocBuffMgrTrace, "Entering in getOCBufferForCP(%s)", cpname.c_str());
	if(cpname.empty())
		cpname = DEFAULT_CPNAME;

	ExOcBuffer* ocBuffer = NULL;

	ACE_Guard<ACE_Thread_Mutex> guard(m_exBuffersLock);

	// Search for cpname in table
	ocMapType::iterator it = ocTableMap.find(cpname);

	if( it != ocTableMap.end() )
	{
		ocBuffer = (*it).second; // get pointer
	}
	else
	{
		ocBuffer = new (std::nothrow) ExOcBuffer(cpname);
		if( NULL != ocBuffer )
		{
			if ( FAILURE == ocBuffer->open() )
			{
				delete ocBuffer;
				ocBuffer = NULL;
			}
			else
			{
				std::pair<ocMapType::iterator,bool> ret;
				ret = ocTableMap.insert(ocMapType::value_type(cpname, ocBuffer));
			}
		}
	}
	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "Leaving getOCBufferForCP()");
	return ocBuffer;
}

/*============================================================================
	ROUTINE: getSBOCBufferForCP
 ============================================================================ */
SbOcBuffer* FMS_CPF_OC_BufferMgr::getSBOCBufferForCP(std::string cpname)
{
	TRACE(fms_cpf_ocBuffMgrTrace, "Entering in getSBOCBufferForCP(%s)", cpname.c_str() );

	if (cpname.empty())
		cpname = DEFAULT_CPNAME;

	SbOcBuffer* sbocBuffer = NULL;

	ACE_Guard<ACE_Thread_Mutex> guard(m_sbBuffersLock);

	// Search for cpname in table
	sbocMapType::iterator it = sbocTableMap.find(cpname);

	if ( it != sbocTableMap.end() )
	{
		// get pointer
		sbocBuffer = (*it).second;
	}
	else
	{
		sbocBuffer = new (std::nothrow) SbOcBuffer(cpname);
		if (NULL != sbocBuffer)
		{

			if ( FAILURE == sbocBuffer->open() )
			{
				delete sbocBuffer;
				sbocBuffer = NULL;
			}
			else
			{
				std::pair<sbocMapType::iterator,bool> ret;
				ret = sbocTableMap.insert(sbocMapType::value_type(cpname, sbocBuffer));
			}
		}
	}
	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "Leaving getSBOCBufferForCP()");
	return sbocBuffer;
}

/*============================================================================
	ROUTINE: pushInOCBuf
 ============================================================================ */
void FMS_CPF_OC_BufferMgr::pushInOCBuf(std::string cpname, FMS_CPF_CPMsg *cpmsg)
{
	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "Entering in pushInOCBuf()");

	ExOcBuffer* myOCBuffer = getOCBufferForCP(cpname);
	if( myOCBuffer != NULL)
	{
		// Insert sync message in the buffer queue
		if(FAILURE == myOCBuffer->putq(cpmsg) )
		{
			cpmsg->release();
			TRACE(fms_cpf_ocBuffMgrTrace, "pushInOCBuf(), CP:<%s> error on put a message", cpname.c_str());
			CPF_Log.Write("pushInOCBuf() error on put a message");
		}
	 }
	 else
	 {
		cpmsg->release();
		TRACE(fms_cpf_ocBuffMgrTrace, "pushInOCBuf()  cpname= %s; error in the buffer creation ", cpname.c_str());
		CPF_Log.Write("pushInOCBuf(), error in the OC buffer creation");
	 }
	 TRACE(fms_cpf_ocBuffMgrTrace, "%s", "Leaving pushInOCBuf()");
}

/*============================================================================
	ROUTINE: pushInSBOCBuf
 ============================================================================ */
void FMS_CPF_OC_BufferMgr::pushInSBOCBuf(std::string cpname, FMS_CPF_CPMsg *cpmsg)
{
	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "Entering in pushInSBOCBuf()");

	SbOcBuffer* mySBOCBuffer = getSBOCBufferForCP(cpname);
	if( mySBOCBuffer != NULL)
	{
		// Insert sync message in the buffer queue
		if( FAILURE == mySBOCBuffer->putq(cpmsg) )
		{
			cpmsg->release();
			TRACE(fms_cpf_ocBuffMgrTrace, "pushInSBOCBuf(), CP:<%s> error on put a message", cpname.c_str());
			CPF_Log.Write("pushInSBOCBuf(), error on put a message");
		}
	}
	else
	{
		cpmsg->release();
		TRACE(fms_cpf_ocBuffMgrTrace, "pushInSBOCBuf()  cpname= %s; error in the buffer creation ", cpname.c_str());
		CPF_Log.Write("pushInSBOCBuf(), error in the SB OC buffer creation");
	}
	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "Leaving pushInSBOCBuf()");
}

/*============================================================================
	ROUTINE: OCBsync
 ============================================================================ */
void FMS_CPF_OC_BufferMgr::OCBsync( std::string cpname)
{
	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "Entering in OCBsync()");

	ExOcBuffer* myOCBuffer = getOCBufferForCP(cpname);

	if( myOCBuffer != NULL)
	{
		CPF_Sync_Msg* syncMsg = new CPF_Sync_Msg();
		// Insert sync message in the buffer queue
		if( FAILURE == myOCBuffer->putq(syncMsg) )
		{
			syncMsg->release();
			TRACE(fms_cpf_ocBuffMgrTrace, "OCBsync(), CP:<%s> error on put the SYNC message", cpname.c_str());
			CPF_Log.Write("OCBsync(), error on put the SYNC message");
		}
	}
	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "Leaving OCBsync()");
}

/*============================================================================
	ROUTINE: SBOCBsync
 ============================================================================ */
void FMS_CPF_OC_BufferMgr::SBOCBsync(std::string cpname)
{
	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "Entering in SBOCBsync()");

	SbOcBuffer* mySBOCBuffer = getSBOCBufferForCP(cpname);
	if( NULL != mySBOCBuffer )
	{
		CPF_Sync_Msg* syncMsg = new CPF_Sync_Msg();
		// Insert sync message in the buffer queue
		if( FAILURE == mySBOCBuffer->putq(syncMsg) )
		{
			syncMsg->release();
			TRACE(fms_cpf_ocBuffMgrTrace, "SBOCBsync(), CP:<%s> error on put the SYNC message", cpname.c_str());
			CPF_Log.Write("SBOCBsync(), error on put the SYNC message");
		}
	}

	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "Leaving SBOCBsync()");
}

/*============================================================================
	ROUTINE: clearFileRefs
 ============================================================================ */
void FMS_CPF_OC_BufferMgr::clearFileRefs(std::string cpname, bool cpSide)
{
	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "Entering in clearFileRefs()");
	CPF_ClearFileRef_Msg* clearRefMsg = NULL;
	if(cpSide)
	{
		TRACE(fms_cpf_ocBuffMgrTrace, "clearFileRefs(), clear file ref msg from CP:<%s> SB side", cpname.c_str());

		SbOcBuffer* myBuffer = getSBOCBufferForCP(cpname);

		if( NULL != myBuffer )
		{
			clearRefMsg = new CPF_ClearFileRef_Msg();

			// Insert sync message in the buffer queue
			if( FAILURE == myBuffer->putq(clearRefMsg) )
			{
				clearRefMsg->release();
				TRACE(fms_cpf_ocBuffMgrTrace, "clearFileRefs()  cpname= %s; error on put the close message", cpname.c_str());
				CPF_Log.Write("clearFileRefs() error on put the close message");
			}
		 }
	 }
	 else
	 {
		 TRACE(fms_cpf_ocBuffMgrTrace, "clearFileRefs(), clear file ref msg from CP:<%s> EX side", cpname.c_str());

		 ExOcBuffer* myBuffer = getOCBufferForCP(cpname);
		 if( NULL != myBuffer )
		 {
			 CPF_ClearFileRef_Msg* clearRefMsg = new CPF_ClearFileRef_Msg();
			 // Insert sync message in the buffer queue
			 if(FAILURE == myBuffer->putq(clearRefMsg) )
			 {
				 clearRefMsg->release();
				 TRACE(fms_cpf_ocBuffMgrTrace, "clearFileRefs()  cpname= %s; error on put the close message", cpname.c_str());
				 CPF_Log.Write("clearFileRefs() error on put the close message");
			 }
		 }
	 }
	 TRACE(fms_cpf_ocBuffMgrTrace, "%s", "Leaving clearFileRefs()");
}

/*============================================================================
	ROUTINE: shutdown
 ============================================================================ */
void FMS_CPF_OC_BufferMgr::shutdown()
{
	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "shutdown() Begin");

	CPF_Close_Msg* closeMsg;
    //Send the close msg to all exBuffers
	ExOcBuffer* ocBuffer = NULL;
	{
		ACE_Guard<ACE_Thread_Mutex> guard1(m_exBuffersLock);

		ocMapType::iterator it1;

		for(it1 = ocTableMap.begin(); it1 != ocTableMap.end(); ++it1)
		{
			closeMsg = new CPF_Close_Msg();
			ocBuffer = (*it1).second; // get pointer

			if( FAILURE != ocBuffer->putq(closeMsg) )
			{
				TRACE(fms_cpf_ocBuffMgrTrace,"%s","shutdown(), wait for svc termination");
				ocBuffer->wait();
				TRACE(fms_cpf_ocBuffMgrTrace,"%s","shutdown(), svc has been terminated");
			}
			else
			{
				closeMsg->release();
				TRACE(fms_cpf_ocBuffMgrTrace,"%s","shutdown(), error on put the close message");
				CPF_Log.Write("FMS_CPF_Oc_BufferMgr::ShutDown, error on put the close message");
			}
		}
	}
	{
		ACE_Guard<ACE_Thread_Mutex> guard2(m_sbBuffersLock);
		//Send the close msg to all sbBuffers
		SbOcBuffer* sbocBuffer = NULL;
		sbocMapType::iterator it2;

		for (it2 = sbocTableMap.begin(); it2 != sbocTableMap.end(); ++it2) {
			closeMsg = new CPF_Close_Msg();
			sbocBuffer = (*it2).second; // get pointer

			if( FAILURE != sbocBuffer->putq(closeMsg) )
			{
				TRACE(fms_cpf_ocBuffMgrTrace,"%s","shutdown(), wait for svc termination");
				sbocBuffer->wait();
				TRACE(fms_cpf_ocBuffMgrTrace,"%s","shutdown(), svc has been terminated");
			}
			else
			{
				closeMsg->release();
				TRACE(fms_cpf_ocBuffMgrTrace,"%s","shutdown(), error on put the close message");
				CPF_Log.Write("FMS_CPF_Oc_BufferMgr::ShutDown, error on put the close message");
			}
		}
	}

	removeCPBuffers();

	TRACE(fms_cpf_ocBuffMgrTrace, "%s", "shutdown() End");
}
