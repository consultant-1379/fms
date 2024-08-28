/*
 * * @file fms_cpf_oi_cpvolume.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_OI_CpVolume.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_oi_cpvolume.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-15
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
 *	| 1.0.0  | 2011-06-15 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_oi_cpvolume.h"
#include "fms_cpf_common.h"
#include "fms_cpf_createvolume.h"
#include "fms_cpf_removevolume.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"

#include <sstream>
#include <boost/format.hpp>

extern ACS_TRA_Logging CPF_Log;

namespace volumeClass {
	const char ImmImplementerName[] = "CPF_OI_CpVolume";
}

/*============================================================================
	ROUTINE: FMS_CPF_OI_CpVolume
 ============================================================================ */
FMS_CPF_OI_CpVolume::FMS_CPF_OI_CpVolume( FMS_CPF_CmdHandler* cmdHandler) : FMS_CPF_OI_BaseObject(cmdHandler, cpf_imm::VolumeClassName) ,
	acs_apgcc_objectimplementerinterface_V3(volumeClass::ImmImplementerName)
{
	fms_cpf_oi_cpvolumeTrace = new ACS_TRA_trace("FMS_CPF_OI_CpVolume");
}

/*============================================================================
	ROUTINE: ~FMS_CPF_OI_CpVolume
 ============================================================================ */
FMS_CPF_OI_CpVolume::~FMS_CPF_OI_CpVolume()
{
	m_volumeOperationTable.clear();

	if(NULL != fms_cpf_oi_cpvolumeTrace)
		delete fms_cpf_oi_cpvolumeTrace;
}

/*============================================================================
	ROUTINE: create
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CpVolume::create(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *className, const char* parentname, ACS_APGCC_AttrValues **attr)
{
	TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "Entering in create(...) callback");

	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	ACS_CC_ReturnType result = ACS_CC_FAILURE;

	// Define volume data structure
	cpVolumeInfo newVolumeInfo;
	newVolumeInfo.action = Create;

	TRACE(fms_cpf_oi_cpvolumeTrace, "create(...), insert a volume under parent=<%s>", parentname);

	// Check the IMM class name
	if( 0 == ACE_OS::strcmp(m_ImmClassName.c_str(), className ) )
	{
		// extract the volume RDN
		for(size_t idx = 0; attr[idx] != NULL; ++idx)
		{
			// check if RDN attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::VolumeKey, attr[idx]->attrName) )
			{
				// attribute found
				result = ACS_CC_SUCCESS;

				// it will be like cpVolumeId=XYZ or cpVolumeId=XYZ:BC1 into MCP system
				std::string volumeRDN = reinterpret_cast<char*>(attr[idx]->attrValues[0]);

				std::string objectName;
				// get the objectName from RDN
				getLastFieldValue(volumeRDN, objectName, false);

				// get the volumeName
				getVolumeName(volumeRDN, newVolumeInfo.volumeName);

				if( !checkVolumeName(newVolumeInfo.volumeName) )
				{
					// Invalid volume name
					result = ACS_CC_FAILURE;

					// Assemble the formated error message
					boost::format errorMsg(errorText::invalidVolumeName);
					errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );
					errorMsg % cpf_imm::VolumeObjectName % objectName;

					setExitCode(errorText::errorValue, errorMsg.str());

					TRACE(fms_cpf_oi_cpvolumeTrace, "create(...), volume name<%s> not valid", newVolumeInfo.volumeName.c_str());
					break;
				}

				if(m_IsMultiCP)
				{
					// MCP system, get the Cp name
					getCpName(volumeRDN, newVolumeInfo.cpName);

					// Validated  the cp Name
					int checkResult = checkCpName(newVolumeInfo.cpName);
					if( SUCCESS != checkResult )
					{
						result = ACS_CC_FAILURE;
						FMS_CPF_Exception exErr(static_cast<FMS_CPF_Exception::errorType>(checkResult) );
						setExitCode(checkResult, exErr.errorText());

						TRACE(fms_cpf_oi_cpvolumeTrace, "create(...), cp name<%s> not valid", newVolumeInfo.cpName.c_str());
						break;
					}
				}
				break;
			}
		}

		if( ACS_CC_SUCCESS == result )
		{
			TRACE(fms_cpf_oi_cpvolumeTrace,"%s", "create(...), create a new volume");
			newVolumeInfo.completed = false;
			m_volumeOperationTable.insert(pair<ACS_APGCC_CcbId,cpVolumeInfo>(ccbId, newVolumeInfo));
		}
	}
	else
	{
		CPF_Log.Write("FMS_CPF_OI_CpVolume::create, error call on not implemented class", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "create(...), error call on not implemented class");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());
	}

	TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "Leaving create(...)");
	return result;
}

/*============================================================================
	ROUTINE: deleted
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CpVolume::deleted(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName)
{
	TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "Entering in deleted(...)");
	ACS_CC_ReturnType result = ACS_CC_FAILURE;

	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	long numOfChildren = getNumberOfChildren(objName);
	std::string errorDetail;

	// Check if empty volume
	if(0 == numOfChildren)
	{
		cpVolumeInfo delVolumeInfo;
		delVolumeInfo.action = Delete;
		delVolumeInfo.completed = false;

		if(getVolumeName(objName, delVolumeInfo.volumeName) &&
				getCpName(objName, delVolumeInfo.cpName) )
		{
			result = ACS_CC_SUCCESS;
			m_volumeOperationTable.insert(pair<ACS_APGCC_CcbId, cpVolumeInfo>(ccbId, delVolumeInfo));
			TRACE(fms_cpf_oi_cpvolumeTrace, "deleted(...), delete empty volume:<%s>, CP name:<%s>", delVolumeInfo.volumeName.c_str(), delVolumeInfo.cpName.c_str());
		}
		else
		{
			TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "deleted(...), error on get volume info");
			FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
			setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());
		}
	}
	else
	{
		setExitCode(errorText::errorValue, errorText::volumeDelete);
		TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "deleted(...), error volume not empty");
	}
	TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "Leaving deleted(...)");
	return result;
}

/*============================================================================
	ROUTINE: modify
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CpVolume::modify(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName, ACS_APGCC_AttrModification **attrMods)
{
	TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "Entering in modify(...)");
	ACS_CC_ReturnType result = ACS_CC_FAILURE;
	// To avoid warning about unused parameter
	UNUSED(oiHandle);
	UNUSED(ccbId);
	UNUSED(objName);
	UNUSED(attrMods);

	TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "Leaving modify(...)");
	return result;
}

/*============================================================================
	ROUTINE: complete
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CpVolume::complete(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "Entering complete(...)");
	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	ACS_CC_ReturnType result = ACS_CC_SUCCESS;

	FMS_CPF_Exception::errorType cmdExeResult;
	std::string errorDetail;

	// find all operations related to the ccbid
	std::pair<operationTable::iterator, operationTable::iterator> operationRange;
	operationRange = m_volumeOperationTable.equal_range(ccbId);

	//for each operation found
	operationTable::iterator element;
	for(element = operationRange.first; (element != operationRange.second) && (result == ACS_CC_SUCCESS); ++element)
	{
		ACE_Future<FMS_CPF_PrivateException> waitResult;
		FMS_CPF_PrivateException operationResult;

		switch(element->second.action)
		{
			case Create :
			{
				CPF_CreateVolume_Request::systemData volInfo;

				// set the volume name
				volInfo.volumeName = element->second.volumeName;
				volInfo.isMultiCP = m_IsMultiCP;

				// checks the system type
				if(m_IsMultiCP)
				{
					// Set the CP Name
					volInfo.cpName = element->second.cpName;
				}
				TRACE(fms_cpf_oi_cpvolumeTrace, "%s","complete(), enqueue the command to the CMD Handler");

				// Create the cmd request
				CPF_CreateVolume_Request* createVolume= new CPF_CreateVolume_Request(waitResult,volInfo);

				m_CmdHandler->enqueue(createVolume);

				TRACE(fms_cpf_oi_cpvolumeTrace, "%s","complete(), wait cmd execution");
				waitResult.get(operationResult);

				cmdExeResult = operationResult.errorCode();

				// checks the cmd execution result
				if(FMS_CPF_Exception::OK != cmdExeResult )
				{
					result = ACS_CC_FAILURE;
					errorDetail = operationResult.errorText();
					errorDetail += operationResult.detailInfo();
					char msgLog[512] = {0};
					ACE_OS::snprintf(msgLog, 511, "FMS_CPF_OI_CpVolume::complete(), error:<%d> on create volume:<%s>", cmdExeResult, volInfo.volumeName.c_str());
					CPF_Log.Write(msgLog, LOG_LEVEL_ERROR);

					TRACE(fms_cpf_oi_cpvolumeTrace, "%s", msgLog);
				}
				else
				{
					// Volume has been created
					element->second.completed = true;
				}
			}
			break;

			case Delete :
			{
				CPF_RemoveVolume_Request::systemData volInfo;

				// set the volume name
				volInfo.volumeName = element->second.volumeName;
				volInfo.isMultiCP = m_IsMultiCP;

				// checks the system type
				if(m_IsMultiCP)
				{
					// Set the CP Name
					volInfo.cpName = element->second.cpName;
				}
				TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "complete(), enqueue the command to the CMD Handler");

				// Create the cmd request
				CPF_RemoveVolume_Request* removeVolume= new CPF_RemoveVolume_Request(waitResult,volInfo);

				m_CmdHandler->enqueue(removeVolume);

				TRACE(fms_cpf_oi_cpvolumeTrace, "%s","complete(), wait cmd execution");
				waitResult.get(operationResult);

				cmdExeResult = operationResult.errorCode();

				// checks the cmd execution result
				if(FMS_CPF_Exception::OK != cmdExeResult )
				{
					result = ACS_CC_FAILURE;
					errorDetail = operationResult.errorText();
					errorDetail += operationResult.detailInfo();
					char msgLog[512] = {0};
					ACE_OS::snprintf(msgLog, 511, "FMS_CPF_OI_CpVolume::complete(), error:<%d> on delete volume:<%s>", cmdExeResult, volInfo.volumeName.c_str());
					CPF_Log.Write(msgLog, LOG_LEVEL_ERROR);
					TRACE(fms_cpf_oi_cpvolumeTrace, "%s", msgLog);
				}
				else
				{
					// Volume has been removed
					element->second.completed = true;
				}
			}
			break;

			case Modify :
			{
				TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "complete(), modify request not enabled");
			}
			break;

			default :
				TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "complete(), volume IMM action unknown");

		}
	}

	// set the exit code to the caller
	setExitCode(static_cast<int>(cmdExeResult), errorDetail);

	TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "Leaving complete(...)");
	return result;
}

/*============================================================================
	ROUTINE: abort
 ============================================================================ */
void FMS_CPF_OI_CpVolume::abort(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "Entering abort(...)");

	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	// find all operations related to the ccbid
	std::pair<operationTable::iterator, operationTable::iterator> operationRange;
	operationRange = m_volumeOperationTable.equal_range(ccbId);

	FMS_CPF_Exception::errorType cmdExeResult;
	//for each operation found
	operationTable::iterator element;
	for(element = operationRange.first; (element != operationRange.second); ++element)
	{
		ACE_Future<FMS_CPF_PrivateException> waitResult;
		FMS_CPF_PrivateException operationResult;

		// Check if the operation is already done
		if(element->second.completed)
		{
			switch(element->second.action)
			{
				case Delete :
				{
					CPF_CreateVolume_Request::systemData volInfo;

					// set the volume name
					volInfo.volumeName = element->second.volumeName;
					volInfo.isMultiCP = m_IsMultiCP;

					// checks the system type
					if(m_IsMultiCP)
					{
						// Set the CP Name
						volInfo.cpName = element->second.cpName;
					}
					TRACE(fms_cpf_oi_cpvolumeTrace, "%s","abort(), enqueue create command to the CMD Handler");

					// Create the cmd request
					CPF_CreateVolume_Request* createVolume= new CPF_CreateVolume_Request(waitResult,volInfo);

					m_CmdHandler->enqueue(createVolume);

					TRACE(fms_cpf_oi_cpvolumeTrace, "%s","abort(), wait cmd execution");
					waitResult.get(operationResult);

					cmdExeResult = operationResult.errorCode();

					// checks the cmd execution result
					if(FMS_CPF_Exception::OK != cmdExeResult )
					{
						char msgLog[512] = {0};
						ACE_OS::snprintf(msgLog, 511, "FMS_CPF_OI_CpVolume::abort(), error:<%d> on volume:<%s> delete undo", cmdExeResult, volInfo.volumeName.c_str());
						CPF_Log.Write(msgLog, LOG_LEVEL_ERROR);
						TRACE(fms_cpf_oi_cpvolumeTrace, "%s", msgLog);
					}
				}
				break;

				case Create :
				{
					CPF_RemoveVolume_Request::systemData volInfo;

					// set the volume name
					volInfo.volumeName = element->second.volumeName;
					volInfo.isMultiCP = m_IsMultiCP;

					// checks the system type
					if(m_IsMultiCP)
					{
						// Set the CP Name
						volInfo.cpName = element->second.cpName;
					}
					TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "complete(), enqueue the command to the CMD Handler");

					// Create the cmd request
					CPF_RemoveVolume_Request* removeVolume= new CPF_RemoveVolume_Request(waitResult,volInfo);

					m_CmdHandler->enqueue(removeVolume);

					TRACE(fms_cpf_oi_cpvolumeTrace, "%s","complete(), wait cmd execution");
					waitResult.get(operationResult);

					cmdExeResult = operationResult.errorCode();

					// checks the cmd execution result
					if(FMS_CPF_Exception::OK != cmdExeResult )
					{
						char msgLog[512] = {0};
						ACE_OS::snprintf(msgLog, 511, "FMS_CPF_OI_CpVolume::abort(), error:<%d> on volume:<%s> create undo", cmdExeResult, volInfo.volumeName.c_str());
						CPF_Log.Write(msgLog, LOG_LEVEL_ERROR);
						TRACE(fms_cpf_oi_cpvolumeTrace, "%s", msgLog);
					}
				}
				break;

				default:
					TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "complete(), volume IMM action unknown");
			}

			TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "abort(...), operation completed and removed from table");
		}
	}

	// Erase all elements from the table of the operations
	m_volumeOperationTable.erase(operationRange.first, operationRange.second);

	TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "Leaving abort(...)");
}

/*============================================================================
ROUTINE: apply
============================================================================ */
void FMS_CPF_OI_CpVolume::apply(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "Entering apply(...)");
	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	// find all operations related to the ccbid
	std::pair<operationTable::iterator, operationTable::iterator> operationRange;
	operationRange = m_volumeOperationTable.equal_range(ccbId);

	// Erase all elements from the table of the operations
	m_volumeOperationTable.erase(operationRange.first, operationRange.second);

	TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "Leaving apply(...)");
}

/*============================================================================
ROUTINE: updateRuntime
============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CpVolume::updateRuntime(const char* p_objName, const char** p_attrName)
{
	ACS_CC_ReturnType result = ACS_CC_FAILURE;
	// To avoid warning about unused parameter
	UNUSED(p_objName);
	UNUSED(p_attrName);
	return result;
}

/*============================================================================
ROUTINE: adminOperationCallback
============================================================================ */
void FMS_CPF_OI_CpVolume::adminOperationCallback(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_InvocationType invocation,
		const char* p_objName, ACS_APGCC_AdminOperationIdType operationId,
		ACS_APGCC_AdminOperationParamType** paramList)
{
	TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "Entering adminOperationCallback(...)");
	// To avoid warning about unused parameter
	UNUSED(p_objName);
	UNUSED(operationId);
	UNUSED(paramList);
	// No actions are defined in CompositeFile class
	adminOperationResult(oiHandle, invocation, actionResult::NOOPERATION);
	TRACE(fms_cpf_oi_cpvolumeTrace, "%s", "Leaving adminOperationCallback(...)");
}
