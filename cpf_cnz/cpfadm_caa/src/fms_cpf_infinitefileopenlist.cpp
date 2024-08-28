/*
 * * @file fms_cpf_infinitefileopenlist.cpp
 *	@brief
 *	Class method implementation for FMS_CPF_InfiniteFileOpenList.
 *
 *  This module contains the implementation of class declared in
 *  the fms_cpf_infinitefileopenlist.h module
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-11-16
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
 *	| 1.0.0  | 2011-11-16| qvincon      | File created.                        |
 *	+========+============+==============+=====================================+
 *	| 1.1.0  | 2011-11-16 | qvincon      | ACE introduced.                     |
 *	+========+============+==============+=====================================+
 */

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */
#include "fms_cpf_infinitefileopenlist.h"
#include "fms_cpf_common.h"

#include <ace/Guard_T.h>

#include <algorithm>

#include "ACS_TRA_trace.h"
#include "ACS_TRA_Logging.h"
extern ACS_TRA_Logging CPF_Log;

/*============================================================================
	ROUTINE: FMS_CPF_InfiniteFileOpenList
 ============================================================================ */
FMS_CPF_InfiniteFileOpenList::FMS_CPF_InfiniteFileOpenList()
{
	fms_cpf_IFOListTrace = new (std::nothrow) ACS_TRA_trace("FMS_CPF_IFOpenList");
}

/*============================================================================
ROUTINE: FMS_CPF_InfiniteFileOpenList
============================================================================ */
FMS_CPF_InfiniteFileOpenList::~FMS_CPF_InfiniteFileOpenList()
{
	if(NULL != fms_cpf_IFOListTrace)
		delete fms_cpf_IFOListTrace;
}

/*============================================================================
	ROUTINE: add
 ============================================================================ */
//HT89089 - Modified for blade cluster
void FMS_CPF_InfiniteFileOpenList::add(const std::string& cpName, const FMS_CPF_FileId& aFileId)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_mutex);

	std::string mapKey = cpName.empty() ? DEFAULT_CPNAME  :  cpName;

	std::list<FMS_CPF_FileId> fileList;
	std::map<std::string,
		std::list<FMS_CPF_FileId> >::iterator mapItr = m_InfiniteFileOpenMap.find(mapKey);

	if (mapItr != m_InfiniteFileOpenMap.end())
	{
		mapItr->second.push_back(aFileId);
	}
	else
	{
		fileList.push_back(aFileId);
		m_InfiniteFileOpenMap.insert(std::pair<std::string, std::list<FMS_CPF_FileId> > (mapKey, fileList));
	}
	TRACE(fms_cpf_IFOListTrace, "add(), CP:<%s> of infinite file <%s> to open list", mapKey.c_str(), aFileId.data());
}


/*============================================================================
	ROUTINE: remove
 ============================================================================ */
//HT89089 - Modified for blade cluster
void FMS_CPF_InfiniteFileOpenList::remove(const std::string& cpName, const FMS_CPF_FileId& aFileId)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_mutex);

	std::string mapKey = cpName.empty() ? DEFAULT_CPNAME  :  cpName;

    std::map<std::string,
    	std::list<FMS_CPF_FileId> >::iterator mapItr = m_InfiniteFileOpenMap.find(mapKey);

    if (mapItr != m_InfiniteFileOpenMap.end())
    {
    	std::list<FMS_CPF_FileId>::iterator listItr;
    	listItr = find(mapItr->second.begin(), mapItr->second.end(), aFileId);
    	if (listItr != mapItr->second.end())
    	{
    		mapItr->second.erase(listItr);
    		TRACE(fms_cpf_IFOListTrace, "remove(), CP:<%s> of infinite "
    					"file<%s> from open list", mapKey.c_str(), aFileId.data());
    	}
    }
}


/*============================================================================
	ROUTINE: removeIfMoreThanOneLeft
 ============================================================================ */
//HT89089 - Modified for blade cluster
bool FMS_CPF_InfiniteFileOpenList::removeIfMoreThanOneLeft(const std::string& cpName, const FMS_CPF_FileId& aFileId)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_mutex);
	bool result = false;
	std::string mapKey = cpName.empty() ? DEFAULT_CPNAME  :  cpName;

	std::map<std::string,
		std::list<FMS_CPF_FileId> >::iterator mapItr = m_InfiniteFileOpenMap.find(mapKey);

	if (mapItr != m_InfiniteFileOpenMap.end())
	{
		if (count(mapItr->second.begin(), mapItr->second.end(), aFileId) > 1)
		{
			remove(mapKey, aFileId);
			result = true;
			TRACE(fms_cpf_IFOListTrace, "removeIfMoreThanOneLeft(), file <%s>, CP:<%s>",
					aFileId.data(), mapKey.c_str());
		}
	}
	return result;
}

