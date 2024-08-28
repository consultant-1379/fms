#include "fms_cpf_privateexception.h"

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------

FMS_CPF_PrivateException::FMS_CPF_PrivateException () : FMS_CPF_Exception (INTERNALERROR)
{
}

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------
FMS_CPF_PrivateException::FMS_CPF_PrivateException (errorType error, const char* detail) :
FMS_CPF_Exception (error, detail)
{
}
//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------

FMS_CPF_PrivateException::FMS_CPF_PrivateException (errorType error, std::string detail) :
FMS_CPF_Exception (error, detail.c_str())
{
}

// Copy Constructor
FMS_CPF_PrivateException::FMS_CPF_PrivateException(const FMS_CPF_PrivateException& object) :
  FMS_CPF_Exception(object)
{

}

FMS_CPF_PrivateException& FMS_CPF_PrivateException::operator= (const FMS_CPF_PrivateException& object)
{
	if(this == &object) return *this;

	FMS_CPF_Exception::operator =(object);

	return *this;
}

//------------------------------------------------------------------------------
//      Store exception to the output stream
//------------------------------------------------------------------------------

std::ostream& operator<< (std::ostream& ostr, const FMS_CPF_PrivateException& ex)
{
  ostr << (unsigned long)ex.error_;
  ostr << ex.detail_;
  return ostr;
}


//------------------------------------------------------------------------------
//      Get exception from the output stream
//------------------------------------------------------------------------------

std::istream& operator>> (std::istream& istr, FMS_CPF_PrivateException& ex)
{
  unsigned long error;

  istr >> error;
  ex.error_ = (FMS_CPF_Exception::errorType)error;
  istr >> ex.detail_;
  return istr;
}


