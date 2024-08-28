/*
 * * @file fms_cpf_jtpcpmsg.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_JTPCpMsg.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_jtpcpmsg.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-08-07
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
 *	| 1.0.0  | 2012-08-07 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_jtpcpmsg.h"
#include "fms_cpf_common.h"

#include "aes_ohi_extfilehandler2.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include <sstream>

extern ACS_TRA_Logging CPF_Log;

namespace APFPprotocol{

	// header size -> 1 byte
	// buffer data size -> 2 byte
	// function code -> 1 byte
	// priority -> 1 byte
	// result code -> 2 byte
	// total 7 byte
	const unsigned short headerSize = 6;

	// 8 date-time values, 5 bytes each (yy mm dd hh mm) -> 40 byte
	// fail reason -> 1 byte
	// file reocrd size -> 4 byte
	// sequence number -> 2 byte
	// lengths info -> 4 byte
	// total 51 byte
	const unsigned short fullInfoSize = 51;

	const unsigned short priority = 1;
}

FMS_CPF_JTPCpMsg::statusInfo::statusInfo()
{
	memset(dateAutoSendStart, 0, 20);
	memset(dateAutoSendEnd, 0, 20);
	memset(dateManSendStart, 0, 20);
	memset(dateManSendEnd, 0, 20);
	memset(dateAutoSendFail, 0, 20);
	memset(dateDumpedOnPrimary, 0, 20);
	memset(dateDumpedOnSecondary, 0, 20);

	reasonForFailure = 0;
	subFileSize = 0;
}

/*============================================================================
	ROUTINE: FMS_CPF_JTPCpMsg
 ============================================================================ */
FMS_CPF_JTPCpMsg::FMS_CPF_JTPCpMsg(const char* buffer, const int bufferLength)
: m_InputBufferLength(bufferLength),
  m_InputBufferPosition(0),
  m_FunctionCode(SUBFILEINIT),
  m_CpResultCode(CpReplyCode::CP_OK),
  m_fileOrder(0),
  m_OutputBufferLength(0),
  m_OutputBufferPosition(0),
  m_OutputBuffer(NULL),
  m_OutputBufferPtr(NULL)
{
	// allocate buffer of bufferLength + 1 size
	m_InputBuffer = new char[m_InputBufferLength];

	// copy the buffer
 	memcpy(m_InputBuffer, buffer, bufferLength);

	m_InputBufferPtr = m_InputBuffer;

	m_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_JTPCpMsg");
}

/*============================================================================
	ROUTINE: ~FMS_CPF_JTPCpMsg
 ============================================================================ */
FMS_CPF_JTPCpMsg::~FMS_CPF_JTPCpMsg()
{
	if(NULL != m_InputBuffer)
		delete[] m_InputBuffer;

	if(NULL != m_OutputBuffer)
		delete[] m_OutputBuffer;

	if(NULL != m_trace)
		delete m_trace;
}

/*============================================================================
	ROUTINE: putByte
 ============================================================================ */
bool FMS_CPF_JTPCpMsg::putByte(uint8_t byte)
{
	TRACE(m_trace, "Entering putByte(<%i>)", byte);

	bool result = true;
	if(m_OutputBufferLength <= m_OutputBufferPosition)
	{
		result = false;
		char msgBuf[256] = {0};
		snprintf(msgBuf, 255, "FMS_CPF_JTPCpMsg::putByte(), output buffer length:<%i>, buffer position:<%i> failed to put char:<%i>",
					m_OutputBufferLength,
					m_OutputBufferPosition,
					byte);
		CPF_Log.Write(msgBuf, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", msgBuf);
	}
	else
	{
		// assign the value
		(*m_OutputBufferPtr) = byte;
		// increase buffer pointer
		m_OutputBufferPtr++;
		m_OutputBufferPosition++;
	}

	TRACE(m_trace, "%s", "Leaving putByte()");
	return result;
}

/*============================================================================
	ROUTINE: putWord
 ============================================================================ */
bool FMS_CPF_JTPCpMsg::putWord(uint16_t word)
{
	TRACE(m_trace, "%s", "Entering putWord()");

	// APFPI is set for big endian
	uint8_t highByte =  static_cast<uint8_t>(word / 256);
	uint8_t lowByte =  static_cast<uint8_t>(word & 255);

	TRACE(m_trace, "putWord(), w:<%i> -> b0:<%i> b1:<%i>", word, lowByte, highByte);
	bool result = (putByte(highByte) && putByte(lowByte)) ;

	if(!result)
	{
		char msgBuf[256] = {0};
		snprintf(msgBuf, 255, "FMS_CPF_JTPCpMsg::putWord(), failed to put word:<%i>, lowByte:<%i>, highByte:<%i>",
					word,
					lowByte,
					highByte);
		CPF_Log.Write(msgBuf, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", msgBuf);
	}

	TRACE(m_trace, "%s", "Leaving putWord()");
	return result;
}

/*============================================================================
	ROUTINE: putLong
 ============================================================================ */
bool FMS_CPF_JTPCpMsg::putLong(uint32_t number)
{
	TRACE(m_trace, "%s", "Entering putLong()");

	uint16_t lowWord = static_cast<uint16_t>(number & 65535);
	uint16_t highWord = static_cast<uint16_t>(number / 65536);

	TRACE(m_trace, "putLong(), n:<%i> -> w0:<%i> w1:<%i>", number, lowWord, highWord);

	// low order word first
	// according to FUFILELISTR signal description
	bool result = ( putWord(lowWord) && putWord(highWord) );

	if(!result)
	{
		char msgBuf[256] = {0};
		snprintf(msgBuf, 255, "FMS_CPF_JTPCpMsg::putLong(), failed to put word32:<%i>, lowWord:<%i>, highWord:<%i>",
					number,
					lowWord,
					highWord);
		CPF_Log.Write(msgBuf, LOG_LEVEL_ERROR);
		TRACE(m_trace, "%s", msgBuf);
	}
	TRACE(m_trace, "%s", "Leaving putLong()");
	return result;
}

/*============================================================================
	ROUTINE: putDatetime
 ============================================================================ */
bool FMS_CPF_JTPCpMsg::putDatetime(const char* date)
{
	TRACE(m_trace, "%s", "Entering putDatetime()");

	bool result = true;
	unsigned short value;
	TRACE(m_trace, "putDatetime(), date:<%s>", date);

	std::string strDate(date);
	if( strDate.empty() )
	{
		for(int idx = 0; idx < 5; ++idx)
		{
			value = 0;
			if(!putByte(value))
			{
				char msgBuf[256] = {0};
				snprintf(msgBuf, 255, "FMS_CPF_JTPCpMsg::putDatetime(), failed to put data:<%s>, value:<%i>, index:<%i>",
							strDate.c_str(),
							value,
							idx);
				CPF_Log.Write(msgBuf, LOG_LEVEL_ERROR);
				TRACE(m_trace, "%s", msgBuf);
			}
		}
	}
	else
	{

		//date format yyyymmddhhmm
		// CP handles only 2 digit years, cut first 2 digits
		std::string partialDate = strDate.substr(2);
		size_t numDigit = 2;

		// 0 -> year, 1 -> month, 2 -> day, 3 -> hour, 4 -> minutes
		// 2 digits each element
		for(int idx = 0; idx < 5; ++idx)
		{
			try
			{
				std::istringstream str2num(partialDate.substr(idx*numDigit, numDigit));
				str2num >> value;
			}
			catch(std::exception& ex)
			{
				value = 0;
				CPF_Log.Write("FMS_CPF_JTPCpMsg::putDatetime(), exception on date string", LOG_LEVEL_ERROR);
			}

			if(!putByte(value))
			{
				char msgBuf[256] = {0};
				snprintf(msgBuf, 255, "FMS_CPF_JTPCpMsg::putDatetime(), failed to put data:<%s>, value:<%i>, index:<%i>",
							strDate.c_str(),
							value,
							idx);
				CPF_Log.Write(msgBuf, LOG_LEVEL_ERROR);
				TRACE(m_trace, "%s", msgBuf);
			}
		}
	}

	TRACE(m_trace, "%s", "Leaving putDatetime()");
	return result;
}

/*============================================================================
	ROUTINE: putBlock
 ============================================================================ */
bool FMS_CPF_JTPCpMsg::putBlock(const char* buffer, const unsigned short length)
{
	TRACE(m_trace, "%s", "Entering putBlock()");

	bool result = putByte(length);

	if(result)
	{
		for(int idx = 0; idx < length; ++idx)
		{
			if(!putByte(buffer[idx]))
			{
				result = false;

				char msgBuf[256] = {0};
				snprintf(msgBuf, 255, "FMS_CPF_JTPCpMsg::putBlock(), failed to put block, value:<%i>, index:<%i>, length:<%i>",
							buffer[idx],
							idx,
							length);
				CPF_Log.Write(msgBuf, LOG_LEVEL_ERROR);
				TRACE(m_trace, "%s", msgBuf);
				break;
			}
		}
	}

	TRACE(m_trace, "%s", "Leaving putBlock()");
	return result;
}

/*============================================================================
	ROUTINE: putString
 ============================================================================ */
bool FMS_CPF_JTPCpMsg::putString(const std::string& dataBlock)
{
	TRACE(m_trace, "%s", "Entering putString()");
	bool result = putBlock(dataBlock.c_str(), dataBlock.length());

	TRACE(m_trace, "%s", "Leaving putString()");
	return result;
}

/*============================================================================
	ROUTINE: getByte
 ============================================================================ */
uint8_t FMS_CPF_JTPCpMsg::getByte()
{
	TRACE(m_trace, "%s", "Entering getByte()");

	uint8_t value;

	if(m_InputBufferLength <= m_InputBufferPosition )
	{
		value = 0;
		TRACE(m_trace, "%s", "getByte(), buffer overrun");
	}
	else
	{
		value = (*m_InputBufferPtr) & 255;
		m_InputBufferPtr++;
		m_InputBufferPosition++;
	}

	TRACE(m_trace, "Leaving getByte(), value:<%i>", value);
	return value;
}

/*============================================================================
	ROUTINE: getWord
 ============================================================================ */
uint16_t FMS_CPF_JTPCpMsg::getWord()
{
	TRACE(m_trace, "%s", "Entering getWord()");

	uint16_t value;
	// APFPI is programmed for big endian
	value = (getByte() * 256) | getByte();

	TRACE(m_trace, "Leaving getWord(), value:<%i>", value);

	return value;
}

/*============================================================================
	ROUTINE: getWord
 ============================================================================ */
uint32_t FMS_CPF_JTPCpMsg::getLong()
{
	TRACE(m_trace, "%s", "Entering getLong()");

	uint32_t value;

	// least significant first
	// according to FUFILELISTR signal description
	value = getWord() | (getWord() * 65536);

	TRACE(m_trace, "Leaving getLong(), value:<%i>", value);

	return value;
}

/*============================================================================
	ROUTINE: getWord
 ============================================================================ */
void FMS_CPF_JTPCpMsg::decode()
{
	TRACE(m_trace, "%s", "Entering decode()");

	// Refer to IWD AP file processing 2/155 19-ANZ 217 30
	// current buffer position return the Header size
	unsigned short headerSize = getByte();
	if(0 != headerSize)
	{
		// Get header from Cp buffer
		char msgHeader[headerSize];
		getBlock(msgHeader, headerSize);
		m_FunctionCode = (CpFunctionCode) msgHeader[4];
	}
	TRACE(m_trace, "decode(), function code:<%i>, header size:<%i>", m_FunctionCode, headerSize);

	// current buffer position return the filename size
	unsigned short fileNameSize = getByte();
	if(0 != fileNameSize)
	{
		// Get header from Cp buffer
		char fileName[fileNameSize+1];
		getBlock(fileName, fileNameSize);
		fileName[fileNameSize] = 0;
		m_FileName = fileName;
	}
	TRACE(m_trace, "decode(), file name:<%s>, length:<%i>", m_FileName.c_str(), fileNameSize);

	// current buffer position return the subfilename size
	unsigned short subFileNameSize = getByte();
	if(0 != subFileNameSize)
	{
		// Get header from Cp buffer
		char subFileName[subFileNameSize+1];
		getBlock(subFileName, subFileNameSize);
		subFileName[subFileNameSize] = 0;
		m_SubFileName = subFileName;
	}

	TRACE(m_trace, "decode(), subFile name:<%s>, length:<%i>", m_SubFileName.c_str(), subFileNameSize);

	// current buffer position return the generation name size
	unsigned short genNameSize = getByte();
	if(0 != genNameSize)
	{
		// Get header from Cp buffer
		char generationName[genNameSize+1];
		getBlock(generationName, genNameSize);
		generationName[genNameSize] = 0;
		m_GenarationName = generationName;
	}

	TRACE(m_trace, "decode(), file generation name:<%s>, length:<%i>", m_GenarationName.c_str(), genNameSize);

	// current buffer position return the generation name size
	unsigned short destinationNameSize = getByte();
	if(0 != destinationNameSize)
	{
		// Get header from Cp buffer
		char destinationName[destinationNameSize+1];
		getBlock(destinationName, destinationNameSize);
		destinationName[destinationNameSize] = 0;
		m_DestinationName = destinationName;
	}

	TRACE(m_trace, "decode(), destination name:<%s>, length:<%i>", m_DestinationName.c_str(), destinationNameSize);
	// check for extra parameter
	if(FUFILELIST == m_FunctionCode )
	{
		m_fileOrder = getByte();
		TRACE(m_trace, "decode(), full file list order:<%i>",  m_fileOrder);
	}

	TRACE(m_trace, "%s", "Leaving decode()");
}

/*============================================================================
	ROUTINE: getBlock
 ============================================================================ */
void FMS_CPF_JTPCpMsg::encode()
{
	TRACE(m_trace, "%s", "Entering encode()");

	allocateOutputBuffer();

	// 0 <- size of header
	putByte(APFPprotocol::headerSize);

	uint16_t extraDataSize = m_OutputBufferLength - (APFPprotocol::headerSize + 1);
	// 1,2 <- size of extra buffer
	putWord(extraDataSize);

	// 3 <- function code is 1 more than it was from the CP
	uint8_t replyFunctionCode = m_FunctionCode + 1;
	putByte(replyFunctionCode);

	// 4 <- priority
	putByte(APFPprotocol::priority);

	// 5,6 <- result code
	putWord(m_CpResultCode);

	TRACE(m_trace, "encode(), extraData size:<%d>, replyFunctionCode:<%d>, Cp reply Code:<%d>", extraDataSize, replyFunctionCode, m_CpResultCode);

	if(FUFILELIST == m_FunctionCode )
	{
		TRACE(m_trace, "%s", "encode(), file status info");

		// length + filename
		putString(m_FileName);

		// length + subfilename
		putString(m_SubFileName);

		// length + generation
		putString(m_GenarationName);

		// length + destination
		putString(m_DestinationName);

		// Time when automatic transfer started
		putDatetime(m_FileStatusInfo.dateAutoSendStart);

		// Time when automatic transfer finished
		putDatetime(m_FileStatusInfo.dateAutoSendEnd);

		// Time when manual transfer started
		putDatetime(m_FileStatusInfo.dateManSendStart);

		// Time when manual transfer finished
		putDatetime(m_FileStatusInfo.dateManSendEnd);

		// Time when automatic transfer failed
		putDatetime(m_FileStatusInfo.dateAutoSendFail);

		// Time when dumped on primary media
		putDatetime(m_FileStatusInfo.dateDumpedOnPrimary);

		// Time when dumped on secondary media, always 00000
		putDatetime(m_FileStatusInfo.dateDumpedOnSecondary);

		// Reason why transfer failed
		putByte(m_FileStatusInfo.reasonForFailure);

		// subfile size in number of records
		putLong(m_FileStatusInfo.subFileSize);

		//Sequence number, always 0
		uint16_t sequenceNumber = 0;
		putWord(sequenceNumber);
	}

	TRACE(m_trace, "%s", "Leaving encode()");
}

/*============================================================================
	ROUTINE: setCpResultCode
 ============================================================================ */
void FMS_CPF_JTPCpMsg::setCpResultCode(const unsigned int& opertationResult)
{
	TRACE(m_trace, "%s", "Entering setCpResultCode()");
	switch(opertationResult)
	{
			case AES_OHI_NOERRORCODE:
				// 0 -> 0
				m_CpResultCode =  CpReplyCode::CP_OK;
				break;

			case AES_OHI_NOPROCORDER:
				// 11 -> 49
				m_CpResultCode =  CpReplyCode::CP_FILEUNKNOWN;
				break;

			// Fall through	12 or 13 -> 35
			case AES_OHI_FILENOTFOUND:
			case AES_OHI_FILENAMEINVALID:
				m_CpResultCode =  CpReplyCode::CP_FILENOTEXIST;
				break;

			// Fall through  22 or 23 -> 60
			case AES_OHI_NODESTINATION:
			case AES_OHI_INVALIDDESTNAME:
				m_CpResultCode =  CpReplyCode::CP_DESTUNKNOWN;
				break;

			case AES_OHI_NOACCESS:
				// 24 -> 65
				m_CpResultCode =  CpReplyCode::CP_FILEACCESSERR;
				break;

			// Fall through	 18 or 19 -> 66
			case AES_OHI_NOSUCHITEM:
			case AES_OHI_SENDITEMNOTREP:
				m_CpResultCode =  CpReplyCode::CP_SUBFILEUNKNOWN;
				break;

			// Fall through	103 or 104 -> 67
			case AES_OHI_SUBFILENOTFOUND:
			case AES_OHI_SUBFILENAMEINVALID:
				m_CpResultCode =  CpReplyCode::CP_SUBFILENOTEXIST;
				break;

			case AES_OHI_SENDITEMEXIST:
				// 17 -> 68
				m_CpResultCode =  CpReplyCode::CP_SUBFILEALREADYDEF;
				break;

			default:
				// others -> 9
				m_CpResultCode =  CpReplyCode::CP_OTHERFAULT;
		}
	TRACE(m_trace, "Leaving setCpResultCode(), OHIcode:<%d> -> Cp result code:<%d>", opertationResult, m_CpResultCode);
}


/*============================================================================
	ROUTINE: getBlock
 ============================================================================ */
void FMS_CPF_JTPCpMsg::getBlock(char* buffer, const unsigned short length)
{
	TRACE(m_trace, "%s", "Entering getBlock()");

	for(unsigned short idx = 0; idx < length; ++idx)
	{
		buffer[idx] = getByte();
	}

	TRACE(m_trace, "%s", "Leaving getBlock()");
}

/*============================================================================
	ROUTINE: getBlock
 ============================================================================ */
void FMS_CPF_JTPCpMsg::allocateOutputBuffer()
{
	m_OutputBufferLength = APFPprotocol::headerSize + 1;

	if(FUFILELIST == m_FunctionCode )
	{
		unsigned short extraDataSize = APFPprotocol::fullInfoSize;

		extraDataSize += ( m_FileName.length() + m_SubFileName.length() +
						  m_GenarationName.length() + m_DestinationName.length() );

		m_OutputBufferLength += extraDataSize;
	}

	// allocate buffer of bufferLength size
	m_OutputBuffer = new char[m_OutputBufferLength];

	// copy the buffer
	memset(m_OutputBuffer, 0, m_OutputBufferLength);

	// set initial pointer position
	m_OutputBufferPtr = m_OutputBuffer;

	TRACE(m_trace, "allocateOutputBuffer(), size:<%i>",  m_OutputBufferLength);
}

