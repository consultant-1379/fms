/*
 * * @file fms_cpf_filehandler.h
 *	@brief
 *	Header file for File class.
 *  This module contains the declaration of the class File.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-07-06
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
 *	| 1.0.0  | 2011-07-06 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FILE_H
#define FILE_H

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_fileid.h"
#include "fms_cpf_attribute.h"

#include <string>
#include <list>
#include <set>

class FileAccess;
class FMS_CPF_FileTransferHndl;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class File
{
 public:

	friend class FileTable;
	friend class FileDescriptor;
	friend class FileAccess;

	File(const FMS_CPF_FileId& fileid,
		 const std::string& volume,
		 const std::string& cpname,
		 const FMS_CPF_Attribute& attribute );

	~File();

	bool operator== (const File& file) const;
	bool operator== (const File* file) const;

	inline FMS_CPF_FileTransferHndl* getFileTQHandler(){ return m_FileTQHandler; };

	inline const char* getFileName() const { return fileid_.data(); };

	inline const char* getVolumeName() const { return volume_.c_str(); };

	inline const char* getCpName() const { return cpname_.c_str(); };

	inline FMS_CPF_Types::fileType getFileType() const { return m_FileType; };

	bool createFileTQHandler();

	void removeFileTQHandler();

 private:

	File(const FMS_CPF_FileId& fileid);

	void addSubFile(const std::string& subFileName);

	void removeSubFile(const std::string& subFileName);

	void changeSubFile(const std::string& oldSubFileName, const std::string& newSubFileName);

	void getAllSubFile(std::list<FMS_CPF_FileId>& subFileList);

	void getAllSubFile(std::set<std::string>& subFileList);

	bool subFileExist(const std::string& subFileName);

	void setImmState(bool state) { m_ImmStateVerified = state; };

	bool getImmState() const { return m_ImmStateVerified; };

	void setFileDN(const std::string& fileDN) { m_fileDN = fileDN; };

	const char* getFileDN() { return m_fileDN.c_str(); };

	// Get the TQ DN
	const char* getTransferQueueDN() { return m_TransferQueueDN.c_str(); };

	// Set TQ DN
	void setTransferQueueDN(const std::string& tqDN) { m_TransferQueueDN = tqDN; };

	// Clear TQ DN
	void clearTransferQueueDN() { m_TransferQueueDN.clear(); };

	FMS_CPF_FileId fileid_;
	std::string volume_;
	std::string cpname_;
	FMS_CPF_Attribute attribute_;

	std::string m_fileDN;

	// Only for infinite file attached to a transfer queue
	std::string m_TransferQueueDN;

	FMS_CPF_Types::fileType m_FileType;

	FMS_CPF_FileTransferHndl* m_FileTQHandler;

	bool gohBlockHandlerExist;

	FileAccess* faccessp_;

	std::set<std::string> m_SubFiles;

	bool m_ImmStateVerified;

	/*
	* subfilelist_ is used by FileTable
	*/
	typedef std::list<FileAccess*> SubFileList;
	SubFileList subfilelist_;

};



#endif
