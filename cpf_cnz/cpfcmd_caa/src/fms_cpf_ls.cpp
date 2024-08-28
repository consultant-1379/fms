
#include "fms_cpf_ls.h"
#include "fms_cpf_trace.h"

#include "ACS_APGCC_Util.H"
#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"

#include <ace/ACE.h>
#include "ace/ARGV.h"
#include "ace/Get_Opt.h"

#include <iomanip>

using namespace std;

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_ls::FMS_CPF_ls(int argc, char* argv [])
: FMS_CPF_command (argc, argv),
  isSubOnly(false),
  all_(true),
  long_(false),
  quiet_(false),
  subfiles_(false),
  path_(false),
  compressed_(false)
{
	cpfls = new ACS_TRA_trace("cpfls");
	TRACE(cpfls, "%s", "FMS_CPF_ls::Constructor()");
}


//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FMS_CPF_ls::~FMS_CPF_ls ()
{
    if (NULL != cpfls)
    {
    	delete cpfls;
    }
}

//------------------------------------------------------------------------------
//      Command parser
//------------------------------------------------------------------------------
void FMS_CPF_ls::parse()
{
  //Call the base common parsing
	FMS_CPF_command::parse();

	int c;
	option_t option;

	optpair_t optlist [] = {{0,0},{0,0},{0,0},{0,0},{0,0}};
	ACE_Get_Opt getopt (argc_, argv_, "klsqp");

	// Parse the options
	while ((c = getopt ()) != EOF)
	{
		switch(c)
		{
			case 'l': option = LONG; break;
			case 's': option = SUBFILES; break;
			case 'q': option = QUIET; break;
			case '?':
			default:
				throw FMS_CPF_Exception (FMS_CPF_Exception::INCORRECT_USAGE, usage() );
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

	// Parse file name
	if (argc_ > getopt.optind)
	{
		std::string filename = (argv_ [getopt.optind]);
		ACS_APGCC::toUpper(filename);
		fileInput = filename;
		
		fileid_ = filename;
		if (!fileid_.isValid ())
		{
			throw FMS_CPF_Exception(FMS_CPF_Exception::INVALIDFILE);
		}

		all_ = false;

		getopt.optind++;
	}
	

	// Set options
	compressed_= optlist[COMPRESSED].opt ? true: false;
	long_ = optlist[LONG].opt ? true: false;
	quiet_ = optlist[QUIET].opt ? true: false; 
	path_ = optlist[PATH].opt ? true: false;
	
	if(fileid_.subfile().empty())
	{
		// Main file, Subfiles option
		subfiles_ = optlist[SUBFILES].opt ? true: false;
	}
	else
	{
		// Subfile
		if (optlist[SUBFILES].opt)
		{
			throw FMS_CPF_Exception(FMS_CPF_Exception::INCORRECT_USAGE, usage());
		}
		subfiles_ = false;
	}
	
	if (long_ == false && compressed_ == true)
	{
		throw FMS_CPF_Exception(FMS_CPF_Exception::INCORRECT_USAGE, usage());
	}
	
	if (path_ && (long_ || quiet_ || subfiles_))
	{
		//If path_ is issued none of the other options is allowed
		throw FMS_CPF_Exception(FMS_CPF_Exception::INCORRECT_USAGE, usage());
	}
	
	if(path_ && all_)
	{
		// A single file must be given when the path is wanted
		throw FMS_CPF_Exception(FMS_CPF_Exception::INCORRECT_USAGE, usage());
	}
	
	if (argc_ != getopt.optind)
	{
		throw FMS_CPF_Exception(FMS_CPF_Exception::INCORRECT_USAGE, usage());
	}
}

//------------------------------------------------------------------------------
//      List users
//------------------------------------------------------------------------------
void FMS_CPF_ls::listUsers(FMS_CPF_Types::userType users)
{
	if(users.ucount == 0){
		if(users.rcount>=users.wcount){
			users.ucount = users.rcount;
		}
		else{
			users.ucount = users.wcount;
		}
		//users.ucount = users.rcount + users.wcount;
		TRACE(cpfls, "FMS_CPF_ls ucount=%ld",users.ucount);
		cout << setw (2) << (unsigned int)users.ucount;
		cout << " [" << setw (2);
		cout << (unsigned int)users.rcount;
		cout << "R " << setw (2);
		cout << (unsigned int)users.wcount;
	}
	else{
		TRACE(cpfls, "FMS_CPF_ls (else) ucount=%ld",users.ucount);
		cout << setw (2) << (unsigned int)users.ucount;

		cout << " [" << setw (2);
		if(users.rcount==1){
			cout << "X";
		}
		else {
			TRACE(cpfls, "FMS_CPF_ls else::rcount=%ld",users.rcount);
			cout << (unsigned int)users.rcount;
		}
		cout << "R " << setw (2);
		if (users.wcount == 1) {
			TRACE(cpfls, "FMS_CPF_ls if::wcount=%ld",users.wcount);
			cout << "X";
		}
		else {
			TRACE(cpfls, "FMS_CPF_ls else::wcount=%ld",users.wcount);
			cout << (unsigned int)users.wcount;
		}


	}
	cout.setf (ios::left, ios::adjustfield);
	cout << setw (5) << "W]";
}

//------------------------------------------------------------------------------
//      List path for a file
//------------------------------------------------------------------------------
void FMS_CPF_ls::listPath(FMS_CPF_FileIterator::FMS_CPF_FileData &fd)
{

	cout << endl;
    cout << "FILE                                PATH" << endl;
	cout.setf (ios::left, ios::adjustfield);
    cout << setw (34) << fd.fileName<<"  ";
	cout.setf (ios::left, ios::adjustfield);
	cout<< fd.pathList<<endl;
}


//------------------------------------------------------------------------------
//      List file entry
//------------------------------------------------------------------------------

void FMS_CPF_ls::listFileEntry(FMS_CPF_FileIterator::FMS_CPF_FileData &fd, FMS_CPF_FileIterator &iter) {
	
    TRACE(cpfls, "FMS_CPF_ls::listFileEntry() file %s, compressed %s", fd.fileName.c_str(), fd.compressed?"true":"false" );

	bool composite(false);
	
	if (quiet_ == false)
	{
		cout << endl;
		cout << "FILE                                TYPE  CMP  VOLUME" << endl;
	}

	if ((long_ == false) && (subfiles_ == false)) {
		quiet_ = true;
	}

	cout.setf (ios::left, ios::adjustfield);
	cout << setw (34) << fd.fileName << "  ";
	cout.setf (ios::left, ios::adjustfield);
	switch (fd.ftype) {
	  case FMS_CPF_Types::ft_REGULAR:
			composite = fd.composite;
			cout << setw (4) << S_REGULAR << "  ";
			cout << setw (3) << ((fd.composite == true) ? S_YES: S_NO);
			break;
	  case FMS_CPF_Types::ft_INFINITE:
			cout << setw (4) << S_INFINITE << "  ";
			composite = true;
			cout << setw (3) << ((fd.composite == true) ? S_YES: S_NO);
			break;
	  default:
		  break; //throw FMS_CPF_Exception(FMS_CPF_Exception::INTERNALERROR);
	}
	cout << "  " << fd.volume  << endl;
	if (long_ == true) {
		cout << endl;
		if (quiet_ == false) {
			cout << "TRANSFER QUEUE                      MODE" << endl;
			cout << setw(36) << fd.tqname;
		}
		else {
			cout << endl << endl;
		}

		switch (fd.mode) {
		case FMS_CPF_Types::tm_FILE:
			cout << setw(7) << s_file << endl;
			break;
		case FMS_CPF_Types::tm_BLOCK:
			cout << setw(7) << s_block << endl;
			break;
		case FMS_CPF_Types::tm_NONE:
			cout << setw(7) << s_none << endl;
			break;
		default: cout << endl;
			break;
		}
		
		if (quiet_ == false) {
			cout << endl;
			cout << "RLENGTH  MAXSIZE  MAXTIME  REL      ACTIVE  ";
			if (compressed_ == true) {
				cout << "      SIZE  USERS          COMPRESSED" << endl;
			}
			else {
				cout << "      SIZE  USERS" << endl;
			}
		}
		else {
			cout << endl;
		}
		
		cout.setf (ios::right, ios::adjustfield);
		switch (fd.ftype) {
		case FMS_CPF_Types::ft_REGULAR:
			cout << setw (7) << fd.recordLength;
			cout << setw (37) << "";
			break;
		case FMS_CPF_Types::ft_INFINITE:
			cout << setw (7) << fd.recordLength << "  ";
			TRACE(cpfls, "maxsize = %ld", fd.maxsize);
			if (fd.maxsize == 0) {
				cout.setf (ios::left, ios::adjustfield);
				cout << setw (7) << S_NO << "  ";
			}
			else {
				cout.setf (ios::right, ios::adjustfield);
				cout << setw (7) << fd.maxsize << "  ";
			}
			cout << setw (7);
			TRACE(cpfls, "maxtime = %d", fd.maxtime);
			if (fd.maxtime == 0) {
				cout.setf (ios::left, ios::adjustfield);
				cout << setw (7) << S_NO << "  ";
				TRACE(cpfls, "maxtime if = %ld", fd.maxtime);
			}
			else {
				cout.setf (ios::right, ios::adjustfield);
				cout << setw (7) << fd.maxtime << "  ";
				TRACE(cpfls, "maxtime else= %ld", fd.maxtime);
			}
			cout.setf (ios::left, ios::adjustfield);
			cout << setw (3);
			cout << (fd.release ? S_YES: S_NO) << "  ";
			cout.setf (ios::right, ios::adjustfield);
			cout << setw (10);
			TRACE(cpfls, "active = %d", fd.active);

			if (fd.active == 0) {
				TRACE(cpfls, "active if = %d", fd.active);

				cout << "";
			}
			else {
				TRACE(cpfls, "active else = %d", fd.active);

				cout << fd.active ;
			}
			cout << "  ";
			break;
		default:
			cout << "Internal error" << endl;
			throw FMS_CPF_Exception (FMS_CPF_Exception::INTERNALERROR);
		}
		cout << setw (10) << fd.file_size << "  ";
		FMS_CPF_Types::userType users;
		users.ucount = fd.ucount;
		users.rcount = fd.rcount;
		users.wcount = fd.wcount;
		listUsers (users);
		
		// List compression status
		if (compressed_ == true) {
			cout.setf (ios::left, ios::adjustfield);
			cout << setw (3) << (fd.compressed? S_YES:S_NO);
		}
		
		cout << endl;
	}



	cout.setf (ios::left, ios::adjustfield);
	
	if ((subfiles_ == true) && (composite == true)) {
		// List subfiles
		if (quiet_ == false) {
			cout << endl << "SUBFILES";
			if (long_ == true) {
				if (compressed_ == true) {
					cout << setw (28) << "" << "      SIZE  USERS          COMPRESSED";
				}
				else {
					cout << setw (28) << "" << "      SIZE  USERS";
				}
			}
			cout << endl;
		}
		// TO BE DONE
		
		//FMS_CPF_FileIterator iterator(fd.fileName.c_str(),long_,true, _multipleCPSystem ? _cpName : "");
		FMS_CPF_FileIterator::FMS_CPF_FileData subfd;
		iter.setSubFiles(true);
		
		while (iter.getNext(subfd)) {
			cout.setf (ios::left, ios::adjustfield);
			cout << setw (34) << subfd.fileName;
			if (long_ == true) {
				cout << "  ";
				cout.setf (ios::right, ios::adjustfield);
				cout << setw (10) << subfd.file_size << "  ";
				
				FMS_CPF_Types::userType s_users;
				s_users.ucount = subfd.ucount;
				s_users.rcount = subfd.rcount;
				s_users.wcount = subfd.wcount;
				listUsers (s_users);
				
				// List compression status
				if (compressed_ == true) {
					TRACE(cpfls, "FMS_CPF_ls::listFileEntry() subfile %s, compressed %s", subfd.fileName.c_str(), subfd.compressed?"true":"false" );
					cout.setf (ios::left, ios::adjustfield);
					cout << setw (3) << (subfd.compressed? S_YES:S_NO);
				}
			}
			cout << endl;
		}
		iter.setSubFiles(false);
	}  
	
	if (quiet_ == false) {
		cout << endl;
	}
}

//------------------------------------------------------------------------------
//      List subfile
//------------------------------------------------------------------------------
void
FMS_CPF_ls::listSubfile(FMS_CPF_FileIterator::FMS_CPF_FileData &fd)throw(FMS_CPF_Exception){
	FMS_CPF_FileIterator iterator(fileInput.c_str(),long_,true, _multipleCPSystem ? _cpName : "");
	FMS_CPF_FileIterator::FMS_CPF_FileData subfd;
	string subFileName = "";
	bool composite(false);
	bool notFound(true);

	if ((long_ == false) && isSubOnly) {
			quiet_ = true;
		}
	TRACE(cpfls, "SUBFILE NAME = %s", fileInput.c_str());

	while (iterator.getNext(subfd)) {

		TRACE(cpfls, "SUBFILE NAME INPUT = %s, SUBFILE NAME ITERATOR = %s", fileInput.c_str(), subfd.fileName.c_str());
		ACS_APGCC::toUpper(subfd.fileName);
		TRACE(cpfls, "SUBFILE2 NAME INPUT = %s, SUBFILE2 NAME ITERATOR = %s", fileInput.c_str(), subfd.fileName.c_str());
		if(fileInput.compare(subfd.fileName)==0){ //  current subfile and input subfile are equals
			TRACE(cpfls, "SUBFILE: %s FOUND ", subfd.fileName.c_str());
			notFound=false;
			cout << "CPF FILE TABLE" << endl;
			cout << endl;
			cout << "FILE                                TYPE  CMP  VOLUME" << endl;


			cout.setf (ios::left, ios::adjustfield);
			cout << setw (34) << subfd.fileName << "  "; //print subfile name
			cout.setf (ios::left, ios::adjustfield);

			switch (subfd.ftype) {  //print subfile type

				case FMS_CPF_Types::ft_REGULAR:
					composite = subfd.composite;
					cout << setw (4) << S_REGULAR << "  ";
					cout << setw (3) << ((subfd.composite == true) ? S_YES: S_NO);
					break;
				case FMS_CPF_Types::ft_INFINITE:
					cout << setw (4) << S_INFINITE << "  ";
					composite = true;
					cout << setw (3) << ((subfd.composite == true) ? S_YES: S_NO);
					break;
				default:
					break; //throw FMS_CPF_Exception(FMS_CPF_Exception::INTERNALERROR);
			}

			cout << "  " << fd.volume  << endl;

			if (long_ == true) { //parameter -l is present
			cout << "TRANSFER QUEUE                      MODE" << endl;
			cout << setw(36) << subfd.tqname; // print transfer queue name

			switch (subfd.mode) { // print transfer mode

					case FMS_CPF_Types::tm_FILE:
						cout << setw(7) << s_file << endl;
						break;
					case FMS_CPF_Types::tm_BLOCK:
						cout << setw(7) << s_block << endl;
						break;
					case FMS_CPF_Types::tm_NONE:
						cout << setw(7) << s_none << endl;
						break;
					default: cout << endl;
						break;
			}

			cout << endl;
			cout << "RLENGTH  MAXSIZE  MAXTIME  REL      ACTIVE  ";

			if (compressed_ == true) {
				cout << "      SIZE  USERS          COMPRESSED" << endl;
			}
			else {
				cout << "      SIZE  USERS" << endl;
			}


			cout.setf (ios::right, ios::adjustfield);

			switch (subfd.ftype) {

				case FMS_CPF_Types::ft_REGULAR:
					cout << setw (7) << subfd.recordLength;
					cout << setw (37) << "";
					break;
				case FMS_CPF_Types::ft_INFINITE:
					cout << setw (7) << subfd.recordLength << "  ";
					if (subfd.maxsize == 0) {
							cout.setf (ios::left, ios::adjustfield);
							cout << setw (7) << S_NO << "  ";
					}
					else {
						cout.setf (ios::right, ios::adjustfield);
						cout << setw (7) << subfd.maxsize << "  ";
					}
					cout << setw (7);
					if (subfd.maxtime == 0) {
							cout.setf (ios::left, ios::adjustfield);
							cout << setw (7) << S_NO << "  ";
					}
					else {
						cout.setf (ios::right, ios::adjustfield);
						cout << setw (7) << (int)subfd.maxtime << "  ";
						}
					cout.setf (ios::left, ios::adjustfield);
					cout << setw (3);
					cout << (subfd.release ? S_YES: S_NO) << "  ";
					cout.setf (ios::right, ios::adjustfield);
					cout << setw (10);
					if (subfd.active == 0) {
						cout << "";
					}
					else {
						cout << subfd.active ;
					}
					cout << "  ";
					break;
				default:
					cout << "Internal error" << endl;
					throw FMS_CPF_Exception (FMS_CPF_Exception::INTERNALERROR);
			}

				cout << setw (10) << subfd.file_size << "  ";
				FMS_CPF_Types::userType s_users;
				s_users.ucount = subfd.ucount;
				s_users.rcount = subfd.rcount;
				s_users.wcount = subfd.wcount;
				listUsers (s_users);


			}

			cout << endl;
			TRACE(cpfls, "BREAK WHILE=%s ",subfd.fileName.c_str());
			break;
		}
		TRACE(cpfls, "SUBFILE: %s NOT FOUND ", subfd.fileName.c_str());
		TRACE(cpfls, "CONTINUE WHILE=%s ",subfd.fileName.c_str());

	}
	if(notFound)
	{
		FMS_CPF_Exception exErr(FMS_CPF_Exception::FILENOTFOUND);
		std::string errMsg(exErr.errorText());
		errMsg += fileInput;
		throw FMS_CPF_Exception (FMS_CPF_Exception::FILENOTFOUND, errMsg);
	}
}

//------------------------------------------------------------------------------
//      Execute command
//------------------------------------------------------------------------------
int FMS_CPF_ls::execute()
{

	bool firstTime(true);	
	
	if (all_ == true) {
		// List all files
		try{
			FMS_CPF_FileIterator iterator("", long_, subfiles_, _multipleCPSystem ? _cpName : "");

		FMS_CPF_FileIterator::FMS_CPF_FileData fd;
		iterator.setSubFiles(false);
		try {
			while(iterator.getNext(fd))
			{

				if ((quiet_ == false) && (firstTime))
				{
					cout << "CPF FILE TABLE" << endl;
					firstTime =false;
				}
				try
				{

					listFileEntry(fd, iterator);
				}
				catch (FMS_CPF_Exception& ex)
				{
					ex.getSlogan(slogan);
					cout << slogan << endl;
					return (int)(ex.errorCode());
				}

			}
		}
		catch (FMS_CPF_Exception& ex) {
			ex.getSlogan(slogan);
			cout << slogan << endl;
			return (int)(ex.errorCode());
		}
		}
		catch (FMS_CPF_Exception& ex){
			if(ex.errorCode()==FMS_CPF_Exception::IMM_NOVOLUME_STRUCT) //TR_HV63599
			{
				cout << "CPF FILE TABLE" << endl;
				cout << " " <<endl;
			}
			else if ( ex.errorCode()==FMS_CPF_Exception::FILENOTFOUND ) //TR_HV63599
			{
				cout << "CPF FILE TABLE" << endl;
				cout << " " <<endl;
				return FMS_CPF_Exception::OK;

			}
			else{
				ex.getSlogan(slogan);
				cout << slogan << endl;
			}
			return (int)(ex.errorCode());
			}
	}
	
	else {
		// List specific file
		// TO BE DONE

		string compFileName="";
		compFileName = fileid_.file(); // store only the name of composite file
		try{
		FMS_CPF_FileIterator iterator(compFileName.c_str(),long_, subfiles_, _multipleCPSystem ? _cpName : "");

		FMS_CPF_FileIterator::FMS_CPF_FileData fd;
		iterator.setSubFiles(false);
		try {
			while(iterator.getNext(fd))
			{

				if(path_)
				{
					try {
						listPath(fd);
					}
					catch (FMS_CPF_Exception& ex)
					{
						ex.getSlogan(slogan);
						cout << slogan << endl;
						return (int)(ex.errorCode());
					}
				}
				else {
					try {
						if (fileInput.find('-') != string::npos)          // It test if the input file is a subfile
						{
							isSubOnly = true;
							listSubfile(fd);
						}
						else{

							if ((quiet_ == false) && (firstTime)) {
								cout << "CPF FILE TABLE" << endl;
								firstTime =false;
							}

							listFileEntry(fd, iterator); //the input file isn't a subfile
							subfiles_ = true;
						}
					}
					catch (FMS_CPF_Exception& ex)
					{
						ex.getSlogan(slogan);
						cout << slogan << endl;
						return (int)(ex.errorCode());
					}
				}
			}
		}
		catch (FMS_CPF_Exception& ex)
		{
			ex.getSlogan(slogan);
			cout << slogan << endl;
			return (int)(ex.errorCode());
		}
		}
		catch (FMS_CPF_Exception& ex){
					if(ex.errorCode()==FMS_CPF_Exception::IMM_NOVOLUME_STRUCT){
						cout << "File was not found: "<<fileInput << endl;

					}
					else{
						ex.getSlogan(slogan);
						cout << slogan << endl;
					}
					return (int)(ex.errorCode());
					}

	}
	cout << endl;
	return 0;
}

//------------------------------------------------------------------------------
//	Command usage
//------------------------------------------------------------------------------
void
FMS_CPF_ls::usage(int eCode) {
	string str;
	char *cpSwitch = const_cast<char *> (_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");


	str = '\n';
	str = str + "Usage: \n";
	str = str + "cpfls" + cpSwitch + " [-l][-s][-q] [file]\n";
	str = str + "cpfls" + cpSwitch + " -l [-s][-q] [file]\n";
	str = str + "cpfls" + cpSwitch + " [-l][-q] file-subfile[-generation]\n";
	str = str + "cpfls" + cpSwitch + " -l [-q] file-subfile[-generation]\n";
	cout << str << endl;
	ACE_OS::exit(eCode);
}

string
FMS_CPF_ls::usage ()
{
	std::string str("Incorrect usage\n");
	char *cpSwitch = const_cast<char *> (_multipleCPSystem ? " -cp (<cpname> | CLUSTER)" : "");

	str = str + "Usage: \n";
	str = str + "cpfls" + cpSwitch + " [-l][-s][-q] [file]\n";
	str = str + "cpfls" + cpSwitch + " -l [-s][-q] [file]\n";
	str = str + "cpfls" + cpSwitch + " [-l][-q] file-subfile[-generation]\n";
	str = str + "cpfls" + cpSwitch + " -l [-q] file-subfile[-generation]\n";
	
	return str;
}


//------------------------------------------------------------------------------
//      Main
//------------------------------------------------------------------------------
int main (int argc, char* argv [])
{

	try
	{
		FMS_CPF_ls ls(argc, argv);
		return ls.launch ();
	}
	catch (FMS_CPF_Exception& ex)
	{
		std::string slogan;
		ex.getSlogan(slogan);
		cout << slogan << endl;
		return (int)(ex.errorCode());
	}
}
