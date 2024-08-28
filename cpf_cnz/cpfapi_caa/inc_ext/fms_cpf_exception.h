//******************************************************************************
//
// .NAME
//  	FMS_CPF_Exception - Exception handler for CP file
// .LIBRARY 3C++
// .PAGENAME FMS_CPF_Exception
// .HEADER  AP/FMS Internal
// .LEFT_FOOTER Ericsson Utvecklings AB
// .INCLUDE FMS_CPF_Exception.H
//
// .COPYRIGHT
//  COPYRIGHT Ericsson Utvecklings AB, Sweden 1997.
//  All rights reserved.
//
//  The Copyright to the computer program(s) herein
//  is the property of Ericsson Utvecklings AB, Sweden.
//  The program(s) may be used and/or copied only with
//  the written permission from Ericsson Utvecklings AB or in
//  accordance with the terms and conditions stipulated in the
//  agreement/contract under which the program(s) have been
//  supplied.
//
// .DESCRIPTION
//
//	This is the exception handler for CP file.
//
//
// .ERROR HANDLING
//
//	-
//
// DOCUMENT NO
//	190 89-CAA 109 0088
//
// AUTHOR
// 	1997-11-18 	by UAB/I/GD  	UABTSO
//
// CHANGES
//
//	CHANGE HISTORY
//
//	DATE 	NAME	DESCRIPTION
//	971118	UABTSO	1:st revision
//	980320	UABTSO	2:st revision
//  990219  QABHALL SIO I4 Ported to NT
//  990222  QABHALL SIO I5 and VC6
//			esalves
//
// .LINKAGE
//	librwtool.so
//
// .SEE ALSO
//	FMS_CPF_File.H and FMS_CPF_FileIterator.H
//
//******************************************************************************

#ifndef FMS_CPF_EXCEPTION_H
#define FMS_CPF_EXCEPTION_H


#define ERROR_ADDRESS \
	"Error detected in file'" << __FILE__ \
	<< "' at line '" << __LINE__ << "'"


#include <iostream>
#include <string>
class ACS_TRA_trace;

class FMS_CPF_Exception {
	public:

	  // Error codes. All error codes that are meaningful to a user of the
	  // API are specified. Errors that are not meaningful for a user of the
	  // API shall be specified with the FATALERROR code, such errors are
	  // sometimes refered to as internal errors.

	  // The first three error codes are defined for the purpose of
	  // being compatible with "Design Rule for AP Commands".

	 enum errorType {
		// Standard return codes for AP commands
		OK					=	0,		// Ok
		GENERAL_FAULT		=	1,		// Error when executing (general fault)
		INCORRECT_USAGE		=	2,		// Incorrect usage

		// Return codes for CP File System
		UNREASONABLE		=	16,		// Unreasonable value
		EXCEEDED			=	17,		// Value exceeded
		INVALIDFILE			=	18,		// Invalid file name
		NOTBASEFILE			=	19,		// Not a simple file or a composite main file
		NOTSUBFILE			=	20,		// Not a subfile
		FILEEXISTS			=	21,		// File exists
		FILEISOPEN			=	22,		// File is already open
		FILENOTFOUND		=	23,		// File was not found
		VOLUMENOTFOUND		=	24,		// Volume was not found
		ACCESSERROR			=	25,		// Access error
		NOTSAMEFILE			=	26,		// Subfile must belong to the same main file
		SAMEVOLUME			=	27,		// Cannot move file to same volume
		PARAMERROR			=	28,		// Parameter error
		TYPEERROR			=	29,		// Operation not permitted for this file type
		INVALIDREF			=	30,		// Invalid file reference
		NOTSIMPLE			=	31,		// Not a simple file or a subfile
		NOTCOMPOSITE		=	32,		// Not a composite main file
		COMPNOTEMPTY		=	33,		// Composite file contains subfiles
		ILLVALUE			=	34,		// Illegal value
		WRONGRECNUM			=	35,		// Wrong record number
		TIMEOUT				=	36,		// Time out on connection
		NOMEMORY			=	37,		// Out of memory
		SOCKETERROR			=	38,		// Socket error
		USERFAULT			=	39,		// API user fault
		PROTOCOLERROR		=	40,		// Protocol error
		PHYSICALERROR		=	41,		// Physical file error
		PHYSICALNOTFOUND	=	42,		// Physical file not found
		INTERNALERROR		=	43,		// Internal program fault
		NOTIMPL				=	44,		// Not (yet) implemented
		FILEISPROT			=	45,		// File is protected
		TQNOTFOUND			=   46,		// Transfer queue not found
		INVTQNAME			=   47,		// Invalid transfer queue name
		INVTRANSFERMODE		=	48,		// Invalid transfer mode
		GOHNOSERVERACCESS	=	49,		// Cannot conect to GOH server:
		INVALIDPATH			=	50,		// Invalid path
		NODEISPASSIVE		=	51,		// Node (Status) is passive   ////HF54685

		VOLUMEEXISTS		=	52,		// Volume exists

		CPNAMENOPASSED		=	53,		// The CP Name value is no passed
		TABLEINSERTFAULT	=	54,		// Insert Table in TableMap failed

		CSNOACCESS			=	55,		// Unable to access to Configuration Service
		CSOTHERFAILURE		=	56,		// Configuration Service cannot fulfill the request because of an error
		ABORT				=	57,    //Aborted by operator  //HK89831 && HK89834
		ATTACHFAILED		=	58,   //Attach failed to Block TQ   //HV61830

		BACKUPONGOING 		= 	114, 	// An AP backup is on going
		ILLOPTION			=	116,	// Illegal option in this system configuration
		UNABLECONNECT		=	117,	// Unable to connect to server

		CSNOENTRY			=	118, 	// There is no matching entry in the Configuration Service table
		CSNOVALUE			=	118, 	// The entry exists in the Configuration Service table but doesn't have a value stored for the attribute
		CPNOTEXISTS			=	118,	// The CP is not defined

		ALARM				=   200,	// Alarm
		IMM_NOVOLUME_STRUCT =   201,
		IMM_NOFILE_FORVOL   =   202,
		IMM_NOVOLUME_FORCP  =   203,
		IMM_ERR_GET_ATTR    =   204,
		//EXTERNALSYSTEMFAILURE = 100  //The external system is malfunctioning
	  };

		FMS_CPF_Exception();
		// Description:
		// Default Constructs the FMS_CPF_Exception object.


		FMS_CPF_Exception (errorType error, std::string detail = "");
		// Description:
		//	Constructs the FMS_CPF_Exception object.
		// Parameters:
		// 	errcode			Error code
		//	detail			Optional textual information
		// Additional information:
		//	None

		FMS_CPF_Exception (errorType error, const char* detail);


		virtual ~FMS_CPF_Exception ();
		// Description:
		//	Destructs the FMS_CPF_Exception object.
		// Additional information:
		//	None

		errorType errorCode () const;
		// Description:
		//	Read the error code.
		// Parameters:
		// 	None
		// Return value:
		//	errcode			Error code
		// Additional information:
		//	None

		const char* errorText () const;
		// Description:
		//	Read the error text.
		// Parameters:
		// 	None
		// Return value:
		//	errortext		Error text string
		// Additional information:
		//	None

		const std::string detailInfo () const;
		// Description:
		//	Read the detailed information.
		// Parameters:
		// 	None
		// Return value:
		//	detail			Optional textual information
		// Additional information:
		//	None

		FMS_CPF_Exception& operator<< (const char* detail);
		// Description:
		//	Append character string to optional textual information.
		// Parameters:
		// 	detail			String to be appended
		// Return value:
		//	Reference to this
		// Additional information:
		//	None

		FMS_CPF_Exception& operator<< (long detail);
		// Description:
		//	Append long to optional textual information.
		// Parameters:
		// 	detail			String to be appended
		// Return value:
		//	Reference to this
		// Additional information:
		//	None

		FMS_CPF_Exception& operator<< (std::istream& s);
		// Description:
		//	Append istream to optional textual information.
		// Parameters:
		// 	s			Stream to be appended
		// Return value:
		//	Reference to this
		// Additional information:
		//	None

		void getSlogan(std::string& );
		// Description:
		//	Get the error slogan.
		// Parameters:
		// 	The string that receive the slogan
		// Return value:
		//	None
		// Additional information:
		//	None

		FMS_CPF_Exception(const FMS_CPF_Exception& );
		// Copy constructor

		FMS_CPF_Exception& operator= (const FMS_CPF_Exception& );
		// Assignment operator


 protected:

		// Error code specified by CPF
		errorType error_;

		// Error text
		std::string detail_;

		private:
		ACS_TRA_trace *fms_cpf_exception_trace;

};



#endif /* FMS_CPF_EXCEPTION_H_ */
