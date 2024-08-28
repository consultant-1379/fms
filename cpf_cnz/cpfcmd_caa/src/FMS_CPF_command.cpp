//**********************************************************************
//
// NAME
//      FMS_CPF_command.C
//
// COPYRIGHT Ericsson Telecom AB, Sweden 2009.
// All rights reserved.
//
// The Copyright to the computer program(s) herein is the property of
// Ericsson Telecom AB, Sweden.
// The program(s) may be used and/or copied only with the written
// permission from Ericsson Telecom AB or in accordance with 
// the terms and conditions stipulated in the agreement/contract under
// which the program(s) have been supplied.

// DESCRIPTION
//
// See FMS_CPF_command.H
// .

// DOCUMENT NO
//	

// AUTHOR 
// 	

// REVISION
//	
//	 	

// CHANGES
//
//	RELEASE REVISION HISTORY
//
// REV NO   DATE     NAME     DESCRIPTION
//          091028   ewalbra  added cp CLUSTER option support
//            
// SEE ALSO 
//
//
//**********************************************************************

#include "FMS_CPF_command.h"
#include <ace/ACE.h>
#include "ACS_APGCC_Util.H"
using namespace std;


//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_command::FMS_CPF_command(int argc, char* argv [])
: argc_(argc),
  argv_(argv),
  _multipleCPSystem(false),
  _cpName(NULL)
{
}

/*
 * Class Name: FMS_CPF_command
 * Method Name: MultipleCpCheck
 * Description: Check the type of the CP
 */
bool FMS_CPF_command::MultipleCpCheck()
{
	/*
	 * invoke the ACS_CS_API to know if we're executing
	 * on a SingleCP System or on a MultipleCP System
	 */
	bool multipleCpSystem;

	//calling to isMultipleCpSystem method
	ACS_CS_API_NS::CS_API_Result returnValue =	ACS_CS_API_NetworkElement::isMultipleCPSystem(multipleCpSystem);

	//check return value
	if(returnValue != ACS_CS_API_NS::Result_Success)
	{//error
		switch(returnValue)
		{
		case ACS_CS_API_NS::Result_NoAccess:
			ACE_OS::printf("%s\n", ERR56);
			exit(56);
		case ACS_CS_API_NS::Result_NoEntry:
			ACE_OS::printf("%s\n", ERR55);
			exit(55);
		case ACS_CS_API_NS::Result_NoValue:
			ACE_OS::printf("%s\n", ERR56);
			exit(56);
		case ACS_CS_API_NS::Result_Failure:
			ACE_OS::printf("%s\n", ERR56);
			exit(56);
		default:
			ACE_OS::printf("%s\n", ERR56);
			exit(56);
		}//switch
	}

	//check return value OK
	
	return multipleCpSystem;
	//return true;
}

//------------------------------------------------------------------------------
//      Launch command
//------------------------------------------------------------------------------
int
FMS_CPF_command::launch ()
{
	int cmdExeResult;
	_multipleCPSystem = MultipleCpCheck();
	try
	{
		if (!ACS_APGCC::is_active_node()) throw FMS_CPF_Exception(FMS_CPF_Exception::UNABLECONNECT);

		parse();
	}
	catch(FMS_CPF_Exception &ex)
	{
		ex.getSlogan(slogan);
		cout << slogan << endl;
		return (int)ex.errorCode();
	}
	cmdExeResult = execute();
  return cmdExeResult;
}

//------------------------------------------------------------------------------
//      Common basic parsing
//------------------------------------------------------------------------------
void
FMS_CPF_command::parse() 
{	
	if (_multipleCPSystem) 
	{
		if ((argc_ < 3) || (strcasecmp(argv_[1], "-cp") != 0) || (argv_[2][0] == '-'))
		{
			throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage()); 
		}
				
		_cpName = argv_[2]; 
		
		argv_[2] = argv_[0];
		argv_ += 2;
		argc_ -= 2;		
	}
	else
	{
		for(int i = 1; i < argc_; i++)
		{
			string app(argv_[i]);
			if(app.length() == 0)
				continue; 
			string::size_type loc = app.find("-cp", 0);
			if( loc != string::npos )
			{		
				throw FMS_CPF_Exception (FMS_CPF_Exception::ILLOPTION);
			}	
		}
	}
	
}

//------------------------------------------------------------------------------
//      Detailed error information
//------------------------------------------------------------------------------

string
FMS_CPF_command::detail (const optpair_t& option) 
{
  //strstream s;
	string s;
	char str[2048];
	
	sprintf(str,"-%c %s\n",option.opt,option.arg);
	s = str;

  //s = "-" + option.opt + " " + option.arg + "\n";
  return s;
}


//------------------------------------------------------------------------------
//      Printout function for incorrect usage design
//------------------------------------------------------------------------------
void
FMS_CPF_command::incorrect_usage()
{
	string str;
	char *cpSwitch = const_cast<char *>(_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");
    str = '\n';
    str = str + argv_[0] + cpSwitch + " <volume>\n";

	cout << "usage:" << endl;
	cout << str << endl;
}

//------------------------------------------------------------------------------
//      To upper function
//------------------------------------------------------------------------------
char*
FMS_CPF_command::toUpper(char* str)
{
  int i = 0;

  while (str[i]) 
    {
      str[i] = ACE_OS::ace_toupper(str[i]);
      i++;
    }

  return str;
}

