#ifndef FMS_CPF_FILEITERATOR_H
#define FMS_CPF_FILEITERATOR_H

#include "fms_cpf_exception.h"
#include "fms_cpf_types.h"
#include "fms_cpf_client.h"

#include <string>
#include <map>
#include <vector>

//******************************************************************************
// Class declaration
//******************************************************************************

class ACS_TRA_trace;
class ACS_APGCC_ImmAttribute;
class FMS_CPF_omCmdHandler;

class FMS_CPF_FileIterator
{
  public:
	typedef std::map<std::string, std::string> mapFileId;

    struct FMS_CPF_FileData    {
            bool valid;
            bool composite;
            bool release;
            bool compressed;
            std::string fileName;
            std::string pathList;
            std::string tqname;
            std::string volume;
            unsigned int file_size;
            unsigned int recordLength;
            unsigned long maxsize;
            unsigned long maxtime;
            unsigned long active;
            unsigned long ucount;
            unsigned long rcount;
            unsigned long wcount;
            FMS_CPF_Types::fileType  ftype;
            FMS_CPF_Types::transferMode mode;
        }fd;



    FMS_CPF_FileIterator(const char* filename, bool longList, const char* pCPName = "")
        throw (FMS_CPF_Exception);

    FMS_CPF_FileIterator(const char* filename, bool longList, bool listSubFiles, const char* pCPName = "")
        throw (FMS_CPF_Exception);
    // Description:
    //    Constructs an instance of the FMS_CPF_FileIterator class for
    //    iterating over all files in the file catalogue or a subset of the
    //    files matching a regular expression. (Regular expression matching
    //    is not yet implemented).
    // Parameters:
    //     None
    // Exceptions:
    //    INVALIDFILE        Invalid file name
    //    FILENOTFOUND        File was not found
    //    NOTMAINFILE        Not a main file
    //    SOCKETERROR        Lost connection with the CPF server
    //    INTERNALERROR        Internal program error
    // Additional information:
    //    None

    ~FMS_CPF_FileIterator ();
    // Description:
    //    Destructs the FMS_CPF_FileIterator instance.
    // Additional information:
    //    None

    bool getNext(FMS_CPF_FileIterator::FMS_CPF_FileData& fd) throw(FMS_CPF_Exception);
    //  CPFDLLEXPORT FMS_CPF_FileData& operator() ()
    //  throw ();
    // Description:
    //    Return next FMS_CPF_File instance.
    // Parameters:
    //     None
    // Return value:
    //    FMS_CPF_File instance. If all instances have been traversed the
    //    empty instance FMS_CPF_File::EOL is returned.
    // Exceptions:
    //    None
    // Additional information:
    //    None
    
    void setSubFiles(bool );

 private:

        int cleanVolumeList(std::vector<std::string>& volumeDNList);

        bool getLastFieldValue(const std::string& objectDN, std::string& fieldValue);

        void getParentDN(const std::string& objectDN, std::string& parentDN);

        void getFileExtraInfo(const std::string& fileDN, FMS_CPF_FileData& fileData);

        void getSubFileExtraInfo(const std::string& subFileDN, FMS_CPF_FileData& fileData);

        bool getVolumeName(const std::string& volumeDN, std::string& volumeName);

        void initialize();

        int loadChildList(const std::string& fname, const std::vector<std::string>& vMaster, std::vector<std::string> &vChild);

        int loadDN(const char* fileName, bool listSB);

        void readConfiguration() throw(FMS_CPF_Exception);

        bool longList_;
        bool listSubiles_;
        std::string m_CpName;
        bool m_bIsSysBC; // the system typw

        std::string curCompFile;

        std::vector<std::string> volumeList;
        std::vector<std::string> compositeFileList;
        std::vector<std::string> subCompositeFileList;

        mapFileId m_volumeNameMap;
        mapFileId m_compositeFileIdMap;
        mapFileId m_subComFileIdMap;

        enum valueString{
                   		maxtime,
                   		maxsize,
                   		activeSubfile,
                   		release,
                   		recordLength,
                   		numReaders,
                   		numWriters,
                   		exclusiveAccess,
                   		size,
                   		transferFilePolicy,
                   		transferBlockPolicy
        };

        std::map<std::string,valueString> s_mapStringValues;

        FMS_CPF_omCmdHandler* m_OmHandler;

        ACS_TRA_trace* cpfIterator;

        // connector object used for infinite file details
        FMS_CPF_Client m_DsdClient;
};

#endif
