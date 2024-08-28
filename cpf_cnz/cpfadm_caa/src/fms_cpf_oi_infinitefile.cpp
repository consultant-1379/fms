/*
 * * @file fms_cpf_oi_compositefile.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_OI_CompositeFile.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_oi_infinitefile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-10-10
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
 *	| 1.0.0  | 2011-10-10 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 *	| 1.1.0  | 2014-04-30 | qvincon      | Infinite subfiles removed from IMM  |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_oi_infinitefile.h"
#include "fms_cpf_createfile.h"
#include "fms_cpf_deletefile.h"
#include "fms_cpf_datafile.h"
#include "fms_cpf_movefile.h"
#include "fms_cpf_changefileattribute.h"
#include "fms_cpf_releasesubfile.h"
#include "fms_cpf_common.h"
#include "fms_cpf_tqchecker.h"

#include "fms_cpf_privateexception.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"

#include <boost/format.hpp>

#include <sstream>
#include <list>

#include <ace/Condition_T.h>
bool FMS_CPF_OI_InfiniteFile::isMoveCpFileOp(false);
void FMS_CPF_OI_InfiniteFile::setMoveCpFileOpStatus(bool val)
{
       isMoveCpFileOp = val;
}


ACE_Thread_Mutex mutex_lock;
ACE_Condition<ACE_Thread_Mutex> cond_var(mutex_lock);

extern ACS_TRA_Logging CPF_Log;

namespace infiniteFileClass{
    const char ImmImplementerName[] = "CPF_OI_InfiniteFile";
}

// Range of MaxTime parameter
namespace TimeRange
{
	const unsigned short MinValue = 0;
	const unsigned int MaxValue = 9999999;
}
// Range of MaxSize parameter
namespace SizeRange
{
	const unsigned short MinValue = 0;
	const unsigned int MaxValue = 9999999;
}


/*============================================================================
	ROUTINE: FMS_CPF_OI_InfiniteFile
 ============================================================================ */
FMS_CPF_OI_InfiniteFile::FMS_CPF_OI_InfiniteFile(FMS_CPF_CmdHandler* cmdHandler) :
		FMS_CPF_OI_BaseObject(cmdHandler, cpf_imm::InfiniteFileClassName),
		acs_apgcc_objectimplementerinterface_V3(infiniteFileClass::ImmImplementerName)
{
		fms_cpf_oi_infFileTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_OI_InfiniteFile");
}

/*============================================================================
	ROUTINE: create
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_InfiniteFile::create(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *className, const char* parentname, ACS_APGCC_AttrValues **attr)
{
	TRACE(fms_cpf_oi_infFileTrace, "%s", "Entering in create(...) callback");

	ACS_CC_ReturnType result = ACS_CC_SUCCESS;
	// To avoid warning about unused parameter
	UNUSED(oiHandle);
	TRACE(fms_cpf_oi_infFileTrace, "create(...), insert a infinite file under parent=<%s>", parentname);

	infiniteFileInfo newInfFileInfo;

	//initiate values
	newInfFileInfo.compression = false;
	newInfFileInfo.recordLength = 0;
	newInfFileInfo.fileName = "";
	newInfFileInfo.maxSize = 0;
	newInfFileInfo.maxTime = 0;
	newInfFileInfo.releaseCondition = false;

	newInfFileInfo.completed = false;
	newInfFileInfo.action = Create;

	// Get volume and cpName by the parent DN
	if(!getVolumeName(parentname, newInfFileInfo.volumeName) ||
			!getCpName(parentname, newInfFileInfo.cpName)	)
	{
		CPF_Log.Write("FMS_CPF_OI_InfiniteFile::create, error on get parent info", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_oi_infFileTrace, "%s", "create(...), error on get parent info");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());

		result = ACS_CC_FAILURE;
		return result;
	}

	// Check the IMM class name
	if( 0 == ACE_OS::strcmp(m_ImmClassName.c_str(), className) )
	{
		std::string objectName;
		// extract the attributes
		for(size_t idx = 0; attr[idx] != NULL ; idx++)
		{
			// check if RDN attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::InfiniteFileKey, attr[idx]->attrName) )
			{
				std::string fileRDN = reinterpret_cast<char *>(attr[idx]->attrValues[0]);

				newInfFileInfo.fileDN = fileRDN + parseSymbol::comma + std::string(parentname);

				// get the fileName from RDN
				getLastFieldValue(fileRDN, newInfFileInfo.fileName, false);

				objectName = newInfFileInfo.fileName;

				ACS_APGCC::toUpper(newInfFileInfo.fileName);
				// Validate the value
				if(!checkFileName(newInfFileInfo.fileName) )
				{
					result = ACS_CC_FAILURE;
					// Assemble the formated error message
					boost::format errorMsg(errorText::invalidFileName);
					errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

					errorMsg % cpf_imm::InfiniteFileObjectName % objectName;

					setExitCode(static_cast<int>(FMS_CPF_Exception::INVALIDFILE), errorMsg.str() );

					TRACE(fms_cpf_oi_infFileTrace, "create(...), file name not valid:<%s>", newInfFileInfo.fileName.c_str());
					break;
				}
				continue;
			}
		}

		// extract the attributes
		for(size_t idx = 0; attr[idx] != NULL && (result != ACS_CC_FAILURE); ++idx)
		{
			// check if recordLength attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::recordLengthAttribute, attr[idx]->attrName) )
			{
				newInfFileInfo.recordLength = *reinterpret_cast<unsigned int*>(attr[idx]->attrValues[0]);

				if( !checkRecordLength(newInfFileInfo.recordLength) )
				{
					result = ACS_CC_FAILURE;
					// Assemble the formated error message
					boost::format errorMsg(errorText::invalidAttribute);
					errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

					errorMsg % cpf_imm::recordLengthAttribute % newInfFileInfo.recordLength % cpf_imm::InfiniteFileObjectName % objectName;

					setExitCode(static_cast<int>(FMS_CPF_Exception::UNREASONABLE), errorMsg.str());

					TRACE(fms_cpf_oi_infFileTrace, "create(...), record length<%d> not valid", newInfFileInfo.recordLength);
					break;
				}

				continue;
			}

			// check if maxSize attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::maxSizeAttribute, attr[idx]->attrName) )
			{
				newInfFileInfo.maxSize = *reinterpret_cast<unsigned int*>(attr[idx]->attrValues[0]);

				if( ( newInfFileInfo.maxSize > SizeRange::MaxValue ) )
				{
					result = ACS_CC_FAILURE;
					// Assemble the formated error message
					boost::format errorMsg(errorText::invalidAttribute);
					errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

					errorMsg % cpf_imm::maxSizeAttribute % newInfFileInfo.maxSize % cpf_imm::InfiniteFileObjectName % objectName;

					setExitCode(static_cast<int>(FMS_CPF_Exception::UNREASONABLE), errorMsg.str());

					TRACE(fms_cpf_oi_infFileTrace, "create(...), MaxSize<%d> parameter not valid", newInfFileInfo.maxSize);
					break;
				}

				continue;
			}

			// check if maxTime attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::maxTimeAttribute, attr[idx]->attrName) )
			{
				newInfFileInfo.maxTime = *reinterpret_cast<unsigned int*>(attr[idx]->attrValues[0]);

				if( (newInfFileInfo.maxTime > TimeRange::MaxValue) )
				{
					result = ACS_CC_FAILURE;

					// Assemble the formated error message
					boost::format errorMsg(errorText::invalidAttribute);
					errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

					errorMsg % cpf_imm::maxTimeAttribute % newInfFileInfo.maxTime % cpf_imm::InfiniteFileObjectName % objectName;

					setExitCode(static_cast<int>(FMS_CPF_Exception::UNREASONABLE), errorMsg.str());

					TRACE(fms_cpf_oi_infFileTrace, "create(...), MaxTime<%d> parameter not valid", newInfFileInfo.maxTime);
					break;
				}

				continue;
			}

			// check if release condition attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::releaseCondAttribute, attr[idx]->attrName) )
			{
				int releaseConditionValue = *reinterpret_cast<int*>(attr[idx]->attrValues[0]);
				newInfFileInfo.releaseCondition = (bool)releaseConditionValue;
				TRACE(fms_cpf_oi_infFileTrace, "create(...), releaseCondition is <%s>", (newInfFileInfo.releaseCondition ? "TRUE" : "FALSE" ));
				continue;
			}

			// check if TQ attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::fileTQAttribute, attr[idx]->attrName) )
			{	
				//fix for IA71801
				if ( 0 == attr[idx]->attrValuesNum )
				{
                                        TRACE(fms_cpf_oi_infFileTrace, "%s", "create(...), Transferqueue attribute is not defined");
                                        continue;
                                }
				std::string transferQueue(reinterpret_cast<char *>(attr[idx]->attrValues[0]));
                                // Get TQ name
                                if( transferQueue.empty() )
                                {
                                        TRACE(fms_cpf_oi_infFileTrace, "%s", "create(...), Transferqueue name is set as empty");
                                        continue;
                                }
				else
				{		
					int errorCode;
					// Get TQ name
					std::string transferQueue(reinterpret_cast<char *>(attr[idx]->attrValues[0]));

					// Check is a block TQ
					bool isValidTQ = TQChecker::instance()->validateBlockTQ(transferQueue, errorCode);

					if(isValidTQ)
					{
						// check extra condition on block TQ name
						if(newInfFileInfo.fileName.compare(transferQueue) != 0)
						{
							// Block TQ is not equal to CP File name in upper case
							isValidTQ = false;
							errorCode = static_cast<int>(FMS_CPF_Exception::INVTQNAME);
						}
						else
						{
							// Block Transfer TQ
							newInfFileInfo.blockTQ = transferQueue;
							TRACE(fms_cpf_oi_infFileTrace, "create(...), connected to Block TQ<%s>", newInfFileInfo.blockTQ.c_str() );
						}
					}
					else
					{
						// Check is a file TQ
						newInfFileInfo.fileTQ = transferQueue;
						isValidTQ = TQChecker::instance()->validateFileTQ(transferQueue, errorCode);
						TRACE(fms_cpf_oi_infFileTrace, "create(...), connected to File TQ<%s>", newInfFileInfo.fileTQ.c_str() );
					}

					if(!isValidTQ )
					{
						result = ACS_CC_FAILURE;
						FMS_CPF_Exception exErr(static_cast<FMS_CPF_Exception::errorType>(errorCode));
						// Assemble the formated error message
						boost::format errorMsg(errorText::invalidAttribute);
						errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

						errorMsg % cpf_imm::fileTQAttribute % transferQueue % cpf_imm::InfiniteFileObjectName % objectName % exErr.errorText();

						setExitCode(errorCode, errorMsg.str());

						TRACE(fms_cpf_oi_infFileTrace, "create(...), Transfer Queue:<%s> not accepted", transferQueue.c_str());
						break;
					}
				}
			}
		}

		// Validate file values
		if( ACS_CC_SUCCESS == result )
		{
			TRACE(fms_cpf_oi_infFileTrace,"%s","create(...), create a new infinite file");
			m_infiniteFileOperationTable.insert(pair<ACS_APGCC_CcbId, infiniteFileInfo>(ccbId, newInfFileInfo));
		}
	}
	else
	{
		CPF_Log.Write("FMS_CPF_OI_InfiniteFile::create, error call on not implemented class", LOG_LEVEL_ERROR);
		TRACE(fms_cpf_oi_infFileTrace, "%s", "create(...), error call on not implemented class");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());

		result = ACS_CC_FAILURE;
	}

	TRACE(fms_cpf_oi_infFileTrace, "%s", "Leaving create(...)");
	return result;
}

/*============================================================================
	ROUTINE: deleted
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_InfiniteFile::deleted(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName)
{
	TRACE(fms_cpf_oi_infFileTrace, "%s", "Entering deleted(...)");

	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	ACS_CC_ReturnType result = ACS_CC_FAILURE;
	infiniteFileInfo delInfiniteFileInfo;

	delInfiniteFileInfo.action = Delete;
	delInfiniteFileInfo.completed = false;

	if(getInfiniteFileName(objName, delInfiniteFileInfo.fileName) &&
			getCpName(objName, delInfiniteFileInfo.cpName)	)
	{
		result = ACS_CC_SUCCESS;
		delInfiniteFileInfo.fileDN = objName;
		m_infiniteFileOperationTable.insert(pair<ACS_APGCC_CcbId, infiniteFileInfo>(ccbId, delInfiniteFileInfo));
		TRACE(fms_cpf_oi_infFileTrace,"%s", "deleted(...), delete a infinite file");
	}
	else
	{
		TRACE(fms_cpf_oi_infFileTrace,"%s", "deleted(...), error on get infinite file info");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());

	}

	TRACE(fms_cpf_oi_infFileTrace, "%s", "Leaving deleted(...)");
	return result;
}

/*============================================================================
	ROUTINE: modify
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_InfiniteFile::modify(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName, ACS_APGCC_AttrModification **attrMods)
{
	TRACE(fms_cpf_oi_infFileTrace, "%s", "Entering modify(...)");
	ACS_CC_ReturnType result = ACS_CC_SUCCESS;

	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	infiniteFileInfo modInfiniteFileInfo;

	modInfiniteFileInfo.action = Modify;
	modInfiniteFileInfo.completed = false;
	modInfiniteFileInfo.maxSize = 0;
	modInfiniteFileInfo.maxTime = 0;
	modInfiniteFileInfo.releaseCondition = false;
	modInfiniteFileInfo.changeMask = 0;

	if(getInfiniteFileName(objName, modInfiniteFileInfo.fileName) &&
			getCpName(objName, modInfiniteFileInfo.cpName)	)
	{
		std::string fileDN(objName);
		std::string objectName;
		// get the objectName from RDN
		getLastFieldValue(fileDN, objectName, false);

		// extract the attributes to modify
		for(size_t idx = 0; attrMods[idx] != NULL ; idx++)
		{
			ACS_APGCC_AttrValues modAttribute = attrMods[idx]->modAttr;

			// check if recordLength attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::recordLengthAttribute, modAttribute.attrName) )
			{
				// change not allowed
				result = ACS_CC_FAILURE;
				// Assemble the formated error message
				boost::format errorMsg(errorText::attributeChange);
				errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

				errorMsg % cpf_imm::recordLengthAttribute % cpf_imm::InfiniteFileObjectName % objectName;

				setExitCode(errorText::errorValue, errorMsg.str());

				TRACE(fms_cpf_oi_infFileTrace, "%s", "modify(...), record length change not allowed");
				break;
			}

			// check if maxSize attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::maxSizeAttribute, modAttribute.attrName) )
			{
				modInfiniteFileInfo.maxSize = *reinterpret_cast<unsigned int*>(modAttribute.attrValues[0]);

				if( (modInfiniteFileInfo.maxSize > SizeRange::MaxValue) )
				{
					result = ACS_CC_FAILURE;

					// Assemble the formated error message
					boost::format errorMsg(errorText::invalidAttribute);
					errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

					errorMsg % cpf_imm::maxSizeAttribute % modInfiniteFileInfo.maxSize % cpf_imm::InfiniteFileObjectName % objectName;

					setExitCode(static_cast<int>(FMS_CPF_Exception::UNREASONABLE), errorMsg.str());

					TRACE(fms_cpf_oi_infFileTrace, "create(...), maxSize parameter not valid:<%d>", modInfiniteFileInfo.maxSize);
					break;
				}

				// Set the flag for max size change
				modInfiniteFileInfo.changeMask |= changeSet::MAXSIZE_CHANGE;

				TRACE(fms_cpf_oi_infFileTrace,  "modify(...), infinite file maxSize change to :<%d>", modInfiniteFileInfo.maxSize);
				continue;
			}

			// check if maxTime attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::maxTimeAttribute, modAttribute.attrName) )
			{
				modInfiniteFileInfo.maxTime = *reinterpret_cast<unsigned int*>(modAttribute.attrValues[0]);

				if( (modInfiniteFileInfo.maxTime > TimeRange::MaxValue) )
				{
					result = ACS_CC_FAILURE;
					// Assemble the formated error message
					boost::format errorMsg(errorText::invalidAttribute);
					errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

					errorMsg % cpf_imm::maxTimeAttribute % modInfiniteFileInfo.maxTime % cpf_imm::InfiniteFileObjectName % objectName;

					setExitCode(static_cast<int>(FMS_CPF_Exception::UNREASONABLE), errorMsg.str());

					TRACE(fms_cpf_oi_infFileTrace, "create(...), MaxTime parameter not valid:<%d>", modInfiniteFileInfo.maxTime );
					break;
				}

				// Set the flag for max time change
				modInfiniteFileInfo.changeMask |= changeSet::MAXTIME_CHANGE;

				TRACE(fms_cpf_oi_infFileTrace, "%s", "modify(...), infinite file maxTime change");
				continue;
			}

			// check if release condition attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::releaseCondAttribute, modAttribute.attrName) )
			{
				int releaseConditionValue = *reinterpret_cast<int*>(modAttribute.attrValues[0]);
				modInfiniteFileInfo.releaseCondition = (bool)releaseConditionValue;

				// Set the flag for release condition change
				modInfiniteFileInfo.changeMask |= changeSet::RELCOND_CHANGE;
				continue;
			}

			// check if file TQ attribute
			if( 0 == ACE_OS::strcmp(cpf_imm::fileTQAttribute, modAttribute.attrName) )
			{

				// Set the flag for TQ change
				modInfiniteFileInfo.changeMask |= changeSet::FILETQ_CHANGE;

				if(modAttribute.attrValuesNum == 0)
				{
					TRACE(fms_cpf_oi_infFileTrace, "%s", "modify(...), file TQ removed");
					continue;
				}

				// Get TQ name
				std::string transferQueue(reinterpret_cast<char *>(modAttribute.attrValues[0]));

				if(transferQueue.empty())
				{
					TRACE(fms_cpf_oi_infFileTrace, "%s", "modify(...), file TQ removed");
					continue;
				}
				else
				{
					// validate tq
					int errorCode;

					// Check is a block TQ
					bool isValidTQ = TQChecker::instance()->validateBlockTQ(transferQueue, errorCode);

					if(isValidTQ)
					{
						// check extra condition on block TQ name
						if(modInfiniteFileInfo.fileName.compare(transferQueue) != 0)
						{
							// Block TQ is not equal to CP File name in upper case
							isValidTQ = false;
							errorCode = static_cast<int>(FMS_CPF_Exception::INVTQNAME);
						}
						else
						{
							// Block Transfer TQ
							modInfiniteFileInfo.blockTQ = transferQueue;
							TRACE(fms_cpf_oi_infFileTrace, "modify(...), Block TQ:<%s>", modInfiniteFileInfo.blockTQ.c_str() );
						}
					}
					else
					{
						// Check is a file TQ
						modInfiniteFileInfo.fileTQ = transferQueue;
						isValidTQ = TQChecker::instance()->validateFileTQ(transferQueue, errorCode);
						TRACE(fms_cpf_oi_infFileTrace, "modify(...), File TQ:<%s>", modInfiniteFileInfo.fileTQ.c_str() );
					}

					if(!isValidTQ )
					{
						result = ACS_CC_FAILURE;
						FMS_CPF_Exception exErr(static_cast<FMS_CPF_Exception::errorType>(errorCode));
						// Assemble the formated error message
						boost::format errorMsg(errorText::invalidAttribute);
						errorMsg.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit ) );

						errorMsg % cpf_imm::fileTQAttribute % transferQueue % cpf_imm::InfiniteFileObjectName % objectName % exErr.errorText();

						setExitCode(errorCode, errorMsg.str());

						TRACE(fms_cpf_oi_infFileTrace, "modify(...), file TQ:<%s> not accepted", modInfiniteFileInfo.fileTQ.c_str());
						break;
					}
				}
				continue;
			}

		}// end for

		if(ACS_CC_FAILURE != result )
		{
			m_infiniteFileOperationTable.insert(pair<ACS_APGCC_CcbId, infiniteFileInfo>(ccbId, modInfiniteFileInfo));
			TRACE(fms_cpf_oi_infFileTrace, "%s", "modify(...),  modify attributes of a infinite file");
		}
	}
	else
	{
		result = ACS_CC_FAILURE;
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());

		TRACE(fms_cpf_oi_infFileTrace, "%s", "modify(...), error on get infinite file info");
	}

	TRACE(fms_cpf_oi_infFileTrace, "%s", "Leaving modify(...)");
	return result;
}

/*============================================================================
	ROUTINE: complete
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_InfiniteFile::complete(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_infFileTrace, "%s", "Entering complete(...)");

	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	ACS_CC_ReturnType result = ACS_CC_SUCCESS;

	FMS_CPF_Exception::errorType cmdExeResult= FMS_CPF_Exception::OK;
	std::string errorDetail;

	// find all operations related to the ccbid
	std::pair<operationTable::iterator, operationTable::iterator> operationRange;
	operationRange = m_infiniteFileOperationTable.equal_range(ccbId);

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
				CPF_CreateFile_Request::cpFileData fileInfo;

				// set all file composite attribute
				fileInfo.fileType = FMS_CPF_Types::ft_INFINITE;
				fileInfo.composite = true;
				fileInfo.recordLength = element->second.recordLength;
				fileInfo.fileName = element->second.fileName;
				fileInfo.volumeName = element->second.volumeName;

				fileInfo.fileDN = element->second.fileDN;

				fileInfo.tqMode = FMS_CPF_Types::tm_NONE;

				// Only one field could be not empty
				if(!element->second.blockTQ.empty())
				{
					fileInfo.transferQueue = element->second.blockTQ;
					fileInfo.tqMode = FMS_CPF_Types::tm_BLOCK;
				}

				if(!element->second.fileTQ.empty())
				{
					fileInfo.transferQueue = element->second.fileTQ;
					fileInfo.tqMode = FMS_CPF_Types::tm_FILE;
				}

				fileInfo.maxSize = element->second.maxSize;
				// convert minutes to seconds
				fileInfo.maxTime = stdValue::SecondsInMinute * element->second.maxTime;
				fileInfo.releaseCondition = element->second.releaseCondition;

				// will be empty string in SCP
				fileInfo.cpName = element->second.cpName;

				TRACE(fms_cpf_oi_infFileTrace, "%s", "complete(), enqueue infinite file create request to the CMD Handler");

				// create the file creation request
				CPF_CreateFile_Request* createFile = new CPF_CreateFile_Request(waitResult, fileInfo);

				m_CmdHandler->enqueue(createFile);

				TRACE(fms_cpf_oi_infFileTrace, "%s","complete(), wait request execution");
				waitResult.get(operationResult);

				cmdExeResult = operationResult.errorCode();

				// checks the cmd execution result
				if(cmdExeResult != FMS_CPF_Exception::OK )
				{
					result = ACS_CC_FAILURE;
					errorDetail = operationResult.errorText();
					errorDetail += operationResult.detailInfo();
					TRACE(fms_cpf_oi_infFileTrace, "%s", "complete(), error on infinite file creation");
				}
				else
				{
					// File has been created
					element->second.completed = true;
					TRACE(fms_cpf_oi_infFileTrace, "%s", "complete(), infinite file created");
				}
			}
			break;

			case Delete :
			{
				CPF_DeleteFile_Request::deleteFileData fileInfo;

				fileInfo.composite = true;
				fileInfo.fileType = FMS_CPF_Types::ft_INFINITE;
				fileInfo.fileName = element->second.fileName;
				fileInfo.cpName = element->second.cpName;

				TRACE(fms_cpf_oi_infFileTrace, "%s","complete(), delete request");

				// create the file deletion request
				CPF_DeleteFile_Request* deleteFile = new CPF_DeleteFile_Request(fileInfo);

				// Acquire exclusive file access for delete
				if( deleteFile->acquireFileLock(operationResult) )
				{
					m_deleteOperationList.push_back(deleteFile);
					// File has been locked for deleted
					element->second.completed = true;
					TRACE(fms_cpf_oi_infFileTrace, "%s", "complete(), infinite file deleted");
				}
				else
				{
					result = ACS_CC_FAILURE;
					errorDetail = operationResult.errorText();
					errorDetail += operationResult.detailInfo();
					TRACE(fms_cpf_oi_infFileTrace, "%s", "complete(), error on infinite file deletion");
				}
			}
			break;

			case Modify :
			{
				CPF_ChangeFileAttribute_Request::cpFileModData fileInfo;

				fileInfo.fileType = FMS_CPF_Types::ft_INFINITE;
				fileInfo.cpName = element->second.cpName;
				fileInfo.currentFileName = element->second.fileName;

				// Set the new parameters
				fileInfo.newMaxSize = element->second.maxSize;
				fileInfo.newMaxTime = stdValue::SecondsInMinute * element->second.maxTime; //convert the minutes to seconds
				fileInfo.newReleaseCondition = element->second.releaseCondition;
				fileInfo.changeFlags = element->second.changeMask;

				fileInfo.newTQMode = FMS_CPF_Types::tm_NONE;

				// Only one field could be not empty
				if(!element->second.blockTQ.empty())
				{
					fileInfo.newTransferQueue = element->second.blockTQ;
					fileInfo.newTQMode = FMS_CPF_Types::tm_BLOCK;
				}

				if(!element->second.fileTQ.empty())
				{
					fileInfo.newTransferQueue = element->second.fileTQ;
					fileInfo.newTQMode = FMS_CPF_Types::tm_FILE;
				}

				TRACE(fms_cpf_oi_infFileTrace, "%s", "complete(), enqueue change request to the CMD Handler");

				// create the file modify request
				CPF_ChangeFileAttribute_Request* modifyFile = new CPF_ChangeFileAttribute_Request(waitResult, fileInfo);

				// The cmd request object will be deleted by the cmd handler
				m_CmdHandler->enqueue(modifyFile);

				TRACE(fms_cpf_oi_infFileTrace, "%s", "complete(), wait request execution");
				waitResult.get(operationResult);

				cmdExeResult = operationResult.errorCode();

				// checks the cmd request
				if( cmdExeResult != FMS_CPF_Exception::OK )
				{
					result = ACS_CC_FAILURE;
					errorDetail = operationResult.errorText();
					errorDetail += operationResult.detailInfo();
					TRACE(fms_cpf_oi_infFileTrace, "%s","complete(), error on composite file modify");
				}
				else
				{
					// File has been changed
					element->second.completed = true;
					// For undo operation
					element->second.maxSize = fileInfo.newMaxSize;
					element->second.maxTime = fileInfo.newMaxTime;
					element->second.releaseCondition = fileInfo.newReleaseCondition;

					if(fileInfo.oldTQMode == FMS_CPF_Types::tm_NONE)
					{
						element->second.fileTQ.clear();
						element->second.blockTQ.clear();
					}
					else if(fileInfo.oldTQMode ==  FMS_CPF_Types::tm_FILE)
					{
						element->second.fileTQ = fileInfo.oldTransferQueue;
					}
					else
						element->second.blockTQ = fileInfo.oldTransferQueue;

					TRACE(fms_cpf_oi_infFileTrace, "%s", "complete(), infinite file modified");
				}
			}
			break;

			default :
				TRACE(fms_cpf_oi_infFileTrace, "%s", "complete(), infiniteFile IMM action unknown");
		}
	}

	// set the exit code to the caller
	setExitCode(static_cast<int>(cmdExeResult), errorDetail);

	TRACE(fms_cpf_oi_infFileTrace, "%s", "Leaving complete(...)");
	return result;
}

/*============================================================================
	ROUTINE: abort
 ============================================================================ */
void FMS_CPF_OI_InfiniteFile::abort(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_infFileTrace, "%s", "Entering abort(...)");
	// To avoid warning about unused parameter
	UNUSED(oiHandle);
	// find all operations related to the ccbid
	std::pair<operationTable::iterator, operationTable::iterator> operationRange;
	operationRange = m_infiniteFileOperationTable.equal_range(ccbId);

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
				case Create :
					{
						CPF_DeleteFile_Request::deleteFileData fileInfo;

						fileInfo.composite = true;
						fileInfo.fileType = FMS_CPF_Types::ft_INFINITE;
						fileInfo.fileName = element->second.fileName;
						fileInfo.cpName = element->second.cpName;
						TRACE(fms_cpf_oi_infFileTrace, "%s", "abort(), undo of infinite file creation");

						// create the file deletion object
						CPF_DeleteFile_Request* deleteFileObj = new (std::nothrow) CPF_DeleteFile_Request(fileInfo);

						if(NULL != deleteFileObj)
						{
							// Acquire exclusive file access
							if( deleteFileObj->acquireFileLock(operationResult) )
							{
								// Remove the created file
								deleteFileObj->deleteFile();
							}
							delete deleteFileObj;
						}
					}
					break;

				case Delete :
					{
						TRACE(fms_cpf_oi_infFileTrace, "%s", "abort(), undo of infinite file deletion");
						continue;
					}
					break;

				case Modify :
					{
						CPF_ChangeFileAttribute_Request::cpFileModData fileInfo;

						fileInfo.fileType = FMS_CPF_Types::ft_INFINITE;
						fileInfo.cpName = element->second.cpName;

						fileInfo.currentFileName = element->second.fileName;
						fileInfo.newMaxSize = element->second.maxSize;
						fileInfo.newMaxTime = element->second.maxTime;
						fileInfo.newReleaseCondition = element->second.releaseCondition;
						fileInfo.changeFlags = element->second.changeMask;

						fileInfo.newTQMode = FMS_CPF_Types::tm_NONE;

						// Only one field could be not empty
						if(!element->second.blockTQ.empty())
						{
							fileInfo.newTransferQueue = element->second.blockTQ;
							fileInfo.newTQMode = FMS_CPF_Types::tm_BLOCK;
						}

						if(!element->second.fileTQ.empty())
						{
							fileInfo.newTransferQueue = element->second.fileTQ;
							fileInfo.newTQMode = FMS_CPF_Types::tm_FILE;
						}

						TRACE(fms_cpf_oi_infFileTrace, "%s", "abort(), undo infinite file modify");

						// create the file modify request
						CPF_ChangeFileAttribute_Request* modifyFile = new CPF_ChangeFileAttribute_Request(waitResult, fileInfo);

						// The cmd request object will be deleted by the cmd handler
						m_CmdHandler->enqueue(modifyFile);
						waitResult.get(operationResult);
					}
					break;
			}

			// checks the cmd result
			if( operationResult.errorCode() != FMS_CPF_Exception::OK )
			{
				TRACE(fms_cpf_oi_infFileTrace, "abort(), undo failed, error:<%s>", operationResult.errorText());
			}
		}

	}
	// Erase all elements from the table of the operations
	m_infiniteFileOperationTable.erase(operationRange.first, operationRange.second);

	// Erase all delete operation
	std::vector<CPF_DeleteFile_Request*>::iterator delOperation;
	for(delOperation = m_deleteOperationList.begin(); delOperation != m_deleteOperationList.end(); ++delOperation)
	{
		delete (*delOperation);
	}

	m_deleteOperationList.clear();
	TRACE(fms_cpf_oi_infFileTrace, "%s", "Leaving abort(...)");
}


/*============================================================================
	ROUTINE: apply
 ============================================================================ */
void FMS_CPF_OI_InfiniteFile::apply(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	TRACE(fms_cpf_oi_infFileTrace, "%s", "Entering apply(...)");
	ACE_GUARD(ACE_Thread_Mutex, guard, mutex_lock);
	// To avoid warning about unused parameter
	UNUSED(oiHandle);

	std::vector<CPF_DeleteFile_Request*>::iterator delOperation;
	CPF_DeleteFile_Request* deleteCmd;

	// Complete all delete requests, all file are already opened in exclusive mode
	for(delOperation = m_deleteOperationList.begin(); delOperation != m_deleteOperationList.end(); ++delOperation)
	{
		deleteCmd = *delOperation;
		// Physical file deletion
		deleteCmd->deleteFile();
		delete deleteCmd;
	}
	m_deleteOperationList.clear();

	// find all operations related to the ccbid
	std::pair<operationTable::iterator, operationTable::iterator> operationRange;
	operationRange = m_infiniteFileOperationTable.equal_range(ccbId);

	// Erase all elements from the table of the operations
	m_infiniteFileOperationTable.erase(operationRange.first, operationRange.second);
	if(isMoveCpFileOp)
        {
               cond_var.signal();
               isMoveCpFileOp = false;
        }

	TRACE(fms_cpf_oi_infFileTrace, "%s", "Leaving apply(...)");
}


/*============================================================================
	ROUTINE: updateRuntime
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_InfiniteFile::updateRuntime(const char* p_objName, const char** p_attrName)
{
	TRACE(fms_cpf_oi_infFileTrace, "%s", "Entering updateRuntime(...)");
	ACS_CC_ReturnType result = ACS_CC_SUCCESS;
	// To avoid warning about unused parameter
	UNUSED(p_attrName);

	CPF_DataFile_Request::dataFile infoFile;

	// get infinite main file name, cp name and subfile name
	if( getInfiniteFileName(p_objName, infoFile.mainFileName) &&
		getCpName(p_objName, infoFile.cpName) )
	{
		ACE_Future<FMS_CPF_PrivateException> waitResult;
		FMS_CPF_PrivateException operationResult;
		FMS_CPF_Exception::errorType cmdExeResult;

		// create the file modify request
		CPF_DataFile_Request* dataFile = new CPF_DataFile_Request(waitResult, infoFile, true, true);

		m_CmdHandler->enqueue(dataFile);

		TRACE(fms_cpf_oi_infFileTrace, "%s","updateRuntime(), wait data file cmd execution");
		waitResult.get(operationResult);

		cmdExeResult = operationResult.errorCode();

		// checks the cmd execution result
		if(FMS_CPF_Exception::OK != cmdExeResult )
		{
			result = ACS_CC_FAILURE;
			FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
			setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());

			TRACE(fms_cpf_oi_infFileTrace, "%s","updateRuntime(), error on infinite file update");
		}
		else
		{
			// Set the active subfile
			ACS_CC_ImmParameter paramActiveSubFile;
			paramActiveSubFile.attrName = cpf_imm::activeSubFileAttribute;
			paramActiveSubFile.attrType = ATTR_UINT32T;
			paramActiveSubFile.attrValuesNum = 1;
			void* lastSubFileValue[1] = { reinterpret_cast<void*>(&infoFile.lastSubFileActive) };
			paramActiveSubFile.attrValues = lastSubFileValue;

			TRACE(fms_cpf_oi_infFileTrace, "updateRuntime(...), active subfile<%d>", infoFile.lastSubFileActive);
			modifyRuntimeObj(p_objName, &paramActiveSubFile);

			// Set the latest subfile sent
			ACS_CC_ImmParameter paramLastSent;
			paramLastSent.attrName = cpf_imm::lastSubFileSentAttribute;
			paramLastSent.attrType = ATTR_UINT32T;
			paramLastSent.attrValuesNum = 1;
			void* lastSentValue[1] = { reinterpret_cast<void*>(&infoFile.lastSubFileSent) };
			paramLastSent.attrValues = lastSentValue;

			TRACE(fms_cpf_oi_infFileTrace, "updateRuntime(...), last sent subfile<%d>", infoFile.lastSubFileSent);
			modifyRuntimeObj(p_objName, &paramLastSent);

			// Set the numOfReaders on File
			ACS_CC_ImmParameter paramNumReaders;
			paramNumReaders.attrName = cpf_imm::numReadersAttribute;
			paramNumReaders.attrType = ATTR_UINT32T;
			paramNumReaders.attrValuesNum = 1;
			void* numReaders[1] = { reinterpret_cast<void*>(&(infoFile.numOfReaders)) };
			paramNumReaders.attrValues = numReaders;

			TRACE(fms_cpf_oi_infFileTrace, "updateRuntime(...), update numOfReaders=<%d>", infoFile.numOfReaders );
			modifyRuntimeObj(p_objName, &paramNumReaders);

			// Set the numOfWriters on File
			ACS_CC_ImmParameter paramNumWriters;
			paramNumWriters.attrName = cpf_imm::numWritersAttribute;
			paramNumWriters.attrType = ATTR_UINT32T;
			paramNumWriters.attrValuesNum = 1;
			void* numWriters[1] = { reinterpret_cast<void*>(&(infoFile.numOfWriters)) };
			paramNumWriters.attrValues = numWriters;

			TRACE(fms_cpf_oi_infFileTrace, "updateRuntime(...), update numOfWriters=<%d>", infoFile.numOfWriters );
			modifyRuntimeObj(p_objName, &paramNumWriters);

			// Set the exclusiveAccess on File
			ACS_CC_ImmParameter paramExcAccess;
			paramExcAccess.attrName = cpf_imm::exclusiveAccessAttribute;
			paramExcAccess.attrType = ATTR_INT32T;
			paramExcAccess.attrValuesNum = 1;
			void* excAccess[1] = { reinterpret_cast<void*>(&(infoFile.exclusiveAccess)) };
			paramExcAccess.attrValues = excAccess;

			TRACE(fms_cpf_oi_infFileTrace, "updateRuntime(...), update exclusiveAccess=<%d>", infoFile.exclusiveAccess);
			modifyRuntimeObj(p_objName, &paramExcAccess);

			// Set the transfer queue DN
			char tmpTqDN[128]={0};
			snprintf(tmpTqDN, sizeof(tmpTqDN)-1, "%s", infoFile.transferQueueDn.c_str());

			ACS_CC_ImmParameter paramTQDN;
			paramTQDN.attrName = cpf_imm::transferQueueDnAttribute;
			paramTQDN.attrType = ATTR_NAMET;
			paramTQDN.attrValuesNum = 1;
			void* tqDN[1] = { reinterpret_cast<void*>(tmpTqDN) };
			paramTQDN.attrValues = tqDN;

			TRACE(fms_cpf_oi_infFileTrace, "updateRuntime(...), TQ DN:<%s>", tmpTqDN);
			modifyRuntimeObj(p_objName, &paramTQDN);

			// Set the active subfile access type
			ACS_CC_ImmParameter paramActExcAccess;
			paramActExcAccess.attrName = cpf_imm::activeSubfilExclusiveAccessAttribute;
			paramActExcAccess.attrType = ATTR_INT32T;
			paramActExcAccess.attrValuesNum = 1;
			void* excActAccess[1] = { reinterpret_cast<void*>(&(infoFile.activeSubfileExclusiveAccess)) };
			paramActExcAccess.attrValues = excActAccess;

			TRACE(fms_cpf_oi_infFileTrace, "updateRuntime(...), active ISF eclusiveAccess=<%d>", infoFile.activeSubfileExclusiveAccess);
			modifyRuntimeObj(p_objName, &paramActExcAccess);

			// Set the numOfReaders on active subfile
			ACS_CC_ImmParameter paramActNumReaders;
			paramActNumReaders.attrName = cpf_imm::activeSubfileNumReadersAttribute;
			paramActNumReaders.attrType = ATTR_UINT32T;
			paramActNumReaders.attrValuesNum = 1;
			void* numActReaders[1] = { reinterpret_cast<void*>(&(infoFile.activeSubfileNumOfReaders)) };
			paramActNumReaders.attrValues = numActReaders;

			TRACE(fms_cpf_oi_infFileTrace, "updateRuntime(...), active ISF numOfReaders=<%d>", infoFile.activeSubfileNumOfReaders );
			modifyRuntimeObj(p_objName, &paramActNumReaders);

			// Set the numOfWriters of active subfile
			ACS_CC_ImmParameter paramActNumWriters;
			paramActNumWriters.attrName = cpf_imm::activeSubfileNumWritersAttribute;
			paramActNumWriters.attrType = ATTR_UINT32T;
			paramActNumWriters.attrValuesNum = 1;
			void* numActWriters[1] = { reinterpret_cast<void*>(&(infoFile.activeSubfileNumOfWriters)) };
			paramActNumWriters.attrValues = numActWriters;

			TRACE(fms_cpf_oi_infFileTrace, "updateRuntime(...), active ISF numOfWriters=<%d>", infoFile.activeSubfileNumOfWriters );
			modifyRuntimeObj(p_objName, &paramActNumWriters);

			// Set the size of active subfile
			ACS_CC_ImmParameter paramActiveSubfileSize;
			paramActiveSubfileSize.attrName = cpf_imm::activeSubfileSizeAttribute;
			paramActiveSubfileSize.attrType = ATTR_UINT32T;
			paramActiveSubfileSize.attrValuesNum = 1;
			void* numActSize[1] = { reinterpret_cast<void*>(&(infoFile.activeSubfileSize)) };
			paramActiveSubfileSize.attrValues = numActSize;

			TRACE(fms_cpf_oi_infFileTrace, "updateRuntime(...), active ISF size=<%d>", infoFile.activeSubfileSize );
			modifyRuntimeObj(p_objName, &paramActiveSubfileSize);

			setExitCode(SUCCESS);
		}
	}
	else
	{
		TRACE(fms_cpf_oi_infFileTrace, "%s", "updateRuntime(...), error to get infinite file name");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());

	}
	TRACE(fms_cpf_oi_infFileTrace, "%s", "Leaving updateRuntime(...)");
	return result;
}


/*============================================================================
ROUTINE: adminOperationCallback
============================================================================ */
void FMS_CPF_OI_InfiniteFile::adminOperationCallback(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_InvocationType invocation,
		const char* p_objName, ACS_APGCC_AdminOperationIdType operationId,
		ACS_APGCC_AdminOperationParamType** paramList)
{
	TRACE(fms_cpf_oi_infFileTrace, "%s", "Entering adminOperationCallback(...)");

	// To avoid warning messages
	UNUSED(operationId);
	UNUSED(paramList);

	CPF_ReleaseISF_Request::fileData fileInfo;

	if(getInfiniteFileName(p_objName, fileInfo.fileName) &&
			getCpName(p_objName, fileInfo.cpName)	)
	{
		TRACE(fms_cpf_oi_infFileTrace, "adminOperationCallback(...), Release subfile on infinite file <%s>, cpName<%s>", fileInfo.fileName.c_str(), fileInfo.cpName.c_str());
		FMS_CPF_PrivateException operationResult;

		ACS_APGCC::toUpper(fileInfo.fileName);
		CPF_ReleaseISF_Request iSFSwicthCmd(fileInfo);
		// checks the cmd execution result
		if( iSFSwicthCmd.executeReleaseISF(operationResult) )
		{
			setExitCode(SUCCESS);
			adminOperationResult(oiHandle, invocation, actionResult::SUCCESS);
			TRACE(fms_cpf_oi_infFileTrace, "%s","adminOperationCallback(...), Infinite subfile released");
		}
		else
		{
			std::string errMsg(operationResult.errorText());
			errMsg += operationResult.detailInfo();
			setExitCode(static_cast<int>(operationResult.errorCode()), errMsg );
			adminOperationResult(oiHandle, invocation, actionResult::FAILED);
			TRACE(fms_cpf_oi_infFileTrace, "%s","adminOperationCallback(), Infinite subfile NOT released");
		}
	}
	else
	{
		FMS_CPF_Exception exErr(FMS_CPF_Exception::GENERAL_FAULT);
		setExitCode(static_cast<int>(FMS_CPF_Exception::GENERAL_FAULT), exErr.errorText());

		adminOperationResult(oiHandle, invocation, actionResult::FAILED);
		TRACE(fms_cpf_oi_infFileTrace, "%s","adminOperationCallback(...), error not found all parameters");
	}

	TRACE(fms_cpf_oi_infFileTrace, "%s", "Leaving adminOperationCallback(...)");
}

/*============================================================================
	ROUTINE: ~FMS_CPF_OI_InfiniteFile
 ============================================================================ */
FMS_CPF_OI_InfiniteFile::~FMS_CPF_OI_InfiniteFile()
{
	m_infiniteFileOperationTable.clear();

	if(NULL != fms_cpf_oi_infFileTrace)
		delete fms_cpf_oi_infFileTrace;
}
