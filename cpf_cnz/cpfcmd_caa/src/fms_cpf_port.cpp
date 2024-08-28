#include "fms_cpf_port.h"
#include "fms_cpf_file.h"
#include "fms_cpf_trace.h"

#include "ACS_TRA_trace.h"
#include "ACS_APGCC_Util.H"
#include "ACS_APGCC_CommonLib.h"

#include <ace/ACE.h>
#include "ace/ARGV.h"
#include "ace/Get_Opt.h"

#define BOOST_FILESYSTEM_VERSION 3

#include <boost/filesystem.hpp>
#include <boost/bind.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/thread/thread.hpp>

namespace {
	const char DirDelim ='/';
	const char fileMCpFiles[] = "cpFiles";
	long long int MaxByteSizeLimit = 2L*1024L*1024L*1024L; //2GB
	const char prompt_[]= "\x03:";		// ETX :   //HO36174

	unsigned int oneSec = 1U;
	const unsigned int dotsDelaySec = 5U;

	const char warningMsg[] = "The file to be ported is more than 2GB.\nThis may take several minutes to complete.\n";
	const char question1Msg[] = "\nDo you want to continue?";
	const char question2Msg[] = "\nEnter Y(ES) or N(O)[default=Y]";

	const char errorInputMsg[] = "Invalid input\n";
	const char waitMsg[] = "Porting is ongoing\n";
	const char endMsg[] = "\nPorting completed successfully.\n";
}

using namespace std;
namespace fs = boost::filesystem;

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_port::FMS_CPF_port(int argc, char* argv[]) :
	FMS_CPF_command(argc, argv),
	zipOpt(false),
	direction_(IN_),
	mode_(FMS_CPF_Types::cm_NORMAL),
	path_(NULL),
	portingFinished(false),
	m_force(false)
{
	cpfport = new (std::nothrow) ACS_TRA_trace("cpfport");
}

FMS_CPF_port::~FMS_CPF_port ()
{
    if (NULL != cpfport)
    {
    	delete cpfport;
    }
}

//------------------------------------------------------------------------------
//      Command parser
//------------------------------------------------------------------------------
void FMS_CPF_port::parse() throw(FMS_CPF_Exception)
{
	//Call the base common parsing
	FMS_CPF_command::parse();

	int c;

    option_t  option;
	optpair_t optlist [] = {{0,0},{0,0},{0,0},{0,0},{0,0}};

	ACE_Get_Opt getopt (argc_, argv_, "eim:zf");
	zipOpt = false;

	// Parse the options
	while( (c = getopt()) != EOF )
	{
		switch(c)
		{
			case 'e': option = EXPORT; break;
			case 'i': option = IMPORT; break;
			case 'm': option = MODE;   break;
			case 'z':
			{
				option = ZIP;
				zipOpt = true;
			}
			break;

			case 'f':
			{
				option = FORCE;
				m_force = true;
			}
			break;

			case '?':
			default:
				throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage() );
		}

		if( !optlist [option].opt )
		{
			optlist [option].opt = c;
			optlist [option].arg = getopt.optarg;
		}
		else
		{
			throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage() );
		}
	}

	TRACE(cpfport, "parse(), zipOpt [%d]", zipOpt);

	// Fetch option argument for export or import
	if (optlist[EXPORT].opt && !optlist[IMPORT].opt)
	{
		  direction_ = OUT_;

		  // Fetch the file name
		  if (argc_ > getopt.optind)
		  {
			  std::string filename = (argv_ [getopt.optind]);
			  ACS_APGCC::toUpper(filename);
			  TRACE(cpfport, "parse(), DIROUT filename:<%s>", filename.c_str());

			  fileid_ = filename;

			  if (!fileid_.isValid ())
			  {
				  TRACE(cpfport, "parse(), filename:<%s> not valid", filename.c_str());
				  throw FMS_CPF_Exception (FMS_CPF_Exception::INVALIDFILE);
			  }
			  getopt.optind++;
		  }
		  else
		  {
			  throw FMS_CPF_Exception(FMS_CPF_Exception::INCORRECT_USAGE, usage());
		  }

		  // Fetch the path name

		  if (argc_ > getopt.optind)
		  {
			  path_ = argv_[getopt.optind];
			  getopt.optind++;
		  }
		  else
		  {
			  throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage());
		  }
	}
	else if (!optlist [EXPORT].opt && optlist [IMPORT].opt)
	{
		  direction_ = IN_;

		  // Fetch the path name
		  if (argc_ > getopt.optind)
		  {
			  path_ = argv_ [getopt.optind];
			  getopt.optind++;
		  }
		  else
		  {
			  throw FMS_CPF_Exception(FMS_CPF_Exception::INCORRECT_USAGE, usage ());
		  }

		  // Fetch the file name
		  if (argc_ > getopt.optind)
		  {
			  std::string filename = (argv_ [getopt.optind]);
			  ACS_APGCC::toUpper(filename);
			  TRACE(cpfport, "parse(), DIRIN filename:<%s>", filename.c_str());

			  fileid_ = filename;

			  if (!fileid_.isValid ())
			  {
				  TRACE(cpfport, "FMS_CPF_port::parse() DIRIN NOT VALID !!! filename [%s]", filename.c_str());
				  throw FMS_CPF_Exception(FMS_CPF_Exception::INVALIDFILE);
			  }

			  getopt.optind++;
		  }
		  else
		  {
			  throw FMS_CPF_Exception(FMS_CPF_Exception::INCORRECT_USAGE, usage());
		  }
	}
	else
	{
		throw FMS_CPF_Exception(FMS_CPF_Exception::INCORRECT_USAGE, usage());
	}

	  // Fetch copying mode
	  if (optlist [MODE].opt)
	  {
		  if( !ACE_OS::strcmp (optlist [MODE].arg, S_CLEAR) || !ACE_OS::strcmp (optlist [MODE].arg, s_clear))
		  {
			  mode_ = FMS_CPF_Types::cm_CLEAR;
		  }
		  else if( !ACE_OS::strcmp (optlist [MODE].arg, S_OVERWRITE) || !ACE_OS::strcmp (optlist [MODE].arg, s_overwrite))
		  {
			  mode_ = FMS_CPF_Types::cm_OVERWRITE;
		  }
		  else if( !ACE_OS::strcmp (optlist [MODE].arg, S_APPEND) || !ACE_OS::strcmp (optlist [MODE].arg, s_append))
		  {
			  mode_ = FMS_CPF_Types::cm_APPEND;
		  }
		  else
		  {
			  FMS_CPF_Exception exErr(FMS_CPF_Exception::UNREASONABLE);
			  std::string errMsg(exErr.errorText());
			  errMsg += detail(optlist[MODE]);
			  throw FMS_CPF_Exception(FMS_CPF_Exception::UNREASONABLE, errMsg);
		  }
	  }
	  else
	  {
		  mode_ = FMS_CPF_Types::cm_NORMAL;
	  }

	  if(zipOpt && (direction_== IN_))
	  {
		  throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage() );
	  }

	  if( (direction_ == OUT_) && (mode_ == FMS_CPF_Types::cm_CLEAR) )
	  {
		  FMS_CPF_Exception exErr(FMS_CPF_Exception::UNREASONABLE);
		  std::string errMsg(exErr.errorText());
		  errMsg += detail (optlist[MODE]);
		  throw FMS_CPF_Exception(FMS_CPF_Exception::UNREASONABLE, errMsg);
	  }

	  if (argc_ != getopt.optind)
	  {
		  throw FMS_CPF_Exception(FMS_CPF_Exception::INCORRECT_USAGE, usage() );
	  }
}

//------------------------------------------------------------------------------
//      Execute command
//------------------------------------------------------------------------------
int FMS_CPF_port::execute() throw(FMS_CPF_Exception)
{
	FMS_CPF_File file(fileid_.data(), _multipleCPSystem ? _cpName : "");
	std::string realpath_(path_);
	boost::thread dotPrinterThread;
	bool threadStart = false;

	TRACE(cpfport, "execute(), MODE:<%d>, file:<%s>, realpath_:<%s>", mode_, fileid_.data(), realpath_.c_str() );
  
	if (direction_ == OUT_)
	{
		TRACE(cpfport, "execute(), EXPORT, force[%s]", (m_force? "YES":"NO"));
		if(!m_force)
		{
			std::string dstAbsolutePath;
			if( getAbsolutePath(realpath_, dstAbsolutePath))
			{
				file.reserve(FMS_CPF_Types::NONE_);
				std::string absolutePath = file.getPhysicalPath();
				file.unreserve();
				threadStart = checkFileSize(absolutePath);
				if(threadStart)
					dotPrinterThread = boost::thread(&FMS_CPF_port::printDots, this);
			}
			else
			{
				throw FMS_CPF_Exception(FMS_CPF_Exception::INVALIDPATH);
			}
		}

		try
		{
			file.fileExport(realpath_, mode_, zipOpt);
		}
		catch(const FMS_CPF_Exception& ex)
		{
			// check if thread for dot printing is up
			if(threadStart)
			{
				portingFinished=true;
				dotPrinterThread.join();
				cout << endl;
			}
			// Re-throw same exception
			throw;
		}
	}
	else
	{
		TRACE(cpfport, "execute(), IMPORT force[%s]", (m_force? "YES":"NO"));
		if(!m_force)
		{
			std::string absolutePath;
			if( getAbsolutePath(realpath_, absolutePath))
			{
				threadStart = checkFileSize(absolutePath);
				if(threadStart)
					dotPrinterThread = boost::thread(&FMS_CPF_port::printDots, this);
			}
			else
			{
				throw FMS_CPF_Exception(FMS_CPF_Exception::INVALIDPATH);
			}
		}
		try
		{
			file.fileImport(realpath_, mode_);
		}
		catch(const FMS_CPF_Exception& ex)
		{
			// check if thread for dot printing is up
			if(threadStart)
			{
				portingFinished=true;
				dotPrinterThread.join();
				cout << endl;
			}
			// Re-throw same exception
			throw;
		}
    }

	if(threadStart)
	{
		portingFinished=true;
		dotPrinterThread.join();
		cout << endMsg << flush;
	}
   
	TRACE(cpfport, "%s","FMS_CPF_port::execute() EXITING...");

	return 0;
}


/*------------------------------------------------------------------------------
CheckFileSize(): TR HK89831 & HK89834
Description:
This method checks the file/folder size. If the file/folder size is more than 2GB then confirms with 
the operator if porting is required. Incase porting is required, a thread is created 
which monitors cpfport process for completion of porting.
------------------------------------------------------------------------------*/
bool FMS_CPF_port::checkFileSize(const std::string& path) throw (FMS_CPF_Exception)
{
	TRACE(cpfport, "%s", "Entering checkFileSize()");
	bool result = false;
	fs::path filePath(path);
	boost::intmax_t sizeBytes = 0;

	if(fs::exists(filePath))
	{
		if(fs::is_regular_file(filePath))
		{
			sizeBytes= fs::file_size(filePath);
		}
		else if(fs::is_directory(filePath))
		{
			TRACE(cpfport, "%s", "checkFileSize(), calculate folder size");
			for(fs::recursive_directory_iterator it(filePath);
			        it != fs::recursive_directory_iterator();
			        ++it)
			{
				if(!fs::is_directory(*it))
			       	sizeBytes += fs::file_size(*it);
			}
		}
	    else
	    {
	    	TRACE(cpfport, "checkFileSize(), path:<%s> error", path.c_str());
	    	FMS_CPF_Exception exErr(FMS_CPF_Exception::PHYSICALERROR);
	    	std::string errMsg(exErr.errorText());
	    	errMsg += std::string(path_);
	    	throw FMS_CPF_Exception (FMS_CPF_Exception::PHYSICALERROR, errMsg);
	    }
	}
	else
	{
		std::string zipFile = path + ".zip";
		fs::path filePath1(zipFile);
		if(fs::exists(filePath1))
		{
			sizeBytes= fs::file_size(filePath1);
		}
		else
		{
			zipFile = path + ".ZIP";
			fs::path filePath2(zipFile);
			if(fs::exists(filePath2))
			{
				sizeBytes= fs::file_size(filePath2);
			}
			else
			{
				TRACE(cpfport, "checkFileSize(), path:<%s> does not exist", path.c_str());
				FMS_CPF_Exception exErr(FMS_CPF_Exception::PHYSICALERROR);
				std::string errMsg(exErr.errorText());
				errMsg += std::string(path_);
				throw FMS_CPF_Exception (FMS_CPF_Exception::PHYSICALERROR, errMsg);
			}
		}
	}

	TRACE(cpfport, "checkFileSize(), path:<%s>, size:<%lld> bytes", path.c_str(), sizeBytes );
	
	
	//if size is more than 2GB  ask confirmation from the user.
    if(sizeBytes > MaxByteSizeLimit)
    {
    	TRACE(cpfport, "%s", "checkFileSize(), show dialog question");
		cout<< warningMsg << flush;
		result = confirm();
        if(!result )
        {
			throw FMS_CPF_Exception (FMS_CPF_Exception::ABORT);  //When user response is No,raise an ABORT exception
		} 
	}
    TRACE(cpfport, "%s", "Leaving checkFileSize()");
    return result;
}

bool FMS_CPF_port::confirm()
{
	TRACE(cpfport, "%s", "Entering in confirm()");
	bool result = false;
	cout << question1Msg << flush;
    while(true)
    {
    	std::string answer;
		//HO36174 : adding ETX character in the prompt and default value is set as yes

		cout << question2Msg << prompt_;
		//HO36174 :begin
		getline(cin,answer);

		ACS_APGCC::toUpper(answer);

		int answerLength = answer.length();

		if(answer.length() > 3) //HO36174
		{
			cout << errorInputMsg << flush;
            continue;
		}
		else if( (0 == answerLength) || (answer.compare("YES") == 0) || ( (1 == answerLength) && (answer[0] == 'Y') ) )//HO36174
		{
			result= true;
			break;
		}
		else if( (answer.compare("NO") == 0) || ( (1 == answerLength) && (answer[0] == 'N') ) )
		{
			result= false;
			break;
		}
		else
		{
			cout << errorInputMsg << flush;
			continue;
		}
    }
    TRACE(cpfport, "confirm(), user answer:<%s>", (result ? "YES":"NO"));
    return result;
}

void FMS_CPF_port::printDots()
{
	TRACE(cpfport, "%s", "printDots() started");
	cout << waitMsg<< flush;
	unsigned int countSec = 0;
	cout << "." << flush;
	while(!portingFinished)
	{
		if(dotsDelaySec == countSec)
		{
			cout << "." << flush;
			countSec = 0;
		}
		sleep(oneSec);
		++countSec;
	}
}

bool FMS_CPF_port::getAbsolutePath(const std::string& relativePath, std::string& absolutePath)
{
	bool result = false;
	std::string nbiPath;
	char pathBuffer[PATH_MAX] = {0};
	// Get the import/export path
	if(getFileMCpFilePath(nbiPath))
	{

		// Check for initial slash into relative path
		size_t slashPos = relativePath.find_first_of(DirDelim);

		if(slashPos != 0)
		{
			// Add the slash
			nbiPath += DirDelim;
		}

		absolutePath = nbiPath + relativePath;

		realpath(absolutePath.c_str(), pathBuffer);

		std::string realPath(pathBuffer);

		if( std::string::npos == realPath.find(nbiPath) )
		{
			TRACE(cpfport, "getAbsolutePath(), the path:<%s> is out the NBI folder:<%s>", absolutePath.c_str(), nbiPath.c_str());
		}
		else
			result = true;
	}
	return result;
}

bool FMS_CPF_port::getFileMCpFilePath(std::string& fileMPath)
{
	bool result = false;
	ACS_APGCC_DNFPath_ReturnTypeT getResult;
	ACS_APGCC_CommonLib fileMPathHandler;
	int bufferLength = 1024;
	char buffer[bufferLength];

	ACE_OS::memset(buffer, 0, bufferLength);
	// get the physical path
	getResult = fileMPathHandler.GetFileMPath(fileMCpFiles, buffer, bufferLength);

	if(ACS_APGCC_DNFPATH_SUCCESS == getResult)
	{
		// path get successful
		fileMPath = buffer;
		result = true;
	}
	else if(ACS_APGCC_STRING_BUFFER_SMALL == getResult)
	{
		TRACE(cpfport, "%s", "getFileMCpFilePath(), error on first get");
		// Buffer too small, but now we have the right size
		char buffer2[bufferLength+1];
		ACE_OS::memset(buffer2, 0, bufferLength+1);
		// try again to get
		getResult = fileMPathHandler.GetFileMPath(fileMCpFiles, buffer2, bufferLength);

		// Check if it now is ok
		if(ACS_APGCC_DNFPATH_SUCCESS == getResult)
		{
			// path get successful now
			fileMPath = buffer2;
			result = true;
		}
	}
	return result;
}

//------------------------------------------------------------------------------
//      Print command usage
//------------------------------------------------------------------------------
std::string FMS_CPF_port::usage ()
{
	std::string str("Incorrect usage\n");

	char *cpSwitch = const_cast<char *>(_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");

	str += std::string("Usage:\n");
	str += std::string("cpfport") + cpSwitch + std::string(" -e [-m mode] [-z] [-f] file[-subfile[-generation]] path\n");
	str += std::string("cpfport") + cpSwitch + std::string(" -i [-m mode] [-f] path file[-subfile[-generation]]\n");
  
  return str;
}

void FMS_CPF_port::usage (int eCode)
{
	std::string str("Incorrect usage\n");
	eCode = 0;
	char *cpSwitch = const_cast<char *>(_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");

	str = str + "Usage: \n";
	str = str + "cpfport" + cpSwitch + " -e [-m mode] [-z] [-f] file[-subfile[-generation]] path\n";
	str = str + "cpfport" + cpSwitch + " -i [-m mode] [-f] path file[-subfile[-generation]]\n";
	
}


//------------------------------------------------------------------------------
//      Main
//------------------------------------------------------------------------------
int ACE_TMAIN (int argc, ACE_TCHAR * argv[])
{
	try
	{
		FMS_CPF_port port(argc, argv);
		return port.launch();
	}
	catch (FMS_CPF_Exception& ex) 
	{
		std::string slogan;
		ex.getSlogan(slogan);
		cout << slogan << endl;
		return (int)(ex.errorCode());
	}
}
