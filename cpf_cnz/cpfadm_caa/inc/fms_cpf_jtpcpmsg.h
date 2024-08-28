//**********************************************************************
//
//
/** @file fms_cpf_jtpcpmsg.h
 *	@brief
 *	Header file for FMS_CPF_JTPCpMsg class.
 *  This module contains the declaration of the class FMS_CPF_JTPCpMsg.
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
 *	| 1.0.0  | 2011-08-07 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_JTPCPMSG_H_
#define FMS_CPF_JTPCPMSG_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include <string>
#include <stdint.h>
/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class ACS_TRA_trace;

/*=====================================================================
						CLASS DECLARATION SECTION
==================================================================== */
// Error codes to CP
namespace CpReplyCode{
	const uint16_t CP_OK = 0;
	const uint16_t CP_OTHERFAULT = 9;
	const uint16_t CP_FILENOTEXIST = 35;
	const uint16_t CP_FILEUNKNOWN =	49;
	const uint16_t CP_DESTUNKNOWN =	60;
	const uint16_t CP_FILEACCESSERR	= 65;
	const uint16_t CP_SUBFILEUNKNOWN = 66;
	const uint16_t CP_SUBFILENOTEXIST = 67;
	const uint16_t CP_SUBFILEALREADYDEF = 68;
}

class FMS_CPF_JTPCpMsg
{
  public:

	struct statusInfo{

		char dateAutoSendStart[20]; // Time when automatic transfer started
		char dateAutoSendEnd[20];   // Time when automatic transfer finished
		char dateManSendStart[20];  // Time when manual transfer started
		char dateManSendEnd[20];    // Time when manual transfer finished
		char dateAutoSendFail[20];  // Time when automatic transfer failed
		char dateDumpedOnPrimary[20];	 // Time when dumped on primary media
		char dateDumpedOnSecondary[20];  // Time when dumped on secondary media

		uint8_t reasonForFailure; // Reason why automatic transfer failed
		uint32_t subFileSize;  // Subfile size in number of records

		// initialize structure members
		statusInfo();
	};

	// Message types from/to CP
    enum CpFunctionCode{
		CREATEFILE = 1,	  // debug
		SUBFILEREM = 9,	  // REMOVESENDITEM
		SUBFILEINIT = 15, // CREATESENDITEM
		SETSFSTAT = 17,   // SETITEMARCHIVE
		FUFILELIST = 19,  // GETSTATUSITEM
	};

	/** @brief Constructor of FMS_CPF_JTPCpMsg class
	 *
	 *	Allocates and initiate an internal buffer with supplied
	 *	data.
	 *	@param buffer: IN, pointer to a buffer that will be copied to the
	 *	internal buffer.
	 *	@param bufferLength: IN, length of the data to be copied
	 *
	 *	@remarks Remarks
	*/
	FMS_CPF_JTPCpMsg(const char* buffer, const int bufferLength);

	/**	@brief	destructor of FMS_CPF_JTPCpMsg class
	*/
    virtual ~FMS_CPF_JTPCpMsg();

    /** @brief decode method
	 *
	 *	This method decodes the received buffer to get function code,
	 *	the file name, subfile name, generation name and destination name
	 *	according to the CP-AP IWD
	 *
	 *	@remarks Remarks
	*/
    void decode();

    /** @brief decode method
	 *
	 *	This method encodes the requested info to reply to the Cp request
	 *	according to the CP-AP IWD
	 *
	 *	@remarks Remarks
	*/
	void encode();

    /** @brief setCpResultCode method
	 *
	 *	This method translates the OHI error to a Cp result code .
	 *
	 *  @param opertationResult: OHI result code.
	 *
	 *	@remarks Remarks
	*/
    void setCpResultCode(const unsigned int& opertationResult);

    /** @brief getOutputBufferSize method
   	 *
   	 *	This method retrieves the size of output buffer.
   	 *
   	 *  @return Current position.
   	 *
   	 *	@remarks Remarks
   	*/
    unsigned short getOutputBufferSize() const { return m_OutputBufferLength; };

    /** @brief getInputBuffer method
	 *
	 *	This method returns the pointer to the internal input buffer
	 *
	 *  @return char pointer.
	 *
	 *	@remarks Remarks
	*/
	char* getOutputBuffer() const { return m_OutputBuffer; };

    /** @brief getFunctionCode method
	 *
	 *	This method returns the function code decoded from cp buffer
	 *
	 *  @return CpFunctionCode.
	 *
	 *	@remarks Remarks
	*/
    CpFunctionCode getFunctionCode() const { return m_FunctionCode; };

    /** @brief getFileName method
	 *
	 *	This method returns the file name decoded from cp buffer
	 *
	 *  @return char pointer.
	 *
	 *	@remarks Remarks
	*/
    const char* getFileName() const {return m_FileName.c_str(); };

    /** @brief getSubFileName method
	 *
	 *	This method returns the sub file name decoded from cp buffer
	 *
	 *  @return char pointer.
	 *
	 *	@remarks Remarks
	*/
	const char* getSubFileName() const {return m_SubFileName.c_str(); };

	/** @brief getSubFileName method
	 *
	 *	This method checks if the sub file name is empty
	 *
	 *  @return bool.
	 *
	 *	@remarks Remarks
	*/
	bool isSubFileEmpty() const { return m_SubFileName.empty(); };

	/** @brief getFileGeneration method
	 *
	 *	This method returns the file generation name decoded from Cp buffer
	 *
	 *  @return char pointer.
	 *
	 *	@remarks Remarks
	*/
	const char* getFileGeneration() const {return m_GenarationName.c_str(); };

	/** @brief getDestinationName method
	 *
	 *	This method returns the destination name decoded from cp buffer
	 *
	 *  @return char pointer.
	 *
	 *	@remarks Remarks
	*/
	const char* getDestinationName() const {return m_DestinationName.c_str(); };

	/** @brief getDestinationName method
	 *
	 *	This method returns the file order decoded from Cp buffer
	 *
	 *  @return file order.
	 *
	 *	@remarks Remarks
	*/
	unsigned short getFileOrder() const { return m_fileOrder; };

	void setSubFileName(const char* subfile) {m_SubFileName = subfile; }; //HU30480

	/** @brief setFileStatusInfo method
	 *
	 *	This method set file status info
	 *
	 *  @return file order.
	 *
	 *	@remarks Remarks
	*/
	void setFileStatusInfo(const statusInfo& fileInfo) { m_FileStatusInfo = fileInfo; };

  private:

    /** @brief putByte method
	 *
	 *	This method stores one unsigned short int at current position and
     *	advances the pointer.
	 *
	 *	@param byte: IN, value to store.
	 *
	 *  @return true On success, on false FAILURE.
	 *
	 *	@remarks Remarks
	*/
	bool putByte(uint8_t byte);

	/** @brief putWord method
	 *
	 *	This method stores one unsigned int at current position and
	 *	advances the pointer.
	 *
	 *	@param word: IN, value to store.
	 *
	 *  @return true On success, on false FAILURE.
	 *
	 *	@remarks Remarks
	*/
	bool putWord(uint16_t word);

	/** @brief putLong method
	 *
	 *	This method stores one unsigned long at current position and
	 *	advances the pointer.
	 *
	 *	@param number: IN, value to store.
	 *
	 *  @return true On success, on false FAILURE.
	 *
	 *	@remarks Remarks
	*/
	bool putLong(uint32_t number);

	/** @brief putDatetime method
	 *
	 *	This method stores one date at current position and
	 *	advances the pointer.
	 *
	 *	@param date: IN, value to store.
	 *
	 *  @return true On success, on false FAILURE.
	 *
	 *	@remarks Remarks
	*/
	bool putDatetime(const char* date);

	/** @brief putBlock method
	 *
	 *	This method stores length + data of the supplied buffer and advances
     *	the pointer.
	 *
	 *	@param buffer: IN, pointer to data that should be stored.
	 *	@param length: IN, length of data
	 *
	 *  @return true On success, on false FAILURE.
	 *
	 *	@remarks Remarks
	*/
    bool putBlock(const char* buffer, const unsigned short length);

    /** @brief putString method
	 *
	 *	This method stores length + data of string and advances
     *	the pointer.
	 *
	 *	@param dataBlock: IN, string that should be stored.
	 *
	 *  @return true On success, on false FAILURE.
	 *
	 *	@remarks Remarks
	*/
    bool putString(const std::string& dataBlock);

    /** @brief getByte method
	 *
	 *	This method reads one unsigned short int from the internal buffer
	 *	and advances the pointer.
	 *
	 *  @return Retrieved data.
	 *
	 *	@remarks Remarks
	*/
    uint8_t getByte();

    /** @brief getWord method
	 *
	 *	This method reads one unsigned int from the internal buffer
	 *	and advances the pointer.
	 *
	 *  @return Retrieved data.
	 *
	 *	@remarks Remarks
	*/
    uint16_t getWord();

    /** @brief getLong method
	 *
	 *	This method reads one unsigned long from the internal buffer
	 *	and advances the pointer.
	 *
	 *  @return Retrieved data.
	 *
	 *	@remarks Remarks
	*/
    uint32_t getLong();

    /** @brief getBlock method
	 *
	 *	This method reads length + a character array from  the current
	 *	position in the buffer and advances the pointer to one
	 *	position beyond the arrays end.
	 *
	 *	@param buffer: IN, reference to store retrieved block.
	 *
	 *	@remarks Remarks
	*/
	void getBlock(char* buffer, const unsigned short length);

	/** @brief getBlock method
	 *
	 *	This method allocates the output buffer
	 *
	 *	@remarks Remarks
	*/
	void allocateOutputBuffer();

    /** @brief	m_bufferLength
	 *
	 *  The length of the internal buffer.
	 *
	 *	@remarks Remarks
	*/
	const int m_InputBufferLength;

	/** @brief	m_bufferPosition
	 *
	 *  The current position in the buffer, relative to the
	 *	beginning of the buffer.
	 *
	 *	@remarks Remarks
	*/
	int m_InputBufferPosition;

    /** @brief
   	 *  Retrieved function code.
   	*/
    CpFunctionCode m_FunctionCode;

    /** @brief	m_CpResultCode
	 *
	 *  The result code to return to the CP.
	 *
	 *	@remarks Remarks
	*/
    uint16_t m_CpResultCode;

    /** @brief
     *  Retrieved file order info.
    */
    unsigned short m_fileOrder;

    /** @brief	m_OutputBufferLength
	 *
	 *  The length of the internal output buffer.
	 *
	 *	@remarks Remarks
	*/
    unsigned short m_OutputBufferLength;

	/** @brief	m_OutputBufferPosition
	 *
	 *  The current position in the output buffer, relative to the
	 *	beginning of the buffer.
	 *
	 *	@remarks Remarks
	*/
	unsigned short m_OutputBufferPosition;

	/** @brief	m_OutputBuffer
	 *
	 *  The allocated internal buffer to handle output data.
	 *
	 *	@remarks Remarks
	*/
	char* m_OutputBuffer;

	/** @brief	m_bufferPtr
	 *
	 *  The pointer to the current position in the output buffer.
	 *
	 *	@remarks Remarks
	*/
	char* m_OutputBufferPtr;

    /** @brief	m_InputBuffer
	 *
	 *  The allocated internal buffer to handle input data.
	 *
	 *	@remarks Remarks
	*/
	char* m_InputBuffer;

	/** @brief	m_bufferPtr
	 *
	 *  The pointer to the current position in the input buffer.
	 *
	 *	@remarks Remarks
	*/
	char* m_InputBufferPtr;

    /** @brief
	 *  Retrieved Cp file name.
	*/
    std::string m_FileName;

    /** @brief
	 *  Retrieved Cp subfile name.
	*/
    std::string m_SubFileName;

    /** @brief
	 *  Retrieved Cp file generation name.
	*/
    std::string m_GenarationName;

    /** @brief
	 *  Retrieved destination name.
	*/
    std::string m_DestinationName;

    statusInfo m_FileStatusInfo;

    /**	@brief	trautil object trace
    */
    ACS_TRA_trace* m_trace;
};


#endif //FMS_CPF_JTPCPMSG_H_
