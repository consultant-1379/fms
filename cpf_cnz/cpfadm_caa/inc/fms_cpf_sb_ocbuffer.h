/*
 * fms_cpf_sb_ocbuffer.h
 *
 *  Created on: Nov 14, 2011
 *      Author: enungai
 */

#ifndef FMS_CPF_SB_OCBUFFER_H_
#define FMS_CPF_SB_OCBUFFER_H_

#include <ace/Task_T.h>
#include <string>
#include "ACS_TRA_Logging.h"
#include "ACS_TRA_trace.h"
#include "fms_cpf_cpmsg.h"
#include "fms_cpf_cpchannel.h"
#include "fms_cpf_cpdopenfilesmgr.h"
#include "fms_cpf_cpopenchannels.h"
#include <map>

class ACS_TRA_trace;

class SbOcBuffer: public ACE_Task <ACE_MT_SYNCH> {
public:
	SbOcBuffer(std::string cpname);
	virtual ~SbOcBuffer();
	/**
	 * 	@brief  Run by a daemon thread
	*/
	virtual int svc(void);

	/**
	 * 	@brief	This method initializes a task and prepare it for execution
	*/
	virtual int open(void *args = 0);

    void sync();

	void clearFileRefs();

private:
	bool syncReceived;
	bool openSyncReceived;
	bool m_firstCycle;
	bool cleanMessages;
	ACS_TRA_trace* fms_cpf_sbbuffer_trace;
	typedef std::map<unsigned short, FMS_CPF_CPMsg*, std::less<unsigned short>, std::allocator<FMS_CPF_CPMsg*> > cpMsgMap;
	cpMsgMap::iterator myCpMsgPtr;
	typedef std::pair<cpMsgMap::iterator, bool> PAIR_IF;
    unsigned short _nextSeqNr;
	cpMsgMap myCpMsgQueue;
    void insertCpMsg(unsigned short, FMS_CPF_CPMsg *);
    FMS_CPF_CPMsg *fetchCpMsg(unsigned short);
    void  cleanMsgQue();
    void handleCpMsg(FMS_CPF_CPMsg* cpmsg);
    std::string m_cpname;


};



#endif /* FMS_CPF_SB_OCBUFFER_H_ */
