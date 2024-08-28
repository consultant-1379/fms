//
/** @file fms_cpf_CPMsg.cpp
 *	@brief
 *	Class method implementation for fms_cpf_CPMsg.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_CPMsg.h module
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
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_cpmsg.h"
#include "fms_cpf_cpchannel.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_common.h"

#include "fms_cpf_privateexception.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include <time.h>	
#include <iostream>

extern ACS_TRA_Logging CPF_Log;

/*===============================================================================
	constructor
 =============================================================================== */
FMS_CPF_CPMsg::FMS_CPF_CPMsg(const char* aInBuffer, const unsigned int aInBufferSize, FMS_CPF_CpChannel* cpChannel, CP_Message_Type msgType) :
ACE_Message_Block(),
fThrd(NULL),
inBufferSize(aInBufferSize),
inBuffer(aInBuffer),
outBuffer(NULL),
outBufferSize(0),
myErrorCode(0),
m_ReadyToSend(false),
m_ReplySizeMsg(0),
inSequenceNumber(0),
protocolVersion(0),
cp_session_ptr(0),
APFMIdoubleBuffers(true),
isEXMsg(true),
msgIn(0),
APFMIversion(0),
m_CpChannel(cpChannel)
{
	this->msg_type(msgType);
    fms_cpf_CPMsg = new (std::nothrow) ACS_TRA_trace("FMS_CPF_CPMsg");
}

/*===============================================================================
	destructor
 =============================================================================== */
FMS_CPF_CPMsg::~FMS_CPF_CPMsg()
{
	if(NULL != inBuffer)
		delete[] inBuffer;

	if(NULL != outBuffer)
		delete[] outBuffer;

    if(NULL != fms_cpf_CPMsg)
        delete fms_cpf_CPMsg;
}

/*===============================================================================
	ROUTINE: requestCode
 =============================================================================== */
unsigned short FMS_CPF_CPMsg::requestCode()
{
  return get_ushort_cp(0);   
}

/*===============================================================================
	ROUTINE: logError
 =============================================================================== */
void FMS_CPF_CPMsg::logError(const FMS_CPF_PrivateException& ex)
{
  // Do not log secondary exceptions.
  if(myErrorCode == 0)
  {
    myErrorCode = ex.errorCode();
  }
}

/*===============================================================================
	ROUTINE: resultCode
 =============================================================================== */
uint16_t FMS_CPF_CPMsg::resultCode() const
{
	// Convert errorCode to a CP result code.
	uint16_t value;

	TRACE(fms_cpf_CPMsg, "resultCode(), myError = %d", myErrorCode);

	switch(myErrorCode)
	{
		case 0:  value = 0;
		  break;
		case FMS_CPF_PrivateException::NOMEMORY:      value = 3;
		  break;
		case FMS_CPF_PrivateException::PHYSICALNOTFOUND: value = 4;
		  break;
		case FMS_CPF_PrivateException::FILENOTFOUND:  value = 4;
		  break;
		case FMS_CPF_PrivateException::ACCESSERROR:   value = 5;
		  break;
		case FMS_CPF_PrivateException::INVALIDREF:    value = 6;
		  break;
		case FMS_CPF_PrivateException::WRONGRECNUM:   value = 7;
		  break;
		case FMS_CPF_PrivateException::PHYSICALERROR: value = 8;
		  break;
		case FMS_CPF_PrivateException::ILLVALUE:      value = 10;
		  break;
		case FMS_CPF_PrivateException::INVALIDFILE:   value = 10;
		  break;
		case FMS_CPF_PrivateException::PARAMERROR:    value = 10;
		  break;
		case FMS_CPF_PrivateException::TYPEERROR:     value = 14;
		  break;
		case FMS_CPF_PrivateException::NOTIMPL:       value = 14;
		  break;
		case FMS_CPF_PrivateException::TIMEOUT:       value = 16;
		  break;
		case FMS_CPF_PrivateException::FILEEXISTS:    value = 17;
		  break;
		case FMS_CPF_PrivateException::NOTCOMPOSITE:  value = 14;
		  break;
		default:                                      value = 9;
	}
  
	TRACE(fms_cpf_CPMsg,"resultCode(), value = %d",value);

	return value;
}

/*===============================================================================
	ROUTINE: go
 =============================================================================== */
unsigned short FMS_CPF_CPMsg::go(bool &reply)
{
  if (myErrorCode == 0)
  {
    // Do not work if exception during unpack
    work();
  }
  pack();
  reply = true;
  return errorCode();
}

/*===============================================================================
	ROUTINE: get_ulong_net
 =============================================================================== */
uint32_t FMS_CPF_CPMsg::get_ulong_net(unsigned short offset)
throw (FMS_CPF_PrivateException)
{
	// To avoid comparison warning
	unsigned short maxPos = offset + 4;
	if( maxPos > inBufferSize)
	{
		std::stringstream detail;
		detail << "ulong inBuffer["<< offset << "] inBufferSize=" << inBufferSize << std::ends;
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PARAMERROR, detail.str() );
	}
	return ulongNetToHost(&inBuffer[offset]);
}

/*===============================================================================
	ROUTINE: set_ulong_net
 =============================================================================== */
void FMS_CPF_CPMsg::set_ulong_net(unsigned short offset, unsigned long value)
throw (FMS_CPF_PrivateException)
{  
	// To avoid comparison warning
	unsigned short maxPos = offset + 4;
	if(maxPos > outBufferSize)
	{
		std::stringstream detail;
		detail << "ulong outBuffer["<< offset << "] outBufferSize=" << outBufferSize << std::ends;
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, detail.str() );
	}
	ulongHostToNet(&outBuffer[offset],value);
}

/*===============================================================================
	ROUTINE: get_uchar_cp
 =============================================================================== */
unsigned char FMS_CPF_CPMsg::get_uchar_cp(unsigned short offset) const
throw (FMS_CPF_PrivateException)
{
	// To avoid comparison warning
	unsigned short maxPos = offset + 1;
	if(maxPos > inBufferSize)
	{
		std::stringstream detail;
		detail << "uchar inBuffer[" << offset << "] inBufferSize=" << inBufferSize << std::ends;
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PARAMERROR, detail.str() );
	}
	return inBuffer[offset];
}

/*===============================================================================
	ROUTINE: set_uchar_cp
 =============================================================================== */
void FMS_CPF_CPMsg::set_uchar_cp(unsigned short offset,  unsigned char value)
throw (FMS_CPF_PrivateException)
{
	// To avoid comparison warning
	unsigned short maxPos = offset + 1;
	if(maxPos > outBufferSize)
	{
		std::stringstream detail;
		detail << "unsigned char outBuffer["<< offset << "] outBufferSize=" << outBufferSize << std::ends;
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, detail.str() );
	}
	outBuffer[offset] = value;
}

/*===============================================================================
	ROUTINE: get_ushort_cp
 =============================================================================== */
uint16_t FMS_CPF_CPMsg::get_ushort_cp(unsigned short offset)
throw (FMS_CPF_PrivateException)
{
	// To avoid comparison warning
	unsigned short maxPos = offset + 2;

	if(maxPos > inBufferSize)
	{
		std::stringstream detail;
		detail << "unsigned short inBuffer["<< offset << "] inBufferSize=" << inBufferSize << std::ends;
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PARAMERROR, detail.str() );
	}
	return ushortCpToHost(&inBuffer[offset]);
}

/*===============================================================================
	ROUTINE: set_ushort_cp
 =============================================================================== */
void FMS_CPF_CPMsg::set_ushort_cp(unsigned short offset,  unsigned short value)
throw (FMS_CPF_PrivateException)
{
	// To avoid comparison warning
	unsigned short maxPos = offset + 2;

	if(maxPos > outBufferSize)
	{
		std::stringstream detail;
		detail << "unsigned short outBuffer["<< offset << "] outBufferSize=" << outBufferSize << std::ends;
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, detail.str() );
	}
	ushortHostToCp(&outBuffer[offset],value);
}

/*===============================================================================
	ROUTINE: get_ulong_cp
 =============================================================================== */
uint32_t FMS_CPF_CPMsg::get_ulong_cp(unsigned short offset)
throw (FMS_CPF_PrivateException)
{
	// To avoid comparison warning
	unsigned short maxPos = offset + 4;

	if(maxPos > inBufferSize)
	{
		std::stringstream detail;
		detail << "ulong inBuffer["<< offset << "] inBufferSize=" << inBufferSize << std::ends;
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PARAMERROR, detail.str() );
	}
	return ulongCpToHost(&inBuffer[offset]);
}

/*===============================================================================
	ROUTINE: set_ulong_cp
 =============================================================================== */
void FMS_CPF_CPMsg::set_ulong_cp (unsigned short offset, unsigned long value)
throw (FMS_CPF_PrivateException)
{
	// To avoid comparison warning
	unsigned short maxPos = offset + 4;

	if(maxPos > outBufferSize)
	{
		std::stringstream detail;
		detail << "unsigned long outBuffer["<< offset << "] outBufferSize=" << outBufferSize << std::ends;
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, detail.str() );
	}
	ulongHostToCp(&outBuffer[offset],value);
}

/*===============================================================================
	ROUTINE: get_string_cp
 =============================================================================== */
std::string FMS_CPF_CPMsg::get_string_cp(unsigned short offset) const
throw (FMS_CPF_PrivateException)
{
	unsigned short length = get_uchar_cp(offset);

	// To avoid comparison warning
	unsigned short maxPos = offset + length + 1;

	if(maxPos > inBufferSize)
	{
		std::stringstream detail;
		detail << "string inBuffer["<< offset << "] strlen=" << length << " inBufferSize=" << inBufferSize << std::ends;
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PARAMERROR, detail.str() );
	}
	return std::string(inBuffer+offset+1,length);
}

/*===============================================================================
	ROUTINE: set_string_cp
 =============================================================================== */
void FMS_CPF_CPMsg::set_string_cp(unsigned short offset, std::string& value)
throw (FMS_CPF_PrivateException)
{
	// To avoid comparison warning
	unsigned int maxPos = offset + value.length() + 1;

	if(maxPos > outBufferSize)
	{
		std::stringstream detail;
		detail << "string outBuffer[" << offset << "] strlen=" << value.length() << " outBufferSize=" << outBufferSize << std::ends;
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, detail.str() );
	}
	set_uchar_cp(offset,(unsigned char)value.length());
	ACE_OS::memcpy(outBuffer+offset+1,value.data(),value.length());
}

/*===============================================================================
	ROUTINE: get_buffer_in
 =============================================================================== */
const char* FMS_CPF_CPMsg::get_buffer_in(unsigned short offset)
throw (FMS_CPF_PrivateException)
{
	if (offset > inBufferSize)
	{
		std::stringstream detail;
		detail << "get_buffer_in("<< offset << ") inBufferSize=" << inBufferSize << std::ends;
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PARAMERROR, detail.str() );
	}
	return inBuffer + offset;
}

/*===============================================================================
	ROUTINE: get_buffer_size_in
 =============================================================================== */
unsigned long FMS_CPF_CPMsg::get_buffer_size_in(unsigned short offset)
throw (FMS_CPF_PrivateException)
{
	if (offset > inBufferSize)
	{
		std::stringstream detail;
		detail << "get_buffer_size_in(" << offset << ") inBufferSize=" << inBufferSize << std::ends;
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PARAMERROR, detail.str() );
	}
	return inBufferSize - offset;
}

/*===============================================================================
	ROUTINE: get_buffer_out
 =============================================================================== */
char * FMS_CPF_CPMsg::get_buffer_out(unsigned short offset)
throw (FMS_CPF_PrivateException)
{
	// To avoid comparison warning
	unsigned int maxPos = offset + 1;

	if(maxPos > outBufferSize)
	{
		std::stringstream detail;
		detail << "get_buffer_out("<< offset << ") outBufferSize=" << outBufferSize << std::ends;
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, detail.str() );
	}
 	return outBuffer + offset;
}

/*===============================================================================
	ROUTINE: createOutBuffer
 =============================================================================== */
void FMS_CPF_CPMsg::createOutBuffer(unsigned short aOutBufferSize)
{
	// Can be called more than one time. Only the first call has effect.
	if(NULL == outBuffer)
	{
		// Allocate memory for the buffer.
		outBufferSize = aOutBufferSize;
		outBuffer = new char[outBufferSize];
		ACE_OS::memset(outBuffer,0,outBufferSize);
    
		if(aOutBufferSize >= sizeof(uint32_t))
		{
			// Set the size of the buffer.
			set_ulong_net(0, aOutBufferSize);
		}
	}
}

/*===============================================================================
	ROUTINE: checkPriority
 =============================================================================== */
void FMS_CPF_CPMsg::checkPriority(unsigned short aPriority)
throw (FMS_CPF_PrivateException)
{
	if (aPriority > 3)
	{
		std::stringstream detail;
		detail << "aPriority = " << aPriority << std::ends;
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PARAMERROR, detail.str() );
	}
}

/*===============================================================================
	ROUTINE: checkSubfileOption
 =============================================================================== */
void FMS_CPF_CPMsg::checkSubfileOption(unsigned short aSubfileOption)
throw (FMS_CPF_PrivateException)
{
  if (aSubfileOption > 1)
  {
	  std::stringstream detail;
	  detail << "aSubfileOption = " << aSubfileOption << std::ends;
	  throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PARAMERROR, detail.str() );
  }
}

/*===============================================================================
	ROUTINE: checkAccessType
 =============================================================================== */
void FMS_CPF_CPMsg::checkAccessType(unsigned short aAccessType)
throw (FMS_CPF_PrivateException)
{
  if (aAccessType < 1 || aAccessType > 8)
  {
	  std::stringstream detail;
	  detail << "aAccessType = " << aAccessType << std::ends;
	  throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PARAMERROR, detail.str() );
  }
}

/*===============================================================================
	ROUTINE: getFileType
 =============================================================================== */
FMS_CPF_Types::fileType FMS_CPF_CPMsg::getFileType(const FMS_CPF_FileId& aFileId)
{

	FMS_CPF_Types::fileType fileType = DirectoryStructureMgr::instance()->getFileType(aFileId.file(), m_CpName);
	
	return fileType;
}

/*===============================================================================
	ROUTINE: ushortCpToHost
 =============================================================================== */
uint16_t FMS_CPF_CPMsg::ushortCpToHost(const char* buffer)
{
  word16_t w;
  w.byte[0] = buffer[0];
  w.byte[1] = buffer[1];
  return w.word16;
}

/*===============================================================================
	ROUTINE: ushortHostToCp
 =============================================================================== */
void FMS_CPF_CPMsg::ushortHostToCp(char* buffer, uint16_t value)
{
  word16_t w;
  w.word16 = value;
  buffer[0] = w.byte[0];
  buffer[1] = w.byte[1];
}

/*===============================================================================
	ROUTINE: ulongCpToHost
 =============================================================================== */
uint32_t FMS_CPF_CPMsg::ulongCpToHost(const char* buffer)
{
  word32_t w;
  w.byte[0] = buffer[0];
  w.byte[1] = buffer[1];
  w.byte[2] = buffer[2];
  w.byte[3] = buffer[3];
  return w.word32;
}

/*===============================================================================
	ROUTINE: ulongHostToCp
 =============================================================================== */
void FMS_CPF_CPMsg::ulongHostToCp(char* buffer, uint32_t value)
{
  word32_t w;
  w.word32 = value;
  buffer[0] = w.byte[0];
  buffer[1] = w.byte[1];
  buffer[2] = w.byte[2];
  buffer[3] = w.byte[3];
}

/*===============================================================================
	ROUTINE: ulongNetToHost
 =============================================================================== */
uint32_t FMS_CPF_CPMsg::ulongNetToHost(const char* buffer)
{
  word32_t w;
  w.byte[0] = buffer[0];
  w.byte[1] = buffer[1];
  w.byte[2] = buffer[2];
  w.byte[3] = buffer[3];
  return ntohl(w.word32);
}

/*===============================================================================
	ROUTINE: ulongHostToNet
 =============================================================================== */
void FMS_CPF_CPMsg::ulongHostToNet(char* buffer, uint32_t value)
{
  word32_t w;
  w.word32 = htonl(value);
  buffer [0] = w.byte [0];
  buffer [1] = w.byte [1];
  buffer [2] = w.byte [2];
  buffer [3] = w.byte [3];
};
