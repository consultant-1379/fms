#include <winsock2.h>
#include <windows.h>
#include ".\OC_SBOC_BufferMgr.h"
#include "commondll.h"
#include <stdio.h>

ACS_TRA_trace oc_sboc_buffer_mgr = ACS_TRA_DEF("oc_sboc_buffer_mgr" ,"C400");

// CONST
const string OC_SBOC_BufferMgr::DEFAULT_CPNAME("default_cpname");

OSF_Recursive_Thread_Mutex OC_SBOC_BufferMgr::singletonSynch;
//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------

OC_SBOC_BufferMgr* OC_SBOC_BufferMgr::instance_ = 0;

OC_SBOC_BufferMgr::OC_SBOC_BufferMgr()
	throw ( FMS_CPF_PrivateException)
{
	if (ACS_TRA_ON(oc_sboc_buffer_mgr))
	{
		char tracep[200] = {0};
		_snprintf(tracep, sizeof(tracep)-1, 
			"OC_SBOC_BufferMgr::OC_SBOC_BufferMgr() creating instance");
		ACS_TRA_event(&oc_sboc_buffer_mgr, tracep);
	}
	
	ocTableMap = new ocMapType();
	sbocTableMap = new sbocMapType();
	if (ACS_TRA_ON(oc_sboc_buffer_mgr))
	{
		char tracep[200] = {0};
		_snprintf(tracep, sizeof(tracep)-1, 
		"OC_SBOC_BufferMgr::Constructor() before Initialize Critical Section");
		ACS_TRA_event(&oc_sboc_buffer_mgr, tracep);
	}		
	InitializeCriticalSection(&handle_concurr_access_OCBuff);
	InitializeCriticalSection(&handle_concurr_access_SBOCBuff);
		if (ACS_TRA_ON(oc_sboc_buffer_mgr))
	{
		char tracep[200] = {0};
		_snprintf(tracep, sizeof(tracep)-1, 
		"OC_SBOC_BufferMgr::Constructor() after Initialize Critical Section");
		ACS_TRA_event(&oc_sboc_buffer_mgr, tracep);
	}	
}


//------------------------------------------------------------------------------
//      Instantiate a single object
//------------------------------------------------------------------------------

OC_SBOC_BufferMgr* 
OC_SBOC_BufferMgr::instance () 
{
  if (!instance_)
  {
	  try 
	  {
		if (ACS_TRA_ON(oc_sboc_buffer_mgr))
		{
			char tracep[200] = {0};
			_snprintf(tracep, sizeof(tracep)-1, 
				"OC_SBOC_BufferMgr::instance() before guard");
			ACS_TRA_event(&oc_sboc_buffer_mgr, tracep);
		}		
		OSF_Guard<OSF_Recursive_Thread_Mutex> guard(singletonSynch);
		if (ACS_TRA_ON(oc_sboc_buffer_mgr))
		{
			char tracep[200] = {0};
			_snprintf(tracep, sizeof(tracep)-1, 
				"OC_SBOC_BufferMgr::instance() after guard");
			ACS_TRA_event(&oc_sboc_buffer_mgr, tracep);
		}		
		if (!instance_)
			{
				if (ACS_TRA_ON(oc_sboc_buffer_mgr))
				{
					char tracep[200] = {0};
					_snprintf(tracep, sizeof(tracep)-1, 
						"OC_SBOC_BufferMgr::instance() before creation");
					ACS_TRA_event(&oc_sboc_buffer_mgr, tracep);
				}		
				instance_ = new OC_SBOC_BufferMgr();

				if (ACS_TRA_ON(oc_sboc_buffer_mgr))
				{
					char tracep[200] = {0};
					_snprintf(tracep, sizeof(tracep)-1, 
						"OC_SBOC_BufferMgr::instance() after creation");
					ACS_TRA_event(&oc_sboc_buffer_mgr, tracep);
				}		

			}
	  }
	  catch(...)
	  {
		  return NULL;
	  }
  }
  		if (ACS_TRA_ON(oc_sboc_buffer_mgr))
		{
			char tracep[200] = {0};
			_snprintf(tracep, sizeof(tracep)-1, 
				"OC_SBOC_BufferMgr::instance() before return instance %d",(!instance_));
			ACS_TRA_event(&oc_sboc_buffer_mgr, tracep);
		}	
  return instance_;
}

OCBuffer* OC_SBOC_BufferMgr::getOCBufferForCP(const char* _cpname)
{
	string cpname;
	if (_cpname == NULL || (string (_cpname)).empty())
		cpname = OC_SBOC_BufferMgr::DEFAULT_CPNAME;
	else
		cpname = _cpname;


	OCBuffer* ocBuffer = NULL;

	if (ACS_TRA_ON(oc_sboc_buffer_mgr))
	{
		char tracep[200] = {0};
		_snprintf(tracep, sizeof(tracep)-1,"%s %s %s", 
			"OCSBOC_BufferMgr::getOCBufferForCP() cpname:",
			cpname.c_str(),
			" before enter in critical section");
		ACS_TRA_event(&oc_sboc_buffer_mgr, tracep);
	}

	EnterCriticalSection(&handle_concurr_access_OCBuff);

	if (ACS_TRA_ON(oc_sboc_buffer_mgr))
	{
		char tracep[200] = {0};
		_snprintf(tracep, sizeof(tracep)-1,"%s %s %s", 
			"OCSBOC_BufferMgr::getOCBufferForCP() cpname:",
			cpname.c_str(),
			" after enter in critical section");
		ACS_TRA_event(&oc_sboc_buffer_mgr, tracep);
	}	
	ocMapType::iterator it = ocTableMap->find(cpname); // Search for cpname in table

	if ( it != ocTableMap->end() )
	{
		ocBuffer = (*it).second; // get pointer
	}
	else
	{
			pair<ocMapType::iterator,bool> ret;
			ocBuffer = new OCBuffer();
			ret = ocTableMap->insert(ocMapType::value_type(cpname, ocBuffer));
	}
	LeaveCriticalSection(&handle_concurr_access_OCBuff);

	return ocBuffer;
//return NULL;
}

SBOCBuffer*  OC_SBOC_BufferMgr::getSBOCBufferForCP(const char* _cpname)
{
	string cpname;
	if (_cpname == NULL)
		cpname = DEFAULT_CPNAME;
	else if ((string (_cpname)).empty())
		cpname = DEFAULT_CPNAME;
	else
		cpname = _cpname;

	SBOCBuffer* sbocBuffer = NULL;
	
	EnterCriticalSection(&handle_concurr_access_SBOCBuff);
	sbocMapType::iterator it = sbocTableMap->find(cpname); // Search for cpname in table

	if ( it != sbocTableMap->end() )
	{
		sbocBuffer = (*it).second; // get pointer
	}
	else
	{
			pair<sbocMapType::iterator,bool> ret;
			sbocBuffer = new SBOCBuffer();
			ret = sbocTableMap->insert(sbocMapType::value_type(cpname, sbocBuffer));
	}
	LeaveCriticalSection(&handle_concurr_access_SBOCBuff);

	return sbocBuffer;
}


void 
OC_SBOC_BufferMgr::pushInOCBuf(const char* _cpname, CPMsg *cpmsg)
{
	 OCBuffer* myOCBuffer;

	 myOCBuffer = getOCBufferForCP( _cpname);
	 if( myOCBuffer != NULL)
		myOCBuffer->pushMsg(cpmsg);
}

void 
OC_SBOC_BufferMgr::pushInSBOCBuf(const char* _cpname, CPMsg *cpmsg)
{
	 SBOCBuffer* mySBOCBuffer;

	 mySBOCBuffer = getSBOCBufferForCP( _cpname);
	 if( mySBOCBuffer != NULL)
		mySBOCBuffer->pushMsg(cpmsg);
}

void OC_SBOC_BufferMgr::OCBsync( const char* _cpname)
{
	 OCBuffer* myOCBuffer;

	 myOCBuffer = getOCBufferForCP( _cpname);
	 if( myOCBuffer != NULL)
		myOCBuffer->sync();
}

void OC_SBOC_BufferMgr::SBOCBsync( const char* _cpname)
{
	 SBOCBuffer* mySBOCBuffer;

	 mySBOCBuffer = getSBOCBufferForCP( _cpname);
	 if( mySBOCBuffer != NULL)
		mySBOCBuffer->sync();
}

void OC_SBOC_BufferMgr::clearFileRefs(const char* _cpname, bool cpSide)
{	
	 if(cpSide)
	 {
		SBOCBuffer* myBuffer;

		myBuffer = getSBOCBufferForCP( _cpname);
		if( myBuffer != NULL)
				myBuffer->clearFileRefs(_cpname);
	 }
	 else
	 {
		  OCBuffer* myBuffer;

		  myBuffer = getOCBufferForCP( _cpname);
		  if( myBuffer != NULL)
				myBuffer->clearFileRefs(_cpname);
	 }
}