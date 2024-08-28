/*
 * * @file fms_cpf_oi_baseobject.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_OI_BaseObject.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_oi_baseobject.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-30
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
 *	| 1.0.0  | 2011-06-30 | qvincon      | File created.                       |
 *	| 1.1.0  | 2011-10-10 | qvincon      | File updated for Infinite file.                       |
 *	+========+============+==============+=====================================+
 */
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_oi_baseobject.h"
#include "fms_cpf_common.h"

#include "fms_cpf_exception.h"

#include "fms_cpf_configreader.h"

#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_parameterhandler.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "acs_apgcc_omhandler.h"
#include "ACS_APGCC_Util.H"

/*============================================================================
	ROUTINE: FMS_CPF_OI_BaseObject
 ============================================================================ */
FMS_CPF_OI_BaseObject::FMS_CPF_OI_BaseObject(FMS_CPF_CmdHandler* cmdHandler, std::string ImmClassName):
m_ImmClassName(ImmClassName),
m_IsMultiCP(false),
m_CmdHandler(cmdHandler)
{
	fms_cpf_oi_baseObjTrace = new (std::nothrow)ACS_TRA_trace("FMS_CPF_OI_BaseObject");

}

/*============================================================================
	ROUTINE: ~FMS_CPF_OI_BaseObject
 ============================================================================ */
FMS_CPF_OI_BaseObject::~FMS_CPF_OI_BaseObject()
{
	m_mapCpNameId.clear();

	if(NULL != fms_cpf_oi_baseObjTrace)
		delete fms_cpf_oi_baseObjTrace;
}

/*============================================================================
	ROUTINE: setSystemParameters
 ============================================================================ */
void  FMS_CPF_OI_BaseObject::setSystemParameters(const bool systemType, const map_CPname_Id& mapConfig )
{
	TRACE(fms_cpf_oi_baseObjTrace,"%s","Entering in setSystemParameters");
	// make a copy of system parameters
	m_IsMultiCP = systemType;
	m_mapCpNameId = mapConfig;

	TRACE(fms_cpf_oi_baseObjTrace,"%s","Leaving setSystemParameters");
}

/*============================================================================
	ROUTINE: getDNbyTag
 ============================================================================ */
bool FMS_CPF_OI_BaseObject::getDNbyTag(const std::string& fullDN, const char* tagOfDN, std::string& outDN)
{
	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Entering in getDNbyTag()");
	bool result = false;
	size_t tagStartPos;
	outDN.clear();
	tagStartPos = fullDN.find(tagOfDN);

	// Check if the tag is present
	if( string::npos != tagStartPos )
	{
		// get the DN
		outDN = fullDN.substr(tagStartPos);

		TRACE(fms_cpf_oi_baseObjTrace, "getDNbyTag(), DN:<%s>", outDN.c_str());
		result= true;
	}
	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Leaving getDNbyTag()");
	return result;
}

/*============================================================================
	ROUTINE: getVolumeName
 ============================================================================ */
bool FMS_CPF_OI_BaseObject::getVolumeName(std::string parentName, std::string& volName)
{
	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Entering in getVolumeName()");

	std::string volumeDN;
	bool result = false;
	volName.clear();

	// get DN of the Cp Volume object ("cpVolumeId=TEMP" or for MCP "cpVolumeId=TEMP:CP1,...")
	if( getDNbyTag(parentName, cpf_imm::VolumeKey, volumeDN) )
	{
		std::string volumeRDN;
		// get RDN of the volume ("TEMP" or for MCP "TEMP:CP1" )
		if(getLastFieldValue(volumeDN, volumeRDN))
		{
			if(m_IsMultiCP)
			{
				size_t tagAtSignPos = volumeRDN.find(parseSymbol::atSign);

				// Check if the tag is present
				if( std::string::npos != tagAtSignPos )
				{
					// get the volumeName ( TEMP )
					volName = volumeRDN.substr(0, tagAtSignPos);
					result = true;
				}
			}
			else
			{
				// RDN is the volume name in SCP
				volName = volumeRDN;
				result = true;
			}
		}
	}
	else
	{
		TRACE(fms_cpf_oi_baseObjTrace, "%s", "getVolumeName(), RDN of cpVolume not found");
	}

	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Leaving getVolumeName()");
	return result;
}

/*============================================================================
	ROUTINE: getInfiniteFileName
 ============================================================================ */
bool FMS_CPF_OI_BaseObject::getInfiniteFileName(std::string parentName, std::string& infiniteFileName)
{
	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Entering in getInfiniteFilename()");

	std::string objectDN;
	bool result = false;
	infiniteFileName = "";

	// get DN of a infinite file
	if( getDNbyTag(parentName, cpf_imm::InfiniteFileKey, objectDN) )
	{
		result = getLastFieldValue(objectDN, infiniteFileName);
	}
	else
	{
		TRACE(fms_cpf_oi_baseObjTrace, "%s", "getInfiniteFilename(), RDN of infiniteFile not found");
	}

	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Leaving getInfiniteFilename()");
	return result;
}

/*============================================================================
	ROUTINE: getSubInfiniteFileName
 ============================================================================ */
bool FMS_CPF_OI_BaseObject::getSubInfiniteFileName(std::string parentName, std::string& infiniteSubFileName)
{
	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Entering in getSubInfiniteFileName()");

	bool result = false;
	std::string field, rdn;
	size_t equalPos, commaPos;
	bool search = true;

	// search the cpVolumeId field in the parent string
	while(search)
	{
		// the fields are separated by comma character
		commaPos = parentName.find_first_of(parseSymbol::comma);

		// Check if it is the last field
		if(string::npos != commaPos )
		{
			// Get the field
			field = parentName.substr(0, commaPos);
			// Remove this field from the parent string
			parentName = parentName.substr(commaPos + 1, parentName.length() - commaPos);
		}
		else
		{
			field = parentName;
			// Reached the last field
			search = false;
		}

		// Split the field in RDN and Value
		equalPos = field.find_first_of(parseSymbol::equal);
		rdn = field.substr(0, equalPos);

		//Check if it is the compositeFileId field
		if(rdn.compare(cpf_imm::InfiniteSubFileKey) == 0)
		{
			// get the value of compositeFileId
			infiniteSubFileName = field.substr(equalPos + 1);

			// stop the search
			search= false;
			result = true;
			TRACE(fms_cpf_oi_baseObjTrace, "getSubInfiniteFileName(), subInfiniteFileName = %s", infiniteSubFileName.c_str() );
		}
	}

	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Leaving getSubInfiniteFileName()");
	return result;
}


/*============================================================================
	ROUTINE: getCompositeFilename
 ============================================================================ */
bool FMS_CPF_OI_BaseObject::getCompositeFileName(std::string parentName, std::string& compositeFileName)
{
	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Entering in getCompositeFilename()");

	std::string objectDN;
	bool result = false;
	compositeFileName.clear();

	// get DN of a composite file
	if( getDNbyTag(parentName, cpf_imm::CompositeFileKey, objectDN) )
	{
		result =  getLastFieldValue(objectDN, compositeFileName);
	}
	else
	{
		TRACE(fms_cpf_oi_baseObjTrace, "%s", "getCompositeFileName(), RDN of compositeFile not found");
	}

	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Leaving getCompositeFilename()");
	return result;
}


/*============================================================================
	ROUTINE: getSubCompositeFileName
 ============================================================================ */
bool FMS_CPF_OI_BaseObject::getSubCompositeFileName(std::string objDN, std::string& subCompositeFileName)
{
	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Entering in getSubCompositeFileName()");

	std::string objectDN;
	bool result = false;
	subCompositeFileName = "";

	// get DN of a composite file
	if( getDNbyTag(objDN, cpf_imm::CompositeSubFileKey, objectDN) )
	{
		result = getLastFieldValue(objectDN, subCompositeFileName);
	}
	else
	{
		TRACE(fms_cpf_oi_baseObjTrace, "%s", "getSubCompositeFileName(), RDN of compositeFile not found");
	}

	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Leaving getSubCompositeFileName()");
	return result;
}

/*============================================================================
	ROUTINE: getSimpleFileName
 ============================================================================ */
bool FMS_CPF_OI_BaseObject::getSimpleFileName(std::string objDN, std::string& simpleFileName)
{
	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Entering in getSimpleFileName()");

	std::string objectDN;
	bool result = false;
	simpleFileName.clear();

	// get DN of a simple file
	if( getDNbyTag(objDN, cpf_imm::SimpleFileKey, objectDN) )
	{
		result =getLastFieldValue(objectDN, simpleFileName);
	}
	else
	{
		TRACE(fms_cpf_oi_baseObjTrace, "%s", "getSimpleFileName(), RDN of simpleFile not found");
	}

	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Leaving getSimpleFileName()");
	return result;
}

/*============================================================================
	ROUTINE: getLastFieldValue
 ============================================================================ */
bool FMS_CPF_OI_BaseObject::getLastFieldValue(const std::string& objDN, std::string& value, bool toUpperValue)
{
	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Entering in getLastFieldValue()");
	bool result = false;

	value.clear();
	// Get the file name from DN
	// Split the field in RDN and Value
	size_t equalPos = objDN.find_first_of(parseSymbol::equal);
	size_t commaPos = objDN.find_first_of(parseSymbol::comma);

	// Check if some error happens
	if( (std::string::npos != equalPos) )
	{
		// check for a single field case
		if( std::string::npos == commaPos )
			value = objDN.substr(equalPos + 1);
		else
			value = objDN.substr(equalPos + 1, (commaPos - equalPos - 1) );

		if(toUpperValue)
		{
			// make the value in upper case
			ACS_APGCC::toUpper(value);
		}

		result = true;
	}

	TRACE(fms_cpf_oi_baseObjTrace, "Leaving getLastFieldValue(), value:<%s>", value.c_str());
	return result;
}

/*============================================================================
	ROUTINE: getCpName
 ============================================================================ */
bool FMS_CPF_OI_BaseObject::getCpName(std::string parentName, std::string& cpName)
{
	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Entering in getCpName()");

	bool result = false;
	cpName.clear();

	// Check the system type
	if(m_IsMultiCP)
	{
		std::string volumeDN;

		// get DN of the volume (cpVolumeId=TEMP:CP1,...)
		if( getDNbyTag(parentName, cpf_imm::VolumeKey, volumeDN) )
		{
			std::string volumeRDN;
			// get RDN of the volume ( TEMP:CP1 )
			if(getLastFieldValue(volumeDN, volumeRDN))
			{
				size_t tagAtSignPos = volumeRDN.find(parseSymbol::atSign);

				// Check if the tag is present
				if( std::string::npos != tagAtSignPos )
				{
					// get the cpName (CP1)
					cpName = volumeRDN.substr(tagAtSignPos + 1);
					result = true;
				}
			}
		}
		else
		{
			TRACE(fms_cpf_oi_baseObjTrace, "%s", "getCpName(), RDN of cpVolume not found");
		}
	}
	else
	{
		// SCP the CpName is an empty string
		result = true;
	}

	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Leaving getCpName()");
	return result;
}

/*============================================================================
	ROUTINE: getStringAttributeValue
 ============================================================================ */
bool FMS_CPF_OI_BaseObject::getStringAttributeValue(const std::string& objectDN, char* attributeName, std::string& attributeValue)
{
	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Entering in getStringAttributeValue()");

	bool result = false;
	attributeValue.clear();

	OmHandler objManager;
	ACS_CC_ImmParameter objectAttribute;
	objectAttribute.attrName = attributeName;

	// Init OM resource
	if (objManager.Init() != ACS_CC_FAILURE)
	{
		if (objManager.getAttribute(objectDN.c_str(), &objectAttribute ) != ACS_CC_FAILURE )
		{
			if(objectAttribute.attrValuesNum != 0)
			{
				// Attribute found and get it
				result = true;
				attributeValue = ((reinterpret_cast<char*>(*(objectAttribute.attrValues))));
				ACS_APGCC::toUpper(attributeValue);
				TRACE(fms_cpf_oi_baseObjTrace, "getStringAttributeValue(), Attribute =%s", attributeValue.c_str() );
			}
		}
		else
		{
			TRACE(fms_cpf_oi_baseObjTrace, "%s", "getStringAttributeValue(), OM getAttribute error");
		}
		// Deallocate OM resource
		objManager.Finalize();
	}
	else
	{
		TRACE(fms_cpf_oi_baseObjTrace, "%s", "getStringAttributeValue(), OM handler init error");
	}

	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Leaving getStringAttributeValue()");
	return result;
}

/*============================================================================
	ROUTINE: getCpId
 ============================================================================ */
short FMS_CPF_OI_BaseObject::getCpId(const std::string& cpName)
{
	map_CPname_Id::iterator idx;
	// find the element in the map
	idx = m_mapCpNameId.find(cpName);
	// checks if found
	if(m_mapCpNameId.end() != idx)
	{
		return idx->second;
	}
	return -1;
}

/*============================================================================
	ROUTINE: checkCpName
 ============================================================================ */
int FMS_CPF_OI_BaseObject::checkCpName(const std::string& cpName)
{
	TRACE(fms_cpf_oi_baseObjTrace, "[%s@%d] - In", __FILE__, __LINE__);
	int result = SUCCESS;

	// Check the system type
    if( m_IsMultiCP )
    {
    	TRACE(fms_cpf_oi_baseObjTrace, "[%s@%d] Multi CP System - CP name= %s", __FILE__, __LINE__, cpName.c_str());
    	map_CPname_Id::iterator idx;

    	idx = m_mapCpNameId.find(cpName);

    	// check if cp name is defined
    	if( m_mapCpNameId.end() == idx )
    	{
    		TRACE(fms_cpf_oi_baseObjTrace, "[%s@%d] CP name= %s NOT found in m_mapCpNameId", __FILE__, __LINE__, cpName.c_str());
    		result = FMS_CPF_Exception::CPNOTEXISTS;
    		FMS_CPF_ConfigReader currentConfiguration;
    		try
    		{
    			currentConfiguration.init();
    			short cpid = currentConfiguration.cs_getCPID(const_cast<char *>(cpName.c_str()));

    			std::string defaultCpName = currentConfiguration.cs_getDefaultCPName(cpid);

    			//Update the internal file table
    			if ( DirectoryStructureMgr::instance()->refreshCpFileTable(defaultCpName) )
    			{
    				//Update the CP ID table
    				m_mapCpNameId.insert(pair<std::string,short>(defaultCpName, cpid));
    				ParameterHndl::instance()->loadMapDataDiskPathForCP(cpid);
    				ParameterHndl::instance()->loadMapLogDiskPathForCP(cpid);
    				result = SUCCESS;
    			}
    			else
    			{
    				//Error
    				TRACE(fms_cpf_oi_baseObjTrace, "[%s@%d] Error: Cannot refresh internal CP file table!!!",  __FILE__, __LINE__);
    			}

    		}
    		catch (FMS_CPF_Exception &ex)
    		{
    			TRACE(fms_cpf_oi_baseObjTrace, "[%s@%d] Error: Cannot read new configuration. Error Text: %s !!!", __FILE__, __LINE__, ex.errorText());
    		}
    	}
    	else
    	{
    		TRACE(fms_cpf_oi_baseObjTrace, "[%s@%d] CP name= %s FOUND in m_mapCpNameId", __FILE__, __LINE__, cpName.c_str());
    	}
    }
    else
    {

    	TRACE(fms_cpf_oi_baseObjTrace, "[%s@%d] Single CP System", __FILE__, __LINE__);
    	// check if the received Cp Name is empty in a SCP system
    	if( !cpName.empty() )
    	{
    		TRACE(fms_cpf_oi_baseObjTrace, "[%s@%d] Error: Received CP Name in a SCP. Illegal Option!!!", __FILE__, __LINE__);
    		result = FMS_CPF_Exception::ILLOPTION;
    	}
    }

	TRACE(fms_cpf_oi_baseObjTrace, "[%s@%d] - Out", __FILE__, __LINE__);
	return result;
}

/*============================================================================
	ROUTINE: checkRecordLength
 ============================================================================ */
bool FMS_CPF_OI_BaseObject::checkRecordLength(const int& recordLength)
{
	TRACE(fms_cpf_oi_baseObjTrace,"%s","Entering in checkRecordLength()");

	bool result = true;
	// Check the record length range
	if( (recordLength < stdValue::MIN_RECORDLENGTH) ||
			(recordLength > stdValue::MAX_RECORDLENGTH) )
	{
		result = false;
		TRACE(fms_cpf_oi_baseObjTrace,"%s","checkRecordLength(), record length not valid");
	}

	TRACE(fms_cpf_oi_baseObjTrace,"%s","Leaving checkRecordLength()");
	return result;
}

/*============================================================================
	ROUTINE: getNumberOfChildren
 ============================================================================ */
long FMS_CPF_OI_BaseObject::getNumberOfChildren(const char* objectDN)
{
	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Entering in getNumberOfChildren()");

	long numOfChildren = -1;
	OmHandler objManager;

	// Init OM resource
	if(objManager.Init() != ACS_CC_FAILURE)
	{
		std::vector<std::string> childrenRDNList;

		if(objManager.getChildren(objectDN, ACS_APGCC_SUBLEVEL, &childrenRDNList) != ACS_CC_FAILURE )
		{
			size_t numOfElement = childrenRDNList.size();
			numOfChildren = static_cast<long>(numOfElement);
			TRACE(fms_cpf_oi_baseObjTrace, "getNumberOfChildren(), found <%d> children", numOfChildren);
		}
		else
		{
			TRACE(fms_cpf_oi_baseObjTrace, "%s", "getNumberOfChildren(), OM getChildren error");
		}
		// Deallocate OM resource
		objManager.Finalize();
	}
	else
	{
		TRACE(fms_cpf_oi_baseObjTrace, "%s", "getNumberOfChildren(), OM handler init error");
	}
	TRACE(fms_cpf_oi_baseObjTrace, "%s", "Leaving getNumberOfChildren()");

	return numOfChildren;
}

/*============================================================================
	ROUTINE: checkFileName
 ============================================================================ */
bool FMS_CPF_OI_BaseObject::checkFileName(const std::string& fName)
{
	TRACE(fms_cpf_oi_baseObjTrace,"%s","Entering in checkFileName()");
	bool result = false;
	size_t found;
	unsigned int fLength = (unsigned int) fName.length();

	TRACE(fms_cpf_oi_baseObjTrace,"checkFileName(), file name= %s", fName.c_str());

	// check the length and the first character
	if( !fName.empty() && (fLength <= stdValue::FILE_LENGTH) && (fName[0] >= stdValue::minStartChr) && (fName[0] <= stdValue::maxStartChr ) )
	{
		// check if there is some not allowed character
		found = fName.find_first_not_of(stdValue::IDENTIFIER);

		if( string::npos == found )
			result = true;
		else
		  TRACE(fms_cpf_oi_baseObjTrace,"%s","checkFileName(), invalid character in file name");

	}
	TRACE(fms_cpf_oi_baseObjTrace,"%s","Leaving checkFileName()");
	return result;
}

/*============================================================================
	ROUTINE: checkFileName
 ============================================================================ */
bool FMS_CPF_OI_BaseObject::checkSubFileName(const std::string& fName)
{
	TRACE(fms_cpf_oi_baseObjTrace,"%s","Entering in checkSubFileName()");
	bool result = true;
	size_t found;
	size_t minusPos;
	std::string namePart;

	// search '-' in the name
	minusPos = fName.find_first_of(parseSymbol::minus);

	// Check if the generator part is present
	if( string::npos != minusPos )
	{
		// subfile with generator : SUBFILE-GEN
		std::string genPart;
		// Split subfile name and gen part
		namePart = fName.substr(0, minusPos);
		genPart = fName.substr(minusPos + 1);
		// validate the gen part
		// check if there is some not allowed character
		found = genPart.find_first_not_of(stdValue::IDENTIFIER);
		if( string::npos != found || genPart.length() > stdValue::GEN_LENGTH)
		{
			result = false;
			TRACE(fms_cpf_oi_baseObjTrace,"checkSubFileName(), generator part not valid, gen=%s", genPart.c_str());
			return result;
		}
	}
	else
	{
		// subfile without generator part
		namePart = fName;
	}

	// check the name part
	found = namePart.find_first_not_of(stdValue::IDENTIFIER);
	if( string::npos != found || namePart.empty() || namePart.length() > stdValue::FILE_LENGTH)
	{
		result = false;
		TRACE(fms_cpf_oi_baseObjTrace,"checkSubFileName(), subfile name not valid, gen=%s", namePart.c_str());
	}

	TRACE(fms_cpf_oi_baseObjTrace,"%s","Leaving checkSubFileName()");
	return result;
}
/*============================================================================
	ROUTINE: checkVolumeName
 ============================================================================ */
bool FMS_CPF_OI_BaseObject::checkVolumeName(const std::string& vName)
{
	TRACE(fms_cpf_oi_baseObjTrace,"%s","Entering in checkVolumeName()");
	bool result = false;
	size_t found;
	unsigned int vLength = (unsigned int) vName.length();

	TRACE(fms_cpf_oi_baseObjTrace,"checkVolumeName(), volume name= %s", vName.c_str());

	// check the length and the first character
	if( (vLength >= stdValue::MINVOL_LENGTH) && (vLength <= stdValue::VOL_LENGTH) &&
			(vName[0] >= stdValue::minStartChr) && (vName[0] <= stdValue::maxStartChr ) )
	{
		// check if there is some not allowed character
		found = vName.find_first_not_of(stdValue::VOL_IDENTIFIER);

		if( string::npos == found )
			result = true;
		else
		  TRACE(fms_cpf_oi_baseObjTrace,"%s","checkVolumeName(), invalid character in volume name");

	}
	TRACE(fms_cpf_oi_baseObjTrace,"%s","Leaving checkVolumeName()");
	return result;
}
