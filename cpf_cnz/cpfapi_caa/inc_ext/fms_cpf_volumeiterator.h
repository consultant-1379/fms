//******************************************************************************
//
// .NAME
//  	FMS_CPF_VolumeIterator - File handler API
// .LIBRARY 3C++
// .PAGENAME FMS_CPF_VolumeIterator
// .HEADER  AP/FMS Internal
// .LEFT_FOOTER Ericsson Utvecklings AB
// .INCLUDE FMS_CPF_VolumeIterator.H
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
//	The FMS_CPF_VolumeIterator class shall be used by users of
//	CP file system in the Adjunct Processor. This class is an
//	iterator class for the FMS_CPF_File class. An instance of
//	the class gives access to the CP file iterator.
//
//
// .ERROR HANDLING
//
//      All errors are reported by throwing the exception class
//	FMS_CPF_Exception.
//
//
// DOCUMENT NO
//	190 89-CAA 109 0088
//
// AUTHOR
// 	2006-12-21 	  	QMICSAL
//
// CHANGES
//
//	CHANGE HISTORY
//  esalves
//
//
// .SEE ALSO
// 	FMS_CPF_Types.H and FMS_CPF_Exception.H
//
//******************************************************************************

#ifndef FMS_CPF_VOLUMEITERATOR_H
#define FMS_CPF_VOLUMEITERATOR_H

#include <sys/types.h>

#include <list>
#include <string.h>
#include "fms_cpf_types.h"
#include "fms_cpf_exception.h"
//#include "fms_cpf_client.h"

//******************************************************************************
// Class declaration
//******************************************************************************

using namespace std;

class FMS_CPF_Client;		//sio_i10 tjer

class FMS_CPF_VolumeIterator
{
public:

	//------------------------------------
	//Constructor
	//------------------------------------

	FMS_CPF_VolumeIterator (const char* pCPName = "")
	throw (FMS_CPF_Exception);
	//	Description:
	//	Constructs an instance of the FMS_CPF_VolumeIterator class for
	//	iterating over all volumes in CPF.
	//	Parameters:
	// 	None
	//	Exceptions:
	//	SOCKETERROR		Lost connection with the CPF server
	//	INTERNALERROR		Internal program error
	//	Additional information:
	//	None

	//------------------------------------
	//Deconstructor
	//------------------------------------

	~FMS_CPF_VolumeIterator ();
	//	Description:
	//	Destructs the FMS_CPF_VolumeIterator instance.
	//	Additional information:
	//	None

	//------------------------------------
	// Operator
	//------------------------------------

	const char* operator() ()
	throw ();
	//	Description:
	//	Return next volume name.
	//	Parameters:
	// 	None
	//	Return value:
	//	Volume name. If all volumes have been traversed the NULL pointer
	//	is returned.
	//	Exceptions:
	//	None
	//	Additional information:
	//	None

	//------------------------------------
	// Functions
	//------------------------------------

	void reset ()
	throw ();
	//	Description:
	//	Reset the iterator to the state it had immediately after construction.
	//	Parameters:
	// 	None
	//	Return value:
	//	None
	//	Exceptions:
	//	None
	//	Additional information:
	//	None

private:

	bool isCP();

	void readConfiguration(const char* pCPName)
		throw (FMS_CPF_Exception);

	//------------------------------------
	// Variables
	//------------------------------------

//	FMS_CPF_Client	obj; //sio_i10 tjer
	//	char trace[400];

	// RWTPtrSlist<RWCString> list_;
	// RWTPtrSlistIterator<RWCString> iterator_;  //SIO_I11 tjer
	bool m_bIsSysBC; //if the system is BC
	bool m_bCPExists; //if the CP exists
	bool m_bIsConfigRead;

	char m_pCPName[20];
	short m_nCP_ID;
	int m_nNumCP;

	typedef std::list<std::string, std::allocator<std::string> > Slist;

	//Slist is not part of the C++ standard
	Slist list_;
	Slist::iterator iterator_;   //sio_i11 tjer
	std::string retString_;
	std::list<string> m_strListCP;
};
#endif
