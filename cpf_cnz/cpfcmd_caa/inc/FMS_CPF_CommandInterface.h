#ifndef FMS_CPF_COMMANDINTERFACE_H
#define FMS_CPF_COMMANDINTERFACE_H

#include "FMS_CPF_command.h"
class FMS_CPF_command;

/**
 * FMS_CPF_CommandInterface class implments the program interface for the commands.
 * This class is part of a Command Design pattern maked by following classes:
 * - the class FMS_CPF_mkvolume is a "concrete command" of the pattern;
 * - the class FMS_CPF_command is the "command" interface of th pattern;
 * - the class FMS_CPF_ServerStub is the "receiver". It implements the command;
 * - the class FMS_CPF_CommandInterface is the "invoker". It makes the callbacks to the commands.
 */

class FMS_CPF_CommandInterface
{
public:
	FMS_CPF_CommandInterface(FMS_CPF_command *mkvol);
	virtual ~FMS_CPF_CommandInterface(void);

	int cpfmkvolume();
private:
	FMS_CPF_command *mkvol_;
};

#endif

