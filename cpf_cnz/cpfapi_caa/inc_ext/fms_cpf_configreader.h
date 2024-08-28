//**********************************************************************
//
// NAME
//      fms_cpf_configreader.h
//
// COPYRIGHT Ericsson Telecom AB, Sweden 2009.
// All rights reserved.
//
// The Copyright to the computer program(s) herein is the property of
// Ericsson Telecom AB, Sweden.
// The program(s) may be used and/or copied only with the written
// permission from Ericsson Telecom AB or in accordance with
// the terms and conditions stipulated in the agreement/contract under
// which the program(s) have been supplied.

// DESCRIPTION
//
//
//

// DOCUMENT NO
//

// AUTHOR
//

// REVISION
//
//

// CHANGES
//
//	RELEASE REVISION HISTORY
//
// REV NO   DATE     NAME     DESCRIPTION
//          091028   ewalbra  added cp CLUSTER option support
//					 esalves
// SEE ALSO
//
//
//**********************************************************************
#ifndef FMS_CPF_CONFIGREADER_H
#define FMS_CPF_CONFIGREADER_H

#include <list>
#include <string>
#include <map>
#include "fms_cpf_exception.h"


// The CLUSTER option to the cpf commands is a unique value that is not known
// to the CS service. We define it here for the use of FMS_CPF_ConfigReader.c
// so that it can build a configuration that includes the CLUSTER values
#define CLUSTER_CPid       5001
#define CLUSTER_name       "CLUSTER"

class ACS_TRA_trace;

class FMS_CPF_ConfigReader
{
   typedef std::map<short, std::string> maptype;
public:
   FMS_CPF_ConfigReader();
   ~FMS_CPF_ConfigReader();

   void init() throw (FMS_CPF_Exception);

   bool IsBladeCluster();
   std::list<std::string> getCP_List();
   std::list<short> getCP_IdList();
   int GetNumCP();

   std::string	cs_getDefaultCPName(short id) throw (FMS_CPF_Exception);
   short	cs_getCPID(char* name) throw (FMS_CPF_Exception);

private:
   std::string	cs_getCPName(short id) throw (FMS_CPF_Exception);
	bool cs_isMultipleCPSystem() throw (FMS_CPF_Exception);
	std::list<short> cs_getCPList() throw (FMS_CPF_Exception);

	char m_cFileNamePath[100];

	bool m_bIsBC;
	int m_nNumCP;

	maptype configurationMap; // [key = cpID, value = cpDefaultName]
	ACS_TRA_trace* fms_cpf_configReader_trace;
};

#endif
