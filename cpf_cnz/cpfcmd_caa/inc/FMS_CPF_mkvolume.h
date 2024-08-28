#ifndef FMS_CPF_MKVOLUME_H
#define FMS_CPF_MKVOLUME_H

#include "FMS_CPF_command.h"
#include "FMS_CPF_ServerStub.h"
#include "FMS_CPF_CommandInterface.h"

using namespace std;

/**
 * FMS_CPF_mvolume class implments the client part of the cpfmkvolume command
 * which is used to create a volume in the CP File System.
 * This class is part of a Command Design pattern maked by following classes:
 * - the class FMS_CPF_mkvolume is a "concrete command" of the pattern;
 * - the class FMS_CPF_command is the "command" interface of th pattern;
 * - the class FMS_CPF_ServerStub is the "receiver". It implements the command;
 * - the class FMS_CPF_CommandInterface is the "invoker". It makes the callbacks to the commands.
 */
class FMS_CPF_mkvolume : public FMS_CPF_command
{
public:
  FMS_CPF_mkvolume (int argc, char* argv [], FMS_CPF_ServerStub stub);
  virtual ~FMS_CPF_mkvolume();

private:
  FMS_CPF_ServerStub m_stub;
  char m_pVolume[MAX_VOLUME_LEN+1];
  void parse (); 
  int execute ();
  void usage(int );
  string usage();

};

#endif

