//
/** @file fms_cpf_common.h
 *	@brief
 *	Header file for for common utilities for CPF.
 *
 *
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
 *	| 1.0.0  | 2011-06-08 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */

#ifndef FMS_CPF_COMMON_H_
#define FMS_CPF_COMMON_H_
#include <ace/ACE.h>

#include <new>
class ACS_TRA_trace;

#include "ACS_CS_API.h"

#include <boost/filesystem.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include "ACS_APGCC_CommonLib.h"

#include "fms_cpf_privateexception.h"

#define BOOST_FILESYSTEM_VERSION 3

// To avoid warning about unused parameter
#define UNUSED(expr) do { (void)(expr); } while (0)

#define TRACE(TRACE_CLASS, FMT, ...) \
		trautil_trace(TRACE_CLASS, FMT, __VA_ARGS__);

namespace {
	const int SUCCESS = 0;
	const int FAILURE = -1;
	const int INVALID = -1;
	const int s_MaxPathLength = 256;
	const char DirDelim ='/';
	const char SubFileSep ='-';
	const char DEFAULT_CPNAME[] = "default_cpname";
	const char FMS_CPF_DAEMON_NAME[] = "fms_cpfd";
	const char FMS_CPF_DAEMON_USER[] = "root";
	const char FMS_CPF_LOG_APPENDER_NAME[] = "FMS_CPFD";
	const char CPF_SERVER_LOCKFILE[] = "/var/run/ap/fms_cpfd.lck";
	const char logExt[] = ".log";
};

namespace errorText {

	const int errorValue = 1;
	const char attributeChange[] = "Not allowed operation. Unchangeable attribute: %1% of %2%=%3%";
	const char volumeDelete[] = "Not allowed operation. Volume is not empty";
	const char illegalAttributeSet[] = "Attribute %1%=%2% of %3%=%4% not settable in this system configuration";
	const char invalidAttribute[] = "Unreasonable value: %1%=%2% of %3%=%4%. %5%";
	const char invalidFileName[] = "Unreasonable value: %1%=%2%. Invalid file name";
	const char invalidVolumeName[] = "Unreasonable value: %1%=%2%. Invalid volume name";

	const char FileAttachedToTQ[] = "File is connected to a transfer queue";
	const char EmptyFolder[]= " is an empty directory";
	const char isAFolder[]= " is a directory";
	const char ModeNotAllowed[] = "mode not allowed for this source type";
	const char AlredyExist[] = " already exists";
	const char NeedDirectory[] = " must be a directory";
	const char SpaceOnSpace[] = " on ";
	const char sourceFail[] = "failed to open ";
	const char unzipFail[] = "failed to extract archive ";
	const char zipFail[] = "failed to create archive ";
	const char zipFailEmptySource[] = "Composite file is empty";
	const char zipFailPathError[] = "archive path not found";
	const char exportModeError[] = "mode CLEAR not allowed";
};

namespace stdValue{
		const char VOL_IDENTIFIER[] = "_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		const char IDENTIFIER[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		const char minStartChr = 'A';
		const char maxStartChr = 'Z';
		const unsigned int MINVOL_LENGTH = 1U;
		const unsigned int VOL_LENGTH = 15U;
		const unsigned int FILE_LENGTH = 12U;
		const unsigned int SUB_LENGTH = 12U;
		const unsigned int GEN_LENGTH = 8U;
		const int MIN_RECORDLENGTH = 1U;
		const int MAX_RECORDLENGTH = 65535U;

		const int CP_TIME_SLICE_DEFAULT = 10000U;
		const int MIN_CP_TIME_SLICE = 500U;
		const int MAX_CP_TIME_SLICE = 60000U;

		const unsigned int SecondsInMinute = 60U;
		const long int MilliSecondsInSecond = 1000U;
		const long int MicroSecondsInMilliSecond = 1000U;
		const long int NanoSecondsInMilliSecond = 1000000U;

		const int MAX_IMM_RETRY = 10U;
		const unsigned int BASE_MICROSEC_SLEEP = 100000U;
};

namespace OHI_USERSPACE{
	const char SUBSYS[] = "FMS";
	const char APPNAME[] = "CPF";
	const char eventText[]= "";
}

namespace parseSymbol{
		const char minus = '-';
		const char plus = '+';
		const char comma = ',';
		const char equal = '=';
		const char underLine = '_';
		const char dot = '.';
		const char atSign = ':';
};

namespace IMM_ErrorCode
{
	const int TRYAGAIN = -6;
	const int BAD_HANDLE = -9;
	const int NOT_EXIST = -12;
	const int EXIST = -14;
}

namespace actionResult{

		const int SUCCESS = 1; // SA_AIS_OK
		const int FAILED = 21; // SA_AIS_ERR_FAILED_OPERATION
		const int NOOPERATION = 28; // SA_AIS_ERR_NO_OP
};

namespace exitCodePar{
		extern char errorCode[];
		extern char errorMessage[];
		extern char errorComCliMessage[];
}

namespace cpf_imm{

		const char parentRoot[] = "AxeCpFileSystemcpFileSystemMId=1";

		const char rootClassName[] = "AxeCpFileSystemCpFileSystemM";
		const char VolumeClassName[] = "AxeCpFileSystemCpVolume";
		const char InfiniteFileClassName[] = "AxeCpFileSystemInfiniteFile";
		const char InfiniteSubFileClassName[]= "AxeCpFileSystemInfiniteSubFile";
		const char CompositeFileClassName[]= "AxeCpFileSystemCompositeFile";
		const char CompositeSubFileClassName[]= "AxeCpFileSystemCompositeSubFile";
		const char SimpleFileClassName[]= "AxeCpFileSystemSimpleFile";

		const char VolumeObjectName[] = "CpVolume";
		const char InfiniteFileObjectName[] = "InfiniteFile";
		const char InfiniteSubFileObjectName[]= "InfiniteSubFile";
		const char CompositeFileObjectName[]= "CompositeFile";
		const char CompositeSubFileObjectName[]= "CompositeSubFile";
		const char SimpleFileObjectName[]= "SimpleFile";

		extern char VolumeKey[];
		extern char InfiniteFileKey[];
		extern char InfiniteSubFileKey[];
		extern char CompositeFileKey[];
		extern char CompositeSubFileKey[];
		extern char SimpleFileKey[];

		// Configurable hidden attribute
		extern char blockTransferTimeSliceAttribute[];

		// Configurable common attributes
		extern char recordLengthAttribute[];
                 //HY46076 
                // Configurable composite file attributes
                extern char deleteFileTimerAttribute[];

		// Configurable cp Infinite file attributes
		extern char maxSizeAttribute[];
		extern char maxTimeAttribute[];
		extern char releaseCondAttribute[];
		extern char fileTQAttribute[];
		extern char blockTQAttribute[];

		// Read-only attributes
		extern char lastSubFileSentAttribute[];
		extern char activeSubFileAttribute[];
		extern char sizeAttribute[];
		extern char numReadersAttribute[];
		extern char numWritersAttribute[];
		extern char exclusiveAccessAttribute[];
		extern char transferQueueDnAttribute[];

		extern char activeSubfileNumReadersAttribute[];
		extern char activeSubfileNumWritersAttribute[];
		extern char activeSubfilExclusiveAccessAttribute[];
		extern char activeSubfileSizeAttribute[];



};

namespace BRInfo{

		const char PSAClearFilePath[] = "/usr/share/pso/storage-paths/clear";
		const char CpfBRFolder[] = "fms-cpfbin";

		const char backupParent[] = "brfParticipantContainerId=1";
		const char backupImmClass[] = "BrfPersistentDataOwner";
		const char cpfBackupRDN[] = "ERIC-APG-FMS-CPF";
		const char cpfBackupDN[] = "brfPersistentDataOwnerId=ERIC-APG-FMS-CPF,brfParticipantContainerId=1";
		const int  reportActionResult = 22;

		extern int brfSuccess;
		extern char backupVersion[];
		extern char backupKey[];
		extern char versionAttribute[];
		extern char backupTypeAttribute[];
		extern char rebootAttribute[];
		extern char requestIdParameter[];
		extern char resulCodeParameter[];
		extern char messageParameter[];
}

void trautil_trace(ACS_TRA_trace* trace_class, const ACE_TCHAR* messageFormat, ...);

// HW79785 BEGIN

bool checkHWVersionInfo();
bool checkNodeArch();
bool checkFileSize(const std::string& path) throw (FMS_CPF_PrivateException);

// HW79785 END

#endif /* FMS_CPF_COMMON_H_ */
