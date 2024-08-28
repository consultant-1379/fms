#include "FMS_CPF_mkvolume.h"
#include <ace/ACE.h>
#include "ace/ARGV.h"
#include "ace/Get_Opt.h"



//------------------------------------------------------------------------------
//	Constructor
//------------------------------------------------------------------------------

FMS_CPF_mkvolume::FMS_CPF_mkvolume (int argc, char* argv [], FMS_CPF_ServerStub stub) :
FMS_CPF_command (argc, argv)
{
  m_stub = stub;
  ACE_OS::memset(m_pVolume, 0, MAX_VOLUME_LEN+1);
}

FMS_CPF_mkvolume::~FMS_CPF_mkvolume()
{
}


//------------------------------------------------------------------------------
//	Command parser
//------------------------------------------------------------------------------

void
FMS_CPF_mkvolume::parse ()
{
	int       c;
	
	//Call the base common parsing
	FMS_CPF_command::parse();
	
	ACE_Get_Opt getopt (argc_, argv_, "");
	// Parse the options
	//cpfmkvolume must not accept options
	while ((c = getopt ()) != EOF) {
		switch (c) {
			case '?':
			default:
				throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage());
		}
	}

	if ((argc_ == 2) && (argv_[1] != NULL))
	{
		if( strlen(argv_[1]) <= MAX_VOLUME_LEN && strlen(argv_[1]) > 0)
		{
		  ACE_OS::strncpy(m_pVolume, argv_[1], MAX_VOLUME_LEN);
		}
		else {
			cout << INVALID_VOLUME << endl;
			ACE_OS::exit(VOLUME_NOT_VALID);
		}
			
	}	
	else
	{
		throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage());
	}
}



//------------------------------------------------------------------------------
//	Execute command
//------------------------------------------------------------------------------
int FMS_CPF_mkvolume::execute ()
{
	char *cpn = const_cast<char *>(_multipleCPSystem ? _cpName : "");
	return m_stub.mkvolume(m_pVolume, cpn);
}


//------------------------------------------------------------------------------
//	Print command usage
//------------------------------------------------------------------------------

void FMS_CPF_mkvolume::usage (int eCode)
{
   string str;
   char *cpSwitch = const_cast<char *>(_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");

   str = '\n';
   str = str + "Usage: \n";
   str = str + "cpfmkvol" + cpSwitch + " <volume>\n";

   cout << str << endl;
   ACE_OS::exit(eCode);
}

std::string FMS_CPF_mkvolume::usage ()
{
	std::string str("Incorrect usage\n");
    char *cpSwitch = const_cast<char *>(_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");

    str += std::string("Usage: \n");
    str += std::string("cpfmkvol") + cpSwitch + std::string(" <volume>\n");

    return str;
}


//------------------------------------------------------------------------------
//	Main
//------------------------------------------------------------------------------

int main (int argc, char* argv [])
{

    FMS_CPF_ServerStub stub;				//create Receiver
	FMS_CPF_mkvolume mkv(argc, argv, stub); //create Concrete Command
	FMS_CPF_CommandInterface invoker(&mkv);	//create Invoker

	//invoke command
	return invoker.cpfmkvolume();
}

