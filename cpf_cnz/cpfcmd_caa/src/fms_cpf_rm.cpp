#include "fms_cpf_rm.h"
//#include <ACS_ExceptionHandler.H>  //SIO_I15.1

using namespace std;


//------------------------------------------------------------------------------
//	Constructor
//------------------------------------------------------------------------------

FMS_CPF_rm::FMS_CPF_rm (int argc, char* argv []) :
FMS_CPF_command (argc, argv),
recursive_(false)
{
}


//------------------------------------------------------------------------------
//	Command parser
//------------------------------------------------------------------------------

void
FMS_CPF_rm::parse ()
throw (FMS_CPF_Exception)
{
  //Call the base common parsing	
  FMS_CPF_command::parse();

  int       c;
  option_t  option;
  optpair_t optlist [] = {{0,0}};

  ACE_Get_Opt getopt (argc_, argv_, "r");

  // Parse the options

  while ((c = getopt ()) != EOF)
  {
    switch (c) 
    {
      case 'r': option = RECURSIVE; break;
      case '?':
      default:
          throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage ());
    }
    if (!optlist [option].opt)
    {
      optlist [option].opt = c;
      optlist [option].arg = getopt.optarg;
    }
    else
    {
        throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage ());
    }
  }

  // Parse the file name

  if (argc_ > getopt.optind)
  {
//    fileid_ = toUpper (argv_ [getopt.optind]); SIO_I8 tjer

	string filename = (argv_ [getopt.optind]); //SIO_I8 tjer
	register size_t index = filename.length();

   if ((index > 12) && (filename.find('-') == string::npos))          // SIO_I12
   {
     throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
   }

/*	while(index--)
	{
		filename[index] = toupper((unsigned char)filename[index]);
	}
*/
   	ACS_APGCC::toUpper(filename);
	fileid_ = filename; //SIO_I8 tjer

    if (!fileid_.isValid ())
    {
     throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
    }
    getopt.optind++;
  }
  else
  {
    throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage ());
  }

  if (fileid_.subfile ().empty () == true)  //SIO_I8 tjer
  {
    // Main file

    // Recursive option

    recursive_ = optlist [RECURSIVE].opt ? true: false;
  }
  else
  {
    // Subfile

    if (optlist [RECURSIVE].opt)
    {
      throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage ());
    }
  }

  if (argc_ != getopt.optind)
  {
    throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage ());
  }
}



//------------------------------------------------------------------------------
//	Execute command
//------------------------------------------------------------------------------

int
FMS_CPF_rm::execute ()
throw (FMS_CPF_Exception)
{
	
  FMS_CPF_File file (fileid_.data (), _multipleCPSystem ? _cpName : "");
  file.deleteFile(recursive_);

  return 0;
}


//------------------------------------------------------------------------------
//	Print command usage
//------------------------------------------------------------------------------

string
FMS_CPF_rm::usage ()
{
   std::string str("Incorrect usage\nUsage:\n");
   char *cpSwitch = const_cast<char *>(_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");

   str += std::string("cpfrm") + cpSwitch + std::string(" [-r] file\n");
   str +=  std::string("cpfrm") + cpSwitch + std::string(" file-subfile[-generation]\n");

   return str;
}

void
FMS_CPF_rm::usage (int eCode)
{
	std::string str("Incorrect usage\n");
	char *cpSwitch = const_cast<char *>(_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");


  str = str + "Usage: \n";

  str = str + "cpfrm" + cpSwitch + " [-r] file\n";
  str = str + "cpfrm" + cpSwitch + " file-subfile[-generation]\n";

  cout << str << endl;

  ACE_OS::exit(eCode);
}


//------------------------------------------------------------------------------
//	Main
//------------------------------------------------------------------------------

int
main (int argc, char* argv [])
{
	try
    {
	  FMS_CPF_rm rm (argc, argv);
	  return rm.launch ();
	}
	catch (FMS_CPF_Exception& ex) 
	{
		std::string slogan;
		ex.getSlogan(slogan);
		cout << slogan << endl;
		return (int)(ex.errorCode());
	}
}
