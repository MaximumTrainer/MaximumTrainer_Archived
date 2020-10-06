/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(ANTFS_DIRECTORY_H)
#define ANTFS_DIRECTORY_H

#include "types.h"

#define ANTFS_GENERAL_FLAG_READ        ((UCHAR)0x80)
#define ANTFS_GENERAL_FLAG_WRITE       ((UCHAR)0x40)
#define ANTFS_GENERAL_FLAG_ERASE       ((UCHAR)0x20)
#define ANTFS_GENERAL_FLAG_ARCHIVE     ((UCHAR)0x10)
#define ANTFS_GENERAL_FLAG_APPEND      ((UCHAR)0x08)
#define ANTFS_GENERAL_FLAG_CRYPTO      ((UCHAR)0x04)

#define ANTFS_SPECIFIC_FLAG_SELECTED   ((UCHAR)0x01)

//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////

typedef struct
{
   UCHAR ucVersion;
   UCHAR ucElementLength;
   UCHAR ucTimeFormat;
   UCHAR ucReserved01;
   UCHAR ucReserved02;
   UCHAR ucReserved03;
   UCHAR ucReserved04;
   UCHAR ucReserved05;
   ULONG ulSystemTime;
   ULONG ulTimeStamp;
} ANTFS_DIRECTORY_HEADER;

typedef struct
{
   USHORT usFileIndex;
   UCHAR  ucFileDataType;
   UCHAR  ucFileSubType;
   USHORT usFileNumber;
   UCHAR  ucSpecificFlags;
   UCHAR  ucGeneralFlags;
   ULONG  ulFileSize;
   ULONG  ulTimeStamp;
} ANTFSP_DIRECTORY;


#define MAX_DATA_SIZE                  65535
//////////////////////

#if defined(__cplusplus)
   extern "C" {
#endif

ULONG ANTFSDir_GetNumberOfFileEntries(void *pvDirectory_, ULONG ulDirectoryFileLength_);
/////////////////////////////////////////////////////////////////
// Returns the number of file entries contained in the direcotry.
//
// Parameters:
//    *pvDirectory_   : Pointer to the downloaded directory file
//    ulDirectoryFileLength_ :  Length of the downloaded directory file
//
// Returns the number of file entries contained in the directory.
/////////////////////////////////////////////////////////////////

BOOL ANTFSDir_LookupFileEntry(void *pvDirectory_, ULONG ulDirectoryFileLength_, ULONG ulFileEntry_, ANTFSP_DIRECTORY *pusDirectoryStruct_);
/////////////////////////////////////////////////////////////////
// Fills in the directory struct with information from the directory
// file.
//
// Parameters:
//    *pvDirectory_   : Pointer to the downloaded directory file
//    ulDirectoryFileLength_ :  Length of the downloaded directory file
//    ulFileEntry_    : Entry number of the file that needs to be looked up (this parameter is zero based).
//    *pusDirectoryStruct_ : Pointer to a ANTFSP_DIRECTORY struct where
//                      the information from the directory will be filled in.
//                      The calling applicaiton should allocate an
//                      ANTFSP_DIRECTORY struct and provide the pointer to it.
// Returns TRUE if successful.  Otherwise, it returns FALSE.
/////////////////////////////////////////////////////////////////

BOOL ANTFSDir_GetNewFileList(void *pvDirectory_, ULONG ulDirectoryFileLength_, USHORT *pusFileIndexList, USHORT * pusListLength);
/////////////////////////////////////////////////////////////////
// Decodes the directory and generates a list of files that needs
// that needs to be downloaded.
//
// Parameters:
//    *pvDirectory_   : Pointer to the downloaded directory file
//    ulDirectoryFileLength_ :  Length of the downloaded directory file
//    *pusFileIndexList : Pointer to a USHORT array where the list
//                      of file indexes will be written. NULL can be
//                      passed to this parameter so that the size can
//                      be returned without actually writing the list.
//                      The application can then call this function again
//                      after it has allocated an array of sufficient
//                      size to handle the list.
//    *pusListLength  : Pointer to a USHORT that will receive
//                      the number of files the list will contain.
// Returns TRUE if successful.  Otherwise, it returns FALSE.
/////////////////////////////////////////////////////////////////

BOOL ANTFSDir_LookupFileIndex(void *pvDirectory_, ULONG ulDirectoryFileLength_, USHORT usFileIndex_, ANTFSP_DIRECTORY *pusDirectoryStruct_);
/////////////////////////////////////////////////////////////////
// Fills in the directory struct with information from the directory
// file.
//
// Parameters:
//    *pvDirectory_   : Pointer to the downloaded directory file
//    ulDirectoryFileLength_ :  Length of the downloaded directory file
//    usFileIndex_    : Indext of the file that needs to be looked up.
//    *pusDirectoryStruct_ : Pointer to a ANTFSP_DIRECTORY struct where
//                      the information from the directory will be filled in.
//                      The calling applicaiton should allocate an
//                      ANTFSP_DIRECTORY struct and provide the pointer to it.
// Returns TRUE if successful.  Otherwise, it returns FALSE.
/////////////////////////////////////////////////////////////////

#if defined(__cplusplus)
   }
#endif


#endif // !defined(ANTFS_DIRECTORY_H)
