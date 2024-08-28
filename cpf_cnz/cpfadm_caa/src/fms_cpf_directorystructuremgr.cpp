/*
 * * @file fms_cpf_directorystructuremgr.cpp
 *	@brief
 *	Class method implementation for DirectoryStructureMgr.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_directorystructuremgr.h module
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

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_common.h"
#include "fms_cpf_filetable.h"
#include "fms_cpf_filehandler.h"
#include "fms_cpf_fileaccess.h"
#include "fms_cpf_filereference.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_filedescriptor.h"

#include "fms_cpf_configreader.h"

#include "fms_cpf_fileid.h"
#include "fms_cpf_attribute.h"
#include "fms_cpf_configreader.h"

#include "ACS_APGCC_CommonLib.h"
#include "ACS_APGCC_RuntimeOwner.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"

#include <boost/filesystem.hpp>

#include <fstream>


extern ACS_TRA_Logging CPF_Log;

namespace defaultCpName {
		const std::string CLUSTER("CLUSTER");
}

namespace cpf_isf
{
	class runtimeObjectHandler : public ACS_APGCC_RuntimeOwner
	{
	 public:

		runtimeObjectHandler()
		:ACS_APGCC_RuntimeOwner()
		{
			init("CPF_RTO_InfiniteSubFile");
		};

		virtual ~runtimeObjectHandler()
		{
			finalize();
		}

		ACS_CC_ReturnType updateCallback(const char* p_objName, const char* p_attrName)
		{
			UNUSED(p_objName);
			UNUSED(p_attrName);
			return ACS_CC_SUCCESS;
		}
	};
}
/*============================================================================
	ROUTINE: FMS_CPF_DirectoryStructureMgr
 ============================================================================ */
FMS_CPF_DirectoryStructureMgr::FMS_CPF_DirectoryStructureMgr()
: m_isMultiCP(false),
  m_isRestartAfterRestore(false),
  m_FileTableMap(),
  m_clearDataPath(),
  m_configReader(0)
{
	fms_cpf_DirMgr = new (std::nothrow) ACS_TRA_trace("FMS_CPF_DirectoryStructureMgr");

    TRACE(fms_cpf_DirMgr, "%s", "DirectoryStructureMgr(), created instance");
}

/*============================================================================
	ROUTINE: FMS_CPF_DirectoryStructureMgr
 ============================================================================ */
FMS_CPF_DirectoryStructureMgr::~FMS_CPF_DirectoryStructureMgr(void)
{
	clearFileTableMap();

	if(NULL != fms_cpf_DirMgr)
		delete fms_cpf_DirMgr;
}

/*============================================================================
	ROUTINE: loadCpFileInfo
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::initializeCpFileSystem(FMS_CPF_ConfigReader *configReader)
{
	bool result = false;
	TRACE(fms_cpf_DirMgr, "%s", "Entering in initializeCpFileSystem");
	m_configReader = configReader;
	if(NULL != m_configReader)
	{
		m_isMultiCP = m_configReader->IsBladeCluster();

		checkRaidDisk();

		// clear old values
		clearFileTableMap();

		m_isRestartAfterRestore = isRestartAfterRestore();
		result = loadCpFileInfo(m_isRestartAfterRestore);

		//HS25665 Begin
		//populate CP file table Map
		if( m_isMultiCP )
		{
			TRACE(fms_cpf_DirMgr, "%s", "initializeCpFileSystem(), Multi CP System");
			std::list<short> cpIdList = configReader->getCP_IdList();
			std::list<short>::iterator idx;
			for( idx = cpIdList.begin(); idx != cpIdList.end(); ++idx)
			{
				// get the CPname from the CPid
				std::string tmpCpName(configReader->cs_getDefaultCPName((*idx)));
				if (!cpExists(tmpCpName))
				{
					refreshCpFileTable(tmpCpName);
				}
			}
		}//HS25665 End
	}

	TRACE(fms_cpf_DirMgr, "Leaving initializeCpFileSystem(), result:<%s>", (result ? "OK" : "NOT OK"));
	return result;

}

/*============================================================================
	ROUTINE: loadCpFileInfo
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::loadCpFileInfo(bool restore)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in loadCpFileInfo");
	bool result = false;

	OmHandler objManager;

	// Init OM resource
	if(objManager.Init() == ACS_CC_SUCCESS)
	{
		//In case of MCP configuration, check for previously created IMM objects
		if (m_isMultiCP)
		{
			FileTable::deleteScpVolumes(&objManager);
		}

		// Get List of all defined volumes
		std::vector<std::string> volumeDNList;

		if(objManager.getChildren(cpf_imm::parentRoot, ACS_APGCC_SUBLEVEL, &volumeDNList) == ACS_CC_SUCCESS )
		{
			TRACE(fms_cpf_DirMgr, "loadCpFileInfo(), found <%zd> volumes", volumeDNList.size());

			if(m_isMultiCP)
			{
				std::map<std::string, std::vector<std::string> > mapCpVolumes;

				filterVolume(volumeDNList, mapCpVolumes);

				TRACE(fms_cpf_DirMgr, "%s", "loadCpFileInfo(), Load Cp Files for Blade Cluster");

				std::map<std::string, std::vector<std::string> >::const_iterator cpElement;

				for(cpElement = mapCpVolumes.begin(); cpElement != mapCpVolumes.end(); ++cpElement)
				{
					result = addCpFileTable(cpElement->first, cpElement->second, restore);

					// if fail exit from loop
					if(!result)
						break;

					TRACE(fms_cpf_DirMgr, "loadCpFileInfo(), FileTable created for CP:<%s>", cpElement->first.c_str());
				}

				//EANFORM - HR79048
				if (!cpExists(defaultCpName::CLUSTER))
				{
					//add to list
					refreshCpFileTable(defaultCpName::CLUSTER);
				}
			}
			else
			{
				TRACE(fms_cpf_DirMgr, "%s", "loadCpFileInfo(), Load Cp Files for Single CP System");

				// create the FileTable object and add to the internal map
				result = addCpFileTable(DEFAULT_CPNAME, volumeDNList, restore);
			}
		}
		else
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "DirectoryStructureMgr::loadCpFileInfo(), error:<%d> on OmHandler getChildren() method", objManager.getInternalLastError());
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_DirMgr, "%s", errMsg);
		}

		// clean up IMM from infinite subfiles
		removeInfiniteSubFilesFromIMM(&objManager);

		// Deallocate OM resource
		objManager.Finalize();
	}
	else
	{
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "DirectoryStructureMgr::loadCpFileInfo(), error:<%d> on OmHandler object init()", objManager.getInternalLastError());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_DirMgr, "%s", errMsg);
	}

	TRACE(fms_cpf_DirMgr, "Leaving loadCpFileInfo(), result:<%s>", (result ? "OK" : "NOT OK"));
	return result;
}

/*============================================================================
	ROUTINE: addCpFileTable
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::addCpFileTable(const std::string& cpName, const std::vector<std::string>& cpVolumeDN, bool restore)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in addCpFileTable");
	bool result = false;

	FileTable* fileTableObj = new (std::nothrow) FileTable(cpName, m_isMultiCP);

	// check allocation error and load CP file from IMM
	if((NULL != fileTableObj) )
	{
		if(restore && !dataDiskEmpty()) 
		{
			const short LOGMSG_LENGTH = 128U;
			char logMsg[LOGMSG_LENGTH] = {0};
			snprintf(logMsg, LOGMSG_LENGTH -1, "%s(), restore with Data Disk as base, Cp:<%s>", __func__, cpName.c_str());
			TRACE(fms_cpf_DirMgr, "%s", logMsg);
			CPF_Log.Write(logMsg, LOG_LEVEL_INFO);

			// Restore with Data Disk as base
			// Align IMM objects with Data disk
			result = fileTableObj->loadCpFileFromDataDisk(cpVolumeDN);

			// Check Infinite file state need for OSU procedure
			std::vector<std::string> infiniteFileList;
			std::vector<std::string>::const_iterator element;
			std::set<std::string> subfileList;

			// get list of infinite filesmake
			fileTableObj->getListOfInfiniteFile(infiniteFileList);

			// update the infinite file attribute
			for(element = infiniteFileList.begin(); element != infiniteFileList.end(); ++element)
			{
				snprintf(logMsg, LOGMSG_LENGTH -1, "%s(), updateInitialState of infinite file:<%s>, Cp:<%s>", __func__, (*element).c_str(), cpName.c_str());
				TRACE(fms_cpf_DirMgr, "%s", logMsg);
				CPF_Log.Write(logMsg, LOG_LEVEL_INFO);

				// get list of all subfiles of the infinite file
				fileTableObj->getListOfInfiniteSubFile( *element, subfileList);

				// check subfiles existence
				if(subfileList.end() != subfileList.begin())
				{
					// update the attribute info of Infinite file
					updateInfiteFileAttribute(fileTableObj, *element, *subfileList.begin(), *subfileList.rbegin() );
				}
				else
				{
					snprintf(logMsg, LOGMSG_LENGTH -1, "%s(), NO Subfiles found for infinite file:<%s>, Cp:<%s>", __func__, (*element).c_str(), cpName.c_str());
					TRACE(fms_cpf_DirMgr, "%s", logMsg);
					CPF_Log.Write(logMsg, LOG_LEVEL_ERROR);
				}

				// Clear the subfile list for the next file
				subfileList.clear();
			}
		}
		else
		{
			// Normal restart or Restore with IMM as base
			// Create all IMM objects on data disk if not present
			result = fileTableObj->loadCpFileFromIMM(cpVolumeDN, restore);
		}

		TRACE(fms_cpf_DirMgr, "%s", "addCpFileTable(), insert new fileTable into the map");

		std::pair<maptype::iterator, bool> insertResult;
		//add new entry
		insertResult = m_FileTableMap.insert(maptype::value_type(cpName, fileTableObj));

		if(!insertResult.second)
		{
			result = false;
			char errMsg[512] = {0};
			snprintf(errMsg, 511, "%s, operation insert failed for CP:<%s>, Map size:<%zd>", __func__, cpName.c_str(), m_FileTableMap.size());
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_DirMgr, "%s", errMsg);
			// reclaim allocated memory
			delete fileTableObj;
			fileTableObj = NULL;
		}
	}

	// check for fails
	if(!result)
	{
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "DirectoryStructureMgr::addCpFileTable(), operation failed for CP:<%s>, Map size:<%zd>", cpName.c_str(), m_FileTableMap.size());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_DirMgr, "%s", errMsg);
	}

	TRACE(fms_cpf_DirMgr, "%s", "Leaving addCpFileTable");
	return result;
}

/*============================================================================
	ROUTINE: updateInfiteFileAttribute
 ============================================================================ */
void FMS_CPF_DirectoryStructureMgr::updateInfiteFileAttribute(FileTable* fileTable, const std::string& fileName, const std::string& firstSubFile, const std::string& lastSubFile)
{
	TRACE(fms_cpf_DirMgr, "Entering in %s", __func__);
	FMS_CPF_FileId fileid(fileName);
	FileReference reference;
	const short LOGMSG_LENGTH = 256;

	try
	{
		char infoMsg[LOGMSG_LENGTH] = {0};

		snprintf(infoMsg, LOGMSG_LENGTH-1, "%s, first subfile:<%s> and last subfile:<%s> founded for file:<%s>, Cp:<%s>",
										__func__, firstSubFile.c_str(), lastSubFile.c_str(), fileName.c_str(), fileTable->getCpName() );

		CPF_Log.Write(infoMsg, LOG_LEVEL_INFO);
		TRACE(fms_cpf_DirMgr, "%s", infoMsg);

		// Open logical file
		reference = fileTable->open(fileid, FMS_CPF_Types::NONE_);

		FMS_CPF_Attribute fileAttribute = reference->getAttribute();

		FMS_CPF_Types::fileType fType;
		unsigned long recordLength=0;
		unsigned long maxSize=0;
		unsigned long maxTime=0;
		unsigned long activeSubFile = 0;
		unsigned long lastSent = 0;
		bool composite;
		bool releaseCondition=false;
		std::string transferQueue;
		FMS_CPF_Types::transferMode transferMode;
		FMS_CPF_Types::transferMode initTransferMode;
                   //HY46076
                long int deleteFileTimer=-1;
		FMS_CPF_Types::extractFileAttr(fileAttribute, fType, recordLength, composite, maxSize,
									   maxTime, releaseCondition ,activeSubFile, lastSent, transferMode,
									   transferQueue, initTransferMode,deleteFileTimer
									  );

		TRACE(fms_cpf_DirMgr, "%s, stored report:<%lu> and active<%lu>", __func__, lastSent, activeSubFile);

		// get the first subfile of the infinite file
		std::istringstream stringToInt(firstSubFile);
		stringToInt >> lastSent;

		TRACE(fms_cpf_DirMgr, "%s, current last report <%d>", __func__, lastSent);

		// Set last subfile sent to the first subfile less one,
		// so the next one to send will be just the current first one
		--lastSent;

		// get the last subfile, it is the current active subfile
		std::istringstream stringToInt2(lastSubFile);
		stringToInt2 >> activeSubFile;

		TRACE(fms_cpf_DirMgr, "%s, current active <%d>", __func__, activeSubFile);

		// Assemble the file attribute with old and new parameters value
		FMS_CPF_Types::fileAttributes newFileAttributes =
				FMS_CPF_Types::createFileAttr(fType, recordLength, composite, maxSize, maxTime,
											  releaseCondition, activeSubFile, lastSent, transferMode,
											  transferQueue, initTransferMode
											 );

		FMS_CPF_Attribute newFileAttribute(newFileAttributes);

		// Change file attributes
		reference->setAttribute(newFileAttribute);

		snprintf(infoMsg, LOGMSG_LENGTH-1, "%s, attribute of file:<%s> on Cp:<%s> updated: last report to <%lu> and active to <%lu>",
						__func__, fileName.c_str(), fileTable->getCpName(), lastSent, activeSubFile );
		CPF_Log.Write(infoMsg, LOG_LEVEL_INFO);
		TRACE(fms_cpf_DirMgr, "%s", infoMsg);

	}
	catch(const FMS_CPF_PrivateException& ex)
	{
		char errorMsg[LOGMSG_LENGTH] = {0};
		snprintf(errorMsg, LOGMSG_LENGTH-1, "%s, attribute file:<%s> on Cp:<%s>, error:<%s>", __func__, fileName.c_str(), fileTable->getCpName(), ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_DirMgr, "%s", errorMsg);
	}

	// close the CP file opened
	try
	{
		fileTable->close(reference);
	}
	catch(const FMS_CPF_PrivateException& ex)
	{
		char errorMsg[LOGMSG_LENGTH] = {0};
		snprintf(errorMsg, LOGMSG_LENGTH-1, "%s, exception on close of file:<%s> on CP:<%s>, error=<%s>", __func__, fileName.c_str(), fileTable->getCpName(), ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_DirMgr, "%s", errorMsg);
	}

	TRACE(fms_cpf_DirMgr, "Leaving %s", __func__);
}

/*============================================================================
	ROUTINE: updateInfiteFileAttribute
 ============================================================================ */
void FMS_CPF_DirectoryStructureMgr::removeInfiniteSubFilesFromIMM(OmHandler* objManager)
{
	TRACE(fms_cpf_DirMgr, "Entering in %s", __func__);

	// get all instance of infinite subfile
	std::vector<std::string> infiniteSubfileDNList;
	ACS_CC_ReturnType getResult = objManager->getClassInstances(cpf_imm::InfiniteSubFileClassName, infiniteSubfileDNList);

	if( (ACS_CC_SUCCESS == getResult) && !infiniteSubfileDNList.empty())
	{
		TRACE(fms_cpf_DirMgr, "%s(), found infinite subfile into IMM", __func__);

		// delete all runtime objects
		cpf_isf::runtimeObjectHandler runtimeOwner;

		std::vector<std::string>::const_iterator ifsDN;
		for(ifsDN = infiniteSubfileDNList.begin(); ifsDN != infiniteSubfileDNList.end(); ++ifsDN)
		{
			runtimeOwner.deleteRuntimeObj( ifsDN->c_str() );
		}
	}
	TRACE(fms_cpf_DirMgr, "Leaving %s", __func__);
}
/*============================================================================
	ROUTINE: refreshCpFileTable
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::refreshCpFileTable(const std::string& cpName)
{
	TRACE(fms_cpf_DirMgr, "[%s@%d] In (cpName == %s)", __FILE__, __LINE__, cpName.c_str());
	bool result = false;

	FileTable* fileTableObj = new (std::nothrow) FileTable(cpName, m_isMultiCP);

	// check allocation error and load CP file from IMM
	if((NULL != fileTableObj) )
	{
		result = true;
		char infoMsg[512] = {0};
		ACE_OS::snprintf(infoMsg, 511, "[%s@%d] insert new fileTable into the map for CP:<%s>", __FILE__, __LINE__, cpName.c_str());
		CPF_Log.Write(infoMsg, LOG_LEVEL_INFO);
		TRACE(fms_cpf_DirMgr, "%s", infoMsg);

		m_FileTableMap.insert(maptype::value_type(cpName, fileTableObj));
	}

	// check for fails
	if(!result)
	{
		char errMsg[512] = {0};
		ACE_OS::snprintf(errMsg, 511, "[%s@%s] operation failed for CP:<%s>, Map size:<%zd>",
				__FILE__, __FUNCTION__, cpName.c_str(), m_FileTableMap.size());
		CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_DirMgr, "%s", errMsg);
	}

	TRACE(fms_cpf_DirMgr, "[%s@%d] Out (result == %d)", __FILE__, __LINE__, result);
	return result;
}

/*============================================================================
	ROUTINE: filterVolume
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::filterVolume(const std::vector<std::string>& allVolumeDN, std::map<std::string, std::vector<std::string> >& volumeMap )
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in filterVolume()");
	bool result = true;

	std::vector<std::string>::const_iterator element;
	std::map<std::string, std::vector<std::string> >::iterator listElement;

	// for each Cp create a list of volume DN
	for(element = allVolumeDN.begin(); element != allVolumeDN.end(); ++element)
	{
		// Get the volume RDN ( cpVolumeId=<volumeName>:<cpName>) from DN
		// Split the field in RDN and Value
		size_t equalPos = (*element).find_first_of(parseSymbol::equal);
		size_t commaPos = (*element).find_first_of(parseSymbol::comma);

		// Check if some error happens
		if( (std::string::npos != equalPos) )
		{
			std::string volumeRDN;
			// check for a single field case
			if( std::string::npos == commaPos )
				volumeRDN = (*element).substr(equalPos + 1);
			else
				volumeRDN = (*element).substr(equalPos + 1, (commaPos - equalPos - 1) );

			// search ':' sign
			size_t tagAtSignPos = volumeRDN.find(parseSymbol::atSign);

			// Check if the tag is present
			if( std::string::npos != tagAtSignPos )
			{
				// get the cpName
				std::string cpName(volumeRDN.substr(tagAtSignPos + 1));

				// make the value in upper case
				ACS_APGCC::toUpper(cpName);

				listElement = volumeMap.find(cpName);

				if(volumeMap.end() != listElement )
				{
					// insert the new volume DN into the list of the right Cp
					listElement->second.push_back((*element));
				}
				else
				{
					// create the list for this Cp and insert the new volume DN into it
					std::vector<std::string> tmpVolumeList;
					tmpVolumeList.push_back((*element));
					volumeMap.insert(std::make_pair(cpName, tmpVolumeList));
				}
			}
			else
			{
				char errMsg[512] = {0};
				ACE_OS::snprintf(errMsg, 511, "DirectoryStructureMgr::filterVolume(), get Cp Name failed for volumeDN:<%s>, volumeRDN:<%s>, at sign not found", (*element).c_str(), volumeRDN.c_str());
				CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
				TRACE(fms_cpf_DirMgr, "%s", errMsg);
				result = false;
			}
		}
		else
		{
			char errMsg[512] = {0};
			ACE_OS::snprintf(errMsg, 511, "DirectoryStructureMgr::filterVolume(), get Cp Name failed for volumeDN:<%s>", (*element).c_str());
			CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
			TRACE(fms_cpf_DirMgr, "%s", errMsg);
			result = false;
		}
	}

	//Add blades for which any CpVolumehas been defined yet.
	std::list<std::string> tmpCpList = m_configReader->getCP_List();
	for (std::list<std::string>::const_iterator it = tmpCpList.begin(); it != tmpCpList.end(); ++it)
	{
		std::string cpName = *it;
		//if cpName is not defined yet...
		if (volumeMap.find(cpName) == volumeMap.end())
		{
			//..then add it to the volumeMap
			std::vector<std::string> emptyVolumeList;
			volumeMap.insert(std::make_pair(cpName, emptyVolumeList));
			TRACE(fms_cpf_DirMgr, "[%s@%d] CP with no volumes: \"%s\"", __FILE__, __LINE__, cpName.c_str());
		}
	}

	TRACE(fms_cpf_DirMgr, "%s", "Leaving filterVolume()");
	return result;
}

/*============================================================================
	ROUTINE: clearFileTableMap
 ============================================================================ */
void FMS_CPF_DirectoryStructureMgr::clearFileTableMap()
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in clearFileTableMap()");

	maptype::iterator it;
	// Delete each defined FileTable object
	for(it = m_FileTableMap.begin(); it != m_FileTableMap.end(); ++it )
	{
		delete it->second;
	}

	m_FileTableMap.clear();

	TRACE(fms_cpf_DirMgr, "%s", "Leaving clearFileTableMap()");
}

/*============================================================================
	ROUTINE: checkRaidDisk
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::dataDiskEmpty()
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in dataDiskEmpty()");

	std::string cpfRoot;

	if(m_isMultiCP)
	{
		// MCP not handled yet
		//Todo MCP solution
		cpfRoot = "/data/fms/data/cp1";
	}
	else
		cpfRoot = ParameterHndl::instance()->getCPFroot();

	boost::filesystem::path cpfDataDiskRoot(cpfRoot);
	boost::system::error_code checkResult;

	bool isEmpty = boost::filesystem::is_empty(cpfDataDiskRoot, checkResult);

	// check result
	if( checkResult.value() != SUCCESS)
	{
		// Maybe the folder does not exist.
		isEmpty = true;
	}

	if( isEmpty )
	{
		CPF_Log.Write("CPF Data Disk is Empty", LOG_LEVEL_INFO);
	}

	TRACE(fms_cpf_DirMgr, "Leaving dataDiskEmpty(), result empty:<%s>", (isEmpty ? "YES":"NO"));
	return isEmpty;
}

/*============================================================================
	ROUTINE: setSystemParameters
 ============================================================================ */
void FMS_CPF_DirectoryStructureMgr::createBackupObject(OmHandler* objManager)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering createBackupObject()");

	std::vector<ACS_CC_ValuesDefinitionType> objAttrList;

	char tmpRDN[512] = {0};
	// Assemble the RDN value
	ACE_OS::snprintf(tmpRDN, 255, "%s=%s", BRInfo::backupKey, BRInfo::cpfBackupRDN);

	// Fill the RDN attribute fields
	ACS_CC_ValuesDefinitionType attributeRDN;
	attributeRDN.attrName = BRInfo::backupKey;
	attributeRDN.attrType = ATTR_STRINGT;
	attributeRDN.attrValuesNum = 1;
	void* tmpValueRDN[1] = { reinterpret_cast<void*>(tmpRDN) };
	attributeRDN.attrValues = tmpValueRDN;

	objAttrList.push_back(attributeRDN);

	// Fill the version attribute fields
	ACS_CC_ValuesDefinitionType attributeVersion;
	attributeVersion.attrName = BRInfo::versionAttribute;
	attributeVersion.attrType = ATTR_STRINGT;
	attributeVersion.attrValuesNum = 1;
	void* tmpVersion[1] = { reinterpret_cast<void*>(BRInfo::backupVersion) };
	attributeVersion.attrValues = tmpVersion;

	objAttrList.push_back(attributeVersion);

	// Fill the Backup Type attribute fields
	ACS_CC_ValuesDefinitionType attributeBackupType;
	attributeBackupType.attrName = BRInfo::backupTypeAttribute ;
	attributeBackupType.attrType = ATTR_INT32T;
	attributeBackupType.attrValuesNum = 1;
	int backupTypeValue = 1;
	void* tmpBackupType[1] = { reinterpret_cast<void*>(&backupTypeValue)};
	attributeBackupType.attrValues = tmpBackupType;

	objAttrList.push_back(attributeBackupType);

	//Retry mechanism - HR81721 - begin
	ACS_CC_ReturnType getResult;
	int maxRetry = 30;
	bool tryAgain = true;
	do
	{
		getResult = objManager->createObject(BRInfo::backupImmClass, BRInfo::backupParent, objAttrList);
		if( ACS_CC_SUCCESS == getResult)
		{
			TRACE(fms_cpf_DirMgr, "createBackupObject(), backup object:<%s> created", tmpRDN);
			tryAgain = false;
		}
		else
		{
			if(-14 == objManager->getInternalLastError())
				tryAgain = false;
			else
			{
				char errMsg[512] = {0};
				ACE_OS::snprintf(errMsg, 511, "FMS_CPF_DirectoryStructureMgr::createBackupObject(), error:<%d> on createObject(<%s>)", objManager->getInternalLastError(), tmpRDN );
				CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
				TRACE(fms_cpf_DirMgr, "%s", errMsg);

				//100 milliseconds
//				usleep(100000);
				sleep(1);
				maxRetry--;
			}
		}

	}while (tryAgain && (maxRetry > 0));
	//Retry mechanism - HR81721 - end

	TRACE(fms_cpf_DirMgr, "%s", "Leaving createBackupObject()");
}


/*============================================================================
	ROUTINE: checkRaidDisk
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::checkRaidDisk()
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in checkRaidDisk()");

	bool result = false;
	// delay in sec
	int DELAY = 5;
	int MAX_RETRY = 10;
	bool tryAgain = true;
	int num_retry = 0;
	std::string raidCpFRoot = ParameterHndl::instance()->getDataDiskRoot();
	DIR* raidRoot;

    while(tryAgain)
    {
	   raidRoot = ::opendir(raidCpFRoot.c_str());
	   if(NULL != raidRoot )
	   {
		   ::closedir(raidRoot);
		   result = true;
		   tryAgain = false;
	   }
	   else
	   {
		   num_retry++;
		   TRACE(fms_cpf_DirMgr, "checkRaidDisk(), Cannot access raid CPF directory<%s>, attempt<%d>", raidCpFRoot.c_str(), num_retry);

		   // check if the max num of retry is reached
		   if(MAX_RETRY < num_retry)
		   {
			   tryAgain = false;
			   char errMsg[128]={'\0'};
			   ACE_OS::snprintf(errMsg, 127, "DirectoryStructureMgr::checkRaidDisk(), Cannot access raid CPF directory<%s> after %d attempts",  raidCpFRoot.c_str(), num_retry);
			   CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		   }
		   else
		   {
				// wait DELAY sec before retry again
				ACE_OS::sleep(DELAY);
		   }
	   }
    }
    TRACE(fms_cpf_DirMgr, "checkRaidDisk(), raid disk root is <%s>", (result ? "OK":"NOT OK"));
	return result;
}

/*============================================================================
	ROUTINE: isRestartAfterRestore
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::isRestartAfterRestore()
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in isRestartAfterRestore()");
	bool restoreDone = false;

	if( m_clearDataPath.empty() && !getClearPath())
	{
		CPF_Log.Write("isRestartAfterRestore(), error on get clear path", LOG_LEVEL_ERROR);
	}
	else
	{
		// check restart type
		std::string cpfClearPath = m_clearDataPath + BRInfo::CpfBRFolder;
		boost::filesystem::path cpfClearFolder(cpfClearPath);

		TRACE(fms_cpf_DirMgr, "isRestartAfterRestore(), check clear folder:<%s>", cpfClearPath.c_str());
		try
		{
			if( boost::filesystem::exists(cpfClearFolder) )
			{
				TRACE(fms_cpf_DirMgr, "%s", "isRestartAfterRestore(), normal restart");
			}
			else
			{
				TRACE(fms_cpf_DirMgr, "%s", "isRestartAfterRestore(), restart after restore");
				// create the folder
				boost::filesystem::create_directory(cpfClearFolder);
				restoreDone = true;
			}
		}
		catch(const boost::filesystem::filesystem_error& ex)
		{
			 char errMsg[128]={'\0'};
			 ACE_OS::snprintf(errMsg, 127, "DirectoryStructureMgr::isRestartAfterRestore(), failed to create PSO folder:<%s>, error:<%s>", cpfClearPath.c_str(), ex.what()  );
			 CPF_Log.Write(errMsg, LOG_LEVEL_ERROR);
		}
	}

	TRACE(fms_cpf_DirMgr, "isRestartAfterRestore(), system restored <%s>", (restoreDone ? "YES":"NO"));
	return restoreDone;
}

/*============================================================================
	ROUTINE: getClearPath
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::getClearPath()
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in setClearPath()");
	bool result = false;
	int bufferLength;
	ifstream clearFileStream;
	// open the clear file
	clearFileStream.open(BRInfo::PSAClearFilePath, ios::binary );

	// check for open error
	if(clearFileStream.good())
	{
		// get length of stored path:
		clearFileStream.seekg(0, ios::end);
		bufferLength = clearFileStream.tellg();
		clearFileStream.seekg(0, ios::beg);

		// allocate the buffer
		char buffer[bufferLength+1];
		ACE_OS::memset(buffer, 0, bufferLength+1);

		// read data
		clearFileStream.read(buffer, bufferLength);

		if(buffer[bufferLength-1] == '\n') buffer[bufferLength-1] = 0;

		m_clearDataPath = buffer;
		m_clearDataPath += DirDelim;
		result = true;
		TRACE(fms_cpf_DirMgr, "setClearPath(), clear path:<%s>", m_clearDataPath.c_str());
	}

	clearFileStream.close();
	TRACE(fms_cpf_DirMgr, "%s", "Leaving setClearPath()");
	return result;
}

/*============================================================================
	ROUTINE: find
 ============================================================================ */
File* FMS_CPF_DirectoryStructureMgr::find(const FMS_CPF_FileId id, const char* _cpname)
{
	File* result = NULL;
	std::string cpname(DEFAULT_CPNAME);

	// Checks the cpname parameter
	if(m_isMultiCP)
	{
		cpname = _cpname;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	// Checks if present in the fileTable Map
	if ( it != m_FileTableMap.end() )
	{
		// get pointer;
		FileTable* table = (*it).second;

		//retrieve the file reference
		result = table->find(id);
	}
	return result;
}

/*============================================================================
	ROUTINE: exists
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::exists(const FMS_CPF_FileId& fileid, const char* _cpname)
throw (FMS_CPF_PrivateException)
{
	bool result = false;
	std::string cpname(DEFAULT_CPNAME);

	// Checks the cpname parameter
	if(m_isMultiCP)
	{
		cpname = _cpname;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	// Checks if present in the fileTable Map
	if ( it != m_FileTableMap.end() )
	{
		// get pointer
		FileTable* table = (*it).second;
		// checks if the file exists
		result = table->exists(fileid);
	}
	return result;
}

/*============================================================================
	ROUTINE: create
 ============================================================================ */
FileReference FMS_CPF_DirectoryStructureMgr::create(const FMS_CPF_FileId& fileid,
											 const std::string& volume,
											 const FMS_CPF_Attribute& attribute,
											 FMS_CPF_Types::accessType access,
											 const std::string& fileDN,
											 const char * _cpname)
throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering create(...)");
	FileReference result;

	std::string cpname(DEFAULT_CPNAME);

	// Checks the cpname parameter
	if(m_isMultiCP)
	{
		cpname = _cpname;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	// Checks if present in the fileTable Map
	if ( it != m_FileTableMap.end() )
	{
		// get pointer
		FileTable* table = (*it).second;

		// Create the new file in the file Table
		result = table->create(fileid, volume, attribute, access, fileDN);
	}
	else
	{
		TRACE(fms_cpf_DirMgr, "%s", "create(...), CP Name not find in TableMap");
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTSUBFILE, fileid.data());
	}
	TRACE(fms_cpf_DirMgr, "%s", "Leaving create(...)");
	return result;
}

/*============================================================================
	ROUTINE: create
 ============================================================================ */
FileReference FMS_CPF_DirectoryStructureMgr::create(const FMS_CPF_FileId& fileid,
											 FMS_CPF_Types::accessType access,
											 const char * _cpname)
throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering create");

	FileReference result;
	std::string cpname(DEFAULT_CPNAME);

	// Checks the cpname parameter
	if(m_isMultiCP)
	{
		cpname = _cpname;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	// Checks if present in the fileTable Map
	if( m_FileTableMap.end() != it  )
	{
		// get pointer
		FileTable* table = (*it).second;
		// Create the new file in the file Table
		result = table->create(fileid, access);
	}
	else
	{
		TRACE(fms_cpf_DirMgr, "%s", "create, CP Name not find in TableMap");
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::NOTSUBFILE, fileid.data());
	}

	TRACE(fms_cpf_DirMgr, "%s", "Leaving create");
	return result;
}

/*============================================================================
	ROUTINE: closeExceptionLess
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::closeExceptionLess(FileReference ref, const char* _cpname)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in closeExceptionLess()");
	bool result = true;
	try{
		close(ref, _cpname);
	}
	catch(FMS_CPF_PrivateException& ex)
	{
		result= false;
		char errorMsg[1024] = {'\0'};
		snprintf(errorMsg, 1023,"closeExceptionLess, exception on file close, error=<%s>", ex.errorText() );
		CPF_Log.Write(errorMsg, LOG_LEVEL_ERROR);
		TRACE(fms_cpf_DirMgr, "%s", "closeExceptionLess(), exception catched");
	}
	TRACE(fms_cpf_DirMgr, "%s", "Leaving closeExceptionLess()");
	return result;
}

/*============================================================================
	ROUTINE: close
 ============================================================================ */
void FMS_CPF_DirectoryStructureMgr::close(FileReference ref, const char * _cpname)
throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in close()");

	std::string cpname(DEFAULT_CPNAME);

	// Checks the cpname parameter
	if(m_isMultiCP)
	{
		cpname = _cpname;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	if( m_FileTableMap.end() != it  )
	{
		// get FileTable pointer
		FileTable* table = (*it).second;
		table->close(ref);
	}
	else
	{
		TRACE(fms_cpf_DirMgr, "close(), Exception CP Name=%s not find in TableMap", cpname.c_str() );
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INVALIDPATH, cpname);
	}

	TRACE(fms_cpf_DirMgr, "%s", "Leaving close()");
}

/*============================================================================
	ROUTINE: remove
 ============================================================================ */
void FMS_CPF_DirectoryStructureMgr::remove(FileReference ref, const char * _cpname)
throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in remove()");

	std::string cpname(DEFAULT_CPNAME);

	// Checks the cpname parameter
	if(m_isMultiCP)
	{
		cpname = _cpname;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	if( m_FileTableMap.end() != it)
	{
		FileTable* table = (*it).second; // get pointer
		//it can throw FMS_CPF_PrivateException
		table->remove(ref);
	}
	else
	{
		TRACE(fms_cpf_DirMgr, "remove(), CP Name =% not find in TableMap", cpname.c_str() );
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::INVALIDPATH, cpname);
	}

	TRACE(fms_cpf_DirMgr, "%s", "Leaving remove()");
}

/*============================================================================
	ROUTINE: removeBlockSender
 ============================================================================ */
void FMS_CPF_DirectoryStructureMgr::removeBlockSender(FileReference reference, const char * _cpname)
throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in removeBlockSender()");

	std::string cpname(DEFAULT_CPNAME);

	// Checks the cpname parameter
	if(m_isMultiCP)
	{
		cpname = _cpname;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	if( m_FileTableMap.end() != it )
	{
		// get pointer to the correct FileTable
		FileTable* table = (*it).second;
		// It can throw FMS_CPF_PrivateException
		table->removeBlockSender(reference);
	}
	else
	{
		TRACE(fms_cpf_DirMgr, "removeBlockSender(), CP Name =% not find in TableMap", cpname.c_str() );
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::INVALIDPATH, cpname); //opp FILENOTFOUND
	}
	TRACE(fms_cpf_DirMgr, "%s", "Leaving removeBlockSender()");
}

/*============================================================================
	ROUTINE: open
 ============================================================================ */
FileReference FMS_CPF_DirectoryStructureMgr::open(const FMS_CPF_FileId& fileid,
										   FMS_CPF_Types::accessType access,
										   const char * _cpname, bool inf)
throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in open()");

	FileTable* table = NULL;
	std::string cpname(DEFAULT_CPNAME);

	// Checks the cpname parameter
	if(m_isMultiCP)
	{
		cpname = _cpname;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	if( m_FileTableMap.end() != it )
	{
		// get pointer
		table = (*it).second;
		return table->open(fileid, access, inf);
	}
	else
	{
		TRACE(fms_cpf_DirMgr, "open(),  CPName=%s not find in TableMap", cpname.c_str() );
		throw FMS_CPF_PrivateException (FMS_CPF_PrivateException::FILENOTFOUND, fileid.file ().c_str());
	}
}

/*============================================================================
	ROUTINE: getListFileId
 ============================================================================ */
int FMS_CPF_DirectoryStructureMgr::getListFileId(const FMS_CPF_FileId& fileId, std::list<FMS_CPF_FileId>& fileList, const char* _cpname)
throw (FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in getListFileId()");

	FileTable* table = NULL;
	std::string cpname(DEFAULT_CPNAME);

	// Checks the cpname parameter
	if(m_isMultiCP)
	{
		cpname = _cpname;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	if( m_FileTableMap.end() != it )
	{
		// get pointer
		table = (*it).second;
		return table->getListFileId(fileId, fileList);
	}
	else
	{
		TRACE(fms_cpf_DirMgr, "%s", "getListField(), CP Name not find in TableMap");
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::INVALIDPATH, cpname);
	}
}

/*============================================================================
	ROUTINE: getListOfInfiniteFile
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::getListOfInfiniteFile(const std::string& cpName, std::vector<std::pair<std::string, std::string> >& fileList)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in getListOfInfiniteFile()");
	bool result = false;
	std::string cpname(DEFAULT_CPNAME);

	// Checks the system type
	if(m_isMultiCP)
	{
		cpname = cpName;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	if( m_FileTableMap.end() != it )
	{

		result = true;
		// get pointer
		(*it).second->getListOfInfiniteFile(fileList);
	}
	TRACE(fms_cpf_DirMgr, "%s", "Leaving getListOfInfiniteFile()");
	return result;
}

/*============================================================================
	ROUTINE: getListOfInfiniteSubFile
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::getListOfInfiniteSubFile(const std::string& infiniteFileName, const std::string& cpName, std::set<std::string>& subFileList)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in getListOfInfiniteSubFile()");
	bool result = false;
	std::string cpname(DEFAULT_CPNAME);

	// Checks the system type
	if(m_isMultiCP)
	{
		cpname = cpName;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	if( m_FileTableMap.end() != it )
	{

		result = true;
		// get pointer
		(*it).second->getListOfInfiniteSubFile(infiniteFileName, subFileList);
	}
	TRACE(fms_cpf_DirMgr, "%s", "Leaving getListOfInfiniteSubFile()");
	return result;
}

/*============================================================================
	ROUTINE: getFileDN
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::getFileDN(const std::string& fileName, const std::string& cpName, std::string& fileDN)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in getFileDN()");
	bool result = false;
	std::string cpname(DEFAULT_CPNAME);

	// Checks the system type
	if(m_isMultiCP)
	{
		cpname = cpName;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	if( m_FileTableMap.end() != it )
	{
		// get pointer
		result = (*it).second->getFileDN(fileName, fileDN);
	}

	TRACE(fms_cpf_DirMgr, "%s", "Leaving getFileDN()");
	return result;
}

/*============================================================================
	ROUTINE: setFileDN
 ============================================================================ */
void FMS_CPF_DirectoryStructureMgr::setFileDN(const std::string& fileName, const std::string& cpName, const std::string& fileDN)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in setFileDN()");
	std::string cpname(DEFAULT_CPNAME);

	// Checks the system type
	if(m_isMultiCP)
	{
		cpname = cpName;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	if( m_FileTableMap.end() != it )
	{
		// get pointer
		(*it).second->setFileDN(fileName, fileDN);
	}
	TRACE(fms_cpf_DirMgr, "%s", "Leaving setFileDN()");
}

/*============================================================================
	ROUTINE: getFileType
 ============================================================================ */
FMS_CPF_Types::fileType FMS_CPF_DirectoryStructureMgr::getFileType(const std::string& fileName, const std::string& cpName)
	throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in getFileType()");
	FMS_CPF_Types::fileType fileType;
	std::string cpname(DEFAULT_CPNAME);

	// Checks the cpname parameter
	if(m_isMultiCP)
	{
		cpname = cpName;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	if( m_FileTableMap.end() != it )
	{
		// get pointer
		fileType = (*it).second->getCpFileType(fileName);
	}
	else
	{
		TRACE(fms_cpf_DirMgr, "open(),  CPName=<%s> not find in TableMap", cpname.c_str() );
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::FILENOTFOUND, fileName);
	}
	TRACE(fms_cpf_DirMgr, "Leaving getFileType(), type:<%d>", fileType);
	return fileType;
}

/*============================================================================
	ROUTINE: isFileLockedForDelete
 ============================================================================ */
bool FMS_CPF_DirectoryStructureMgr::isFileLockedForDelete(const std::string& fileName, const std::string& cpName)
{
	TRACE(fms_cpf_DirMgr, "%s", "Entering in isFileLockedForDelete()");
	bool isLocked = false;
	std::string cpname(DEFAULT_CPNAME);

	// Checks the system type
	if(m_isMultiCP)
	{
		cpname = cpName;
	}

	// Search for cpname in table
	maptype::const_iterator it = m_FileTableMap.find(cpname);

	if( m_FileTableMap.end() != it )
	{
		// get pointer and call method
		isLocked = (*it).second->isFileLockedForDelete(fileName);
	}

	TRACE(fms_cpf_DirMgr, "%s", "Leaving isFileLockedForDelete()");
	return isLocked;
}

bool FMS_CPF_DirectoryStructureMgr::cpExists(const std::string& cpName)
{
	bool result = true;

	// Checks the cpname parameter
	if(m_isMultiCP)
	{
		// Search for cpname in table
		maptype::const_iterator it = m_FileTableMap.find(cpName);

		// Checks if is not present in the fileTable Map
		if ( it == m_FileTableMap.end() )
		{
			result = false;
		}
	}

	return result;
}
