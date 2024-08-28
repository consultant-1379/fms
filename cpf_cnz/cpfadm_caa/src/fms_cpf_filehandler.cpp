/*
 * * @file fms_cpf_filehandler.cpp
 *	@brief
 *	Class method implementation for File.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_filehandler.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-07-07
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
 *	| 1.0.0  | 2011-07-07 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_filehandler.h"
#include "fms_cpf_filetransferhndl.h"
#include "fms_cpf_parameterhandler.h"
#include "fms_cpf_common.h"

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
File::File(const FMS_CPF_FileId& fileid) :
fileid_(fileid),
volume_(),
cpname_(),
attribute_(),
m_fileDN(),
m_TransferQueueDN(),
m_FileType(FMS_CPF_Types::ft_TEXT),
m_FileTQHandler(NULL),
gohBlockHandlerExist(false),
faccessp_(NULL),
m_ImmStateVerified(false)
{

}

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
File::File(const FMS_CPF_FileId& fileid,
            const std::string& volume,
			const std::string& cpname,
            const FMS_CPF_Attribute& attribute ) :
fileid_(fileid),
volume_(volume),
cpname_(cpname),
attribute_(attribute),
m_fileDN(),
m_TransferQueueDN(),
m_FileTQHandler(NULL),
gohBlockHandlerExist(false),
faccessp_(0),
m_ImmStateVerified(false)
{
	m_FileType = attribute_.type();
}

//------------------------------------------------------------------------------
//      createFileTQHandler
//------------------------------------------------------------------------------
bool File::createFileTQHandler()
{
	bool result = true;
	if(NULL == m_FileTQHandler)
	{
		std::string mainFile(fileid_.file());
		std::string filePath = ParameterHndl::instance()->getCPFroot(cpname_.c_str()) + DirDelim + volume_ + DirDelim + mainFile + DirDelim;
		// cpName empty in a SCP
		std::string cpName;
		if(!cpname_.empty() && (cpname_.compare(DEFAULT_CPNAME) != 0) )
			cpName = cpname_;

		m_FileTQHandler = new (std::nothrow) FMS_CPF_FileTransferHndl(mainFile,
																	  filePath,
																	  cpName
																	 );
		if(NULL == m_FileTQHandler)
		{
			// Allocation problem
			result = false;
		}
	}
	return result;
}

void File::removeFileTQHandler()
{
	if(NULL != m_FileTQHandler)
	{
		delete m_FileTQHandler;
		m_FileTQHandler = NULL;
	}
}

void File::addSubFile(const std::string& subFileName)
{
	m_SubFiles.insert(subFileName);
}

void File::removeSubFile(const std::string& subFileName)
{
	std::set<std::string>::iterator subFile = m_SubFiles.find(subFileName);

	if(m_SubFiles.end() != subFile)	m_SubFiles.erase(subFile);
}

void File::changeSubFile(const std::string& oldSubFileName, const std::string& newSubFileName)
{
	removeSubFile(oldSubFileName);
	addSubFile(newSubFileName);
}

void File::getAllSubFile(std::list<FMS_CPF_FileId>& subFileList)
{
	std::set<std::string>::const_iterator subFile;
	std::string fileName = fileid_.data();
	for(subFile = m_SubFiles.begin(); subFile != m_SubFiles.end(); ++subFile)
	{
		std::string subFileName(fileName);
		subFileName += parseSymbol::minus + (*subFile);

		subFileList.push_back(subFileName);
	}
}

void File::getAllSubFile(std::set<std::string>& subFileList)
{
	subFileList = m_SubFiles;
}

bool File::subFileExist(const std::string& subFileName)
{
	bool result = false;
	std::set<std::string>::const_iterator subFile = m_SubFiles.find(subFileName);

	if(m_SubFiles.end() != subFile) result = true;

	return result;
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
File::~File()
{
	if(NULL != m_FileTQHandler)
		delete m_FileTQHandler;
}

//------------------------------------------------------------------------------
//      Equality operator
//------------------------------------------------------------------------------
bool File::operator==(const File& file) const
{
  return file.fileid_ == fileid_;
}

bool File::operator==(const File* file) const
{
  return file->fileid_ == fileid_;
}
