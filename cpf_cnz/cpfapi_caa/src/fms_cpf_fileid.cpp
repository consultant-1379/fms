/*
 * fms_cpf_fileid.c
 *
 *  Created on: Dec 3, 2010
 *      Author: esalves
 */

#include "fms_cpf_fileid.h"

#include <ace/ACE.h>

// SIO_I8 qabhall rwtool cleaned out

#include "fms_cpf_api_trace.h"
#include "ACS_TRA_trace.h"

FMS_CPF_FileId FMS_CPF_FileId::NIL;

using namespace ACE_OS;

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_FileId::FMS_CPF_FileId () :
str_ (),
fnamesize_ (0),
snamesize_ (0),
gensize_ (0),
isValid_ (false)
{

}


//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_FileId::FMS_CPF_FileId (const char* str)
{
  *this = std::string (str);
}


//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_FileId::FMS_CPF_FileId (const std::string &str)
{
  *this = str;
}


//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------
FMS_CPF_FileId::~FMS_CPF_FileId ()
{

}

//------------------------------------------------------------------------------
//      Assignment operator
//
//
//      Assigns the following parts in the object:
//      - sizes of substring(s) in str (fnamesize_, snamesize_ and gensize_)
//      - bool isValid is set if str holds valid chars and each substring is
//        of valid size
//      - string str_ is set to str if the above is fulfilled
//
//      IN: str     containg filename and optional subfilename, and if subfilename
//                  is contained, a optional generation, all separated with "-".
//------------------------------------------------------------------------------
FMS_CPF_FileId& FMS_CPF_FileId::operator= (const std::string& str)
{
  str_.clear();
  fnamesize_ = 0;
  snamesize_ = 0;
  gensize_   = 0;
  isValid_   = false;

  size_t size  = str.length();
  if(size > 0)
  {
	  std::string sep("-");
	  std::string part;

	  // Fetch file part
	  size_t start = 0;
	  size_t pend  = str.find_first_of(sep, start);
	  pend         = (pend > size) ? size : pend;
	  part         = str.substr(start, pend);

	  if(((part[0] >= 'A') && (part[0] <= 'Z')) || ((part[0] >= 'a') && (part[0] <= 'z')))  // first char in filename must be within
	  {
		fnamesize_ = part.find_first_not_of(A_CHARACTERS, 0);
		fnamesize_ = (fnamesize_ > part.length()) ? part.length() : fnamesize_;

		if(fnamesize_ == part.length())         // no invalid chars in filename
		{
		  if (fnamesize_ == size)
		  {
			str_ = str;
			isValid_ = true;
		  }
		  else if(size > pend)                       // more to unwind
		  {
			// Fetch subfile part
			size       -= fnamesize_ + 1;
			start      = pend + 1;
			pend       = str.find_first_of(sep, start);
			pend       = (pend > str.length()) ? str.length() : pend;
			part       = str.substr(start, pend - start);
			snamesize_ = part.find_first_not_of(A_CHARACTERS, 0);
			snamesize_ = (snamesize_ > part.length()) ? part.length() : snamesize_;
			if(snamesize_ == part.length())         // no invalid chars in subfilename
			{
			  if (( snamesize_ <= 12U) && ( snamesize_ > 0U))
			  {
				if (snamesize_ == size)
				{
				  str_ = str;
				  isValid_ = true;
				}
				else if(str.length() > pend)                // more to unwind
				{
				  // Fetch generation part
				  size     -= snamesize_ + 1;
				  start    = pend + 1;
				  pend     = str.find_first_of(sep, start);
				  pend     = (pend > str.length()) ? str.length() : pend;
				  part     = str.substr(start, pend - start);
				  gensize_ = part.find_first_not_of(A_CHARACTERS, 0);
				  gensize_ = (gensize_ > part.length()) ? part.length() : gensize_;
				  if(gensize_ == part.length())         // no invalid chars in subfilename
				  {
					if((gensize_ <= 8U) && (gensize_ > 0U))
					{
					  if (gensize_ == size)
					  {
						str_ = str;
						isValid_ = true;
					  }
					}
				  }
				}
			  }
			}
		  }
		}
	  }
  }
  return *this;
}


//------------------------------------------------------------------------------
//      Return file identity as character array
//------------------------------------------------------------------------------
const char* FMS_CPF_FileId::data () const
{
  return str_.c_str();
}


//------------------------------------------------------------------------------
//      Read file name
//------------------------------------------------------------------------------
std::string FMS_CPF_FileId::file () const
{
  return str_.substr(0, fnamesize_);
}


//------------------------------------------------------------------------------
//      Read subfile name
//------------------------------------------------------------------------------
std::string FMS_CPF_FileId::subfile () const
{
  if (str_.length() < fnamesize_ + 1 + snamesize_)
		{
			return std::string();
		}

  return str_.substr(fnamesize_ + 1, snamesize_);
}


//------------------------------------------------------------------------------
//      Read generation name
//------------------------------------------------------------------------------
std::string FMS_CPF_FileId::generation () const
{
  if (str_.length() < fnamesize_ + snamesize_ + 2 + gensize_)
	   return std::string();
  return str_.substr(fnamesize_ + snamesize_ + 2, gensize_);
}


//------------------------------------------------------------------------------
//      Read subfile and generation name
//------------------------------------------------------------------------------
std::string FMS_CPF_FileId::subfileAndGeneration () const
{
  if (str_.length() < fnamesize_ + 1 + snamesize_)
	   return std::string();
  return str_.substr(fnamesize_ + 1, snamesize_ + gensize_ + (gensize_ ? 1: 0));
}


//------------------------------------------------------------------------------
//      Returns TRUE if file identity is valid
//------------------------------------------------------------------------------
bool FMS_CPF_FileId::isValid () const
{
  return isValid_;
}


//------------------------------------------------------------------------------
//      Returns TRUE if file identity is empty
//------------------------------------------------------------------------------
bool FMS_CPF_FileId::isNull () const
{
  return str_.empty();
}


//------------------------------------------------------------------------------
//      Equality operator
//------------------------------------------------------------------------------
//SIO_I4 qablake 990201 rep int with bool
bool FMS_CPF_FileId::operator== (const FMS_CPF_FileId& fileid) const
{
  return str_ == fileid.str_;
}


//------------------------------------------------------------------------------
//      Unequality operator
//------------------------------------------------------------------------------
//SIO_I4 qablake 990201 rep int with bool
bool FMS_CPF_FileId::operator!= (const FMS_CPF_FileId& fileid) const
{
  return str_ != fileid.str_;
}


//------------------------------------------------------------------------------
//      Less-than operator
//------------------------------------------------------------------------------
//SIO_I4 qablake 990201 rep int with bool
bool FMS_CPF_FileId::operator< (const FMS_CPF_FileId& fileid) const
{
  return str_ < fileid.str_;
}


//------------------------------------------------------------------------------
//      Write file identity to ostream
//------------------------------------------------------------------------------
std::ostream& operator<< (std::ostream& ostr, const FMS_CPF_FileId& fileid)
{
  ostr << fileid.str_;
  return ostr;
}


//------------------------------------------------------------------------------
//      Read file identity from istream
//------------------------------------------------------------------------------
std::istream& operator>> (std::istream& istr, FMS_CPF_FileId& fileid)
{
	std::string str;

  istr >> str;

  register size_t N = str.length();
  while ( N-- ) { str[N] = toupper((unsigned char)str[N]); ;}
  fileid = str;
  return istr;
}


