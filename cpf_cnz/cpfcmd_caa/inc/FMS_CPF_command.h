#ifndef FMS_CPF_COMMAND_H
#define FMS_CPF_COMMAND_H

#include <iostream>
//#include <string>
#include <string.h>
#include <ACS_CS_API.h>
#include "fms_cpf_exception.h"

#define ERR55 "Unable to connect to configuration server"
#define ERR56 "Configuration Server Error"

const char S_NO [] =        "no";
const char S_YES [] =       "yes";
const char S_REGULAR [] =   "reg";
const char S_TEXT [] =      "txt";
const char S_INFINITE [] =  "inf";
const char S_DATALINK [] =  "dl";
const char S_CLEAR [] =     "clr";
const char S_OVERWRITE [] = "over";
const char S_APPEND [] =    "app";
const char S_NONE [] =      "none";
const char S_FILE [] =      "file";
const char S_BLOCK [] =     "block";
const char S_SUB[] =		"sub";
const char S_MAIN[] =		"main";

const char s_no [] =        "NO";
const char s_yes [] =       "YES";
const char s_regular [] =   "REG";
const char s_text [] =      "TXT";
const char s_infinite [] =  "INF";
const char s_clear [] =     "CLR";
const char s_overwrite [] = "OVER";
const char s_append [] =    "APP";
const char s_none [] =      "NONE";
const char s_file [] =      "FILE";
const char s_block [] =     "BLOCK";
const char s_sub[] =		"SUB";
const char s_main[] =		"MAIN";


struct optpair_t
{
	char opt;
	char *arg;
};

class FMS_CPF_command
{
public:
  int launch ();

protected:
  FMS_CPF_command(int argc, char* argv []);
  virtual void parse();
  virtual int execute() = 0;
  //virtual void usage(int ) = 0;
  virtual std::string usage() = 0;
  void incorrect_usage();
  std::string detail(const optpair_t& option);
  char* toUpper(char* str);
  bool MultipleCpCheck();

  int argc_;
  char** argv_;
  bool _multipleCPSystem;
  char *_cpName;
  std::string slogan;
};

#endif

