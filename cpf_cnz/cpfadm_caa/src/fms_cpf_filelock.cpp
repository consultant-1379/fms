/*
 * * @file fms_cpf_filelock.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_FileLock.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_filelock.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-18
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
 *	| 1.0.0  | 2011-11-18 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_filelock.h"
#include "fms_cpf_common.h"

#include "ACS_TRA_trace.h"

FileLock::FileLock(const std::string& afileId, const std::string& cpName):
m_Users(0),
m_FileName(afileId),
m_CpName(cpName)
{
	m_trace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_FileLock");
	TRACE(m_trace, "FileLock(), on file<%s>, CP<%s>", m_FileName.c_str(), m_CpName.c_str());
}

void FileLock::lock()
{
	m_lock.acquire();
}

void FileLock::unlock()
{
	m_lock.release();
}

FileLock::~FileLock()
{
	TRACE(m_trace, "%s", "Destroying FileLock");
	if(NULL!= m_trace)
		delete m_trace;
}
