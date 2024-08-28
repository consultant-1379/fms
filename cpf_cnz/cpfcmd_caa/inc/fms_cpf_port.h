#ifndef FMS_CPF_PORT_H
#define FMS_CPF_PORT_H

#include "FMS_CPF_command.h"
#include "fms_cpf_types.h"
#include "fms_cpf_fileid.h"


class ACS_TRA_trace;

class FMS_CPF_port : public FMS_CPF_command {

public:

	FMS_CPF_port(int argc, char* argv []);

    virtual ~FMS_CPF_port ();

private:

    enum option_t {
	   	   	   EXPORT,
	   	   	   IMPORT,
	   	   	   MODE,
	   	   	   ZIP,
	   	   	   FORCE
    };


    void parse() throw (FMS_CPF_Exception);

    int execute() throw (FMS_CPF_Exception);

    std::string usage();

    void usage(int );

    bool checkFileSize(const std::string& path) throw(FMS_CPF_Exception);  // Check the file/folder size

    bool confirm(void);// Checks the user response

    bool getAbsolutePath(const std::string& relativePath, std::string& absolutePath);

    bool getFileMCpFilePath(std::string& fileMPath);

    void printDots();//Printing Dots during cpfport execution for filesize more than 2 GB
    //TR HK89831 & HK89834:END

    bool zipOpt;

    enum {
    	OUT_,
    	IN_
    } direction_;

    FMS_CPF_Types::copyMode mode_;

    FMS_CPF_FileId fileid_;

    char* path_;

    //Checks whether cpfport command execution is completed for filesize more than 2GB.
    volatile bool portingFinished;

    bool m_force;

    ACS_TRA_trace* cpfport;
  
};

#endif
