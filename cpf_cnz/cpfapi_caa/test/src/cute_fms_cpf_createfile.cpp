/*
 * cute_fms_cpf_createfile.cpp
 *
 *  Created on: Jan 26, 2011
 *      Author: esalves
 */

#include "cute_fms_cpf_createfile.h"

#include "cute.h"
#include "ide_listener.h"
#include "cute_runner.h"

#include <ace/ACE.h>

#include "fms_cpf_types.h"
#include "fms_cpf_file.h"

using namespace ACE_OS;

void CUTE_FMS_CPF_CreateFile::init_t1(){

	char filename[10];
	ACE_OS::memset(filename,0, sizeof(filename));
	ACE_OS::strcpy(filename,"afilename");

	char cpName[7];
	ACE_OS::memset(cpName,0, sizeof(cpName));
	ACE_OS::strcpy(cpName,"CP1");

	file = new FMS_CPF_File(filename, cpName);

}

void CUTE_FMS_CPF_CreateFile::init_t2(){
	file = new FMS_CPF_File();
}

void CUTE_FMS_CPF_CreateFile::copy_constr(){
	static FMS_CPF_File *tmp;
	tmp = new FMS_CPF_File();

	file = new FMS_CPF_File(*tmp);

	ASSERT_EQUALM("Copy constructor", tmp, file);
}

void CUTE_FMS_CPF_CreateFile::destroy(){
	//Destruct object file.
	delete file;
//	file = 0;
}

void CUTE_FMS_CPF_CreateFile::create_t1(){

	//set attributes
	FMS_CPF_Types::fileAttributes fileAttr;
	fileAttr.ftype = FMS_CPF_Types::ft_REGULAR; //regular file
	//Record length = 100
	//simple file
//	fileAttr.regular = {100, false};
	fileAttr.regular.rlength = 100;
	fileAttr.regular.composite = false;

	char volume[8];
	ACE_OS::memset(volume,0, sizeof(volume));
	ACE_OS::strcpy(volume,"relvol");

	FMS_CPF_Types::accessType access = FMS_CPF_Types::R_;

	bool compress = false;

	file->create(fileAttr, volume, access, compress);

	ASSERTM("Create file", file->exists());
}

void CUTE_FMS_CPF_CreateFile::create_t2(){

	FMS_CPF_Types::accessType access = FMS_CPF_Types::R_W_;
	bool compress = true;

	file->create(access, compress);

	ASSERTM("Create file", file->exists());
}

void CUTE_FMS_CPF_CreateFile::unreserve(){

	file->unreserve();

	ASSERTM("Reverse file", file->isReserved());
}

cute::suite CUTE_FMS_CPF_CreateFile::makeFMSCPFCreateFileASuite_t1(){

	cute::suite s;

	s.push_back(CUTE(CUTE_FMS_CPF_CreateFile::create_t1));
	s.push_back(CUTE(CUTE_FMS_CPF_CreateFile::unreserve));

	return s;

}
cute::suite CUTE_FMS_CPF_CreateFile::makeFMSCPFCreateFileASuite_t2(){
	cute::suite s;

	s.push_back(CUTE(CUTE_FMS_CPF_CreateFile::copy_constr));
	s.push_back(CUTE(CUTE_FMS_CPF_CreateFile::create_t2));
	s.push_back(CUTE(CUTE_FMS_CPF_CreateFile::unreserve));

	return s;
}
