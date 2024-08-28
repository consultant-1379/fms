#include "cute.h"
#include "ide_listener.h"
#include "cute_runner.h"

#include "cute_fms_cpf_createfile.h"

//void thisIsATest() {
//	ASSERTM("start writing tests", false);
//}
//
//void runSuite(){
//	cute::suite s;
//	//TODO add your test here
//	s.push_back(CUTE(thisIsATest));
//	cute::ide_listener lis;
//	cute::makeRunner(lis)(s, "The Suite");
//}

void runSuiteCreateFile_t1(){

	cute::ide_listener lis;

	CUTE_FMS_CPF_CreateFile::init_t1();

	cute::makeRunner(lis)(CUTE_FMS_CPF_CreateFile::makeFMSCPFCreateFileASuite_t1(),
			"T1) Running suite for FMS_CPF_FIle\n\n");

	CUTE_FMS_CPF_CreateFile::destroy();
}

void runSuiteCreateFile_t2(){
	cute::ide_listener lis;
	CUTE_FMS_CPF_CreateFile::init_t2();
	cute::makeRunner(lis)(CUTE_FMS_CPF_CreateFile::makeFMSCPFCreateFileASuite_t2(),
				"T2) Running suite for FMS_CPF_FIle\n\n");

	CUTE_FMS_CPF_CreateFile::destroy();

}

int main(){
    runSuiteCreateFile_t1();
    runSuiteCreateFile_t2();

    return 0;
}



