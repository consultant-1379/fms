/*
 * * @file fms_cpf_cpdfile.cpp
 *	@brief
 *	Class method implementation for CPDFile.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_cpdfile.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-22
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
 *	| 1.0.0  | 2011-11-22 | qvincon      | File imported.                      |
 *	+========+============+==============+=====================================+
 */
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_cpdfile.h"
#include "fms_cpf_directorystructuremgr.h"
#include "fms_cpf_common.h"
#include "fms_cpf_types.h"

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	ROUTINE: CPDFile
 ============================================================================ */
CPDFile::CPDFile(std::string& cpname) :
m_CpName(cpname),
m_FileReference(0)
{
	fms_cpf_CPDFileTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_CPDFile");
}

/*============================================================================
	ROUTINE: ~CPDFile()
 ============================================================================ */
CPDFile::~CPDFile()
{
	if(NULL != fms_cpf_CPDFileTrace)
		delete fms_cpf_CPDFileTrace;
}

/*============================================================================
	ROUTINE: getRecordLength()
 ============================================================================ */
unsigned short CPDFile::getRecordLength() throw(FMS_CPF_PrivateException)
{
  return m_FileReference->getRecordLength();
}

/*============================================================================
	ROUTINE: getFileSize()
 ============================================================================ */
off64_t CPDFile::getFileSize(const char* path) throw(FMS_CPF_PrivateException)
{
	off64_t status = m_FileReference->phyGetFileSize(path);

	if(-1L == status)
	{
		char detail[64] = {'\0'};
		ACE_OS::snprintf(detail, 63, "CPDFile::getFileSize, on file<%s> failed", path );
		TRACE(fms_cpf_CPDFileTrace, "%s", detail);
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, detail);
	}
	return status;
}

std::string CPDFile::getFileName()
{
	std::string fileName = m_FileReference->getFileid().data();
	TRACE(fms_cpf_CPDFileTrace, "getFileName(), main file:<%s>", fileName.c_str());
	return fileName;
}

/*============================================================================
	ROUTINE: fileReference()
 ============================================================================ */
void CPDFile::setFileReference(const FileReference& aFileReference)
{
	TRACE(fms_cpf_CPDFileTrace,"%s", "Set FileReference()");
	m_FileReference = aFileReference;
}
/*============================================================================
	ROUTINE: isCompositeClass()
 ============================================================================ */
bool CPDFile::isCompositeClass() throw(FMS_CPF_PrivateException)
{
	bool result;
	result = m_FileReference->getAttribute().composite();
	TRACE(fms_cpf_CPDFileTrace,"isCompositeClass() result = <%s>", (result ? "TRUE" : "FALSE") );
	return result;
}

/*============================================================================
	ROUTINE: isSubFile()
 ============================================================================ */
bool CPDFile::isSubFile()
{
	bool result = true;
	FMS_CPF_FileId fileId = m_FileReference->getFileid();
	if(fileId.subfile().empty())
	{
		result = false;
	}
	TRACE(fms_cpf_CPDFileTrace,"isSubFile() result = <%s>", (result ? "TRUE" : "FALSE") );
	return result;
}

/*============================================================================
	ROUTINE: isSubFile()
 ============================================================================ */
bool CPDFile::isSubFile(const FMS_CPF_FileId &fileId)
{
  bool result = true;
  if(fileId.subfile().empty())
  {
	  result = false;
  }
  TRACE(fms_cpf_CPDFileTrace,"isSubFile(...) result = <%s>", (result ? "TRUE" : "FALSE") );
  return result;
}

/*============================================================================
	ROUTINE: checkFileClass()
 ============================================================================ */
void CPDFile::checkFileClass() throw (FMS_CPF_PrivateException)
{
  if(m_FileReference->getFileid().subfile().empty() &&
		  m_FileReference->getAttribute().composite() )
  {
	  TRACE(fms_cpf_CPDFileTrace,"%s" ,"checkFileClass() not passed");
      throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::TYPEERROR, "checkFileClass() not passed");
  }
} 			

/*============================================================================
	ROUTINE: checkReadAccess()
 ============================================================================ */
void CPDFile::checkReadAccess() throw(FMS_CPF_PrivateException)
{
	TRACE(fms_cpf_CPDFileTrace,"%s" ,"Entering in checkReadAccess()");

	FMS_CPF_Types::accessType mode = m_FileReference->getAccess();

	if(FMS_CPF_Types::W_ == mode || FMS_CPF_Types::XW_ == mode)
	{
		char detail[64] = {'\0'};
		ACE_OS::snprintf(detail, 63, "CPDFile::checkReadAccess, accessType = <%d>", static_cast<int>(mode) );
		TRACE(fms_cpf_CPDFileTrace, "%s", detail);
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::ACCESSERROR, detail);
	}
}

/*============================================================================
	ROUTINE: checkWriteAccess()
 ============================================================================ */
void CPDFile::checkWriteAccess() throw(FMS_CPF_PrivateException)
{
	FMS_CPF_Types::accessType mode = m_FileReference->getAccess();
	if(FMS_CPF_Types::R_ == mode || FMS_CPF_Types::XR_ == mode)
	{
		char detail[64] = {'\0'};
		ACE_OS::snprintf(detail, 63, "CPDFile::checkWriteAccess, accessType = <%d>", static_cast<int>(mode) );
		TRACE(fms_cpf_CPDFileTrace, "%s", detail);
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::ACCESSERROR, detail);
	}
}

/*============================================================================
	ROUTINE: unifyFileSize()
 ============================================================================ */
long CPDFile::unifyFileSize(const std::string& aFilePath, unsigned short aRecordLength)
  throw(FMS_CPF_PrivateException)
{
	long status = m_FileReference->phyUnifyFileSize(aFilePath.c_str(), aRecordLength);

	if( -1L == status)
	{
		char detail[64] = {'\0'};
		ACE_OS::snprintf(detail, 63, "CPDFile::unifyFileSize, on file<%s> failed", aFilePath.c_str() );
		TRACE(fms_cpf_CPDFileTrace, "%s", detail);
		throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, detail);
	}

	return status;
}


/*============================================================================
	ROUTINE: closeWithoutException()
 ============================================================================ */
void CPDFile::closeWithoutException(const FileReference& aFileReference) const
{
    DirectoryStructureMgr::instance()->closeExceptionLess(aFileReference, m_CpName.c_str() );
}


