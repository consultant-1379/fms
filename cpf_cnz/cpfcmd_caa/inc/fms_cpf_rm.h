#ifndef FMS_CPF_RM_H
#define FMS_CPF_RM_H

#include "FMS_CPF_command.h"
#include "fms_cpf_file.h"
#include <ace/ACE.h>
#include "ace/ARGV.h"
#include "ace/Get_Opt.h"
#include "fms_cpf_exception.h"
#include "fms_cpf_types.h"
#include "fms_cpf_fileid.h"
#include "fms_cpf_trace.h"
#include "ACS_APGCC_Util.H"

class FMS_CPF_rm : public FMS_CPF_command
{
public:
  FMS_CPF_rm (int argc, char* argv []);

private:
  enum option_t {
    RECURSIVE};
 
  void parse ()
  throw (FMS_CPF_Exception);

  int execute ()
  throw (FMS_CPF_Exception);

  void usage(int );

  //RWCString usage (); SIO_I8 tjer
  std::string usage();

  bool		 recursive_;
  FMS_CPF_FileId fileid_;
};

#endif
