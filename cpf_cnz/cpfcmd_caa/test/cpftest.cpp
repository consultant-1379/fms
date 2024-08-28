/*
 * cpftest.cpp
 *
 *  Created on: Aug 12, 2011
 *      Author: eivanna
 *  This file contains the functions implementation to call the CPF API
 */

#include "cpftest.h"


/** Function implementation **/

CPFTEST::CPFTEST(){}
CPFTEST::~CPFTEST(){}

int CPFTEST::printMenu(){

	int val = 0;

	cout << endl;
	cout <<"Select a value: "
		<<"\n*----------------------------------------------*"
		<<"\n*\t Value\t\t Action Type           *"
		<<"\n*----------------------------------------------*"
		<<"\n\t 0.\t"<< "\t\t INSERT 0 TO TERMINATE"
		<<"\n\t 1.\t"<< "\t\t Test function create(Composite Regular file)"
		<<"\n\t 2.\t"<< "\t\t Test function exist"
		<<"\n\t 3.\t"<< "\t\t Test function remove"
		<<"\n\t 4.\t"<< "\t\t Test function rename"
		<<"\n\t 5.\t"<< "\t\t Test function copy"
		<<"\n\t 6.\t"<< "\t\t Test function iterator"
		<<"\n\t 7.\t"<< "\t\t Test function reserve"
		<< endl;
	cin >> val;
	return val;

}

void CPFTEST::choiceCpName()
throw (FMS_CPF_Exception)
{
	string singleCP;
	bool multipleCpSystem;
	cpName="";

	//calling to isMultipleCpSystem method
	ACS_CS_API_NS::CS_API_Result returnValue =	ACS_CS_API_NetworkElement::isMultipleCPSystem(multipleCpSystem);
	//check return value
	if(returnValue != ACS_CS_API_NS::Result_Success)
	{//error
		switch(returnValue)
		{
			case ACS_CS_API_NS::Result_NoEntry:
				{
					throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOENTRY, "");
				}
				break;
			case ACS_CS_API_NS::Result_NoValue:
				{
					throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOVALUE, "");
				}
				break;
			case ACS_CS_API_NS::Result_NoAccess:
				{
					throw FMS_CPF_Exception (FMS_CPF_Exception::CSNOACCESS, "");
				}
				break;
			case ACS_CS_API_NS::Result_Failure:
				{
					throw FMS_CPF_Exception (FMS_CPF_Exception::CSOTHERFAILURE, "");
				}
				break;

			default:
				throw FMS_CPF_Exception (FMS_CPF_Exception::CSOTHERFAILURE, "");
				break;

		}


		if (multipleCpSystem){
			cout << "Insert the CP name: " << endl;
			cin >> cpName;
		}
		else{
			cpName="";
		}
    }
}

void CPFTEST::testCreate(string fileName, string volume){

	try{
		FMS_CPF_File file (fileName.c_str(), cpName.c_str());
		if(file.exists()){
			cout << "The file already exist" << endl;
			//testIterator(file);
		}
		else{
			//Filling in the attribute structure
			FMS_CPF_Types::fileAttributes attributes;
			attributes.ftype = FMS_CPF_Types::ft_REGULAR;
			// we set some default values
			attributes.regular.rlength = 1024;
			attributes.regular.composite = true;
			//Create the file
			file.create (attributes, volume.c_str(), FMS_CPF_Types::NONE_, false);
			//Unreserve the file because it has been automatically reserved
			//if it is not going to be used any further
			file.unreserve();
			cout << "The file " << fileName << " has been created in "
			<< volume << endl;

		}

	} catch(FMS_CPF_Exception &e)
	{
		cout <<"Exception on create function: " << e.errorCode()<<" , "
		<< e.errorText() << endl;
	}

}


void CPFTEST::testExist(string file){

	try{
		FMS_CPF_File fileTest (file.c_str(), cpName.c_str());
		if(fileTest.exists()){
			cout << "The file exist" << endl;
		}
		else{
			cout << "The file: " << file.c_str() << " doesn't exist"
			<< endl;
		}

	} catch(FMS_CPF_Exception &e)
	{
		cout <<"Exception on exists function: " << e.errorCode()<<" , "
		<< e.errorText() << endl;
	}

}

void CPFTEST::testRm(string file){

	string answer = "";
	try{

		FMS_CPF_File fileTest (file.c_str(), cpName.c_str());
		FMS_CPF_FileId subFile(file);
		// it is a main file
		cout <<  "The file " << file << " must be removed" << endl;
		if(subFile.subfile().empty()){

			cout << "Do you want to remove also the subfiles? (y or n)"
			<< endl;
			cin >> answer;

			if(answer == "y") {
				fileTest.deleteFile(true);
				cout << "The file and sub-files have been removed" << endl;
			}
			else if(answer == "n"){
				fileTest.deleteFile(false);
				cout << "The file has been removed" << endl;
			}
			else
				cout << "Incorrect Usage " << endl;

		}
		else{
			//it is a sub-file
			fileTest.deleteFile(false);
			cout << "The sub-file has been removed" << endl;

		}

	} catch(FMS_CPF_Exception &e )
	{
		cout <<  "Exception on delete function: " << e.errorCode() << ", "
		<< e.errorText() << endl;
	}
}


void CPFTEST::testRename(string file, string newname){

	try{

		FMS_CPF_File fileTest (file.c_str(), cpName.c_str());
		fileTest.rename(newname.c_str());
		cout << "The file " << file << " has been renamed in "
		<< newname << endl;

	}catch (FMS_CPF_Exception &e)
	{
		cout << "Exception on Rename: " << e.errorCode() << ", "
		<< e.errorText() << endl;
	}

}

void CPFTEST::testCopy(string file, string newname, FMS_CPF_Types::copyMode copyMode){

	try{

		FMS_CPF_File fileTest (file.c_str(), cpName.c_str());
		fileTest.copy(newname.c_str(), copyMode);
		cout << "The file " << file << " has been copied" << endl;

	}catch (FMS_CPF_Exception &e)
	{
		cout << "Exception on Copy: " << e.errorCode() << ", " << e.errorText()
		<< endl;
	}

}
void  CPFTEST::testReserve(string file){
		try{
			string car;
			FMS_CPF_File fileTest (file.c_str(), cpName.c_str());
				fileTest.reserve(FMS_CPF_Types::XR_XW_);
				cout << "The file " << file << " has been reserved "
					 << endl;
				cout << "Press any key to unreserve file" << endl;
				cin >> car;

			}catch (FMS_CPF_Exception &e)
			{
				cout << "Exception on Reserve: " << e.errorCode() << ", "
				<< e.errorText() << endl;
			}
	}

void CPFTEST::testIterator(){

	//Create an FMS_CPF_FileIterator instance
	FMS_CPF_FileIterator iterator ("", true, cpName.c_str());

	//Create the file descriptor linked to the file
	FMS_CPF_FileIterator ::FMS_CPF_FileData  fd;

	try{
		while(iterator.getNext(fd)){
			FMS_CPF_FileIterator iterators(fd.fileName.c_str(), true, true, cpName.c_str());
			FMS_CPF_FileIterator :: FMS_CPF_FileData  subfd;

			try{
				cout << endl;
				cout<<"*-----------------------------------------------------------------*\n";
				cout << "File name:" << fd.fileName << endl;
				switch(fd.ftype){
						case FMS_CPF_Types :: ft_INFINITE:
								cout << "Type: INFINITE" << endl;
								cout <<  "Volume: " << fd.volume << endl;
								cout << "RecordLenght: "<< fd.recordLength;
								cout <<  " MaxTime: " << fd.maxtime;
								cout <<  " MaxSize: " << fd.maxsize << endl;
								cout << endl;
								cout.setf (ios::left, ios::adjustfield);
								cout << "SUBFILES";
								cout << setw (28) << "" << "      SIZE"<< endl;
								while(iterators.getNext(subfd)){
									cout.setf (ios::left, ios::adjustfield);
									cout << setw (34) << subfd.fileName;
									cout << "  ";
									cout.setf (ios::right, ios::adjustfield);
									cout << setw (10) << subfd.file_size << "  ";
									cout << endl;
								}
								break;
						case FMS_CPF_Types :: ft_REGULAR:
								cout << "Type: REGULAR" << endl;
								cout <<  "Volume: " << fd.volume << endl;
								cout << "RecordLenght: "<< fd.recordLength <<endl;
								cout << endl;
								cout.setf (ios::left, ios::adjustfield);
								cout << "SUBFILES";
								cout << setw (28) << "" << "      SIZE"<< endl;
								if(fd.composite){
									//cout << "Composite" << endl;

									while(iterators.getNext(subfd)){
										cout.setf (ios::left, ios::adjustfield);
										cout << setw (34) << subfd.fileName;
										cout << "  ";
										cout.setf (ios::right, ios::adjustfield);
										cout << setw (10) << subfd.file_size << "  ";
										cout << endl;
									}
								}
								break;
						default:
								cout << "ERROR" << endl;
								break;
				}

			}
			catch(FMS_CPF_Exception  &e)
				{
					cout << "Exception on iterator function : "<< e.errorCode() << ", " << e.errorText() << endl;
				}
		}

	} catch (FMS_CPF_Exception  &e)
	{
		cout << "Exception on iterator function: " << e.errorCode() << ", " << e.errorText() << endl;
	}
}



