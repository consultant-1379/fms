/**
 *  author: esalves
 */
#ifndef FMS_CPF_PRIVATEEXCEPTION_H
#define FMS_CPF_PRIVATEEXCEPTION_H

#include "fms_cpf_exception.h"
#include <iostream>


class FMS_CPF_PrivateException : public FMS_CPF_Exception
{

 public:

	FMS_CPF_PrivateException();

	virtual ~FMS_CPF_PrivateException(){};

	FMS_CPF_PrivateException(errorType error, const char* detail = "");

	FMS_CPF_PrivateException(errorType error, std::string detail);

    // Copy constructor
    FMS_CPF_PrivateException(const FMS_CPF_PrivateException&);

    FMS_CPF_PrivateException& operator= (const FMS_CPF_PrivateException&);

    friend std::ostream& operator<< (std::ostream& ostr, const FMS_CPF_PrivateException& ex);

    friend std::istream& operator>> (std::istream& istr, FMS_CPF_PrivateException& ex);

};

#endif

