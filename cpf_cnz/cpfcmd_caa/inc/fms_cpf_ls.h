#ifndef FMS_CPF_LS_H
#define FMS_CPF_LS_H

#include "FMS_CPF_command.h"
#include "fms_cpf_exception.h"
#include "fms_cpf_types.h"
#include "fms_cpf_fileid.h"
#include "fms_cpf_fileiterator.h"

#include <string>

class ACS_TRA_trace;

class FMS_CPF_ls : public FMS_CPF_command {
public:

	FMS_CPF_ls(int argc, char* argv []);
	
	virtual ~FMS_CPF_ls ();

private:

	enum option_t {
		COMPRESSED,
		LONG,
		SUBFILES,
		QUIET,
		PATH
	};

	void parse();

	void listUsers(FMS_CPF_Types::userType users);

	void listFileEntry(FMS_CPF_FileIterator::FMS_CPF_FileData &fd, FMS_CPF_FileIterator &iter);

	void listPath(FMS_CPF_FileIterator::FMS_CPF_FileData &fd);

	int execute();

	void usage(int);

	std::string usage();

	void listSubfile(FMS_CPF_FileIterator::FMS_CPF_FileData &fd)throw(FMS_CPF_Exception);

	bool isSubOnly;
	bool all_;
	bool long_;
	bool quiet_;
	bool subfiles_;
	bool path_;
	bool compressed_;

	std::string fileInput;

	FMS_CPF_FileId fileid_;

	ACS_TRA_trace* cpfls;
};
#endif
