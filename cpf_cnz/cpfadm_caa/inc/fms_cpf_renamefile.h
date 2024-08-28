/*
 * * @file fms_cpf_renamefile.h
 *	@brief
 *	Header file for CPF_RenameFile_Request class.
 *  This module contains the declaration of the class CPF_RenameFile_Request.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2012-09-26
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
 *	| 1.0.0  | 2012-09-26 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_RENAMEFILE_H_
#define FMS_CPF_RENAMEFILE_H_
/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_privateexception.h"
#include "fms_cpf_types.h"

#include <string>
#include <list>

/*===================================================================
                        CLASS FORWARD DECLARATION SECTION
=================================================================== */
class ACS_TRA_trace;
class OmHandler;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */

class CPF_RenameFile_Request
{
 public:

	struct renameFileData{
				std::string cpName;
				std::string currentFile;
				std::string newFile;
				std::string volumeName;

				std::string fileDN;
				std::string filePath;
				std::string newFilePath;
	};

	/**
		@brief	Constructor of CPF_RenameFile_Request class
	*/
	CPF_RenameFile_Request(const renameFileData& renameInfo);

	/**
		@brief	Destructor of CPF_RenameFile_Request class
	*/
	virtual ~CPF_RenameFile_Request();

	/**
		@brief	This method implements the action requested
	*/
	bool executeRenameFile();

	/**
		@brief	This method gets the error code
	*/
	int getErrorCode() const { return m_ExitCode; };

	/**
		@brief	This method gets the error message
	*/
	const char* getErrorMessage() const { return m_ExitMessage.c_str(); };

	/**
		@brief	This method gets the com-cli error message
	*/
	const char* getComCliErrorMessage() const { return m_ComCliExitMessage.c_str(); };

 private:

	/**
		 	@brief	This method the new named regular file object
	*/
	void renameRegularFile(bool composite, unsigned int recordLength, int deleteFileTimer, const std::list<std::string>& subFilesList)
		throw(FMS_CPF_PrivateException);

	/**
		@brief	This method creates the new named infinite file object
	*/
	void renameInfiniteFile(FMS_CPF_Types::fileAttributes& attribute)
		throw(FMS_CPF_PrivateException);

	/**
	 	@brief	This method creates the new named composite subfile objects
	*/
	void renameSubFile(const std::string& oldSubFileName, const std::string& newSubFileName )
		throw(FMS_CPF_PrivateException);

	/**
		@brief	This method updates the infinite file state
	*/
	void updateInfiniteFileState(FMS_CPF_Types::fileAttributes& attribute, const std::list<std::string>& subFilesList);

	/**
	 	@brief	This method assembles the lists of subfiles to create
	*/
	void makeSubFilesList(std::list<std::string>& subFilesToCreate);

	/**
	 	@brief	This method restore original state on error
	*/
	void undoRename();

	/**
	 	@brief	This method hidden the current physical file
	*/
	void hiddenFile() throw(FMS_CPF_PrivateException);

	/**
	 	@brief	This method shown the hidden physical file
	*/
	void undoHiddenFile();

	/**
	 	@brief	This method swaps the physical files
	*/
    void swapFile() throw(FMS_CPF_PrivateException);

    /**
     	@brief	This method undo the swap of physical files
    */
    void undoSwapFile() throw(FMS_CPF_PrivateException);

	/**
	 	@brief	This method gets parent DN from the child DN
	*/
    void getParentDN(const std::string& fileDN, std::string& parentDN) throw(FMS_CPF_PrivateException);

	/**
		@brief	The file rename parameters structure
	*/
	renameFileData m_RenameInfo;

	/**
		@brief	The flag to indicates file hidden
	*/
	bool m_fileHidden;

	/**
		@brief	The flag to indicates file swapped
	*/
	bool m_fileSwapped;

	/**
		@brief	List of created files
	*/
	std::list<std::string> m_CreatedFileDNs;

	/**
		@brief	List of deleted files
	*/
	std::list<std::string> m_DeleteFileDNs;

	/**
		@brief	The action exit code
	*/
	int m_ExitCode;

	/**
		@brief	The action exit message
	*/
	std::string m_ExitMessage;

	/**
		@brief	The action COM-CLI exit message
	*/
	std::string m_ComCliExitMessage;

	ACS_TRA_trace* m_trace;
};

#endif /* FMS_CPF_RENAMEFILE_H_ */
