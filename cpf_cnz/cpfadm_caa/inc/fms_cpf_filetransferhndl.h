/*
 * * @file fms_cpf_filetransferhndl.h
 *	@brief
 *	Header file for FMS_CPF_FileTransferHndl class.
 *  This module contains the declaration of the class FMS_CPF_FileTransferHndl.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-03-14
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
 *	| 1.0.0  | 2012-03-14 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_FILETRANSFERHNDL_H_
#define FMS_CPF_FILETRANSFERHNDL_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "aes_ohi_filehandler.h"
#include <ace/RW_Thread_Mutex.h>
#include <string>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class FMS_CPF_FileTransferHndl
{

 public:

	/**
	 * 	@brief	Constructor of FMS_CPF_FileTransferHndl class
	*/
	FMS_CPF_FileTransferHndl(const std::string& infiniteFileName, const std::string& mainFilePath, const std::string& cpName);

	/**
	 * 	@brief	Destructor of FMS_CPF_FileTransferHndl class
	*/
	virtual ~FMS_CPF_FileTransferHndl();

	/** @brief setFileTQ method
	 *
	 *	This method sets the file transfer queue to use
	 *
	 *  @param currentTQ: the name of file transfer queue
	 *
	 *  @return SUCCESS on success, otherwise an error code
	 */
	int setFileTQ(const std::string& currentTQ);

	/** @brief moveFileToTransfer method
	 *
	 *	This method moves and renames the file form CP file system to the transfer queue folder
	 *
	 *  @param infiniteSubFile: the current infinite subfile to move
	 *
	 *  @return SUCCESS on success, otherwise an error code
	 */
	int moveFileToTransfer(const unsigned long infiniteSubFile);

	/** @brief moveFileToTransfer method
	 *
	 *	This method moves and renames the file form CP file system to the transfer queue folder
	 *
	 *  @param infiniteSubFile: the current infinite subfile to move
	 *
	 *  @return SUCCESS on success, otherwise an error code
	 */
	int moveFileToTransfer(const std::string& subFileName);

	/** @brief sendCurrentFile method
	 *
	 *	This method send the current file
	 *
	 *  @return true on success, otherwise false
	 */
	bool sendCurrentFile(bool isRelCmdHdf = false);

	/** @brief getOhiErrotText method
	 *
	 *	This method get last ohi error text
	 *
	 * @param errorText: OHI error text
	 */
	void getOhiErrotText(std::string& errorText);

	/** @brief getOhiError method
	 *
	 *	This method get last ohi error
	 *
	 */
	int getOhiError() { return m_lastOhiError; };

        /** @brief linkFileToTransfer method
         *
         *      This method create hardlink of the file form CP file system to the transfer queue folder
         *
         *  @param infiniteSubFile: the current regular subfile to link
         *
         *  @return SUCCESS on success, otherwise an error code
         */
        int linkFileToTransfer(const std::string& subFileName);

 private:

	/** @brief createTQHandler method
	 *
	 *	This method creates the file TQ object handler and attach it to GOH
	 *
	 *	@param tqName: the name of file transfer queue
	 *
	 *  @return SUCCESS on success, otherwise an error code
	 *
	*/
	int createTQHandler(const std::string& tqName);

	/** @brief checkFilesExist method
	 *
	 *	This method checks if the file to transfer or the infinite subfile are present
	 *
	*/
	bool checkFilesExist();

	/** @brief moveWithTimeStamp method
	 *
	 *	This method moves and renames with timestamp the file form CP file system to the transfer queue folder
	 *
	*/
	int moveWithTimeStamp();

	/** @brief removeTQHandler method
	 *
	 *	This method remove the file transfer object
	 *
	*/
	void removeTQHandler();

	/** @brief makeFileNameToSend method
	 *
	 *	This method prepares the name of the file to send
	 *
	 *	@param subFile: the name of subfile
	 *
	*/
	void makeFileNameToSend(const char* subFile);

	/**
	 * 	@brief	m_infiniteFileName: the infinite file name
	*/
	std::string m_infiniteFileName;

	/**
	 * 	@brief	m_infiniteFilePath: the infinite file path into CP file system
	*/
	std::string m_infiniteFilePath;

	/**
	 * 	@brief	cpName: the CP name
	*/
	std::string m_cpName;

	/**
	 * 	@brief	m_fileTQ: transfer queue name
	*/
	std::string m_fileTQ;

	/**
	 * 	@brief	m_TQPath: transfer queue folder
	*/
	std::string m_TQPath;

	/**
	 * 	@brief	m_fileToSend: current file to transfer
	*/
	std::string m_fileToSend;

	/**
	 * 	@brief	m_srcFilePath: path of source file
	*/
	std::string m_srcFilePath;

	/**
	 * 	@brief	m_dstFilePath: path of destination file
	*/
	std::string m_dstFilePath;

	/**
	 * 	@brief	m_subfileMoved: current subfile moved
	*/
	unsigned long m_currentSubFile;

	/**
	 * 	@brief	m_isFaultyStatus: current TQ handler status
	*/
	bool m_isFaultyStatus;

	/**
	 * 	@brief	m_subfileMoved: current subfile moved
	*/
	unsigned int m_lastOhiError;

	/**
	 * 	@brief	m_ohiFileHandler: OHI file handler object
	*/
	AES_OHI_FileHandler* m_ohiFileHandler;

	/**
	 * 	@brief	m_trace: trace object
	*/
	ACS_TRA_trace* m_trace;
};

#endif /* FMS_CPF_FILETRANSFERHNDL_H_ */
