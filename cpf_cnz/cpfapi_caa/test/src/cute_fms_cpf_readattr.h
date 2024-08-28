/*
 * cute_fms_cpf_readattr.h
 *
 *  Created on: Feb 1, 2011
 *      Author: root
 */

#ifndef CUTE_FMS_CPF_READATTR_H_
#define CUTE_FMS_CPF_READATTR_H_

#include "cute.h"
#include "ide_listener.h"
#include "cute_runner.h"

#include "fms_cpf_file.h"

class CUTE_FMS_CPF_ReadAtt {
public:

	static void init();

	static void reserve_t1();

	static void reserve_t2_a();

	static void reserve_t2_b();

	static void reserve_t2_c();

	static void getAttributes_t1();

	static void getAttributes_t3();

	static void unreserve();

	static void destroy();

	static cute::suite makeFMSCPFReadAttSuite_t1();

	static cute::suite makeFMSCPFReadAttSuite_t2();

	static cute::suite makeFMSCPFReadAttSuite_t3();

	static FMS_CPF_File *file;
};

#endif /* CUTE_FMS_CPF_READATTR_H_ */
