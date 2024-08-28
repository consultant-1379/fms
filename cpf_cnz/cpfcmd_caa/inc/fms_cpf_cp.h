#ifndef FMS_CPF_CP_H
#define FMS_CPF_CP_H

#include "FMS_CPF_command.h"
#include "fms_cpf_file.h"
#include "fms_cpf_fileid.h"
#include "fms_cpf_exception.h"
#include "fms_cpf_types.h"
#include <ace/ACE.h>
#include "ace/ARGV.h"
#include "ace/Get_Opt.h"
#include "fms_cpf_api_trace.h"
#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
#include "ACS_APGCC_Util.H"

class FMS_CPF_cp : public FMS_CPF_command
{
public:
  FMS_CPF_cp (int argc, char* argv []);
  ~FMS_CPF_cp ();
private:
  enum option_t {
    MODE};

  void parse ()
  throw (FMS_CPF_Exception);

  int execute ()
  throw (FMS_CPF_Exception);

  void usage (int eCode);

  std::string usage();

  FMS_CPF_Types::copyMode mode_;
  FMS_CPF_FileId	  fileid1_;
  FMS_CPF_FileId	  fileid2_;
  ACS_TRA_trace* cpfcp;
};

#endif
