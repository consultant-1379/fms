//**********************************************************************
//
// NAME
//      FMS_CPF_BaseAttribute.C
//
// COPYRIGHT Ericsson Telecom AB, Sweden 1999.
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
// See FMS_CPF_BaseAttribute.H
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
//	REV NO		DATE		NAME 		DESCRIPTION
//            99              Ported from UNIX to NT  SIO_I4 - SIO_I7
//							esalves
//
// SEE ALSO
//
//
//**********************************************************************
#include "fms_cpf_baseattribute.h"
#include <ace/ACE.h>

using namespace ACE_OS;

//------------------------------------------------------------------------------
//      Constructor
//------------------------------------------------------------------------------


FMS_CPF_BaseAttribute::FMS_CPF_BaseAttribute () :
refcount_(0)
{
}

//------------------------------------------------------------------------------
//      Destructor
//------------------------------------------------------------------------------

FMS_CPF_BaseAttribute::~FMS_CPF_BaseAttribute ()
{
}


