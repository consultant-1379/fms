/*
 * fms_cpf_exception.c
 *
 *  Created on: Dec 3, 2010
 *      Author: esalves
 */

#include "fms_cpf_exception.h"
#include "fms_cpf_api_trace.h"
#include "ACS_TRA_trace.h"

#include <iostream>
#include <sstream>
#include <string>


const std::string fms_cpf_exception_name = "FMS_CPF_Exception";

// Fault messages
namespace ExcepetionMessages{

		const char S_OK [] =				"";
		const char S_GENERAL_FAULT [] =		"General fault ";
		const char S_INCORRECT_USAGE [] =	"Incorrect usage ";
		const char S_UNREASONABLE [] =		"Unreasonable value: ";
		const char S_EXCEEDED [] =			"Value exceeded: ";
		const char S_INVALIDFILE [] =		"Invalid file name ";
		const char S_NOTBASEFILE [] =		"Not a simple file or a composite file: ";
		const char S_NOTSUBFILE [] =		"Not a subfile: ";
		const char S_FILEEXISTS [] =		"File exists: ";
		const char S_FILEISOPEN [] =		"File is already open: ";
		const char S_FILENOTFOUND [] =		"File was not found: ";
		const char S_VOLUMENOTFOUND [] =	"Volume was not found: ";
		const char S_ACCESSERROR [] =		"Access error ";
		const char S_NOTSAMEFILE [] =		"Subfile must belong to the same composite file: ";
		const char S_SAMEVOLUME [] =		"Cannot move file to same volume: ";
		const char S_PARAMERROR [] =		"Parameter error: ";
		const char S_TYPEERROR [] =			"Operation not permitted for this file type ";
		const char S_INVALIDREF [] =		"Invalid file reference ";
		const char S_NOTSIMPLE [] =			"Not a simple file or subfile: ";
		const char S_NOTCOMPOSITE [] =		"Not a composite file: ";
		const char S_COMPNOTEMPTY [] =		"Composite file contains subfiles: ";
		const char S_ILLVALUE [] =			"Illegal value: ";
		const char S_WRONGRECNUM [] =		"Wrong record number: ";
		const char S_TIMEOUT [] =			"Time out on connection ";
		const char S_NOMEMORY [] =			"Out of memory ";
		const char S_SOCKETERROR [] =		"Socket error: ";
		const char S_USERFAULT [] =			"API user fault ";
		const char S_PROTOCOLERROR [] =		"Protocol error ";
		const char S_PHYSICALERROR [] =		"Physical file error: ";
		const char S_PHYSICALNOTFOUND [] =	"Physical file not found: ";
		const char S_INTERNALERROR [] =		"Internal program fault: ";
		const char S_NOTIMPL [] =			"Not (yet) implemented: ";
		const char S_FILEISPROT [] =		"File is protected. ";
		const char S_TQNOTFOUND [] =		"Transfer queue not found ";
		const char S_INVTQNAME [] =			"Invalid transfer queue name ";
		const char S_INVTRANSFERMODE [] =	"Invalid transfer mode";
		const char S_GOHNOSERVERACCESS [] = "Cannot connect to GOH server: ";
		const char S_ALARM [] =				"Alarm ";
		const char S_INVALIDPATH [] =       "Invalid path ";
		const char S_VOLUMEEXISTS [] =		"Volume exists: ";
		const char S_CPNAMENOPASSED	[] =	"CP Name not passed";
		const char S_TABLEINSERTFAULT []=	"Insert Table in TableMap failed";

		const char S_CSNOACCESS []	=		"Unable to access to Configuration Service";
		const char S_CSOTHERFAILURE []	=	"Configuration Service cannot fulfill the request because of an error";

		const char S_CPNOTEXISTS []	=		"CP is not defined";
		const char S_NODEISPASSIVE [] =     "Unable to connect to FMS_CPF_server : "; //HF54685
		const char S_UNABLECONNECT [] =		"Unable to connect to server";
		const char S_ILLOPTION [] =			"Illegal option in this system configuration";
		const char S_ABORT [] =     "Aborted by operator "; //HK89831 && HK89834: CPFPORT:Aborted by operator
		const char S_IMM_NOVOLUME_STRUCT [] = "Nothing volume object present";
		const char S_IMM_NOFILE_FORVOL [] = "Nothing file object present";
		const char S_IMM_NOVOLUME_FORCP [] = "No volume present for this cp";
		const char S_IMM_ERR_GET_ATTR [] = "Error while retrieving an attribute value";
		const char S_BACKUPONGOING[] = "Action not executed, AP backup in progress";
		const char S_ATTACHFAILED[] = "Unable to connect Block Transfer Queue";//HV61830
}

FMS_CPF_Exception::FMS_CPF_Exception() :
error_(FMS_CPF_Exception::OK),
detail_("")
{
	fms_cpf_exception_trace = new (std::nothrow) ACS_TRA_trace(fms_cpf_exception_name);
}
//------------------------------------------------------------------------------
//	Constructor
//------------------------------------------------------------------------------

FMS_CPF_Exception::FMS_CPF_Exception(errorType error, std::string detail ) :
error_ (error),
detail_(detail)
{
	fms_cpf_exception_trace = new (std::nothrow) ACS_TRA_trace(fms_cpf_exception_name);
}


//------------------------------------------------------------------------------
//	Constructor
//------------------------------------------------------------------------------

FMS_CPF_Exception::FMS_CPF_Exception(errorType error, const char* detail) ://SIO_I8 tjer
error_ (error),
detail_(detail)
{
	fms_cpf_exception_trace = new (std::nothrow) ACS_TRA_trace(fms_cpf_exception_name);
}

//------------------------------------------------------------------------------
//	Copy Constructor
//------------------------------------------------------------------------------
FMS_CPF_Exception::FMS_CPF_Exception(const FMS_CPF_Exception& object)
{
	error_  = object.error_;
	detail_ = object.detail_;
	this->fms_cpf_exception_trace = new (std::nothrow) ACS_TRA_trace(fms_cpf_exception_name);
}

FMS_CPF_Exception& FMS_CPF_Exception::operator= (const FMS_CPF_Exception & object)
{
	if(this == &object)
		return *this;
	error_  = object.error_;
	detail_ = object.detail_;
	return *this;
}

//------------------------------------------------------------------------------
//	Destructor
//------------------------------------------------------------------------------

FMS_CPF_Exception::~FMS_CPF_Exception()
{
	if (NULL != fms_cpf_exception_trace)
	{
		delete fms_cpf_exception_trace;
		fms_cpf_exception_trace = NULL;
	}
}


//------------------------------------------------------------------------------
//	Get error code
//------------------------------------------------------------------------------

FMS_CPF_Exception::errorType FMS_CPF_Exception::errorCode() const
{
	TRACE(fms_cpf_exception_trace,"errorCode(), Error code is = %d", (int)error_);
	return error_;
}


//------------------------------------------------------------------------------
//	Get error text
//------------------------------------------------------------------------------

const char* FMS_CPF_Exception::errorText () const
{
  const char* text;
  switch (error_)
  {
    case FMS_CPF_Exception::OK					:	text = ExcepetionMessages::S_OK;				break;
    case FMS_CPF_Exception::GENERAL_FAULT		:   text = ExcepetionMessages::S_GENERAL_FAULT;		break;
    case FMS_CPF_Exception::INCORRECT_USAGE		:	text = ExcepetionMessages::S_INCORRECT_USAGE;	break;
    case FMS_CPF_Exception::UNREASONABLE		:   text = ExcepetionMessages::S_UNREASONABLE;		break;
    case FMS_CPF_Exception::EXCEEDED			:   text = ExcepetionMessages::S_EXCEEDED;			break;
    case FMS_CPF_Exception::INVALIDFILE			:   text = ExcepetionMessages::S_INVALIDFILE;		break;
    case FMS_CPF_Exception::NOTBASEFILE			:   text = ExcepetionMessages::S_NOTBASEFILE;		break;
    case FMS_CPF_Exception::NOTSUBFILE			:   text = ExcepetionMessages::S_NOTSUBFILE;		break;
    case FMS_CPF_Exception::FILEEXISTS			:   text = ExcepetionMessages::S_FILEEXISTS;		break;
    case FMS_CPF_Exception::FILEISOPEN			:   text = ExcepetionMessages::S_FILEISOPEN;		break;
    case FMS_CPF_Exception::FILENOTFOUND		:   text = ExcepetionMessages::S_FILENOTFOUND;		break;
    case FMS_CPF_Exception::VOLUMENOTFOUND		:   text = ExcepetionMessages::S_VOLUMENOTFOUND;	break;
    case FMS_CPF_Exception::ACCESSERROR			:   text = ExcepetionMessages::S_ACCESSERROR;		break;
    case FMS_CPF_Exception::NOTSAMEFILE			:   text = ExcepetionMessages::S_NOTSAMEFILE;		break;
    case FMS_CPF_Exception::SAMEVOLUME			:   text = ExcepetionMessages::S_SAMEVOLUME;		break;
    case FMS_CPF_Exception::PARAMERROR			:   text = ExcepetionMessages::S_PARAMERROR;		break;
    case FMS_CPF_Exception::TYPEERROR			:   text = ExcepetionMessages::S_TYPEERROR;			break;
    case FMS_CPF_Exception::INVALIDREF			:   text = ExcepetionMessages::S_INVALIDREF;		break;
    case FMS_CPF_Exception::NOTSIMPLE			:   text = ExcepetionMessages::S_NOTSIMPLE;			break;
    case FMS_CPF_Exception::NOTCOMPOSITE		:   text = ExcepetionMessages::S_NOTCOMPOSITE;		break;
    case FMS_CPF_Exception::COMPNOTEMPTY		:   text = ExcepetionMessages::S_COMPNOTEMPTY;		break;
    case FMS_CPF_Exception::ILLVALUE			:   text = ExcepetionMessages::S_ILLVALUE;			break;
    case FMS_CPF_Exception::WRONGRECNUM			:   text = ExcepetionMessages::S_WRONGRECNUM;		break;
    case FMS_CPF_Exception::TIMEOUT				:   text = ExcepetionMessages::S_TIMEOUT;			break;
    case FMS_CPF_Exception::NOMEMORY			:   text = ExcepetionMessages::S_NOMEMORY;			break;
    case FMS_CPF_Exception::SOCKETERROR			:   text = ExcepetionMessages::S_SOCKETERROR;		break;
    case FMS_CPF_Exception::USERFAULT			:	text = ExcepetionMessages::S_USERFAULT;			break;
    case FMS_CPF_Exception::PROTOCOLERROR		:   text = ExcepetionMessages::S_PROTOCOLERROR;		break;
    case FMS_CPF_Exception::PHYSICALERROR		:   text = ExcepetionMessages::S_PHYSICALERROR;		break;
    case FMS_CPF_Exception::PHYSICALNOTFOUND	:	text = ExcepetionMessages::S_PHYSICALNOTFOUND;	break;
    case FMS_CPF_Exception::INTERNALERROR		:   text = ExcepetionMessages::S_INTERNALERROR;		break;
    case FMS_CPF_Exception::NOTIMPL				:   text = ExcepetionMessages::S_NOTIMPL;			break;
    case FMS_CPF_Exception::FILEISPROT			:   text = ExcepetionMessages::S_FILEISPROT;		break;
    case FMS_CPF_Exception::TQNOTFOUND			:   text = ExcepetionMessages::S_TQNOTFOUND;		break;
    case FMS_CPF_Exception::INVTQNAME			:   text = ExcepetionMessages::S_INVTQNAME;			break;
    case FMS_CPF_Exception::INVTRANSFERMODE		:	text = ExcepetionMessages::S_INVTRANSFERMODE;	break;
    case FMS_CPF_Exception::GOHNOSERVERACCESS	:	text = ExcepetionMessages::S_GOHNOSERVERACCESS;	break;
	case FMS_CPF_Exception::INVALIDPATH			:	text = ExcepetionMessages::S_INVALIDPATH;		break;
    case FMS_CPF_Exception::ALARM				:   text = ExcepetionMessages::S_ALARM    ;			break;
	case FMS_CPF_Exception::CPNAMENOPASSED		:	text = ExcepetionMessages::S_CPNAMENOPASSED;	break;
	case FMS_CPF_Exception::TABLEINSERTFAULT	:	text = ExcepetionMessages::S_TABLEINSERTFAULT;	break;

	case FMS_CPF_Exception::VOLUMEEXISTS		:   text = ExcepetionMessages::S_VOLUMEEXISTS;		break;
	case FMS_CPF_Exception::CSNOACCESS			:	text = ExcepetionMessages::S_CSNOACCESS;		break;
	case FMS_CPF_Exception::CSOTHERFAILURE		:	text = ExcepetionMessages::S_CSOTHERFAILURE;	break;

	case FMS_CPF_Exception::ILLOPTION			:	text = ExcepetionMessages::S_ILLOPTION;			break;
	case FMS_CPF_Exception::UNABLECONNECT		:	text = ExcepetionMessages::S_UNABLECONNECT;		break;
	case FMS_CPF_Exception::CPNOTEXISTS			:	text = ExcepetionMessages::S_CPNOTEXISTS;		break;
	case FMS_CPF_Exception::NODEISPASSIVE		:   text = ExcepetionMessages::S_NODEISPASSIVE;		break;  ////HF54685
    case FMS_CPF_Exception::ABORT				:	text = ExcepetionMessages::S_ABORT;				break;  //HK89831 && HK89834 : CPFPORT:Aborted by operator
    case FMS_CPF_Exception::BACKUPONGOING		:	text = ExcepetionMessages::S_BACKUPONGOING;		break;
    case FMS_CPF_Exception::IMM_NOVOLUME_STRUCT :   text = ExcepetionMessages::S_IMM_NOVOLUME_STRUCT; break;
    case FMS_CPF_Exception::IMM_NOFILE_FORVOL   :   text = ExcepetionMessages::S_IMM_NOFILE_FORVOL;   break;
    case FMS_CPF_Exception::IMM_NOVOLUME_FORCP  :   text = ExcepetionMessages::S_IMM_NOVOLUME_FORCP; break;
    case FMS_CPF_Exception::IMM_ERR_GET_ATTR    :   text = ExcepetionMessages::S_IMM_ERR_GET_ATTR; break;
	case FMS_CPF_Exception::ATTACHFAILED  		:   text = ExcepetionMessages::S_ATTACHFAILED; break;	//HV61830
	default										:   text = "";
  }

  TRACE(fms_cpf_exception_trace, "errorText(), Error text is = %s", text);
  return text;
}

void FMS_CPF_Exception::getSlogan(std::string& slogan)
{
	if( detail_.empty() )
	{
		slogan = errorText();
	}
	else
	{
		slogan = detail_;
	}
}
//------------------------------------------------------------------------------
//	Get detailed info about the error
//------------------------------------------------------------------------------

const std::string FMS_CPF_Exception::detailInfo() const
{
	TRACE(fms_cpf_exception_trace,"detailInfo(), Detailed info is = %s", detail_.c_str());
    return detail_;
}

//------------------------------------------------------------------------------
//	Append character string to optional textual information
//------------------------------------------------------------------------------

FMS_CPF_Exception& FMS_CPF_Exception::operator<< (const char* detail)
{
   detail_ += detail;
   return *this;
}


//------------------------------------------------------------------------------
//	Append long to optional textual information
//------------------------------------------------------------------------------

FMS_CPF_Exception& FMS_CPF_Exception::operator<< (long detail)
{
   std::stringstream buff;
   buff << detail;
   detail_ += buff.str();
   return *this;
}


//------------------------------------------------------------------------------
//	Append istream to optional textual information
//------------------------------------------------------------------------------

FMS_CPF_Exception& FMS_CPF_Exception::operator<< (std::istream& s)
{
  std::string str;
  s >> str;
  detail_ += str;
  return *this;
}


