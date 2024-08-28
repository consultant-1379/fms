#ifndef FMS_CPF_MKFILE_H
#define FMS_CPF_MKFILE_H

#include <string>
#include "FMS_CPF_command.h"
//#include "fms_cpf_exception.h"
#include "fms_cpf_file.h"
#include "fms_cpf_fileid.h"
#include "fms_cpf_eventhandler.h"
#include "fms_cpf_types.h"
#include "fms_cpf_trace.h"
class ACS_TRA_trace;
using namespace std;


class FMS_CPF_mkfile : public FMS_CPF_command
{
public:
  FMS_CPF_mkfile (int argc, char* argv []);
  ~FMS_CPF_mkfile ();
private:
  enum option_t {
    FTYPE, 
    RLENGTH, 
    COMPOSITE,
	COMPRESS,
    MAXSIZE,
    MAXTIME,
    RELEASE,
    TRANSFERQUEUE
  };
  
  void parse ()
  throw (FMS_CPF_Exception);
  int execute ()
  throw (FMS_CPF_Exception);
  void usage(int );
  string usage();
  string transferQueue_;
  FMS_CPF_Types::fileType ftype_;
  unsigned int rlength_;
  bool composite_;
  bool compress_;
  bool composite_datalink_;
  bool release_;
  unsigned long maxsize_;
  unsigned long maxtime_;
  FMS_CPF_FileId fileid_;
  char* volume_; 
  ACS_TRA_trace* cpfmkfile;
};
#endif
