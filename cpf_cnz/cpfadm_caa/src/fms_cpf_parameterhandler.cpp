/*
 * * @file fms_cpf_parameterhandler.cpp
 *	@brief
 *	Class method implementation for ParameterHandler.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_parameterhandler.h module
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
 *	| 1.0.0  | 2011-07-04 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_common.h"
#include "fms_cpf_eventalarmhndl.h"
#include "fms_cpf_configreader.h"

#include "ACS_TRA_trace.h"
#include "ACS_APGCC_Util.H"
#include "ACS_APGCC_CommonLib.h"


//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
ParameterHandler::ParameterHandler(int dboCrash)
: rootdir_(),
  rootlogdir_(),
  timeout_(0),
  m_CpTimeSlice(stdValue::CP_TIME_SLICE_DEFAULT),
  m_isMultiCP(false),
  m_FmsUserId(0),
  m_conf(NULL),
  fms_cpf_ParHdl( new (std::nothrow) ACS_TRA_trace("FMS_CPF_ParameterHandler"))
{

}

/*============================================================================
	ROUTINE: ~ParameterHandler
 ============================================================================ */
ParameterHandler::~ParameterHandler()
{
	dataDiskPathForCPMap.clear();
	logDiskPathForCPMap.clear();
	if(NULL != fms_cpf_ParHdl)
		delete fms_cpf_ParHdl;
}

//------------------------------------------------------------------------------
//      Read parameters from the parameter handler
//------------------------------------------------------------------------------
void ParameterHandler::load()
{
	TRACE(fms_cpf_ParHdl, "%s", "Entering load()");

	ACS_APGCC_CommonLib pathHandler;
	ACS_APGCC_DNFPath_ReturnTypeT res;
	std::string fms_data("FMS_DATA");
	std::string fms_log("FMS_LOGS");
	char tmp_path[s_MaxPathLength]={0};
	int tmp_pathLen = s_MaxPathLength;

	loadCpTimeSliceFromImm();

	// List of defined CPs
	std::list <short> cpIdList;

	if(m_conf != NULL)
    {
    	//is blade cluster flag const_cast<FMS_CPF_ConfigReader*>
		m_isMultiCP = m_conf->IsBladeCluster();
    }
    else
    {
    	TRACE(fms_cpf_ParHdl, "%s", "load(), Error: configuration reader is null");
    }

	res = pathHandler.GetDataDiskPath(fms_data.c_str(), tmp_path, tmp_pathLen);
	rootdir_ = tmp_path;

	// Checks the system type
    if(m_isMultiCP)
    {
    	cpIdList = m_conf->getCP_IdList();

    	TRACE(fms_cpf_ParHdl, "load(), Multiple CP System environment, num. of CP=%zd", cpIdList.size());

		// load Map of Data Disk Path For CP
		loadMapDataDiskPathForCP(cpIdList);

		// load Map of Log Disk Path For CP
		loadMapLogDiskPathForCP(cpIdList);
    }
    else
    {
    	TRACE(fms_cpf_ParHdl, "%s", "load(), Single CP System environment");
    }

	TRACE(fms_cpf_ParHdl, "load(), FMS Data path= %s", rootdir_.c_str());

	res = pathHandler.GetDataDiskPath(fms_log.c_str(), tmp_path, tmp_pathLen);
	rootlogdir_ = tmp_path;
	TRACE(fms_cpf_ParHdl, "load(), FMS Log path= %s", rootlogdir_.c_str());

	//TODO  TQ dir??
    /*
       createCPFTQdir(rootdir_);

	   char gohdir[256];
	   gohdir[0] = '\0';
	   strcat(gohdir,rootdir_.c_str());
	   strcat(gohdir, "GOHdest");
	   createDir(gohdir);
     */

     if (!m_isMultiCP)
     {
    	/* TRACE(fms_cpf_ParHdl, "%s", "load(), Creating default directory for SCP");

    	 std::string volumeTemp = rootdir_;
    	 volumeTemp += (DirDelim + CPFDir + DirDelim + DefVolTEMP);

    	 TRACE(fms_cpf_ParHdl, "load(), TEMPVOLUME dir= %s", volumeTemp.c_str());
    	 //create TEMPVOLUME directory
    	 //TODO add IMM interaction to create the volume object
    	 //ACS_APGCC::create_directories(volumeTemp.c_str());

    	 std::string volumeRel = rootdir_;
    	 volumeRel += (DirDelim + CPFDir + DirDelim + DefVolREL);
    	 TRACE(fms_cpf_ParHdl, "load(), RELVOLUMSW dir= %s", volumeRel.c_str());
    	 //create RELVOLUMSW directory
    	 //TODO add IMM interaction to create the volume object
    	 //ACS_APGCC::create_directories(volumeRel.c_str());

    	 std::string logDir = rootlogdir_;
    	 logDir += (DirDelim + CPFDir);
    	 TRACE(fms_cpf_ParHdl, "load(), FMS LOG dir= %s", logDir.c_str());
    	 //create CPF LOG directory
    	 ACS_APGCC::create_directories(logDir.c_str());
    	 */

    }
    else
    {
     	 loadBCDirectoryStructure("fmsapdata", cpIdList);
    	 loadBCDirectoryStructure("fmsaplogs",	cpIdList, true);
    }
    TRACE(fms_cpf_ParHdl, "%s", "Leaving load()");
}
//------------------------------------------------------------------------------
//      Load directory structure for MCP
//------------------------------------------------------------------------------
void ParameterHandler::loadBCDirectoryStructure(const char *szLogicName, std::list<short> cpIdList, bool log)
{
	TRACE(fms_cpf_ParHdl, "%s", "Entering loadBCDirectoryStructure()");
    // Check for the type of query
   	char szLogName[10];
    ::memset(szLogName, 0, 10);

    if(::strcmp(szLogicName,  "fmsapdata") == 0)
    {
    	::strcpy(szLogName, "FMS_DATA");
    }
    else if(::strcmp(szLogicName, "fmsaplogs") == 0)
    {
    	// else check for for Log directory
        ::strcpy(szLogName, "FMS_LOGS");
    }
    else
    {
    	//else query is for FTP volume // else check  for FTP vol directory
        ::strcpy(szLogName, "FTP_VOL");
    }

    TRACE(fms_cpf_ParHdl, "loadBCDirectoryStructure(), logical name=%s", szLogName);

    std::list<short>::iterator it;

    for (it = cpIdList.begin(); it != cpIdList.end(); it++)
    {
      ACS_APGCC_DNFPath_ReturnTypeT res;
      int nCP_ID = (*it);
      char tmp_dir[s_MaxPathLength] = {0};
      int tmp_dirLen = s_MaxPathLength;
      std::string path;

      // Check for the CLUSTER cp id
      if (nCP_ID == CLUSTER_CPid)
      {
         // For the CLUSTER cp id, we need to build a specific path
    	 res = ACS_APGCC_DNFPATH_SUCCESS;
         path = rootdir_;
         path += ( DirDelim + ClusterDir + DirDelim + CPFDir );
      }
      else
      {
    	  ACS_APGCC_CommonLib pathHandler;
		  // get the CP path by APGCC
    	  res = pathHandler.GetDataDiskPathForCp(szLogName, nCP_ID, tmp_dir, tmp_dirLen);
		  path = tmp_dir;
		  path += ( DirDelim + CPFDir );
      }

      if ( ACS_APGCC_DNFPATH_SUCCESS == res )
      {
    	  TRACE(fms_cpf_ParHdl, "loadBCDirectoryStructure(), creating folder=%s", path.c_str());
    	  //Creating the folders
    	  ACS_APGCC::create_directories(path.c_str());
      }
      else
      {
    	  TRACE(fms_cpf_ParHdl, "loadBCDirectoryStructure(), failed to get get path for CPid=%i, error=%i", nCP_ID, res);
      }

      if (!log)
      {
        /* TODO Creation of default Volumes in MCP
         * // create the volume RELVOLUMSW in CPF
         strcpy(relvolumsw_dir, cpf_dir);
         strcat(relvolumsw_dir, "\\");
         strcat(relvolumsw_dir, "RELVOLUMSW");
         createDir(relvolumsw_dir);
         if (ACS_TRA_ON(fms_cpf_param))
         {
            sprintf(tracep, 
               "ParameterHandler::loadBCDirectoryStructure creating directory %s", relvolumsw_dir);
            ACS_TRA_event(&fms_cpf_param, tracep);
         }

         // create the volume TEMPVOLUME in CPF
         strcpy(tempvolume_dir, cpf_dir);
         strcat(tempvolume_dir, "\\");
         strcat(tempvolume_dir, "TEMPVOLUME");
         createDir(tempvolume_dir);
         if (ACS_TRA_ON(fms_cpf_param))
         {
            sprintf(tracep, 
               "ParameterHandler::loadBCDirectoryStructure creating directory %s", tempvolume_dir);
            ACS_TRA_event(&fms_cpf_param, tracep);
         }

         // create the TMP directory
         strcpy(tmp_dir, cp_dir);
         strcat(tmp_dir, "\\");
         strcat(tmp_dir, "tmp");
         createDir(tmp_dir);
         if (ACS_TRA_ON(fms_cpf_param))
         {
            sprintf(tracep, 
               "ParameterHandler::loadBCDirectoryStructure creating directory %s", tmp_dir);
            ACS_TRA_event(&fms_cpf_param, tracep);
         }
         */
      }
   }
   TRACE(fms_cpf_ParHdl, "%s", "Leaving loadBCDirectoryStructure()");
}

//------------------------------------------------------------------------------
//      Init the static variables
//------------------------------------------------------------------------------
void ParameterHandler::init(FMS_CPF_ConfigReader* config)
{
	m_conf = config;

	dataDiskPathForCPMap.clear();
	logDiskPathForCPMap.clear();

	//Initialize all
	load();
}

//------------------------------------------------------------------------------
//     Delete dynamic objects
//------------------------------------------------------------------------------
void ParameterHandler::deleteInstance()
{
	if(NULL != fms_cpf_ParHdl)
		delete fms_cpf_ParHdl;
}

//------------------------------------------------------------------------------
// Get path to CPF root directory
//------------------------------------------------------------------------------
std::string ParameterHandler::getCPFroot(const char* cpname)
{
	if(m_isMultiCP)
	{
		return ParameterHandler::findDataPathForCP(cpname);
	}
	else
	{
		std::string rootPath(rootdir_);
		rootPath += ( DirDelim + CPFDir );
		return rootPath;
	}
}

//------------------------------------------------------------------------------
//      Get path to CPF log directory
//------------------------------------------------------------------------------
std::string ParameterHandler::getCPFlogDir(const char* cpname)
{
	if(m_isMultiCP)
	{
		return ParameterHandler::findLogPathForCP(cpname);
	}
	else
		return rootlogdir_;
}

//------------------------------------------------------------------------------
//      Get timeout time for CP communication channels
//------------------------------------------------------------------------------
int ParameterHandler::getTimeOut()
{
   return (timeout_ * 1000); 
}

// We try to avoid the frequency access to the library
// creating a cache map with all CPs path (key=CPname, value=CPPath)
// In a Single CP system  will have key=CP_DEFAULT_NAME, value=FMS_DATA$/CPF.
void ParameterHandler::loadMapDataDiskPathForCP(const std::list<short>& cpIdList)
{
   TRACE(fms_cpf_ParHdl, "%s", "Entering in loadMapDataDiskPathForCP()");
   std::string logName("FMS_DATA");
   std::list<short>::const_iterator it;   //iterator it;
   ACS_APGCC_CommonLib pathHandler;
   ACS_APGCC_DNFPath_ReturnTypeT res;

   for(it = cpIdList.begin(); it != cpIdList.end(); it++)
   {
	  int nCP_ID = (*it);
	  char tmp_dir[s_MaxPathLength] = {0};
	  int tmp_dirLen = s_MaxPathLength;
	  std::string path;

	  // Check for the CLUSTER cp id
	  if (nCP_ID == CLUSTER_CPid)
	  {
		 // For the CLUSTER cp id, we need to build a specific path
		 res = ACS_APGCC_DNFPATH_SUCCESS;
		 path = rootdir_;
		 path += ( DirDelim + ClusterDir + DirDelim + CPFDir );
	  }
	  else
	  {
		  // get the CP path by APGCC
		  res = pathHandler.GetDataDiskPathForCp(logName.c_str(), nCP_ID, tmp_dir, tmp_dirLen);
		  path = tmp_dir;
		  path += ( DirDelim + CPFDir );
	  }

	  if ( ACS_APGCC_DNFPATH_SUCCESS == res )
	  {
		  std::string cpDefName = m_conf->cs_getDefaultCPName(nCP_ID);
		  std::pair<maptype2::iterator, bool> ret;
		  ret = dataDiskPathForCPMap.insert(maptype2::value_type(cpDefName, path));
		  if (ret.second == false)
		  {
			  TRACE(fms_cpf_ParHdl, "loadMapDataDiskPathForCP failed to insert dir. %s", path.c_str());
		  }
	  }
	  else
	  {
		  TRACE(fms_cpf_ParHdl, "loadMapDataDiskPathForCP(), failed to get path for CPid=%i, error=%i", nCP_ID, res);
	  }
   }
   TRACE(fms_cpf_ParHdl, "%s", "Leaving loadMapDataDiskPathForCP()");
}



//-----------------------------------------------------------------------------------------
// loadMapDataDiskPathForCP
// This method is used in MCP system to update the internal map with a new defined CP
//-----------------------------------------------------------------------------------------
void ParameterHandler::loadMapDataDiskPathForCP(short &cpId)
{
	TRACE(fms_cpf_ParHdl, "[%s::%s()@%d] - In", __FILE__, __FUNCTION__, __LINE__);
	std::string logicalName("FMS_DATA");
	ACS_APGCC_CommonLib pathHandler;
	ACS_APGCC_DNFPath_ReturnTypeT res = ACS_APGCC_DNFPATH_FAILURE;

	char tmp_dir[s_MaxPathLength] = {0};
	int tmp_dirLen = s_MaxPathLength;
	std::string path;

	// Check for the CLUSTER cp id
	if (m_isMultiCP && CLUSTER_CPid != cpId)
	{
		res = pathHandler.GetDataDiskPathForCp(logicalName.c_str(), cpId, tmp_dir, tmp_dirLen);
		path = tmp_dir;
		path += ( DirDelim + CPFDir );
	}

	if ( ACS_APGCC_DNFPATH_SUCCESS == res )
	{
		std::string cpDefName = m_conf->cs_getDefaultCPName(cpId);
		std::pair<maptype2::iterator, bool> ret;
		ret = dataDiskPathForCPMap.insert(maptype2::value_type(cpDefName, path));
		if (ret.second == false)
		{
			TRACE(fms_cpf_ParHdl, "[%s::%s()@%d] - failed to insert dir %s", __FILE__, __FUNCTION__, __LINE__, path.c_str());
		}
	}
	else
	{
		TRACE(fms_cpf_ParHdl, "[%s::%s()@%d] - failed to get path for CPid=%i, error=%i", __FILE__, __FUNCTION__, __LINE__, cpId, res);
	}

	TRACE(fms_cpf_ParHdl, "[%s::%s()@%d] - Out", __FILE__, __FUNCTION__, __LINE__);
}

// We try to avoid the frequency access to the library
// creating a cache map with all CPs path (key=CPname, value=CPPath)
// In a Single CP system  will have key=CP_DEFAULT_NAME, value=FMS_DATA$/CPF.
void ParameterHandler::loadMapLogDiskPathForCP(const std::list<short>& cpIdList)
{
	TRACE(fms_cpf_ParHdl, "[%s::%s()@%d] - In", __FILE__, __FUNCTION__, __LINE__);
   std::string logName("FMS_LOGS");
   std::list<short>::const_iterator it;

   ACS_APGCC_DNFPath_ReturnTypeT res;
   ACS_APGCC_CommonLib pathHandler;

   for (it = cpIdList.begin(); it != cpIdList.end(); it++)
   {
	   int nCP_ID = (*it);
	   char tmp_dir[s_MaxPathLength] = {0};
	   int tmp_dirLen = s_MaxPathLength;
	   std::string path;

      // For the CLUSTER cp id, we need to build a specific path
      if (nCP_ID != CLUSTER_CPid)
      {
    	  res = pathHandler.GetDataDiskPathForCp(logName.c_str(), nCP_ID, tmp_dir, tmp_dirLen);
    	  path = tmp_dir;
    	  path += ( DirDelim + CPFDir + DirDelim);
      }
      else
      {
    	  res = ACS_APGCC_DNFPATH_SUCCESS;
    	  path = rootlogdir_;
    	  path += ( DirDelim + ClusterDir + DirDelim + CPFDir + DirDelim);
      }

      if( ACS_APGCC_DNFPATH_SUCCESS == res )
      {
    	  std::string cpDefName = m_conf->cs_getDefaultCPName(nCP_ID);
		  pair<maptype2::iterator,bool> ret;
		  ret = logDiskPathForCPMap.insert(maptype2::value_type(cpDefName, path));
		  if (ret.second == false)
		  {
			  TRACE(fms_cpf_ParHdl, "loadMapLogDiskPathForCP failed to insert dir. %s", path.c_str());
   	      }
	  }
      else
      {
    	  TRACE(fms_cpf_ParHdl, "loadMapLogDiskPathForCP(), failed to get path for CPid=%i, error=%i", nCP_ID, res);
      }
   }
   TRACE(fms_cpf_ParHdl, "%s", "Leaving loadMapLogDiskPathForCP()");
}

//-----------------------------------------------------------------------------------------
// loadMapLogDiskPathForCP
// This method is used in MCP system to update the internal map with a new defined CP
//-----------------------------------------------------------------------------------------
void ParameterHandler::loadMapLogDiskPathForCP(short& cpId)
{
	TRACE(fms_cpf_ParHdl, "[%s::%s()@%d] - In", __FILE__, __FUNCTION__, __LINE__);
	std::string logicalName("FMS_LOGS");
	ACS_APGCC_DNFPath_ReturnTypeT res = ACS_APGCC_DNFPATH_FAILURE;
	ACS_APGCC_CommonLib pathHandler;
	char tmp_dir[s_MaxPathLength] = {0};
	int tmp_dirLen = s_MaxPathLength;
	std::string path;

	// Check for the CLUSTER cp id
	if (m_isMultiCP && CLUSTER_CPid != cpId)
	{
		res = pathHandler.GetDataDiskPathForCp(logicalName.c_str(), cpId, tmp_dir, tmp_dirLen);
		path = tmp_dir;
		path += ( DirDelim + CPFDir + DirDelim);
	}

	if( ACS_APGCC_DNFPATH_SUCCESS == res )
	{
		std::string cpDefName = m_conf->cs_getDefaultCPName(cpId);
		pair<maptype2::iterator,bool> ret;
		ret = logDiskPathForCPMap.insert(maptype2::value_type(cpDefName, path));
		if (ret.second == false)
		{
			TRACE(fms_cpf_ParHdl, "[%s::%s()@%d] failed to insert dir \"%s\"", __FILE__, __FUNCTION__, __LINE__, path.c_str());
		}
	}
	else
	{
		TRACE(fms_cpf_ParHdl, "[%s::%s()@%d] failed to get path for CPid=%i, error=%i", __FILE__, __FUNCTION__, __LINE__, cpId, res);
	}

   TRACE(fms_cpf_ParHdl, "[%s::%s()@%d] - Out", __FILE__, __FUNCTION__, __LINE__);
}

//------------------------------------------------------------------------------
//      CPs path find from Cache
//------------------------------------------------------------------------------
std::string ParameterHandler::findDataPathForCP(const std::string& cpName)
{
	std::string dataPath;

	// Search for cpname in table
	maptype2::iterator it = dataDiskPathForCPMap.find(cpName);
	if ( it != dataDiskPathForCPMap.end() )
	{
		// get the data path of cp
		dataPath = (*it).second;
	}

	return dataPath;
}
//------------------------------------------------------------------------------
//      CPs path find from Cache
//------------------------------------------------------------------------------
std::string ParameterHandler::findLogPathForCP(const std::string& cpName)
{
	std::string logPath;
	// Search for cpname in table

	maptype2::iterator it = logDiskPathForCPMap.find(cpName);
	if ( it != dataDiskPathForCPMap.end() )
	{
		// get the log path of cp
		logPath = (*it).second;
	}

	return logPath;
}

void ParameterHandler::setCpTimeSlice(const int& cpTimeSlice)
{
	m_CpTimeSlice= cpTimeSlice;
}

//------------------------------------------------------------------------------
//      Read CpTimeSlice From IMM
//------------------------------------------------------------------------------
void ParameterHandler::loadCpTimeSliceFromImm()
{
	TRACE(fms_cpf_ParHdl, "Entering %s", __func__);

	OmHandler objManager;

	// Init OM resource
	if(objManager.Init() != ACS_CC_FAILURE)
	{
		//attributes to get
		ACS_CC_ImmParameter blockTQTimeSliceAttribute;
		blockTQTimeSliceAttribute.attrName = cpf_imm::blockTransferTimeSliceAttribute;
		blockTQTimeSliceAttribute.attrType = ATTR_INT32T;

		// Get attribute by IMM
		ACS_CC_ReturnType getResult = objManager.getAttribute(cpf_imm::parentRoot, &blockTQTimeSliceAttribute );

		if( (ACS_CC_FAILURE != getResult) && (0 != blockTQTimeSliceAttribute.attrValuesNum ) )
		{
			m_CpTimeSlice = (*reinterpret_cast<int*>(blockTQTimeSliceAttribute.attrValues[0]));
		}
		else
		{
			TRACE(fms_cpf_ParHdl, "%s, failed to get attribute:<%s> value", __func__, cpf_imm::blockTransferTimeSliceAttribute);
		}

		objManager.Finalize();
	}

	TRACE(fms_cpf_ParHdl, "Leaving %s, block time slice:<%d>", __func__, m_CpTimeSlice);
}
