#ifndef OC_SBOC_BUFFER_MGR_H
#define OC_SBOC_BUFFER_MGR_H

#include "osf/Synch_T.h"
#include "osf/os.h"
#include "ocbuffer.h"
#include "sbocbuffer.h"
#include <string>
#include <list>
#include <map>

using namespace std;

class OC_SBOC_BufferMgr
{
	typedef map<string, OCBuffer*>   ocMapType;
	typedef map<string, SBOCBuffer*> sbocMapType;

	public:
		static OC_SBOC_BufferMgr* instance();

		// return OCBuffer instance given cpname
		OCBuffer* getOCBufferForCP(const char* cpname);

		// return SBOCBuffer instance given cpname
		SBOCBuffer* getSBOCBufferForCP(const char* cpname);
		void pushInOCBuf(const char* _cpname, CPMsg *cpmsg);
		void pushInSBOCBuf(const char* _cpname, CPMsg *cpmsg);
		void OCBsync( const char* _cpname);
		void SBOCBsync( const char* _cpname);
		void clearFileRefs(const char* _cpname, bool cpSide);
		static const string				DEFAULT_CPNAME;

	private:
		static OC_SBOC_BufferMgr		*instance_;
		//It holds OCBuffer instance per cpnmame
		ocMapType						*ocTableMap;
		//It holds SBOCBuffer instance per cpname
		sbocMapType						*sbocTableMap;
		
		CRITICAL_SECTION handle_concurr_access_OCBuff;
		CRITICAL_SECTION handle_concurr_access_SBOCBuff;
  		
		// Mutex used to implement double checl pattern on singleton
  		static OSF_Recursive_Thread_Mutex singletonSynch;

		OC_SBOC_BufferMgr()
			throw ( FMS_CPF_PrivateException);
};

#endif