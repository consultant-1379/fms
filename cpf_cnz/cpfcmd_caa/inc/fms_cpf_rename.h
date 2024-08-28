#ifndef FMS_CPF_RENAME_H
#define FMS_CPF_RENAME_H

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

class ACS_TRA_trace;


class FMS_CPF_rename : public FMS_CPF_command
{
public:
  FMS_CPF_rename (int argc, char* argv []);
  ~FMS_CPF_rename ();

private:
  enum option_t {
    }; 

void parse ()
  throw (FMS_CPF_Exception);

  int execute ()
  throw (FMS_CPF_Exception);

  void usage (int eCode);

  bool isInfiniteSubfile;
  //RWCString usage (); //SIO_I8 tjer
  std::string usage();
 
  FMS_CPF_FileId fileid1_;
  FMS_CPF_FileId fileid2_;
  ACS_TRA_trace* cpfrename;

};

#endif
