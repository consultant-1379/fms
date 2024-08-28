#ifndef FMS_CPF_SERVERSTUB_H
#define FMS_CPF_SERVERSTUB_H

/**
 * FMS_CPF_ServerStub class implments the cpfmkvolume command together APDFile class.
 * This class is part of a Command Design pattern maked by following classes:
 * - the class FMS_CPF_mkvolume is a "concrete command" of the pattern;
 * - the class FMS_CPF_command is the "command" interface of th pattern;
 * - the class FMS_CPF_ServerStub is the "receiver". It implements the command;
 * - the class FMS_CPF_CommandInterface is the "invoker". It makes the callbacks to the commands.
 */

#include "fms_cpf_exception.h"
#define MAX_VOLUME_LEN 12
const int VOLUME_NOT_VALID = 57;
const std::string INVALID_VOLUME = "Invalid volume name";

class FMS_CPF_ServerStub
{
public:
	FMS_CPF_ServerStub();
	virtual ~FMS_CPF_ServerStub(void);
	int mkvolume(char *volumename, char * cpname);
};

#endif

