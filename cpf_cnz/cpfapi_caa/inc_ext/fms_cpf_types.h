//******************************************************************************
//
// .NAME
//      FMS_CPF_Types - File handler API
// .LIBRARY 3C++
// .PAGENAME FMS_CPF_Types
// .HEADER  AP/FMS Internal
// .LEFT_FOOTER Ericsson Utvecklings AB
// .INCLUDE FMS_CPF_Types.H
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
//      This header file contains types used by the CP file API.
//
//
// .ERROR HANDLING
//
//      All errors are reported by throwing the exception class
//      FMS_CPF_Exception.
//
//
// DOCUMENT NO
//      190 89-CAA 109 0088
//
// AUTHOR
//      1997-11-18      by UAB/I/TD     UABTSO
//
// CHANGES
//
//      CHANGE HISTORY
//
//      DATE    NAME    DESCRIPTION
//      980113  UABTSO  1:st revision
//      980615  UABTSO  2:nd revision
//		020506	qabtjer	Removed datalink type
//				esalves
//
// .LINKAGE
//      -
//
// .SEE ALSO
//      Class FMS_CPF_File and FMS_CPF_FileIterator
//
//******************************************************************************

#ifndef FMS_CPF_TYPES_H
#define FMS_CPF_TYPES_H


#include <sys/types.h>
#include <string>

const unsigned long FMS_CPF_EXCLUSIVE = (unsigned long)65535;

// Max allowed length of the transfer queue name
const int FMS_CPF_TQMAXLENGTH = 32;

//##ModelId=3F0D66690177

class FMS_CPF_Types
{
 public:

  //****************************************************************************
  // Type declarations
  //****************************************************************************


  // File types
//SIO_I4 prefix ft qablake 990201 conflict win INFINITE
	//##ModelId=3F0D66690222
  enum fileType {
		//##ModelId=3F0D66690240
    ft_REGULAR,			// Regular file type
		//##ModelId=3F0D6669024A
    ft_TEXT,			// Text file type (not implemented)
		//##ModelId=3F0D6669025E
    ft_INFINITE			// Infinite file type

  };

    //##ModelId=3F169CFF015C
  enum transferMode
	{
		//##ModelId=3F169CFF015D
		tm_NONE,
		//##ModelId=3F169CFF015E
		tm_FILE,
		//##ModelId=3F169CFF015F
		tm_BLOCK,
        tm_UNDEF
	};

  // Regular file type

	//##ModelId=3F0D66690290
  struct regularType
  {
		//##ModelId=3F0D666902AE
    unsigned long rlength;            // Record length
		//##ModelId=3F0D666902C2
    bool    composite;          // True if composite file
     // HY46076
    long int  deleteFileTimer;   
  };

  // Text file type (not implemented)

	//##ModelId=3F0D666902EA
  struct textType
  {
		//##ModelId=3F0D66690308
    bool composite;             // True if composite file
  };

  // Infinite file type

	//##ModelId=3F0D66690376
  struct infiniteType
  {
		//##ModelId=3F0D66690394
    unsigned long rlength;            // Record length in octets
		//##ModelId=3F0D666903A8
    unsigned long  maxsize;            // Max file size in number of records before
				// next subfile change
		//##ModelId=3F0D666903B2
    unsigned long  maxtime;            // Max time in seconds before next subfile
				// change
		//##ModelId=3F0D666903BC
    bool    release;            // Release condition for subfile change
		//##ModelId=3F0D666903D0
    unsigned long  active;		// Active subfile
		//##ModelId=3F0D666903DA
	unsigned long  lastReportedSubfile;		//The last reported subfile to GOH   drop 1 INGO3 tjer
	    //##ModelId=3F16A21501A5
    char transferQueue[FMS_CPF_TQMAXLENGTH+1];
	    //##ModelId=3F16A22B01B0
	transferMode mode;
    transferMode inittransfermode; //030821
  };


  // File attributes

	//##ModelId=3F0D666A0024

  struct fileAttributes
  {
  public:
		//##ModelId=3F0D666A004D
    fileType ftype;             // File type
    union
    {
      regularType  regular;     // Regular file
      textType     text;        // Text file (not implemented)
      infiniteType infinite;    // Infinite file

    };
  };

  // Access authority for an open file

	//##ModelId=3F0D666A00E3
  enum accessType
  {
		//##ModelId=3F0D666A0101
    NONE_,                      // No access
		//##ModelId=3F0D666A010B
    R_,                         // Shared read access
		//##ModelId=3F0D666A011F
    XR_,                        // Exclusive read access
		//##ModelId=3F0D666A0129
    W_,                         // Shared write access
		//##ModelId=3F0D666A013D
    XW_,                        // Exclusive write access
		//##ModelId=3F0D666A0147
    R_W_,                       // Shared read and shared write access
		//##ModelId=3F0D666A0151
    XR_W_,                      // Exclusive read and shared write access
		//##ModelId=3F0D666A0165
    R_XW_,                      // Shared read and exclusive write access
		//##ModelId=3F0D666A016F
    XR_XW_,                      // Exclusive read and exclusive write access
    			// Exclusive Lock to delete
    DELETE_
  };


  // Number of users of a file

	//##ModelId=3F0D666A01B5
  struct userType
  {
		//##ModelId=3F0D666A01D3
    unsigned long ucount;             // Total number of users of a file.
		//##ModelId=3F0D666A01E7
    unsigned long rcount;             // Number of users with read access.
                                // FMS_CPF_EXCLUSIVE denotes exclusive read
                                // access.
		//##ModelId=3F0D666A01F1
    unsigned long wcount;             // Number of users with write access.
                                // FMS_CPF_EXCLUSIVE denotes exclusive write
                                // access.
  };

  // Status information for a file

	//##ModelId=3F0D666A022D
  struct fileStat
  {
		//##ModelId=3F0D666A0241
    size_t size;		// File size in number of records. For a
				// text file the size is given in number of
				// bytes
		//##ModelId=3F0D666A025F
    time_t time;		// Time of last modification of the file
				// (Not yet implemented)
  };

  // Copying mode
//SIO_I4 prefix cm qablake 990201
	//##ModelId=3F0D666A02B9
  enum copyMode
  {
	      cm_OVERWRITE,			// Overwrite files if they exists
		  cm_APPEND,			// Append data
	      cm_CLEAR,				// Remove all subfiles before copying
	      cm_NORMAL			// Do not copy if source destination file exists
  };

    //##ModelId=3F0D666901A0
  static fileAttributes createFileAttr(fileType ftype, unsigned long recordLength, bool composite =false,
										unsigned long maxSize = 0, unsigned long maxTime = 0,
										bool release=false, unsigned long  active=0, unsigned long lastSent = 0,
										transferMode mode = tm_NONE, const std::string transferQueue = "",
										transferMode inittransfermode = tm_UNDEF,long int deleteFileTimer = -1
									   )
  {
    fileAttributes  fileattr;
    fileattr.ftype = ftype;
    switch (ftype)
    {
      case ft_REGULAR:
        fileattr.regular.rlength =recordLength;
        fileattr.regular.composite = composite;
        //HY46076
        fileattr.regular.deleteFileTimer =deleteFileTimer;
        break;

      case ft_TEXT:
        fileattr.text.composite = composite;
        break;

      case ft_INFINITE:
        fileattr.infinite.rlength = recordLength;
        fileattr.infinite.maxsize = maxSize;
        fileattr.infinite.maxtime = maxTime;
        fileattr.infinite.release = release;
        fileattr.infinite.active = active;
        fileattr.infinite.lastReportedSubfile = lastSent;
        size_t length = transferQueue.copy(fileattr.infinite.transferQueue, FMS_CPF_TQMAXLENGTH-1);

        fileattr.infinite.transferQueue[length] = '\0';
        fileattr.infinite.mode = mode;
        fileattr.infinite.inittransfermode = inittransfermode;
        break;

    }
    return  fileattr;
  }

    //##ModelId=3F0D666901C8
  static void extractFileAttr(fileAttributes fileattr, fileType &fType, unsigned long& recordLength,
		  	  	  	  	  	  bool& composite, unsigned long& maxSize , unsigned long& maxTime,
		  	  	  	  	  	  bool& release, unsigned long& active, unsigned long& lastSent, transferMode& mode,
		  	  	  	  	  	  std::string& transferQueue, transferMode& inittransfermode, long int& deleteFileTimer
		  	  	  	  	  	 )
  {
    fType = fileattr.ftype;
    switch (fType)
    {
      case ft_REGULAR:
        recordLength = fileattr.regular.rlength;
        composite = fileattr.regular.composite;
        //HY46076
        deleteFileTimer = fileattr.regular.deleteFileTimer;
        break;

      case ft_TEXT:
        composite = fileattr.text.composite;
        break;

      case ft_INFINITE:
        recordLength = fileattr.infinite.rlength;
        maxSize = fileattr.infinite.maxsize;
        maxTime =fileattr.infinite.maxtime;
        release = fileattr.infinite.release;
        transferQueue = fileattr.infinite.transferQueue;
        mode = fileattr.infinite.mode;
        active = fileattr.infinite.active;
        lastSent = fileattr.infinite.lastReportedSubfile;
        inittransfermode = fileattr.infinite.inittransfermode;
        break;

    }
  }
};


#endif /* FMS_CPF_TYPES_H */
