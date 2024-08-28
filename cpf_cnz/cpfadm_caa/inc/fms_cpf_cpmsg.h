//
/** @file fms_cpf_cpmsg.h
 *	@brief
 *	Header file for FMS_CPF_CPMsg class.
 *  This module contains the declaration of the class FMS_CPF_CPMsg.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-11
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
 *	| 1.0.0  | 2011-11-11 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_CPMSG_H_
#define FMS_CPF_CPMSG_H_

#include "fms_cpf_filereference.h"
#include "fms_cpf_fileid.h"
#include "fms_cpf_privateexception.h"
#include "fms_cpf_types.h"

#include <ace/Message_Block.h>

#include <sstream>
#include <iomanip>
#include <sys/timeb.h>
#include <string>


namespace cpMsgFuncCodes
{
  // CP-messages function codes:
  const unsigned short SyncCPMsgFcCode        = 1;  
  const unsigned short EchoCPMsgFcCode        = 3;
  const unsigned short OpenCPMsgFcCode        = 5;
  const unsigned short CloseCPMsgFcCode       = 7;
  const unsigned short WriterandCPMsgFcCode   = 16;
  const unsigned short ReadrandCPMsgFcCode    = 18;
  const unsigned short WritenextCPMsgFcCode   = 28;
  const unsigned short ReadnextCPMsgFcCode    = 30;
  const unsigned short RewriteCPMsgFcCode     = 32;
  const unsigned short ResetCPMsgFcCode       = 34;
  const unsigned short ConnectCPMsgFcCode     = 50;
}

namespace cpMsgConst
{
  // CP-messages constants
  const unsigned short CPMsgOffset          = 0; //Offset to data in CP-messages
  const unsigned short cpMsgVersionMask     = 0x0F000;
  const unsigned short cpMsgFcCodeMask      = 0x0FFF;
  const unsigned short cpMsgVersion2        = 0x08000;
  const unsigned short cpMsgSBSide			= 0x02000;
  const unsigned short cpMsgTwobuffers		= 0x04000;
}

namespace cpMsgInOffset
{
  // Offsets for data in incoming CP-messages :
  const unsigned short cpMsgInFcCodePos     = 0 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgInPrioPos       = 2 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgInSubNetIdPos   = 2 + cpMsgConst::CPMsgOffset; 
  const unsigned short cpMsgInSeqNrPos      = 4 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgInFileRefPos    = 6 + cpMsgConst::CPMsgOffset; 
  const unsigned short cpMsgInSubFilOptPos  = 10 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgInNrOfRecPos    = 10 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgInRecNrPos      = 10 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgInOCSeqPos      = 12 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgInSBOCSeqPos    = 12 + cpMsgConst::CPMsgOffset;    
  const unsigned short cpMsgInSessPtrPos    = 14 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgInBufferPos     = 28 + cpMsgConst::CPMsgOffset;
}

namespace cpMsgOutOffset
{
  // Offsets for data in outgoing CP-messages :
  const unsigned short cpMsgOutFcCodePos    = 0 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgOutSeqNrPos     = 2 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgOutResCodePos   = 4 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgOutMwAllowPos   = 6 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgOutFileRefPos   = 6 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgOutRecsReadPos  = 6 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgOutLstRecWrPos  = 6 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgOutOCSeqPos     = 12 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgOutSBOCSeqPos   = 12 + cpMsgConst::CPMsgOffset;   
  const unsigned short cpMsgOutSessPtrPos   = 14 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgOutRecLenPos    = 28 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgOutFsizePos     = 32 + cpMsgConst::CPMsgOffset;
  const unsigned short cpMsgOutOreplyPos    = 38 + cpMsgConst::CPMsgOffset;
}

namespace cpMsgReplySize
{
  const unsigned short SyncCPMsgSize	    = 18;
  const unsigned short EchoCPMsgSize        = 18;
  const unsigned short OpenCPMsgSize        = 39;
  const unsigned short CloseCPMsgSize       = 18;
  const unsigned short WriterandCPMsgSize   = 18;
  const unsigned short ReadrandCPMsgSize    = 28;
  const unsigned short WritenextCPMsgSize   = 18;
  const unsigned short ReadnextCPMsgSize    = 28;
  const unsigned short RewriteCPMsgSize     = 18;
  const unsigned short ResetCPMsgSize       = 18;
  const unsigned short ConnectCPMsgSize     = 22;
  const unsigned short UnknownCPMsgSize     = 22;
}

namespace cpMsgReplyCode
{
  const unsigned short SyncCPMsgCode	    = cpMsgFuncCodes::SyncCPMsgFcCode + 1;
  const unsigned short EchoCPMsgCode        = cpMsgFuncCodes::EchoCPMsgFcCode + 1;
  const unsigned short OpenCPMsgCode        = cpMsgFuncCodes::OpenCPMsgFcCode + 1;
  const unsigned short CloseCPMsgCode       = cpMsgFuncCodes::CloseCPMsgFcCode + 1;
  const unsigned short WriterandCPMsgCode   = cpMsgFuncCodes::WriterandCPMsgFcCode + 1;
  const unsigned short ReadrandCPMsgCode    = cpMsgFuncCodes::ReadrandCPMsgFcCode + 1;
  const unsigned short WritenextCPMsgCode   = cpMsgFuncCodes::WritenextCPMsgFcCode + 1;
  const unsigned short ReadnextCPMsgCode    = cpMsgFuncCodes::ReadnextCPMsgFcCode + 1;
  const unsigned short RewriteCPMsgCode     = cpMsgFuncCodes::RewriteCPMsgFcCode + 1;
  const unsigned short ResetCPMsgCode       = cpMsgFuncCodes::ResetCPMsgFcCode + 1;
  const unsigned short ConnectCPMsgCode     = cpMsgFuncCodes::ConnectCPMsgFcCode + 1;
  const unsigned short UnknownCPMsgCode     = 0;
}

union word16_t
{
	char byte[2];
	uint16_t word16;
};

union word32_t
{
	char byte[4];
	uint32_t word32;
};


/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class CPDFileThrd;
class FMS_CPF_CpChannel;
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_CPMsg : public ACE_Message_Block
{
 public:

	enum{
			MT_EXIT_MSG = 0x301,
			MT_SYNC_MSG,
			MT_ECHO_MSG,
			MT_OPEN_MSG,
			MT_CLOSE_MSG,
			MT_WRITEAND_MSG,
			MT_READAND_MSG,
			MT_WRITENEXT_MSG,
			MT_READNEXT_MSG,
			MT_REWRITE_MSG,
			MT_RESET_MSG,
			MT_CONNECT_MSG,
			MT_UNKNOW_MSG
		};

	typedef int CP_Message_Type;

	// The constructor do unpack and sets all in* instance variables.
	// This makes it possible to question the object about inBuffer
	// values before the ::go method has been executed.
	// The errorCode method must be executed after creation
	// to check whether an error occured during unpack.
	FMS_CPF_CPMsg(const char* aInBuffer, const unsigned int aInBufferSize, FMS_CPF_CpChannel* cpChannel, CP_Message_Type msgType);

	virtual ~FMS_CPF_CPMsg();

	// Do your work. Returns errorCode.
	virtual unsigned short go(bool &reply);

	// Do your work, from thread. Returns errorCode.
	virtual unsigned short thGo() { return 0; };

	// Increment of sequence number allow or not
	virtual bool incSeqNrAllowed() const { return true; };

	//  Check if message is a forced close
	virtual bool forcedClose() const { return false; };

	// Returns the error code from the exception, if one has
	// occured during the ::go method. If no errors occured
	// 0 is returned.
	unsigned short errorCode() const { return myErrorCode; };


	// Returns the CP result code. This is a translated errorCode.
	uint16_t resultCode() const;

	// Returns address of the object that the buffer arrived on.
	FMS_CPF_CpChannel* cpChannel() const { return m_CpChannel; };

	// Returns the name of CP
	const char* getCpName() const { return m_CpName.c_str(); };

	// Set the CP name
	void setCpName(std::string& cpName) { m_CpName = cpName; };

	// Returns the request code.
	uint16_t requestCode();

	// Returns the reply buffer size.
	unsigned long replyBufferSize() const { return outBufferSize; };

	// Returns the reply buffer.
	char* replyBuffer() const { return outBuffer; };

	// Returns the sequence number in the message
	uint16_t getInSequenceNumber() const { return inSequenceNumber; };

	// Returns the OC sequence number of the message
	virtual uint16_t getOCSeq() const { return 0; };

	// Returns the SBOC sequence number of the message
	virtual uint16_t getSBOCSeq() const { return 0; };

	// Set errorcode
	void forceErrorCode(unsigned short code) {myErrorCode = code; };

    //Returns true if message is from EX-side APFMI
	bool messageIsFromEX() const {return isEXMsg; };

    // Returns the protocolversion
	uint16_t protoVersion() const { return protocolVersion; };
    
	void setReadyToSend(bool value) { m_ReadyToSend = value; };

	bool getReadyToSend() const { return m_ReadyToSend; };

	// Pointer to corresponding file thread, if any.
	CPDFileThrd* fThrd;

    const unsigned int inBufferSize;
    
 private:

	virtual void unpack() = 0;
	virtual bool work() = 0;
	virtual void pack() = 0;
	virtual void thWork() { };

	const char* inBuffer;
	char* outBuffer;
	unsigned long	outBufferSize;
	unsigned short	myErrorCode;

	bool m_ReadyToSend;

	ACS_TRA_trace* fms_cpf_CPMsg;

 protected:

	// Big endian, MSB first, LSB last.
	// Small endian, MSB last, LSB first.
	// The net is always considered to be Big endian.
	// Sun & Tandem is Big endian.
	// CP is small endian.

	// Get parameters from the net packed in big endian format.
	uint32_t get_ulong_net(unsigned short offset)
	throw (FMS_CPF_PrivateException);

	void set_ulong_net(unsigned short offset, unsigned long value)
	throw (FMS_CPF_PrivateException);

	// Get parameters from the CP packed in small endian format.
	unsigned char get_uchar_cp(unsigned short offset) const
	throw (FMS_CPF_PrivateException);

	void set_uchar_cp(unsigned short offset,  unsigned char value)
	throw (FMS_CPF_PrivateException);

	uint16_t get_ushort_cp(unsigned short offset)
	throw (FMS_CPF_PrivateException);

	void set_ushort_cp(unsigned short offset,  unsigned short value)
	throw (FMS_CPF_PrivateException);

	uint32_t get_ulong_cp(unsigned short offset)
	throw (FMS_CPF_PrivateException);

	void set_ulong_cp (unsigned short offset, unsigned long value)
	throw (FMS_CPF_PrivateException);

	std::string get_string_cp (unsigned short offset) const
	throw (FMS_CPF_PrivateException);

	void set_string_cp (unsigned short offset, std::string& value)
	throw (FMS_CPF_PrivateException);

	const char* get_buffer_in(unsigned short offset)
	throw (FMS_CPF_PrivateException);

	unsigned long get_buffer_size_in(unsigned short offset)
	throw (FMS_CPF_PrivateException);

	char* get_buffer_out(unsigned short offset)
	throw (FMS_CPF_PrivateException);

	void logError(const FMS_CPF_PrivateException& ex);

	void createOutBuffer(unsigned short aOutBufferSize);

	void checkPriority(unsigned short aPriority)
	throw (FMS_CPF_PrivateException);

	void checkSubfileOption(unsigned short aSubfileOption)
	throw (FMS_CPF_PrivateException);

	void checkAccessType(unsigned short aAccessType)
	throw (FMS_CPF_PrivateException);

	FMS_CPF_Types::fileType getFileType(const FMS_CPF_FileId& aFileId);

	unsigned long m_ReplySizeMsg;
	uint16_t inSequenceNumber;
	uint16_t protocolVersion;
	uint32_t cp_session_ptr;

	bool APFMIdoubleBuffers;
	bool isEXMsg;
	unsigned short msgIn;
	unsigned short APFMIversion;

	FMS_CPF_CpChannel* m_CpChannel;

	std::string m_CpName;

	// methods for CONVERT purpose
	uint16_t ushortCpToHost(const char* buffer);
	void ushortHostToCp(char* buffer, uint16_t value);

	uint32_t ulongCpToHost(const char* buffer);
	void ulongHostToCp(char* buffer, uint32_t value);

	uint32_t ulongNetToHost(const char* buffer);
	void ulongHostToNet(char* buffer, uint32_t value);
};

#endif //FMS_CPF_CPMSG_H_
