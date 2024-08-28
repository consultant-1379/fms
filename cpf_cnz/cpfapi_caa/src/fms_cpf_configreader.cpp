//**********************************************************************
//
// NAME
//      FMS_CPF_ConfigReader.C
//
// COPYRIGHT Ericsson Telecom AB, Sweden 2009.
// All rights reserved.
//
// The Copyright to the computer program(s) herein is the property of
// Ericsson Telecom AB, Sweden.
// The program(s) may be used and/or copied only with the written
// permission from Ericsson Telecom AB or in accordance with
// the terms and conditions stipulated in the agreement/contract under
// which the program(s) have been supplied.

// DESCRIPTION
//
// See FMS_CPF_ConfigReader.H
// .

// DOCUMENT NO
//

// AUTHOR
//

// REVISION
//
//

// CHANGES
//
//	RELEASE REVISION HISTORY
//
// REV NO   DATE     NAME     DESCRIPTION
//          091028   ewalbra  added cp CLUSTER option support
//					esalves
// SEE ALSO
//
//
//**********************************************************************


#include "fms_cpf_configreader.h"
#include "fms_cpf_api_trace.h"

#include "ACS_TRA_trace.h"
#include <ACS_CS_API.h>
#include <ace/ACE.h>

using namespace ACE_OS;



std::string fms_cpf_configReader_name = "FMS_CPF_ConfigReader";

FMS_CPF_ConfigReader::FMS_CPF_ConfigReader() : configurationMap()
{
   m_bIsBC = false;
   m_nNumCP = 0;
   fms_cpf_configReader_trace = new (std::nothrow) ACS_TRA_trace(fms_cpf_configReader_name);
   configurationMap.erase(configurationMap.begin(),configurationMap.end());
}

FMS_CPF_ConfigReader::~FMS_CPF_ConfigReader()
{
	if (NULL != fms_cpf_configReader_trace)
		delete fms_cpf_configReader_trace;
}

void FMS_CPF_ConfigReader::init()
throw (FMS_CPF_Exception)
{
   TRACE(fms_cpf_configReader_trace,"%s", "Entering FMS_CPF_ConfigReader::init()");
   try
   {
      m_bIsBC = cs_isMultipleCPSystem();
      if (m_bIsBC)
      {
         std::list<short> cpIdList = cs_getCPList();
         m_nNumCP = cpIdList.size();
         if (m_nNumCP <= 0)
            throw FMS_CPF_Exception (FMS_CPF_Exception::INTERNALERROR, "FMS_CPF_ConfigReader - The system is multiple CP, but the CP list is empty!");

         std::list<short>::iterator it;
         for (it = cpIdList.begin(); it != cpIdList.end(); it++)
         {
            short id = *it;

            std::string default_name = cs_getDefaultCPName(id);

            //add new entry in the configuration map
            std::pair<maptype::iterator, bool> ret;
            ret = configurationMap.insert(maptype::value_type(id, std::string(default_name)));
            if (ret.second == false)
            {
               TRACE(fms_cpf_configReader_trace,"FMS_CPF_ConfigReader::init() - Error inserting default name: %s", default_name.c_str());
               throw FMS_CPF_Exception (FMS_CPF_Exception::INTERNALERROR, "FMS_CPF_ConfigReader - Error creating the configuration Map!");
            }
         }
      }
   }
   catch (FMS_CPF_Exception& ex)
   {
      throw ex;
   }
}

bool FMS_CPF_ConfigReader::IsBladeCluster()
{
	return m_bIsBC;
}

std::list<std::string> FMS_CPF_ConfigReader::getCP_List()
{
	std::list<std::string> cpNameList;
	if (!configurationMap.empty())
	{
		maptype::iterator it;
		for (it = configurationMap.begin(); it != configurationMap.end(); it++)
		{
			std::string name = (*it).second;
			cpNameList.push_back(name);
		}
	}
	return cpNameList;
}

std::list<short> FMS_CPF_ConfigReader::getCP_IdList()
{
   std::list<short> cpIdList;
   if (!configurationMap.empty())
   {
      maptype::iterator it;
      for (it = configurationMap.begin(); it != configurationMap.end(); it++)
      {
         short id = (*it).first;
         cpIdList.push_back(id);
         TRACE(fms_cpf_configReader_trace, "FMS_CPF_ConfigReader::getCP_IdList() - getCP_IdList: id = %d", id);
      }
   }
   return cpIdList;
}


int FMS_CPF_ConfigReader::GetNumCP()
{
	return m_nNumCP;
}

//CS COMMUNICATION METHODS

//query for Network Element Information

bool FMS_CPF_ConfigReader::cs_isMultipleCPSystem()
	 throw (FMS_CPF_Exception)
{
	TRACE(fms_cpf_configReader_trace,"%s", "Entering cs_isMultipleCPSystem()");

	// Check if multiple CP system
	bool isMultipleCPSystem = false;
	ACS_CS_API_NS::CS_API_Result result = ACS_CS_API_NS::Result_Success;

	// There is a problem in CS, therefore the next instruction is commented.
	result = ACS_CS_API_NetworkElement::isMultipleCPSystem(isMultipleCPSystem);

	TRACE(fms_cpf_configReader_trace,"%s", "cs_isMultipleCPSystem(), after CS call");

		switch (result)
		{
			case ACS_CS_API_NS::Result_Success:
				{
					return isMultipleCPSystem;
				}
			case ACS_CS_API_NS::Result_NoEntry:
				{
					TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_isMultipleCPSystem() - CS System return Result_NoEntry");
					throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOENTRY, "");
				}
				break;
			case ACS_CS_API_NS::Result_NoValue:
				{
					TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_isMultipleCPSystem() - CS System return Result_NoValue");
					throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOVALUE, "");
				}
				break;
			case ACS_CS_API_NS::Result_NoAccess:
				{
					TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_isMultipleCPSystem() - CS System return Result_NoAccess");
					throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOACCESS, "");
				}
				break;
			case ACS_CS_API_NS::Result_Failure:
				{
					TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_isMultipleCPSystem() - CS System return Result_Failure");
					throw FMS_CPF_Exception (FMS_CPF_Exception::CSOTHERFAILURE, "");
				}
				break;
		}
		return false;

	//}
}


//query for Network Element Information

std::string FMS_CPF_ConfigReader::cs_getDefaultCPName(short id)
throw (FMS_CPF_Exception)
{

   TRACE(fms_cpf_configReader_trace,"FMS_CPF_ConfigReader::cs_getDefaultCPName() configMap.size = %d",configurationMap.size() );

   //access to configuration map
   if (!configurationMap.empty())
   {
      maptype::iterator it = configurationMap.find(id);
      if (it != configurationMap.end())
      {
    	  std::string value = (*it).second;
         return value;
      }
   }

   //access to configuration service
   std::string dname = "";

   if (m_bIsBC)
   {
         CPID cpid(id);
         ACS_CS_API_Name CPName("");

         ACS_CS_API_NS::CS_API_Result result = ACS_CS_API_NetworkElement::getDefaultCPName(cpid, CPName);

         switch (result)
         {
         case ACS_CS_API_NS::Result_Success:
         {
        	 char name[100] = {0};
        	 size_t length = CPName.length();
        	 CPName.getName(name, length);
        	 dname = name;
        	 return dname;
         }
         case ACS_CS_API_NS::Result_NoEntry:
            {
               // Check if we are being asked for the name of the CLUSTER cp id
               if (id == CLUSTER_CPid)
               {
                  // return the proper name
                  dname = CLUSTER_name;
                  return dname;
               }
               TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getDefaultCPName() - CS System return Result_NoEntry");
               throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOENTRY, "");
            }
            break;
         case ACS_CS_API_NS::Result_NoValue:
            {
               TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getDefaultCPName() - CS System return Result_NoValue");
               throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOVALUE, "");
            }
            break;
         case ACS_CS_API_NS::Result_NoAccess:
            {
               TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getDefaultCPName() - CS System return Result_NoAccess");
               throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOACCESS, "");
            }
            break;
         case ACS_CS_API_NS::Result_Failure:
            {
               // FIXME - remove once TR HL20758 is fixed.
               // Check if we are being asked for the name of the CLUSTER cp id
               if (id == CLUSTER_CPid)
               {
                  // return the proper name
                  dname = CLUSTER_name;
                  return dname;
               }
               TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getDefaultCPName() - CS System return Result_Failure");
               throw FMS_CPF_Exception (FMS_CPF_Exception::CSOTHERFAILURE, "");
            }
            break;
         }
   }
   return dname;
}

//query for CP Name and Version Table

short FMS_CPF_ConfigReader::cs_getCPID(char * name)
	throw (FMS_CPF_Exception)
{
   //check
   if (name == NULL)
   {
	  TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPID() - the cpname input parameter is NULL");
      return -1;
   }

   //access to configuration map
   if (!configurationMap.empty())
   {
      maptype::iterator it;
      for (it = configurationMap.begin(); it != configurationMap.end(); it++)
      {
    	  std::string value = (*it).second;
         if (ACE_OS::strcmp(value.c_str(), name) == 0)
         {
            short key = (*it).first;
            return key;
         }
      }
   }

   //access to configuration service
   ACS_CS_API_CP * CPNameVer = ACS_CS_API::createCPInstance();
   ACS_CS_API_Name CPName(name);
   CPID cpid;
   ACS_CS_API_NS::CS_API_Result result = CPNameVer->getCPId(CPName, cpid);

   ACS_CS_API::deleteCPInstance(CPNameVer);

//   if (result == ACS_CS_API_NS::Result_Success)
//   {
//      return cpid;
//   }
//   else
//   {
      switch (result)
      {
      case ACS_CS_API_NS::Result_Success:
      {
    	  return cpid;
      }
      case ACS_CS_API_NS::Result_NoEntry:
         {
            // Check if this is the CLUSTER cp name
            // FIXME: needs to be changed if moved to a non-Windows platform
            // function lexicographically compares case-insensitive
        	//if (_strnicmp(name, CLUSTER_name, strlen(CLUSTER_name)) == 0)

        	//function lexicographically compares case-sensitive
        	//if (ACE_OS::strncmp(name, CLUSTER_name, ACE_OS::strlen(CLUSTER_name)) == 0)
        	if (ACE_OS::strcmp(name, CLUSTER_name) == 0)	
            {
        	   TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPID( %s ) - CS System return Result_NoEntry", name);
               // Return CP id for the CLUSTER name
               return CLUSTER_CPid;
            }
        	TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPID( %s ) - CS System return Result_NoEntry", name);
            throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOENTRY, "");
         }
         break;
      case ACS_CS_API_NS::Result_NoValue:
         {
        	TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPID() - CS System return Result_NoValue");
            throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOVALUE, "");
         }
         break;
      case ACS_CS_API_NS::Result_NoAccess:
         {
        	TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPID() - CS System return Result_NoAccess");
            throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOACCESS, "");
         }
         break;
      case ACS_CS_API_NS::Result_Failure:
         {
        	TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPID() - CS System return Result_Failure");
            throw FMS_CPF_Exception (FMS_CPF_Exception::CSOTHERFAILURE, "");
         }
         break;
      }
      return -1;
   //}
}


std::string FMS_CPF_ConfigReader::cs_getCPName(short id)
throw (FMS_CPF_Exception)
{
	std::string cpname;
   CPID cpid(id);
   ACS_CS_API_Name CPName;
   ACS_CS_API_CP * CPNameVer = ACS_CS_API::createCPInstance();
   ACS_CS_API_NS::CS_API_Result result = CPNameVer->getCPName(cpid, CPName);

   ACS_CS_API::deleteCPInstance(CPNameVer);

      switch (result)
      {
      case ACS_CS_API_NS::Result_Success:
            {
            	char str[100] = {0};
            	size_t length = CPName.length();
            	CPName.getName(str, length);
            	cpname = str;
            	return cpname;
            }
      case ACS_CS_API_NS::Result_NoEntry:
         {
        	 TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPName() - CS System return Result_NoEntry");
             throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOENTRY, "");
         }
         break;
      case ACS_CS_API_NS::Result_NoValue:
         {
        	TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPName() - CS System return Result_NoValue");
            throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOVALUE, "");
         }
         break;
      case ACS_CS_API_NS::Result_NoAccess:
         {
        	 TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPName() - CS System return Result_NoAccess");
             throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOACCESS, "");
         }
         break;
      case ACS_CS_API_NS::Result_Failure:
         {
            TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPName() - CS System return Result_Failure");
            throw FMS_CPF_Exception (FMS_CPF_Exception::CSOTHERFAILURE, "");
         }
         break;
      }
      return cpname;
   //}
}


std::list<short> FMS_CPF_ConfigReader::cs_getCPList()
throw (FMS_CPF_Exception)
{
   std::list<short> cplist;
   cplist.clear();

   if (m_bIsBC)
   {
      // retrieve a list, from CS, of all known CP identities
      ACS_CS_API_IdList idlist;
      ACS_CS_API_CP * CPNameVer = ACS_CS_API::createCPInstance();
      ACS_CS_API_NS::CS_API_Result result = CPNameVer->getCPList(idlist);
      ACS_CS_API::deleteCPInstance(CPNameVer);

         switch (result)
         {
         case ACS_CS_API_NS::Result_Success:
         {
        	 int n = idlist.size();
        	 //for (int i = 0; i < idlist.size(); i++)
        	 for (int i = 0; i < n; i++)
        	 {
        	   cplist.push_back(idlist[i]);
        	 }

        	 // add the CP called "CLUSTER" to the list of known CPs
        	 TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPList() - add CP named CLUSTER");
        	 cplist.push_back(short(CLUSTER_CPid));

        	 return cplist;
         }
         case ACS_CS_API_NS::Result_NoEntry:
            {
               TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPList() - CS System return Result_NoEntry");
               throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOENTRY, "");
            }
            break;
         case ACS_CS_API_NS::Result_NoValue:
            {
            	TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPList() - CS System return Result_NoValue");
                throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOVALUE, "");
            }
            break;
         case ACS_CS_API_NS::Result_NoAccess:
            {
            	TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPList() - CS System return Result_NoAccess");
                throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOACCESS, "");
            }
            break;
         case ACS_CS_API_NS::Result_Failure:
            {
               TRACE(fms_cpf_configReader_trace,"%s", "FMS_CPF_ConfigReader::cs_getCPList() - CS System return Result_Failure");
               throw FMS_CPF_Exception (FMS_CPF_Exception::CSOTHERFAILURE, "");
            }
            break;
         }
   }
   return cplist;
}
