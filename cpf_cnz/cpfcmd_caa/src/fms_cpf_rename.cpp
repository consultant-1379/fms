#include "fms_cpf_rename.h"
//#include <ACS_ExceptionHandler.H>  //SIO_I15.1


using namespace std;


//------------------------------------------------------------------------------
//	Constructor
//------------------------------------------------------------------------------

FMS_CPF_rename::FMS_CPF_rename (int argc, char* argv []) :
FMS_CPF_command (argc, argv),
isInfiniteSubfile(false)
{
	cpfrename = new ACS_TRA_trace("cpfrename");
		TRACE(cpfrename, "%s", "fms_cpf_rename::Constructor()");
}
FMS_CPF_rename::~FMS_CPF_rename (){
	 if (NULL != cpfrename)
	    {
	    	delete cpfrename;
	    }
}


//------------------------------------------------------------------------------
//	Command parser
//------------------------------------------------------------------------------

void
FMS_CPF_rename::parse ()
throw (FMS_CPF_Exception)
{
  //Call the base common parsing	
  FMS_CPF_command::parse();


  int c;

  ACE_Get_Opt getopt (argc_, argv_, "");

  // Parse the options

  while ((c = getopt ()) != EOF)
  {
    switch (c) 
    {
      case '?': 
      default:
		  throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage ());
    }
  }

  // Parse old file name

  if (argc_ > getopt.optind)
  {
	string filename = (argv_ [getopt.optind]); //SIO_I8 tjer
	register size_t index = filename.length();
	TRACE(cpfrename,"fms_cpf_rename parse(): filename input = %s",filename.c_str());

   if ((index > 12) && (filename.find('-') == string::npos))          // SIO_I13
	{
	   throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
	}

	//Check that the file name does not contain an underscore    //INGO3 drop 1 katj 010807
  	if (filename.find('_') != string::npos)          // SIO_I12
	{
  		throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
	}
    ACS_APGCC::toUpper(filename);

	fileid1_= filename;
	TRACE(cpfrename,"fms_cpf_rename parse(): filename input = %s, fileid1_=%s",filename.c_str(),fileid1_.file().c_str());
    if (!fileid1_.isValid ())
    {
    	//ACE_OS::exit(FMS_CPF_Exception::INVALIDFILE);
    	throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
    }

	if(filename.find('-') != string::npos)			//The filename is a subfile    //SIO_I15 begin
	{									
		isInfiniteSubfile = true;
	}																	//SIO_I15 end

    getopt.optind++;
  }
  else
  {
    throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage ());
  }

  //Check if the filename that is to be changed is a subfile or not

 
  // Parse new file name

  if (argc_ > getopt.optind)
  {
   // fileid2_ = toUpper (argv_ [getopt.optind]); SIO_I8 tjer

	string filename = (argv_ [getopt.optind]); //SIO_I8 tjer
	register size_t index = filename.length();
	TRACE(cpfrename,"fms_cpf_rename parse(): new filename input = %s",filename.c_str());
	// Check to see that the filename length is within given boundaries

	if ((index > 12) && (filename.find('-') == string::npos))          // SIO_I13
	{
		throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
	}

	//Check that the file name does not contain an underscore    //INGO3 drop 1 katj 010807
   	if (filename.find('_') != string::npos)          // SIO_I12
	{
   		throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
	}

	fileid2_ = filename;
	TRACE(cpfrename,"fms_cpf_rename parse(): filename input = %s, fileid2_=%s",filename.c_str(),fileid2_.file().c_str());
    if (!fileid2_.isValid ())
    {
    	throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
    }
    getopt.optind++;
  }
  else
  {
    throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage ());
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
FMS_CPF_rename::execute ()
throw (FMS_CPF_Exception)
{
	TRACE(cpfrename, "%s", "fms_cpf_rename::Execute()");

  try
  {
	  FMS_CPF_File file (fileid1_.data (), _multipleCPSystem ? _cpName : "");
	  TRACE(cpfrename, "fms_cpf_rename:execute() Verify that %s exists", fileid1_.file().c_str());
	/*  if(!file.exists()){
		  TRACE(cpfrename,"fms_cpf_rename execute(): old file = %s not exists",fileid1_.data());
		  FMS_CPF_Exception ex(FMS_CPF_Exception::FILENOTFOUND,fileid1_.data());
		  ex.getSlogan(slogan);
		  cout << slogan << endl;
		  return (int)ex.errorCode();
	  }
*/
	  FMS_CPF_File file2 (fileid2_.data (), _multipleCPSystem ? _cpName : "");

	/*  if(file2.exists()){
		  TRACE(cpfrename,"fms_cpf_rename execute(): new file = %s exists",fileid2_.data());
		  FMS_CPF_Exception ex(FMS_CPF_Exception::FILEEXISTS,fileid2_.data());
		  ex.getSlogan(slogan);
		  cout << slogan << endl;
		  return (int)ex.errorCode();
	  }	*/																		//SIO_I15 end
	  file.rename (fileid2_.data ());
  }
  catch (FMS_CPF_Exception& ex)
  {
	ex.getSlogan(slogan);
	cout << slogan << endl;
	return (int)ex.errorCode();
  }
  //file.unreserve ();
  return 0;
}


//------------------------------------------------------------------------------
//	Command usage
//------------------------------------------------------------------------------
string
FMS_CPF_rename::usage ()
{
	std::string str("Incorrect usage\nUsage:\n");
	char *cpSwitch = const_cast<char *>(_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");

	str += std::string("cpfrename") + cpSwitch + std::string(" file1 file2\n");
	str += std::string("cpfrename") + cpSwitch + std::string(" file-subfile1[-generation1] file-subfile2[-generation2]\n");
	
	return str;
}

void
FMS_CPF_rename::usage (int eCode)
{
	string str;
	char *cpSwitch = const_cast<char *>(_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");


	str = '\n';
	str = str + "Usage: \n";
	str = str + "cpfrename" + cpSwitch + " file1 file2\n";
	str = str + "cpfrename" + cpSwitch + " file-subfile1[-generation1] " + "file-subfile2[-generation2]\n";

	cout << str << endl;
	ACE_OS::exit(eCode);
}



//------------------------------------------------------------------------------
//	Main
//------------------------------------------------------------------------------

int
main (int argc, char* argv [])
{
	FMS_CPF_rename rename (argc, argv);
	return rename.launch ();
}
