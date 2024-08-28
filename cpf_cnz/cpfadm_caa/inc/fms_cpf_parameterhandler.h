/*
 * * @file fms_cpf_parameterhandler.h
 *	@brief
 *	Header file for ParameterHandler class.
 *  This module contains the declaration of the class ParameterHandler.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-07-05
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
 *	| 1.0.0  | 2011-07-05 | qvincon      | File imported.                       |
 *	+========+============+==============+=====================================+
 */
/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef PARAMETERHANDLER_H
#define PARAMETERHANDLER_H

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include <ace/Singleton.h>
#include <ace/Task_T.h>
#include <ace/Synch.h>
#include <ace/RW_Thread_Mutex.h>
#include "fms_cpf_configreader.h"
#include <ACS_APGCC_Collections.H>
#include <acs_apgcc_omhandler.h>
#include <ACS_APGCC_Util.H>
#include <map>
#include <string>
#include <list>

namespace
{
   const std::string CPFDir("cpf");
   const std::string ClusterDir("cluster");
   const std::string DefVolTEMP("TEMPVOLUME");
   const std::string DefVolREL("RELVOLUMSW");
}

class FMS_CPF_ConfigReader;
class ACS_TRA_trace;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class ParameterHandler
{
	typedef std::map<std::string, std::string> maptype2;

 public:
	friend class ACE_Singleton<ParameterHandler, ACE_Recursive_Thread_Mutex>;
	void init(FMS_CPF_ConfigReader* config);

	std::string getCPFroot(const char* cpname = "");
	int getTimeOut();
	std::string getCPFlogDir(const char* cpname = "");

	FMS_CPF_ConfigReader* getConfigReader(){ return m_conf; };
	void deleteInstance();

	std::string& getCPFTQroot();

	const char* getDataDiskRoot() { return rootdir_.c_str(); };

	int getCpTimeSlice() const { return m_CpTimeSlice; };

	void setCpTimeSlice(const int& cpTimeSlice);

	unsigned int getFmsUserId() { return m_FmsUserId; };
	void loadMapDataDiskPathForCP(short& cpId);
	void loadMapLogDiskPathForCP(short& cpId);


 private:
		
	ParameterHandler(int dboCrash = 0);
	void load();
	void loadBCDirectoryStructure(const char *szLogicName, std::list<short> cpIdList, bool log = false);
	void loadMapDataDiskPathForCP(const std::list<short>& cpIdList);
	void loadMapLogDiskPathForCP(const std::list<short>& cpIdList);
	std::string findDataPathForCP(const std::string& cpName);
	std::string findLogPathForCP(const std::string& cpName);

	/**
	 * 	@brief	Load CpTimeSlice From IMM
	*/
	void loadCpTimeSliceFromImm();

	/**
	 * 	@brief	Destructor of ParameterHandler class
	*/
	virtual ~ParameterHandler();

	std::string rootdir_;
	std::string rootlogdir_;
	int timeout_;
	int m_CpTimeSlice;
	bool m_isMultiCP;

	unsigned int m_FmsUserId;

	FMS_CPF_ConfigReader* m_conf;
	ACS_TRA_trace* fms_cpf_ParHdl;

	// contains all data disk path for each CP in
	// a multiple CP system
	maptype2 dataDiskPathForCPMap;
	maptype2 logDiskPathForCPMap;


};
typedef ACE_Singleton<ParameterHandler, ACE_Recursive_Thread_Mutex> ParameterHndl;
#endif
