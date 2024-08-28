//**********************************************************************
//
/*
 * * @file fms_cpf_file.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_File.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_file.h module
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

#include "fms_cpf_file.h"
#include "fms_cpf_privateexception.h"
#include "fms_cpf_attribute.h"
#include "fms_cpf_apdcommonfile.h"
#include "fms_cpf_eventhandler.h"
#include "fms_cpf_configreader.h"
#include "fms_cpf_api_trace.h"
#include "fms_cpf_omcmdhandler.h"
#include "fms_cpf_adminoperation.h"

#include <ACS_APGCC_Command.H>
#include "ACS_APGCC_Util.H"
#include "ACS_TRA_trace.h"

#include <ace/ACE.h>
#include <aes_ohi_extfilehandler2.h>
#include "aes_ohi_blockhandler2.h"
#include "aes_ohi_errorcodes.h"

#include <sys/time.h>


const FMS_CPF_File FMS_CPF_File::EOL;

namespace{

	const std::string cpFileSystemRoot("AxeCpFileSystemcpFileSystemMId=1");
	const std::string CpVolumeIdTag("cpVolumeId=");

	const std::string INF_FILE_TAG("infiniteFileId=");
	const std::string SIMPLE_FILE_TAG("simpleFileId=");
	const std::string CompositeFileId("compositeFileId=");

	const char AtSign = ':';
	const char Comma = ',';
	const char Equal = '=';
	const char DirDelim = '/';
	const char IDENTIFIER[] = "_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	const int ELEMENT_NOT_EXIST = -12;
	const int ELEMENT_ALREADY_EXIST = -14;
}

namespace OHI_USERSPACE{

	const std::string SUBSYS("FMS");
	const std::string APPNAME("CPF");
	const char TQCHARS[] = "_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
}

namespace actionParameters {
	char srcCpFile[] = "sourceCpFile";
	char dstCpFile[] = "destCpFile";
	char source[] = "source";
	char destination[] = "dest";
	char volume[] = "volume";
	char mode[]= "mode";
	char zip[]= "toZip";

	char currentName[] = "currentName";
	char newName[] = "newName";
	char cpName[] = "cpName";
}

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_File::FMS_CPF_File() throw(FMS_CPF_Exception)
:filename_(),
 volume_(),
 reference_(0),
 m_bIsSysBC(false),
 m_bCPExists(false),
 m_bIsConfigRead(false),
 m_nCP_ID(0),
 m_nNumCP(0)
{
	fmsCpfFileTrace = new ACS_TRA_trace("FMS_CPF_File");
	ACE_OS::memset(m_pCPName, 0, 20);
}

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_File::FMS_CPF_File(const char* filename, const char* pCPName) throw(FMS_CPF_Exception)
:filename_(filename),
 volume_(),
 reference_(0),
 m_bIsSysBC(false),
 m_bIsConfigRead(false),
 m_nCP_ID(0),
 m_nNumCP(0)
{
	fmsCpfFileTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_File");

	std::string cpUp(pCPName);
	ACS_APGCC::toUpper(cpUp);
	readConfiguration(cpUp.c_str());
}

//------------------------------------------------------------------------------
//      Copy constructor
//------------------------------------------------------------------------------
FMS_CPF_File::FMS_CPF_File(const FMS_CPF_File& file) throw(FMS_CPF_Exception)
:filename_(file.filename_),
 volume_(),
 reference_(0)
{
	m_bIsConfigRead = file.m_bIsConfigRead;
	m_bIsSysBC = file.m_bIsSysBC;
	m_bCPExists = file.m_bCPExists;

	m_nNumCP = file.m_nNumCP;
	m_nCP_ID = file.m_nCP_ID;

	ACE_OS::memset(m_pCPName, 0, 20);
	strncpy(m_pCPName, file.m_pCPName, 19);

	m_strListCP = file.m_strListCP;

	fmsCpfFileTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_File");
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FMS_CPF_File::~FMS_CPF_File ()
{
	if (reference_ != 0)
	{
		try
		{
			unreserve();
		}
		catch(FMS_CPF_Exception& ex)
		{
			FMS_CPF_EventHandler::instance()->event(ex);
		}
	}

	if(NULL != fmsCpfFileTrace)
		delete fmsCpfFileTrace;
}

//------------------------------------------------------------------------------
//      Assignment operator
//------------------------------------------------------------------------------
const FMS_CPF_File& FMS_CPF_File::operator= (const FMS_CPF_File& file) throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entring in operator=()");
	if (reference_ != 0)
	{
		FMS_CPF_Exception exErr(FMS_CPF_Exception::FILEISOPEN);
		std::string errMsg(exErr.errorText());
		errMsg += filename_;

		throw FMS_CPF_Exception(FMS_CPF_Exception::FILEISOPEN, errMsg);
	}

	filename_ = file.filename_;

	TRACE(fmsCpfFileTrace, "%s", "Leaving operator=()");
	return file;
}

//------------------------------------------------------------------------------
//      Equality operator
//------------------------------------------------------------------------------
int FMS_CPF_File::operator== (const FMS_CPF_File& file) const
{
	return filename_ == file.filename_;
}

//------------------------------------------------------------------------------
//      Unequality operator
//------------------------------------------------------------------------------
int FMS_CPF_File::operator!= (const FMS_CPF_File& file) const
{
	return filename_ != file.filename_;
}

//------------------------------------------------------------------------------
//      Reserve (open) file
//------------------------------------------------------------------------------
void FMS_CPF_File::reserve(FMS_CPF_Types::accessType access) throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace,"%s","Entering in reserve()");
	//Check if the file is already opened
	if (reference_ != 0)
	{
		TRACE(fmsCpfFileTrace,"%s","reserve(), file already opened");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::FILEISOPEN);
		std::string errMsg(exErr.errorText());
		errMsg += filename_;
		throw FMS_CPF_Exception(FMS_CPF_Exception::FILEISOPEN, errMsg);
	}

	FMS_CPF_FileId fileid(filename_);
	//Check if the file name is valid
	if (!fileid.isValid ())
	{
		TRACE(fmsCpfFileTrace,"reserve(), invalid file name:<%s>", filename_.c_str());
		throw FMS_CPF_Exception(FMS_CPF_Exception::INVALIDFILE);
	}

	TRACE(fmsCpfFileTrace, "reserve(), Send to server, cmd = OPEN_, filename = %s, access = %u",
							filename_.c_str(),
							(unsigned long)access);

	std::string cpname(m_pCPName);

	ACS_APGCC_Command cmd;
	cmd.cmdCode = CPF_API_Protocol::OPEN_;
	cmd.data[0] = filename_;
	cmd.data[1] = (unsigned long)access;

	if (m_bIsSysBC)
	{
		// Check cpName in MCP
		m_bCPExists = isCP();
		if(!m_bCPExists)
		{
			throw FMS_CPF_Exception(FMS_CPF_Exception::CPNOTEXISTS, "");
		}

		if(cpname.empty())
		{
			throw FMS_CPF_Exception(FMS_CPF_Exception::CPNAMENOPASSED, "");
		}

		cmd.data[2] = cpname;
		cmd.data[3] = m_nCP_ID;
	}

	int result  = DsdClient.execute(cmd);

	// Start management result
	if (result == 0)
	{
		if (cmd.result ==  FMS_CPF_Exception::OK)
		{
			// File reserved
			TRACE(fmsCpfFileTrace,"reserve(), File= %s, reserved OK", filename_.c_str() );
			reference_ =  cmd.data[0];
		}
		else
		{
			// File not reserved
			std::string errorText(cmd.data[0]);
			std::string detailInfo(cmd.data[1]);
			TRACE(fmsCpfFileTrace,"reserve(), Exeption!, Error text = %s, detailed info = %s",
								   errorText.c_str(),
								   detailInfo.c_str() );
			std::string errMsg = errorText + detailInfo;
			
			throw FMS_CPF_Exception(FMS_CPF_Exception::errorType(cmd.result), errMsg);
		}
	}
	else
	{
		TRACE(fmsCpfFileTrace, "%s", "reserve(), Exeption!, Connection broken");

		FMS_CPF_Exception exErr(FMS_CPF_Exception::SOCKETERROR);
		std::string errMsg(exErr.errorText());
		errMsg += "Connection broken";

		throw FMS_CPF_Exception(FMS_CPF_Exception::SOCKETERROR, errMsg);
	}
	TRACE(fmsCpfFileTrace,"%s","Leaving reserve()");
}

//------------------------------------------------------------------------------
//      Unreserve (close) file
//------------------------------------------------------------------------------
void FMS_CPF_File::unreserve() throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in unreserve()");

	if(0 != reference_)
	{
		std::string cpname(m_pCPName);

		TRACE(fmsCpfFileTrace, "unreserve(), Send to server, cmd = CLOSE_, filename:<%s>", filename_.c_str() );

		ACS_APGCC_Command cmd;
		cmd.cmdCode = CPF_API_Protocol::CLOSE_;
		cmd.data[0] = reference_;

		if (m_bIsSysBC)
		{
			TRACE(fmsCpfFileTrace, "unreserve(), Cp Name:<%s>", m_pCPName);
			m_bCPExists = isCP();
			if (!m_bCPExists)
			{
				throw FMS_CPF_Exception (FMS_CPF_Exception::CPNOTEXISTS, "");
			}

			if (cpname.empty())
			{
				throw FMS_CPF_Exception (FMS_CPF_Exception::CPNAMENOPASSED, "");
			}

			cmd.data[1] = cpname;
			cmd.data[2] = m_nCP_ID;
		}

		int result = DsdClient.execute(cmd);

		if(result == 0)
		{
			if (cmd.result ==  FMS_CPF_Exception::OK)
			{
				TRACE(fmsCpfFileTrace, "%s", "unreserve(), file closed");
			}
			else
			{
				reference_ = 0;
				std::string errorText(cmd.data[0]);
				std::string detailInfo( cmd.data[1]);
				TRACE(fmsCpfFileTrace, "unreserve(), Exception!, Error text = %s, detailed info = %s",
																						errorText.c_str(),
																						detailInfo.c_str() );
				std::string errMsg = errorText + detailInfo;
				throw FMS_CPF_Exception(FMS_CPF_Exception::errorType(cmd.result), errMsg);
			}
		}
		else
		{
			TRACE(fmsCpfFileTrace, "%s", "unreserve(), Exception!, Connection broken");

			FMS_CPF_Exception exErr(FMS_CPF_Exception::SOCKETERROR);
			std::string errMsg(exErr.errorText());
			errMsg += "Connection broken";

			throw FMS_CPF_Exception(FMS_CPF_Exception::SOCKETERROR, errMsg);
		}

		reference_ = 0;
	}
	TRACE(fmsCpfFileTrace, "%s", "Leaving unreserve()");
}

//------------------------------------------------------------------------------
//      Create file
//------------------------------------------------------------------------------
void FMS_CPF_File::create(const FMS_CPF_Types::fileAttributes& fileattr, const char* volume, bool compress)
throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in create(), main file");
	UNUSED(compress);
	if (reference_ != 0)
	{
		TRACE(fmsCpfFileTrace, "create(), error file:<%s> is open", filename_.c_str());
		FMS_CPF_Exception exErr(FMS_CPF_Exception::FILEISOPEN);
		std::string errMsg(exErr.errorText());
		errMsg += filename_;
		throw FMS_CPF_Exception (FMS_CPF_Exception::FILEISOPEN, errMsg);
	}

	FMS_CPF_FileId fileid(filename_);

	TRACE(fmsCpfFileTrace, "create(), file to create:<%s>", filename_.c_str());

	if(!fileid.subfile().empty())
	{
		TRACE(fmsCpfFileTrace, "%s", "create(), call with a subfile not allowed");

		FMS_CPF_Exception exErr(FMS_CPF_Exception::NOTBASEFILE);
		std::string errMsg(exErr.errorText());
		errMsg += filename_;
		throw FMS_CPF_Exception (FMS_CPF_Exception::NOTBASEFILE, errMsg);
	}

	std::string tempVolume(volume);

	// Convert to uppercase
	ACS_APGCC::toUpper(tempVolume);

	if (!checkVolName(tempVolume))
	{
		TRACE(fmsCpfFileTrace, "create(), invalid volume name:<%s>", volume);
        // here there is a little bit tricky, volume not found is incorrect here
		// but in some cases this message is that to does for us

		FMS_CPF_Exception exErr(FMS_CPF_Exception::VOLUMENOTFOUND);
		std::string errMsg(exErr.errorText());
		errMsg += volume;

		throw FMS_CPF_Exception (FMS_CPF_Exception::VOLUMENOTFOUND, errMsg);
	}

	volume_ = tempVolume;

	switch(fileattr.ftype)
	{
		  case FMS_CPF_Types::ft_REGULAR:
		  {
			   // check for a composite file
			   if(fileattr.regular.composite)
			   {
				   TRACE(fmsCpfFileTrace, "%s", "create(), regular composite file creation");
				  // Composite file
				   writeCompositeFile(fileid, fileattr);
			   }
			   else
			   {
				   // simple file
				   TRACE(fmsCpfFileTrace, "%s", "create(), regular simple file creation");
				   writeSimpleFile(fileid, fileattr.regular.rlength);
			   }
		  }
		  break;

		  case FMS_CPF_Types::ft_INFINITE:
		  {
			   TRACE(fmsCpfFileTrace, "%s", "create(), infinite file creation");
			   writeInfiniteFile(fileid, fileattr);
		  }
		  break;

		  default:
			  throw FMS_CPF_Exception(FMS_CPF_Exception::GENERAL_FAULT);
	}

	TRACE(fmsCpfFileTrace, "%s", "Leaving create()");
}

//------------------------------------------------------------------------------
//      Create subfile
//------------------------------------------------------------------------------
void FMS_CPF_File::create(bool compress) throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "Entering in create(), subfile:<%s>", filename_.c_str());

	std::string subFileName;
	std::string mainfilename;
	UNUSED(compress);

	if( 0 != reference_ )
	{
		TRACE(fmsCpfFileTrace, "create(), file<%s> is already open", filename_.c_str());

		FMS_CPF_Exception exErr(FMS_CPF_Exception::FILEISOPEN);
		std::string errMsg(exErr.errorText());
		errMsg += filename_;

		throw FMS_CPF_Exception(FMS_CPF_Exception::FILEISOPEN, errMsg);
	}

	FMS_CPF_FileId fileid(filename_);

	// Check file name
	if( !fileid.isValid() )
	{
		TRACE(fmsCpfFileTrace, "create(), file name:<%s> is invalid", filename_.c_str() );
		throw FMS_CPF_Exception(FMS_CPF_Exception::INVALIDFILE);
	}

	// Check subfile part
	if( fileid.subfile().empty() )
	{
		TRACE(fmsCpfFileTrace, "%s", "create(), file is not a subfile");

		FMS_CPF_Exception exErr(FMS_CPF_Exception::NOTSUBFILE);
		std::string errMsg(exErr.errorText());
		errMsg += filename_;

		throw FMS_CPF_Exception(FMS_CPF_Exception::NOTSUBFILE, errMsg);
	}
	else
	{
		mainfilename = fileid.file();
		subFileName = fileid.subfileAndGeneration();
	}

	ACS_APGCC::toUpper(mainfilename);
	TRACE(fmsCpfFileTrace, "create(), composite file:<%s>, subfile:<%s>", mainfilename.c_str(), subFileName.c_str());

	std::string compositeFileDN;
	std::string volume = getVolumeInfo(mainfilename);
	getCompositeFileDN(volume, mainfilename, compositeFileDN);

   	TRACE(fmsCpfFileTrace, "create(), volume:<%s>, composite file DN:<%s>", volume.c_str(), compositeFileDN.c_str());

	// check file type
   	// only creation of composite subfile is allowed
	if(std::string::npos == compositeFileDN.find(CompositeFileId) )
	{
		// main file infinite file
		std::string errMsg;
		FMS_CPF_Exception::errorType error = FMS_CPF_Exception::TYPEERROR;

		if(std::string::npos != compositeFileDN.find(SIMPLE_FILE_TAG) )
		{
			// main file is a simple file
			error = FMS_CPF_Exception::NOTCOMPOSITE;
			FMS_CPF_Exception exErr(error);
			errMsg = exErr.errorText();
			errMsg += mainfilename;
		}

		throw FMS_CPF_Exception(error, errMsg);
	}

	// write the new object into IMM
	writeCompositeSubFile(compositeFileDN, subFileName);

   	TRACE(fmsCpfFileTrace, "%s", "Leaving create(), subfile created");
}

//------------------------------------------------------------------------------
//      Remove file
//------------------------------------------------------------------------------
void FMS_CPF_File::remove() throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in remove()");
	deleteFile(false);
	TRACE(fmsCpfFileTrace, "%s", "Leaving remove()");
}

//------------------------------------------------------------------------------
//      Delete file
//------------------------------------------------------------------------------
void FMS_CPF_File::deleteFile(bool recursive_) throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in deleteFile()");
	bool isInfinite = false;

	FMS_CPF_FileId fileid(filename_);
	if(!fileid.isValid())
	{
		TRACE(fmsCpfFileTrace, "deleteFile(), invalid file name:<%s>", filename_.c_str());
		throw FMS_CPF_Exception(FMS_CPF_Exception::INVALIDFILE);
	}

	// if the file does not exist an exception filenotfound will be raised
	std::string volume = getVolumeInfo(filename_);

	if (m_bIsSysBC)
	{
		std::string cpname(m_pCPName);
		if (cpname.empty())
		{
			throw FMS_CPF_Exception(FMS_CPF_Exception::CPNAMENOPASSED, "CP Name is not passed");
		}

		m_bCPExists = isCP();

		if (!m_bCPExists)
		{
			throw FMS_CPF_Exception(FMS_CPF_Exception::CPNOTEXISTS, "CP is not defined");
		}
	}

	std::string objectDn;
	int result = 0;

	getCompositeFileDN(volume, fileid.file(), objectDn);

	FMS_CPF_omCmdHandler OMHandler;
	std::string childDN;
	std::vector<std::string> listOfDN;
	int numOfSubfiles = 0;
	std::vector<std::string>::iterator element;

	// if the main file is an infinite file sub files cannot must be considered
	if( std::string::npos != objectDn.find(INF_FILE_TAG) )
	{
		isInfinite = true; // is an infinite file
		TRACE(fmsCpfFileTrace, "deleteFile(), infinite file DN=%s",objectDn.c_str());
	}
	else
	{
		// Load the list of subfiles (DNs list)
		OMHandler.loadChildInst(objectDn.c_str(), ACS_APGCC_SUBLEVEL, &listOfDN);
		numOfSubfiles = static_cast<int>(listOfDN.size());
		TRACE(fmsCpfFileTrace,"deleteFile(), file with %i subfiles", numOfSubfiles);
	}

	//checks if it is a subfile
	std::string subFileName = fileid.subfileAndGeneration();
	if( subFileName.empty() )
	{
		// Delete a Main or Simple file
		if( ((numOfSubfiles != 0) && (!recursive_)) || ((isInfinite) && (!recursive_)) )
		{
			// Composite file not empty and recursive flag is false
			TRACE(fmsCpfFileTrace,"%s", "deleteFile(), composite file not empty");

			FMS_CPF_Exception exErr(FMS_CPF_Exception::COMPNOTEMPTY);
			std::string errMsg(exErr.errorText());
			errMsg += filename_;

			throw FMS_CPF_Exception(FMS_CPF_Exception::COMPNOTEMPTY, errMsg);
		}

		if(isInfinite)
		{
			// Delete Infinite File and its subfile
			result = OMHandler.deleteObject(objectDn.c_str(), ACS_APGCC_SUBTREE);
		}
		else
		{
			// Delete composite or simple file
			// Delete subfiles
			for( element = listOfDN.begin(); element != listOfDN.end(); ++element)
			{
				childDN = *element;
				result = OMHandler.deleteObject(childDN.c_str());

				if(0 != result)
				{
					FMS_CPF_Exception::errorType errCode;
					std::string errMsg;

					// IMM error
					if(result < 0)
					{
						// Map to CPF error
						errCode = FMS_CPF_Exception::INTERNALERROR;

						if(ELEMENT_NOT_EXIST == result)
						{
							errCode =  FMS_CPF_Exception::UNABLECONNECT;
						}
					}
					else
					{
						// CPF custom error code
						errCode = static_cast<FMS_CPF_Exception::errorType>(result);
						errMsg = OMHandler.getLastImmError();
					}

					TRACE(fmsCpfFileTrace, "delete sub File() exitCode:<%d>, error Text:<%s>", result, OMHandler.getLastImmError().c_str());
					throw FMS_CPF_Exception(errCode, errMsg);
				}
				TRACE(fmsCpfFileTrace, "deleteFile(), subFile:<%s> deleted", childDN.c_str() );
			}
			// Delete main file
			result =  OMHandler.deleteObject(objectDn.c_str());
		}
		TRACE(fmsCpfFileTrace, "deleteFile(), DN=<%s> result:<%d>", objectDn.c_str(), result );
	}
	else
	{
		// Delete a subFile
		// Check for infinite subfile deletion
		if(isInfinite)
		{
			// Send to the CPF server the remove request
			ACS_APGCC_Command cmd;
			cmd.cmdCode = CPF_API_Protocol::REMOVE_INFINITESUBFILE;
			std::string file(filename_);
			ACS_APGCC::toUpper(filename_);

			cmd.data[0] = file;
			cmd.data[1] = m_pCPName;
			int result = DsdClient.execute(cmd);

			// check command execution
			if(result == 0)
			{
				// check result of remove
				if (cmd.result != FMS_CPF_Exception::OK)
				{
					std::string errorText(cmd.data[0]);
					std::string detailInfo(cmd.data[1]);
					TRACE(fmsCpfFileTrace,"%s, infinite subfile:<%s> delete failed!, Error text:<%s>, detailed info:<%s>",
											__func__,
											file.c_str(),
											errorText.c_str(),
											detailInfo.c_str() );

					std::string errMsg = errorText + detailInfo;
					throw FMS_CPF_Exception(FMS_CPF_Exception::errorType(cmd.result), errMsg);
				}

				TRACE(fmsCpfFileTrace,"%s, infinite subfile:<%s> deleted", __func__, file.c_str());
			}
			else
			{
				// Communication error with CPF server
				TRACE(fmsCpfFileTrace, "%s infinite subfile delete failed!, Connection broken", __func__);
				FMS_CPF_Exception exErr(FMS_CPF_Exception::SOCKETERROR);
				std::string errMsg(exErr.errorText());
				errMsg += "Connection broken";

				throw FMS_CPF_Exception(FMS_CPF_Exception::SOCKETERROR, errMsg);
			}
		}
		// Delete a subfile
		TRACE(fmsCpfFileTrace, "deleteFile(), subFile=%s deletion, <%i> subfiles found", subFileName.c_str(), numOfSubfiles);

		// Search Subfile DN
		for( element = listOfDN.begin(); element != listOfDN.end(); ++element)
		{
			// Split the field in RDN and Value
			size_t equalPos = (*element).find_first_of(Equal);
			size_t commaPos = (*element).find_first_of(Comma);

			// Check if some error happens
			if( (std::string::npos != equalPos) && (std::string::npos != commaPos) )
			{
				std::string childName = (*element).substr(equalPos + 1, (commaPos - equalPos - 1) );
				ACS_APGCC::toUpper(childName);
				// check the subfile name
				if(subFileName.compare(childName) == 0)
				{	//Begin of the fix HZ23201
					for(int i=0;i<3;i++)
                                        {
                                                TRACE(fmsCpfFileTrace,"Entering going to delete the file = %s",subFileName.c_str());
                                                result =  OMHandler.deleteObject((*element).c_str());
                                                std::string admFilename="ADMARKER";
                                                if(result && ((admFilename.compare(subFileName)==0)))
                                                {
                                                        TRACE(fmsCpfFileTrace,"Entering into retry mechanism for the ADMARKER file=%s",subFileName.c_str());
                                                        ACE_Time_Value selectTime;
                                                        selectTime.set(1,0);
                                                        ACE_INT32 result = 0;
                                                        while((result = ACE_OS::poll(0,0,selectTime)) == -1 && (errno == EINTR))
                                                        {
                                                                continue;
                                                        }
                                                        TRACE(fmsCpfFileTrace, "deleteFile(), Retry mechanism <%d>",i);
                                                        continue;
                                                }
                                                else
                                                {
                                                        break;

                                                }
                                        }//End of the fix HZ23201

					// subfile found, delete it
					TRACE(fmsCpfFileTrace, "deleteFile(), subFile DN:<%s> result:<%d>", (*element).c_str(), result);
					break;
				}
			}
		}
	}

	// Check delete result
	if(0 != result)
	{
		TRACE(fmsCpfFileTrace, "deleteFile(), exitCode:<%d>", result);
		FMS_CPF_Exception::errorType errCode;
		std::string errMsg;

		// IMM error
		if(result < 0)
		{
			// Map to CPF error
			errCode = FMS_CPF_Exception::INTERNALERROR;

			if(ELEMENT_NOT_EXIST == result)
			{
				errCode =  FMS_CPF_Exception::UNABLECONNECT;
			}
		}
		else
		{
			// CPF custom error code
			errCode = static_cast<FMS_CPF_Exception::errorType>(result);
			errMsg = OMHandler.getLastImmError();
		}

		TRACE(fmsCpfFileTrace, "deleteFile(), error Text:<%s>", errMsg.c_str());
		throw FMS_CPF_Exception(errCode, errMsg);
	}

	TRACE(fmsCpfFileTrace, "%s", "Leaving deleteFile()");
}

//------------------------------------------------------------------------------
//      Check if file instance is reserved
//------------------------------------------------------------------------------
bool FMS_CPF_File::isReserved() const
{
	return ( (reference_ != 0) ? true: false);
}

//------------------------------------------------------------------------------
//      Check if file exists
//------------------------------------------------------------------------------
bool FMS_CPF_File::exists() throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in exists()");
	std::string cpname(m_pCPName);

	if (m_bIsSysBC)
	{
		m_bCPExists = isCP();

		if (!m_bCPExists)
		{
			throw FMS_CPF_Exception(FMS_CPF_Exception::CPNOTEXISTS, cpname);
		}

		if (cpname.empty())
		{
			throw FMS_CPF_Exception(FMS_CPF_Exception::CPNAMENOPASSED, cpname);
		}
	}

	FMS_CPF_FileId fileid(filename_);

	if (!fileid.isValid ())
	{
		TRACE(fmsCpfFileTrace, "exists(), invalid filename:<%s>", filename_.c_str() );
		throw FMS_CPF_Exception(FMS_CPF_Exception::INVALIDFILE);
	}

	int exists = 0;

	ACS_APGCC_Command cmd;
	cmd.cmdCode = CPF_API_Protocol::EXISTS_;
	std::string file = filename_;
	ACS_APGCC::toUpper(file);
	cmd.data[0] = file;
	cmd.data[1] = cpname;

	int result = DsdClient.execute(cmd);

	if (result == 0)
	{
		if (cmd.result == FMS_CPF_Exception::OK)
		{
			exists = cmd.data[0];
			TRACE(fmsCpfFileTrace, "exists(), file=<%s> %s .", filename_.c_str(), (exists? "exist":"not exist"));
		}
		else
		{
			std::string errorText(cmd.data[0]);
			std::string detailInfo(cmd.data[1]);
			TRACE(fmsCpfFileTrace,"exists(), Exception!, Error text = %s, detailed info = %s",
									errorText.c_str(),
									detailInfo.c_str() );
			std::string errMsg = errorText + detailInfo;

			throw FMS_CPF_Exception(FMS_CPF_Exception::errorType(cmd.result), errMsg);
		}
	}
	else
	{
		TRACE(fmsCpfFileTrace, "%s", "exists(), Exception!, Connection broken");

		FMS_CPF_Exception exErr(FMS_CPF_Exception::SOCKETERROR);
		std::string errMsg(exErr.errorText());
		errMsg += "Connection broken";

		throw FMS_CPF_Exception(FMS_CPF_Exception::SOCKETERROR);
	}
	TRACE(fmsCpfFileTrace, "%s", "Leaving exists()");
	return (exists != 0) ? true : false;
}

//------------------------------------------------------------------------------
//      Create volume
//------------------------------------------------------------------------------
void FMS_CPF_File::createVolume(const char* volumename) throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in createVolume()");

	std::string classObj = "AxeCpFileSystemCpVolume";
	volume_ = std::string(volumename);

	// Convert to Upper Case because the volume name is not case sensitive
	ACS_APGCC::toUpper(volume_);

	std::string volumeId = CpVolumeIdTag + volume_;

	if(m_bIsSysBC)
	{
		TRACE(fmsCpfFileTrace, "createVolume(), MCP system Cp:<%s>", m_pCPName);
		volumeId += AtSign;
		volumeId += std::string(m_pCPName);
	}

	TRACE(fmsCpfFileTrace, "createVolume(), volume RDN:<%s>, parent DN:<%s>", volumeId.c_str(), cpFileSystemRoot.c_str());

	FMS_CPF_omCmdHandler OmHandler(1, cpFileSystemRoot.c_str(), classObj.c_str());

	char attributeRDN[] = "cpVolumeId";
    // volumeId Attribute
	OmHandler.addNewAttr(attributeRDN, ATTR_STRINGT, volumeId.c_str());

    int result = OmHandler.writeObj();

	if(FMS_CPF_Exception::OK != result)
	{
		TRACE(fmsCpfFileTrace, "createVolume(), get error:<%d>", result);
		// Check for IMM error
		if( result < 0)
		{
			FMS_CPF_Exception::errorType errCode;

			std::string detail;
			// Map IMM error to CPF custom error
			switch(result)
			{
				  case ELEMENT_NOT_EXIST:
					  errCode = FMS_CPF_Exception::UNABLECONNECT;
					  break;
				  case ELEMENT_ALREADY_EXIST:
					  errCode = FMS_CPF_Exception::VOLUMEEXISTS;
					  detail = volumename;
					  break;
				  default:
					  errCode = FMS_CPF_Exception::INTERNALERROR;
			 }

			FMS_CPF_Exception exErr(errCode);
			std::string errMsg(exErr.errorText());
			errMsg += detail;
			TRACE(fmsCpfFileTrace, "createVolume(), error: <%d>,<%s> ", errCode, errMsg.c_str());
			throw FMS_CPF_Exception(errCode, errMsg);
		}
		else
		{
			std::string errorPrt = OmHandler.getLastImmError();
			TRACE(fmsCpfFileTrace, "createVolume(), exitCode:<%d>, error msg:<%s>", result, errorPrt.c_str());
			// CPF custom error
			throw FMS_CPF_Exception(static_cast<FMS_CPF_Exception::errorType>(result), errorPrt);
		}
	 }

	TRACE(fmsCpfFileTrace, "%s", "Leaving createVolume()");
}

//------------------------------------------------------------------------------
//      For COMPOSITE SUB-FILE
//------------------------------------------------------------------------------
int FMS_CPF_File::writeCompositeSubFile(std::string dn, std::string subFile)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in writeCompositeSubFile()");

	std::string classObj = "AxeCpFileSystemCompositeSubFile";
	char attributeRDN[] = "compositeSubFileId";
	int numOfAttribute = 1;

	TRACE(fmsCpfFileTrace, "writeCompositeSubFile(), parentName DN:<%s>, subfile:<%s>", dn.c_str(), subFile.c_str());

	char compositeSubFileRDN[64] = {0};
	ACE_OS::snprintf(compositeSubFileRDN, 63, "%s=%s", attributeRDN, subFile.c_str());
	TRACE(fmsCpfFileTrace, "writeCompositeFile(), subfile RDN:<%s>", compositeSubFileRDN);

	FMS_CPF_omCmdHandler OmHandler(numOfAttribute, dn.c_str(), classObj.c_str());
	// Set CompositeSubFile Class attribute
	OmHandler.addNewAttr(attributeRDN, ATTR_STRINGT, compositeSubFileRDN);

	int exitCode = OmHandler.writeObj();

	TRACE(fmsCpfFileTrace, "writeCompositeSubFile(), IMM exitCode:<%d>", exitCode);

	// Check operation result
	if( FMS_CPF_Exception::OK != exitCode )
	{
		// error handling on subfile creation
		FMS_CPF_Exception::errorType errCode;
		std::string errMsg;

		// Check error type
		if( exitCode < 0)
		{
			// IMM error
			errCode = FMS_CPF_Exception::INTERNALERROR;

			if(ELEMENT_ALREADY_EXIST == exitCode)
			{
				errCode = FMS_CPF_Exception::FILEEXISTS;
				FMS_CPF_Exception exErr(errCode);
				errMsg = exErr.errorText();
				errMsg += filename_;
			}
		}
		else
		{
    		// CPF custom error code
    		errCode = static_cast<FMS_CPF_Exception::errorType>(exitCode);
    		errMsg = OmHandler.getLastImmError();
    	}

		TRACE(fmsCpfFileTrace, "writeCompositeSubFile(), error Text:<%s>", errMsg.c_str());
    	throw FMS_CPF_Exception(errCode, errMsg);
	}
	TRACE(fmsCpfFileTrace, "%s", "Leaving writeCompositeSubFile()");
	return exitCode;
}

//------------------------------------------------------------------------------
//    Create the InfiniteFile Object into IMM
//------------------------------------------------------------------------------
int FMS_CPF_File::writeInfiniteFile(FMS_CPF_FileId &fileid_, const FMS_CPF_Types::fileAttributes& fileattr)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in writeInfiniteFile()");

    std::string classObj = "AxeCpFileSystemInfiniteFile";
    char attributeRDN[] = "infiniteFileId";
    char attributeMaxSize[] = "maxSize";
    char attributeMaxTime[] = "maxTime";
    char attributeRecordLength[] = "recordLength";
    char attributeRelCond[] = "overrideSubfileClosure";
    char attributeFileTQ[] = "transferQueue";
    unsigned int numOfAttribute = 5;

    unsigned int ohiCode = AES_OHI_NOERRORCODE;

    int relCondition = (fileattr.infinite.release ? 1 : 0);
    unsigned int maxTime = fileattr.infinite.maxtime;
    unsigned int maxSize = fileattr.infinite.maxsize;
    unsigned int recordLength = fileattr.infinite.rlength;
    std::string tq_name(fileattr.infinite.transferQueue);

    if( !tq_name.empty() )
    {
    	//Verify if the TQ name is Valid
		if (!isTQNameValid(tq_name))
		{
			throw FMS_CPF_Exception(FMS_CPF_Exception::INVTQNAME);
		}

		//Query to GOH to take the TQ type info
		TqType transferType;
		std::string ohiErrorTxt;
		ohiCode = getTqType(tq_name, transferType, ohiErrorTxt);

		if(ohiCode != AES_OHI_NOERRORCODE)
		{
			FMS_CPF_Exception::errorType errorCode;
			std::string ohi_detail;
			switch(ohiCode)
			{
				case AES_OHI_FILENAMEINVALID:
					errorCode = FMS_CPF_Exception::INVTQNAME;
					break;

				case AES_OHI_TQNOTFOUND:
					errorCode = FMS_CPF_Exception::TQNOTFOUND;
					break;

				default:
					errorCode = FMS_CPF_Exception::GOHNOSERVERACCESS;
					ohi_detail = ohiErrorTxt;
			}

			FMS_CPF_Exception exErr(errorCode);
			std::string errMsg(exErr.errorText());
			errMsg += ohi_detail;
			throw FMS_CPF_Exception(errorCode, errMsg);
		}

		++numOfAttribute;
		TRACE(fmsCpfFileTrace, "writeInfiniteFile(), TQ name:<%s>, TQ type:<%d>", tq_name.c_str(), transferType);
    }

    std::string volumeDN = getVolumeDN(volume_);

    TRACE(fmsCpfFileTrace, "writeInfiniteFile(), parentName DN:<%s> under to create file:<%s>", volumeDN.c_str(), fileid_.data());

    char infiniteFileRDN[64] = {0};
    // Getting the time stamp value
    ACE_OS::snprintf(infiniteFileRDN, 63, "%s=%s", attributeRDN, fileid_.data());
    TRACE(fmsCpfFileTrace, "writeInfiniteFile(), infinite file RDN:<%s>", infiniteFileRDN);

    FMS_CPF_omCmdHandler omHandler(numOfAttribute, volumeDN.c_str(), classObj.c_str());
    // Set InfiniteFile Class attribute
    omHandler.addNewAttr(attributeRDN, ATTR_STRINGT, infiniteFileRDN); // infiniteFileId
    omHandler.addNewAttr(attributeMaxSize, ATTR_UINT32T, &maxSize); // maxsize
    omHandler.addNewAttr(attributeMaxTime, ATTR_UINT32T, &maxTime); // maxtime
    omHandler.addNewAttr(attributeRecordLength, ATTR_UINT32T, &recordLength); // recordLength
    omHandler.addNewAttr(attributeRelCond, ATTR_INT32T, &relCondition);  // releaseCondition

    if(!tq_name.empty())
    	omHandler.addNewAttr(attributeFileTQ, ATTR_STRINGT, tq_name.c_str());  // TQ name

    int exitCode = omHandler.writeObj();

    if (exitCode != FMS_CPF_Exception::OK)
    {
    	TRACE(fmsCpfFileTrace, "writeInfiniteFile(), exitCode:<%d>", exitCode);
		// IMM error
    	FMS_CPF_Exception::errorType errCode;
    	std::string errMsg;
    	if(exitCode < 0)
		{
    		errCode = FMS_CPF_Exception::INTERNALERROR;

			if(ELEMENT_NOT_EXIST == exitCode)
			{
				errCode = FMS_CPF_Exception::VOLUMENOTFOUND;
				FMS_CPF_Exception exErr(errCode);
				errMsg = exErr.errorText();
				errMsg +=  volume_;
			}
			else if(ELEMENT_ALREADY_EXIST == exitCode)
			{
				errCode = FMS_CPF_Exception::FILEEXISTS;
				FMS_CPF_Exception exErr(errCode);
				errMsg = exErr.errorText();
				errMsg += filename_;
			}
		}
    	else
    	{
    		// CPF custom error code
    		errCode = static_cast<FMS_CPF_Exception::errorType>(exitCode);
    		errMsg = omHandler.getLastImmError();
    	}
    	TRACE(fmsCpfFileTrace, "writeInfiniteFile(), error Text:<%s>", errMsg.c_str());
    	throw FMS_CPF_Exception(errCode, errMsg);
    }
    TRACE(fmsCpfFileTrace, "%s", "Leaving writeInfiniteFile()");
	return exitCode;
}

//------------------------------------------------------------------------------
//   Create the CompositeFile object into IMM
//------------------------------------------------------------------------------
int FMS_CPF_File::writeCompositeFile(FMS_CPF_FileId& fileid_, const FMS_CPF_Types::fileAttributes& fileattr)
{
    // IMM MOC name
    std::string immClassName = "AxeCpFileSystemCompositeFile";
    // IMM MOC attributes
    char attributeRDN[] = "compositeFileId";
    char attributeRecordLength[] = "recordLength";
    char attributedeleteFileTimer[] = "deleteFileTimer";
    int numOfAttribute = 3;
    int deleteFileTimer = fileattr.regular.deleteFileTimer;
    unsigned int recordLength = fileattr.regular.rlength;

    std::string volumeDN = getVolumeDN(volume_);

    char compositeFileRDN[64] = {0};
    ACE_OS::snprintf(compositeFileRDN, 63, "%s=%s", attributeRDN, fileid_.data());

    TRACE(fmsCpfFileTrace, "writeCompositeFile(), file:<%s>, RDN:<%s>, parent DN:<%s>", fileid_.data(), compositeFileRDN, volumeDN.c_str());
	TRACE(fmsCpfFileTrace, "mkfile testing delefiletiner:<%d>", deleteFileTimer);
	TRACE(fmsCpfFileTrace, "mkfile testing attribute.delefiletimer:<%ld>", fileattr.regular.deleteFileTimer);
    FMS_CPF_omCmdHandler omHandler(numOfAttribute, volumeDN.c_str(), immClassName.c_str());

    // Set CompositeFile Class attributes
    omHandler.addNewAttr(attributeRDN, ATTR_STRINGT, compositeFileRDN);
    omHandler.addNewAttr(attributeRecordLength, ATTR_UINT32T, &recordLength);
    omHandler.addNewAttr(attributedeleteFileTimer, ATTR_INT32T, &deleteFileTimer);

    int exitCode = omHandler.writeObj();

    if(FMS_CPF_Exception::OK != exitCode)
    {
    	TRACE(fmsCpfFileTrace, "writeCompositeFile(), exitCode:<%d>", exitCode);
		FMS_CPF_Exception::errorType errCode;
		std::string errMsg;

		// IMM error
		if(exitCode < 0)
		{
			// Map to CPF error
			errCode = FMS_CPF_Exception::INTERNALERROR;

			if(ELEMENT_NOT_EXIST == exitCode)
			{
				errCode = FMS_CPF_Exception::VOLUMENOTFOUND;
				FMS_CPF_Exception exErr(errCode);
				errMsg = exErr.errorText();
				errMsg += volume_;
			}
			else if(ELEMENT_ALREADY_EXIST == exitCode)
			{
				errCode = FMS_CPF_Exception::FILEEXISTS;
				FMS_CPF_Exception exErr(errCode);
				errMsg = exErr.errorText();
				errMsg += filename_;
			}
		}
		else
		{
			// CPF custom error code
			errCode = static_cast<FMS_CPF_Exception::errorType>(exitCode);
			errMsg = omHandler.getLastImmError();
		}

		TRACE(fmsCpfFileTrace, "writeCompositeFile(), error Text:<%s>", errMsg.c_str());
		throw FMS_CPF_Exception(errCode, errMsg);
    }

    TRACE(fmsCpfFileTrace, "%s", "Leaving writeCompositeFile()");
	return exitCode;
}

// Creates a SimpleFile object into IMM
int FMS_CPF_File::writeSimpleFile(FMS_CPF_FileId& fileid_, unsigned int rlength_)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in writeSimpleFile()");

	// IMM MOC name
	const char immClassName[]= "AxeCpFileSystemSimpleFile";
	// IMM MOC attributes
	char attributeRDN[]= "simpleFileId";
	char attributeRecordLength[]= "recordLength";
	int numOfAttribute = 2;

	std::string volumeDN = getVolumeDN(volume_);

	char simpleFileRDN[64] = {0};
	ACE_OS::snprintf(simpleFileRDN, 63, "%s=%s", attributeRDN, fileid_.data());

	TRACE(fmsCpfFileTrace, "writeSimpleFile(), file:<%s>, RDN:<%s>, parent DN:<%s>", fileid_.data(), simpleFileRDN, volumeDN.c_str());

	FMS_CPF_omCmdHandler omHandler(numOfAttribute, volumeDN.c_str(), immClassName);
	// Set SimpleFile Class attribute
	omHandler.addNewAttr(attributeRDN, ATTR_STRINGT, simpleFileRDN);
	omHandler.addNewAttr(attributeRecordLength, ATTR_UINT32T, &rlength_);

	int exitCode = omHandler.writeObj();

	if( FMS_CPF_Exception::OK != exitCode )
	{
		TRACE(fmsCpfFileTrace, "writeSimpleFile(), exitCode:<%d>", exitCode);
		FMS_CPF_Exception::errorType errCode;
		std::string errMsg;

		// IMM error
		if(exitCode < 0)
		{
			// Map to CPF error
			errCode = FMS_CPF_Exception::INTERNALERROR;
			if(ELEMENT_NOT_EXIST == exitCode)
			{
				errCode = FMS_CPF_Exception::VOLUMENOTFOUND;
				FMS_CPF_Exception exErr(errCode);
				errMsg = exErr.errorText();
				errMsg +=  volume_;
			}
			else if(ELEMENT_ALREADY_EXIST == exitCode)
			{
				errCode = FMS_CPF_Exception::FILEEXISTS;
				FMS_CPF_Exception exErr(errCode);
				errMsg = exErr.errorText();
				errMsg += filename_;
			}
		}
		else
		{
			// CPF custom error code
			errCode = static_cast<FMS_CPF_Exception::errorType>(exitCode);
			errMsg = omHandler.getLastImmError();
		}

		TRACE(fmsCpfFileTrace, "writeSimpleFile(), error Text:<%s>", errMsg.c_str());
		throw FMS_CPF_Exception(errCode, errMsg);
	}

	TRACE(fmsCpfFileTrace, "%s", "Leaving writeSimpleFile()");
	return exitCode;
}

int FMS_CPF_File::createInternalSubFiles(const std::string& compositeFileName, const std::list<std::string>& subFileNameList, const std::string& volume, const std::string& cpName)
{
	std::string parentDN;
	int retCode = -1;
	volume_ = volume;

	TRACE(fmsCpfFileTrace,"createInternalSubFile(), file=<%s>, volume=<%s>, cpName=<%s>", compositeFileName.c_str(), volume.c_str(), cpName.c_str());

	// Set the cpName value
	if(!cpName.empty() )
	{
		m_bIsSysBC = true;
		snprintf(m_pCPName, (sizeof(m_pCPName) / sizeof(char)), "%s", cpName.c_str());
	}

	try
	{
		// Get the DN of compositeFile
		getCompositeFileDN(volume_, compositeFileName, parentDN);

		if( !parentDN.empty() )
		{
			TRACE(fmsCpfFileTrace, "createInternalSubFile(), <%zd> objects to create", (int)subFileNameList.size());
			std::list<std::string>::const_iterator subFileName;
			retCode = 0;
			// Create all new subfiles
			for(subFileName = subFileNameList.begin(); (subFileName != subFileNameList.end()); ++subFileName)
			{
				retCode = writeCompositeSubFile(parentDN, (*subFileName));
			}
		}
	}
	catch(FMS_CPF_Exception& ex)
	{
		TRACE(fmsCpfFileTrace, "createInternalSubFile(), caught an exception:<%s>", ex.errorText());
		retCode = -1;
	}

	TRACE(fmsCpfFileTrace,"createInternalSubFile(), creation result:<%i>", retCode);

	return retCode;
}

int FMS_CPF_File::deleteInternalSubFiles(const std::string& compositeFileName, const std::string& volume, const std::string& cpName)
{
	std::string parentDN;
	int retCode = -1;
	volume_ = volume;

	TRACE(fmsCpfFileTrace, "deleteInternalSubFiles(), file=<%s>, volume=<%s>, cpName=<%s>", compositeFileName.c_str(), volume.c_str(), cpName.c_str());

	// Assemble the correct volume RDN
	if(!cpName.empty() )
	{
		m_bIsSysBC = true;
		snprintf(m_pCPName, (sizeof(m_pCPName) / sizeof(char)), "%s", cpName.c_str());
	}

	try
	{
		std::string mainFileName;
		std::string subFileName;
		FMS_CPF_FileId fileId(compositeFileName);

		// Check for subfile or file
		bool singleSubFile = (!fileId.subfileAndGeneration().empty());

		if(singleSubFile)
		{
			TRACE(fmsCpfFileTrace, "%s", "deleteInternalSubFiles(), delete a single subfile");
			mainFileName = fileId.file();
			subFileName = fileId.subfileAndGeneration();
		}
		else
		{
			mainFileName = compositeFileName;
		}

		// Get the DN of compositeFile
		getCompositeFileDN(volume_, mainFileName, parentDN);

		if( !parentDN.empty())
		{
			FMS_CPF_omCmdHandler OMHandler;
			std::vector<std::string> listOfDN;
			std::vector<std::string>::iterator element;

			// Load the list of subfiles (DNs list)
			OMHandler.loadChildInst(parentDN.c_str(), ACS_APGCC_SUBLEVEL, &listOfDN);

			TRACE(fmsCpfFileTrace, "deleteInternalSubFiles(), found <%zd> objects", listOfDN.size() );

			retCode = 0;
			// Delete all subfiles or single subfile
			for( element = listOfDN.begin(); element != listOfDN.end(); ++element)
			{
				// check deletion type
				if(singleSubFile)
				{
					// Get the file name from DN
					// Split the field in RDN and Value
					size_t equalPos = (*element).find_first_of(Equal);
					size_t commaPos = (*element).find_first_of(Comma);

					// Check if some error happens
					if( (std::string::npos != equalPos) && (std::string::npos != commaPos) )
					{
						std::string fileName = (*element).substr(equalPos + 1, (commaPos - equalPos - 1) );
						// make the name in upper case
						ACS_APGCC::toUpper(fileName);
						// Compare the file name
						if(subFileName.compare(fileName) == 0)
						{
							// subfile found
							// delete it and exit from the loop
							retCode = OMHandler.deleteObject((*element).c_str());
							TRACE(fmsCpfFileTrace, "deleteInternalSubFiles(), deleted subfile:<%s> of composite file:<%s>", subFileName.c_str(), mainFileName.c_str() );
							break;
						}
					}
				}
				else
					retCode = OMHandler.deleteObject((*element).c_str());
			}
		}
	}
	catch(FMS_CPF_Exception& ex)
	{
		TRACE(fmsCpfFileTrace, "deleteInternalSubFiles(), caught an exception:<%s>", ex.errorText());
		retCode = -1;
	}

	TRACE(fmsCpfFileTrace, "deleteInternalSubFiles(), deletion result:<%i>", retCode);

	return retCode;
}

//------------------------------------------------------------------------------
//      Get CP file name for the file
//------------------------------------------------------------------------------
const char* FMS_CPF_File::getCPFname() const
{
	TRACE(fmsCpfFileTrace, "getCPFname(), Filename = %s", filename_.c_str() );
	return filename_.c_str();
}

//------------------------------------------------------------------------------
//      Rename file
//------------------------------------------------------------------------------
void FMS_CPF_File::rename(const char* newName) throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s" ,"Entering in rename()");

	// Vector of action parameters
	std::vector<ACS_APGCC_AdminOperationParamType> parametersList;

	// create cpName parameter
	ACS_APGCC_AdminOperationParamType cpNameParameter;

	// Set the action Id for SCP
	ActionType actionID(Rename_CpFile);

	// Check environment type SCP/MCP
	if (m_bIsSysBC)
	{
		TRACE(fmsCpfFileTrace, "rename(), file of Cp:<%s>", m_pCPName);

		// Change the action Id for MCP
		actionID = Rename_CpClusterFile;

		std::string cpname(m_pCPName);

		// check the Cp Name
		if (cpname.empty())
		{
			throw FMS_CPF_Exception(FMS_CPF_Exception::CPNAMENOPASSED, cpname);
		}

		m_bCPExists = isCP();

		if (!m_bCPExists)
		{
			throw FMS_CPF_Exception(FMS_CPF_Exception::CPNOTEXISTS, cpname);
		}

		// Set values of action parameter cpName
		cpNameParameter.attrName = actionParameters::cpName;
		cpNameParameter.attrType = ATTR_STRINGT;
		cpNameParameter.attrValues = reinterpret_cast<void*>(m_pCPName);

		// add parameter to the list
		parametersList.push_back(cpNameParameter);
	}

	TRACE(fmsCpfFileTrace, "rename(), file:<%s> to <%s>", filename_.c_str(), newName );
	// Check the file name
	FMS_CPF_FileId currentFileId(filename_);
	FMS_CPF_FileId newFileId(newName);

	if(!currentFileId.isValid() || !newFileId.isValid())
	{
		TRACE(fmsCpfFileTrace, "%s", "rename(), invalid file name" );
		throw FMS_CPF_Exception(FMS_CPF_Exception::INVALIDFILE);
	}

	// Unreserve the file if it is been reserved in this same context
	try
	{
		unreserve();
	}
	catch(FMS_CPF_Exception& ex)
	{
		if(ex.errorCode() != FMS_CPF_Exception::INVALIDREF) throw;
	}

	// Prepare action parameters
	// create currentName parameter
	ACS_APGCC_AdminOperationParamType currentNameParameter;

	currentNameParameter.attrName = actionParameters::currentName;
	currentNameParameter.attrType = ATTR_STRINGT;

	char tmpCurrentName[filename_.length() + 1];
	ACE_OS::strcpy(tmpCurrentName, filename_.c_str());
	currentNameParameter.attrValues = reinterpret_cast<void*>(tmpCurrentName);

	parametersList.push_back(currentNameParameter);

	// create newName parameter
	ACS_APGCC_AdminOperationParamType newNameParameter;

	newNameParameter.attrName = actionParameters::newName;
	newNameParameter.attrType = ATTR_STRINGT;

	char tmpNewName[64] = {0};
	ACE_OS::strncpy(tmpNewName, newName, 63);
	newNameParameter.attrValues = reinterpret_cast<void*>(tmpNewName);

	parametersList.push_back(newNameParameter);

	FMS_CPF_AdminOperation asyncAction;

	//invoke admin operation
	int exitCode = asyncAction.sendAsyncActionToServer(cpFileSystemRoot, actionID, parametersList);

	// Check operation result
	if(FMS_CPF_Exception::OK != exitCode)
	{
		std::string errMsg;
		asyncAction.getErrorDetail(errMsg);
		FMS_CPF_Exception::errorType errorCode = static_cast<FMS_CPF_Exception::errorType>(exitCode);

		TRACE(fmsCpfFileTrace, "rename(), error:<%d, %s> on rename operation", exitCode, errMsg.c_str());

		throw FMS_CPF_Exception(errorCode, errMsg);
	}

	// set the new name
	filename_ = newName;

	TRACE(fmsCpfFileTrace, "%s", "Leaving rename()");
}

//------------------------------------------------------------------------------
//      Move file to another volume
//------------------------------------------------------------------------------
void FMS_CPF_File::move(const char* volume) throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in move()");

	// Vector of action parameters
	std::vector<ACS_APGCC_AdminOperationParamType> parametersList;

	// cpName parameter
	ACS_APGCC_AdminOperationParamType cpNameParameter;

	// Set the action Id for SCP
	ActionType actionID(Move_CpFile);

	if (m_bIsSysBC)
	{
		TRACE(fmsCpfFileTrace, "move(), CP name:<%s>", m_pCPName);

		// Change the action Id for MCP
		actionID = Move_CpClusterFile;

		std::string cpname(m_pCPName);

		if(cpname.empty())
		{
			throw FMS_CPF_Exception (FMS_CPF_Exception::CPNAMENOPASSED, "");
		}

		m_bCPExists = isCP();

		if (!m_bCPExists)
		{
			throw FMS_CPF_Exception (FMS_CPF_Exception::CPNOTEXISTS, "");
		}

		// Set values of action parameter cpName
		cpNameParameter.attrName = actionParameters::cpName;
		cpNameParameter.attrType = ATTR_STRINGT;
		cpNameParameter.attrValues = reinterpret_cast<void*>(m_pCPName);

		// Add parameter to the list
		parametersList.push_back(cpNameParameter);
	}

	TRACE(fmsCpfFileTrace, "move(), file:<%s> to volume:<%s>", filename_.c_str(), volume);

	char tmpSrcFileValue[filename_.length() + 1];
	ACE_OS::strcpy(tmpSrcFileValue, filename_.c_str());
	// create srcFile parameter for list
	ACS_APGCC_AdminOperationParamType srcFile;
	srcFile.attrName = actionParameters::srcCpFile;
	srcFile.attrType = ATTR_STRINGT;
	srcFile.attrValues = reinterpret_cast<void*>(tmpSrcFileValue);

	parametersList.push_back(srcFile);

	char tmpVolumeValue[32] = {0};
	ACE_OS::strncpy(tmpVolumeValue, volume, 31);

	//create destFile parameter for list
	ACS_APGCC_AdminOperationParamType newVolumeParam;
	newVolumeParam.attrName = actionParameters::volume;
	newVolumeParam.attrType = ATTR_STRINGT;
	newVolumeParam.attrValues = reinterpret_cast<void*>(tmpVolumeValue);

	parametersList.push_back(newVolumeParam);

	FMS_CPF_AdminOperation asyncAction;

	//invoke admin operation
	int exitCode = asyncAction.sendAsyncActionToServer(cpFileSystemRoot, actionID, parametersList);

	// Check operation result
	if(FMS_CPF_Exception::OK != exitCode)
	{
		std::string errMsg;
		asyncAction.getErrorDetail(errMsg);
		FMS_CPF_Exception::errorType errorCode = static_cast<FMS_CPF_Exception::errorType>(exitCode);
		TRACE(fmsCpfFileTrace, "move(), error:<%d, %s> on move operation", exitCode, errMsg.c_str());
		throw FMS_CPF_Exception(errorCode, errMsg);
	}

	TRACE(fmsCpfFileTrace, "%s", "Leaving move()");
}

//------------------------------------------------------------------------------
//      Get volume name for the file
//------------------------------------------------------------------------------
const char* FMS_CPF_File::getVolume() throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in getVolume()");

	volume_ = getVolumeInfo(filename_);
	TRACE(fmsCpfFileTrace, "%s", "Leaving getVolume()");
	return volume_.c_str();
}

//------------------------------------------------------------------------------
//      Get physical path
//------------------------------------------------------------------------------
std::string FMS_CPF_File::getPhysicalPath() throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in getPhysicalPath()");

	ACS_APGCC_Command cmd;
	cmd.cmdCode = CPF_API_Protocol::GET_PATH_;
	cmd.data[0] = reference_;

	int result = DsdClient.execute(cmd);

	if(result == 0)
	{
		if (cmd.result ==  FMS_CPF_Exception::OK)
		{
			TRACE(fmsCpfFileTrace, "Leaving getPhysicalPath(), file path<%s>", cmd.data[0].c_str());
			return std::string(cmd.data[0]);
		}
		else
		{
			std::string errorText(cmd.data[0]);
			std::string detailInfo( cmd.data[1]);
			TRACE(fmsCpfFileTrace, "getPhysicalPath(),Exeption!, Error text = %s, detailed info = %s",
									errorText.c_str(),
									detailInfo.c_str() );
			std::string errMsg = errorText + detailInfo;
			throw FMS_CPF_Exception(FMS_CPF_Exception::errorType(cmd.result), errMsg);
		}
	}
	else
	{
		TRACE(fmsCpfFileTrace, "%s", "getPhysicalPath(), Exeption!, Connection broken");

		FMS_CPF_Exception exErr(FMS_CPF_Exception::SOCKETERROR);
		std::string errMsg(exErr.errorText());
		errMsg += "Connection broken";
		throw FMS_CPF_Exception(FMS_CPF_Exception::SOCKETERROR, errMsg);
	}
}

//------------------------------------------------------------------------------
//      Get physical path
//------------------------------------------------------------------------------
int FMS_CPF_File::getPhysicalPath(char* pathFile, int& bufferLenth) throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in getPhysicalPath()");

	ACS_APGCC_Command cmd;
	cmd.cmdCode = CPF_API_Protocol::GET_PATH_;
	cmd.data[0] = reference_;

	int result = DsdClient.execute(cmd);

	if(result == 0)
	{
		if (cmd.result ==  FMS_CPF_Exception::OK)
		{
			std::string path(cmd.data[0]);
			int pathLength = path.length();

			if((pathLength + 1) > bufferLenth)
			{
				bufferLenth = pathLength + 1;
				TRACE(fmsCpfFileTrace, "Leaving getPhysicalPath(), buffer too small, path length:<%d>", bufferLenth);
				return -1;
			}
			else
			{
				strncpy(pathFile, path.c_str(), pathLength);
				pathFile[pathLength] = 0;
				TRACE(fmsCpfFileTrace, "Leaving getPhysicalPath(), file path<%s>", pathFile);
				return pathLength;
			}
		}
		else
		{
			std::string errorText(cmd.data[0]);
			std::string detailInfo( cmd.data[1]);
			TRACE(fmsCpfFileTrace, "getPhysicalPath(),Exeption!, Error text = %s, detailed info = %s",
									errorText.c_str(),
									detailInfo.c_str() );

			std::string errMsg = errorText + detailInfo;
			throw FMS_CPF_Exception(FMS_CPF_Exception::errorType(cmd.result), errMsg);
		}
	}
	else
	{
		TRACE(fmsCpfFileTrace, "%s", "getPhysicalPath(), Exeption!, Connection broken");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::SOCKETERROR);
		std::string errMsg(exErr.errorText());
		errMsg += "Connection broken";
		throw FMS_CPF_Exception(FMS_CPF_Exception::SOCKETERROR, errMsg);
	}
}

//------------------------------------------------------------------------------
//      Copy the file to another file
//------------------------------------------------------------------------------
void FMS_CPF_File::copy(const char* dstFileName, FMS_CPF_Types::copyMode mode)
throw (FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in copy()");

	if (m_bIsSysBC)
	{
		TRACE(fmsCpfFileTrace, "copy(), CP name:<%s>", m_pCPName);
		std::string cpname(m_pCPName);

		if(cpname.empty())
		{
			throw FMS_CPF_Exception (FMS_CPF_Exception::CPNAMENOPASSED, "");
		}

		m_bCPExists = isCP();

		if (!m_bCPExists)
		{
			throw FMS_CPF_Exception (FMS_CPF_Exception::CPNOTEXISTS, "");
		}
	}

	TRACE(fmsCpfFileTrace, "copy(), srcFile:<%s>, dstFile:<%s>, mode:<%d>", filename_.c_str(), dstFileName, mode);

	copyFileAction(dstFileName, mode);

	TRACE(fmsCpfFileTrace, "%s", "Leaving copy()");
}

//------------------------------------------------------------------------------
//      Copy the File to the destination file
//------------------------------------------------------------------------------
int FMS_CPF_File::copyFileAction(const std::string& dstFileName, FMS_CPF_Types::copyMode copyMode)
throw (FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in copyFile()");

	// Vector of action parameters
	std::vector<ACS_APGCC_AdminOperationParamType> parametersList;

	// create srcFile parameter for list
	ACS_APGCC_AdminOperationParamType srcFile;

	char tmpSrcFileValue[filename_.length() + 1];
	ACE_OS::strcpy(tmpSrcFileValue, filename_.c_str());

	srcFile.attrName = actionParameters::srcCpFile;
	srcFile.attrType = ATTR_STRINGT;
	srcFile.attrValues = reinterpret_cast<void*>(tmpSrcFileValue);

	parametersList.push_back(srcFile);

	//create destFile parameter for list
	ACS_APGCC_AdminOperationParamType dstFile;

	char tmpDstFileValue[dstFileName.length() + 1];
	ACE_OS::strcpy(tmpDstFileValue, dstFileName.c_str());

	dstFile.attrName = actionParameters::dstCpFile;
	dstFile.attrType = ATTR_STRINGT;
	dstFile.attrValues = reinterpret_cast<void*>(tmpDstFileValue);

	parametersList.push_back(dstFile);

	// create mode Element of parameter list
	ACS_APGCC_AdminOperationParamType mode;

	mode.attrName = actionParameters::mode;
	mode.attrType = ATTR_INT32T ;
	mode.attrValues = reinterpret_cast<void*>(&copyMode);

	parametersList.push_back(mode);

	// cpName parameter
	ACS_APGCC_AdminOperationParamType cpNameParameter;

	// Set the action Id for SCP
	ActionType actionID(Copy_CpFile);

	if (m_bIsSysBC)
	{
		// Change the action Id for MCP
		actionID = Copy_CpClusterFile;

		// Set values of action parameter cpName
		cpNameParameter.attrName = actionParameters::cpName;
		cpNameParameter.attrType = ATTR_STRINGT;
		cpNameParameter.attrValues = reinterpret_cast<void*>(m_pCPName);

		// Add parameter to the list
		parametersList.push_back(cpNameParameter);
	}

	FMS_CPF_AdminOperation asyncAction;

	//invoke admin operation
	int exitCode = asyncAction.sendAsyncActionToServer(cpFileSystemRoot, actionID, parametersList);

	// Check operation result
	if(FMS_CPF_Exception::OK != exitCode)
	{
		std::string errMsg;
		asyncAction.getErrorDetail(errMsg);
		FMS_CPF_Exception::errorType errorCode = static_cast<FMS_CPF_Exception::errorType>(exitCode);

		TRACE(fmsCpfFileTrace, "copyFile(), error:<%d, %s> on copy operation", exitCode, errMsg.c_str());
		throw FMS_CPF_Exception(errorCode, errMsg);
	}

	TRACE(fmsCpfFileTrace, "%s", "Leaving copyFile()");
	return exitCode;
}

//------------------------------------------------------------------------------
//      Export file
//------------------------------------------------------------------------------
void FMS_CPF_File::fileExport(std::string dstFileName, FMS_CPF_Types::copyMode mode, bool toZip)
throw (FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in fileExport()");

	if (m_bIsSysBC)
	{
		std::string cpname(m_pCPName);
		TRACE(fmsCpfFileTrace, "fileExport(), CP name:<%s>", m_pCPName);

		m_bCPExists = isCP();

		if(cpname.empty())
		{
			throw FMS_CPF_Exception (FMS_CPF_Exception::CPNAMENOPASSED, "");
		}

		if (!m_bCPExists)
		{
			throw FMS_CPF_Exception(FMS_CPF_Exception::CPNOTEXISTS, "");
		}
	}

	TRACE(fmsCpfFileTrace, "fileExport(), file:<%s>, dstFile:<%s>, mode:<%d>, toZip:<%d>", filename_.c_str(), dstFileName.c_str(), mode, toZip);

	fileExportAction(dstFileName, mode, toZip);

	TRACE(fmsCpfFileTrace, "%s", "Leaving fileExport()");
}


int FMS_CPF_File::fileExportAction(const std::string& dstFileName, FMS_CPF_Types::copyMode copyMode, bool toZip)
throw (FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "fileExportAction(), src fileName:<%s>, dst:<%s>", filename_.c_str(), dstFileName.c_str());

	std::vector<ACS_APGCC_AdminOperationParamType> parametersList;
	// create srcFile parameter for list
	ACS_APGCC_AdminOperationParamType srcFile;

	srcFile.attrName = actionParameters::srcCpFile;
	srcFile.attrType = ATTR_STRINGT;

	char tmpSrcFileValue[filename_.length() + 1];
	ACE_OS::strcpy(tmpSrcFileValue, filename_.c_str());
	srcFile.attrValues = reinterpret_cast<void*>(tmpSrcFileValue);

	parametersList.push_back(srcFile);

	// create destFile parameter for list
	ACS_APGCC_AdminOperationParamType dstFile;

	dstFile.attrName = actionParameters::destination;
	dstFile.attrType = ATTR_STRINGT;

	char tmpDstFileValue[dstFileName.length() + 1];
	strcpy(tmpDstFileValue, dstFileName.c_str());
	dstFile.attrValues = reinterpret_cast<void*>(tmpDstFileValue);

	parametersList.push_back(dstFile);

	// create mode Element of parameter list
	ACS_APGCC_AdminOperationParamType mode;

	mode.attrName = actionParameters::mode;
	mode.attrType = ATTR_INT32T ;
	mode.attrValues = reinterpret_cast<void*>(&copyMode);
	
	parametersList.push_back(mode);
	
	int zipping = (toZip ? 1 : 0);
	// create zip Element of parameter list
	ACS_APGCC_AdminOperationParamType zipFile;

	zipFile.attrName = actionParameters::zip;
	zipFile.attrType = ATTR_INT32T;
	zipFile.attrValues = reinterpret_cast<void*>(&zipping);

	parametersList.push_back(zipFile);

	// cpName parameter
	ACS_APGCC_AdminOperationParamType cpNameParameter;

	// Set the action Id for SCP
	ActionType actionID(Export_CpFile);


	if (m_bIsSysBC)
	{
		// Change the action Id for MCP
		actionID = Export_CpClusterFile;

		// Set values of action parameter cpName
		cpNameParameter.attrName = actionParameters::cpName;
		cpNameParameter.attrType = ATTR_STRINGT;
		cpNameParameter.attrValues = reinterpret_cast<void*>(m_pCPName);

		// Add parameter to the list
		parametersList.push_back(cpNameParameter);
	}

	FMS_CPF_AdminOperation asyncAction;

	// invoke admin operation
	int exitCode = asyncAction.sendAsyncActionToServer(cpFileSystemRoot, actionID, parametersList);

	// Check operation result
	if(FMS_CPF_Exception::OK != exitCode)
	{
		std::string errMsg;
		asyncAction.getErrorDetail(errMsg);
		FMS_CPF_Exception::errorType errorCode = static_cast<FMS_CPF_Exception::errorType>(exitCode);

		TRACE(fmsCpfFileTrace, "fileExportAction(), error:<%d, %s> on export operation", exitCode, errMsg.c_str());
		throw FMS_CPF_Exception(errorCode, errMsg);
	}

	TRACE(fmsCpfFileTrace, "%s", "Leaving fileExportAction()");
	return exitCode;
}

//------------------------------------------------------------------------------
//      Import file
//------------------------------------------------------------------------------
void FMS_CPF_File::fileImport(std::string path, FMS_CPF_Types::copyMode mode) throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in fileImport()");

	if (m_bIsSysBC)
	{
		TRACE(fmsCpfFileTrace, "fileImport(), CP name:<%s>", m_pCPName);
		std::string cpname(m_pCPName);

		if (cpname.empty())
		{
			throw FMS_CPF_Exception (FMS_CPF_Exception::CPNAMENOPASSED, "");
		}

		m_bCPExists = isCP();

		if (!m_bCPExists)
		{
			throw FMS_CPF_Exception (FMS_CPF_Exception::CPNOTEXISTS, "");
		}
	}

	TRACE(fmsCpfFileTrace, "fileImport(), srcPath:<%s>, dstFile:<%s>, mode:<%d>", path.c_str(), filename_.c_str(), mode);

	fileImportAction(path, mode);

	TRACE(fmsCpfFileTrace, "%s", "Leaving fileImport()");
}

//------------------------------------------------------------------------------
//      Import file
//------------------------------------------------------------------------------
int FMS_CPF_File::fileImportAction(const std::string& path, FMS_CPF_Types::copyMode copyMode)
throw (FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in fileImportAction()");

	std::vector<ACS_APGCC_AdminOperationParamType> parametersList;

	// create srcFile parameter for list
	ACS_APGCC_AdminOperationParamType srcFile;

	srcFile.attrName = actionParameters::source;
	srcFile.attrType = ATTR_STRINGT;

	char tmpPathValue[path.length() + 1];
	ACE_OS::strcpy(tmpPathValue, path.c_str());
	srcFile.attrValues = reinterpret_cast<void*>(tmpPathValue);

	parametersList.push_back(srcFile);

	// create destFile parameter for list
	ACS_APGCC_AdminOperationParamType dstFile;

	dstFile.attrName = actionParameters::dstCpFile;
	dstFile.attrType = ATTR_STRINGT;

	char tmpFileValue[filename_.length() + 1];
	ACE_OS::strcpy(tmpFileValue, filename_.c_str());
	dstFile.attrValues = reinterpret_cast<void*>(tmpFileValue);

	parametersList.push_back(dstFile);

	//create mode Element of parameter list
	ACS_APGCC_AdminOperationParamType mode;

	mode.attrName = actionParameters::mode;
	mode.attrType = ATTR_INT32T ;
	mode.attrValues = reinterpret_cast<void*>(&copyMode);

	parametersList.push_back(mode);

	// cpName parameter
	ACS_APGCC_AdminOperationParamType cpNameParameter;

	// Set the action Id for SCP
	ActionType actionID(Import_CpFile);

	if (m_bIsSysBC)
	{
		// Change the action Id for MCP
		actionID = Import_CpClusterFile;

		// Set values of action parameter cpName
		cpNameParameter.attrName = actionParameters::cpName;
		cpNameParameter.attrType = ATTR_STRINGT;
		cpNameParameter.attrValues = reinterpret_cast<void*>(m_pCPName);

		// Add parameter to the list
		parametersList.push_back(cpNameParameter);
	}

	FMS_CPF_AdminOperation asyncAction;

	// invoke admin operation
	int exitCode = asyncAction.sendAsyncActionToServer(cpFileSystemRoot, actionID, parametersList);

	// Check operation result
	if(FMS_CPF_Exception::OK != exitCode)
	{
		std::string errMsg;
		asyncAction.getErrorDetail(errMsg);
		FMS_CPF_Exception::errorType errorCode = static_cast<FMS_CPF_Exception::errorType>(exitCode);

		TRACE(fmsCpfFileTrace, "fileImportAction(), error:<%d, %s> on import operation", exitCode, errMsg.c_str());
		throw FMS_CPF_Exception(errorCode, errMsg);
	}

	TRACE(fmsCpfFileTrace, "%s", "Leaving fileImportAction()");

	return exitCode;
}



//this function return true if the CP in m_pCPName exists otherwise return false
//to be defined

bool FMS_CPF_File::isCP()
{
	bool bRet = false;

	std::list<string>::iterator it = m_strListCP.begin();

	while (it != m_strListCP.end())
	{
		if(((*it).compare(m_pCPName)) == 0)
		{
			bRet = true;
			break;
		}
		++it;
	}

	return bRet;
}


void FMS_CPF_File::readConfiguration(const char* pCPName)
	throw (FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace,"%s","entering in readConfiguration()");
	FMS_CPF_ConfigReader myReader;
	myReader.init();
	m_bIsSysBC = myReader.IsBladeCluster();

	if(m_bIsSysBC)
	{
		m_strListCP = myReader.getCP_List();
		m_nNumCP = myReader.GetNumCP();

		ACE_OS::memset(m_pCPName, 0, 20);

		if ((pCPName != NULL) && (ACE_OS::strcmp(pCPName, "") != 0))
		{
			m_nCP_ID = myReader.cs_getCPID((char *)pCPName);

			if (m_nCP_ID >= 0)
				ACE_OS::strcpy(m_pCPName, (myReader.cs_getDefaultCPName(m_nCP_ID)).c_str());
			else
			{
				ACE_OS::strcpy(m_pCPName, "");

				TRACE(fmsCpfFileTrace,"%s", "readConfiguration(), CP_ID value is not correct" );
				throw FMS_CPF_Exception (FMS_CPF_Exception::ILLOPTION);
			}
		}
		else
		{
			ACE_OS::strcpy(m_pCPName, "");
			TRACE(fmsCpfFileTrace,"%s", "readConfiguration(), System is BC but CP Name is NULL");
			throw FMS_CPF_Exception (FMS_CPF_Exception::CPNAMENOPASSED);
		}
	}
	else
	{
		if ((pCPName != NULL) && (ACE_OS::strcmp(pCPName, "") != 0))
		{
			TRACE(fmsCpfFileTrace,"%s", "readConfiguration(), System is not BC but CP Name is not NULL");
			throw FMS_CPF_Exception (FMS_CPF_Exception::ILLVALUE);
		}

		ACE_OS::strcpy(m_pCPName, "");
	}

	m_bIsConfigRead = true;
	TRACE(fmsCpfFileTrace,"%s", "Leaving readConfiguration()");
}

//------------------------------------------------------------------------------
//      Chech volume name
//------------------------------------------------------------------------------
bool FMS_CPF_File::checkVolName(const std::string& vName)
{
	TRACE(fmsCpfFileTrace, "%s" , "Entering in checkVolName()");
	bool result = false;

	// first char in filename must be an alphabetic character
	if ((vName[0] >= 'A') && (vName[0] <= 'Z')
		&& (vName.length() <= (unsigned)VOL_LENGTH))
	{
		result = (vName.find_first_not_of(IDENTIFIER) == std::string::npos);
	}

	TRACE(fmsCpfFileTrace, "checkVolName(), volume name:<%s> is <%s>", vName.c_str(),
																	   result ? "OK" : "NOT OK");

	return result;
}


//------------------------------------------------------------------------------
//      Get the Volume Name related to a file
//------------------------------------------------------------------------------
std::string FMS_CPF_File::getVolumeInfo(std::string filename)
throw (FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in getVolumeInfo()");
	std::string volumeName;
	std::string cpname(m_pCPName);
	TRACE(fmsCpfFileTrace,"getVolumeInfo() filename= %s", filename.c_str() );

	if (m_bIsSysBC)
	{
		m_bCPExists = isCP();

		if (!m_bCPExists)
		{
			throw FMS_CPF_Exception (FMS_CPF_Exception::CPNOTEXISTS, "");
		}

		if (cpname.empty())
		{
			throw FMS_CPF_Exception (FMS_CPF_Exception::CPNAMENOPASSED, "");
		}
	}

	FMS_CPF_FileId fileid(filename);

	if (!fileid.isValid ())
	{
		TRACE(fmsCpfFileTrace, "getVolumeInfo(), Exception!, invalid filename:<%s>", filename.c_str() );

		throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
	}

	ACS_APGCC_Command cmd;
	cmd.cmdCode = CPF_API_Protocol::GET_FILEINFO_;
	std::string file(filename);

	ACS_APGCC::toUpper(file);
	cmd.data[0] = file;
	cmd.data[1] = cpname;
	int result = DsdClient.execute(cmd);

	if(result == 0)
	{
		if (cmd.result == FMS_CPF_Exception::OK)
		{
			std::string tmp (cmd.data[0]);
			volumeName = tmp;
			TRACE(fmsCpfFileTrace,"getVolumeInfo(), file= %s; Volume = %s", filename.c_str(), volumeName.c_str());
		}
		else
		{
			std::string errorText(cmd.data[0]);
			std::string detailInfo(cmd.data[1]);
			TRACE(fmsCpfFileTrace,"getVolumeInfo(), Exception!, Error text = %s, detailed info = %s",
									errorText.c_str(),
									detailInfo.c_str() );

			std::string errMsg = errorText + detailInfo;
			throw FMS_CPF_Exception(FMS_CPF_Exception::errorType(cmd.result), errMsg);
		}
	}
	else
	{
		TRACE(fmsCpfFileTrace, "%s", "getVolumeInfo(), Exception!, Connection broken");
		FMS_CPF_Exception exErr(FMS_CPF_Exception::SOCKETERROR);
		std::string errMsg(exErr.errorText());
		errMsg += "Connection broken";

		throw FMS_CPF_Exception(FMS_CPF_Exception::SOCKETERROR, errMsg);
	}
	TRACE(fmsCpfFileTrace,"%s","Leaving getVolumeInfo()");
	return volumeName;
}

//------------------------------------------------------------------------------
//      Get Composite File Id by the Volume Tag Id
//------------------------------------------------------------------------------
int FMS_CPF_File::getCompositeFileDN(std::string& volumeName, const std::string& compFileName, std::string& comFileDn)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in getCompositeFileId()");
	int result = FMS_CPF_Exception::FILENOTFOUND;

	// Assemble the volume DN
	std::string volumeDn = getVolumeDN(volumeName);

	TRACE(fmsCpfFileTrace, "getCompositeFileId(), volume DN:<%s>", volumeDn.c_str());

	// Gets the DN list of all files under the volume
	std::vector<std::string> fileList;
	FMS_CPF_omCmdHandler omHandler;
	omHandler.loadChildInst(volumeDn.c_str(), ACS_APGCC_SUBLEVEL, &fileList);

	std::vector<std::string>::iterator itFileList;
	size_t equalPos;
	size_t commaPos;

	// Search the specific file into file list
	for(itFileList = fileList.begin(); itFileList != fileList.end(); ++itFileList)
	{
		// Get the file name from DN
		// Split the field in RDN and Value
		equalPos = (*itFileList).find_first_of(Equal);
		commaPos = (*itFileList).find_first_of(Comma);

		// Check if some error happens
	    if( (std::string::npos != equalPos) && (std::string::npos != commaPos) )
	    {
	    	std::string fileName = (*itFileList).substr(equalPos + 1, (commaPos - equalPos - 1) );
	    	// make the name in upper case
			ACS_APGCC::toUpper(fileName);
			// Compare the file name
			if(compFileName.compare(fileName) == 0)
			{
				// File Found
				comFileDn = *itFileList;
				TRACE(fmsCpfFileTrace, "getCompositeFileId(), found file DN:<%s>", comFileDn.c_str());
				result = FMS_CPF_Exception::OK;
				break;
			}
	    }
	    else
	    {
	    	// error on parse DN
	    	TRACE(fmsCpfFileTrace, "getCompositeFileId(), error on parse:<%s>", (*itFileList).c_str());
	    }
	}

	fileList.clear();

	if( FMS_CPF_Exception::OK != result )
	{
		// file not found
		FMS_CPF_Exception exErr(FMS_CPF_Exception::FILENOTFOUND);
		std::string errMsg(exErr.errorText());
		errMsg += compFileName;

		throw FMS_CPF_Exception(FMS_CPF_Exception::FILENOTFOUND, errMsg);
	}

	TRACE(fmsCpfFileTrace, "%s", "Leaving getCompositeFileId()");
	return result;
}

//------------------------------------------------------------------------------
//      Get Sub Composite File DN by the Composite File DN and the subfile name
//------------------------------------------------------------------------------
int FMS_CPF_File::getSubCompositeFileDN(const std::string& comFileDN, const std::string& subFileName, std::string &subComFileDN)
{
	TRACE(fmsCpfFileTrace, "getSubCompositeFileDN(), compositeFileDN:<%s>, subFileName:<%s>", comFileDN.c_str(), subFileName.c_str());

	int result = FMS_CPF_Exception::FILENOTFOUND;

	std::vector<std::string> subFileList;
	std::vector<std::string>::iterator itFileList;

	FMS_CPF_omCmdHandler omHandler;
	omHandler.loadChildInst(comFileDN.c_str(), ACS_APGCC_SUBLEVEL, &subFileList);

    // Search the specific subfile into subfile list
    for(itFileList = subFileList.begin() ; itFileList != subFileList.end() ; ++itFileList)
    {
    	// Split the field in RDN and Value
    	size_t equalPos = (*itFileList).find_first_of(Equal);
		size_t commaPos = (*itFileList).find_first_of(Comma);

		// Check if some error happens
		if( (std::string::npos != equalPos) && (std::string::npos != commaPos) )
		{
			std::string childName = (*itFileList).substr(equalPos + 1, (commaPos - equalPos - 1) );
			ACS_APGCC::toUpper(childName);

			if (subFileName.compare(childName) == 0)
		    {
		    	subComFileDN = (*itFileList);
			    TRACE(fmsCpfFileTrace, "getSubCompositeFileId(), found subfile DN:<%s>", subComFileDN.c_str());
			    result = FMS_CPF_Exception::OK;
			    break;
		    }
	   }
	}
    subFileList.clear();

	return result;
}

unsigned int FMS_CPF_File::getTqType(std::string tq_name, TqType& tq_type, std::string& errorTxt)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in getTqType()");

	unsigned result = AES_OHI_NOERRORCODE;
	unsigned int reply;
	tq_type = TQ_UNDEFINED;
	errorTxt = "";
	//define
	AES_OHI_ExtFileHandler2* extFileObj =  NULL;
	extFileObj = new AES_OHI_ExtFileHandler2(OHI_USERSPACE::SUBSYS.c_str(), OHI_USERSPACE::APPNAME.c_str());

	//attach
	reply = extFileObj->attach();
	if ( reply != AES_OHI_NOERRORCODE )
	{
		result = reply;
		TRACE(fmsCpfFileTrace,"getTqType attach failed, error = %s", extFileObj->getErrCodeText(reply));
	}
	else
	{
		//verify if the transfer queue is defined
		reply = extFileObj->fileTransferQueueDefined(tq_name.c_str());
		if ( reply != AES_OHI_NOERRORCODE )
		{
			result = reply;
			//TRACE(fmsCpfFileTrace,"getTqType: TQ:<%s> not defined error:<%s>, errorCode:<%d>", tq_name.c_str(), extFileObj->getErrCodeText(reply), reply);
			TRACE(fmsCpfFileTrace,"getTqType: TQ:<%s> not defined as FILE TQ: error:<%s>, errorCode:<%d>", tq_name.c_str(), extFileObj->getErrCodeText(reply), reply);
		}
		else
		{
			tq_type = TQ_FILE;
			TRACE(fmsCpfFileTrace,"getTqType: FILE TQ:<%s> is defined", tq_name.c_str());
		}
		TRACE(fmsCpfFileTrace,"%s", "getTqType detach");
		//detach
		reply = extFileObj->detach();
		if ( reply != AES_OHI_NOERRORCODE )
		{
			TRACE(fmsCpfFileTrace,"getTqType(), detach failed error:<%s>", extFileObj->getErrCodeText(reply));
		}
	}
	//delete object
	if(extFileObj != NULL)
	{
		const char* error = extFileObj->getErrCodeText(result);
		if (error)
			errorTxt = error;
		delete extFileObj;
		extFileObj = NULL;
	}

	// If the given TQ is not defined as file TQ, try if it's defined as block TQ
	if ( tq_type == TQ_UNDEFINED )
	{
		result = AES_OHI_BlockHandler2::blockTransferQueueDefined(tq_name.c_str());
		if ( result == AES_OHI_NOERRORCODE )
		{
			tq_type = TQ_BLOCK;
			TRACE(fmsCpfFileTrace,"getTqType: BLOCK TQ:<%s> is defined", tq_name.c_str());
		}
		else
			TRACE(fmsCpfFileTrace,"getTqType: TQ:<%s> not defined as BLOCK TQ: errorCode:<%d>", tq_name.c_str(), reply);
	}

	TRACE(fmsCpfFileTrace, "Leaving getTqType(), exitCode:<%d>", result);
	return result;
}


bool FMS_CPF_File::isTQNameValid(const std::string& tqName)
{
	TRACE(fmsCpfFileTrace, "%s", "Entering in checkTQName()");
	bool validName = false;
	std::string tmpFileTQ(tqName);

	ACS_APGCC::toUpper(tmpFileTQ);

	if( !tmpFileTQ.empty() && ( (tmpFileTQ[0] >= 'A') && (tmpFileTQ[0] <= 'Z') ) )
	{
		size_t notValidPos = tmpFileTQ.find_first_not_of(OHI_USERSPACE::TQCHARS);
		validName = (std::string::npos == notValidPos);
	}

	TRACE(fmsCpfFileTrace, "Leaving checkTQName(), TQ name:<%s> is <%s>", tqName.c_str(), (validName ? "OK" : "NOT OK") );
	return validName;
}

/**********************************************************************/

//------------------------------------------------------------------------------
//      Set attributes for the file
//------------------------------------------------------------------------------
void FMS_CPF_File::setAttributes(const FMS_CPF_Types::fileAttributes& fileattr)
throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "setAttributes(), NYI");
	UNUSED(fileattr);
}

//------------------------------------------------------------------------------
//      Get attributes for the file
//------------------------------------------------------------------------------
FMS_CPF_Types::fileAttributes FMS_CPF_File::getAttributes() throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "getAttributes(), NYI");

	FMS_CPF_Attribute attribute;
	return attribute;
}

//------------------------------------------------------------------------------
//      Infinite file end
//------------------------------------------------------------------------------
void FMS_CPF_File::infiniteEnd() throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "infiniteEnd(), NYI");
}

//------------------------------------------------------------------------------
//      Get number of users of the file
//------------------------------------------------------------------------------
FMS_CPF_Types::userType FMS_CPF_File::getUsers() throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "getUsers(), NYI");

	FMS_CPF_Types::userType users;
	users.rcount = 0;
	users.wcount = 0;
	users.ucount = 0;

	return users;
}

//------------------------------------------------------------------------------
//      Get status for the file
//------------------------------------------------------------------------------
void FMS_CPF_File::getStat(FMS_CPF_Types::fileStat& stat) throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "getStat(), NYI");
	stat.size = 0;
}

//------------------------------------------------------------------------------
//      Manually report a file to GOH
//------------------------------------------------------------------------------
void FMS_CPF_File::manuallyReportFile (const char* destination, int retries, int timeinterval)
throw (FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace,"%s", "Entering in manuallyReportFile(), NYI");
	TRACE(fmsCpfFileTrace,"%s","manuallyReportFile(), dst:<%s>, retries:<%d>, time:<%d>", destination, retries, timeinterval);
	TRACE(fmsCpfFileTrace,"%s","Leaving manuallyReportFile()");
}

//------------------------------------------------------------------------------
//      setCompression()
//------------------------------------------------------------------------------
void FMS_CPF_File::setCompression(bool compressSub)
throw (FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "setCompression(compressSub:<%d>), NYI", compressSub);
}

//------------------------------------------------------------------------------
//      uncompressItem()
//------------------------------------------------------------------------------
bool FMS_CPF_File::uncompressItem(const char* path)
{
	TRACE(fmsCpfFileTrace, "uncompressItem(path:<%s>), NYI", path);
	return false;
}

//------------------------------------------------------------------------------
//      Resolve path name
//------------------------------------------------------------------------------
std::string FMS_CPF_File::getVolumeDN(std::string& volumeName)
{
	TRACE(fmsCpfFileTrace, "%s", " Entering in getVolumeDN()" );

	// assemble the volume RDN to search
	// volumeRDN = <volumeName>:<cpName>
	std::string volumeRDNtoFind(volumeName);

	// Check system type
	if(m_bIsSysBC)
	{
		// MCP system
		volumeRDNtoFind += AtSign;
		volumeRDNtoFind += std::string(m_pCPName);
	}

	// Make all up for compare
	ACS_APGCC::toUpper(volumeRDNtoFind);

	TRACE(fmsCpfFileTrace, "getVolumeDN(), search volumeRDN:<%s>", volumeRDNtoFind.c_str());

	std::vector<std::string> volumeDNList;
	std::vector<std::string>::const_iterator volumeDNIdx;
	std::string volumeDN;

	// Get the DN list of all defined volumes
	FMS_CPF_omCmdHandler omHandler;
	omHandler.loadChildInst(cpFileSystemRoot.c_str(), ACS_APGCC_SUBLEVEL, &volumeDNList);

	// compare each volume RDN with the volume RDN to find
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

			// Make all up for compare
			ACS_APGCC::toUpper(volumeRDN);

			if(volumeRDNtoFind.compare(volumeRDN) == 0)
			{
				// volume DN found
				volumeDN = (*volumeDNIdx);
				TRACE(fmsCpfFileTrace, "getVolumeDN(), found volume DN:<%s>", volumeDN.c_str());
				break;
			}
		}
		else
		{
			// error on parse DN
			TRACE(fmsCpfFileTrace, "getVolumeDN(), error on parse:<%s>", (*volumeDNIdx).c_str());
		}
	}
	TRACE(fmsCpfFileTrace, "%s", "Leaving getVolumeDN()");
	return volumeDN;
}

//------------------------------------------------------------------------------
//     End of file
//------------------------------------------------------------------------------
bool FMS_CPF_File::endOfFile() throw(FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "%s", "endOfFile, NYI");
	// not implemented
	return false;
}

//------------------------------------------------------------------------------
//      compressItem()
//------------------------------------------------------------------------------
bool FMS_CPF_File::compressItem(const char* path)
{
	TRACE(fmsCpfFileTrace, "compressItem(path:<%s>), NYI", path);
	return true;
}

//------------------------------------------------------------------------------
//      unsetCompression()
//------------------------------------------------------------------------------
void FMS_CPF_File::unsetCompression(bool uncompressSub)
throw (FMS_CPF_Exception)
{
	TRACE(fmsCpfFileTrace, "unsetCompression(uncompressSub:<%d>), NYI", uncompressSub);
}

//------------------------------------------------------------------------------
//      findFirstSubfile()
//------------------------------------------------------------------------------
const char* FMS_CPF_File::findFirstSubfile()
{
	TRACE(fmsCpfFileTrace, "%s", "findFirstSubfile(), NYI");
	return "";
}

const std::list<string> FMS_CPF_File::findFileList(const std::string& path)
{
	TRACE(fmsCpfFileTrace, "findFileList(path.<%s>), NYI", path.c_str());
	std::list<string> fileList;
	return fileList;
}

int FMS_CPF_File::compressFile(FILE *source, FILE *dest, int level)
{
	TRACE(fmsCpfFileTrace, "compressFile(%d), NYI", level);
	UNUSED(source);
	UNUSED(dest);
    return -1;
}

int FMS_CPF_File::decompressFile(FILE *source, FILE *dest)
{
	TRACE(fmsCpfFileTrace, "%s", "decompressFile(), NYI");
	UNUSED(source);
	UNUSED(dest);
	return -1;
}

