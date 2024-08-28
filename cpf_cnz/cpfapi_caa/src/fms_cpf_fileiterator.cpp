//**********************************************************************
//
/*
 * * @file fms_cpf_fileiterator.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_FileIterator.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_fileiterator.h module
 *
 *	@author
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
 *	| 1.0.0  | 2011-07-07 | 		     | File imported.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_fileiterator.h"
#include "fms_cpf_fileid.h"
#include "fms_cpf_privateexception.h"
#include "fms_cpf_apdcommonfile.h"
#include "fms_cpf_eventhandler.h"
#include "fms_cpf_configreader.h"
#include "fms_cpf_api_trace.h"
#include "fms_cpf_omcmdhandler.h"


#include <ACS_APGCC_Command.H>
#include "ACS_TRA_trace.h"
#include "ACS_APGCC_Util.H"
#include "aes_ohi_blockhandler2.h"
#include "aes_ohi_errorcodes.h"

#include <ace/ACE.h>

#include <algorithm>
#include <sstream>

namespace immTag {
		const char volumeClassName[]= "AxeCpFileSystemCpVolume";
		char VolumeKey[] = "cpVolumeId";
		char InfiniteFileKey[] = "infiniteFileId";
		char InfiniteSubFileKey[] = "infiniteSubFileId=";
		char CompositeFileKey[] = "compositeFileId";
		char CompositeSubFileKey[] = "compositeSubFileId";
		char SimpleFileKey[] = "simpleFileId";

		const char cpVolumeNameAttribute[] = "volumeName";
		const char cpNameAttribute[]= "cpName";
		const char recordLengthAttribute[] = "recordLength";
		const char maxSizeAttribute[] = "maxSize";
		const char maxTimeAttribute[] = "maxTime";
		const char releaseCondAttribute[] = "overrideSubfileClosure";
		const char fileTQAttribute[] = "transferQueue";

		const char activeSubFileAttribute[] = "activeSubFile";

		const char sizeAttribute[] = "size";
		const char numReadersAttribute[] = "numOfReaders";
		const char numWritersAttribute[] = "numOfWriters";
		const char exclusiveAccessAttribute[]= "exclusiveAccess";
}

namespace {
	const char Comma = ',';
	const char Equal = '=';
	const char Dash = '-';
	const char AtSign = ':';
	const char Space = ' ';
}

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_FileIterator::FMS_CPF_FileIterator(const char* filename, bool longList, const char* pCPName)
throw(FMS_CPF_Exception)
: longList_(longList),
  listSubiles_(false),
  m_CpName(pCPName),
  m_bIsSysBC(false)
{
	cpfIterator = new (std::nothrow) ACS_TRA_trace("FMS_CPF_FileIterator");
	m_OmHandler = new FMS_CPF_omCmdHandler();

	readConfiguration();

    TRACE(cpfIterator, "FMS_CPF_FileIterator(), filename = %s, longlist = %s cpName = %s",
            filename, longList?"true":"false", m_CpName.c_str());

    int retCode = m_OmHandler->loadClassInst(immTag::volumeClassName, volumeList);

    if(retCode != 0)
    {
    	throw FMS_CPF_Exception(FMS_CPF_Exception::GENERAL_FAULT);
    }
    
    // if in BC system delete all the volume objects not in cpname
    if (m_bIsSysBC)
    {
    	retCode = cleanVolumeList(volumeList);
    	if(retCode != 0)
    	{
    		throw FMS_CPF_Exception ((FMS_CPF_Exception::errorType) retCode);
    	}
    }

    // loading childs objects
    retCode = loadDN(filename, false);

    if(retCode != 0)
    {
    	if(FMS_CPF_Exception::FILENOTFOUND == retCode )
		{
			FMS_CPF_Exception ex(FMS_CPF_Exception::FILENOTFOUND);
			std::string errMsg(ex.errorText());
			errMsg +=  std::string(filename);
			throw FMS_CPF_Exception (FMS_CPF_Exception::FILENOTFOUND, errMsg);
		}
		else
		{
			 throw FMS_CPF_Exception(FMS_CPF_Exception::GENERAL_FAULT);
		}
    }
}

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------


FMS_CPF_FileIterator::FMS_CPF_FileIterator(const char* filename, bool longList, bool listSubFiles, const char* pCPName)
throw (FMS_CPF_Exception)
:longList_(longList),
 listSubiles_(listSubFiles),
 m_CpName(pCPName),
 m_bIsSysBC(false)
{
	m_OmHandler = new FMS_CPF_omCmdHandler();
	cpfIterator = new (std::nothrow) ACS_TRA_trace("FMS_CPF_FileIterator");

	readConfiguration();

	TRACE(cpfIterator, "FMS_CPF_FileIterator(), filename = %s, longList = %s, listSubfiles = %s, pCPName = %s",
            filename,
            longList?"true":"false",
            listSubFiles?"true":"false",
            m_CpName.c_str());


    int retCode = m_OmHandler->loadClassInst(immTag::volumeClassName, volumeList);

    if(retCode != 0)
    {
    	throw FMS_CPF_Exception (FMS_CPF_Exception::GENERAL_FAULT);
    }
    
    // if in MCP system delete all the volume not of this Cp name
    if(m_bIsSysBC)
    {

    	retCode = cleanVolumeList(volumeList);
    	if(retCode != 0)
    	{
    	  throw FMS_CPF_Exception((FMS_CPF_Exception::errorType) retCode);
    	}
    }
    
    // loading childs objects
    retCode = loadDN(filename, listSubFiles);

    if(retCode != 0)
    {
    	if(FMS_CPF_Exception::FILENOTFOUND == retCode )
    	{
    		FMS_CPF_Exception ex(FMS_CPF_Exception::FILENOTFOUND);
    		std::string errMsg(ex.errorText());
    		errMsg +=  std::string(filename);
    		throw FMS_CPF_Exception (FMS_CPF_Exception::FILENOTFOUND, errMsg);
    	}
    	else
    	{
    		 throw FMS_CPF_Exception(FMS_CPF_Exception::GENERAL_FAULT);
    	}
    }
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FMS_CPF_FileIterator::~FMS_CPF_FileIterator ()
{
	if(NULL != m_OmHandler)
		delete m_OmHandler;

    if (NULL != cpfIterator)
    {
    	delete cpfIterator;
    }

    compositeFileList.clear();
    subCompositeFileList.clear();
    m_compositeFileIdMap.clear();
    s_mapStringValues.clear();
    m_subComFileIdMap.clear();
}

int FMS_CPF_FileIterator::cleanVolumeList(std::vector<std::string>& volumeDNList)
{
	TRACE(cpfIterator, "cleanVolumeList(), found all volumes of Cp Name<%s>", m_CpName.c_str());
	std::vector<std::string> tmpVolumeCpList;

	int retCode = 0;
	std::vector<std::string>::const_iterator volumeDNIdx;

	// Check each defined volume
	for(volumeDNIdx = volumeDNList.begin(); volumeDNIdx != volumeDNList.end(); ++volumeDNIdx)
	{
		// Get the volume RDN ( cpVolumeId=<volumeName>:<cpName>) from DN
		// Split the field in volume key and value
		size_t equalPos = (*volumeDNIdx).find_first_of(Equal);
		size_t commaPos = (*volumeDNIdx).find_first_of(Comma);

		// Check if some error happens
		if( (std::string::npos != equalPos) )
		{
			std::string volumeRDN;

			// check for a single field case
			if( std::string::npos == commaPos )
				volumeRDN = (*volumeDNIdx).substr(equalPos + 1);
			else
				volumeRDN = (*volumeDNIdx).substr(equalPos + 1, (commaPos - equalPos - 1) );

			// search ':' sign
			size_t tagAtSignPos = volumeRDN.find(AtSign);

			// Check if the tag is present
			if( std::string::npos != tagAtSignPos )
			{
				// get the cpName
				std::string cpName(volumeRDN.substr(tagAtSignPos + 1));

				// make the value in upper case
				ACS_APGCC::toUpper(cpName);

				// check the cpName
				if(m_CpName.compare(cpName) == 0)
				{
					// volume of this Cp, add to the DN list
					tmpVolumeCpList.push_back(*volumeDNIdx);
				}
			}
			else
				TRACE(cpfIterator, "cleanVolumeList(), parse error on volume DN:<%s>", (*volumeDNIdx).c_str());
		}
		else
			TRACE(cpfIterator, "cleanVolumeList(), parse error on volume DN:<%s>", (*volumeDNIdx).c_str());
	}

	// Set the new DN list of volumes
	volumeDNList.clear();
	volumeDNList = tmpVolumeCpList;

	TRACE(cpfIterator, "cleanVolumeList(), found <%zd> volumes", volumeDNList.size());
	return retCode;
}

int FMS_CPF_FileIterator::loadChildList(const std::string& fname, const std::vector<std::string>& vMaster, std::vector<std::string> &vChild)
{
	int retCode = 0;

	std::vector<std::string>::const_iterator volumeDN;
	std::vector<std::pair<std::string, std::string> > volumeSortList;
	std::string volumeName;
	//Volume sorting
	for(volumeDN = vMaster.begin(); volumeDN != vMaster.end(); ++volumeDN )
	{
		if(getVolumeName((*volumeDN), volumeName) )
		{
			m_volumeNameMap.insert(std::make_pair((*volumeDN), volumeName));
			volumeSortList.push_back(std::make_pair(volumeName, (*volumeDN)));
		}
	}

	sort(volumeSortList.begin(), volumeSortList.end());

	TRACE(cpfIterator, "loadChildList(), fileName:<%s>", fname.c_str());

	std::vector<pair<std::string, std::string> > VectorList;
	std::vector<std::string> fileList;
	std::vector<pair<std::string, std::string> >::const_iterator itVolList;

	std::vector<std::string>::iterator itFileList;

	for( itVolList = volumeSortList.begin(); itVolList != volumeSortList.end();  ++itVolList)
	{
		TRACE(cpfIterator, "loadChildList(), current volume (dn) = %s", (itVolList->second).c_str());
		m_OmHandler->loadChildInst((itVolList->second).c_str(), ACS_APGCC_SUBLEVEL, &fileList);

		  for(itFileList = fileList.begin(); itFileList != fileList.end(); ++itFileList)
		  {
			  size_t equalPos =(*itFileList).find_first_of(Equal);
			  size_t commaPos = (*itFileList).find_first_of(Comma);

			  // Check if some error happens
			  if( (std::string::npos != equalPos) && (std::string::npos != commaPos) )
			  {
				  std::string fileName = (*itFileList).substr(equalPos + 1, (commaPos - equalPos - 1) );
				  // make the name in upper case
				  ACS_APGCC::toUpper(fileName);
				  m_compositeFileIdMap.insert(std::make_pair(*itFileList, fileName));

				  if(!fname.empty())
				  {
					  if(fname.compare(fileName) == 0)
					  {
						  vChild.push_back(*itFileList);
						  TRACE(cpfIterator, "loadChildList(), file <%s> has been found", fileName.c_str());
						  fileList.clear();
						  return 0;
					  }
				  }
				  else
				  {
					  TRACE(cpfIterator, "loadChildList(), add file:<%s> DN:<%s>", fileName.c_str(), (*itFileList).c_str());
					  VectorList.push_back(std::make_pair(fileName, *itFileList));
				  }
			  }
		  }
		  fileList.clear();

		  //File sorting
		  sort(VectorList.begin(), VectorList.end());

		  std::vector<pair<std::string, std::string> >::const_iterator  iterFileList;
		  for(iterFileList=VectorList.begin(); iterFileList!=VectorList.end(); ++iterFileList)
		  {
			  vChild.push_back(iterFileList->second);
		  }

		  VectorList.clear();
	  }

	  if(vChild.size() == 0U)
		 retCode = FMS_CPF_Exception::FILENOTFOUND;

	  return retCode;
}
	
int FMS_CPF_FileIterator::loadDN(const char* fileName, bool listSB)
{
	TRACE(cpfIterator, "loadDN(), InputFile:<%s>", fileName);

	// get file name part
	FMS_CPF_FileId fileIn(fileName);
	curCompFile = fileIn.file();

    initialize();

    int retCode = loadChildList(curCompFile, volumeList, compositeFileList);

    if(listSB && retCode == 0)
	{
    	 TRACE(cpfIterator, "%s", "loadDN(), get subfiles list");

    	 std::string subfile(fileIn.subfileAndGeneration());
    	 bool isSubFilesList = subfile.empty();

    	 std::vector<std::string> subFileList;
    	 vector<pair<std::string, std::string> > VectorSubList;

    	 // Loading sub files
		 TRACE(cpfIterator, "loadDN() compFileList size:<%zd>", compositeFileList.size());

		 std::vector<std::string>::const_iterator itFileList;
		 std::vector<std::string>::const_iterator itList;

		 // for each file get its subfiles
		 for(itFileList = compositeFileList.begin(); itFileList != compositeFileList.end();  ++itFileList)
		 {
			 TRACE(cpfIterator, "loadDN(), file : <%s>", (*itFileList).c_str() );
			 //check for file type
			 if(std::string::npos != (*itFileList).find(immTag::InfiniteFileKey))
			 {
				 // Infinite file handling
				 std::string infiniteFileName;
				 // get file name
				 if(getLastFieldValue((*itFileList), infiniteFileName))
				 {
					 TRACE(cpfIterator,"%s, get subfiles of infinite file:<%s>", __func__, infiniteFileName.c_str() );
					 // Assemble the command to send to CPF server
					 ACS_APGCC_Command cmd;
					 cmd.cmdCode = CPF_API_Protocol::GET_SUBFILESLIST;
					 cmd.data[0] = infiniteFileName;
					 cmd.data[1] = m_CpName;

					 try
					 {
						 // send command to CPF server to get all subfiles
						 if(0 == m_DsdClient.execute(cmd) )
						 {
							 // check the command result
							 if(FMS_CPF_Exception::OK == cmd.result)
							 {
								 // get all received info
								 int numOfSubfiles = cmd.data[0];

								 TRACE(cpfIterator,"%s, found:<%d> subfiles under <%s>", __func__, numOfSubfiles, (*itFileList).c_str() );

								 for(int idx = 1; idx <= numOfSubfiles; ++idx)
								 {
									 std::string subFileName(cmd.data[idx]);

									 std::string fileDn(immTag::InfiniteSubFileKey);
									 fileDn.append(subFileName);
									 fileDn.push_back(Comma);
									 fileDn.append(*itFileList);

									 m_subComFileIdMap.insert(std::make_pair(fileDn, subFileName));

									 // All subfile list
									 if(isSubFilesList)
									 {
										 subCompositeFileList.push_back(fileDn);
									 }
									 else if( subfile.compare(subFileName) == 0)
									 {
										 TRACE(cpfIterator,"%s, founded subfile:<%s>", __func__, subfile.c_str() );
										 // Specific subfile found
										 subCompositeFileList.push_back(fileDn);
										 return FMS_CPF_Exception::OK;
									 }
								 }
							 }
						 }
						 else
						 {
							 // error on retrieve info from CPF
							 TRACE(cpfIterator, "%s, Communication error with CPF server", __func__ );
						 }
					 }
					 catch(const FMS_CPF_Exception& ex)
					 {
						 TRACE(cpfIterator, "%s, Communication error:<%s> with CPF server", __func__, ex.errorText() );
					 }
				 }

				 //continue with next file
				 continue;
			 }

			 // Not an infinite file
			 if( m_OmHandler->loadChildInst((*itFileList).c_str(), ACS_APGCC_SUBLEVEL, &subFileList) != 0)
			 {
				 TRACE(cpfIterator, "loadDN(), failed to get subfile of object:<%s>", (*itFileList).c_str());
				 return FMS_CPF_Exception::GENERAL_FAULT;
			 }

			 std::string subFileName;
			 for(itList = subFileList.begin(); itList != subFileList.end();  ++itList )
			 {

				 // get sub file name
				 if(getLastFieldValue((*itList), subFileName))
				 {
					 m_subComFileIdMap.insert(std::make_pair(*itList,subFileName));

					 if(isSubFilesList)
					 {
						 // add the subfile to list, after sort it
						 VectorSubList.push_back(std::make_pair(subFileName, *itList));
					 }
					 else if(subfile.compare(subFileName) == 0)
					 {
						 TRACE(cpfIterator, "loadDN(), subfile:<%s> found, DN:<%s>", subfile.c_str(), (*itList).c_str());
						 subCompositeFileList.push_back(*itList);
						 subFileList.clear();
						 return FMS_CPF_Exception::OK;
					 }
				 }
				 else
				 {
					 TRACE(cpfIterator, "loadDN(), failed to get name of object:<%s>", (*itList).c_str());
					 return FMS_CPF_Exception::GENERAL_FAULT;
				 }
			 }

			 if(!VectorSubList.empty())
			 {
				 //Subfile sorting
				 sort(VectorSubList.begin(),VectorSubList.end());
				 std::vector<pair<std::string,std::string> >::iterator  iterFileList;
				 for(iterFileList = VectorSubList.begin(); iterFileList != VectorSubList.end(); ++iterFileList)
				 {
					 subCompositeFileList.push_back(iterFileList->second);
				 }
				 VectorSubList.clear();
			 }
			 subFileList.clear();
		 }
	}

    TRACE(cpfIterator, "loadDN() Final subCompositeFileList size =%zd, retCode = %d", subCompositeFileList.size(), retCode);
    return retCode;
}

bool FMS_CPF_FileIterator::getVolumeName(const std::string& volumeDN, std::string& volumeName)
{
	TRACE(cpfIterator, "getVolumeName(), volume DN:<%s>", volumeDN.c_str());
	bool result = false;
	volumeName.clear();

	// Get the volume RDN ( cpVolumeId=<volumeName>:<cpName>) from DN
	// Split the field in RDN and Value
	size_t equalPos = volumeDN.find_first_of(Equal);
	size_t commaPos = volumeDN.find_first_of(Comma);

	// Check if some error happens
	if( (std::string::npos != equalPos) )
	{
		std::string volumeRDN;

		// check for a single field case
		if( std::string::npos == commaPos )
			volumeRDN = volumeDN.substr(equalPos + 1);
		else
			volumeRDN = volumeDN.substr(equalPos + 1, (commaPos - equalPos - 1) );

		if(m_bIsSysBC)
		{
			size_t tagAtSignPos = volumeRDN.find(AtSign);

			// Check if the tag is present
			if( std::string::npos != tagAtSignPos )
			{
				// get the volumeName
				volumeName = volumeRDN.substr(0, tagAtSignPos);
				ACS_APGCC::toUpper(volumeName);
				result = true;
			}
		}
		else
		{
			// RDN is the volume name in SCP
			volumeName = volumeRDN;
			ACS_APGCC::toUpper(volumeName);
			result = true;
		}
    }

    TRACE(cpfIterator, "getVolumeName(), value:<%s>", volumeName.c_str());
    return result;
}

//------------------------------------------------------------------------------
//      Get next file entry
//------------------------------------------------------------------------------
bool FMS_CPF_FileIterator::getNext(FMS_CPF_FileIterator::FMS_CPF_FileData& fd)
throw (FMS_CPF_Exception)
{
	bool result = false;
	// Always false on linux is not avieable
    fd.compressed = false;
    fd.valid = false;

    fd.recordLength = 0;
    fd.file_size = 0;
    fd.mode = FMS_CPF_Types::tm_UNDEF;
    fd.maxsize = 0;
    fd.maxtime = 0;
    fd.active = 0;
    fd.release = false;
    fd.tqname.clear();
    fd.ucount = 0;
    fd.rcount = 0;
    fd.wcount = 0;


    if( (compositeFileList.size() != 0U) && !listSubiles_)
    {
        	// get File DN of the first file
    	std::string fileDN = compositeFileList.front();
        
    	// check file type from DN
        fd.composite = true;
        if( fileDN.find(immTag::InfiniteFileKey) != std::string::npos)
        {
        	fd.ftype = FMS_CPF_Types::ft_INFINITE;
        }
        else
        {
        	fd.ftype = FMS_CPF_Types::ft_REGULAR;
        	// check for simple file
        	if( fileDN.find(immTag::SimpleFileKey) != std::string::npos )
        	{
        		fd.composite = false;
        	}
        }

        TRACE(cpfIterator, "getNext(), fileDN:<%s>, composite:<%s>, type:<%d>", fileDN.c_str(), (fd.composite ? "YES" : "NO"), fd.ftype );

        if(longList_)
        {
        	TRACE(cpfIterator, "%s", "getNext()(), long list is true");
        	getFileExtraInfo(fileDN, fd);
        }

        // Get the file name from DN
        mapFileId::const_iterator element = m_compositeFileIdMap.find(fileDN);

        if(m_compositeFileIdMap.end() != element)
        	fd.fileName = element->second;

        // get volume name of the file
	    std::string volumeDN;
	    getParentDN(fileDN, volumeDN);

	    element = m_volumeNameMap.find(volumeDN);

	    if(m_volumeNameMap.end() != element)
	    	fd.volume = element->second;

	    fd.valid = true;

	    TRACE(cpfIterator, "getNext(), only file info: fileName = %s, maxtime = %d, maxsize = %d, active = %d, release = %d,recordLength = %d, num of READERS = %d, num of WRITERS= %d, exclusive access = %d, size = %d, TQ_name =%s, mode = %d",
						fd.fileName.c_str(),
						fd.maxtime,
						fd.maxsize,
						fd.active,
						fd.release,
						fd.recordLength,
						fd.rcount,
						fd.wcount,
						fd.ucount,
						fd.file_size,
						fd.tqname.c_str(),
						fd.mode);

        // set current father file
        curCompFile = fd.fileName;

        // Store info to internal structure
        this->fd.fileName = fd.fileName;
        this->fd.recordLength = fd.recordLength;
        this->fd.maxsize = fd.maxsize;
        this->fd.maxtime = fd.maxtime;
        this->fd.active = fd.active;
        this->fd.release = fd.release;
        this->fd.tqname = fd.tqname;
        this->fd.mode = fd.mode;
        this->fd.ucount = fd.ucount;
        this->fd.rcount = fd.rcount;
        this->fd.wcount = fd.wcount;
        this->fd.volume = fd.volume;
        this->fd.ftype = fd.ftype;

        //remove the first element from the main file list
        compositeFileList.erase(compositeFileList.begin());

       result = true;
    }
    else
    {
    	if(listSubiles_)
        {
    		if(subCompositeFileList.size() != 0U)
    		{
    			std::string mainFile;
    			std::string subfile;
    			std::string fileDN;

    			// get First subfile into the list
    			std::string subFileDN = subCompositeFileList.front();

    			getParentDN(subFileDN, fileDN);

    			mapFileId::const_iterator fileIt = m_compositeFileIdMap.find(fileDN);

  			    if (fileIt != m_compositeFileIdMap.end())
				{
					mainFile = fileIt->second;
				}
				else
				{
					TRACE(cpfIterator, "getNext()SUBFILES - the fileIdMap map doesn't contain the key =%s", fileDN.c_str());
				    return false;
				}

                fileIt = m_subComFileIdMap.find(subFileDN);
	            if (fileIt != m_subComFileIdMap.end())
	            {
	        	   subfile = fileIt->second;
	            }
	            else
	            {
	               TRACE(cpfIterator, "getNext()SUBFILES - the subfileIdMap map doesn't contain the key =%s", subFileDN.c_str());
	               throw FMS_CPF_Exception (FMS_CPF_Exception::GENERAL_FAULT);
	            }

	            if(curCompFile.compare(mainFile) != 0)
					  return false;

			   // true because there are subfiles
			   fd.composite = true;

			   if(this->fd.fileName.compare(mainFile) != 0)
			   {
				   TRACE(cpfIterator, "getNext(), before get info on mainFile:<%s>",  mainFile.c_str() );

				   this->fd.fileName = mainFile;
				   this->fd.composite = true;
				   this->fd.recordLength = 0;
				   this->fd.maxsize = 0;
				   this->fd.maxtime =0;
				   this->fd.active= 0;
				   this->fd.release = false;
				   this->fd.tqname.clear();
				   this->fd.mode = FMS_CPF_Types::tm_UNDEF;

				   // check that is an infinite or regular file
				   this->fd.ftype = FMS_CPF_Types::ft_REGULAR;
				   if( fileDN.find(immTag::InfiniteFileKey) != std::string::npos)
				   {
					   this->fd.ftype = FMS_CPF_Types::ft_INFINITE;
				   }

				   if(longList_)
				   {
					   TRACE(cpfIterator, "%s", "getNext(), extra info on mainFile");
					   getFileExtraInfo(fileDN, this->fd);
				   }

				   // get volume name of the file
				   std::string volumeDN;
				   getParentDN(fileDN, volumeDN);

				   mapFileId::const_iterator element = m_volumeNameMap.find(volumeDN);

				   if(m_volumeNameMap.end() != element)
					   this->fd.volume = element->second;
			   }

			   // set info from the main file
			   fd.recordLength = this->fd.recordLength;
			   fd.maxsize = this->fd.maxsize;
			   fd.maxtime = this->fd.maxtime;
			   fd.active = this->fd.active;
			   fd.release = this->fd.release;
			   fd.tqname = this->fd.tqname;
			   fd.mode = this->fd.mode;
			   fd.ftype = this->fd.ftype;
			   fd.volume = this->fd.volume;

			   fd.fileName.assign(mainFile);
			   fd.fileName.push_back(Dash);
			   fd.fileName.append(subfile);

			   // Check file type
			   if(FMS_CPF_Types::ft_INFINITE == fd.ftype)
			   {
				   ACS_APGCC_Command cmd;
				   cmd.cmdCode = CPF_API_Protocol::GET_USERS;
				   cmd.data[0] = fd.fileName;
				   cmd.data[1] = m_CpName;

				   try
				   {
					   // send command to CPF server to get all subfiles
					   if(0 == m_DsdClient.execute(cmd) && (FMS_CPF_Exception::OK == cmd.result) )
					   {
							fd.rcount = cmd.data[0];
							fd.wcount = cmd.data[1];
							fd.ucount = cmd.data[2];
							fd.file_size = cmd.data[3];
					   }
					   else
					   {
						   // failed to get info
						   TRACE(cpfIterator, "%s, request to CPF server failed", __func__ );
					   }
				   }
				   catch(const FMS_CPF_Exception& ex)
				   {
					   TRACE(cpfIterator, "%s, Communication error:<%s> with CPF server", __func__, ex.errorText() );
				   }
			   }

			   if(longList_ && (FMS_CPF_Types::ft_INFINITE != fd.ftype))
			   {
				   // get extra info of subfile
				   getSubFileExtraInfo(subFileDN, fd);
			   }

			   TRACE(cpfIterator, "getNext(), main file info ,fileName = %s, maxtime = %d, maxsize = %d, active = %d, release = %d, recordLength = %d, num of READERS = %ld, num of WRITERS= %d, exclusive access = %d, size = %d, transferQueue = %s, mode = %d",
								fd.fileName.c_str(),
								fd.maxtime,
								fd.maxsize,
								fd.active,
								fd.release,
								fd.recordLength,
								fd.rcount,
								fd.wcount,
								fd.ucount,
								fd.file_size,
								fd.tqname.c_str(),
								fd.mode);

				fd.valid = true;

				//remove the first element from the sub-file list
				subCompositeFileList.erase(subCompositeFileList.begin());
				result = true;
    	    }
    		else
    		{
    			TRACE(cpfIterator, "%s", "getNext(), subfile list is empty");
    		}
        }
    	else
    	{
    		 TRACE(cpfIterator, "%s", "getNext(), file list is empty");
    	}
    }

    return result;
}


void FMS_CPF_FileIterator::readConfiguration() throw(FMS_CPF_Exception)
{
	TRACE(cpfIterator, "%s", "Entering in readConfiguration()");
    FMS_CPF_ConfigReader myReader;
    myReader.init();
    m_bIsSysBC = myReader.IsBladeCluster();

    if(m_bIsSysBC)
    {
    	 TRACE(cpfIterator, "%s", "readConfiguration(), MCP system");
    	 ACS_APGCC::toUpper(m_CpName);

    	 if(!m_CpName.empty())
         {
    		 // Search Cp from defined CP
         	 std::list<std::string> listOfCP = myReader.getCP_List();
        	 std::list<std::string>::const_iterator element;
        	 bool found = false;
        	 for(element = listOfCP.begin(); element != listOfCP.end(); ++element)
        	 {
        	 	 if(m_CpName.compare(*element) == 0)
        		 {
        			 found = true;
        			 break;
        		 }
        	 }

        	 if(!found)
        	 {
        		 TRACE(cpfIterator, "readConfiguration(), Cp Name:<%s> not found", m_CpName.c_str() );
        		 throw FMS_CPF_Exception(FMS_CPF_Exception::CPNOTEXISTS);
        	 }
        }
        else
        {
            TRACE(cpfIterator, "%s", "readConfiguration(), Cp Name is empty");
            throw FMS_CPF_Exception (FMS_CPF_Exception::CPNAMENOPASSED);
        }
    }
    else
    {
    	 TRACE(cpfIterator, "%s", "readConfiguration(), SCP system");
    	 if(!m_CpName.empty())
         {
    		 TRACE(cpfIterator, "readConfiguration(), Cp Name<%s> is not empty", m_CpName.c_str());
             throw FMS_CPF_Exception(FMS_CPF_Exception::ILLOPTION);
         }
    }
    TRACE(cpfIterator, "%s", "Leaving readConfiguration()");
}

bool FMS_CPF_FileIterator::getLastFieldValue(const std::string& objectDN, std::string& fieldValue)
{
	bool result = false;
    // Get the file name from DN
	// Split the field in RDN and Value
	size_t equalPos = objectDN.find_first_of(Equal);
	size_t commaPos = objectDN.find_first_of(Comma);

	// Check if some error happens
	if( (std::string::npos != equalPos) && (std::string::npos != commaPos) )
	{
		fieldValue = objectDN.substr(equalPos + 1, (commaPos - equalPos - 1) );
		// make the name in upper case
		ACS_APGCC::toUpper(fieldValue);
		result = true;
	}

	return result;
}

void FMS_CPF_FileIterator::getParentDN(const std::string& objectDN, std::string& parentDN)
{
	size_t posOfcom;
	if((posOfcom = objectDN.find(Comma)) != std::string::npos)
	{
		parentDN = objectDN.substr(posOfcom+1);
	}
	else
	{
		parentDN.clear();
		TRACE(cpfIterator, "getParentDN(), error on get parent of object DN:<%s>", objectDN.c_str());
	}

}

void FMS_CPF_FileIterator::getFileExtraInfo(const std::string& fileDN, FMS_CPF_FileData& fileData)
{
	ACS_APGCC_ImmAttribute recordLengthAttr;
	recordLengthAttr.attrName = immTag::recordLengthAttribute;

	ACS_APGCC_ImmAttribute sizeAttr;
	sizeAttr.attrName = immTag::sizeAttribute;

	ACS_APGCC_ImmAttribute maxTimeAttr;
	maxTimeAttr.attrName = immTag::maxTimeAttribute;

	ACS_APGCC_ImmAttribute maxSizeAttr;
	maxSizeAttr.attrName = immTag::maxSizeAttribute;

	ACS_APGCC_ImmAttribute activeAttr;
	activeAttr.attrName = immTag::activeSubFileAttribute;

	ACS_APGCC_ImmAttribute releaseAttr;
	releaseAttr.attrName = immTag::releaseCondAttribute;

	ACS_APGCC_ImmAttribute numWritersAttr;
	numWritersAttr.attrName = immTag::numWritersAttribute;

	ACS_APGCC_ImmAttribute numReadersAttr;
	numReadersAttr.attrName = immTag::numReadersAttribute;

	ACS_APGCC_ImmAttribute exclusiveAccessAttr;
	exclusiveAccessAttr.attrName = immTag::exclusiveAccessAttribute;

	ACS_APGCC_ImmAttribute transferFilePolicyAttr;
	transferFilePolicyAttr.attrName = immTag::fileTQAttribute;

    std::vector<ACS_APGCC_ImmAttribute*> attributeList;

    attributeList.push_back(&recordLengthAttr);
    attributeList.push_back(&numWritersAttr);
    attributeList.push_back(&numReadersAttr);
    attributeList.push_back(&exclusiveAccessAttr);

	if(fileData.ftype == FMS_CPF_Types::ft_INFINITE)
	{
		attributeList.push_back(&maxSizeAttr);
		attributeList.push_back(&maxTimeAttr);
		attributeList.push_back(&activeAttr);
		attributeList.push_back(&releaseAttr);
		attributeList.push_back(&transferFilePolicyAttr);
	}
	else if(!fileData.composite)
	{
		attributeList.push_back(&sizeAttr);
	}

	if(m_OmHandler->getAttrListVal(fileDN, attributeList) != 0)
	{
		TRACE(cpfIterator, "getFileExtraInfo(), attribute list size:<%zd>, DN:<%s> failed", attributeList.size(), fileDN.c_str() );
		throw FMS_CPF_Exception (FMS_CPF_Exception::GENERAL_FAULT);
	}

	std::vector<ACS_APGCC_ImmAttribute*>::const_iterator attributeIdx;

	for(attributeIdx = attributeList.begin(); attributeIdx != attributeList.end(); ++attributeIdx)
	{
		ACS_APGCC_ImmAttribute* attr = (*attributeIdx);
		switch(s_mapStringValues[attr->attrName])
		{
			case maxtime:
			{
				if(attr->attrValuesNum != 0)
					fileData.maxtime = *reinterpret_cast<unsigned int*>(attr->attrValues[0]);
			}
			break;

			case maxsize:
			{
				if(attr->attrValuesNum != 0)
					fileData.maxsize = *reinterpret_cast<unsigned int*>(attr->attrValues[0]);
			}
			break;

			case activeSubfile:
			{
				if(attr->attrValuesNum != 0)
					fileData.active = *reinterpret_cast<unsigned int*>(attr->attrValues[0]);
			}
			break;

			case release:
			{
				if(attr->attrValuesNum !=0 )
					fileData.release = *reinterpret_cast<unsigned int*>(attr->attrValues[0]);
			}
			break;

			case recordLength:
			{
				if(attr->attrValuesNum !=0 )
					fileData.recordLength = *reinterpret_cast<unsigned int*>(attr->attrValues[0]);
			}
			break;

			case numReaders:
			{
				if(attr->attrValuesNum!=0)
					fileData.rcount = *reinterpret_cast<unsigned int*>(attr->attrValues[0]);
			}
			break;

			case numWriters:
			{
				if(attr->attrValuesNum != 0)
					fileData.wcount = *reinterpret_cast<unsigned int*>(attr->attrValues[0]);
			}
			break;

			case exclusiveAccess:
			{
				if(attr->attrValuesNum != 0)
					fileData.ucount = *reinterpret_cast<int*>(attr->attrValues[0]);
			}
			break;

			case size:
			{
				if(attr->attrValuesNum != 0 )
					fileData.file_size = *reinterpret_cast<unsigned int*>(attr->attrValues[0]);
			}
			break;

			case transferFilePolicy:
			{
				if(attr->attrValuesNum != 0)
				{
					std::string fileTQ = reinterpret_cast<char*>(attr->attrValues[0]);

					// Check the type of TQ and set in the correct way "mode" field
					const unsigned int call_result = AES_OHI_BlockHandler2::blockTransferQueueDefined(fileTQ.c_str());

					if ( call_result == AES_OHI_NOERRORCODE )
						fileData.mode = FMS_CPF_Types::tm_BLOCK;
					else
						fileData.mode = FMS_CPF_Types::tm_FILE;

					ACS_APGCC::toUpper(fileTQ);
					fileData.tqname = fileTQ;
				}
			}
			break;

			default:
				  TRACE(cpfIterator, "getFileExtraInfo(), key:<%s> not found", attr->attrName.c_str());
		}
	}
}

void FMS_CPF_FileIterator::getSubFileExtraInfo(const std::string& subFileDN, FMS_CPF_FileData& fileData)
{
	std::vector<ACS_APGCC_ImmAttribute*> attributeList;

	ACS_APGCC_ImmAttribute sizeAttr;
	sizeAttr.attrName = immTag::sizeAttribute;
	attributeList.push_back(&sizeAttr);

	ACS_APGCC_ImmAttribute numWritersAttr;
	numWritersAttr.attrName = immTag::numWritersAttribute;
	attributeList.push_back(&numWritersAttr);

	ACS_APGCC_ImmAttribute numReadersAttr;
	numReadersAttr.attrName = immTag::numReadersAttribute;
	attributeList.push_back(&numReadersAttr);

	ACS_APGCC_ImmAttribute exclusiveAccessAttr;
	exclusiveAccessAttr.attrName = immTag::exclusiveAccessAttribute;
	attributeList.push_back(&exclusiveAccessAttr);

	if(m_OmHandler->getAttrListVal(subFileDN, attributeList) == 0)
	{
		std::vector<ACS_APGCC_ImmAttribute*>::const_iterator attributeIdx;

		for(attributeIdx = attributeList.begin(); attributeIdx != attributeList.end(); ++attributeIdx)
		{
			ACS_APGCC_ImmAttribute* attr = (*attributeIdx);
			switch(s_mapStringValues[attr->attrName])
			{
				case numReaders:
				{
					if(attr->attrValuesNum!=0)
						fileData.rcount = *reinterpret_cast<unsigned int*>(attr->attrValues[0]);
				}
				break;

				case numWriters:
				{
					if(attr->attrValuesNum != 0)
						fileData.wcount = *reinterpret_cast<unsigned int*>(attr->attrValues[0]);
				}
				break;

				case exclusiveAccess:
				{
					if(attr->attrValuesNum != 0)
						fileData.ucount = *reinterpret_cast<int*>(attr->attrValues[0]);
				}
				break;

				case size:
				{
					if(attr->attrValuesNum != 0 )
						fileData.file_size= *reinterpret_cast<unsigned int*>(attr->attrValues[0]);
				}
				break;
				default:
					TRACE(cpfIterator, "getSubFileExtraInfo(), key:<%s> not found", attr->attrName.c_str());

			}
		}
	}
	else
	{
		TRACE(cpfIterator, "getSubFileExtraInfo(), failed to get attributes of file:<%s>", subFileDN.c_str());
	}
}

void FMS_CPF_FileIterator::setSubFiles(bool bVal)
{
	listSubiles_ = bVal;
}

void FMS_CPF_FileIterator::initialize()
{
	s_mapStringValues[immTag::maxTimeAttribute] = maxtime;
	s_mapStringValues[immTag::maxSizeAttribute] = maxsize;
	s_mapStringValues[immTag::activeSubFileAttribute] = activeSubfile;
	s_mapStringValues[immTag::releaseCondAttribute] = release;
	s_mapStringValues[immTag::recordLengthAttribute] = recordLength;
	s_mapStringValues[immTag::numReadersAttribute] = numReaders;
	s_mapStringValues[immTag::numWritersAttribute] = numWriters;
	s_mapStringValues[immTag::exclusiveAccessAttribute] = exclusiveAccess;
	s_mapStringValues[immTag::sizeAttribute] = size;
	s_mapStringValues[immTag::fileTQAttribute] = transferFilePolicy;
}
