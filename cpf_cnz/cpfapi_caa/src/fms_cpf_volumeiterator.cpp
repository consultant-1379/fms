//missing comment

//#ifdef WIN32
//#pragma warning( disable : 4786 )
//#include <strstream>
//#endif

#include "ACS_TRA_trace.h"
//#include "fms_cpf_client.h"
#include "fms_cpf_volumeiterator.h"
#include "fms_cpf_privateexception.h"
#include "fms_cpf_apdcommonfile.h"
#include "fms_cpf_eventhandler.h"
#include "fms_cpf_configreader.h"

#include <string>

#include <ace/ACE.h>

/*** API

using namespace ACE_OS;
using namespace std;

//ACS_TRA_DEF replace of ACS_TRA_trace
//ACS_TRA_trace traceFmsCpfVolumeIterator = ACS_TRA_DEF("fms_cpf_volumeiterator","C400");
//ACS_TRA_trace traceFmsCpfVolumeIterator = new ACS_TRA_trace("fms_cpf_volumeiterator","C400");
string fms_cpf_volumeIter_name = "FMS_CPF_VolumeIterator";
string fms_cpf_volumeIter_format = "C400";
ACS_TRA_trace *fms_cpf_volumeIter_trace = new ACS_TRA_trace(fms_cpf_volumeIter_name, fms_cpf_volumeIter_format);

//------------------------------------------------------------------------------
//Constructor
//------------------------------------------------------------------------------

FMS_CPF_VolumeIterator::FMS_CPF_VolumeIterator (const char* pCPName)
throw (FMS_CPF_Exception)
{
	m_bIsConfigRead = false;
	m_nNumCP = 0;
	ACE_OS::memset(m_pCPName, 0, 20);

//	readConfiguration(pCPName);

	string cpname(m_pCPName);

	m_bCPExists = isCP();

	if (m_bIsSysBC)
	{
		if (cpname.empty())
		{
			throw FMS_CPF_Exception (FMS_CPF_Exception::CPNAMENOPASSED, "CP Name is not passed");
		}

		if (!m_bCPExists)
		{
			throw FMS_CPF_Exception (FMS_CPF_Exception::CPNOTEXISTS, "");
		}
	}


	API ***/

	// to use IMM
	/*
	FMS_Command cmd;
	retString_ = "";
	cmd.cmdCode = LIST_VOLUMES_;

	if (m_bIsSysBC)
		cmd.data[0] = cpname;

	int result = obj.execute(cmd);

	if(result == 0)
	{
		if(cmd.result ==  FMS_CPF_Exception::OK)
		{
			if (fms_cpf_volumeIter_trace->ACS_TRA_ON())
			{
				char trace[400];
				sprintf(trace, "Constructor()\n Command OK");
				ACS_TRA_event(&traceFmsCpfVolumeIterator,trace);
			}

			int noOfVolumes = cmd.data[0];

			for(int i = 1; i <= noOfVolumes; i++)
			{
				string volume = cmd.data[i];
				list_.insert (list_.end(),volume);
			}

			iterator_ = list_.begin();
		}
		else if(cmd.result !=  FMS_CPF_Exception::OK)
		{
			string errorText  = cmd.data[0];
			string detailInfo = cmd.data[1];

			if (fms_cpf_volumeIter_trace->ACS_TRA_ON())
			{
				char trace[400];

				sprintf(trace,	"Constructor()\n Exeption!, Error text = %s, detailed info = %s", errorText.c_str(), detailInfo.c_str() );

				fms_cpf_volumeIter_trace->ACS_TRA_event(1,trace)
			}
			throw FMS_CPF_Exception (FMS_CPF_Exception::errorType(cmd.result), errorText + detailInfo);
		}
	}
	else
	{
		if (fms_cpf_volumeIter_trace->ACS_TRA_ON())
		{
			char trace[400];
			sprintf(trace,	"Constructor()\n Exeption!, Connection broken");
			fms_cpf_volumeIter_trace->ACS_TRA_event(1,trace)
		}
		throw FMS_CPF_Exception (FMS_CPF_Exception::SOCKETERROR, "Connection broken");
	}
	*/


/*** API

}

//------------------------------------------------------------------------------
//Destructor
//------------------------------------------------------------------------------
FMS_CPF_VolumeIterator::~FMS_CPF_VolumeIterator ()
{
	list_.clear();
}
//------------------------------------------------------------------------------
//Get next file entry
//------------------------------------------------------------------------------
const char* FMS_CPF_VolumeIterator::operator() ()
throw ()
{
	static int noOfVolumes = 0;
	static int listSize = 0;
	string retString;

	if((listSize = list_.size()) == 0)
	{
		return "";
	}

	iterator_ = list_.begin();
	advance(iterator_, noOfVolumes);

	if(noOfVolumes < listSize)
	{
		noOfVolumes++;
		retString_ = *iterator_;

		if (fms_cpf_volumeIter_trace->ACS_TRA_ON())
		{
			char trace[400];
			ACE_OS::sprintf(trace,	"operator() ()\n Return = %s", retString_.c_str() );
			fms_cpf_volumeIter_trace->ACS_TRA_event(1,trace);
		}

		return retString_.c_str();
	}
	else
	{
		listSize = 0;
		noOfVolumes = 0;
		return "";
	}
}
//------------------------------------------------------------------------------
//Reset the iterator
//------------------------------------------------------------------------------
void FMS_CPF_VolumeIterator::reset ()
throw ()
{
	iterator_ = list_.begin();
}

//this function return true if the CP in m_pCPName exists otherwise return false
//to be defined
bool FMS_CPF_VolumeIterator::isCP()
{
	bool bRet = false;

	std::list <string>::iterator it = m_strListCP.begin();

	while (it != m_strListCP.end())
	{
		if(((*it).compare(m_pCPName)) == 0)
		{
			bRet = true;
			break;
		}
		it++;
	}

	return bRet;
}


void FMS_CPF_VolumeIterator::readConfiguration(const char* pCPName)
	throw (FMS_CPF_Exception)
{
	FMS_CPF_ConfigReader myReader;
	myReader.init();
	m_bIsSysBC = myReader.IsBladeCluster();

	if (m_bIsSysBC)
	{
		m_strListCP = myReader.getCP_List();
		m_nNumCP = myReader.GetNumCP();

		ACE_OS::memset(m_pCPName, 0, 20);

		if ((pCPName != NULL) && (ACE_OS::strcmp(pCPName, "") != 0))
		{
			m_nCP_ID = myReader.cs_getCPID((char *)pCPName);

			if (m_nCP_ID >= 0)
				ACE_OS::strcpy(m_pCPName, (myReader.cs_getDefaultCPName(m_nCP_ID)).c_str());
			else
			{
				ACE_OS::strcpy(m_pCPName, "");
				if (fms_cpf_volumeIter_trace->ACS_TRA_ON())
				{
					char trace[400];
					ACE_OS::sprintf(trace, "FMS_CPF_VolumeIterator::readConfiguration() - CP_ID value is not correct");
					fms_cpf_volumeIter_trace->ACS_TRA_event(1,trace);
				}

				throw FMS_CPF_Exception (FMS_CPF_Exception::ILLOPTION);

			}
		}
		else
		{
			ACE_OS::strcpy(m_pCPName, "");
			if (fms_cpf_volumeIter_trace->ACS_TRA_ON())
			{
				char trace[400];
				ACE_OS::sprintf(trace, "FMS_CPF_VolumeIterator::readConfiguration() - System is BC but CP Name is NULL");
				fms_cpf_volumeIter_trace->ACS_TRA_event(1,trace);
			}

			throw FMS_CPF_Exception (FMS_CPF_Exception::CPNAMENOPASSED);
		}
	}
	else
	{
		if ((pCPName != NULL) && (ACE_OS::strcmp(pCPName, "") != 0))
		{
			if (fms_cpf_volumeIter_trace->ACS_TRA_ON())
			{
				char trace[400];
				ACE_OS::sprintf(trace, "FMS_CPF_VolumeIterator::readConfiguration() - System is not BC but CP Name is not NULL");
				fms_cpf_volumeIter_trace->ACS_TRA_event(1,trace);
			}
			throw FMS_CPF_Exception (FMS_CPF_Exception::ILLVALUE);
		}

		ACE_OS::strcpy(m_pCPName, "");
	}

	m_bIsConfigRead = true;
}
API ***/
