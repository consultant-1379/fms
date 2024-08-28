//******************************************************************************
//
// .NAME
//      FMS_CPF_File - File handler API
// .LIBRARY 3C++
// .PAGENAME FMS_CPF_File
// .HEADER  AP/FMS Internal
// .LEFT_FOOTER Ericsson Utvecklings AB
// .INCLUDE FMS_CPF_File.H
//
// .COPYRIGHT
//  COPYRIGHT Ericsson Utvecklings AB, Sweden 1997.
//  All rights reserved.
//
//  The Copyright to the computer program(s) herein
//  is the property of Ericsson Utvecklings AB, Sweden.
//  The program(s) may be used and/or copied only with
//  the written permission from Ericsson Utvecklings AB or in
//  accordance with the terms and conditions stipulated in the
//  agreement/contract under which the program(s) have been
//  supplied.
//
// .DESCRIPTION
//
//      The FMS_CPF_File class shall be used by users of
//      CP file system in the Adjunct Processor.
//      An instance of the class gives access to a CP file.
//
//
// .ERROR HANDLING
//
//      All errors are reported by throwing the exception class
//      FMS_CPF_Exception.
//
//
// DOCUMENT NO
//      190 89-CAA 109 0088
//
// AUTHOR
//      1997-11-18      by UAB/I/GD     UABTSO
//
// CHANGES
//
//      CHANGE HISTORY
//
//      DATE    NAME    DESCRIPTION
//      980119  UABTSO  1:st revision
//      990201  QABLAKE  NTversion
//          Add prefix to copy mode and file type
//			20020916 QABDAPA added member trace.
//				esalves
//
// .LINKAGE
//      librwtool.so
//
// .SEE ALSO
//      FMS_CPF_Types.H, FMS_CPF_FileIterator.H and FMS_CPF_Exception.H
//
//******************************************************************************

#ifndef FMS_CPF_FILE_H
#define FMS_CPF_FILE_H

#include "fms_cpf_types.h"
#include "fms_cpf_exception.h"
#include "fms_cpf_fileid.h"
#include "fms_cpf_client.h"

#include <list>
#include <string>

class ACS_TRA_trace;

//******************************************************************************
// Class declaration
//******************************************************************************

class FMS_CPF_File
{

 public:

	friend class CPF_CopyFile_Request;
	friend class CPF_ImportFile_Request;
	friend class RegularCPDFile;
	friend class CPF_MoveFile_Request;
	//------------------------------------
	//Constructors
	//------------------------------------

	FMS_CPF_File() throw(FMS_CPF_Exception);
	// Description:
	//	Creates an empty instance of class FMS_CPF_File.
	// Parameters:
	// 	None
	// Exceptions:
	//    	None
	// Additional information:
	//   	None

	FMS_CPF_File(const char* filename, const char* pCPName = "") throw(FMS_CPF_Exception);
	// Description:
	//    	Constructs an FMS_CPF_File instance intialized with the CP file name.
	//    	The file name is not checked, nor is the file reserved by the
	//	constructor.
	// Parameters:
	//    	filename		CP file name
	// Exceptions:
	//    	None
	// Additional information:
	//   	None

	FMS_CPF_File(const FMS_CPF_File& file) throw(FMS_CPF_Exception);
	// Description:
	//    	Copy constructor for the FMS_CPF_File class. The new instance
	//	contains a copy of the file name. The file name is not checked,
	//	nor is the file reserved by the constructor.
	// Parameters:
	//   	file                    FMS_CPF_File instance
	// Exceptions:
	//    	None
	// Additional information:
	//   	None

	//------------------------------------
	//Destructor
	//------------------------------------
	~FMS_CPF_File();
	// Description:
	//    	Destructs an FMS_CPF_File instance
	// Additional information:
	//    	None

	//------------------------------------
	// Operators
	//------------------------------------

	const FMS_CPF_File& operator= (const FMS_CPF_File& file) throw(FMS_CPF_Exception);
	// Description:
	//    	Assignment operator method. The new instance contains a copy of the
	//	file name. The file name is not checked, nor is the file reserved
	//	by the assignment.
	// Parameters:
	//    	file                    FMS_CPF_File instance.
	// Return value:
	//    	FMS_CPF_File instance
	// Exceptions:
	//	FILEISOPEN		File is already open
	// Additional information:
	//    	None

	int operator== (const FMS_CPF_File& file) const;
	// Description:
	//    	Equality test operator method. This method compares two FMS_CPF_File
	//	instances for equality between the file names.
	// Parameters:
	//    	file                    FMS_CPF_File instance
	// Return value:
	//    	1 if file names are equal, 0 otherwise
	// Exceptions:
	//    	None
	// Additional information:
	//    	None

	int operator!= (const FMS_CPF_File& file) const;
	// Description:
	//    	Unequality test operator method. This method compares two FMS_CPF_File
	//	instances for unequality between the file names.
	// Parameters:
	//    	file                    FMS_CPF_File instance
	// Return value:
	//    	1 if file names are unequal,
	//	0 otherwise
	// Exceptions:
	//    	None
	// Additional information:
	//    	None

	//------------------------------------
	// Functions
	//------------------------------------

	void reserve(FMS_CPF_Types::accessType access)
		throw (FMS_CPF_Exception);
	// Description:
	//    	Reserve the file. This method reserves (opens) an existing CP file
	//	with a certain access. The FMS_CPF_File instance must have been
	//	constructed with a valid file name.
	// Parameters:
	//    	access		Access type, se FMS_CPF_Types for a
	//			detailed description
	// Return value:
	//    	None
	// Exceptions:
	//	INVALIDFILE		Invalid file name
	//	FILEISOPEN		File is already open
	//	FILENOTFOUND		File was not found
	//	ACCESSERROR		Access error
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	void unreserve() throw(FMS_CPF_Exception);
	// Description:
	//    	Unreserve the file. This method unreserves (closes) a file which was
	//	previously reserved.
	// Parameters:
	//    	None
	// Return value:
	//    	None
	// Exceptions:
	//	INVALIDREF		File not reserved
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	void create(const FMS_CPF_Types::fileAttributes& attributes, const char* volume, bool compress = false)
		throw (FMS_CPF_Exception);
	// Description:
	//    	Create a file. This method creates a CP file and reserves (opens)
	//	it with a certain access. The FMS_CPF_File instance must have been
	//	constructed with a valid file name.
	// Parameters:
	//    	attributes	File attributes
	//    	volume		Volume name
	//    	access		Access type, se FMS_CPF_Types for a detailed description
	//	compress	Flag for determing compression. True if file is to be compressed.
	// Return value:
	//    	None
	// Exceptions:
	//	INVALIDFILE	Invalid file name
	//	NOTMAINFILE	Not a main file
	//	FILEEXISTS		File does already exist
	//	ILLVALUE		Illegal value
	//	PHYSICALERROR	Physical file error
	//	SOCKETERROR	Lost connection with the CPF server
	//	INTERNALERROR	Internal program error
	// Additional information:
	//    	None

	void create(bool compress = false) throw(FMS_CPF_Exception);
	// Description:
	//    	Create a subfile. This method creates a CP subfile and reserves it
	//	with a certain access. The FMS_CPF_File instance must have been
	//	constructed with a valid subfile name.
	// Parameters:
	//	compress		Flag for determing compression. True if file is to be compressed.
	// Return value:
	//    None
	// Exceptions:
	//	INVALIDFILE		Invalid file name
	//	NOTSUBFILE		Not a subfile
	//	FILENOTFOUND		File was not found
	//	FILEEXISTS			File does already exist
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	void remove() throw(FMS_CPF_Exception);
	// Description:
	//    	Remove the file. This method removes a file from the CP file system
	//	and removes the physical file.
	// A composite file may not contain subfiles in order to remove it.
	// Parameters:
	//   	None
	// Return value:
	//    	None
	// Exceptions:
	//	CMPNOTEMPTY		Composite file contains subfiles
	//	INVALIDREF		File not reserved
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	void deleteFile(bool recursive_ = false) throw(FMS_CPF_Exception);
	// Description:
	//    	Deletes the file. This method removes a file from the CP file system
	//	and removes the physical file.
	// Parameters:
	//    	Recursive, set if subfiles of a mainfile shall be removed
	// Return value:
	//    	None
	// Exceptions:
	//	CMPNOTEMPTY		Composite file contains subfiles
	//	INVALIDREF		File not reserved
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	bool isReserved() const;
	// Description:
	//    	Checks if the file instance is reserved. This method checks if the
	//	FMS_CPF_File instance is reserved.
	// Parameters:
	//    	None
	// Return value:
	//    	Returns true if the FMS_CPF_File instance is reserved, false
	//	otherwise.
	// Exceptions:
	//    	None
	// Additional information:
	//    	None

	bool exists() throw(FMS_CPF_Exception);
	// Description:
	//   	Checks if the file exists. This method checks if a file exists in
	//	the CP file system. No reservation of the file is required.
	// Parameters:
	//    	None
	// Return value:
	//    Returns true if the file exists, false otherwise.
	// Exceptions:
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    None

	const char* getCPFname() const;
	// Description:
	//    Return CP file name for the file. This method returns the file name
	//	that the instance was constructed by. It does not indicate whether
	//	the file exists or not.
	// Parameters:
	//    	None
	// Return value:
	//    	CP File name
	// Exceptions:
	//   	None
	// Additional information:
	//    	None

	void rename(const char* filename) throw(FMS_CPF_Exception);
	// Description:
	//     	Rename a file. This method renames a file in the CP file system.
	//	The file must have been reserved with XR_XW_ access.
	// Parameters:
	//    	filename		The new CP file name
	// Return value:
	//    	None
	// Exceptions:
	//	INVALIDFILE		Invalid file name
	//	NOTMAINFILE		Not a main file
	//	FILEEXISTS		File does already exist
	//	ACCESSERROR		Access error
	//	INVALIDREF		File not reserved
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	void setAttributes(const FMS_CPF_Types::fileAttributes& attributes) throw(FMS_CPF_Exception);
	// Description:
	//    	Change attributes for the file. This method changes attributes for
	//	an infinite file in the CP file system.
	// Parameters:
	//    	attributes              File attributes
	// Return value:
	//    	None
	// Exceptions:
	//	ACCESSERROR		Access error
	//	TYPEERROR		Operation not permitted for this file type
	//	INVALIDREF		File not reserved
	//	ILLVALUE		Illegal value
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	FMS_CPF_Types::fileAttributes getAttributes() throw(FMS_CPF_Exception);
	// Description:
	//    	Get attributes for the file. This method reads the attributes for
	//	a file in the CP file system.
	// Parameters:
	//    	None
	// Return value:
	//    	The new file attributes
	// Exceptions:
	//	INVALIDREF		File not reserved
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	void infiniteEnd() throw(FMS_CPF_Exception);
	// Description:
	//    	Imfinite file end. This method closes the active infinite subfile
	//	and creates a new active subfile.
	// Parameters:
	//    	None
	// Return value:
	//    	The new file attributes
	// Exceptions:
	//	INVALIDREF		File not reserved
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	void move(const char* volume) throw(FMS_CPF_Exception);
	// Description:
	//    	Move file to another volume. This method moves the file in the
	//	CP file system to another volume.
	// Parameters:
	//    	volume                  The new volume name
	// Return value:
	//    	None
	// Exceptions:
	//	ACCESSERROR		Access error
	//	INVALIDREF		File not reserved
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	const char* getVolume() throw(FMS_CPF_Exception);
	// Description:
	//    	Get the volume name. This method reads the volume name for a file
	//	in the CP file system.
	// Parameters:
	//    	None
	// Return value:
	//    	The volume name
	// Exceptions:
	//	INVALIDREF		File not reserved
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	FMS_CPF_Types::userType getUsers() throw(FMS_CPF_Exception);
	// Description:
	//    	Read the number of users of the file. This method reads the number
	//	of users (read access, write access and total number).
	//  The source file must have been reserved
	// Parameters:
	//    	None
	// Return value:
	//    	users                   Number of users of the file
	// Exceptions:
	//	INVALIDREF		File not reserved
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	void getStat(FMS_CPF_Types::fileStat& stat) throw(FMS_CPF_Exception);
	// Description:
	//    	Read file status for the file. This method reads status information
	//	for a file in the CP file system. The source file must have been reserved
	// Parameters:
	//    	stat         Status information for the file, see
	//			FMS_CPF_Types for a detailed description
	// Return value:
	//    	None
	// Exceptions:
	//	INVALIDREF		File not reserved
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	std::string getPhysicalPath() throw(FMS_CPF_Exception);
	// Description:
	//    	Get physical path. This method reads the full physical path name
	//	for the CP file. The source file must have been reserved
	// Parameters:
	//    	None
	// Return value:
	//    	The physical path name
	// Exceptions:
	//	INVALIDREF		File not reserved
	//	PHYSICALERROR	Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR	Internal program error
	// Additional information:
	//    	None

	int getPhysicalPath(char* filePath, int& bufferLenth) throw(FMS_CPF_Exception);
	// Description:
	//    	Get physical path. This method reads the full physical path name
	//	for the CP file. The source file must have been reserved
	// Parameters:
	//    	filePath: The physical path name
	//		bufferLenth: buffer size
	// Return value:
	//    	The length of the file path on success
	// Exceptions:
	//	INVALIDREF		File not reserved
	//	PHYSICALERROR	Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR	Internal program error
	// Additional information:
	//    	None

	void copy(const char* filename, FMS_CPF_Types::copyMode mode = FMS_CPF_Types::cm_NORMAL)
		throw (FMS_CPF_Exception);
	// Description:
	//    	Copy the file to a new file. This method copies a file in the CP
	//	file system to another CP file. The destination main file must exist.
	//	The source file must have been reserved with R_XW_ or XR_XW access.
	//	The destination file must not be reserved.
	// Parameters:
	//    	filename         	Destination CP file name
	//	mode			Copying mode
	// Return value:
	//    None
	// Exceptions:
	//	INVALIDFILE		Invalid file name
	//	FILENOTFOUND,		Destination file was not found
	//	ACCESSERROR		Access error
	//	INVALIDREF		File not reserved
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	void fileExport(std::string path, FMS_CPF_Types::copyMode mode = FMS_CPF_Types::cm_NORMAL, bool toZip = false)
		throw (FMS_CPF_Exception);
	// Description:
	//    	Export the file. This method exports a file in the CP file system
	//	to a physical path. The file must have been reserved with R_XW_
	//	or XR_XW access.
	// Parameters:
	//    	path			Destination physical path
	//		mode			Copying mode
	//		toZip 			Zip indication: if true, it means that the file exported must be zipped.
	// Return value:
	//    	None
	// Exceptions:
	//	INVALIDFILE		Invalid file name
	//	ACCESSERROR		Access error
	//	INVALIDREF		File not reserved
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	void fileImport(std::string path, FMS_CPF_Types::copyMode mode = FMS_CPF_Types::cm_NORMAL)
		throw (FMS_CPF_Exception);
	// Description:
	//    	Import the file. This method imports a physical path to a file in
	//	the CP file system. The file must not have been reserved.
	// Parameters:
	//    	path			Source physical path
	//	mode			Copying mode
	// Return value:
	//   	None
	// Exceptions:
	//	INVALIDFILE		Invalid file name
	//	ACCESSERROR		Access error
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	void manuallyReportFile(const char* destination, int retries, int timeinterval)
		throw (FMS_CPF_Exception);
	// Description:
	//    	Manual reporting of a file to GOH.
	//	This method enables a user to manually report
	//	a specified file to GOH. The file need not to be reserved.
	// Parameters:
	//   	None
	// Return value:
	//    	None
	// Exceptions:
	//	None
	// Additional information:
	//    	None

	void setCompression(bool compressSub = false) throw(FMS_CPF_Exception);
	// Description:
	//	Set compression for a file.
	// Parameters:
	//	compressSub If true subfiles will also be compressed.
	// Return value:
	//	None.
	// Exceptions:
	//	INTERNALERROR
	// Additional information:
	//	None.

	void unsetCompression(bool uncompressSub = false) throw(FMS_CPF_Exception);
	// Description:
	//	Unset compression for a file.
	// Parameters:
	//	uncompressSub If true subfiles will also be decompressed.
	// Return value:
	//	None.
	// Exceptions:
	//	INTERNALERROR
	// Additional information:
	//	None.

	void createVolume(const char * volumename) throw(FMS_CPF_Exception);
	// Description:
	//  	Creates a Volume. This method creates a volume into CP file system.
	// Parameters:
	//    	Volumename, the volume name to be created
	// Return value:
	//    	None
	// Exceptions:
	//	PHYSICALERROR		Physical file error
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	// Additional information:
	//    	None

	static const FMS_CPF_File EOL;
	// Description:
	//	The EOL (end of list) constant is used when iterating over
	//	FMS_CPF_File instances with the FMS_CPF_FileIterator class.

private:

	enum ActionType {
		Import_CpFile = 1,
		Export_CpFile,
		Copy_CpFile,
		Move_CpFile,
		Rename_CpFile,
		Import_CpClusterFile,
		Export_CpClusterFile,
		Copy_CpClusterFile,
		Move_CpClusterFile,
		Rename_CpClusterFile
	};

	enum TqType {
	  TQ_FILE,
	  TQ_BLOCK,
	  TQ_UNDEFINED
	};

	/** @brief getTqType method
	 *
	 *	This method returns the TQ type
	 *  @param tq_name : TQ name
	 *  @param tq_type : TQ type returned by GOH
	 *  @param errorTxt : Error description after the GOH operation
	 *
	 *	Return the error code
	 *
	*/
	unsigned int getTqType (std::string tq_name, TqType& tq_type, std::string& errorTxt);

	/** @brief isTQNameValid method
	 *
	 *	This method returns true if the TQ name is valid
	 *
	 *	Return the TQ type
	*/
	bool isTQNameValid (const std::string& tqName);

	// Get Volume name by the composite file name and the CP name
	std::string getVolumeInfo(std::string filename) throw(FMS_CPF_Exception);

	//  get volume DN
	std::string getVolumeDN(std::string& volumeName);

	// Get DN related to the Composite File
	int getCompositeFileDN(std::string& volumeName, const std::string& compFileName, std::string& comFileDN);

	// Get DN related to the Composite SubFile
	int getSubCompositeFileDN(const std::string& comFileDN, const std::string& subFileName, std::string& subComFileDN);

	int decompressFile(FILE *source, FILE *dest);

	int compressFile(FILE *source, FILE *dest, int level);

	//this function return true if the CP in m_pCPName exists otherwise return false
	//to be defined
	bool isCP();

	void readConfiguration(const char* pCPName)
		throw (FMS_CPF_Exception);

	//not implem.
	bool endOfFile() throw (FMS_CPF_Exception);

	// Check volume name
	bool checkVolName(const std::string &vName);

	bool compressItem(const char* path);

	bool uncompressItem(const char* path);

	const char* findFirstSubfile();

	const std::list<std::string> findFileList(const std::string& path);

	int writeCompositeFile(FMS_CPF_FileId& fileid_, const FMS_CPF_Types::fileAttributes& attributes);
	
	int writeCompositeSubFile(std::string dn, std::string subFile);

	int writeInfiniteFile(FMS_CPF_FileId& fileid_, const FMS_CPF_Types::fileAttributes& attributes);

	int writeSimpleFile(FMS_CPF_FileId& fileid_, unsigned int rlength_);

	// copy the file to a destination file
	int copyFileAction(const std::string& dstFileName, FMS_CPF_Types::copyMode mode)
	throw (FMS_CPF_Exception);

	// Import the file (path) to a CP destination file
	int fileImportAction(const std::string& path, FMS_CPF_Types::copyMode mode)
	throw (FMS_CPF_Exception);

	int createInternalSubFiles(const std::string& compositeFileName, const std::list<std::string>& subFileNameList, const std::string& volume, const std::string& cpName);

	int deleteInternalSubFiles(const std::string& compositeFileName, const std::string& volume, const std::string& cpName);

	int fileExportAction(const std::string& dstFileName, FMS_CPF_Types::copyMode copyMode, bool toZip = false)
		throw (FMS_CPF_Exception);

	//------------------------------------
	// Variables
	//------------------------------------
	ACS_TRA_trace* fmsCpfFileTrace;

	FMS_CPF_Client DsdClient;

	FMS_CPF_Types::accessType access_;
	std::string filename_;
	std::string volume_;

	unsigned long reference_;

	bool m_bIsSysBC; //if the system is BC
	bool m_bCPExists; //if the CP exists
	bool m_bIsConfigRead;

	char m_pCPName[20];
	short m_nCP_ID;
	int m_nNumCP;

	std::list<std::string> m_strListCP;

};

#endif /* FMS_CPF_FILE_H */
