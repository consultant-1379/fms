/*
 * * @file fms_cpf_message.h
 *	@brief
 *	Header file for FMS_CPF_Message class.
 *  This module contains the declaration of the class FMS_CPF_Message.
 *
 *	@author qvincon (Vincenzo Conforti)
 *	@date 2011-10-22
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
 *	| 1.0.0  | 2011-10-22 | qvincon      | File created.                       |
 *	+========+============+==============+=====================================+
 */
/*=====================================================================
						DIRECTIVE DECLARATION SECTION
==================================================================== */
#ifndef FMS_CPF_MESSAGE_H_
#define FMS_CPF_MESSAGE_H_

/*===================================================================
                        INCLUDE DECLARATION SECTION
=================================================================== */

#include <ace/Message_Block.h>
#include <string>

class FMS_CPF_CpChannel;

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_Message_Base : public ACE_Message_Block
{
  public:

	enum
	{
		MT_EXIT = 0x201,
		MT_CREATE_ISF,
		MT_DELETE_ISF,
		MT_INSERT_DNENTRY,
		MT_UPDATE_DNENTRY,
		MT_REMOVE_DNENTRY,
		MT_CLEAR_DNMAP,
		MT_REMOVE_CHANNEL,
		MT_SYNC,
		MT_CLEAR_FILES,
		MT_SEND_BLOCK,
		MT_UPDATE_BLOCK,
		MT_REMOVE_BLOCK,
		MT_UNDEF,
	};

	typedef int CPF_Message_Type;

	CPF_Message_Base();
	CPF_Message_Base(CPF_Message_Type msgType);

	int getMessageLife() const { return m_MessageLife; };
	int decreaseMessageLife() { return (--m_MessageLife); };

	virtual ~CPF_Message_Base(){ };

  private:

	int m_MessageLife;

	// Disallow copying and assignment.
	CPF_Message_Base(const CPF_Message_Base&);
	void operator= (const CPF_Message_Base&);
};

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_Close_Msg : public CPF_Message_Base
{
  public:

	CPF_Close_Msg() : CPF_Message_Base(MT_EXIT){};

	virtual ~CPF_Close_Msg(){};

  private:

	// Disallow copying and assignment.
	CPF_Close_Msg(const CPF_Close_Msg&);
	void operator= (const CPF_Close_Msg&);

};
/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_CreateISF_Msg: public CPF_Message_Base
{
  public:

	struct createISFData{
				std::string infiniteFileName;
				unsigned int subFileValue;
				std::string volumeName;
				std::string cpName;
	};

	CPF_CreateISF_Msg(const createISFData& subFileData, bool isFirst = false);

	virtual ~CPF_CreateISF_Msg(){};

	inline void getCreationData(createISFData& subFileData){subFileData = m_Data;};

	inline bool isFirstISF() { return m_FirstISF; };

  private:

	createISFData m_Data;

	bool m_FirstISF;

	// Disallow copying and assignment.
	CPF_CreateISF_Msg(const CPF_CreateISF_Msg&);
	void operator= (const CPF_CreateISF_Msg&);
};

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_DeleteISF_Msg: public CPF_Message_Base
{
  public:

	struct deleteISFData
	{
				std::string infiniteFileName;
				unsigned int subFileValue;
				std::string volumeName;
				std::string cpName;
	};

	CPF_DeleteISF_Msg(const deleteISFData& subFileData);

	CPF_DeleteISF_Msg(const std::string subFileDN);

	inline bool isKnowDNofISF() const { return m_KnowDN; };

	inline std::string getISFDN() const { return m_SubFileDN; };

	virtual ~CPF_DeleteISF_Msg(){};

	inline void getCreationData(deleteISFData& subFileData){subFileData = m_Data;};

  private:

	deleteISFData m_Data;

	bool m_KnowDN;

	std::string m_SubFileDN;

	// Disallow copying and assignment.
	CPF_DeleteISF_Msg(const CPF_DeleteISF_Msg&);
	void operator= (const CPF_DeleteISF_Msg&);
};

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_InsertDN_Msg: public CPF_Message_Base
{
  public:

	struct infoEntry{
				std::string fileName;
				std::string cpName;
				std::string fileDN;
	};

	CPF_InsertDN_Msg(const infoEntry& data);

	virtual ~CPF_InsertDN_Msg(){};

	inline void getData(infoEntry& entryData){entryData = m_Data;};

  private:

	infoEntry m_Data;

	// Disallow copying and assignment.
	CPF_InsertDN_Msg(const CPF_InsertDN_Msg&);
	void operator= (const CPF_InsertDN_Msg&);
};
/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_UpdateDN_Msg: public CPF_Message_Base
{
  public:

	struct infoEntry{
				std::string fileName;
				std::string newFilename;
				std::string cpName;
				std::string fileDN;
	};

	CPF_UpdateDN_Msg(const infoEntry& data);

	virtual ~CPF_UpdateDN_Msg(){};

	inline void getData(infoEntry& entryData){entryData = m_Data;};

  private:

	infoEntry m_Data;

	// Disallow copying and assignment.
	CPF_UpdateDN_Msg(const CPF_UpdateDN_Msg&);
	void operator= (const CPF_UpdateDN_Msg&);
};

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_RemoveDN_Msg: public CPF_Message_Base
{
  public:

	struct infoEntry{
				std::string fileName;
				std::string cpName;
	};

	CPF_RemoveDN_Msg(const infoEntry& data);

	virtual ~CPF_RemoveDN_Msg(){};

	inline void getData(infoEntry& entryData){entryData = m_Data;};

  private:

	infoEntry m_Data;

	// Disallow copying and assignment.
	CPF_RemoveDN_Msg(const CPF_RemoveDN_Msg&);
	void operator= (const CPF_RemoveDN_Msg&);
};

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_ClearDNMap_Msg: public CPF_Message_Base
{
  public:

	CPF_ClearDNMap_Msg() : CPF_Message_Base(MT_CLEAR_DNMAP){};

	virtual ~CPF_ClearDNMap_Msg(){};

  private:

	// Disallow copying and assignment.
	CPF_ClearDNMap_Msg(const CPF_ClearDNMap_Msg&);
	void operator= (const CPF_ClearDNMap_Msg&);
};

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_RemoveChannel_Msg: public CPF_Message_Base
{
  public:

	CPF_RemoveChannel_Msg( FMS_CPF_CpChannel* cpChannel );

	virtual ~CPF_RemoveChannel_Msg(){};

	inline FMS_CPF_CpChannel* getCpChannel() const { return m_cpChannel; };

  private:

	FMS_CPF_CpChannel* m_cpChannel;

	// Disallow copying and assignment.
	CPF_RemoveChannel_Msg(const CPF_RemoveChannel_Msg&);
	void operator= (const CPF_RemoveChannel_Msg&);
};

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_Sync_Msg: public CPF_Message_Base
{
  public:

	CPF_Sync_Msg() : CPF_Message_Base(MT_SYNC){};

	virtual ~CPF_Sync_Msg(){};

  private:

 	// Disallow copying and assignment.
	CPF_Sync_Msg(const CPF_Sync_Msg&);
 	void operator= (const CPF_Sync_Msg&);

};

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_ClearFileRef_Msg: public CPF_Message_Base
{
  public:

	CPF_ClearFileRef_Msg();

	virtual ~CPF_ClearFileRef_Msg(){};

  private:

	// Disallow copying and assignment.
	CPF_ClearFileRef_Msg(const CPF_ClearFileRef_Msg&);
	void operator= (const CPF_ClearFileRef_Msg&);

};

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_BlockSend_Msg: public CPF_Message_Base
{
  public:

	CPF_BlockSend_Msg(const std::string& cpName)
	: CPF_Message_Base(MT_SEND_BLOCK),
	  m_CpName(cpName)
	{};

	virtual ~CPF_BlockSend_Msg(){};

	inline const char* getCpName() const { return m_CpName.c_str(); };

  private:

	std::string m_CpName;

	// Disallow copying and assignment.
	CPF_BlockSend_Msg(const CPF_BlockSend_Msg&);
	void operator= (const CPF_BlockSend_Msg&);
};

/*=====================================================================
					CLASS DECLARATION SECTION
==================================================================== */
class CPF_BlockUpdate_Msg: public CPF_Message_Base
{
  public:

	CPF_BlockUpdate_Msg()
	: CPF_Message_Base(MT_UPDATE_BLOCK)
	{};

	virtual ~CPF_BlockUpdate_Msg(){};

  private:

	// Disallow copying and assignment.
	CPF_BlockUpdate_Msg(const CPF_BlockUpdate_Msg&);
	void operator= (const CPF_BlockUpdate_Msg&);
};

#endif /* FMS_CPF_MESSAGE_H_ */
