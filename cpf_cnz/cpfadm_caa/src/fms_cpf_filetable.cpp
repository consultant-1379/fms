/*
 * * @file fms_cpf_filetable.cpp
 *	@brief
 *	Class method implementation for FileTable.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_filetable.h module
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

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#define BOOST_FILESYSTEM_VERSION 3

#include "fms_cpf_filetable.h"
#include "fms_cpf_filetable.h"
#include "fms_cpf_filehandler.h"
#include "fms_cpf_fileaccess.h"
#include "fms_cpf_filereference.h"
#include "fms_cpf_filemgr.h"
#include "fms_cpf_filedescriptor.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_common.h"
#include "fms_cpf_eventalarmhndl.h"
#include "fms_cpf_tqchecker.h"
#include "fms_cpf_jtpconnectionhndl.h"

#include "acs_apgcc_omhandler.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"

#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <boost/filesystem.hpp>


extern ACS_TRA_Logging CPF_Log;

namespace defaultVolume {
	const std::string TEMPVOLUME("TEMPVOLUME");
	const std::string RELVOLUMSW("RELVOLUMSW");
	const std::string EXCHVOLUME("EXCHVOLUME");
}

FileTable::FileTable(std::string _cpname, bool isMultiCp) :
			m_CpName(_cpname),
			m_isMultiCP(isMultiCp),
			m_CpRoot("") //HW56759
{
	fms_cpf_FileTable = new (std::nothrow) ACS_TRA_trace("FMS_CPF_FileTable");
}

FileTable::~FileTable(void)
{
	// Remove all File objects
	FileTableList::iterator filePtr;
	for(filePtr = m_FileTable.begin(); filePtr != m_FileTable.end(); ++filePtr)
	{
		delete (*filePtr);
	}

	if(NULL != fms_cpf_FileTable)
		delete fms_cpf_FileTable;
}

/*============================================================================
	ROUTINE: loadCpFileFromDataDisk
 ============================================================================ */
bool FileTable::loadCpFileFromDataDisk(const std::vector<std::string>& listVolumeDN)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in loadCpFileFromDataDisk()");
	bool result = false;
	// Set this Cp file root
	m_CpRoot = (ParameterHndl::instance()->getCPFroot(m_CpName.c_str()) + DirDelim);

	const short LOGMSG_LENGTH = 128U;
	char logMsg[LOGMSG_LENGTH] = {0};
	snprintf(logMsg, LOGMSG_LENGTH -1, "%s(), search CP:<%s> files into path:<%s>", __func__, m_CpName.c_str(), m_CpRoot.c_str());
	TRACE(fms_cpf_FileTable, "%s", logMsg);
	CPF_Log.Write(logMsg, LOG_LEVEL_INFO);

	// Open CPF directory
	DIR* cpfRootFolder = ACE_OS::opendir(m_CpRoot.c_str());

	// Build the file table
	if( NULL != cpfRootFolder )
	{
		struct dirent* volumeEntry;

		std::set<std::string> dataDiskVolume;

		// Reads all volume in the CPF root
		while( (volumeEntry = ACE_OS::readdir(cpfRootFolder) ) != 0)
		{
			// Get the volume name
			std::string volumeName(volumeEntry->d_name);

			TRACE(fms_cpf_FileTable, "load(), found volume %s", volumeName.c_str());

			// Checks if it is a valid CPF volume
			if( (volumeName.length() <= stdValue::VOL_LENGTH) &&
				((volumeName[0] >= stdValue::minStartChr) && (volumeName[0] <= stdValue::maxStartChr)) &&
				(volumeName.find_first_not_of(stdValue::VOL_IDENTIFIER) == std::string::npos) )
			{
				// Found a valid volume
				snprintf(logMsg, LOGMSG_LENGTH -1, "%s(), found valid volume:<%s>", __func__, volumeName.c_str());
				TRACE(fms_cpf_FileTable, "%s", logMsg);
				CPF_Log.Write(logMsg, LOG_LEVEL_INFO);

				// Load all cp file in the founded volume
				addCpFilesOfVolume(volumeName);

				dataDiskVolume.insert(volumeName);
			}
		}
		// close folder
		ACE_OS::closedir(cpfRootFolder);

		// Update IMM in according to the files found on data disk
		updateIMMObjects(listVolumeDN, dataDiskVolume);

		result = true;
	}
	else
	{
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "%s, error:<%d> on opendir:<%s> call, Cp:<%s>", __func__, errno, m_CpRoot.c_str(), m_CpName.c_str());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileTable, "%s", errMsg);
	}

    TRACE(fms_cpf_FileTable, "%s", "Leaving loadCpFileFromDataDisk()");
    return result;
}

/*============================================================================
	ROUTINE: addCpFilesOfVolume
 ============================================================================ */
void FileTable::addCpFilesOfVolume(const std::string& volumeName)
{
	TRACE(fms_cpf_FileTable, "Entering in addCpFilesOfVolume(<%s>)", volumeName.c_str());

	std::string volumePath = m_CpRoot + volumeName;

	// Open volume directory
	DIR* volumeFolder = ACE_OS::opendir(volumePath.c_str() );

	if(NULL != volumeFolder)
	{
		struct dirent* fileEntry;
		const short LOGMSG_LENGTH = 128U;
		char logMsg[LOGMSG_LENGTH] = {0};

		// Reads all files into the volume
		while((fileEntry = ACE_OS::readdir(volumeFolder)) != NULL)
		{
			// get the file name
			std::string fileName(fileEntry->d_name);

			// Checks if it is a valid file name
			if( (fileName.length() <= stdValue::FILE_LENGTH) &&
				((fileName[0] >= stdValue::minStartChr) && (fileName[0] <= stdValue::maxStartChr)) &&
				(fileName.find_first_not_of(stdValue::IDENTIFIER) == std::string::npos) )
			{
				snprintf(logMsg, LOGMSG_LENGTH -1, "%s(), CP file:<%s> founded", __func__, fileName.c_str());
				TRACE(fms_cpf_FileTable, "%s", logMsg);
				CPF_Log.Write(logMsg, LOG_LEVEL_INFO);

				std::string filePath = volumePath + DirDelim + fileName;

				try
				{
					FMS_CPF_FileId cpFileId = fileName;
					FMS_CPF_Attribute attribute;

					// Read file attributes
					attribute.readFile(filePath);

					File* cpFile = new File(cpFileId, volumeName, m_CpName, attribute);

					// Check file type
					if(FMS_CPF_Types::ft_REGULAR == attribute.type() && attribute.composite())
					{
						// Composite file
						addSubFilesOfCompositeFile(filePath, cpFile);
						//TRHY46076
						if(!m_isMultiCP)
						{	
							FMS_CPF_JTPConnectionHndl::updateDeleteFileTimer(fileName, attribute.getDeleteFileTimer());
							FMS_CPF_JTPConnectionHndl::initFromFile(fileName);
						}
					}

					// Insert the file
					m_FileTable.push_back(cpFile);
				}
				catch(FMS_CPF_PrivateException& ex)
				{
					char errMsg[1024]={0};
					ACE_OS::snprintf(errMsg, 1023, "addCpFilesOfVolume(), error on Cp:<%s> file:<%s> load, attribute file:<%s> corrupt or missing", fileName.c_str(), m_CpName.c_str(), filePath.c_str() );
					CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);

					TRACE(fms_cpf_FileTable, "%s", errMsg);
				}
			}
		}
		// close folder
		ACE_OS::closedir(volumeFolder);
	}
	else
	{
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "FileTable::addCpFilesOfVolume(), error:<%d> on opendir:<%s> call, Cp:<%s>", errno, volumePath.c_str(), m_CpName.c_str());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileTable, "%s", errMsg);
	}

	TRACE(fms_cpf_FileTable, "%s", "Leaving addCpFilesOfVolume()");
}

/*============================================================================
	ROUTINE: addSubFilesOfCompositeFile
 ============================================================================ */
void FileTable::addSubFilesOfCompositeFile(const std::string& filePath, File* compositeFileHndl)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in addSubFilesOfCompositeFile()");

	// Open composite file directory
	DIR* fileFolder = ACE_OS::opendir(filePath.c_str() );

	if(NULL != fileFolder)
	{
		struct dirent* subFileEntry;
		const short LOGMSG_LENGTH = 128U;
		char logMsg[LOGMSG_LENGTH] = {0};

		// add all subfiles
		while( (subFileEntry = ACE_OS::readdir(fileFolder)) != NULL )
		{
			// Get subfile name
			std::string subFileName(subFileEntry->d_name);

			// check name
			if(checkSubFileName(subFileName))
			{
				compositeFileHndl->addSubFile(subFileName);

				snprintf(logMsg, LOGMSG_LENGTH -1, "%s(), subfile:<%s> founded", __func__, subFileName.c_str());
				TRACE(fms_cpf_FileTable, "%s", logMsg);
				CPF_Log.Write(logMsg, LOG_LEVEL_INFO);
			}
		}
		// close folder
		ACE_OS::closedir(fileFolder);
	}
	else
	{
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "FileTable::addSubFilesOfCompositeFile(), error:<%d> on opendir:<%s> call, Cp:<%s>", errno, filePath.c_str(), m_CpName.c_str());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileTable, "%s", errMsg);
	}
	TRACE(fms_cpf_FileTable, "%s", "Leaving addSubFilesOfCompositeFile()");
}

/*============================================================================
	ROUTINE: checkSubFileName
 ============================================================================ */
bool FileTable::checkSubFileName(const std::string& subfile)
{
	TRACE(fms_cpf_FileTable,"%s","Entering in checkSubFileName()");

	bool result = true;
	size_t found;
	std::string namePart(subfile);

	// search '-' in the name
	size_t minusPos = subfile.find_first_of(parseSymbol::minus);

	// Check if the generator part is present
	if( std::string::npos != minusPos )
	{
		// subfile with generator : SUBFILE-GEN
		std::string genPart;
		// Split subfile name and gen part
		namePart = subfile.substr(0, minusPos);
		genPart = subfile.substr(minusPos + 1);
		// validate the gen part
		// check if there is some not allowed character
		found = genPart.find_first_not_of(stdValue::IDENTIFIER);
		if( std::string::npos != found || genPart.length() > stdValue::GEN_LENGTH)
		{
			result = false;
			TRACE(fms_cpf_FileTable, "checkSubFileName(), generator part:<%s> not valid", genPart.c_str());
			return result;
		}
	}

	// check the name part
	found = namePart.find_first_not_of(stdValue::IDENTIFIER);
	if( std::string::npos != found || namePart.empty() || namePart.length() > stdValue::FILE_LENGTH)
	{
		result = false;
		TRACE(fms_cpf_FileTable, "checkSubFileName(), subfile name:<%s> not valid", namePart.c_str());
	}

	TRACE(fms_cpf_FileTable,"%s","Leaving checkSubFileName()");
	return result;
}

/*============================================================================
	ROUTINE: updateIMMObjects()
 ============================================================================ */
bool FileTable::updateIMMObjects(const std::vector<std::string>& listVolumeDN, std::set<std::string>& dataDiskVolume)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in updateIMMObjects()");
	bool result = true;
	OmHandler objManager;
	// Init OM resource
	if(objManager.Init() != ACS_CC_FAILURE)
	{
		std::map<std::string, std::string> volumeNameToDN;

		std::vector<std::string>::const_iterator volumeDN;
		std::set<std::string>::iterator element;

		// get all cp file for each volume
		for(volumeDN = listVolumeDN.begin(); volumeDN != listVolumeDN.end(); ++volumeDN)
		{
			std::string volumeName;
			// get the volume name
			if( getVolumeName((*volumeDN), volumeName) )
			{
				element = dataDiskVolume.find(volumeName);
				if(dataDiskVolume.end() == element)
				{
					// Volume present into IMM but not on Data Disk
					// Remove the Object
					if (!deleteIMMObject(&objManager, (*volumeDN).c_str()))
					{
						char errMsg[512] = {0};
						ACE_OS::snprintf(errMsg, 511, "FileTable::updateIMMObjects(), error:<%d> on deleteObject() volume DN:<%s>, CP:<%s>", objManager.getInternalLastError(),
								(*volumeDN).c_str(),
								m_CpName.c_str());
						CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
						TRACE(fms_cpf_FileTable, "%s", errMsg);
					}
				}
				else
				{
					// Volume present on data disk and IMM
					// Check its files
					dataDiskVolume.erase(element);
					// Save the volume DN
					volumeNameToDN.insert( std::make_pair(volumeName, (*volumeDN)) );

					std::vector<std::string> fileDNList;

					// get all file of this volume
					if(objManager.getChildren((*volumeDN).c_str(), ACS_APGCC_SUBLEVEL, &fileDNList) == ACS_CC_SUCCESS )
					{
						std::vector<std::string>::const_iterator fileDN;
						// insert each file in the internal file table
						for(fileDN = fileDNList.begin(); fileDN != fileDNList.end(); ++fileDN)
						{
							checkCpFile(volumeName, (*fileDN), &objManager);
						}
					}
					else
					{
						char errMsg[512] = {0};
						ACE_OS::snprintf(errMsg, 511, "FileTable::updateIMMObjects(), error:<%d> on getChildren() of volume:<%s> DN:<%s>, CP:<%s>", objManager.getInternalLastError(),
																																			  volumeName.c_str(),
																																			  (*volumeDN).c_str(),
																																			  m_CpName.c_str());
						CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
						TRACE(fms_cpf_FileTable, "%s", errMsg);
					}
				}
			}
		}

		TRACE(fms_cpf_FileTable, "updateIMMObjects(), create <%zd> volume object into IMM", dataDiskVolume.size() );

		// re-create IMM volume objects not found
		for(element = dataDiskVolume.begin(); element != dataDiskVolume.end(); ++element)
		{
			std::string newVolumeDN = createVolume((*element), &objManager);
			// Save the new create volume DN
			volumeNameToDN.insert( std::make_pair((*element), newVolumeDN) );
		}

		std::map<std::string, std::string>::const_iterator volumeIdx;

		// re-create IMM File objects not found
		FileTableList::iterator cpFileIterator;
		File* cpFile;
		for(cpFileIterator = m_FileTable.begin(); cpFileIterator != m_FileTable.end(); ++cpFileIterator)
		{
			cpFile = (*cpFileIterator);
			// Check the IMM object state
			if(!cpFile->getImmState())
			{
				// Get the volume DN
				volumeIdx = volumeNameToDN.find(cpFile->volume_);
				// create the IMM object related to the cp file
				if(volumeNameToDN.end() != volumeIdx)
					createCpFile(&objManager, cpFile, volumeIdx->second);
			}
		}

		volumeNameToDN.clear();
		// Deallocate OM resource
		objManager.Finalize();
	}
	else
	{
		//Error Handling
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "FileTable::updateIMMObjects(), error:<%d> on OmHandler object init(), Cp:<%s>", objManager.getInternalLastError(), m_CpName.c_str());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileTable, "%s", errMsg);
		result = false;
	}

	TRACE(fms_cpf_FileTable, "%s", "Leaving updateIMMObjects()");
	return result;
}

/*============================================================================
	ROUTINE: createVolume
 ============================================================================ */
std::string FileTable::createVolume(const std::string& volumeName, OmHandler* objManager)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in createVolume()");

	std::vector<ACS_CC_ValuesDefinitionType> objAttrList;

	char tmpRDN[256] = {0};

	// Assemble the RDN value
	if(m_isMultiCP)
		ACE_OS::snprintf(tmpRDN, 255, "%s=%s%c%s", cpf_imm::VolumeKey, volumeName.c_str(), parseSymbol::atSign, m_CpName.c_str());
	else
		ACE_OS::snprintf(tmpRDN, 255, "%s=%s", cpf_imm::VolumeKey, volumeName.c_str());

	// Fill the RDN attribute fields
	ACS_CC_ValuesDefinitionType attributeRDN;
	attributeRDN.attrName = cpf_imm::VolumeKey;
	attributeRDN.attrType = ATTR_STRINGT;
	attributeRDN.attrValuesNum = 1;
	void* tmpValueRDN[1] = { reinterpret_cast<void*>(tmpRDN) };
	attributeRDN.attrValues = tmpValueRDN;

	objAttrList.push_back(attributeRDN);

	if (!createIMMObject(objManager,cpf_imm::VolumeClassName, cpf_imm::parentRoot, objAttrList, tmpRDN))
	{
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "FileTable::createVolume(), IMM error:<%d> on createObject(<%s>), CP:<%s>", objManager->getInternalLastError(), tmpRDN, m_CpName.c_str());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileTable, "%s", errMsg);
	}

	char volumeDN[512] = {0};
	ACE_OS::snprintf(volumeDN, 255, "%s,%s", tmpRDN, cpf_imm::parentRoot);

	TRACE(fms_cpf_FileTable, "%s", "Leaving createVolume()");
	return std::string(volumeDN);
}

/*============================================================================
	ROUTINE: createCpFile
 ============================================================================ */
void FileTable::createCpFile(OmHandler* objManager, File* cpFile, const std::string& volumeDN)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in createCpFile()");

	std::vector<ACS_CC_ValuesDefinitionType> objAttrList;

	std::string fileName(cpFile->getFileName());

	ACS_CC_ValuesDefinitionType attributeRecordLength;
	attributeRecordLength.attrName = cpf_imm::recordLengthAttribute;
	attributeRecordLength.attrType = ATTR_UINT32T;
	attributeRecordLength.attrValuesNum = 1;
	unsigned int recLengthValue = cpFile->attribute_.getRecordLength();
	void* tmpValueRecLen[1] = { reinterpret_cast<void*>(&recLengthValue) };
	attributeRecordLength.attrValues = tmpValueRecLen;

	objAttrList.push_back(attributeRecordLength);

	TRACE(fms_cpf_FileTable, "createCpFile(), fileName:<%s>, reocrd Length:<%d>, parent:<%s>", fileName.c_str(), recLengthValue, volumeDN.c_str());

	ACS_CC_ValuesDefinitionType attributeRDN;
	attributeRDN.attrType = ATTR_STRINGT;
	attributeRDN.attrValuesNum = 1;
	char tmpRDN[128] = {0};
	void* tmpValueRDN[1];
  	//HY46076 
        ACS_CC_ValuesDefinitionType deleteFileTimerAttribute;
	void* tmpdeleteFileTimer[1];

	ACS_CC_ValuesDefinitionType maxSizeAttribute;
	void* tmpMaxSize[1];

	ACS_CC_ValuesDefinitionType maxTimeAttribute;
	void* tmpMaxTime[1];

	ACS_CC_ValuesDefinitionType relCondAttribute;
	int releaseConditionValue;
	void* tmpRelCond[1];

	ACS_CC_ValuesDefinitionType fileTQAttribute;
	char tmpFileTQ[64]={0};
	void* tmpTQValue[1];

	std::string immClassName;
	bool composite = false;

	unsigned long fileMaxSize, fileMaxTime;
	int deleteFileTimer;
	if( cpFile->getFileType() == FMS_CPF_Types::ft_REGULAR)
	{
		if(cpFile->attribute_.composite())
		{
			// Composite file
			composite = true;
			immClassName = cpf_imm::CompositeFileClassName;
			attributeRDN.attrName = cpf_imm::CompositeFileKey;
			ACE_OS::snprintf(tmpRDN, 127, "%s=%s", cpf_imm::CompositeFileKey, fileName.c_str());
			//HY46076
			if(!m_isMultiCP)
			{
				deleteFileTimer = cpFile->attribute_.getDeleteFileTimer();
                        	if(-1 != deleteFileTimer)
				{
					TRACE(fms_cpf_FileTable, "createCpFile(), set deleteFileTimer <%d>", deleteFileTimer);
					deleteFileTimerAttribute.attrName = cpf_imm::deleteFileTimerAttribute;
					deleteFileTimerAttribute.attrType = ATTR_INT32T;
					deleteFileTimerAttribute.attrValuesNum = 1;
					tmpdeleteFileTimer[0] =  reinterpret_cast<void*>(&deleteFileTimer);
					deleteFileTimerAttribute.attrValues = tmpdeleteFileTimer;
					objAttrList.push_back(deleteFileTimerAttribute);
				}
			}
			//HY46076 end
		}
		else
		{
			// simple file
			immClassName = cpf_imm::SimpleFileClassName;
			attributeRDN.attrName = cpf_imm::SimpleFileKey;
			ACE_OS::snprintf(tmpRDN, 127, "%s=%s", cpf_imm::SimpleFileKey, fileName.c_str());
		}
	}
	else
	{
		// Infinite File
		immClassName = cpf_imm::InfiniteFileClassName;
		attributeRDN.attrName = cpf_imm::InfiniteFileKey;
		ACE_OS::snprintf(tmpRDN, 127, "%s=%s", cpf_imm::InfiniteFileKey, fileName.c_str());

		unsigned long active, lastSent;
		FMS_CPF_Types::transferMode tqMode, initMode;
		std::string transferQueue;
		bool fileRelCond;

		cpFile->attribute_.extractExtAttributes(fileMaxSize, fileMaxTime, fileRelCond,
												active, lastSent, tqMode,
												transferQueue, initMode);

		if(0 != fileMaxSize )
		{
			TRACE(fms_cpf_FileTable, "createCpFile(), set maxSize <%d>", fileMaxSize );
			maxSizeAttribute.attrName = cpf_imm::maxSizeAttribute;
			maxSizeAttribute.attrType = ATTR_UINT32T;
			maxSizeAttribute.attrValuesNum = 1;
			tmpMaxSize[0] =  reinterpret_cast<void*>(&fileMaxSize);
			maxSizeAttribute.attrValues = tmpMaxSize;
			objAttrList.push_back(maxSizeAttribute);
		}

		if(0 != fileMaxTime )
		{
			TRACE(fms_cpf_FileTable, "createCpFile(), set maxTime <%d>", fileMaxTime );
			maxTimeAttribute.attrName = cpf_imm::maxTimeAttribute;
			maxTimeAttribute.attrType = ATTR_UINT32T;
			maxTimeAttribute.attrValuesNum = 1;
			tmpMaxTime[0] =  reinterpret_cast<void*>(&fileMaxTime);
			maxTimeAttribute.attrValues = tmpMaxTime;
			objAttrList.push_back(maxTimeAttribute);
		}

		TRACE(fms_cpf_FileTable, "createCpFile(), set release cond. to <%s>", (fileRelCond ? "ON" :"OFF") );
		releaseConditionValue = (fileRelCond ? 1 : 0);
		relCondAttribute.attrName = cpf_imm::releaseCondAttribute;
		relCondAttribute.attrType = ATTR_INT32T;
		relCondAttribute.attrValuesNum = 1;
		tmpRelCond[0] = reinterpret_cast<void*>(&releaseConditionValue);
		relCondAttribute.attrValues = tmpRelCond;
		objAttrList.push_back(relCondAttribute);

		if( (FMS_CPF_Types::tm_FILE == tqMode) || (FMS_CPF_Types::tm_BLOCK == tqMode) )
		{
			TRACE(fms_cpf_FileTable, "createCpFile(), set TQ:<%s> mode:<%d>", transferQueue.c_str(), tqMode);

			fileTQAttribute.attrName = cpf_imm::fileTQAttribute;

			fileTQAttribute.attrType = ATTR_STRINGT;
			fileTQAttribute.attrValuesNum = 1;

			ACE_OS::strcpy(tmpFileTQ, transferQueue.c_str());
			tmpTQValue[0] = reinterpret_cast<void*>(tmpFileTQ);
			fileTQAttribute.attrValues = tmpTQValue;
			objAttrList.push_back(fileTQAttribute);
		}
	}

	tmpValueRDN[0] = reinterpret_cast<void*>(tmpRDN);
	attributeRDN.attrValues = tmpValueRDN;

	objAttrList.push_back(attributeRDN);

	TRACE(fms_cpf_FileTable, "createCpFile(), creating object:<%s> of type:<%s>", tmpRDN, immClassName.c_str());

	if(!createIMMObject(objManager, immClassName.c_str(), volumeDN.c_str(), objAttrList, fileName.c_str()))
	{
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "FileTable::createCpFile(), error:<%d> on createObject() on file:<%s> DN:<%s>, CP:<%s>", objManager->getInternalLastError(),
				fileName.c_str(),
				volumeDN.c_str(),
				m_CpName.c_str());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileTable, "%s", errMsg);
	}
	else
	{
		char tmpDN[512] = {0};
		ACE_OS::snprintf(tmpDN, 511, "%s,%s", tmpRDN, volumeDN.c_str());
		std::string fileDN(tmpDN);

		cpFile->setImmState(true);
		// Store the file DN
		cpFile->setFileDN(fileDN);

		TRACE(fms_cpf_FileTable, "createCpFile(), object created DN:<%s>", fileDN.c_str());

		// File created into IMM
		if(composite) checkCpSubFile(fileDN, objManager, cpFile);
	}

	TRACE(fms_cpf_FileTable, "%s", "Leaving createCpFile()");
}

/*============================================================================
	ROUTINE: checkCpFile
 ============================================================================ */
void FileTable::checkCpFile(const std::string& volumeName, const std::string& fileDN, OmHandler* objManager)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in checkCpFile()");
	//List of attributes to get
	std::vector<ACS_APGCC_ImmAttribute*> attributeList;

	// get the filename and recordLength attributes of a regular file
	std::string fileName;
	getLastFieldValue(fileDN, fileName);

	ACS_APGCC_ImmAttribute recordLengthAttribute;
	recordLengthAttribute.attrName = cpf_imm::recordLengthAttribute;

	attributeList.push_back(&recordLengthAttribute);

	// Get attributes by IMM
	ACS_CC_ReturnType getResult = objManager->getAttribute(fileDN.c_str(), attributeList );

	if( (ACS_CC_FAILURE != getResult) && !fileName.empty()
			&& (0 != recordLengthAttribute.attrValuesNum ) )
	{
		unsigned int recordLength = (*reinterpret_cast<unsigned int*>(recordLengthAttribute.attrValues[0]));

		bool foundAndMatch = false;
		bool composite = false;
		File* cpFile = find(fileName);
		// File exists on data disk in the same volume
		if( (NULL != cpFile) && (volumeName.compare(cpFile->getVolumeName()) == 0))
		{
			FMS_CPF_Types::fileType fileType = cpFile->attribute_.type();

			// check all attributes
			switch( getFileType(fileDN) )
			{
				case SIMPLE :
				{
					foundAndMatch = ( (FMS_CPF_Types::ft_REGULAR == fileType) &&
										!cpFile->attribute_.composite() );
				}
				break;

				case COMPOSITE :
				{
					composite = true;
					foundAndMatch = ( (FMS_CPF_Types::ft_REGULAR == fileType) &&
										cpFile->attribute_.composite() );
				}
				break;

				case INFINITE :
				{
					foundAndMatch = ( FMS_CPF_Types::ft_INFINITE == fileType);
				}
				break;

				default:
					TRACE(fms_cpf_FileTable, "%s", "checkCpFile(), unknown file type");
			}
		}

		// File found and match in type
		if(foundAndMatch)
		{
			TRACE(fms_cpf_FileTable, "checkCpFile(), file:<%s> found", fileName.c_str());
			// check if some attribute value has been changed after the backup
			updateObjectAttributes(fileDN, objManager, cpFile, recordLength);

			// Set Imm state aligned
			cpFile->setImmState(true);
			// Store the file DN
			cpFile->setFileDN(fileDN);

			// Check subfile of composite file
			if(composite)
				checkCpSubFile(fileDN, objManager, cpFile);
		}
		else
		{
			TRACE(fms_cpf_FileTable, "checkCpFile(), file:<%s> not found or not match", fileName.c_str());
			// File not found on data disk, remove it from IMM
			if(!deleteIMMObject(objManager, fileDN.c_str()))
			{
				char errMsg[512] = {0};
				ACE_OS::snprintf(errMsg, 511, "%s(), error:<%d> on deleteObject() on file:<%s> DN:<%s>, CP:<%s>", __func__, objManager->getInternalLastError(),
						fileName.c_str(),
						fileDN.c_str(),
						m_CpName.c_str());
				CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
				TRACE(fms_cpf_FileTable, "%s", errMsg);
			}
		}
	}
	else
	{
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "%s(), error:<%d> on getAttribute() on file:<%s> DN:<%s>, CP:<%s>", __func__, objManager->getInternalLastError(),
				fileName.c_str(),
				fileDN.c_str(),
				m_CpName.c_str());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileTable, "%s", errMsg);
	}

	TRACE(fms_cpf_FileTable, "%s", "Leaving checkCpFile()");
}

/*============================================================================
	ROUTINE: checkCpSubFile
 ============================================================================ */
void FileTable::checkCpSubFile(const std::string& fileDN, OmHandler* objManager, File* cpFile)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in checkCpSubFile()");

	std::vector<std::string> subfileDNList;
	// get all subfile of the composite file
	ACS_CC_ReturnType getResult = objManager->getChildren(fileDN.c_str(), ACS_APGCC_SUBLEVEL, &subfileDNList);

	if(ACS_CC_SUCCESS == getResult)
	{
		std::set<std::string> subFileList;

		// get list of all subfile on data disk
		cpFile->getAllSubFile(subFileList);

		std::vector<std::string>::const_iterator subfileDN;

		// for each subfile checks if it exists on data disk
		for(subfileDN = subfileDNList.begin(); subfileDN != subfileDNList.end(); ++subfileDN)
		{
			// retrieve the subfile name from its DN
			std::string subFileName;
			getLastFieldValue((*subfileDN).c_str(), subFileName);

			if(!subFileName.empty())
			{
				// search the subfile in the subfile founded on data disk
				std::set<std::string>::iterator subFile = subFileList.find(subFileName);

				if(subFileList.end() != subFile)
				{
					// subfile exists on data disk
					subFileList.erase(subFile);
				}
				else
				{
					deleteIMMObject(objManager, (*subfileDN).c_str());
				}
			}
		}

		std::vector<ACS_CC_ValuesDefinitionType> objAttrList;

		ACS_CC_ValuesDefinitionType attributeRDN;
		attributeRDN.attrName = cpf_imm::CompositeSubFileKey;
		attributeRDN.attrType = ATTR_STRINGT;
		attributeRDN.attrValuesNum = 1;

		std::set<std::string>::const_iterator subFileToCreate;

		for(subFileToCreate = subFileList.begin(); subFileToCreate != subFileList.end(); ++subFileToCreate)
		{
			// Fill the RDN Attribute
			char tmpRDN[64] = {0};

			ACE_OS::sprintf(tmpRDN, "%s=%s", cpf_imm::CompositeSubFileKey, (*subFileToCreate).c_str());
			void* valueRDN[1] = {reinterpret_cast<void*>(tmpRDN)};
			attributeRDN.attrValues = valueRDN;

			//Add the attributes to vector
			objAttrList.push_back(attributeRDN);

			if (!createIMMObject(objManager,cpf_imm::CompositeSubFileClassName, fileDN.c_str(), objAttrList, (*subFileToCreate).c_str()))
			{
				char errMsg[512] = {0};
				ACE_OS::snprintf(errMsg, 511, "FileTable::checkCpSubFile(), error:<%d> on createObject() on composite subfile:<%s> DN:<%s>, CP:<%s>", objManager->getInternalLastError(),
						(*subFileToCreate).c_str(),
						fileDN.c_str(),
						m_CpName.c_str());
				CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
				TRACE(fms_cpf_FileTable, "%s", errMsg);
			}

			objAttrList.clear();
		}
	}
	else
	{
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "FileTable::checkCpSubFile(), error:<%d> on getChildren() of composite file DN:<%s>, CP:<%s>", objManager->getInternalLastError(),
																															  fileDN.c_str(),
																															  m_CpName.c_str());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileTable, "%s", errMsg);
	}

	TRACE(fms_cpf_FileTable, "%s", "Leaving checkCpSubFile()");
}


/*============================================================================
	ROUTINE: updateObjectAttributes
 ============================================================================ */
void FileTable::updateObjectAttributes(const std::string& fileDN, OmHandler* objManager, File* cpFile, unsigned int immRecordLength)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in updateObjectAttributes()");

	unsigned int fileRecLength = cpFile->attribute_.getRecordLength();

	// File removed and re-created after backup with the same name
	if(fileRecLength != immRecordLength)
	{
		// update cp file record length
		ACS_CC_ImmParameter recordLengthAttribute;
		recordLengthAttribute.attrName = cpf_imm::recordLengthAttribute;
		recordLengthAttribute.attrType = ATTR_UINT32T;
		recordLengthAttribute.attrValuesNum = 1;
		void* tmpValue[1] = { reinterpret_cast<void*>(&fileRecLength) };
		recordLengthAttribute.attrValues = tmpValue;
		objManager->modifyAttribute(fileDN.c_str(), &recordLengthAttribute);
	}
        //HY46076 start  
          // Check if an composite file
	if(cpFile->attribute_.type() == FMS_CPF_Types::ft_REGULAR)	
	 { 
	    if(cpFile->attribute_.composite())
		{
  			 FMS_CPF_Types::fileAttributes attributeValue;
			// Retrieve attributes value from IMM
			bool result = getCompositeFileAttributes(fileDN, objManager, attributeValue);
		
	    		if(result)
			{
			 	long int deleteFileTimer = cpFile->attribute_.getDeleteFileTimer();
		 
			 // Check if it has been changed from the backup time
				if(attributeValue.regular.deleteFileTimer != deleteFileTimer)
				{
					// Value changed, update to the current one
					ACS_CC_ImmParameter deleteFileTimerAttribute;
					deleteFileTimerAttribute.attrName = cpf_imm::deleteFileTimerAttribute;
					deleteFileTimerAttribute.attrType = ATTR_UINT32T;
					deleteFileTimerAttribute.attrValuesNum = 1;
					void* tmpValue[1] = { reinterpret_cast<void*>(&deleteFileTimer) };
					deleteFileTimerAttribute.attrValues = tmpValue;
					objManager->modifyAttribute(fileDN.c_str(), &deleteFileTimerAttribute);
				} 
       			 }
		}			
	 }	
	//HY46076 end 
	// Check if an infinite file
	if(cpFile->attribute_.type() == FMS_CPF_Types::ft_INFINITE)
	{
		FMS_CPF_Types::fileAttributes attributeValue;
		// Retrieve attributes value from IMM
		bool result = getInfiniteFileAttributes(fileDN, objManager, attributeValue);

		if(result)
		{
			unsigned long fileMaxSize, fileMaxTime, active, lastSent;
			FMS_CPF_Types::transferMode tqMode, initMode;
			std::string transferQueue;
			bool fileRelCond;
			cpFile->attribute_.extractExtAttributes(fileMaxSize, fileMaxTime, fileRelCond,
													active, lastSent, tqMode,
													transferQueue, initMode);

			// Check if it has been changed from the backup time
			if(attributeValue.infinite.maxsize != fileMaxSize)
			{
				// Value changed, update to the current one
				ACS_CC_ImmParameter maxSizeAttribute;
				maxSizeAttribute.attrName = cpf_imm::maxSizeAttribute;
				maxSizeAttribute.attrType = ATTR_UINT32T;
				maxSizeAttribute.attrValuesNum = 1;
				void* tmpValue[1] = { reinterpret_cast<void*>(&fileMaxSize) };
				maxSizeAttribute.attrValues = tmpValue;
				objManager->modifyAttribute(fileDN.c_str(), &maxSizeAttribute);
			}

			// Check if it has been changed from the backup time
			if(attributeValue.infinite.maxtime != fileMaxTime)
			{
				// Value changed, update to the current one
				// convert the value from seconds to minutes
				unsigned long fileMaxTimeInMinute = static_cast<unsigned long>(fileMaxTime / stdValue::SecondsInMinute);
				ACS_CC_ImmParameter maxTimeAttribute;
				maxTimeAttribute.attrName = cpf_imm::maxTimeAttribute;
				maxTimeAttribute.attrType = ATTR_UINT32T;
				maxTimeAttribute.attrValuesNum = 1;
				void* tmpValue[1] = { reinterpret_cast<void*>(&fileMaxTimeInMinute) };
				maxTimeAttribute.attrValues = tmpValue;
				objManager->modifyAttribute(fileDN.c_str(), &maxTimeAttribute);
			}

			// Check if it has been changed from the backup time
			if(attributeValue.infinite.release != fileRelCond)
			{
				int releaseConditionValue = (fileRelCond ? 1 : 0);
				// Value changed, update to the current one
				ACS_CC_ImmParameter relCondAttribute;
				relCondAttribute.attrName = cpf_imm::releaseCondAttribute;
				relCondAttribute.attrType = ATTR_INT32T;
				relCondAttribute.attrValuesNum = 1;
				void* tmpValue[1] = { reinterpret_cast<void*>(&releaseConditionValue) };
				relCondAttribute.attrValues = tmpValue;
				objManager->modifyAttribute(fileDN.c_str(), &relCondAttribute);
			}

			// Get current IMM attribute value
			if( FMS_CPF_Types::tm_NONE != tqMode )
			{
				// Check if it has been changed from the backup time
				if(transferQueue.compare(attributeValue.infinite.transferQueue) != 0)
				{
					char tmpFileTQ[transferQueue.length() + 1];
					ACE_OS::strcpy(tmpFileTQ, transferQueue.c_str());
					void* tmpValue[1] = {reinterpret_cast<void*>(tmpFileTQ)};

					// Value changed, update to the current one
					ACS_CC_ImmParameter fileTQAttribute;

					fileTQAttribute.attrName = cpf_imm::fileTQAttribute;
					fileTQAttribute.attrType = ATTR_STRINGT;
					fileTQAttribute.attrValuesNum = 1;
					fileTQAttribute.attrValues = tmpValue;

					objManager->modifyAttribute(fileDN.c_str(), &fileTQAttribute);
				}
			}
			else if(FMS_CPF_Types::tm_NONE != attributeValue.infinite.mode )
			{
				// TQ defined in IMM, but removed after the backup
				// Remove any value
				char tmpFileTQ[2]={0};
				void* tmpValue[1] = {reinterpret_cast<void*>(tmpFileTQ)};

				// Value changed, update to the current one
				ACS_CC_ImmParameter fileTQAttribute;

				fileTQAttribute.attrName = cpf_imm::fileTQAttribute;

				fileTQAttribute.attrType = ATTR_STRINGT;
				fileTQAttribute.attrValuesNum = 1;

				fileTQAttribute.attrValues = tmpValue;
				objManager->modifyAttribute(fileDN.c_str(), &fileTQAttribute);
			}
		}
	}
	TRACE(fms_cpf_FileTable, "%s", "Leaving updateObjectAttributes()");
}

/*============================================================================
	ROUTINE: loadCpFileFromIMM
 ============================================================================ */
bool FileTable::loadCpFileFromIMM(const std::vector<std::string>& listVolumeDN, bool restore)
{
	TRACE(fms_cpf_FileTable, "Entering in loadCpFileFromIMM(<%s>)", m_CpName.c_str());
	bool result = true;
	OmHandler objManager;
	// Init OM resource
	if(objManager.Init() != ACS_CC_FAILURE)
	{
		// Set this Cp file root
		m_CpRoot = (ParameterHndl::instance()->getCPFroot(m_CpName.c_str()) + DirDelim);

		if(!m_isMultiCP && listVolumeDN.empty())
		{
			// create Default volumes
			createVolume(defaultVolume::TEMPVOLUME, &objManager);
			checkVolumeFolder(defaultVolume::TEMPVOLUME);

			createVolume(defaultVolume::RELVOLUMSW, &objManager);
			checkVolumeFolder(defaultVolume::RELVOLUMSW);

			createVolume(defaultVolume::EXCHVOLUME, &objManager);
			checkVolumeFolder(defaultVolume::EXCHVOLUME);
		}

		std::vector<std::string>::const_iterator volumeDN;
		// get all cp file for each volume
		for(volumeDN = listVolumeDN.begin(); volumeDN != listVolumeDN.end(); ++volumeDN)
		{
			std::string volumeName;
			// get the volume name
			if( getVolumeName((*volumeDN), volumeName) )
			{
				TRACE(fms_cpf_FileTable, "loadCpFileFromIMM(), volume:<%s>", volumeName.c_str());

				// Check physical folder
				checkVolumeFolder(volumeName);

				std::vector<std::string> fileDNList;
				// get all file of this volume
				if(objManager.getChildren((*volumeDN).c_str(), ACS_APGCC_SUBLEVEL, &fileDNList) == ACS_CC_SUCCESS )
				{
					bool composite;
					std::vector<std::string>::const_iterator fileDN;
					// insert each file in the internal file table
					for(fileDN = fileDNList.begin(); fileDN != fileDNList.end(); ++fileDN)
					{
						composite = true;
						switch( getFileType((*fileDN)) )
						{
							case SIMPLE :
								composite = false;
							case COMPOSITE :
							{
								addCpRegularFile((*fileDN), volumeName, &objManager, composite, restore);
							}
							break;

							case INFINITE :
							{
								addCpInfiniteFile((*fileDN), volumeName, &objManager, restore);
							}
							break;

							default:
								TRACE(fms_cpf_FileTable, "%s", "loadCpFileFromIMM(), unknown file type");
						}
					}
				}
				else
				{
					char errMsg[512] = {0};
					ACE_OS::snprintf(errMsg, 511, "FileTable::loadCpFileFromIMM(), error:<%d> on getChildren() of volume:<%s> DN:<%s>, CP:<%s>", objManager.getInternalLastError(),
																																		  volumeName.c_str(),
																																		  (*volumeDN).c_str(),
																																		  m_CpName.c_str());
					CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
					TRACE(fms_cpf_FileTable, "%s", errMsg);
				}
			}
		}
		// Deallocate OM resource
		objManager.Finalize();
	}
	else
	{
		//Error Handling
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "FileTable::loadCpFileFromIMM(), error:<%d> on OmHandler object init(), CP:<%s>", objManager.getInternalLastError(), m_CpName.c_str());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileTable, "%s", errMsg);
		result = false;
	}
	TRACE(fms_cpf_FileTable, "%s", "Leaving loadCpFileFromIMM()");
	return result;
}

/*============================================================================
	ROUTINE: getVolumeName
 ============================================================================ */
bool FileTable::getVolumeName(const std::string& volumeDN, std::string& volumeName )
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in getVolumeName()");

	volumeName.clear();
	bool result = false;

	std::string volumeRDN;
	getLastFieldValue(volumeDN, volumeRDN);

	// get volume RDN
	if( !volumeRDN.empty() )
	{
		TRACE(fms_cpf_FileTable, "getVolumeName(), volume RDN:<%s>", volumeRDN.c_str());

		if(m_isMultiCP)
		{
			// MCP, volume RDN = volumeName:cpName
			size_t tagAtSignPos = volumeRDN.find_first_of(parseSymbol::atSign);

			// Check if the tag is present
			if( std::string::npos != tagAtSignPos )
			{
				// get the volumeName from RDN
				volumeName = volumeRDN.substr(0, tagAtSignPos);
				result = true;
			}
			else
			{
				char errMsg[512] = {0};
				ACE_OS::snprintf(errMsg, 511, "FileTable::getVolumeName(DN:<%s>), tag <%c> not found in <%s>", volumeDN.c_str(), parseSymbol::atSign, volumeRDN.c_str());
				CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
				TRACE(fms_cpf_FileTable, "%s", errMsg);
			}
		}
		else
		{
			// SCP, volume RDN = volumeName
			volumeName = volumeRDN;
			result = true;
		}
	}
	TRACE(fms_cpf_FileTable, "%s", "Leaving getVolumeName()");
	return result;
}


/*============================================================================
	ROUTINE: addCpRegularFile
 ============================================================================ */
void FileTable::addCpRegularFile(const std::string& fileDN, const std::string& volumeName, OmHandler* objManager, bool composite, bool restore)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in addCpRegularFile()");

	//List of attributes to get
	std::vector<ACS_APGCC_ImmAttribute*> attributeList;

	// get the filename and recordLength attributes of a regular file
	std::string fileName;

	getLastFieldValue(fileDN, fileName);

	ACS_APGCC_ImmAttribute recordLengthAttribute;
	recordLengthAttribute.attrName = cpf_imm::recordLengthAttribute;
	attributeList.push_back(&recordLengthAttribute);
	//	HY46076
	ACS_APGCC_ImmAttribute deleteFileTimerAttribute;
	if((!m_isMultiCP) && composite)   //HY89517
	{
		deleteFileTimerAttribute.attrName = cpf_imm::deleteFileTimerAttribute;
		attributeList.push_back(&deleteFileTimerAttribute);
	}
	//      HY46076 end
	// Get attributes by IMM
	ACS_CC_ReturnType getResult = objManager->getAttribute(fileDN.c_str(), attributeList );

	if( (ACS_CC_FAILURE != getResult) && !fileName.empty()
			&& (0 != recordLengthAttribute.attrValuesNum ) )
	{
		FMS_CPF_Types::fileAttributes fileRegularAttr;
		fileRegularAttr.ftype = FMS_CPF_Types::ft_REGULAR;

		fileRegularAttr.regular.rlength = (*reinterpret_cast<unsigned int*>(recordLengthAttribute.attrValues[0]));
		fileRegularAttr.regular.composite = composite;	
		//HY46076 start
		if((!m_isMultiCP) && (composite) && (0 != deleteFileTimerAttribute.attrValuesNum )) //HY89517
		{
			fileRegularAttr.regular.deleteFileTimer = (*reinterpret_cast<long int*>(deleteFileTimerAttribute.attrValues[0]));
			FMS_CPF_JTPConnectionHndl::updateDeleteFileTimer(fileName, fileRegularAttr.regular.deleteFileTimer);
			FMS_CPF_JTPConnectionHndl::initFromFile(fileName);	
		}
		//HY46076 end
		//create the attribute object
		FMS_CPF_Attribute regularAttribute(fileRegularAttr);

		TRACE(fms_cpf_FileTable,"addCpRegularFile(), file:<%s>, recordLegth:<%d>, composite:<%s>, deleteFileTimer:<%d>", fileName.c_str(), fileRegularAttr.regular.rlength, (composite ? "YES" : "NO"), fileRegularAttr.regular.deleteFileTimer);

		FMS_CPF_FileId cpFileId = fileName;

		File* newRegFileHndl = new (std::nothrow) File(cpFileId, volumeName, m_CpName, regularAttribute);

		if(NULL != newRegFileHndl)
		{
			// Check for restart after a data disk restore
			if(restore) 
			{
				TRACE(fms_cpf_FileTable, "%s", "addCpRegularFile(), restore of regular Cp File");
				// Create physical file and attribute file
				try
				{
					FileMgr filemgr(cpFileId, volumeName, regularAttribute, m_CpName.c_str());
					filemgr.create();
				}
				catch(FMS_CPF_PrivateException& ex)
				{
					// error handling
					char errMsg[512] = {0};
					ACE_OS::snprintf(errMsg, 511, "addCpRegularFile(), Error:<%s,%s> on file:<%s> restore, Cp:<%s> ", ex.errorText(), ex.detailInfo().c_str(), fileName.c_str(), m_CpName.c_str());
					CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
					TRACE(fms_cpf_FileTable, "%s", errMsg);
				}
			}

			//Store the File DN
			newRegFileHndl->setFileDN(fileDN);

			// check type of regular file
			if(composite)
			{
				// Composite file check physical folder
				checkMainFileFolder(fileName, volumeName);

				// get all subfiles of composite file
				addCompositeSubFile(fileDN, objManager, newRegFileHndl);
			}
			else
			{
				checkFile(fileName, volumeName);
			}

			// Insert the file
			m_FileTable.push_back(newRegFileHndl);
		}
		else
		{
			//Error Handling
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "addCpRegularFile(DN:<%s>), error on File object creation of File:<%s> on Cp:<%s>", fileDN.c_str(), fileName.c_str(), m_CpName.c_str() );
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_FileTable, "%s", errMsg);
		}

		TRACE(fms_cpf_FileTable, "%s", "addCpRegularFile(), file added in the file table");
	}
	else
	{
		//Error Handling
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "FileTable::addCpRegularFile(DN:<%s>), error:<%d> on OmHandler ", fileDN.c_str(),  objManager->getInternalLastError());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileTable, "%s", errMsg);
	}

	TRACE(fms_cpf_FileTable, "%s", "Leaving addCpRegularFile()");
}

/*============================================================================
	ROUTINE: addCompositeSubFile
 ============================================================================ */
void FileTable::addCompositeSubFile(const std::string& fileDN, OmHandler* objManager, File* compositeFileHndl)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in addCompositeSubFile()");
	std::vector<std::string> subFileDNList;
	ACS_CC_ReturnType getResult;

	// get all subfile of this composite file
	getResult = objManager->getChildren(fileDN.c_str(), ACS_APGCC_SUBLEVEL, &subFileDNList);
	if(ACS_CC_SUCCESS == getResult)
	{
		std::string compositeFileName = compositeFileHndl->fileid_.data();
		compositeFileName += DirDelim;

		std::vector<std::string>::const_iterator subFileDN;
		// insert each file in the internal file table
		for(subFileDN = subFileDNList.begin(); subFileDN != subFileDNList.end(); ++subFileDN)
		{
			// Get the filename
			std::string subFileName;
			getLastFieldValue((*subFileDN).c_str(), subFileName);

			if( !subFileName.empty() )
			{
				compositeFileHndl->addSubFile(subFileName);
				std::string relativeFileName = compositeFileName + subFileName;
				checkFile(relativeFileName, compositeFileHndl->volume_);

				TRACE(fms_cpf_FileTable, "addCompositeSubFile(), added subFile:<%s>", subFileName.c_str());
			}
			else
			{
				//Error Handling
				char errMsg[512] = {0};
				ACE_OS::snprintf(errMsg, 511, "FileTable::addCompositeSubFile(), error:<%d> on getAttribute(DN:<%s>)", objManager->getInternalLastError(), (*subFileDN).c_str() );
				CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
				TRACE(fms_cpf_FileTable, "%s", errMsg);
			}
		}
	}
	else
	{
		//Error Handling
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "FileTable::addCompositeSubFile(DN:<%s>), error:<%d> on getChildren()", fileDN.c_str(),  objManager->getInternalLastError());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileTable, "%s", errMsg);
	}

	TRACE(fms_cpf_FileTable, "%s", "Leaving addCompositeSubFile()");
}


/*============================================================================
	ROUTINE: addCpInfiniteFile
 ============================================================================ */
void FileTable::addCpInfiniteFile(const std::string& fileDN, const std::string& volumeName, OmHandler* objManager, bool restore)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in addCpInfiniteFile()");

	// to get the filename of a infinite file
	std::string fileName;
	getLastFieldValue(fileDN, fileName);

	// check for mandatory attributes
	if( !fileName.empty() )
	{
		TRACE(fms_cpf_FileTable,"addCpInfiniteFile(), file:<%s>", fileName.c_str() );

		std::string filePath = (m_CpRoot + volumeName + DirDelim + fileName);

		FMS_CPF_FileId cpFileId = fileName;

		// Check for restart after data disk restore
		if(restore)
		{
			// Re-create physical file
			TRACE(fms_cpf_FileTable, "%s", "addCpRegularFile(), restore of regular Cp File");

			FMS_CPF_Types::fileAttributes attributeValue;

			// Retrieve attributes value from IMM
			getInfiniteFileAttributes(fileDN, objManager, attributeValue);

			FMS_CPF_Attribute tmpAttribute(attributeValue);
			try
			{
				FileMgr filemgr(cpFileId, volumeName, tmpAttribute, m_CpName.c_str());
				filemgr.create();
			}
			catch(FMS_CPF_PrivateException& ex)
			{
				// error handling
				char errMsg[512] = {0};
				ACE_OS::snprintf(errMsg, 511, "addCpInfiniteFile(), Error:<%s,%s> on file:<%s> restore, Cp:<%s> ", ex.errorText(), ex.detailInfo().c_str(), fileName.c_str(), m_CpName.c_str());
				CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
				TRACE(fms_cpf_FileTable, "%s", errMsg);
			}
		}

		try
		{
			FMS_CPF_Attribute infiniteAttribute;
			// Read the attributes from the data disk
			infiniteAttribute.readFile(filePath);

			File* infiteFile = new (std::nothrow) File(cpFileId, volumeName, m_CpName, infiniteAttribute);

			// Insert the file
			if(NULL != infiteFile)
			{
				//Store the File DN
				infiteFile->setFileDN(fileDN);

				// check physical folder
				checkMainFileFolder(fileName, volumeName);

				m_FileTable.push_back(infiteFile);
			}
			else
			{
				//Error Handling
				char errMsg[512] = {0};
				ACE_OS::snprintf(errMsg, 511, "addCpInfiniteFile(DN:<%s>), error on File object creation of File:<%s> on Cp:<%s>", fileDN.c_str(), fileName.c_str(), m_CpName.c_str() );
				CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
				TRACE(fms_cpf_FileTable, "%s", errMsg);
			}
		}
		catch(FMS_CPF_PrivateException& ex)
		{
			// attribute error
			char errMsg[512]={0};
			ACE_OS::snprintf(errMsg, 511, "addCpInfiniteFile(), Error:<%s,%s> on file:<%s> of Cp:<%s> load", ex.errorText(), ex.detailInfo().c_str(), fileName.c_str(), m_CpName.c_str());
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_FileTable, "%s", errMsg);
		}
	}
	TRACE(fms_cpf_FileTable, "%s", "Leaving addCpInfiniteFile()");
}

/*============================================================================
	ROUTINE: getInfiniteFileAttributes
 ============================================================================ */
bool FileTable::getInfiniteFileAttributes(const std::string& fileDN, OmHandler* objManager, FMS_CPF_Types::fileAttributes& attributeValue)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in getInfiniteFileAttributes()");
	bool result = false;

	attributeValue.ftype = FMS_CPF_Types::ft_INFINITE;
	attributeValue.infinite.rlength = 1024;
	attributeValue.infinite.active = 0;
	attributeValue.infinite.lastReportedSubfile = 0;
	attributeValue.infinite.maxsize = 0;
	attributeValue.infinite.maxtime = 0;
	attributeValue.infinite.release = false;

	// initialize TQ info
	attributeValue.infinite.mode = FMS_CPF_Types::tm_UNDEF;
	attributeValue.infinite.inittransfermode = FMS_CPF_Types::tm_UNDEF;
	ACE_OS::memset(attributeValue.infinite.transferQueue,0,FMS_CPF_TQMAXLENGTH+1);

	//List of attributes to get
	std::vector<ACS_APGCC_ImmAttribute*> attributeList;

	// to get the record length of a infinite file
	ACS_APGCC_ImmAttribute recordLengthAttribute;
	recordLengthAttribute.attrName = cpf_imm::recordLengthAttribute;
	attributeList.push_back(&recordLengthAttribute);

	// to get the maxSize of a infinite file
	ACS_APGCC_ImmAttribute maxSizeAttribute;
	maxSizeAttribute.attrName = cpf_imm::maxSizeAttribute;
	attributeList.push_back(&maxSizeAttribute);

	// to get the maxTime of a infinite file
	ACS_APGCC_ImmAttribute maxTimeAttribute;
	maxTimeAttribute.attrName = cpf_imm::maxTimeAttribute;
	attributeList.push_back(&maxTimeAttribute);

	// to get the release Condition of a infinite file
	ACS_APGCC_ImmAttribute relCondAttribute;
	relCondAttribute.attrName = cpf_imm::releaseCondAttribute;
	attributeList.push_back(&relCondAttribute);

	// to get the file TQ of a infinite file
	ACS_APGCC_ImmAttribute fileTQAttribute;
	fileTQAttribute.attrName = cpf_imm::fileTQAttribute;
	attributeList.push_back(&fileTQAttribute);

	ACS_CC_ReturnType getResult = objManager->getAttribute(fileDN.c_str(), attributeList );
	// check for mandatory attributes
	if( ACS_CC_FAILURE != getResult)
	{
		result = true;
		// Get current IMM attribute value
		if( 0 != recordLengthAttribute.attrValuesNum )
		{
			attributeValue.infinite.rlength = (*reinterpret_cast<unsigned int*>(recordLengthAttribute.attrValues[0]));
		}

		// Get current IMM attribute value
		if(0 != maxSizeAttribute.attrValuesNum )
		{
			attributeValue.infinite.maxsize = (*reinterpret_cast<unsigned int*>(maxSizeAttribute.attrValues[0]));
		}

		// Get current IMM attribute value
		if(0 != maxTimeAttribute.attrValuesNum )
		{
			attributeValue.infinite.maxtime = stdValue::SecondsInMinute * (*reinterpret_cast<unsigned int*>(maxTimeAttribute.attrValues[0]));
		}

		// Get current IMM attribute value
		if(0 != relCondAttribute.attrValuesNum )
		{
			int releaseConditionValue = (*reinterpret_cast<int*>(relCondAttribute.attrValues[0]));
			attributeValue.infinite.release = (bool)releaseConditionValue;
		}

		// Get current IMM attribute value
		if(0 != fileTQAttribute.attrValuesNum)
		{
			std::string immTQ(reinterpret_cast<char*>(fileTQAttribute.attrValues[0]));

			ACE_OS::strncpy(attributeValue.infinite.transferQueue, immTQ.c_str(), FMS_CPF_TQMAXLENGTH);

			int errorCode = 0;
			// Check is a block TQ
			if(TQChecker::instance()->validateBlockTQ(immTQ, errorCode))
			{
				attributeValue.infinite.mode = FMS_CPF_Types::tm_BLOCK;
			}
			else
			{
				TRACE(fms_cpf_FileTable, "getInfiniteFileAttributes(), TQ:<%s> not a block TQ, error:<%d>", immTQ.c_str(), errorCode);
				attributeValue.infinite.mode = FMS_CPF_Types::tm_FILE;
			}

			attributeValue.infinite.inittransfermode = attributeValue.infinite.mode;
		}
	}
	else
	{
		//Error Handling
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "getInfiniteFileAttributes(DN:<%s>), error:<%d> on OmHandler ", fileDN.c_str(),  objManager->getInternalLastError());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileTable, "%s", errMsg);
	}

	TRACE(fms_cpf_FileTable, "%s", "Leaving getInfiniteFileAttributes()");
	return result;
}


/*============================================================================
	ROUTINE: checkVolumeFolder
 ============================================================================ */
void FileTable::checkVolumeFolder(const std::string& volumeName)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in checkVolumeFolder()");

	std::string fullVolumePath = m_CpRoot + volumeName;

	TRACE(fms_cpf_FileTable, "checkVolumeFolder(), physical folder:<%s>", fullVolumePath.c_str());
	//check if volume exists
	ACE_stat statbuf;
	if( (ACE_OS::stat(fullVolumePath.c_str(), &statbuf) == FAILURE) && (errno == ENOENT) )
	{
		TRACE(fms_cpf_FileTable, "%s", "checkVolumeFolder(), folder not found");
		// volume folder does not exist, create it
		if( ACS_APGCC::create_directories(fullVolumePath.c_str()) != 1)
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "checkVolumeFolder(), error:<%d> on create volume:<%s>", errno, fullVolumePath.c_str());
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_FileTable, "%s", errMsg);
		}
		else
		{
			TRACE(fms_cpf_FileTable, "%s", "checkVolumeFolder(), folder created");
		}
	}

	TRACE(fms_cpf_FileTable, "%s", "Leaving checkVolumeFolder()");
}

/*============================================================================
	ROUTINE: checkMainFileFolder
 ============================================================================ */
void FileTable::checkMainFileFolder(const std::string& mainFileName, const std::string& volumeName)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in checkMainFileFolder()");

	std::string fullMainFilePath = m_CpRoot + volumeName + DirDelim + mainFileName;

	TRACE(fms_cpf_FileTable, "checkMainFileFolder(), physical folder:<%s>", fullMainFilePath.c_str());

	//check if volume exists
	ACE_stat statbuf;
	if( (ACE_OS::stat(fullMainFilePath.c_str(), &statbuf) == -1) && (errno == ENOENT) )
	{
		TRACE(fms_cpf_FileTable, "%s", "checkMainFileFolder(), main file folder not found");
		mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
		ACE_OS::umask(0);
		if(ACE_OS::mkdir(fullMainFilePath.c_str(), mode ) == FAILURE)
		{
			 char errorText[256] = {0};
			 std::string errorDetail(strerror_r(errno, errorText, 255));
			 char errMsg[512] = {0};
			 ACE_OS::snprintf(errMsg, 511, "checkMainFileFolder(), error:<%s> to create main file:<%s>", errorDetail.c_str(), fullMainFilePath.c_str());
			 CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			 TRACE(fms_cpf_FileTable, "%s", errMsg);
		}
	}
	TRACE(fms_cpf_FileTable, "%s", "Leaving checkMainFileFolder()");
}

/*============================================================================
	ROUTINE: checkFile
 ============================================================================ */
void FileTable::checkFile(const std::string& fileName, const std::string& volumeName)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in checkFile()");

	std::string fullFilePath = m_CpRoot + volumeName + DirDelim + fileName;

	TRACE(fms_cpf_FileTable, "checkFile(), physical file:<%s>", fullFilePath.c_str());
	ACE_stat statbuf;
	if( (ACE_OS::stat(fullFilePath.c_str(), &statbuf) == FAILURE) && (errno == ENOENT) )
	{
		TRACE(fms_cpf_FileTable, "%s", "checkFile(), physical file not found");

		// creation flag
		int oflag = O_WRONLY | O_CREAT | O_TRUNC | _O_BINARY;

		// Set permission 744 (RWXR--R--) on file
		mode_t mode = S_IRWXU | S_IRGRP | S_IROTH;
		ACE_OS::umask(0);

		// file creation
		int fileHandle = ACE_OS::open(fullFilePath.c_str(), oflag, mode);

		// create the physical file
		if( FAILURE != fileHandle )
		{
			ACE_OS::close(fileHandle);
		}
		else
		{
			char errorText[256] = {0};
			std::string errorDetail(strerror_r(errno, errorText, 255));
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "checkFile(), error:<%s> to create main file:<%s>", errorDetail.c_str(), fullFilePath.c_str());
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_FileTable, "%s", errMsg);
		}
	}
	TRACE(fms_cpf_FileTable, "%s", "Leaving checkFile()");
}

/*============================================================================
	ROUTINE: getFileType
 ============================================================================ */
FileTable::cpFileType FileTable::getFileType(const std::string& fileDN)
{
	cpFileType fileType = COMPOSITE;
	size_t found;

	// First try composite
	found = fileDN.find(cpf_imm::CompositeFileKey);

	if(std::string::npos == found)
	{
		// Second try infinite
		fileType = INFINITE;

		found = fileDN.find(cpf_imm::InfiniteFileKey);
		if(std::string::npos == found)
		{
			// else last chance simple
			fileType = SIMPLE;

			found = fileDN.find(cpf_imm::SimpleFileKey);
			if(std::string::npos == found)
			{
				// error on file type
				fileType = UNKNOW;
			}
		}
	}
	return fileType;
}


File* FileTable::find(const FMS_CPF_FileId id)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in find()");
	File* element = NULL;

	FileTableList::iterator listPtr;
	for(listPtr = m_FileTable.begin(); listPtr != m_FileTable.end(); ++listPtr)
    {
		TRACE(fms_cpf_FileTable, "find(), file:<%s>", (*listPtr)->fileid_.data());
    	if((*listPtr)->fileid_ == id)
    	{
    		element = (*listPtr);
    		break;
    	}
    }
    TRACE(fms_cpf_FileTable, "%s", "Leaving find()");
    return element;
}

bool FileTable::exists (const FMS_CPF_FileId& fileid)
throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering exists()");

	bool result = false;

	// File identity valid ?
	if (!fileid.isValid ())
	{
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::INVALIDFILE);
	}
	
	// Check if main file exists
	File* filep = find(fileid.file());
	if( NULL == filep )
	{
		TRACE(fms_cpf_FileTable, "%s", "exists(), returns FALSE on main file");
		return result;
	}

	bool isSimpleFile = (filep->attribute_.composite() == false);
	// Checks the file type main or subfile
	if( fileid.subfileAndGeneration().empty() )
	{
		// Main file check
		// exist into IMM
		result = true;

		// Check if physical file exists
		// if it does not exist create it physically
		if(isSimpleFile)
		{
			TRACE(fms_cpf_FileTable, "%s", "exists(), simple file check");
			checkFile(fileid.file(), filep->volume_);
		}
		else
		{
			TRACE(fms_cpf_FileTable, "%s", "exists(), main file check");
			checkMainFileFolder(fileid.file(), filep->volume_);
		}
	}
	else
	{
		// Subfile
		// Check if a simple file
		if(!isSimpleFile )
		{
			// Check if a infinite subfile only a physical check
			if( filep->getFileType() == FMS_CPF_Types::ft_INFINITE )
			{
				// an infinite subfile makes only a physical check
				FileMgr filemgr(fileid, filep->volume_, filep->attribute_, m_CpName.c_str());
				result = filemgr.exists();
				TRACE(fms_cpf_FileTable, "%s", "exists(), infinite subfile check");
			}
			else
			{
				// Check if defined into IMM
				result = filep->subFileExist(fileid.subfileAndGeneration());

				// Check physical file exist, create it if not exist
				if(result)
				{
					std::string relativeFilePath = fileid.file() + DirDelim + fileid.subfileAndGeneration();
					checkFile(relativeFilePath, filep->volume_);
				}
				TRACE(fms_cpf_FileTable, "%s", "exists(), composite subfile check");
			}
		}
		else
		{
			TRACE(fms_cpf_FileTable, "%s", "exists(), subfile of a simple file never exist");
		}
	}

	TRACE(fms_cpf_FileTable, "Leaving exists(), result:<%s>", (result ? "TRUE" : "FALSE"));
	return result;
}

//------------------------------------------------------------------------------
//      Create file
//------------------------------------------------------------------------------

FileReference FileTable::create(const FMS_CPF_FileId& fileid,
								const std::string& volume,
								const FMS_CPF_Attribute& attribute,
								FMS_CPF_Types::accessType access,
								const std::string& fileDN)
throw (FMS_CPF_PrivateException)
{
	// Allow only one thread at the time to enter this code.
	// guard is destroyed when out of scope, automatically releasing the lock.
	ACE_Guard<ACE_Thread_Mutex> guard(m_FileTableLock);

	TRACE(fms_cpf_FileTable, "%s", "Entering in create(...)");
	
	// checks if the File identity is valid
	if (!fileid.isValid ())
	{
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::INVALIDFILE);
	}
	FileDescriptor* fd;

	// Checks the file type main or subfile
	if( fileid.subfileAndGeneration().empty() )
	{
		// Main file
		
		// Check if file already exists
		if( find(fileid.file()) != 0 )
		{
			TRACE(fms_cpf_FileTable, "%s", "create(...), The physical file to create already exists");
			throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::FILEEXISTS, fileid.file().c_str());
		}

		TRACE(fms_cpf_FileTable, "%s", "create(...) Create physical file ");

		// Create physical file
		FileMgr filemgr(fileid, volume, attribute, m_CpName.c_str());
		filemgr.create();
		
		// Insert file in file table
		File* filep = new File(fileid, volume, m_CpName, attribute);
		filep->setFileDN(fileDN);
		m_FileTable.push_back(filep);
		
		// Insert entry in file access and create file descriptor
		filep->faccessp_ = new FileAccess (fileid, filep);
		fd = filep->faccessp_->create(access, false);
	}
	else
	{
		// Subfile
		TRACE(fms_cpf_FileTable, "%s", "create(...), The file is not a simple file or a main composite file");

		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTBASEFILE, fileid.data ());
	}
	
	TRACE(fms_cpf_FileTable, "%s", "Leaving create(...)");
	return FileReference::insert(fd);
}

//------------------------------------------------------------------------------
//      Create subfile
//------------------------------------------------------------------------------
FileReference FileTable::create(const FMS_CPF_FileId& fileid, FMS_CPF_Types::accessType access)
throw (FMS_CPF_PrivateException)
{
	// Allow only one thread at the time to enter this code.
	// guard is destroyed when out of scope, automatically releasing the lock.
	ACE_Guard<ACE_Thread_Mutex> guard(m_FileTableLock);

	TRACE(fms_cpf_FileTable, "%s", "Entering in create() subfile ");
	
	FileDescriptor* fd;
	if( !fileid.subfileAndGeneration().empty() )
	{
		// Subfile
		
		// Check if main file exists
		File* filep = find(fileid.file());
		if( NULL == filep )
		{
			TRACE(fms_cpf_FileTable, "%s", "create() subfile, the main file not exists ");
			throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::FILENOTFOUND, fileid.file().c_str());
		}
		
		TRACE(fms_cpf_FileTable, "%s", "create() subfile, check file type");
		// Check if main file is composite
		if( !filep->attribute_.composite() )
		{
			TRACE(fms_cpf_FileTable, "%s", "create() subfile, the main file is not a composite file");
			throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTCOMPOSITE, fileid.file ().c_str());
		}

		if(filep->subFileExist(fileid.subfileAndGeneration()))
		{
			TRACE(fms_cpf_FileTable, "%s", "create(), subfile already exists");
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILEEXISTS, fileid.data() );
		}

		TRACE(fms_cpf_FileTable, "%s", "create() subfile,  file ok");

		// Check access against main file first
		if( filep->faccessp_ )
		{
			if( !filep->faccessp_->checkAccess(access) )
			{
				TRACE(fms_cpf_FileTable, "%s", "create() subfile, Error to access main file");
				throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::ACCESSERROR);
			}
		}
		
		// Create physical file
		FileMgr filemgr (fileid, filep->volume_, filep->attribute_, m_CpName.c_str());
		filemgr.create();
		
		// Logical adds of subfile to its composite file
		filep->addSubFile(fileid.subfileAndGeneration());

		// Insert entry in subfile access list and create file descriptor
		FileAccess* faccessp = new FileAccess (fileid, filep);
		filep->subfilelist_.push_back (faccessp);
		fd = faccessp->create (access, false);
	}
	else
	{
		// Main file
		TRACE(fms_cpf_FileTable, "%s", "create() subfile, the file is not a subfile");
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTSUBFILE, fileid.data ());
	}
	
	TRACE(fms_cpf_FileTable, "%s", "create subfile leaving");
	return FileReference::insert(fd);
}

//------------------------------------------------------------------------------
//      Close file
//------------------------------------------------------------------------------
void FileTable::close(FileReference ref) throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in close()");

	// Allow only one thread at the time to enter this code.
	// guard is destroyed when out of scope, automatically releasing the lock.
	ACE_Guard<ACE_Thread_Mutex> guard(m_FileTableLock);

	FileDescriptor* fd;
	if ((fd = FileReference::find(ref)) == 0)
	{
		TRACE(fms_cpf_FileTable, "%s", "close(), Invalid reference file");
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::INVALIDREF);
	}

	FileAccess* faccessp = fd->faccessp_;
	File* filep = faccessp->filep_;

	if (faccessp == filep->faccessp_)
	{
		TRACE(fms_cpf_FileTable, "%s", "close(), Main file closure");
		// Main file
		// Remove entry from file access and delete file descriptor
		faccessp->remove(fd);
		if(faccessp->fdList_.empty())
		{
			TRACE(fms_cpf_FileTable, "%s", "close(), Main file fdList_ empty remove fileaccess");
			delete faccessp;
			filep->faccessp_ = 0;
		}
	}
	else
	{
		TRACE(fms_cpf_FileTable, "%s", "close(), subfile closure");
		// Subfile
		faccessp->remove(fd);
		if(faccessp->fdList_.empty())
		{
			FileAccess key(faccessp->fileid_);
			SubFileList::iterator listPtr;

			SubFileList::size_type listSize;
			listSize = filep->subfilelist_.size();

			// search the FileAccess object with the same fileId
			for(listPtr = filep->subfilelist_.begin(); listPtr != filep->subfilelist_.end(); ++listPtr)
			{
				FileAccess *next = *listPtr;
				if(*next == key)
				{
					// found and remove from the list
					listPtr = filep->subfilelist_.erase(listPtr);
					break;
				}
			}

			delete faccessp;

			if(filep->subfilelist_.size() == listSize )
			{
				TRACE(fms_cpf_FileTable, "%s", "close(), error on subfile key remove");
				EventReport::instance()->reportException("close(), error on subfile key remove", FMS_CPF_PrivateException::INTERNALERROR);
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "close(), error on subfile key remove");
			}
		}
	}
	TRACE(fms_cpf_FileTable, "%s", "Leaving close()");
	FileReference::remove(ref);
}

//------------------------------------------------------------------------------
//      Remove file
//------------------------------------------------------------------------------
void FileTable::remove(FileReference ref) throw(FMS_CPF_PrivateException)
{ 
	TRACE(fms_cpf_FileTable, "%s", "Entering in remove()");

	// Allow only one thread at the time to enter this code.
	// guard is destroyed when out of scope, automatically releasing the lock.
	ACE_Guard<ACE_Thread_Mutex> guard(m_FileTableLock);

	FileDescriptor* fd;
	// search file descriptor from file reference
	if( (fd = FileReference::find(ref)) == 0 )
	{
		TRACE(fms_cpf_FileTable, "%s", "remove(), Invalid file reference");
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INVALIDREF);
	}

	// check access type on the file
	if(fd->access_ != FMS_CPF_Types::DELETE_)
	{
		TRACE(fms_cpf_FileTable, "%s", "remove(), Error access to file not exclusive");
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::ACCESSERROR);
	}
	
	FileAccess* faccessp = fd->faccessp_;

	// Check the number of users on the file
	if(faccessp->users_.ucount != 1)
	{
		TRACE(fms_cpf_FileTable, "%s", "remove(), Error access to file other user on it");
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::ACCESSERROR);
	}
	
	File* filep = faccessp->filep_;

	if(faccessp == filep->faccessp_)
	{
		TRACE(fms_cpf_FileTable, "%s", "remove(), physical main file removing");
		// Main file
		// Remove physical file
		FileMgr filemgr(filep->fileid_, filep->volume_, filep->attribute_, m_CpName.c_str());
		filemgr.remove();
		
		TRACE(fms_cpf_FileTable, "%s", "remove(), main file removing from file table");
		// Remove file from the file table
		FileTableList::iterator listPtr;
		size_t oldListSize = m_FileTable.size();

		// search file entry in file table
		for(listPtr = m_FileTable.begin(); listPtr != m_FileTable.end(); ++listPtr)
		{
			File *next = *listPtr;
			if (filep->fileid_ == next->fileid_)
			{
				listPtr = m_FileTable.erase(listPtr);
				break;
			}
		}
		
		// checks if removed
		if( oldListSize == m_FileTable.size() )
		{
			TRACE(fms_cpf_FileTable, "%s", "remove(), Internal error on remove file from the File table");

			CPF_Log.Write("remove(), error  on remove file from the File table", LOG_LEVEL_WARN);
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR, "error on remove file from the File table");
		}

		delete filep;
		TRACE(fms_cpf_FileTable, "%s", "remove(), file access removing");
		// Remove entry from file access and delete file descriptor
		faccessp->remove(fd);
		if(faccessp->fdList_.empty())
		{
			// delete file descriptor
			delete faccessp;
			faccessp = NULL;
		}
		else
		{
			CPF_Log.Write("remove() main file, error file descriptor list not empty", LOG_LEVEL_WARN);
			TRACE(fms_cpf_FileTable, "%s", "remove() main file, Internal error file descriptor list not empty");
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR);
		}
	}
	else
	{
		TRACE(fms_cpf_FileTable, "%s", "remove(), physical subfile removing");
		// Subfile
		// Remove physical file
		FileMgr filemgr(faccessp->fileid_, filep->volume_, filep->attribute_, m_CpName.c_str());
		filemgr.remove();
		
		// Logical remove of subfile from its composite file
		filep->removeSubFile(faccessp->fileid_.subfileAndGeneration());

		// Remove entry from subfile access list and delete file descriptor
		FileAccess key(faccessp->fileid_);

		TRACE(fms_cpf_FileTable, "%s", "remove(), subfile access entry removing");
		SubFileList::iterator listPtr;
		FileAccess* next;
		bool subFileFound = false;

		for(listPtr = filep->subfilelist_.begin(); listPtr != filep->subfilelist_.end(); ++listPtr)
		{
			next = *listPtr;
			// compare FileAccess by fileId
			if(*next == key)
			{
				// subfile fileAccess found
				filep->subfilelist_.erase(listPtr);
				subFileFound = true;
				break;
			}
		}
		// check if found
		if( !subFileFound )
		{
			//Error subfile fileAccess not found in the list
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "FileTable::remove(), error file descriptor of subfile:<%s> not found in fdList", faccessp->fileid_.data());
			CPF_Log.Write(errMsg, LOG_LEVEL_WARN);
			TRACE(fms_cpf_FileTable, "%s", errMsg);
		}

		TRACE(fms_cpf_FileTable, "%s", "remove(), subfile access removing");
		faccessp->remove(fd);

		if( faccessp->fdList_.empty() )
		{
			// delete file descriptor
			delete faccessp;
			faccessp = NULL;
		}
		else
		{
			CPF_Log.Write("remove() subfile, error file descriptor list not empty", LOG_LEVEL_WARN);
			TRACE(fms_cpf_FileTable, "%s", "remove() subfile, Internal error file descriptor list not empty");
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR);
		}
	}

	FileReference::remove(ref);
	TRACE(fms_cpf_FileTable, "%s", "Leaving remove()");
}


void FileTable::removeBlockSender(FileReference reference)
throw (FMS_CPF_PrivateException)
{ 
	TRACE(fms_cpf_FileTable, "%s", "Entering in removeBlockSender()");

	// Allow only one thread at the time to enter this code.
	// guard is destroyed when out of scope, automatically releasing the lock.
	ACE_Guard<ACE_Thread_Mutex> guard(m_FileTableLock);

	FileDescriptor* fd;
	if ((fd = FileReference::find(reference)) == 0)
	{
		TRACE(fms_cpf_FileTable, "%s", "removeBlockSender(), Invalid file reference");
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::INVALIDREF);
	}
	
	if (fd->access_ != FMS_CPF_Types::DELETE_)
	{
		TRACE(fms_cpf_FileTable, "%s", "removeBlockSender(), Error access to file not exclusive");
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::ACCESSERROR);
	}
	
	FileAccess* faccessp = fd->faccessp_;
	if (faccessp->users_.ucount != 1)
	{
		TRACE(fms_cpf_FileTable, "%s", "removeBlockSender(), Error access to file other user on it");
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::ACCESSERROR);
	}
	
	File* filep = faccessp->filep_;

	if (faccessp == filep->faccessp_)
	{

		if (filep->gohBlockHandlerExist == true)
		{
/*
			int rf_res = GohdatablockMgr::instance()->removeFile(filep->fileid_.file(),filep->cpname_.c_str());
			
			if (rf_res == ALLFILEREMOVED)
			{
				HANDLE fd;
				bool result =  GohdatablockMgr::instance()->getHandle(filep->fileid_.file(),fd);
#ifdef LOSTBUFFERS1
				cout<<"FileTable::remove GohdatablockMgr::instance()->getHandle(fd) = "<<result<<endl;
#endif
				GohdatablockMgr::instance()->terminateSendBlockThread(filep->fileid_.file());
				WaitForSingleObject(fd,10000);
#ifdef LOSTBUFFERS1
				cout<<"FileTable::remove gohblockhandler object"<<endl;
#endif
				if(ACS_TRA_ON(fms_cpf_tablefile))
				{
					char trace[200];
					sprintf(trace, "%s\n", "FileTable::remove in !filep->gohBlockHandler_->threadIsTerminated()");
					ACS_TRA_event(&fms_cpf_tablefile,trace);
				}
#ifdef LOSTBUFFERS1
				cout<<"FileTable::remove gohblockhandler object before delete"<<endl;
#endif
				if(ACS_TRA_ON(fms_cpf_tablefile))
				{
					char trace[200];
					sprintf(trace, "%s\n", "FileTable::remove before delete GOHblockHandler object");
					ACS_TRA_event(&fms_cpf_tablefile,trace);
				}
				try
				{
					GohdatablockMgr::instance()->removeSendBlock(filep->fileid_.file());
					//delete filep->gohBlockHandler_;        
				}
				catch (...)
				{
					//cout<<"try test"<<endl;
					FMS_CPF_PrivateException ex(FMS_CPF_PrivateException::INTERNALERROR);
					ex << "Exception when deleting GOHblockHandler object";
					FMS_CPF_EventHandler::instance()->event(ex);
				}
#ifdef LOSTBUFFERS1
			cout<<"FileTable::remove gohblockhandler object after delete"<<endl;
#endif
			if(ACS_TRA_ON(fms_cpf_tablefile))
			{
				char trace[200];
				sprintf(trace, "%s\n", "FileTable::remove after delete GOHblockHandler object");
				ACS_TRA_event(&fms_cpf_tablefile,trace);
			}
#ifdef LOSTBUFFERS
			cout<<"FileTable::remove gohblockhandler object after remove"<<endl;
#endif
			//filep->gohBlockHandler_ = NULL;
			} //INGO3 Drop 1 --	

			//delete filep->gohBlockHandler_;
            // TR_HJ23649
		    //delete .log file
		    Sleep(2000);
		*/
		}	

	}
	TRACE(fms_cpf_FileTable, "%s", "Leaving removeBlockSender()");
}


//------------------------------------------------------------------------------
//      Open file
//------------------------------------------------------------------------------
FileReference FileTable::open(const FMS_CPF_FileId& fileid, FMS_CPF_Types::accessType access, bool inf)
  throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in open()");
	
	// Allow only one thread at the time to enter this code.
	// guard is destroyed when out of scope, automatically releasing the lock.
	ACE_Guard<ACE_Thread_Mutex> guard(m_FileTableLock);

	// File identity valid ?
	if(!fileid.isValid ())
	{
		TRACE(fms_cpf_FileTable, "%s", "open(), invalid file identity");
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INVALIDFILE);
	}

	// Check if main file exists
	File* filep= find(fileid.file());
	if( NULL == filep)
	{
		TRACE(fms_cpf_FileTable, "%s", "open(), Main file not exists");
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILENOTFOUND, fileid.file().c_str());
	}
	
	FileDescriptor* fd;
	SubFileList::iterator listPtr;
	if(fileid.subfileAndGeneration().empty())
	{
		TRACE(fms_cpf_FileTable, "%s", "open(), main file");
		// Main file
		// Check access against all subfiles first
		for(listPtr = filep->subfilelist_.begin(); listPtr != filep->subfilelist_.end(); listPtr++)
		{
			FileAccess *next = *listPtr;
			if(next->checkAccess (access) == false)
			{
				TRACE(fms_cpf_FileTable, "%s", "open(), Error access to file");
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::ACCESSERROR);
			}
		}
		
		// Insert entry in file access and create file descriptor
		if (!filep->faccessp_)
		{
			// File not referenced
			filep->faccessp_ = new (std::nothrow) FileAccess(fileid, filep);

			if(NULL == filep->faccessp_)
			{
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR);
			}
		}

		fd = filep->faccessp_->create(access, inf);
		
		// Check the file type: infinite or composite
		if(inf == true)
		{
			TRACE(fms_cpf_FileTable, "%s", "open(), infinite file");
			// Infinite file access
			if(filep->faccessp_->icount_ > 0)
			{
				FMS_CPF_Types::fileAttributes fileattr = filep->attribute_;
				if(fileattr.infinite.maxtime > 0)
				{
					if(filep->attribute_.getChangeAfterClose() == false)
					{
						filep->attribute_.setChangeAfterClose(true);
						filep->attribute_.setChangeTime();
						
						// Change attribute file
						FileMgr filemgr(filep->fileid_, filep->volume_, filep->attribute_, m_CpName.c_str());
						filemgr.setAttribute ();
					}
				}
			}
		}
	}
	else
	{
		TRACE(fms_cpf_FileTable, "%s", "open(),  subfile");
		// Subfile
		// Check if main file is composite
		if(filep->attribute_.composite() == false)
		{
			TRACE(fms_cpf_FileTable, "%s", "open(),  The main file is not composite");
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::NOTCOMPOSITE, fileid.file().c_str());
		}
		
		// Check if physical file exists
		bool subFileExist;

		if( filep->getFileType() == FMS_CPF_Types::ft_REGULAR )
		{
			// composite subfile
			TRACE(fms_cpf_FileTable, "%s", "open(), composite subfile check");
			// Check if defined into IMM
			subFileExist = filep->subFileExist(fileid.subfileAndGeneration());
		}
		else
		{
			// infinite subfile make only a physical check
			TRACE(fms_cpf_FileTable, "%s", "exists(), infinite subfile check");
			FileMgr filemgr(fileid, filep->volume_, filep->attribute_, m_CpName.c_str());
			subFileExist = filemgr.exists();
		}

		if(!subFileExist)
		{
			TRACE(fms_cpf_FileTable, "%s", "open(), subfile not found");
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILENOTFOUND, fileid.data() );
		}
		
		// Check access against main file first
		if(NULL != filep->faccessp_)
		{
			if(filep->faccessp_->checkAccess(access) == false )
			{
				TRACE(fms_cpf_FileTable, "%s", "open(),  Error to access the file");
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::ACCESSERROR);
			}
		}
		
		// Insert entry in subfile access list and create file descriptor
		FileAccess akey(fileid);
		FileAccess* faccessp = NULL;
		FileAccess* next;

		TRACE(fms_cpf_FileTable, "%s", "open(),  Insert entry in subfile access list and create file descriptor");
		for(listPtr = filep->subfilelist_.begin(); listPtr != filep->subfilelist_.end(); ++listPtr)
		{
			next = *listPtr;
			if(*next == akey)
			{
				//Subfile is found
				faccessp = next;
				break;
			}
		}
		
		if(NULL == faccessp )
		{
			// File not referenced
			TRACE(fms_cpf_FileTable, "%s", "open(), File not referenced");

			faccessp = new (std::nothrow) FileAccess(fileid, filep);

			if(NULL == faccessp )
			{
				throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INTERNALERROR);
			}

			filep->subfilelist_.push_back(faccessp);
		}
		// subfile access
		fd = faccessp->create(access, false);
	}

	TRACE(fms_cpf_FileTable, "%s", "Leaving open()");

	return FileReference::insert(fd);
}

int FileTable::getListFileId(const FMS_CPF_FileId& fileid, std::list<FMS_CPF_FileId>& fileList)
throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in getListFileId()");

	if( fileid.isNull() )
	{
		// Create a list of all main files
		FileTable::FileTableList::const_iterator listPtr;

		for(listPtr = m_FileTable.begin(); listPtr != m_FileTable.end(); ++listPtr)
		{
			fileList.push_back( (*listPtr)->fileid_ );
		}
	}
	else
	{
		// Create a list of all subfiles for a specific composite main file

		// Check if the file is valid
		if(!fileid.isValid())
		{
			TRACE(fms_cpf_FileTable, "%s", "getListFileId(), Invalid file");
			throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::INVALIDFILE, fileid.file());
		}

		// Check that the file is not a sub file
		if( !fileid.subfileAndGeneration().empty() )
		{
			TRACE(fms_cpf_FileTable, "%s", "getListFileId(), the file is not simple or main composite file");
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::NOTBASEFILE);
		}

		// get the File object
		File* filep = find(fileid);

		if( NULL == filep )
		{
			TRACE(fms_cpf_FileTable, "%s", "getListFileId(), File is not found");
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILENOTFOUND,	fileid.file() );
		}

		if( !filep->attribute_.composite() )
		{
			TRACE(fms_cpf_FileTable, "%s", "getListFileId(), File is not composite");
			throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::NOTCOMPOSITE,	fileid.file() );
		}

		// get all the subfile of the composite file
		if( filep->getFileType() == FMS_CPF_Types::ft_INFINITE )
		{
			FMS_CPF_FileId nextfileid;

			// get all subfile of infinite file
			FileMgr filemgr(filep->fileid_, filep->volume_, filep->attribute_, m_CpName.c_str());
			filemgr.opendir();

			while(! ((nextfileid = filemgr.readdir()).isNull()) )
			{
				fileList.push_back(nextfileid);
			}

			filemgr.closedir();
		}
		else
		{
			// get all the subfile of the composite file
			filep->getAllSubFile(fileList);
		}
	}

	TRACE(fms_cpf_FileTable, "%s", "Leaving getListFileId()");
	return fileList.size();
}

/*============================================================================
	ROUTINE: getFileDN
 ============================================================================ */
bool FileTable::getFileDN(const std::string& fileName, std::string& fileDN)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in getFileDN()");
	bool result = false;
	// Allow only one thread at the time to enter this code.
	// guard is destroyed when out of scope, automatically releasing the lock.
	ACE_Guard<ACE_Thread_Mutex> guard(m_FileTableLock);

	FMS_CPF_FileId fileId(fileName);

	// File identity valid
	if(fileId.isValid())
	{
		// Check if main file exists
		File* filePtr= find(fileId.file());
		if( NULL != filePtr)
		{
			fileDN = filePtr->getFileDN();
			result = true;
		}
	}

	TRACE(fms_cpf_FileTable, "Leaving getFileDN(), file:<%s>, DN:<%s>, result:<%s>", fileName.c_str(),
																		fileDN.c_str(),
																		(result ? "OK" : "NOT OK") );
	return result;
}

/*============================================================================
	ROUTINE: getFileDN
 ============================================================================ */
void FileTable::setFileDN(const std::string& fileName, const std::string& fileDN)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in setFileDN()");
	// Allow only one thread at the time to enter this code.
	// guard is destroyed when out of scope, automatically releasing the lock.
	ACE_Guard<ACE_Thread_Mutex> guard(m_FileTableLock);

	FMS_CPF_FileId fileId(fileName);

	// File identity valid
	if(fileId.isValid())
	{
		// Check if main file exists
		File* filePtr= find(fileId.file());
		if( NULL != filePtr)
		{
			filePtr->setFileDN(fileDN);
		}
	}

	TRACE(fms_cpf_FileTable, "Leaving setFileDN(), file:<%s>, DN:<%s>", fileName.c_str(), fileDN.c_str() );
}
/*============================================================================
	ROUTINE: getCpFileType
 ============================================================================ */
FMS_CPF_Types::fileType FileTable::getCpFileType(const std::string& fileName)
	throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in getFileType()");

	// Allow only one thread at the time to enter this code.
	// guard is destroyed when out of scope, automatically releasing the lock.
	ACE_Guard<ACE_Thread_Mutex> guard(m_FileTableLock);

	FMS_CPF_Types::fileType fileType;
	FMS_CPF_FileId fileId(fileName);

	// File identity valid
	if(fileId.isValid())
	{
		// Check if main file exists
		File* filePtr= find(fileId.file());

		if( NULL != filePtr)
		{
			fileType = filePtr->getFileType();
		}
		else
		{
			TRACE(fms_cpf_FileTable, "getFileType(), file name:<%s> not found", fileName.c_str());
			throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::FILENOTFOUND, fileName);
		}
	}
	else
	{
		TRACE(fms_cpf_FileTable, "getFileType(), invalid file name:<%s>", fileName.c_str());
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::INVALIDFILE);
	}

	TRACE(fms_cpf_FileTable, "%s", "Leaving getFileType()");
	return fileType;
}

/*============================================================================
	ROUTINE: getListOfInfiniteFile
 ============================================================================ */
void FileTable::getListOfInfiniteFile(std::vector<infiniteFileInfo>& fileList)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in getListOfInfiniteFile()");
	// Create a list of all main files
	FileTable::FileTableList::const_iterator listPtr;

	// Allow only one thread at the time to enter this code.
	// guard is destroyed when out of scope, automatically releasing the lock.
	ACE_Guard<ACE_Thread_Mutex> guard(m_FileTableLock);

	for(listPtr = m_FileTable.begin(); listPtr != m_FileTable.end(); ++listPtr)
	{
		if( (*listPtr)->getFileType() == FMS_CPF_Types::ft_INFINITE)
		{
			fileList.push_back(std::make_pair((*listPtr)->getFileName(), (*listPtr)->getFileDN() ));
		}
	}

	TRACE(fms_cpf_FileTable, "%s", "Leaving getListOfInfiniteFile()");
}

/*============================================================================
	ROUTINE: getListOfInfiniteFile
 ============================================================================ */
void FileTable::getListOfInfiniteFile(std::vector<std::string>& fileList)
{
	TRACE(fms_cpf_FileTable, "Entering in %s", __func__);
	// Create a list of all main files
	FileTable::FileTableList::const_iterator listPtr;

	// Allow only one thread at the time to enter this code.
	// guard is destroyed when out of scope, automatically releasing the lock.
	ACE_Guard<ACE_Thread_Mutex> guard(m_FileTableLock);

	for(listPtr = m_FileTable.begin(); listPtr != m_FileTable.end(); ++listPtr)
	{
		if( (*listPtr)->getFileType() == FMS_CPF_Types::ft_INFINITE)
		{
			fileList.push_back( std::string((*listPtr)->getFileName()) );
		}
	}

	TRACE(fms_cpf_FileTable, "Leaving %s", __func__);
}

/*============================================================================
	ROUTINE: getListOfInfiniteSubFile
 ============================================================================ */
void FileTable::getListOfInfiniteSubFile(const std::string& infiniteFileName, std::set<std::string>& subFileList)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in getListOfInfiniteSubFile()");
	// Allow only one thread at the time to enter this code.
	// guard is destroyed when out of scope, automatically releasing the lock.
	ACE_Guard<ACE_Thread_Mutex> guard(m_FileTableLock);

	FMS_CPF_FileId cpFileId(infiniteFileName);
	// get the File object
	File* infiniteFile = find(cpFileId);
	
	m_CpRoot = (ParameterHndl::instance()->getCPFroot(m_CpName.c_str()) + DirDelim); //HW56759

	if( NULL != infiniteFile )
	{
		std::string filePath = m_CpRoot + infiniteFile->volume_ + DirDelim + infiniteFileName;
		TRACE(fms_cpf_FileTable,"getListOfInfiniteSubFile(), open folder:<%s>", filePath.c_str() );
		// Open composite file directory
		DIR* fileFolder = ACE_OS::opendir(filePath.c_str() );

		if(NULL != fileFolder)
		{
			struct dirent* subFileEntry;
			// add all subfiles
			while( (subFileEntry = ACE_OS::readdir(fileFolder)) != NULL )
			{
				// Get subfile name
				std::string subFileName(subFileEntry->d_name);

				// check name
				if(checkSubFileName(subFileName))
				{
					subFileList.insert(subFileName);
					// Todo only for test scope to remove i!
					TRACE(fms_cpf_FileTable, "getListOfInfiniteSubFile(), found ISF:<%s>", subFileName.c_str() );
				}
			}
			// close folder
			ACE_OS::closedir(fileFolder);
		}
		else
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "getListOfInfiniteSubFile(), error:<%d> on opendir:<%s> call, Cp:<%s>", errno, filePath.c_str(), m_CpName.c_str());
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_FileTable, "%s", errMsg);
		}
	}
	TRACE(fms_cpf_FileTable, "%s", "Leaving getListOfInfiniteSubFile()");
}

/*============================================================================
	ROUTINE: getLastFieldValue
 ============================================================================ */
void FileTable::getLastFieldValue(const std::string& fileDN, std::string& value)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in getLastFieldValue()");

	value.clear();
	// Get the file name from DN
	// Split the field in RDN and Value
	size_t equalPos = fileDN.find_first_of(parseSymbol::equal);
	size_t commaPos = fileDN.find_first_of(parseSymbol::comma);

	// Check if some error happens
	if( (std::string::npos != equalPos) )
	{
		// check for a single field case
		if( std::string::npos == commaPos )
			value = fileDN.substr(equalPos + 1);
		else
			value = fileDN.substr(equalPos + 1, (commaPos - equalPos - 1) );

		// make the value in upper case
		ACS_APGCC::toUpper(value);
	}
	TRACE(fms_cpf_FileTable, "Leaving getLastFieldValue(), value:<%s>", value.c_str());
}

/*============================================================================
	ROUTINE: isFileLockedForDelete
 ============================================================================ */
bool FileTable::isFileLockedForDelete(const std::string& fileName)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in isFileLockedForDelete()");
	bool lockedForDelete = false;
	// Allow only one thread at the time to enter this code.
	// guard is destroyed when out of scope, automatically releasing the lock.
	ACE_Guard<ACE_Thread_Mutex> guard(m_FileTableLock);

	FMS_CPF_FileId cpFileId(fileName);
	// Check if main file exists
	File* filep= find(cpFileId.file());

	// check if the file exist and is open
	if( (NULL != filep) && (filep->faccessp_ != NULL) )
	{
		// check if it is open for delete
		lockedForDelete = filep->faccessp_->isLockedforDelete();
	}
	TRACE(fms_cpf_FileTable, "%s", "Leaving isFileLockedForDelete()");
	return lockedForDelete;
}

/*============================================================================
	ROUTINE: deleteScpVolumes

	Delete default folders created in Single CP System configuration
 ============================================================================ */
void FileTable::deleteScpVolumes(OmHandler* objManager)
{
	//-----------------------------
	//Remove volumes from IMM

	// Assemble the RDN values
	char tmpVolumeDN[256] = {0}, relVolumeDN[256] = {0};
	ACE_OS::snprintf(tmpVolumeDN, sizeof(tmpVolumeDN) - 1, "%s=%s,%s", cpf_imm::VolumeKey, defaultVolume::TEMPVOLUME.c_str(), cpf_imm::parentRoot);
	ACE_OS::snprintf(relVolumeDN, sizeof(relVolumeDN) - 1, "%s=%s,%s", cpf_imm::VolumeKey, defaultVolume::RELVOLUMSW.c_str(), cpf_imm::parentRoot);


	//Delete TEMPVOLUME from IMM
	ACS_CC_ReturnType immResult;
	immResult = objManager->deleteObject(tmpVolumeDN);
	if (ACS_CC_SUCCESS != immResult)
	{
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, sizeof(errMsg) - 1, "[%s@%d] Cannot delete object: %s. Error: %d, %s",
				__FUNCTION__, __LINE__, tmpVolumeDN, objManager->getInternalLastError(), objManager->getInternalLastErrorText());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
	}

	//Delete RELVOLUMSW from IMM
	immResult = objManager->deleteObject(relVolumeDN);
	if (ACS_CC_SUCCESS != immResult)
	{
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, sizeof(errMsg) - 1, "[%s@%d] Cannot delete object: %s. Error: %d, %s",
				__FUNCTION__, __LINE__, relVolumeDN, objManager->getInternalLastError(), objManager->getInternalLastErrorText());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
	}

	//-----------------------------
	//Remove volumes from Datadisk
	ACE_stat statbuf;
	//Delete TEMPVOLUME from Datadisk
	char tempVolumePath[256] = {0};
	ACE_OS::snprintf(tempVolumePath, sizeof(tempVolumePath) - 1,"%s/cpf/%s",
			ParameterHndl::instance()->getDataDiskRoot(), defaultVolume::TEMPVOLUME.c_str());

	if( ACE_OS::stat(tempVolumePath, &statbuf) == SUCCESS )
	{
		try
		{
			(void)boost::filesystem::remove_all(boost::filesystem::path(tempVolumePath));
		}
		catch(boost::filesystem::filesystem_error &ex)
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, sizeof(errMsg) - 1, "[%s@%d] Call 'boost::filesystem::remove_all' failed: removing the entry '%s'. Exception: '%s'",
					__FUNCTION__, __LINE__, tempVolumePath, ex.what());
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		}
	}

	//Delete RELVOLUMSW from Datadisk
	char relVolumePath[256] = {0};
	ACE_OS::snprintf(relVolumePath, sizeof(relVolumePath) - 1,"%s/cpf/%s",
			ParameterHndl::instance()->getDataDiskRoot(), defaultVolume::RELVOLUMSW.c_str());

	if( ACE_OS::stat(relVolumePath, &statbuf) == SUCCESS )
	{
		try
		{
			boost::filesystem::remove_all(boost::filesystem::path(relVolumePath));
		}
		catch(boost::filesystem::filesystem_error &ex)
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, sizeof(errMsg) - 1, "[%s@%d] Call 'boost::filesystem::remove_all' failed: removing the entry '%s'. Exception: '%s'",
					__FUNCTION__, __LINE__, relVolumePath, ex.what());
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		}
	}
}

/*============================================================================
	ROUTINE: createIMMObject
 ============================================================================ */
bool FileTable::createIMMObject(OmHandler* objManager, const char* className, const char* parentName, std::vector<ACS_CC_ValuesDefinitionType> attrValuesList, const char* objDN )
{
	TRACE(fms_cpf_FileTable, "%s", "Entering createIMMObject()");
	bool result = false;

	//Retry mechanism - HR81721 - begin
	ACS_CC_ReturnType getResult;
	int retryCount = 0;
	do
	{
		getResult = objManager->createObject(className, parentName, attrValuesList);

		if( ACS_CC_SUCCESS == getResult)
		{
			char infoMsg[512] = {0};
			ACE_OS::snprintf(infoMsg, 511, "%s(), object:<%s> created", __func__, objDN);
			CPF_Log.Write(infoMsg, LOG_LEVEL_INFO);
			TRACE(fms_cpf_FileTable, "%s", infoMsg);

			result = true;
			break;
		}
		else
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "FileTable::createIMMObject(), error:<%d> on create object:<%s>, attempts:<%d>", objManager->getInternalLastError(), objDN, retryCount);
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_FileTable, "%s", errMsg);

			if(IMM_ErrorCode::TRYAGAIN != objManager->getInternalLastError())
			{
				break;
			}
			else
			{
				++retryCount;
				__useconds_t timeWaitMicroSec = static_cast<unsigned int>(exp2(retryCount)) * stdValue::BASE_MICROSEC_SLEEP;

				char errMsg[512] = {0};
				ACE_OS::snprintf(errMsg, 511, "FileTable::createIMMObject(), wait <%d> usec before retry", timeWaitMicroSec);
				CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);

				usleep(timeWaitMicroSec);
			}
		}

	}while(stdValue::MAX_IMM_RETRY >= retryCount );
	//Retry mechanism - HR81721 - end

	TRACE(fms_cpf_FileTable, "Leaving createIMMObject(), result:<%s>", (result ? "TRUE":"FALSE"));
	return result;
}

/*============================================================================
	ROUTINE: deleteIMMObject
 ============================================================================ */
bool FileTable::deleteIMMObject(OmHandler* objManager, const char* objectName )
{
	TRACE(fms_cpf_FileTable, "%s", "Entering deleteIMMObject()");
	bool result = false;

	//Retry mechanism - HR81721 - begin
	ACS_CC_ReturnType getResult;
	int retryCount = 0U;
	do
	{
		// File not found on data disk, remove it from IMM
		getResult = objManager->deleteObject(objectName, ACS_APGCC_SUBTREE);

		if( ACS_CC_SUCCESS == getResult)
		{
			result = true;
			char infoMsg[512] = {0};
			ACE_OS::snprintf(infoMsg, 511, "%s(), object:<%s> deleted", __func__, objectName);
			CPF_Log.Write(infoMsg, LOG_LEVEL_INFO);
			TRACE(fms_cpf_FileTable, "%s", infoMsg);
			break;
		}
		else
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "FileTable::deleteIMMObject(), error:<%d> on delete object:<%s>, attempts:<%d>", objManager->getInternalLastError(), objectName, retryCount);
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_FileTable, "%s", errMsg);

			if(IMM_ErrorCode::TRYAGAIN != objManager->getInternalLastError())
			{
				break;
			}
			else
			{
				++retryCount;
				__useconds_t timeWaitMicroSec = static_cast<unsigned int>(exp2(retryCount)) * stdValue::BASE_MICROSEC_SLEEP;

				ACE_OS::snprintf(errMsg, 511, "FileTable::deleteIMMObject(), wait <%d> usec before retry, attempts:<%d>", timeWaitMicroSec, retryCount);
				CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);

				usleep(timeWaitMicroSec);
			}
		}

	}while(stdValue::MAX_IMM_RETRY >= retryCount);
	//Retry mechanism - HR81721 - end

	TRACE(fms_cpf_FileTable, "Leaving deleteIMMObject(), result:<%s>", (result ? "TRUE":"FALSE"));
	return result;
}


//HY46076 start

 /*============================================================================
	ROUTINE: getCompositeFileAttributes
 ============================================================================ */
bool FileTable::getCompositeFileAttributes(const std::string& fileDN, OmHandler* objManager, FMS_CPF_Types::fileAttributes& attributeValue)
{
	TRACE(fms_cpf_FileTable, "%s", "Entering in getCompositeFileAttributes()");
	bool result = false;
    	attributeValue.ftype = FMS_CPF_Types::ft_REGULAR;
	attributeValue.regular.rlength = 1024;
	attributeValue.regular.deleteFileTimer = -1;
	
	//List of attributes to get
	std::vector<ACS_APGCC_ImmAttribute*> attributeList;
	
	// to get the record length of a regular file
	ACS_APGCC_ImmAttribute recordLengthAttribute;
	recordLengthAttribute.attrName = cpf_imm::recordLengthAttribute;
	attributeList.push_back(&recordLengthAttribute);
	
	// to get the deleteFileTimer of a composite file
	ACS_APGCC_ImmAttribute deleteFileTimerAttribute;
	deleteFileTimerAttribute.attrName = cpf_imm::deleteFileTimerAttribute;
	attributeList.push_back(&deleteFileTimerAttribute);
	ACS_CC_ReturnType getResult = objManager->getAttribute(fileDN.c_str(), attributeList );
	
	if( ACS_CC_FAILURE != getResult)
	{
		result = true;
        	if( 0 != recordLengthAttribute.attrValuesNum )
		{
			attributeValue.regular.rlength = (*reinterpret_cast<unsigned int*>(recordLengthAttribute.attrValues[0]));
		}
			
		// Get current IMM attribute value
		if(0 != deleteFileTimerAttribute.attrValuesNum )
		{
			attributeValue.regular.deleteFileTimer = (*reinterpret_cast<long int*>(deleteFileTimerAttribute.attrValues[0]));
		}

	}
	else
	{
		//Error Handling
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "getCompositeFileAttributes(DN:<%s>), error:<%d> on OmHandler ", fileDN.c_str(),  objManager->getInternalLastError());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_FileTable, "%s", errMsg);
	}
    	TRACE(fms_cpf_FileTable, "%s", "Leaving getCompositeFileAttributes()");
	return result;
}
//HY46076 end
