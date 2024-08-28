/*
 * cpftest.h
 *
 *  Created on: Aug 12, 2011
 *      Author: eivanna
 *
 *  This file contains the functions definitions to call the CPF API
 */



#include "fms_cpf_file.h"
#include "fms_cpf_fileiterator.h"
#include "fms_cpf_exception.h"
#include "fms_cpf_types.h"
#include "fms_cpf_fileid.h"
#include <ACS_CS_API.h>
#include <iostream>
#include <iomanip>

using namespace std;

class CPFTEST{
	public:

		 CPFTEST();
		~CPFTEST();

		string cpName;
		int printMenu();
		void choiceCpName()
		throw (FMS_CPF_Exception);
		void testCreate(string file, string volume);
		void testExist(string  file);
		void testRm(string file);
		void testRename(string file, string newname);
		void testCopy(string file, string newname, FMS_CPF_Types::copyMode copyMode);
		void testIterator();
		void testReserve(string file);




};
