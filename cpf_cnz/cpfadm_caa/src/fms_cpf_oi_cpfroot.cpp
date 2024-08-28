/*
 * * @file fms_cpf_oi_cpfroot.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_OI_CPFRoot.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_oi_compositefile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-10-03
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
 *	| 1.0.0  | 2012-10-03 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */


#include "fms_cpf_oi_cpfroot.h"
#include "fms_cpf_common.h"
#include "fms_cpf_copyfile.h"
#include "fms_cpf_importfile.h"
#include "fms_cpf_exportfile.h"
#include "fms_cpf_movefile.h"
#include "fms_cpf_renamefile.h"

#include "fms_cpf_privateexception.h"
#include "fms_cpf_directorystructuremgr.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"
#include "ACS_APGCC_CommonLib.h"
#include "fms_cpf_parameterhandler.h"

extern ACS_TRA_Logging CPF_Log;

// Implementer Name
namespace cpfRootClass {
	const char ImmImplementerName[] = "CPF_OI_CpFileSystem";
}

// Action parameter name
namespace actionParameters{

	const char cpName[] = "cpName";
	const char srcCpFile[] = "sourceCpFile";
	const char dstCpFile[] = "destCpFile";
	const char source[] = "source";
	const char destination[] = "dest";
	const char volume[] = "volume";
	const char mode[] = "mode";
	const char zip[] = "toZip";

	const char currentName[] = "currentName";
	const char newName[] = "newName";

}

// FileM Cp files root
namespace fileMPath{
	const char cpFiles[] = "cpFiles";
}

namespace {
	const char NBI_PREFIX[] ="@ComNbi@"; // To show message inside Com-Cli
}
/*============================================================================
	ROUTINE: FMS_CPF_OI_CPFRoot
 ============================================================================ */
FMS_CPF_OI_CPFRoot::FMS_CPF_OI_CPFRoot(FMS_CPF_CmdHandler* cmdHandler)
: FMS_CPF_OI_BaseObject(cmdHandler, cpf_imm::rootClassName)
, acs_apgcc_objectimplementerinterface_V3(cpfRootClass::ImmImplementerName)
{
	m_trace = new ACS_TRA_trace("FMS_CPF_OI_CPFRoot");
}

/*============================================================================
	ROUTINE: ~FMS_CPF_OI_CPFRoot
 ============================================================================ */
FMS_CPF_OI_CPFRoot::~FMS_CPF_OI_CPFRoot()
{
	if(NULL != m_trace)
		delete m_trace;
}
/*============================================================================
ROUTINE: adminOperationCallback
============================================================================ */
void FMS_CPF_OI_CPFRoot::adminOperationCallback(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_InvocationType invocation,
		const char* p_objName, ACS_APGCC_AdminOperationIdType operationId,
		ACS_APGCC_AdminOperationParamType** paramList)
{
	TRACE(m_trace, "%s", "Entering adminOperationCallback(...)");
	// To avoid unused warning
	UNUSED(p_objName);

	std::vector<ACS_APGCC_AdminOperationParamType> outParameteres;
	std::string CpName("");
	std::string errorDetail("");
	bool actResult;
	bool checkValidity = true;
	int exitCode =  static_cast<int>(FMS_CPF_PrivateException::GENERAL_FAULT);

	// cast to enum definedOperation the received operationId
	definedOperations actionToExe = (definedOperations)operationId;

	// Check for invalid system configuration
	if( m_IsMultiCP && (importCpFile == actionToExe || exportCpFile == actionToExe|| copyCpFile == actionToExe || moveCpFile == actionToExe || renameCpFile == actionToExe) )
	{
		TRACE(m_trace, "%s", "adminOperationCallback(...), invalid system configuration");
		// reply to the action call
		replyToActionCall(oiHandle, invocation, FMS_CPF_Exception::ILLOPTION, "Not allowed in this system configuration: The action is not allowed in Multi-CP System.", "Not allowed in this system configuration: The action is not allowed in Multi-CP System.");
		checkValidity = false;
	}

	if( !m_IsMultiCP && (importCpClusterFile == actionToExe || exportCpClusterFile == actionToExe|| copyCpClusterFile == actionToExe || moveCpClusterFile == actionToExe || renameCpClusterFile == actionToExe) )
	{
		TRACE(m_trace, "%s", "adminOperationCallback(...), invalid system configuration");
		// reply to the action call
		replyToActionCall(oiHandle, invocation, FMS_CPF_Exception::ILLOPTION, "Not allowed in this system configuration: The action is not allowed in Single-CP System.", "Not allowed in this system configuration: The action is not allowed in Single-CP System.");
		checkValidity = false;
	}

	if (checkValidity)
	{
		// check the System type
		if(m_IsMultiCP && !getOperationParameter(CpName, actionParameters::cpName, paramList) )
		{
			TRACE(m_trace, "%s", "adminOperationCallback(...) error to get Cp Name");
			exitCode = static_cast<int>(FMS_CPF_Exception::CPNAMENOPASSED);
			// reply to the action call
			replyToActionCall(oiHandle, invocation, exitCode);

			CPF_Log.Write("FMS_CPF_OI_CPFRoot::adminOperationCallback(), error to get Cp Name", LOG_LEVEL_ERROR);
		}
		else
		{
			FMS_CPF_Types::copyMode mode;

			// Get the copy mode option
			getOperationParameter(mode, actionParameters::mode, paramList);

			if ( SUCCESS != checkCpName(CpName) )	//HS25665
			{
				TRACE(m_trace, "%s", "adminOperationCallback(...) error on Cp Name");
				exitCode = static_cast<int>(FMS_CPF_Exception::CPNOTEXISTS);
				// reply to the action call
				replyToActionCall(oiHandle, invocation, exitCode);

				CPF_Log.Write("FMS_CPF_OI_CPFRoot::adminOperationCallback(), error on Cp Name", LOG_LEVEL_ERROR);
			}
			else
			{

				switch(actionToExe)
				{
				case importCpFile:
				case importCpClusterFile:
				{
					TRACE(m_trace, "%s", "adminOperationCallback(...) import a Cp File");

					CPF_ImportFile_Request::importFileData importInfo;

					if( getOperationParameter(importInfo.relativeSrcPath, actionParameters::source, paramList) &&
							getOperationParameter(importInfo.dstFile, actionParameters::dstCpFile, paramList) )
					{
						// check the absolute path result
						if(getAbsolutePath(importInfo.relativeSrcPath, importInfo.srcPath))
						{
							ACS_APGCC::toUpper(importInfo.dstFile);
							importInfo.importOption = mode;
							importInfo.cpName = CpName;

							TRACE(m_trace, "adminOperationCallback(), srcPath=<%s>, dstFile=<%s>", importInfo.srcPath.c_str(), importInfo.dstFile.c_str() );
							TRACE(m_trace, "adminOperationCallback(), cpName=<%s>, mode=<%i>", importInfo.cpName.c_str(), (int)importInfo.importOption );
							// Create the cmd request
							CPF_ImportFile_Request importFiles(importInfo);

							TRACE(m_trace, "%s", "adminOperationCallback(), wait import execution");

							// execute import
							actResult = importFiles.executeImport();

							if(actResult)
							{
								TRACE(m_trace, "%s(...), files are imported", __func__);
								// reply to the action call
								replyToActionCall(oiHandle, invocation, SUCCESS);
							}
							else
							{
								TRACE(m_trace, "%s(...), files are NOT imported", __func__);
								// reply to the action call
								replyToActionCall(oiHandle, invocation, importFiles.getErrorCode(), importFiles.getErrorMessage(), importFiles.getComCliErrorMessage());
							}
						}
						else
						{
							TRACE(m_trace, "%s", "adminOperationCallback(...), invalid absolute path to import");
							// reply to the action call
							exitCode =  static_cast<int>(FMS_CPF_PrivateException::INVALIDPATH);
							replyToActionCall(oiHandle, invocation, exitCode);
							CPF_Log.Write("FMS_CPF_OI_CPFRoot::adminOperationCallback(), error on absolute path to import", LOG_LEVEL_ERROR);
						}
					}
					else
					{
						TRACE(m_trace, "%s", "adminOperationCallback(...), error not found all parameters to import");
						// reply to the action call
						replyToActionCall(oiHandle, invocation, exitCode);
						CPF_Log.Write("FMS_CPF_OI_CPFRoot::adminOperationCallback(), error not found all parameters to import", LOG_LEVEL_ERROR);
					}
				}
				break;

				case exportCpFile:
				case exportCpClusterFile:
				{
					TRACE(m_trace, "%s", "adminOperationCallback(...) export a Cp File");

					CPF_ExportFile_Request::exportFileData exportInfo;

					if( getOperationParameter(exportInfo.srcFile, actionParameters::srcCpFile, paramList) &&
							getOperationParameter(exportInfo.relativeDstPath, actionParameters::destination, paramList) &&
							getOperationParameter(exportInfo.zipData, actionParameters::zip, paramList) )
					{
						// check the absolute path
						if(getAbsolutePath(exportInfo.relativeDstPath, exportInfo.dstPath))
						{
							ACS_APGCC::toUpper(exportInfo.srcFile);
							exportInfo.exportOption = mode;
							exportInfo.cpName = CpName;

							TRACE(m_trace, "adminOperationCallback(), srcFile:<%s>, dstPath:<%s>", exportInfo.srcFile.c_str(), exportInfo.dstPath.c_str() );
							TRACE(m_trace, "adminOperationCallback(), cpName:<%s>, mode=<%i>, isToZip:<%s>", exportInfo.cpName.c_str(), (int)exportInfo.exportOption, (exportInfo.zipData ? "TRUE" : "FALSE") );

							// Create the cmd request
							CPF_ExportFile_Request exportFiles(exportInfo);

							TRACE(m_trace, "%s", "adminOperationCallback(), wait export execution");

							// execute export
							actResult = exportFiles.executeExport();

							if(actResult)
							{
								TRACE(m_trace, "%s(...), files are exported", __func__);
								// reply to the action call
								replyToActionCall(oiHandle, invocation, SUCCESS);
							}
							else
							{
								TRACE(m_trace, "%s(...), files are NOT exported", __func__);
								// reply to the action call
								replyToActionCall(oiHandle, invocation, exportFiles.getErrorCode(), exportFiles.getErrorMessage(), exportFiles.getComCliErrorMessage());
							}
						}
						else
						{
							TRACE(m_trace, "%s", "adminOperationCallback(...), error on absolute path to export");
							// reply to the action call
							exitCode =  static_cast<int>(FMS_CPF_PrivateException::INVALIDPATH);
							replyToActionCall(oiHandle, invocation, exitCode);
							CPF_Log.Write("FMS_CPF_OI_CPFRoot::adminOperationCallback(), error on absolute path to export", LOG_LEVEL_ERROR);
						}
					}
					else
					{
						TRACE(m_trace, "%s", "adminOperationCallback(...), error not found all parameters to export");

						// reply to the action call
						replyToActionCall(oiHandle, invocation, exitCode);
						CPF_Log.Write("FMS_CPF_OI_CPFRoot::adminOperationCallback(), error not found all parameters to export", LOG_LEVEL_ERROR);
					}
				}
				break;

				case copyCpFile:
				case copyCpClusterFile:
				{
					TRACE(m_trace, "%s", "adminOperationCallback(...) copy a Cp File");

					CPF_CopyFile_Request::copyFileData copyInfo;

					if( getOperationParameter(copyInfo.srcFile, actionParameters::srcCpFile, paramList) &&
							getOperationParameter(copyInfo.dstFile, actionParameters::dstCpFile, paramList) )
					{
						ACS_APGCC::toUpper(copyInfo.srcFile);
						ACS_APGCC::toUpper(copyInfo.dstFile);
						copyInfo.copyOption = mode;
						copyInfo.cpName = CpName;

						TRACE(m_trace, "adminOperationCallback(), srcFile=%s, dstFile=%s", copyInfo.srcFile.c_str(), copyInfo.dstFile.c_str() );
						TRACE(m_trace, "adminOperationCallback(), cpName=%s, mode=%i", copyInfo.cpName.c_str(), (int)copyInfo.copyOption );

						// Create the cmd request
						CPF_CopyFile_Request copyFiles(copyInfo);

						TRACE(m_trace, "%s", "adminOperationCallback(), wait copy execution");

						// execute copy
						actResult = copyFiles.executeCopy();

						if(actResult)
						{
							TRACE(m_trace, "%s(...), files are copied", __func__);
							// reply to the action call
							replyToActionCall(oiHandle, invocation, SUCCESS);
						}
						else
						{
							TRACE(m_trace, "%s(...), files are NOT copied", __func__);
							// reply to the action call
							replyToActionCall(oiHandle, invocation, copyFiles.getErrorCode(), copyFiles.getErrorMessage(), copyFiles.getComCliErrorMessage());
						}
					}
					else
					{
						TRACE(m_trace, "%s", "adminOperationCallback(...), error not found all parameters to copy");
						// reply to the action call
						replyToActionCall(oiHandle, invocation, exitCode);
						CPF_Log.Write("FMS_CPF_OI_CPFRoot::adminOperationCallback(), error not found all parameters to copy", LOG_LEVEL_ERROR);
					}
				}
				break;

				case moveCpFile:
				case moveCpClusterFile:
				{
					TRACE(m_trace, "%s", "adminOperationCallback(...) move a Cp File");
					CPF_MoveFile_Request::moveFileData moveInfo;

					if( getOperationParameter(moveInfo.srcFile, actionParameters::srcCpFile, paramList) &&
							getOperationParameter(moveInfo.dstVolume, actionParameters::volume, paramList) )
					{
						ACS_APGCC::toUpper(moveInfo.srcFile);
						ACS_APGCC::toUpper(moveInfo.dstVolume);
						moveInfo.cpName = CpName;

						TRACE(m_trace, "adminOperationCallback(), srcFile=<%s>, volume=<%s>, cpName=<%s>",
								moveInfo.srcFile.c_str(),
								moveInfo.dstVolume.c_str(),
								moveInfo.cpName.c_str() );

						// Create the cmd request
						CPF_MoveFile_Request moveFiles(moveInfo);

						TRACE(m_trace, "%s","adminOperationCallback(), wait move execution");

						// execute move
						actResult = moveFiles.executeMove();

						if(actResult)
						{
							TRACE(m_trace, "%s(...), files are moved", __func__);
							// reply to the action call
							replyToActionCall(oiHandle, invocation, SUCCESS);
						}
						else
						{
							TRACE(m_trace, "%s(...), files are NOT moved", __func__);
							// reply to the action call
							replyToActionCall(oiHandle, invocation, moveFiles.getErrorCode(), moveFiles.getErrorMessage(), moveFiles.getComCliErrorMessage());
						}
					}
					else
					{
						TRACE(m_trace, "%s","adminOperationCallback(...), error not found all parameters to move");

						// reply to the action call
						replyToActionCall(oiHandle, invocation, exitCode);

						CPF_Log.Write("FMS_CPF_OI_CPFRoot::adminOperationCallback(), error not found all parameters to move", LOG_LEVEL_ERROR);
					}
				}
				break;

				case renameCpFile:
				case renameCpClusterFile:
				{
					TRACE(m_trace, "%s", "adminOperationCallback(...) rename a Cp File");

					CPF_RenameFile_Request::renameFileData renameInfo;

					if( getOperationParameter(renameInfo.currentFile, actionParameters::currentName, paramList) &&
							getOperationParameter(renameInfo.newFile, actionParameters::newName, paramList) )
					{
						ACS_APGCC::toUpper(renameInfo.currentFile);
						ACS_APGCC::toUpper(renameInfo.newFile);

						renameInfo.cpName = CpName;

						TRACE(m_trace, "adminOperationCallback(), currentFile:<%s>, newFile:<%s>, cpName=<%s>",
								renameInfo.currentFile.c_str(),
								renameInfo.newFile.c_str(),
								renameInfo.cpName.c_str() );

						// Create the cmd request
						CPF_RenameFile_Request renameFile(renameInfo);

						TRACE(m_trace, "%s","adminOperationCallback(), wait rename execution");

						// execute rename
						actResult = renameFile.executeRenameFile();

						if(actResult)
						{
							TRACE(m_trace, "%s(...), files are renamed", __func__);
							// reply to the action call
							replyToActionCall(oiHandle, invocation, SUCCESS);
						}
						else
						{
							TRACE(m_trace, "%s(...), files are NOT renamed", __func__);
							// reply to the action call
							replyToActionCall(oiHandle, invocation, renameFile.getErrorCode(), renameFile.getErrorMessage(), renameFile.getComCliErrorMessage());
						}
					}
					else
					{
						TRACE(m_trace, "%s", "adminOperationCallback(...), error not found all parameters to rename");

						// reply to the action call
						replyToActionCall(oiHandle, invocation, exitCode);

						CPF_Log.Write("FMS_CPF_OI_CPFRoot::adminOperationCallback(), error not found all parameters to rename", LOG_LEVEL_ERROR);
					}
				}
				break;

				default:

					TRACE(m_trace, "%s", "adminOperationCallback(...) error action not defined");

					// reply to the action call
					replyToActionCall(oiHandle, invocation, exitCode);
					CPF_Log.Write("FMS_CPF_OI_CPFRoot::adminOperationCallback(), error action not defined", LOG_LEVEL_ERROR);
				}
			}
		}
	}
	TRACE(m_trace, "%s", "Leaving adminOperationCallback(...)");
}

/*============================================================================
ROUTINE: getOperationParameter
============================================================================ */
bool FMS_CPF_OI_CPFRoot::getOperationParameter(std::string& parValue, const char* parName, ACS_APGCC_AdminOperationParamType** paramList)
{
	TRACE(m_trace, "%s", "Entering in getOperationParameters(...)");
	bool result = false;
	// extract the parameters
	for(size_t idx = 0; paramList[idx] != NULL ; ++idx)
	{
		// check the parameter name
		if( 0 == ACE_OS::strcmp(parName, paramList[idx]->attrName) )
		{
			parValue = reinterpret_cast<char *>(paramList[idx]->attrValues);
			TRACE(m_trace, "getOperationParameters(...), found parameter : <%s> = <%s>", parName, parValue.c_str() );
			result= true;
			break;
		}
	}
	TRACE(m_trace, "%s", "Leaving getOperationParameters(...)");
	return result;
}

/*============================================================================
ROUTINE: getOperationParameter
============================================================================ */
bool FMS_CPF_OI_CPFRoot::getOperationParameter(bool& parValue, const char* parName, ACS_APGCC_AdminOperationParamType** paramList)
{
	TRACE(m_trace, "%s", "Entering in getOperationParameters(...)");
	bool result = false;
	// extract the parameters
	for(size_t idx = 0; paramList[idx] != NULL ; ++idx)
	{
		// check the parameter name
		if( 0 == ACE_OS::strcmp(parName, paramList[idx]->attrName) )
		{
			int toZip = *reinterpret_cast<int*>(paramList[idx]->attrValues);
			parValue = static_cast<bool>(toZip);
			TRACE(m_trace, "getOperationParameters(...), found parameter : <%s> = <%s>", parName, (parValue ? "TRUE" : "FALSE") );
			result= true;
			break;
		}
	}
	TRACE(m_trace, "%s", "Leaving getOperationParameters(...)");
	return result;
}

/*============================================================================
ROUTINE: getOperationParameter
============================================================================ */
void FMS_CPF_OI_CPFRoot::getOperationParameter(FMS_CPF_Types::copyMode& parValue, const char* parName, ACS_APGCC_AdminOperationParamType** paramList)
{
	TRACE(m_trace, "%s", "Entering in getOperationParameters(...)");

	//Set the default value
	parValue = FMS_CPF_Types::cm_NORMAL;

	// extract the parameters
	for(size_t idx = 0; paramList[idx] != NULL ; ++idx)
	{
		// check the parameter name
		if( 0 == ACE_OS::strcmp(parName, paramList[idx]->attrName) )
		{
			parValue = (FMS_CPF_Types::copyMode) *reinterpret_cast<unsigned int*>(paramList[idx]->attrValues);
			TRACE(m_trace, "getOperationParameters(...), found parameter : <%s> = <%i>", parName, parValue );
			break;
		}
	}
	TRACE(m_trace, "%s", "Leaving getOperationParameters(...)");
}

/*============================================================================
ROUTINE: replyToActionCall
============================================================================ */
void FMS_CPF_OI_CPFRoot::replyToActionCall(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_InvocationType invocation, const int errorCode)
{
	TRACE(m_trace, "%s", "Entering in replyToActionCall(...)");
	int actResult = actionResult::SUCCESS;

	std::vector<ACS_APGCC_AdminOperationParamType> outParameteres;
	char tmpMsgEc[5] = {0};
	char tmpMsgEm[512] = {0};
	char tmpMsgEcm[512] = {0};

	// Convert the exit code to string
	std::stringstream strExitCode;
	strExitCode << errorCode;

	ACE_OS::snprintf(tmpMsgEc, 4, "%s", strExitCode.str().c_str());
	addActionResult(exitCodePar::errorCode, tmpMsgEc, outParameteres);

	if( SUCCESS != errorCode )
	{
		actResult = actionResult::FAILED;
		FMS_CPF_Exception exErrMsg(static_cast<FMS_CPF_Exception::errorType>(errorCode));

		ACE_OS::snprintf(tmpMsgEm, 511, "%s", exErrMsg.errorText());
		addActionResult(exitCodePar::errorMessage, tmpMsgEm, outParameteres);

		ACE_OS::snprintf(tmpMsgEcm, 511, "%s%s", NBI_PREFIX, exErrMsg.errorText());
		addActionResult(exitCodePar::errorComCliMessage, tmpMsgEcm, outParameteres);
	}

	ACS_CC_ReturnType result = adminOperationResult(oiHandle, invocation, actResult, outParameteres);

	if(ACS_CC_SUCCESS != result)
    {
    	TRACE(m_trace, "%s", "replyToActionCall(), error on action result reply");
    	CPF_Log.Write("FMS_CPF_OI_CPFRoot::replyToActionCall(), error on action result reply", LOG_LEVEL_ERROR);
    }

	TRACE(m_trace, "%s", "Leaving replyToActionCall(...)");
}

/*============================================================================
ROUTINE: replyToActionCall
============================================================================ */
void FMS_CPF_OI_CPFRoot::replyToActionCall(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_InvocationType invocation, const int errorCode, const char* errorMsg, const char* errorComCliMsg)
{
	TRACE(m_trace, "%s", "Entering in replyToActionCall(...)");

	std::vector<ACS_APGCC_AdminOperationParamType> outParameteres;
	char tmpMsgEc[5] = {0};
	char tmpMsgEm[512] = {0};
	char tmpMsgEcm[512] = {0};

	// Convert the exit cote to string
	std::stringstream strExitCode;
	strExitCode << errorCode;
	ACE_OS::snprintf(tmpMsgEc, 4, "%s", strExitCode.str().c_str());
	addActionResult(exitCodePar::errorCode, tmpMsgEc, outParameteres);

	ACE_OS::snprintf(tmpMsgEm, 511, "%s", errorMsg);
	addActionResult(exitCodePar::errorMessage, tmpMsgEm, outParameteres);

	ACE_OS::snprintf(tmpMsgEcm, 511, "%s%s", NBI_PREFIX, errorComCliMsg);
	addActionResult(exitCodePar::errorComCliMessage, tmpMsgEcm, outParameteres);

	ACS_CC_ReturnType result = adminOperationResult(oiHandle, invocation, actionResult::FAILED, outParameteres);

    if(ACS_CC_SUCCESS != result)
    {
    	TRACE(m_trace, "%s", "replyToActionCall(), error on action result reply");
    	CPF_Log.Write("FMS_CPF_OI_CPFRoot::replyToActionCall(.), error on action result reply", LOG_LEVEL_ERROR);
    }

	TRACE(m_trace, "%s", "Leaving replyToActionCall(...)");
}

/*============================================================================
ROUTINE: addActionResult
============================================================================ */
void FMS_CPF_OI_CPFRoot::addActionResult(char* attrName, char* message, std::vector<ACS_APGCC_AdminOperationParamType>& outParameteres)
{
	TRACE(m_trace, "Entering in addActionResult(), errorText:<%s>", message);

	ACS_APGCC_AdminOperationParamType detail;

	detail.attrName = attrName;
	detail.attrType = ATTR_STRINGT;
	detail.attrValues = reinterpret_cast<void*>(message);

	outParameteres.push_back(detail);

	TRACE(m_trace, "%s", "Leaving addActionResult()");
}


/*============================================================================
ROUTINE: getFileMCpFilePath
============================================================================ */
void FMS_CPF_OI_CPFRoot::getFileMCpFilePath(std::string& path)
{
	TRACE(m_trace, "%s", "Entering in getFileMCpFilePath()");

	if(m_ImportExportPath.empty())
	{
		bool result = false;
		ACS_APGCC_DNFPath_ReturnTypeT getResult;
		ACS_APGCC_CommonLib fileMPathHandler;
		int bufferLength = 512;
		char buffer[bufferLength];

		ACE_OS::memset(buffer, 0, bufferLength);
		// get the physical path
		getResult = fileMPathHandler.GetFileMPath(fileMPath::cpFiles, buffer, bufferLength);

		if(ACS_APGCC_DNFPATH_SUCCESS == getResult)
		{
			// path get successful
			m_ImportExportPath = buffer;
			result = true;
		}
		else if(ACS_APGCC_STRING_BUFFER_SMALL == getResult)
		{
			TRACE(m_trace, "%s", "getFileMCpFilePath(), error on first get");
			// Buffer too small, but now we have the right size
			char buffer2[bufferLength+1];
			ACE_OS::memset(buffer2, 0, bufferLength+1);
			// try again to get
			getResult = fileMPathHandler.GetFileMPath(fileMPath::cpFiles, buffer2, bufferLength);

			// Check if it now is ok
			if(ACS_APGCC_DNFPATH_SUCCESS == getResult)
			{
				// path get successful now
				m_ImportExportPath = buffer2;
				result = true;
			}
		}

		// In any case log the result
		char logBuffer[512] = {'\0'};

		if(result)
		{
			// path retrieved log it
			snprintf(logBuffer, 511, "FMS_CPF_OI_CPFRoot::getFileMCpFilePath,  path of FileM:<%s> is <%s>", fileMPath::cpFiles, m_ImportExportPath.c_str() );
			CPF_Log.Write(logBuffer, LOG_LEVEL_INFO);
		}
		else
		{
			// error occurs log it
			snprintf(logBuffer, 511, "FMS_CPF_OI_CPFRoot::getFileMCpFilePath, error:<%d> on get path of FileM:<%s>", static_cast<int>(getResult), fileMPath::cpFiles );
			CPF_Log.Write(logBuffer, LOG_LEVEL_ERROR);
		}
		TRACE(m_trace, "%s", logBuffer);
	}

	path = m_ImportExportPath;

	TRACE(m_trace, "%s", "Leaving getFileMCpFilePath()");
}

/*============================================================================
ROUTINE: getAbsolutePath
============================================================================ */
bool FMS_CPF_OI_CPFRoot::getAbsolutePath(const std::string& relativePath, std::string& absolutePath)
{
	TRACE(m_trace, "%s", "Entering in getAbsolutePath()");
	bool result = true;
	std::string nbiPath;
	char pathBuffer[PATH_MAX] = {0};
	// Get the import/export path
	getFileMCpFilePath(nbiPath);

	// Check for initial slash into relative path
	size_t slashPos = relativePath.find_first_of(DirDelim);

	if(slashPos != 0)
	{
		// Add the slash
		nbiPath += DirDelim;
	}

	absolutePath = nbiPath + relativePath;

	realpath(absolutePath.c_str(), pathBuffer);

	std::string realPath(pathBuffer);

	if( std::string::npos == realPath.find(nbiPath) )
	{
		TRACE(m_trace, "getAbsolutePath(), the path:<%s> is out the NBI folder:<%s>", absolutePath.c_str(), nbiPath.c_str());
		result = false;
	}
	TRACE(m_trace, "%s", "Leaving getAbsolutePath()");
	return result;
}

/*============================================================================
	ROUTINE: create
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CPFRoot::create(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *className, const char* parentname, ACS_APGCC_AttrValues **attr)
{
	// To avoid warning about unused parameter
	UNUSED(oiHandle);
	UNUSED(ccbId);
	UNUSED(className);
	UNUSED(parentname);
	UNUSED(attr);
	return ACS_CC_SUCCESS;
}

/*============================================================================
	METHOD: deleted
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CPFRoot::deleted(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName)
{
	UNUSED(oiHandle);
	UNUSED(ccbId);
	UNUSED(objName);
	return ACS_CC_FAILURE;
}

/*============================================================================
	METHOD: modify
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CPFRoot::modify(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName, ACS_APGCC_AttrModification **attrMods)
{
	TRACE(m_trace, "Entering %s", __func__);
	UNUSED(oiHandle);
	UNUSED(ccbId);
	UNUSED(objName);
	//UNUSED(attrMods);
	// check changed attributes
	ACS_CC_ReturnType result = ACS_CC_FAILURE;
	for(size_t idx = 0; attrMods[idx] != NULL ; idx++)
	{
		ACS_APGCC_AttrValues modAttribute = attrMods[idx]->modAttr;
		// check if block transfer time slice attribute
		if( 0 == ACE_OS::strcmp(cpf_imm::blockTransferTimeSliceAttribute, modAttribute.attrName) )
		{
			// Only change value is allowed
			if(0U != modAttribute.attrValuesNum)
			{
				int timeSlice = (*reinterpret_cast<int *>(modAttribute.attrValues[0]));
				// check for valid value
				if( (stdValue::MIN_CP_TIME_SLICE <= timeSlice) && (stdValue::MAX_CP_TIME_SLICE >= timeSlice) )
				{
					// update cached value
					ParameterHndl::instance()->setCpTimeSlice(timeSlice);
					result = ACS_CC_SUCCESS;
					char msgLog[128] = {0};
					snprintf(msgLog, 127, "%s, Block Transfer Cp Time Slice changed to:<%d>", __func__, timeSlice);
					TRACE(m_trace, "%s", msgLog);
				}
				else
				{
					TRACE(m_trace, "%s, Block Transfer Cp Time Slice:<%d> not allowed", __func__, timeSlice);
				}
			}
			break;
		}
	}
	TRACE(m_trace, "Leaving %s", __func__);
	return result;
}


/*============================================================================
	METHOD: complete
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CPFRoot::complete(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	UNUSED(oiHandle);
	UNUSED(ccbId);
	return ACS_CC_SUCCESS;
}

/*============================================================================
	METHOD: abort
 ============================================================================ */
void FMS_CPF_OI_CPFRoot::abort(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	UNUSED(oiHandle);
	UNUSED(ccbId);
}

/*============================================================================
	METHOD: apply
 ============================================================================ */
void FMS_CPF_OI_CPFRoot::apply(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
	UNUSED(oiHandle);
	UNUSED(ccbId);
}

/*============================================================================
	METHOD: complete
 ============================================================================ */
ACS_CC_ReturnType FMS_CPF_OI_CPFRoot::updateRuntime(const char* p_objName, const char** p_attrName)
{
	UNUSED(p_objName);
	UNUSED(p_attrName);
	return ACS_CC_FAILURE;
}
