/*
 * * @file fms_cpf_directorystructuremgr.h
 *	@brief
 *	Header file for DirectoryStructureMgr class.
 *  This module contains the declaration of the class DirectoryStructureMgr.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-07-04
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
 *	| 1.0.0  | 2011-07-04 | qvincon      | File imported.                      |
 *	+========+============+==============+=====================================+
 	| 2.0.0  | 2012-06-19 | qvincon      | Backup adaption.                    |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef DIRECTORY_STRUCTURE_MGR_H
#define DIRECTORY_STRUCTURE_MGR_H

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_privateexception.h"
#include "fms_cpf_types.h"

#include <ace/Singleton.h>
#include <ace/Synch.h>

#include <string>
#include <list>
#include <vector>
#include <map>
#include <set>

class FMS_CPF_FileId;
class FMS_CPF_Attribute;

class File;
class FileTable;
class FileReference;

class FMS_CPF_ConfigReader;

class ACS_TRA_trace;

class OmHandler;
/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class FMS_CPF_DirectoryStructureMgr
{
 public:
	friend class FMS_CPF_Server;

	friend class ACE_Singleton<FMS_CPF_DirectoryStructureMgr, ACE_Recursive_Thread_Mutex>;

	/* @brief	find method
	 *
	 * This method find a file in the directory structure
	 *
	 * @param id: FMS_CPF_FileId object of the file to find
	 *
	 * @param _cpname: The CP name
	 *
	 * @return a reference to the File object
	 *
	 * @remarks Remarks
	*/
	File* find(const FMS_CPF_FileId id, const char* _cpname = NULL);

	/* @brief	exists method
	 *
	 * This method verifies if a file is present in the directory structure
	 *
	 * @param id: FMS_CPF_FileId object of the file to find
	 *
	 * @param _cpname: The CP name
	 *
	 * @return true if the file is present, otherwise false
	 *
	 * @remarks Remarks
	*/
	bool exists(const FMS_CPF_FileId& fileid, const char* _cpname = NULL)
        throw (FMS_CPF_PrivateException);

	FileReference create(const FMS_CPF_FileId& fileid,
						 const std::string& volume,
						 const FMS_CPF_Attribute& attribute,
						 FMS_CPF_Types::accessType access,
						 const std::string& fileDN,
						 const char* _cpname = NULL
						) throw(FMS_CPF_PrivateException);

	FileReference create(const FMS_CPF_FileId& fileid, FMS_CPF_Types::accessType access, const char* _cpname = NULL)
		throw (FMS_CPF_PrivateException);

	void close(FileReference ref, const char* _cpname = NULL)
			throw(FMS_CPF_PrivateException);

	bool closeExceptionLess(FileReference ref, const char* _cpname = NULL);

	FileReference open(const FMS_CPF_FileId& fileid, FMS_CPF_Types::accessType access, const char* _cpname = NULL, bool inf = false)
			throw(FMS_CPF_PrivateException);

	void remove(FileReference ref, const char *_cpname = NULL)
		throw (FMS_CPF_PrivateException);

	void removeBlockSender(FileReference reference, const char *_cpname = NULL)
		throw (FMS_CPF_PrivateException);

	int getListFileId(const FMS_CPF_FileId& fileId, std::list<FMS_CPF_FileId>& fileList, const char* _cpname = NULL)
		throw (FMS_CPF_PrivateException);

	bool getListOfInfiniteFile( const std::string& cpName, std::vector<std::pair<std::string, std::string> >& fileList);

	bool getListOfInfiniteSubFile(const std::string& infiniteFileName, const std::string& cpName, std::set<std::string>& subFileList);

	bool getFileDN(const std::string& fileName, const std::string& cpName, std::string& fileDN);

	void setFileDN(const std::string& fileName, const std::string& cpName, const std::string& fileDN);

	FMS_CPF_Types::fileType getFileType(const std::string& fileName, const std::string& cpName) throw(FMS_CPF_PrivateException);

	/** @brief	isFileLockedForDelete
	 *
	 *  This method checks if the composite file is locked for delete
	 *
	 *  @remarks Remarks
	 */
	bool isFileLockedForDelete(const std::string& fileName, const std::string& cpName);

	/** @brief	isAfterRestore
	 *
	 *  This method gets the flag that indicates a restart after a restore
	 *
	 *  @remarks Remarks
	 */
	bool isAfterRestore() const { return m_isRestartAfterRestore; };

	/* @brief refreshCpFileTable method
	 *
	 * This method adds a newFileTable object into the internal map
	 *
	 * @param cpName: The Cp dafault name
	 *
	 * @param cpVolumeDN: DN's list of all volume of the Cp specified by cpName parameter
	 *
	 * @remarks Remarks
	 */
	bool refreshCpFileTable(const std::string& cpName);

	/* @brief	exists method
	 *
     * This method verifies if a CP is present
	 *
	 * @param cpName: The CP default name
	 *
	 * @remarks Remarks
	 */
	bool cpExists(const std::string& cpName);


 private:

	/**
	 * 	@brief	Constructor of FMS_CPF_DirectoryStructureMgr class
	*/
	FMS_CPF_DirectoryStructureMgr();

	/**
	 * 	@brief	Destructor of FMS_CPF_DirectoryStructureMgr class
	*/
	virtual ~FMS_CPF_DirectoryStructureMgr();

	/* @brief initializeCpFileSystem method
	 *
	 * This method initialize the Cp file system
	 *
	 * @param systemType: Flag of system type (SCP/MCP)
	 *
	 * @return true on success, otherwise false
	 *
	 * @remarks Remarks
	*/
	bool initializeCpFileSystem(FMS_CPF_ConfigReader *configReader);

	/* @brief loadCpFileInfo method
	 *
	 * This method load Cp File info from IMM
	 *
	 * @param restore: Flag to indicate the restart type
	 *
	 * @return true on success, otherwise false
	 *
	 * @remarks Remarks
	*/
	bool loadCpFileInfo(bool restore);

	/* @brief addCpFileTable method
	 *
	 * This method adds to internal map a newFileTable object
	 *
	 * @param cpName: The Cp dafault name
	 *
	 * @param cpVolumeDN: DN's list of all volume of the Cp specified by cpName parameter
	 *
	 * @return true on success, otherwise false
	 *
	 * @remarks Remarks
	*/
	bool addCpFileTable(const std::string& cpName, const std::vector<std::string>& cpVolumeDN, bool restore);

	/* @brief updateInfiteFileState method
	 *
	 * 	This method will be called at start-up on restore to check consistency between
     *  physical files on data disk and file attribute value
     *
     *  @param FileTable: FileTable object.
     *  @param fileName: Cp file to check.
     *  @param firstSubFile: First physical subfile.
     *  @param lastSubFile: Last physical subfile.
     *
	 * @remarks Remarks
	*/
	void updateInfiteFileAttribute(FileTable* fileTable, const std::string& fileName, const std::string& firstSubFile, const std::string& lastSubFile);

	/* @brief updateInfiteFileState method
	 *
	 * 	This method will be called at start-up to remove all infinite subfiles from IMM
	 *
	 * @remarks Remarks
	*/
	void removeInfiniteSubFilesFromIMM(OmHandler* objManager);

	/* @brief filterVolume method
	 *
	 * This method arranges the volumes'DN
	 *
	 * @param allVolume : DN's list of all volumes
	 *
	 * @param volumeMap: map of DN'volume arragend by CpName
	 *
	 * @return true on success, otherwise false
	 *
	 * @remarks Remarks
	*/
	bool filterVolume(const std::vector<std::string>& allVolumeDN, std::map<std::string, std::vector<std::string> >& volumeMap );

	/* @brief clearFileTableMap method
	 *
	 * This method arranges the volumes'DN
	 *
	 * @remarks Remarks
	*/
	void clearFileTableMap();

	/* @brief isRestartAfterRestore
	 *
	 * This method checks for restart after a system restore
	 *
	 * @remarks
	*/
	bool isRestartAfterRestore();

	/* @brief getClearPath
	 *
	 * This method retrieves PSA clear path by usage of PSA API
	 * and store it into internal variable m_clearDataPath
	 *
	 * @return true on restart after restore, otherwise false
	 *
	 * @remarks
	*/
	bool getClearPath();

	/* @brief dataDiskEmpty
	 *
	 * This method checks if the data disk is empty
	 *
	 * @return true on data disk empty, otherwise false
	 *
	 * @remarks
	*/
	bool dataDiskEmpty();

	/**
	   @brief  	This method creates the backup object
	*/
	void createBackupObject(OmHandler* objManager);

	bool checkRaidDisk();

	/**
	 * 	@brief
	 *
	 * 	Flag of system type (SCP/MCP)
	*/
	bool m_isMultiCP;

	/**
	 * 	@brief
	 *
	 * 	Flag of restore state
	*/
	bool m_isRestartAfterRestore;

	typedef std::map<std::string, FileTable*> maptype;

	/**
		@brief The map of all FileTable objects
	*/
	maptype m_FileTableMap;
	
	std::string m_clearDataPath;

	/**		@brief	fms_cpf_DirMgr
	*/
	ACS_TRA_trace* fms_cpf_DirMgr;

	/**
	 * @brief The configuration reader used to access to system data
	 */
	FMS_CPF_ConfigReader *m_configReader;
};

typedef ACE_Singleton<FMS_CPF_DirectoryStructureMgr, ACE_Recursive_Thread_Mutex> DirectoryStructureMgr;

#endif
