/*
 * * @file fms_cpf_filetable.h
 *	@brief
 *	Header file for FileTable class.
 *  This module contains the declaration of the class FileTable.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-07-05
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
 *	| 1.0.0  | 2011-07-05 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FILE_TABLE_H
#define FILE_TABLE_H

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_privateexception.h"
#include "fms_cpf_fileid.h"
#include "fms_cpf_attribute.h"
#include "fms_cpf_types.h"

#include "ACS_CC_Types.h"

#include <string>
#include <vector>
#include <list>
#include <set>

#include <ace/Thread_Mutex.h>

class File;
class FileReference;
class FileAccess;

class ACS_TRA_trace;
class OmHandler;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FileTable
{
  public:

	friend class FileDescriptor;
	friend class FMS_CPF_DirectoryStructureMgr;

	/**
	 * 	@brief	Constructor of FileTable class
	*/
	FileTable(std::string _cpname, bool isMultiCp);

	/**
	 * 	@brief	Destructor of FileTable class
	*/
	virtual ~FileTable();

	File* find(const FMS_CPF_FileId id);

	bool exists(const FMS_CPF_FileId& fileid) throw( FMS_CPF_PrivateException);

	FileReference create(const FMS_CPF_FileId& fileid,
							const std::string& volume,
							const FMS_CPF_Attribute& attribute,
							FMS_CPF_Types::accessType access,
							const std::string& fileDN)
		throw (FMS_CPF_PrivateException);

	FileReference create(const FMS_CPF_FileId& fileid, FMS_CPF_Types::accessType access)
		throw(FMS_CPF_PrivateException);

	void close(FileReference ref) throw(FMS_CPF_PrivateException);

	FileReference open(const FMS_CPF_FileId& fileid,
						 FMS_CPF_Types::accessType access,
						 bool inf = false)
		throw(FMS_CPF_PrivateException);


	void remove (FileReference ref)	throw (FMS_CPF_PrivateException);

	void removeBlockSender(FileReference reference ) throw (FMS_CPF_PrivateException);

	int getListFileId(const FMS_CPF_FileId& fileid, std::list<FMS_CPF_FileId>& fileList)
		throw (FMS_CPF_PrivateException);

	bool getFileDN(const std::string& fileName, std::string& fileDN);

	void setFileDN(const std::string& fileName, const std::string& fileDN);

	FMS_CPF_Types::fileType getCpFileType(const std::string& fileName) throw (FMS_CPF_PrivateException);

	const char* getCpName() const { return m_CpName.c_str(); };

 private:

	/**	@brief	enumeration of type file
	*/
	enum cpFileType{
			INFINITE = 1,
			COMPOSITE,
			SIMPLE,
			UNKNOW
	};

	/** @brief	loadCpFileFromIMM
	 *
	 *  This method gets all defined CP files of a specific CP from data disk
	 *  and rebuilds IMM objects in according to it
	 *
	 *  @remarks Remarks
	 */
	bool loadCpFileFromDataDisk(const std::vector<std::string>& listVolumeDN);

	/** @brief	addCpFilesOfVolume
	 *
	 *  This method gets all defined CP files of a specific volume from data disk
	 *
	 *  @remarks Remarks
	*/
	void addCpFilesOfVolume(const std::string& volumeName);

	/** @brief	addSubFilesOfCompositeFile
	 *
	 *  This method gets all defined CP subfiles of a specific composite file from data disk
	 *
	 *  @remarks Remarks
	*/
	void addSubFilesOfCompositeFile(const std::string& filePath, File* compositeFileHndl);

	/** @brief	checkSubFileName
	 *
	 *  This method validate the file name
	 *
	 *  @param subfile : the subfile name
	 *
	 *  @return true if the name is valid, otherwise false
	 *
	 *  @remarks Remarks
	 */
	bool checkSubFileName(const std::string& subfile);

	/** @brief	updateIMMObjects
	 *
	 *  This method updates the IMM objects in according to the data disk
	 *
	 *  @return true on success, otherwise false
	 *
	 *  @remarks Remarks
	*/
	bool updateIMMObjects(const std::vector<std::string>& listVolumeDN, std::set<std::string>& dataDiskVolume);

	/** @brief	createVolume
	 *
	 *  This method creates an volume object into IMM
	 *
	 *  @remarks Remarks
	 */
	std::string createVolume(const std::string& volumeName, OmHandler* objManager);


	static void deleteScpVolumes(OmHandler* objManager);

	/** @brief	createCpFile
	 *
	 *  This method creates an File object into IMM
	 *
	 *  @remarks Remarks
	*/
	void createCpFile(OmHandler* objManager, File* cpFile, const std::string& volumeDN);

	/** @brief	addSubFilesOfCompositeFile
	 *
	 *  This method checks if an object into IMM exists on data disk
	 *
	 *  @remarks Remarks
	*/
	void checkCpFile(const std::string& volumeName, const std::string& fileDN, OmHandler* objManager);

	/** @brief	checkCpSubFile
	 *
	 *  This method checks if an object into IMM exists on data disk
	 *
	*/
	void checkCpSubFile(const std::string& fileDN, OmHandler* objManager, File* cpFile);

	/** @brief	updateObjectAttributes
	 *
	 *  This method checks if the IMM object attributes must be updated
	 *
	 *  @remarks Remarks
	*/
	void updateObjectAttributes(const std::string& fileDN, OmHandler* objManager, File* cpFile, unsigned int immRecordLength);

	/** @brief	loadCpFileFromIMM
	 *
	 *  This method gets all defined CP files of a specific CP from IMM
	 *  and aligns data disk info in according to it
	 *
	 *  @return true on success, otherwise false
	 *
	 *  @remarks Remarks
	 */
	bool loadCpFileFromIMM(const std::vector<std::string>& listVolumeDN, bool restore);

	/** @brief	getVolumeName
	 *
	 *  This method gets the volume name from its volume DN
	 *
	 *  @return true on success, otherwise false
	 *
	 *  @remarks Remarks
	 */
	bool getVolumeName(const std::string& volumeDN, std::string& volumeName );

	/** @brief	addCpRegularFile
	 *
	 *  This method adds a regular file to the internal File table
	 *
	 *  @remarks Remarks
	 */
	void addCpRegularFile(const std::string& fileDN, const std::string& volumeName, OmHandler* objManager, bool composite, bool restore);

	/** @brief	addCpRegularFile
	 *
	 *  This method adds a regular file to the internal File table
	 *
	 *  @remarks Remarks
	 */
	void addCompositeSubFile(const std::string& fileDN, OmHandler* objManager, File* compositeFileHndl);

	/** @brief	addCpRegularFile
	 *
	 *  This method adds an infinite file to the internal File table
	 *
	 *  @remarks Remarks
	 */
	void addCpInfiniteFile(const std::string& fileDN, const std::string& volumeName, OmHandler* objManager, bool restore);

	/** @brief	getInfiniteFileAttributes
	 *
	 *  This method gets infinite file attributes from IMM
	 *
	 *  @remarks Remarks
	 */
	bool getInfiniteFileAttributes(const std::string& fileDN, OmHandler* objManager, FMS_CPF_Types::fileAttributes& attributeValue);

         /** @brief     getCompositeFileAttributes
         *
         *  This method gets composite file attributes from IMM
         *
         *  @remarks Remarks
         */
        bool getCompositeFileAttributes(const std::string& fileDN, OmHandler* objManager, FMS_CPF_Types::fileAttributes& attributeValue);           

	/** @brief	checkVolumeFolder
	 *
	 *  This method creates, if it does not exist, the volume folder
	 *
	 *  @remarks Remarks
	 */
	void checkVolumeFolder(const std::string& volumeName);

	/** @brief	checkMainFileFolder
	 *
	 *  This method creates, if it does not exist, the main file folder
	 *
	 *  @remarks Remarks
	 */
	void checkMainFileFolder(const std::string& mainFileName, const std::string& volumeName);

	/** @brief	checkFile
	 *
	 *  This method creates, if it does not exist, the physical file
	 *
	 *  @remarks Remarks
	 */
	void checkFile(const std::string& fileName, const std::string& volumeName);

	/** @brief
	 *
	 *  This method returns the file type from its DN
	 *
	 *  @remarks Remarks
	 */
	cpFileType getFileType(const std::string& fileDN);

	typedef std::pair<std::string,std::string> infiniteFileInfo;

	/** @brief	getListOfInfiniteFile
	 *
	 *  This method returns the list of pairs <infinite fileName, DN>
	 *
	 *  @remarks Remarks
	 */
	void getListOfInfiniteFile(std::vector<infiniteFileInfo>& fileList);

	/** @brief	getListOfInfiniteFile
	 *
	 *  This method returns the list of infinite file name
	 *
	 *  @remarks Remarks
	 */
	void getListOfInfiniteFile(std::vector<std::string>& fileList);

	/** @brief	getListOfInfiniteSubFile
	 *
	 *  This method returns the list of infinite subFiles
	 *
	 *  @remarks Remarks
	 */
	void getListOfInfiniteSubFile(const std::string& infiniteFileName, std::set<std::string>& subFileList);

	/** @brief	getLastFieldValue
	 *
	 *  This method returns the value of last field of a DN
	 *
	 *  @remarks Remarks
	 */
	void getLastFieldValue(const std::string& fileDN, std::string& value);

	/** @brief	isFileLockedForDelete
	 *
	 *  This method checks if the composite file is locked for delete
	 *
	 *  @remarks Remarks
	 */
	bool isFileLockedForDelete(const std::string& fileName);

	/** @brief	isFileLockedForDelete
	 *
	 *  This method creates an IMM object with a retry mechanism
	 *
	 *  @remarks Remarks
	 */
	bool createIMMObject(OmHandler* objManager, const char* className, const char* parentName, std::vector<ACS_CC_ValuesDefinitionType> attrValuesList, const char* objDN);

	/** @brief	isFileLockedForDelete
	 *
	 *  This method deletes an IMM object with a retry mechanism
	 *
	 *  @remarks Remarks
	 */
	bool deleteIMMObject(OmHandler* objManager, const char* objectName );

	typedef std::list<FileAccess*, std::allocator<FileAccess*> >SubFileList;
	SubFileList subfilelist_;
	
	typedef std::list<File*, std::allocator<File*> >FileTableList;
	FileTableList m_FileTable;
	
	/**
	 * 	@brief
	 *
	 * 	Cp name of this file table.
	*/
	std::string m_CpName;

	/**
	 * 	@brief
	 *
	 * 	Flag of system type (SCP/MCP)
	*/
	bool m_isMultiCP;

	std::string m_CpRoot;

	ACE_Thread_Mutex m_FileTableLock;

	/**		@brief	fms_cpf_DirMgr
	*/
	ACS_TRA_trace* fms_cpf_FileTable;
};

#endif
