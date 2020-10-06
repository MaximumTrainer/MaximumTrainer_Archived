/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#include "dsi_convert.h"
#include "antfs_directory.h"


typedef struct
{
   UCHAR  usFileIndexByte0;
   UCHAR  usFileIndexByte1;
   UCHAR  ucFileDataType;
   UCHAR  ucFileSubType;
   UCHAR  usFileNumberByte0;
   UCHAR  usFileNumberByte1;
   UCHAR  ucSpecificFlags;
   UCHAR  ucGeneralFlags;
   UCHAR  ulFileSizeByte0;
   UCHAR  ulFileSizeByte1;
   UCHAR  ulFileSizeByte2;
   UCHAR  ulFileSizeByte3;
   UCHAR  ulTimeStampByte0;
   UCHAR  ulTimeStampByte1;
   UCHAR  ulTimeStampByte2;
   UCHAR  ulTimeStampByte3;
} ANTFSP_DIRECTORY_LOOKUP;

///////////////////////////////////////////////////////////////////////
ULONG ANTFSDir_GetNumberOfFileEntries(void *pvDirectory_, ULONG ulDirectoryFileLength_)
{
   ANTFS_DIRECTORY_HEADER *psDirHeader;

   if (pvDirectory_ == NULL)
      return 0;

   psDirHeader = (ANTFS_DIRECTORY_HEADER*)pvDirectory_;

   if (psDirHeader->ucVersion != 1)
      return 0;

   if (psDirHeader->ucElementLength != 16)
      return 0;

   if ((sizeof(ANTFS_DIRECTORY_HEADER) + psDirHeader->ucElementLength) > ulDirectoryFileLength_)   //return 0 if the directory size is smaller than the header + 1 entry
      return 0;

   return ((ulDirectoryFileLength_ - sizeof(ANTFS_DIRECTORY_HEADER)) / psDirHeader->ucElementLength);
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSDir_LookupFileEntry(void *pvDirectory_, ULONG ulDirectoryFileLength_, ULONG ulFileEntry_, ANTFSP_DIRECTORY *pusDirectoryStruct_)
{
   ANTFS_DIRECTORY_HEADER *psDirHeader;
   ANTFSP_DIRECTORY_LOOKUP *psCurrentDirElement;

   if (pvDirectory_ == NULL)
      return FALSE;

   psDirHeader = (ANTFS_DIRECTORY_HEADER*)pvDirectory_;

   if (psDirHeader->ucVersion != 1)
      return FALSE;

   if (psDirHeader->ucElementLength != 16)
      return FALSE;

   if (pusDirectoryStruct_ == (ANTFSP_DIRECTORY*)NULL)
      return FALSE;

   if ((sizeof(ANTFS_DIRECTORY_HEADER) + ((ulFileEntry_ + 1) * psDirHeader->ucElementLength)) <= ulDirectoryFileLength_)
   {
      psCurrentDirElement = (ANTFSP_DIRECTORY_LOOKUP *)((UCHAR*)pvDirectory_ + sizeof(ANTFS_DIRECTORY_HEADER) + (psDirHeader->ucElementLength * ulFileEntry_));

      pusDirectoryStruct_->usFileIndex = Convert_Bytes_To_USHORT(psCurrentDirElement->usFileIndexByte1,psCurrentDirElement->usFileIndexByte0);
      pusDirectoryStruct_->ucFileDataType = psCurrentDirElement->ucFileDataType;
      pusDirectoryStruct_->ucFileSubType = psCurrentDirElement->ucFileSubType;
      pusDirectoryStruct_->usFileNumber = Convert_Bytes_To_USHORT(psCurrentDirElement->usFileNumberByte1,psCurrentDirElement->usFileNumberByte0);
      pusDirectoryStruct_->ucSpecificFlags = psCurrentDirElement->ucSpecificFlags;
      pusDirectoryStruct_->ucGeneralFlags = psCurrentDirElement->ucGeneralFlags;
      pusDirectoryStruct_->ulFileSize = Convert_Bytes_To_ULONG(psCurrentDirElement->ulFileSizeByte3,psCurrentDirElement->ulFileSizeByte2,psCurrentDirElement->ulFileSizeByte1,psCurrentDirElement->ulFileSizeByte0);
      pusDirectoryStruct_->ulTimeStamp = Convert_Bytes_To_ULONG(psCurrentDirElement->ulTimeStampByte3,psCurrentDirElement->ulTimeStampByte2,psCurrentDirElement->ulTimeStampByte1,psCurrentDirElement->ulTimeStampByte0);

      return TRUE;
   }

   return FALSE;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSDir_GetNewFileList(void *pvDirectory_, ULONG ulDirectoryFileLength_, USHORT *pusFileIndexList_, USHORT * pusListLength_)
{
   ANTFS_DIRECTORY_HEADER *psDirHeader;
   ANTFSP_DIRECTORY_LOOKUP *psCurrentDirElement;
   ULONG ulElementNum = 0;
   USHORT usNumberOfMatches = 0;

   if (pvDirectory_ == NULL)
      return FALSE;

   psDirHeader = (ANTFS_DIRECTORY_HEADER*)pvDirectory_;

   if (psDirHeader->ucVersion != 1)
      return FALSE;

   if (psDirHeader->ucElementLength != 16)
      return FALSE;

   while ((sizeof(ANTFS_DIRECTORY_HEADER) + ((ulElementNum + 1) * psDirHeader->ucElementLength)) <= ulDirectoryFileLength_)
   {
      psCurrentDirElement = (ANTFSP_DIRECTORY_LOOKUP *)((UCHAR*)pvDirectory_ + sizeof(ANTFS_DIRECTORY_HEADER) + (psDirHeader->ucElementLength * ulElementNum));

      if (psCurrentDirElement->ucFileDataType == 0x80)  //ANTFS+ data type
      {
         if (psCurrentDirElement->ucFileSubType == 4)  //ANTFS+ session sub type
         {
            if ((!(psCurrentDirElement->ucGeneralFlags & ANTFS_GENERAL_FLAG_ARCHIVE)) &&
               (psCurrentDirElement->ucGeneralFlags & ANTFS_GENERAL_FLAG_READ))            //This has never been downloaded and can be read
            {
               usNumberOfMatches++;

               if (pusFileIndexList_ != NULL)
               {
                  *pusFileIndexList_ = Convert_Bytes_To_USHORT(psCurrentDirElement->usFileIndexByte1,psCurrentDirElement->usFileIndexByte0);
                  pusFileIndexList_ ++;
               }
            }
         }
      }

      ulElementNum++;
   }

   if (pusListLength_ != NULL)
      *pusListLength_ = usNumberOfMatches;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSDir_LookupFileIndex(void *pvDirectory_, ULONG ulDirectoryFileLength_, USHORT usFileIndex_, ANTFSP_DIRECTORY *pusDirectoryStruct_)
{
   ANTFS_DIRECTORY_HEADER *psDirHeader;
   ANTFSP_DIRECTORY_LOOKUP *psCurrentDirElement;
   ULONG ulElementNum = 0;

   if (pvDirectory_ == NULL)
      return FALSE;

   psDirHeader = (ANTFS_DIRECTORY_HEADER*)pvDirectory_;

   if (psDirHeader->ucVersion != 1)
      return FALSE;

   if (psDirHeader->ucElementLength != 16)
      return FALSE;

   if (pusDirectoryStruct_ == (ANTFSP_DIRECTORY*)NULL)
      return FALSE;

   while ((sizeof(ANTFS_DIRECTORY_HEADER) + ((ulElementNum + 1) * psDirHeader->ucElementLength)) <= ulDirectoryFileLength_)
   {
      USHORT usCurrentFileIndex;
      psCurrentDirElement = (ANTFSP_DIRECTORY_LOOKUP *)((UCHAR*)pvDirectory_ + sizeof(ANTFS_DIRECTORY_HEADER) + (psDirHeader->ucElementLength * ulElementNum));

      usCurrentFileIndex = Convert_Bytes_To_USHORT(psCurrentDirElement->usFileIndexByte1,psCurrentDirElement->usFileIndexByte0);

      if (usCurrentFileIndex == usFileIndex_)  //If this is the index we want.
      {
         pusDirectoryStruct_->usFileIndex = usCurrentFileIndex;
         pusDirectoryStruct_->ucFileDataType = psCurrentDirElement->ucFileDataType;
         pusDirectoryStruct_->ucFileSubType = psCurrentDirElement->ucFileSubType;
         pusDirectoryStruct_->usFileNumber = Convert_Bytes_To_USHORT(psCurrentDirElement->usFileNumberByte1,psCurrentDirElement->usFileNumberByte0);
         pusDirectoryStruct_->ucSpecificFlags = psCurrentDirElement->ucSpecificFlags;
         pusDirectoryStruct_->ucGeneralFlags = psCurrentDirElement->ucGeneralFlags;
         pusDirectoryStruct_->ulFileSize = Convert_Bytes_To_ULONG(psCurrentDirElement->ulFileSizeByte3,psCurrentDirElement->ulFileSizeByte2,psCurrentDirElement->ulFileSizeByte1,psCurrentDirElement->ulFileSizeByte0);
         pusDirectoryStruct_->ulTimeStamp = Convert_Bytes_To_ULONG(psCurrentDirElement->ulTimeStampByte3,psCurrentDirElement->ulTimeStampByte2,psCurrentDirElement->ulTimeStampByte1,psCurrentDirElement->ulTimeStampByte0);

         return TRUE;
      }
      ulElementNum++;
   }


   return FALSE;
}
