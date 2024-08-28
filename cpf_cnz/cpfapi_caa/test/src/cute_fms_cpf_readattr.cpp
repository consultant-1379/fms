/*
 * cute_fms_cpf_readattr.cpp
 *
 *  Created on: Feb 1, 2011
 *      Author: root
 */

#include "cute_fms_cpf_readattr.h"

//CUTE_FMS_CPF_ReadAtt::CUTE_FMS_CPF_ReadAtt() {
//	// TODO Auto-generated constructor stub
//
//}
//
//CUTE_FMS_CPF_ReadAtt::~CUTE_FMS_CPF_ReadAtt() {
//	// TODO Auto-generated destructor stub
//}

void CUTE_FMS_CPF_ReadAtt::init(){
	char filename[10];
	ACE_OS::memset(filename,0, sizeof(filename));
	ACE_OS::strcpy(filename,"afilename");

	char cpName[7];
	ACE_OS::memset(cpName,0, sizeof(cpName));
	ACE_OS::strcpy(cpName,"CP1");

	file = new FMS_CPF_File(filename, cpName);
}

void CUTE_FMS_CPF_ReadAtt::reserve_t1(){

}
