#include "fms_cpf_cp.h"

using namespace std;


//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_cp::FMS_CPF_cp (int argc, char* argv []) :
FMS_CPF_command (argc, argv),
mode_(FMS_CPF_Types::cm_NORMAL)
{
	cpfcp = new ACS_TRA_trace("cpfcp");
	TRACE(cpfcp, "%s", "FMS_CPF_cp::Constructor()");
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------

FMS_CPF_cp::~FMS_CPF_cp ()
{
    if (NULL != cpfcp)
    {
    	delete cpfcp;
    }
}

//------------------------------------------------------------------------------
//      Command parser
//------------------------------------------------------------------------------
void
FMS_CPF_cp::parse ()
throw (FMS_CPF_Exception)
{
  //Call the base common parsing	
  FMS_CPF_command::parse();

  int c;
  option_t  option;
  optpair_t optlist [] = {{0,0}};

  ACE_Get_Opt getopt (argc_, argv_, "m:");
  // Parse the options
  while ((c = getopt ()) != EOF) {
    switch (c) {
      case 'm': option = MODE;   break;
      case '?':// fall through
      default:
          throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage ());
    }
    if (!optlist [option].opt) {
      optlist [option].opt = c;
      optlist [option].arg = getopt.optarg;
    }
    else {
        throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage ());
    }
  }
  // Parse source file name
  if (argc_ > getopt.optind) {
		string filename = (argv_ [getopt.optind]);
		TRACE(cpfcp, "FMS_CPF_cp::parse() filename1 = %s", filename.c_str());
		register size_t index = filename.length();
	if ((index > 12) && (filename.find('-') == std::string::npos)) {
		 throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
	}

	//ACS_APGCC::toUpper(filename);
	fileid1_ = filename;
	TRACE(cpfcp, "FMS_CPF_cp::parse() filename = %s, fileid1_=%s", filename.c_str(), fileid1_.data());
    if (!fileid1_.isValid ()) {
      throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
    }
    getopt.optind++;
  }
  else {
      throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage ());
  }
  // Parse destination file name
  if (argc_ > getopt.optind) {
		string filename = (argv_ [getopt.optind]);
		register size_t index = filename.length();
		TRACE(cpfcp, "FMS_CPF_cp::parse() filename2 = %s", filename.c_str());
		if ((index > 12) && (filename.find('-') == std::string::npos))	{
		   throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
	    }

		//ACS_APGCC::toUpper(filename);
	    fileid2_ = filename;
	    TRACE(cpfcp, "FMS_CPF_cp::parse() filename = %s, fileid2_=%s", filename.c_str(), fileid2_.data());
		if (!fileid2_.isValid ()) {
		  throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
		}
        getopt.optind++;
  }
  else {
      throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage ());
  }
  // Fetch copying mode
  if (optlist [MODE].opt) {
    if (!strcmp (optlist [MODE].arg, S_CLEAR) || !strcmp (optlist [MODE].arg, s_clear)) {
      mode_ = FMS_CPF_Types::cm_CLEAR;
    }
    else if (!strcmp (optlist [MODE].arg, S_OVERWRITE) || !strcmp (optlist [MODE].arg, s_overwrite)) {
      mode_ = FMS_CPF_Types::cm_OVERWRITE;
    }
    else if (!strcmp (optlist [MODE].arg, S_APPEND) ||!strcmp (optlist [MODE].arg, s_append)) {
      mode_ = FMS_CPF_Types::cm_APPEND;
    }
    else
    {
    	FMS_CPF_Exception exErr(FMS_CPF_Exception::UNREASONABLE);
    	std::string errMsg(exErr.errorText());
    	errMsg += detail (optlist [MODE]);
    	throw FMS_CPF_Exception(FMS_CPF_Exception::UNREASONABLE, errMsg);
    }
  }
  else {
    mode_ = FMS_CPF_Types::cm_NORMAL;
  }
  if (argc_ != getopt.optind) {
      throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage ());
  }
}

//------------------------------------------------------------------------------
//      Execute command
//------------------------------------------------------------------------------
int FMS_CPF_cp::execute ()
throw (FMS_CPF_Exception)
{
  try {
   FMS_CPF_File file (fileid1_.data (), _multipleCPSystem ? _cpName : "");
   FMS_CPF_File file2(fileid2_.data(), _multipleCPSystem ? _cpName : "");
   file.copy (fileid2_.data (), mode_);
  }
  catch (FMS_CPF_Exception& ex) 
  {  	  
	ex.getSlogan(slogan);
	cout << slogan << endl;
	return (int)(ex.errorCode());
  }
  return 0;
}

//------------------------------------------------------------------------------
//      Print command usage
//------------------------------------------------------------------------------
string 
FMS_CPF_cp::usage ()
{
	std::string str("Incorrect usage\n");
   char *cpSwitch = const_cast<char *>(_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");

   str = str + "Usage: \n";
   str = str + "cpfcp" + cpSwitch + " [-m mode] file1[-subfile1[-generation1]] ";
   str = str + "file2[-subfile2[-generation2]]\n";
   return str;
}

void
FMS_CPF_cp::usage (int eCode)
{
	string str;
	char *cpSwitch = const_cast<char *>(_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");
	str = '\n';
	str = str + "Usage: \n";
	str = str + "cpfcp" + cpSwitch + " [-m mode] file1[-subfile1[-generation1]] ";
	str = str + "file2[-subfile2[-generation2]]\n";
	cout << str << endl;
	ACE_OS::exit(eCode);
}

//------------------------------------------------------------------------------
//      Main
//------------------------------------------------------------------------------
int
main (int argc, char* argv [])
{
	FMS_CPF_cp cp (argc, argv);
	return cp.launch ();
}
