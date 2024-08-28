//**********************************************************************
//
// NAME
//      OCBuffer.C
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
//				011128		qabtjer		Modified for AGP40. Solution from APG30
//				021025      qablake     Empty message que after a sync to avoid
//										dublicated entry and to trigger old messages
//										Delete msg when CP channel has died 
//										avoid memory eater 
//            
//
// SEE ALSO 
//
//
//**********************************************************************


#ifdef WIN32 // 030825
  #pragma warning ( disable : 4786 )  
#endif

#include "ACS_TRA_trace.H"
#include "CPChannelThrd.H"
#include "CPDOpenChannels.H"
#include "FMS_CPF_EventHandler.H"
#include "OCBuffer.H"
#include "CPDOpenFilesMgr.H"
#include "CPDOpenFiles.H" //040421

#include <iostream>

CRITICAL_SECTION OCBuffer::ocBufCS;  //040421
//------------------------------------------------------------------------------
//      Tracepoints
//------------------------------------------------------------------------------
static ACS_TRA_trace FMS_CPF_OCBuffer =
ACS_TRA_DEF("FMS_CPF_OCBuffer" ,"C400");
static char trace[400];

//------------------------------------------------------------------------------
//      Static objects
//------------------------------------------------------------------------------
// qassore     /*OCBuffer *OCBuffer::_instance = new OCBuffer();*/

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
OCBuffer::OCBuffer(): FCC_Thread<CPMsg>("FMS_CPF"), _nextSeqNr(1)
    
{
    if(ACS_TRA_ON(FMS_CPF_OCBuffer)) {
		ACS_TRA_event(&FMS_CPF_OCBuffer, "OCBuffer::OCBuffer()");
	}

    // Start thread
    if (!resume()) {
        FMS_CPF_PrivateException ex(FMS_CPF_Exception::INTERNALERROR);
        ex << "Could not start Open-Close buffer";
        //ex << ", file=" << __FILE__ << ", line=" << __LINE__;
        FMS_CPF_EventHandler::instance()->event(ex);
    }
//	std::cout<<"InitializeCriticalSection(&ocBufCS)"<<std::endl;
	InitializeCriticalSection(&ocBufCS); //040421
	openSyncReceived = false; //040421
}


//------------------------------------------------------------------------------
//      insertCpMsg
//------------------------------------------------------------------------------
void 
OCBuffer::insertCpMsg(unsigned short seqNr, CPMsg* aMsg)  //qabtjer 011128
{
  PAIR_IF inserted = 
  myCpMsgQueue.insert(cpMsgMap::value_type(seqNr, aMsg));
  
#ifdef HD11123OCb
  std::cout<<"Enters OCBuffer::insertCpMsg"<<std::endl;
#endif
  if(! inserted.second)
  {
    FMS_CPF_PrivateException ex(FMS_CPF_PrivateException::INTERNALERROR);
    ex << "OCBuffer::insertCpMsg, duplicate keys";
    FMS_CPF_EventHandler::instance()->event(ex);
  }
  else
  {
    #ifdef TRACE_B
      cerr << "CPDFileThrd::insertCpMsg(" << seqNr << ")" << endl;
    #endif
  }
}


//------------------------------------------------------------------------------
//      fetchCpMsg
//------------------------------------------------------------------------------
CPMsg*  OCBuffer::fetchCpMsg(unsigned short aSeqNr)  //qabtjer 011128
{
  myCpMsgPtr = myCpMsgQueue.find(aSeqNr);

  if (myCpMsgPtr != myCpMsgQueue.end())
  {
#ifdef HD11123OCb
	  std::cout<<"Enters OCBuffer::fetchCpMsg ifstatement"<<std::endl;
#endif
    CPMsg* aCpMsg = (*myCpMsgPtr).second;
    myCpMsgQueue.erase(aSeqNr);
    return aCpMsg;
  }
  return 0;
}

//------------------------------------------------------------------------------
//      cleanMsgQue
//------------------------------------------------------------------------------

void  OCBuffer::cleanMsgQue()  //021025  qablake
{
	
	myCpMsgPtr = myCpMsgQueue.begin();
	while (myCpMsgPtr != myCpMsgQueue.end())
	{  
		if(ACS_TRA_ON(FMS_CPF_OCBuffer)) 
		{
            sprintf(trace, "OCBuffer::cleanMsgQue()");
            ACS_TRA_event(&FMS_CPF_OCBuffer, trace);
         }
		CPMsg* aCpMsg = (*myCpMsgPtr).second;
		delete aCpMsg;
		myCpMsgQueue.erase(myCpMsgPtr);
		myCpMsgPtr = myCpMsgQueue.begin();
	}
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
OCBuffer::~OCBuffer()
{
    if(ACS_TRA_ON(FMS_CPF_OCBuffer)) {
		ACS_TRA_event(&FMS_CPF_OCBuffer, "OCBuffer::~OCBuffer()");
	}

    // Terminate thread
    terminate();   
}


//------------------------------------------------------------------------------
//      Thread execution
//------------------------------------------------------------------------------
bool OCBuffer::execute()
{
	static int executecounter = 0;
    bool cleanMessages = false; 

    if(ACS_TRA_ON(FMS_CPF_OCBuffer)) {
		ACS_TRA_event(&FMS_CPF_OCBuffer, "OCBuffer::execute()");
	}
    syncReceived = false;//021025  qablake ++
    // Execute thread until someone wants to terminate
    while (!Terminated) {
		if(ACS_TRA_ON(FMS_CPF_OCBuffer)) 
		{
            sprintf(trace, "OCBuffer::execute()");
            ACS_TRA_event(&FMS_CPF_OCBuffer, trace);
         }

        // Get a msg from thread queue with a timeout of 10 secs
        CPMsg *cpmsg;
        if (getMsg(cpmsg, 10000, true) == TMSG_ARRIVED) {

            int seq = cpmsg->getOCSeq();
            if(ACS_TRA_ON(FMS_CPF_OCBuffer)) {
                sprintf(trace, "OCBuffer::execute() - seqnr = %d  %d", seq ,_nextSeqNr);
                ACS_TRA_event(&FMS_CPF_OCBuffer, trace);
            }
			if (syncReceived) { //021025  qablake ++
                // Clear number to 1
                _nextSeqNr = 1;
				syncReceived = false;
				if (executecounter == 0)
				{
					executecounter++;
				}
				else
				{
					openSyncReceived = true; //040501
				}
                cleanMessages = true;
			} //021025  qablake ++

            if (cleanMessages) {
				cleanMsgQue(); // remove old msg 
                cleanMessages = false;
            }

            // Check if it is next in sequence
            if (seq == _nextSeqNr || seq == 0) { // seq == 0 equals old APFMI
                do {
						if(ACS_TRA_ON(FMS_CPF_OCBuffer)) 
						{
							sprintf(trace, "OCBuffer::execute do - while 1");
							ACS_TRA_event(&FMS_CPF_OCBuffer, trace);
						}
                    // Do the work for this message
                    bool replyNow = false;
                    cpmsg->go(replyNow);
                    if (replyNow) {
                        // Send answer to CP
                        CPChannelThrd *cp = (CPChannelThrd *)cpmsg->cpChannel();
                      //  if (CPDOpenChannels::Instance()->lookup(cp))
                      //      cp->pushMsg(cpmsg); //qabtjer 011128 changed from sendAnswer replaced with reserve()
						if (CPDOpenChannels::Instance()->reserve(cp))   
						{
#ifdef HD11123OCb
							std::cout<<"Enters OCBuffer::cp->pushMsg"<<std::endl;
#endif						
		 					cp->pushMsg(cpmsg);          // send reply
#ifdef HD11123OCb
							std::cout<<"Leaves OCBuffer::cp->pushMsg"<<std::endl;
#endif				
							CPDOpenChannels::Instance()->unreserve(cp);
#ifdef HD11123OCb
							std::cout<<"Leaves OCBuffer::Instance()->unreserve(cp);"<<std::endl;
#endif				
						} //021025  qablake ++
						else {
							delete cpmsg;
						}//021025  qablake --					
                    }
                    
                    if (syncReceived) {
                        _nextSeqNr = 1;
				        syncReceived = false;
				        if (executecounter == 0)
				        {
					        executecounter++;
				        }
				        else
				        {
					        openSyncReceived = true; //040501
				        }
                        cleanMessages = true;
                    }
                    else {
                        // Increase the counter for next message
                        _nextSeqNr++;
                    }
               } while (cpmsg = fetchCpMsg(_nextSeqNr));
            }
            else {
                // Stuff it into the internal queue
                insertCpMsg(seq, cpmsg);
            }
        }
    }
    return true;
}


//------------------------------------------------------------------------------
//      Synchronize sequence numbers with CP
//------------------------------------------------------------------------------
void OCBuffer::sync()
{
    if(ACS_TRA_ON(FMS_CPF_OCBuffer)) {
		ACS_TRA_event(&FMS_CPF_OCBuffer, "OCBuffer::sync()");
	}

	syncReceived = true; //021025  qablake  Clear que in main loop
}

/*
//------------------------------------------------------------------------------
//      Synchronize sequence numbers with CP
//------------------------------------------------------------------------------
unsigned OCBuffer::hash(unsigned short &key)
{
    if(ACS_TRA_ON(FMS_CPF_OCBuffer)) {
		ACS_TRA_event(&FMS_CPF_OCBuffer, "OCBuffer::hash(unsigned short &)");
	}

    return key;
}*/



//------------------------------------------------------------------------------
//      Make sure that all filreferences are removed before handling first open message
//------------------------------------------------------------------------------
void OCBuffer::clearFileRefs(const char * cpname)  //040421
{
    if(ACS_TRA_ON(FMS_CPF_OCBuffer)) 
	{
		ACS_TRA_event(&FMS_CPF_OCBuffer, "OCBuffer::clearFileRefs() begins");
	}
	EnterCriticalSection(&ocBufCS);
    if(openSyncReceived)
	{
		if(ACS_TRA_ON(FMS_CPF_OCBuffer)) 
		{
			ACS_TRA_event(&FMS_CPF_OCBuffer, "OCBuffer::clearFileRefs() before removeAll");
		}

		CPDOpenFilesMgr::Instance()->removeAll(true, cpname);
		if(ACS_TRA_ON(FMS_CPF_OCBuffer)) 
		{
			ACS_TRA_event(&FMS_CPF_OCBuffer, "OCBuffer::clearFileRefs() after removeAll");
		}
	
		openSyncReceived = false; //040421 

	}
	if(ACS_TRA_ON(FMS_CPF_OCBuffer)) 
	{
		ACS_TRA_event(&FMS_CPF_OCBuffer, "OCBuffer::clearFileRefs() ends");
	}
	LeaveCriticalSection(&ocBufCS);
	

}
