/*
 * cute_fms_cpf_createfile.h
 *
 *  Created on: Jan 26, 2011
 *      Author: esalves
 */

#ifndef CUTE_FMS_CPF_CREATEFILE_H_
#define CUTE_FMS_CPF_CREATEFILE_H_

#include "cute.h"
#include "ide_listener.h"
#include "cute_runner.h"

#include "fms_cpf_file.h"

class CUTE_FMS_CPF_CreateFile {

public:

//	CUTE_FMS_CPF_CreateFile();
	static void init_t1();

	static void init_t2();

	static void copy_constr();

//	virtual ~CUTE_FMS_CPF_CreateFile();
	static void destroy();

	static void create_t1();

	static void create_t2();

	static void unreserve();

	static cute::suite makeFMSCPFCreateFileASuite_t1();

	static cute::suite makeFMSCPFCreateFileASuite_t2();

	static FMS_CPF_File *file;
};

#endif /* CUTE_FMS_CPF_CREATEFILE_H_ */
