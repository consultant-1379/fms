//
/** @file fms_cpf_common.cpp
 *	@brief
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-06-08
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
 *	| 1.0.0  | 2011-06-15 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include "fms_cpf_common.h"
#include "ACS_TRA_trace.h"

extern ACS_TRA_Logging CPF_Log;

namespace exitCodePar{
		char errorCode[] = "errorCode";
		char errorMessage[] = "errorMessage";
		char errorComCliMessage[] = "errorComCliMessage";
};

namespace cpf_imm{

		char VolumeKey[] = "cpVolumeId";
		char InfiniteFileKey[] = "infiniteFileId";
		char InfiniteSubFileKey[] = "infiniteSubFileId";
		char CompositeFileKey[] = "compositeFileId";
		char CompositeSubFileKey[] = "compositeSubFileId";
		char SimpleFileKey[] = "simpleFileId";

		char blockTransferTimeSliceAttribute[] = "blockTransferTimeSlice";

		char recordLengthAttribute[] = "recordLength";
                //HY46076
                char deleteFileTimerAttribute[] = "deleteFileTimer"; 
		char maxSizeAttribute[] = "maxSize";
		char maxTimeAttribute[] = "maxTime";
		char releaseCondAttribute[] = "overrideSubfileClosure";
		char fileTQAttribute[] = "transferQueue";

		char blockTQAttribute[] = "transferBlockPolicy";

		// Read-only attributes
		char lastSubFileSentAttribute[] = "lastSubfileSent";
		char activeSubFileAttribute[] = "activeSubFile";
		char sizeAttribute[] = "size";
		char numReadersAttribute[] = "numOfReaders";
		char numWritersAttribute[] = "numOfWriters";
		char exclusiveAccessAttribute[] = "exclusiveAccess";
		char transferQueueDnAttribute[] = "transferQueueDn";

		char activeSubfileNumReadersAttribute[] = "activeSubfileNumOfReaders";
		char activeSubfileNumWritersAttribute[]= "activeSubfileNumOfWriters";
		char activeSubfilExclusiveAccessAttribute[] = "activeSubfileExclusiveAccess";
		char activeSubfileSizeAttribute[] = "activeSubfileSize";
}

namespace BRInfo{

		char backupKey[] = "brfPersistentDataOwnerId";
		char versionAttribute[] = "version";
		char backupTypeAttribute[] = "backupType";
		char rebootAttribute[] = "rebootAfterRestore";
		char requestIdParameter[] = "requestId";
		char resulCodeParameter[] = "resultCode";
		char messageParameter[] = "message";

		int brfSuccess = 0;
		char backupVersion[] = "1.0";
}

namespace fileSize {
        const long long int MaxByteSizeLimit = 100L*1024L*1024L; //100 MB 
}

namespace fs = boost::filesystem;

/*============================================================================
	ROUTINE: trautil_trace
 ============================================================================ */

void trautil_trace(ACS_TRA_trace* trace_class, const ACE_TCHAR* messageFormat, ...)
{
	const ACE_UINT32 TRACE_BUF_SIZE = 1024;

	if( (NULL != trace_class) && (trace_class->isOn()) )
	{
		if( messageFormat && *messageFormat )
		{
			va_list params;
			va_start(params, messageFormat);
			ACE_TCHAR traceBuffer[TRACE_BUF_SIZE]={'\0'};
			ACE_OS::vsnprintf(traceBuffer, TRACE_BUF_SIZE - 1, messageFormat, params);
			trace_class->trace(traceBuffer);
			va_end(params);
		}
	}
}
/* HW79785 BEGIN
 * 
 */
/*============================================================================
	ROUTINE: checkHWVersionInfo
 ============================================================================ */

bool checkHWVersionInfo()
{
	char errorText[512] = {0};
	snprintf(errorText, 511, "%s() begin", __func__ );
	CPF_Log.Write(errorText, LOG_LEVEL_TRACE);

	bool result=false;
	ACS_APGCC_CommonLib acsApgccCommonLib;
	ACS_APGCC_HWINFO hwInfo;
	ACS_APGCC_HWINFO_RESULT hwInfoResult;

	acsApgccCommonLib.GetHwInfo(&hwInfo, &hwInfoResult, ACS_APGCC_GET_HWVERSION);
	result = (ACS_APGCC_HWINFO_SUCCESS == hwInfoResult.hwVersionResult);
	snprintf(errorText, 511, "%s(): fetched HW version is %d", __func__ ,hwInfoResult.hwVersionResult);
	CPF_Log.Write(errorText, LOG_LEVEL_WARN);

	if(result)
	{
		if (hwInfo.hwVersion == ACS_APGCC_HWVER_GEP5)
		{
			result = true;
			char errorText[512] = {0};
			snprintf(errorText, 511, "checkHWVersionInfo():HW version is GEP5");
			CPF_Log.Write(errorText, LOG_LEVEL_WARN);
		}
		else
			result = false;
	}
	else
	{  
		snprintf(errorText, 511, "%s(): Error on getting HW info", __func__ );
		CPF_Log.Write(errorText, LOG_LEVEL_ERROR);
	}

	snprintf(errorText, 511, "%s() Leaving", __func__ );
	CPF_Log.Write(errorText, LOG_LEVEL_TRACE);
	return result;
}

/*============================================================================
	ROUTINE: checkNodeArch
 ============================================================================ */

bool checkNodeArch()
{
    char errorText[512] = {0};
    bool result = false;
	
    snprintf(errorText, 511, "%s() begin", __func__ );
    CPF_Log.Write(errorText, LOG_LEVEL_TRACE);
    
    ACS_CS_API_CommonBasedArchitecture::ArchitectureValue nodeArchitecture;
    ACS_CS_API_NS::CS_API_Result returnValue = ACS_CS_API_NetworkElement::getNodeArchitecture(nodeArchitecture);
    
    if(returnValue == ACS_CS_API_NS::Result_Success)
    {
       snprintf(errorText, 511, "checkNodeArch() : node architecture is <%d>", nodeArchitecture);
       CPF_Log.Write(errorText, LOG_LEVEL_WARN);
        
       if ( nodeArchitecture == ACS_CS_API_CommonBasedArchitecture::SCB || ACS_CS_API_CommonBasedArchitecture::SCX)
          {
             result = true;
          }
    }
    else
    {
	   snprintf(errorText, 511, "%s(): Error on getting node architecture", __func__ );
	   CPF_Log.Write(errorText, LOG_LEVEL_ERROR);
    }
        
    snprintf(errorText, 511, "%s() Leaving", __func__ );
    CPF_Log.Write(errorText, LOG_LEVEL_TRACE);
    return result;
}

/*============================================================================*/
/* ROUTINE checkFileSize                                                      */
/*============================================================================*/

bool checkFileSize(const std::string& path) throw (FMS_CPF_PrivateException)
{
    bool result = false;
    fs::path filePath(path);
    boost::intmax_t sizeBytes = 0;
    char errorText[512] = {0};

    snprintf(errorText, 511, "%s() begin", __func__ );
    CPF_Log.Write(errorText, LOG_LEVEL_TRACE);
    	
    if(fs::exists(filePath))
    {
       if(fs::is_regular_file(filePath))
       {
          sizeBytes= fs::file_size(filePath);
          snprintf(errorText, 511, "checkFileSize (), File size is calculated::<%lld>" , static_cast < long long int> (sizeBytes));
          CPF_Log.Write(errorText, LOG_LEVEL_WARN);
       }
       else
       {    	   
     	  snprintf(errorText, 511, "%s(): Error on checking regular File", __func__ );
     	  CPF_Log.Write(errorText, LOG_LEVEL_ERROR);
          std::string errorDetail(strerror_r(errno, errorText, 511));
          throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail);
       }
     }
     else
     {
        std::string zipFile = path + ".zip";
        fs::path filePath1(zipFile);
        if(fs::exists(filePath1))
        {
           sizeBytes= fs::file_size(filePath1);  
           snprintf(errorText, 511, "checkFileSize (), zip File size is calculated::<%lld>" , static_cast < long long int> (sizeBytes));
           CPF_Log.Write(errorText, LOG_LEVEL_WARN);
        }
        else
        {
           zipFile = path + ".ZIP";
           fs::path filePath2(zipFile);
           if(fs::exists(filePath2))
           {
              sizeBytes= fs::file_size(filePath2);
              snprintf(errorText, 511, "checkFileSize (), ZIP File size is calculated::<%lld>" , static_cast < long long int> (sizeBytes));
              CPF_Log.Write(errorText, LOG_LEVEL_WARN);
           }
           else
           {       
     	      snprintf(errorText, 511, "%s(): Error on checking zip File", __func__ );
     	      CPF_Log.Write(errorText, LOG_LEVEL_ERROR);
     	      std::string errorDetail(strerror_r(errno, errorText, 511));
              throw FMS_CPF_PrivateException(FMS_CPF_PrivateException::PHYSICALERROR, errorDetail); 
           }    
        }
     }

     if  ( sizeBytes >= fileSize::MaxByteSizeLimit )
         result = true;
       
     snprintf(errorText, 511, "%s() Leaving", __func__ );
     CPF_Log.Write(errorText, LOG_LEVEL_TRACE);
     
     return result;
}
/* HW79785 END
 * 
 */
