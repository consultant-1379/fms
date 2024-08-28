//**********************************************************************
//
// NAME
//      SBOCBuffer.H
//
// COPYRIGHT Ericsson Utvecklings AB, Sweden 2001.
// All rights reserved.
//
// The Copyright to the computer program(s) herein is the property of
// Ericsson Utvecklings AB, Sweden.
// The program(s) may be used and/or copied only with the written
// permission from Ericsson Utvecklings AB or in accordance with 
// the terms and conditions stipulated in the agreement/contract under
// which the program(s) have been supplied.

// DESCRIPTION
//
// 

// DOCUMENT NO
//	

// AUTHOR 
// 	

// REVISION
//	
//	 	

// CHANGES
//
//	RELEASE REVISION HISTORY
//
//	REV NO		DATE		NAME 		DESCRIPTION
//              011025      qabmnnn     First draft
//		    	  011128	qabtjer		Message buffer introduced
//										to work with redesigned APFMI.
//										Solution from APG30.  
//				020911		qabtjer		Copied to create a second sequence number queue
//										due to problems with FCCPL command for example.
//				021025      qablake     Add cleanMsgQue and syncReceived
//                                      to clean msg que after sync                        
//            
//
// SEE ALSO 
//
//
//**********************************************************************

#ifndef _SBOCBUFFER_H
#define _SBOCBUFFER_H

//#include <rw/tvhdict.h>  //qabtjer 011128


#include <map>      
#include <string>  
#include "FCC_Thread.H"
#include "CPMsg.H"

#ifdef WIN32  
  #pragma warning( disable : 4786 )
#endif



class SBOCBuffer: public FCC_Thread<CPMsg>
{
private:
    //typedef unsigned (*HashType)(const unsigned short &);  //qabtjer 011128
  //  RWTValHashDictionary<unsigned short, CPMsg *> _queue;  //qabtjer 011128
	/* sorted CP msg wait queue */
	typedef std::map<unsigned short, CPMsg*, std::less<unsigned short>, std::allocator<CPMsg*> > cpMsgMap;
	cpMsgMap::iterator myCpMsgPtr;
	typedef struct std::pair<cpMsgMap::iterator, bool> PAIR_IF;
    unsigned short _nextSeqNr;
	cpMsgMap myCpMsgQueue; //qabtjer 011128

	bool syncReceived;//qablake 021025
	bool openSyncReceived;
	static CRITICAL_SECTION sbocBufCS;
    void insertCpMsg(unsigned short, CPMsg *);
    CPMsg *fetchCpMsg(unsigned short);
    //	static unsigned hash(unsigned short &); //qabtjer 011128
    void  cleanMsgQue();  //qablake 021025
public:
	SBOCBuffer();
    virtual ~SBOCBuffer();
    virtual bool execute();
    void sync();
	void clearFileRefs(const char * cpname);
};

#endif
