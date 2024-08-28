#include "FMS_CPF_ServerStub.h"

#include <iostream>
#include "fms_cpf_file.h"
#include <string>

using namespace std;


FMS_CPF_ServerStub::FMS_CPF_ServerStub(void)
{
}

FMS_CPF_ServerStub::~FMS_CPF_ServerStub(void)
{
}

int FMS_CPF_ServerStub::mkvolume(char *volumename, char *cpname)
{
	try
	{
		FMS_CPF_File file ("", cpname);
		file.createVolume(volumename);
	}
	catch (FMS_CPF_Exception& ex) 
	{

		if(ex.errorCode() == FMS_CPF_Exception::INVALIDFILE) {
			cout << INVALID_VOLUME << endl;
			return (VOLUME_NOT_VALID);
		}
		std::string slogan;
		ex.getSlogan(slogan);
		cout << slogan << endl;
		return (int)(ex.errorCode());
	}
	return 0;
}
