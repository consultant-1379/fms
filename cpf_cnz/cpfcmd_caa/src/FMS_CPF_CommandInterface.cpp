#include "FMS_CPF_CommandInterface.h"

FMS_CPF_CommandInterface::FMS_CPF_CommandInterface(FMS_CPF_command *mkvol)
{
	mkvol_ = mkvol;
}

FMS_CPF_CommandInterface::~FMS_CPF_CommandInterface(void)
{
}

int
FMS_CPF_CommandInterface::cpfmkvolume()
{
	return mkvol_->launch();
}

