/*
 * * @file fms_cpf_apdcommonfile.h
 *	@brief
 *	This module contains the common declaration of the API communication protocol.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-08-09
 *	@version 1.0.0
 *
 *	COPYRIGHT Ericsson AB, 2010
 *	All rights reserved.
 *
 *	The information in this document is the property of Ericsson.
 *	Except as specifically authorized in writing by Ericsson, the receiver of
 *	this document shall keep the information contained herein confidential and
 *	shall protect the same in whole or in part from disclosure and dissemination
 *	to third parties. Disclosure and disseminations to the receivers employees
 *	shall only be made on a strict need to know basis.
 *
 *	REVISION INFO
 *	+========+============+==============+=====================================+
 *	| REV    | DATE       | AUTHOR       | DESCRIPTION                         |
 *	+========+============+==============+=====================================+
 *	| 1.0.0  | 2011-08-09 | qvincon      | File updated.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */

#ifndef FMS_CPF_APDCOMMONFILE_H
#define FMS_CPF_APDCOMMONFILE_H
#include <string>

namespace CPF_API_Protocol
{
    const char CPF_DSD_Address[] = "FMS_CPFD:FMS";
    const std::string CPF_ApplicationName = "FMS_CPFD";
   	const std::string FMS_Domain = "FMS";

	const unsigned long  MAGIC_NUMBER = 0x5d3a6831;
	const unsigned short VERSION = 5;

	enum
	{
		INIT_Session = 0,
		OPEN_,
		CLOSE_,
		EXISTS_,
		GET_PATH_,
		GET_VOLUME_,
		GET_STATUS_,
		GET_FILEINFO_,
		GET_USERS,
		GET_SUBFILESLIST,
		REMOVE_INFINITESUBFILE,
		EXIT_
	};

	enum
	{
		LIST_FILES_NOTOK,
		LIST_FILES_OK,
		LIST_FILES_START,
		LIST_FILES_END
	};
}
#endif
