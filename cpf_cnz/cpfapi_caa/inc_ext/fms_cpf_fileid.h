//**********************************************************************
//
// NAME
//      fms_cpf_fileid.h
//
// COPYRIGHT Ericsson Telecom AB, Sweden 1999.
// All rights reserved.
//
// The Copyright to the computer program(s) herein is the property of
// Ericsson Telecom AB, Sweden.
// The program(s) may be used and/or copied only with the written
// permission from Ericsson Telecom AB or in accordance with
// the terms and conditions stipulated in the agreement/contract under
// which the program(s) have been supplied.

// DESCRIPTION
//
//
// .

// DOCUMENT NO
//

// AUTHOR
//

// REVISION
//
//

// CHANGES
//
//	RELEASE REVISION HISTORY
//
//	REV NO		DATE		NAME 		DESCRIPTION
//            99              Ported from UNIX to NT  SIO_I4 - SIO_I7
//  	        990622  qabhall rwtool cleaned out      SIO_I8
//            990930  qabhall String chars corrected    SIO_I11B
//						esalves
//
// SEE ALSO
//
//
//**********************************************************************
#ifndef FMS_CPF_FILEID_H
#define FMS_CPF_FILEID_H

#include <string>
#include <sstream>

const short VOL_LENGTH = 15;
const short FIL_LENGTH = 12;
const short SUB_LENGTH = 12;
const short GEN_LENGTH = 8;

//LLV16 underscore delete from valid characters
const char A_CHARACTERS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

class ACS_TRA_trace;

class  FMS_CPF_FileId
{
 public:
	FMS_CPF_FileId();
	FMS_CPF_FileId(const char* str);

	FMS_CPF_FileId(const std::string& str);

	~FMS_CPF_FileId();

	FMS_CPF_FileId& operator= (const std::string& str);

	const char* data() const;
	std::string file() const;
	std::string subfile() const;
	std::string generation() const;
	std::string subfileAndGeneration() const;
	bool isValid() const;
	bool isNull() const;
	bool operator== (const FMS_CPF_FileId& fileid) const;
	bool operator!= (const FMS_CPF_FileId& fileid) const;
	bool operator< (const FMS_CPF_FileId& fileid) const;


	friend std::ostream& operator<< (std::ostream& ostr, const FMS_CPF_FileId& fileid);
	friend std::istream& operator>> (std::istream& istr, FMS_CPF_FileId& fileid);

    static FMS_CPF_FileId NIL;

 private:

	std::string str_;
	size_t fnamesize_;
	size_t snamesize_;
	size_t gensize_;
	bool isValid_;
};

#endif
