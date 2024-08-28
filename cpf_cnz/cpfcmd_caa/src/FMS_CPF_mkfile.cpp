#include "FMS_CPF_mkfile.h"
#include <ace/ACE.h>
#include "ace/ARGV.h"
#include "ace/Get_Opt.h"
#include "ACS_APGCC_Util.H"
#include "ACS_TRA_trace.h"

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_mkfile::FMS_CPF_mkfile (int argc, char* argv []) :
FMS_CPF_command (argc, argv),
volume_(NULL)
{
	cpfmkfile = new ACS_TRA_trace("cpfmkfile");
	TRACE(cpfmkfile, "%s", "FMS_CPF_mkfile::Constructor()");
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------

FMS_CPF_mkfile::~FMS_CPF_mkfile ()
{
    if (NULL != cpfmkfile)
    {
    	delete cpfmkfile;
    }
}

//------------------------------------------------------------------------------
//      Command parser
//------------------------------------------------------------------------------
void
FMS_CPF_mkfile::parse ()
throw (FMS_CPF_Exception)
{
	//Call the base common parsing
	FMS_CPF_command::parse();
	
	int       c;
	size_t found;
	option_t  option = FTYPE;
	optpair_t optlist [] = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}};
	
	ACE_Get_Opt getopt (argc_, argv_, "d:f:kl:m:cs:t:r");
	
	
	// Parse the options
	while ((c = getopt ()) != EOF) {
		switch (c) {
		case 'd': option = TRANSFERQUEUE; break;
		case 'f': option = FTYPE;     break;
		case 'l': option = RLENGTH;   break;
		case 'c': option = COMPOSITE; break;
		case 's': option = MAXSIZE;   break;
		case 't': option = MAXTIME;   break;
		case 'r': option = RELEASE;   break;
		case '?': 
		default:
		  throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage()); 
			
		}
		if (!optlist [option].opt) {
			optlist [option].opt = c;
			optlist [option].arg = getopt.optarg;
		}
		else {
			throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage()); 
		}
	}
	
	// Fetch the file name
	if (argc_ > getopt.optind) 
	{
		string filename = (argv_ [getopt.optind]);
		register size_t index = filename.length();
		TRACE(cpfmkfile, "FMS_CPF_mkfile filename input:<%s>",filename.c_str());
		//Check that the file name does not contain an underscore
		found = filename.find('_');
		if (found != string::npos) {
			TRACE(cpfmkfile, "FMS_CPF_mkfile EXCEPTION _ filename:<%s>",filename.c_str());
			throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
		}
		
		// Check to see that the filename length is within given boundaries
		found = filename.find('-');
		if ((index > 12) && (found == string::npos)) {
			TRACE(cpfmkfile, "FMS_CPF_mkfile EXCEPTION - filename:%s",filename.c_str());
			throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
		}

		fileid_ = filename;

		TRACE(cpfmkfile, "FMS_CPF_mkfile(), filename:<%s>",fileid_.file().c_str(), filename.c_str());
		if (!fileid_.isValid ())
		{
			throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
		}
		
		getopt.optind++;
	}
	else 
	{
		throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage()); 
	}

	// Fetch option for compress
	compress_ = optlist[COMPRESS].opt ? true:false;

	if (fileid_.subfile ().empty () == true) {
		// Main file
		// Fetch the volume name
		if (argc_ > getopt.optind) {
			volume_ = argv_ [getopt.optind];
			getopt.optind++;
		}
		else {
			throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage()); 
		}
		
		// Fetch option argument for file type
		if (optlist [FTYPE].opt) {
			if (!ACE_OS::strcmp (optlist [FTYPE].arg, S_REGULAR) || !ACE_OS::strcmp (optlist [FTYPE].arg, s_regular)) {
				ftype_ = FMS_CPF_Types::ft_REGULAR;
			}
			else if (!ACE_OS::strcmp (optlist [FTYPE].arg, S_INFINITE) || !ACE_OS::strcmp (optlist [FTYPE].arg, s_infinite)) {
				ftype_ = FMS_CPF_Types::ft_INFINITE;
			}
			else
			{
				FMS_CPF_Exception exErr(FMS_CPF_Exception::UNREASONABLE);
				std::string errMsg(exErr.errorText());
				errMsg += detail (optlist[FTYPE]);
				throw FMS_CPF_Exception (FMS_CPF_Exception::UNREASONABLE, errMsg);
			}
		}
		else {
			ftype_ = FMS_CPF_Types::ft_REGULAR;
		}
		
		// Check the attributes against the file type
		switch (ftype_) {
		case FMS_CPF_Types::ft_REGULAR:
			if (( optlist[RLENGTH].opt &&
				!optlist[MAXSIZE].opt &&
				!optlist[MAXTIME].opt && 
				!optlist[RELEASE].opt &&
				!optlist[TRANSFERQUEUE].opt) == 0)
			{
				throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage());
			}
			break;
			
		case FMS_CPF_Types::ft_INFINITE:
			if (( optlist [RLENGTH].opt && !optlist [COMPOSITE].opt) == 0) {
				throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage());
			}
			break;
			
		default: 
		  throw FMS_CPF_Exception (FMS_CPF_Exception::INTERNALERROR);
		}

		// Fetch option argument for record length
		if (optlist[RLENGTH].opt) {
			int num = ACE_OS::atoi (optlist [RLENGTH].arg);
			rlength_ = num;
			if (rlength_ < 1 || rlength_ > 65535)
			{
				FMS_CPF_Exception exErr(FMS_CPF_Exception::UNREASONABLE);
				std::string errMsg(exErr.errorText());
				errMsg += detail (optlist[RLENGTH]);
				throw FMS_CPF_Exception (FMS_CPF_Exception::UNREASONABLE, errMsg);
			}
		}
		
		// Fetch option for file structure 
		composite_ = optlist [COMPOSITE].opt ? true: false;
				
		
		// Fetch option argument for max size
		if (optlist [MAXSIZE].opt)
		{
			char* ptr = 0;
			maxsize_ = ACE_OS::strtoul (optlist [MAXSIZE].arg, &ptr, 10);
			if( (*ptr) || (maxsize_ == 0))
			{
				FMS_CPF_Exception exErr(FMS_CPF_Exception::UNREASONABLE);
				std::string errMsg(exErr.errorText());
				errMsg += detail (optlist[MAXSIZE]);
				throw FMS_CPF_Exception(FMS_CPF_Exception::UNREASONABLE, errMsg);
			}
		}
		else
		{
			maxsize_ = 0;
		}
		
		// Fetch option argument for max time
		if (optlist [MAXTIME].opt)
		{
			char* ptr = 0;
			maxtime_ = ACE_OS::strtoul (optlist [MAXTIME].arg, &ptr, 10);
			if( (*ptr)  || (maxtime_ == 0) )
			{
				FMS_CPF_Exception exErr(FMS_CPF_Exception::UNREASONABLE);
				std::string errMsg(exErr.errorText());
				errMsg += detail (optlist[MAXTIME]);

				throw FMS_CPF_Exception(FMS_CPF_Exception::UNREASONABLE, errMsg);
			}

		}
		else
		{
			maxtime_ = 0;
		}
		
		// Fetch option for release condition 
		release_ = optlist [RELEASE].opt ? true: false;
		
		// Fetch option argument for transfer queue
		if (optlist [TRANSFERQUEUE].opt)
		{
			if (ACE_OS::strlen(optlist [TRANSFERQUEUE].arg) > (size_t) FMS_CPF_TQMAXLENGTH) {
				throw FMS_CPF_Exception (FMS_CPF_Exception::INVTQNAME);
			}
			transferQueue_ = optlist [TRANSFERQUEUE].arg;

		}
		else
		{
			transferQueue_ = "";
		}
  }
  else {
	  // Subfile
	  if ((!optlist [FTYPE].opt
		  && !optlist [RLENGTH].opt
		  && !optlist [COMPOSITE].opt
		  && !optlist [MAXSIZE].opt
		  && !optlist [MAXTIME].opt
		  && !optlist [RELEASE].opt
		  && !optlist [TRANSFERQUEUE].opt == 0)) {
		  throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage());
	  }
  }
  if (argc_ != getopt.optind) {
	  throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage());
  }
}


//------------------------------------------------------------------------------
//      Execute command
//------------------------------------------------------------------------------
int FMS_CPF_mkfile::execute() throw(FMS_CPF_Exception)
{   
	FMS_CPF_File file(fileid_.data(), _multipleCPSystem ? _cpName : "");
	
	if(fileid_.subfile().empty())
	{
		// Main file
		FMS_CPF_Types::fileAttributes attributes;
		
		attributes.ftype = ftype_;
		switch (ftype_) 
		{
			case FMS_CPF_Types::ft_REGULAR:
				 attributes.regular.rlength = rlength_;
				 attributes.regular.composite = composite_;
			     	 attributes.regular.deleteFileTimer = -1;
				break;

			case FMS_CPF_Types::ft_INFINITE:
				attributes.infinite.rlength = rlength_;
				attributes.infinite.maxsize = maxsize_;
				attributes.infinite.maxtime = maxtime_;
				attributes.infinite.release = release_;
				(void) ACE_OS::strncpy(attributes.infinite.transferQueue, transferQueue_.c_str(), FMS_CPF_TQMAXLENGTH);
				attributes.infinite.transferQueue[FMS_CPF_TQMAXLENGTH] = '\0';
				attributes.infinite.mode = FMS_CPF_Types::tm_NONE;
				TRACE(cpfmkfile, "FMS_CPF_mkfile(), infinite file name:<%s>, TQ:<%s>, transfer mode:<%d>", fileid_.data(), attributes.infinite.transferQueue, attributes.infinite.mode);
				break;
			default:
				throw FMS_CPF_Exception(FMS_CPF_Exception::INTERNALERROR);
		}

		file.create(attributes, volume_, compress_);
	}
	else 
	{
		file.create(compress_);
	}
	
	return 0;
}

//------------------------------------------------------------------------------
//      Command usage
//------------------------------------------------------------------------------
void
FMS_CPF_mkfile::usage (int eCode){
	string str;
	char *cpSwitch = const_cast<char *>(_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");

	str = '\n';
	str = str + "Usage: \n";
	str = str + "cpfmkfile"  + cpSwitch +  " [-f " + S_REGULAR + "] -l rlength [-c] file volume\n";
	str = str + "cpfmkfile"  + cpSwitch +  " -f " + S_INFINITE + " -l rlength [-r] [-s maxsize]\n";
    str = str + "           [-t maxtime] file volume\n"; 
	str = str + "cpfmkfile"  + cpSwitch +  " -f " + S_INFINITE + " -d transferqueue -l rlength \n";
	str = str + "           [-r] [-s maxsize] [-t maxtime] file volume\n";
	str = str + "cpfmkfile"  + cpSwitch +  " file-subfile[-generation]\n";
	
	cout << str << endl;
	ACE_OS::exit(eCode);
}

string
FMS_CPF_mkfile::usage ()
{
	std::string str("Incorrect usage\n");
	char *cpSwitch = const_cast<char *>(_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");

	str = str + "Usage: \n";
	str = str + "cpfmkfile"  + cpSwitch +  " [-f " + S_REGULAR + "] -l rlength [-c] file volume\n";
	str = str + "cpfmkfile"  + cpSwitch +  " -f " + S_INFINITE + " -l rlength [-r] [-s maxsize]\n";
    str = str + "           [-t maxtime] file volume\n"; 
	str = str + "cpfmkfile"  + cpSwitch +  " -f " + S_INFINITE + " -d transferqueue -l rlength \n";
	str = str + "           [-r] [-s maxsize] [-t maxtime] file volume\n";
	str = str + "cpfmkfile"  + cpSwitch +  " file-subfile[-generation]\n";
	
	return str;
}


//------------------------------------------------------------------------------
//      Main
//------------------------------------------------------------------------------
int main (int argc, char* argv [])
{
	try
	{
	  FMS_CPF_mkfile mkfile (argc, argv);
	  return mkfile.launch ();
	}
	catch (FMS_CPF_Exception& ex) 
	{
		std::string slogan;
		ex.getSlogan(slogan);
		cout << slogan << endl;
		return (int)(ex.errorCode());
	}
}
