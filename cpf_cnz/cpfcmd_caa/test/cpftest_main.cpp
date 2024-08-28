/*
 * cpftest.cpp
 *
 *  Created on: Aug 11, 2011
 *      Author: eivanna (first version)
 *  This file contains the main method of the TEST command program
 *
 */

#include <iostream>
#include "cpftest.h"
#include "fms_cpf_types.h"
#include "ACS_APGCC_Util.H"

class FMS_CPF_Types;

int main(){

	int choice = -1;
	int copyMode = 0;
	string objectTest ;
	string name;
	string volume;
	CPFTEST cpftest;


	cout<<"*-----------------------------------------------------------------*\n"
		<<"*                                                                 *\n"
		<<"*                      RUNTIME COMMAND DEMO                       *\n"
		<<"*                                                                 *\n"
		<<"*-----------------------------------------------------------------*\n"
		<<endl;


	while(choice!=0){
	  try {
		choice = cpftest.printMenu();

		switch(choice){
			case 0:{
				cout << "Goodbye" << endl;
				break;
			}
			case 1: {

				cout << endl;
				cout << "Insert the name of the file to create" << endl;
				cin >> name;
				ACS_APGCC::toUpper(name);
				cout << "Insert the volume name" << endl;
				cin >> volume;
				ACS_APGCC::toUpper(volume);
				cpftest.choiceCpName();
				cpftest.testCreate(name, volume);
				break;
			}
			case 2: {
				cout << endl;
				cout << "Insert the file name to test" << endl;
				cin >> objectTest;
				ACS_APGCC::toUpper(objectTest);
				cpftest.choiceCpName();
				cpftest.testExist(objectTest);
				break;
			}
			case 3: {
				cout << endl;
				cout << "Insert the file name to remove" << endl;
				cin >> objectTest;
				ACS_APGCC::toUpper(objectTest);
				cpftest.choiceCpName();
				cpftest.testRm(objectTest);
				break;
			}
			case 4: {
				cout << endl;
				cout << "Insert the file to rename" << endl;
				cin >> objectTest;
				ACS_APGCC::toUpper(objectTest);
				cout << "Insert the new name" << endl;
				cin >> name;
				ACS_APGCC::toUpper(name);
				cpftest.choiceCpName();
				cpftest.testRename(objectTest, name);
				break;
			}
			case 5: {
				cout << endl;
				cout << "Insert the file to copy" << endl;
				cin >> objectTest;
				ACS_APGCC::toUpper(objectTest);
				cout << "Insert the new file to copy " << endl;
				cin >> name;
				ACS_APGCC::toUpper(name);
				cout << "Insert the copy modality (0=OVERWRITE, 1=APPEND, 2=CLEAR, 3=NORMAL)" << endl;

				cin >> copyMode;
				if ((copyMode < 0) || (copyMode > 3))
				{
					cout << endl;
					cout << "Attention! Incorrect value" << endl;
					break;
				}
				cpftest.choiceCpName();
				cpftest.testCopy(objectTest, name, (FMS_CPF_Types::copyMode) copyMode);
				break;
			}
			case 6: {
				cout << endl;
				cout << "It shows all the files present in the CP File System:" << endl;
				cpftest.testIterator();
				break;
			}
			case 7: {
					cout << endl;
					cout << "Insert the file to reserve" << endl;
					cin >> objectTest;
					ACS_APGCC::toUpper(objectTest);
					cpftest.choiceCpName();
					cpftest.testReserve(objectTest);
					break;
					}

			default:
				cout << endl;
				cout << "Attention! Incorrect value" << endl;
				break;

	    }

	 } catch(FMS_CPF_Exception& ex)
	 {
		std::string slogan;
		ex.getSlogan(slogan);
		cout << slogan << endl;
		return (int)ex.errorCode();
	 }

	}


	return 0;
}
