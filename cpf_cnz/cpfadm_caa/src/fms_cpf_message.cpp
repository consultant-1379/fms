/*
 * fms_cpf_message.cpp
 *
 *  Created on: Oct 22, 2011
 *      Author: qvincon
 */

#include "fms_cpf_message.h"


/*=====================================================================
					CLASS CPF_Message_Base
==================================================================== */
CPF_Message_Base::CPF_Message_Base() : ACE_Message_Block()
{
	msg_type(MT_UNDEF);
	m_MessageLife = 10;
}

CPF_Message_Base::CPF_Message_Base(CPF_Message_Type msgType): ACE_Message_Block()
{
    msg_type(msgType);
    m_MessageLife = 10;
}

/*=====================================================================
					CLASS CPF_CreateISF_Msg
==================================================================== */
CPF_CreateISF_Msg::CPF_CreateISF_Msg(const createISFData& subFileData, bool isFirst):
  CPF_Message_Base(MT_CREATE_ISF),
  m_Data(subFileData),
  m_FirstISF(isFirst)
{

}

/*=====================================================================
					CLASS CPF_DeleteISF_Msg
==================================================================== */
CPF_DeleteISF_Msg::CPF_DeleteISF_Msg(const deleteISFData& subFileData):
  CPF_Message_Base(MT_DELETE_ISF),
  m_Data(subFileData),
  m_KnowDN(false),
  m_SubFileDN()
{

}

/*=====================================================================
                                        CLASS CPF_DeleteISF_Msg
==================================================================== */
CPF_DeleteISF_Msg::CPF_DeleteISF_Msg(const std::string subFileDN):
  CPF_Message_Base(MT_DELETE_ISF),
  m_KnowDN(true),
  m_SubFileDN(subFileDN)
{

}

/*=====================================================================
					CLASS CPF_InsertDN_Msg
==================================================================== */
CPF_InsertDN_Msg::CPF_InsertDN_Msg(const infoEntry& data):
  CPF_Message_Base(MT_INSERT_DNENTRY),
  m_Data(data)
{

}

/*=====================================================================
					CLASS CPF_UpdateDN_Msg
==================================================================== */
CPF_UpdateDN_Msg::CPF_UpdateDN_Msg(const infoEntry& data):
  CPF_Message_Base(MT_UPDATE_DNENTRY),
  m_Data(data)
{

}

/*=====================================================================
					CLASS CPF_RemoveDN_Msg
==================================================================== */
CPF_RemoveDN_Msg::CPF_RemoveDN_Msg(const infoEntry& data):
  CPF_Message_Base(MT_REMOVE_DNENTRY),
  m_Data(data)
{

}

/*=====================================================================
					CLASS CPF_RemoveChannel_Msg
==================================================================== */
CPF_RemoveChannel_Msg::CPF_RemoveChannel_Msg( FMS_CPF_CpChannel* cpChannel):
  CPF_Message_Base(MT_REMOVE_CHANNEL),
  m_cpChannel(cpChannel)
{

}

/*=====================================================================
					CLASS CPF_ClearFileRef_Msg
==================================================================== */
CPF_ClearFileRef_Msg::CPF_ClearFileRef_Msg():
  CPF_Message_Base(MT_CLEAR_FILES)
{

}
