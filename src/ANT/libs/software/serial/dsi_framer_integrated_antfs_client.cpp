/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#include "defines.h"
#include "antmessage.h"
#include "antdefines.h"
#include "checksum.h"
#include "dsi_thread.h"
#include "dsi_framer_ant.hpp"
#include <string.h>

#define WAIT_TO_FEED_TRANSFER
#include "dsi_debug.hpp"
#if defined(DEBUG_FILE)
#define SERIAL_DEBUG
#endif

//////////////////////////////////////////////////
//ANTFS Tx-Message Buffer Offsets
//////////////////////////////////////////////////
#define OFFSET_COMMAND_ID     ((UCHAR)0x00)
#define OFFSET_REQUEST_ID     ((UCHAR)0x00)
#define OFFSET_MESSAGE_ID     ((UCHAR)0x01)
#define OFFSET_DATA0          ((UCHAR)0x02)

//////////////////////////////////////////////////
//ANTFS Command-Response Buffer Offsets
//////////////////////////////////////////////////
#define OFFSET_RESPONSE_RESPONSE_ID_HIGH           ((UCHAR)0x00)
#define OFFSET_RESPONSE_COMMAND_ID_LOW             ((UCHAR)0x01)
#define OFFSET_RESPONSE_COMMAND_ID_HIGH            ((UCHAR)0x02)
#define OFFSET_RESPONSE_FSRESPONSE                 ((UCHAR)0x03)

//////////////////////////////////////////////////
//ANTFS Request-Response Buffer Offsets
//////////////////////////////////////////////////
#define OFFSET_REQUEST_RESPONSE_RESPONSE_ID        ((UCHAR)0x00)
#define OFFSET_REQUEST_RESPONSE_FSRESPONSE         ((UCHAR)0x01)
#define OFFSET_REQUEST_RESPONSE_PAYLOAD            ((UCHAR)0x02)

////////////////////////////////////////////////////////
//Public Functions
// This file contains the implementation of the extended ANT messaging
// for integrated ANT-FS client.  Function declarations are
// in dsi_framer_ant.hpp
////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SendFSCommand(FS_MESSAGE *pstFSMessage_, USHORT usMessageSize_, UCHAR* pucFSResponse, ULONG ulResponseTime_)
{
   ANTMessageResponse *pclCommandResponse = (ANTMessageResponse*)NULL;

   // If we are going to be waiting for a response setup the Response object
   if (ulResponseTime_ != 0)
   {
      UCHAR aucDesiredData[MESG_FS_REQUEST_RESPONSE_SIZE];
      UCHAR bytesToMatch = MESG_FS_REQUEST_RESPONSE_SIZE;

      aucDesiredData[OFFSET_RESPONSE_RESPONSE_ID_HIGH] = (UCHAR)(MESG_EXT_RESPONSE_ID & 0xFF);
      aucDesiredData[OFFSET_RESPONSE_COMMAND_ID_LOW] = pstFSMessage_->ucCommandID;
      aucDesiredData[OFFSET_RESPONSE_COMMAND_ID_HIGH] = pstFSMessage_->ucMessageID;

      pclCommandResponse = new ANTMessageResponse();
      pclCommandResponse->Attach((UCHAR)((MESG_EXT_RESPONSE_ID >> 8) & 0xFF), aucDesiredData, bytesToMatch, this);
   }

   // Write the command message.
   if (!WriteMessage(pstFSMessage_, usMessageSize_))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("Framer->SendFSCommand():  WriteMessage Failed.");
      #endif

      if(pclCommandResponse != NULL)
      {
         delete pclCommandResponse;
         pclCommandResponse = (ANTMessageResponse*)NULL;
      }

      return FALSE;
   }

   // Return immediately if we aren't waiting for the response.
   if (ulResponseTime_ == 0)
      return TRUE;

   // Wait for the response.
   pclCommandResponse->WaitForResponse(ulResponseTime_);

   pclCommandResponse->Remove();                                               //detach from list

   // We haven't received a response in the allotted time.
   //if (pclCommandResponse->stMessageItem.ucSize == 0)
   if (pclCommandResponse->bResponseReady == FALSE)
   {
      delete pclCommandResponse;
      #if defined(SERIAL_DEBUG)
      DSIDebug::ThreadWrite("Framer->SendCommand():  Timeout.");
      #endif
      return FALSE;
   }

   // Check the response.
   if (pclCommandResponse->stMessageItem.stANTMessage.aucData[OFFSET_RESPONSE_FSRESPONSE] != FS_NO_ERROR_RESPONSE)
   {
      delete pclCommandResponse;
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("Framer->SendFSCommand():  Response != RESPONSE_NO_ERROR.");
      #endif
      return FALSE;
   }

   *pucFSResponse = pclCommandResponse->stMessageItem.stANTMessage.aucData[OFFSET_RESPONSE_FSRESPONSE];                         //Save the FSResponse

   delete pclCommandResponse;
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SendFSRequest(UCHAR MesgSize, ANT_MESSAGE_ITEM *pstANTResponse_, FS_MESSAGE stMessage, ULONG ulResponseTime_)
{
   ANTMessageResponse *pclRequestResponse = (ANTMessageResponse*)NULL;

   // If we are going to be waiting for a response setup the Response object
   if ((ulResponseTime_ != 0) && (pstANTResponse_ != NULL))
   {
      pclRequestResponse = new ANTMessageResponse();
      pclRequestResponse->Attach(MESG_EXT_ID_2, (UCHAR*)NULL, 0, this);
   }

   // Write the command message.
   if (!WriteMessage(&stMessage, MesgSize))
   {
      if(pclRequestResponse != NULL)
      {
         delete pclRequestResponse;
         pclRequestResponse = (ANTMessageResponse*)NULL;
      }
      return FALSE;
   }

   // Return immediately if we aren't waiting for the response.
   if ((ulResponseTime_ == 0) || (pstANTResponse_ == NULL))
      return TRUE;

   // Wait for the response.
   pclRequestResponse->WaitForResponse(ulResponseTime_);

   pclRequestResponse->Remove();

   // We haven't received a response in the allotted time.
   if (pclRequestResponse->bResponseReady == FALSE)
   {
      delete pclRequestResponse;
      return FALSE;
   }

   // Copy out the response
   pstANTResponse_->ucSize = pclRequestResponse->stMessageItem.ucSize;
   pstANTResponse_->stANTMessage.ucMessageID = pclRequestResponse->stMessageItem.stANTMessage.ucMessageID;
   memcpy (pstANTResponse_->stANTMessage.aucData, pclRequestResponse->stMessageItem.stANTMessage.aucData, pclRequestResponse->stMessageItem.ucSize);

   delete pclRequestResponse;
   return TRUE;
}


UCHAR getByte_ulLE(int byteNum, ULONG value)
{
   if (byteNum == 1)          //Return low byte
      return (UCHAR)(value & 0x000000FF);
   else if (byteNum == 2)           //Return mid-low byte
      return (UCHAR)((value & 0x0000FF00) >> 8);
   else if (byteNum == 3)           //Return mid-high byte
      return (UCHAR)((value & 0x00FF0000) >> 16);
   else if (byteNum == 4)           //Return high byte
      return (UCHAR)((value & 0xFF000000) >> 24);

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("getByte_ulLE():  Invalid byteNum, returning 0.");
   #endif
   return 0; //If bytenum is invalid always return 0
}

/////////////////////////////////////////////////////
//ANTFS_Integrated Serial Functions
////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////
/* MEMDev COMMANDS */
//////////////////////////////////////////////////////////////
BOOL DSIFramerANT::InitEEPROMDevice(USHORT usPageSize_, UCHAR ucAddressConfig_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_MEMDEV_EEPROM_INIT_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_MEMDEV_EEPROM_INIT_ID & 0xFF);
   stMessage.aucData[0] = getByte_ulLE(1,usPageSize_);
   stMessage.aucData[1] = getByte_ulLE(2,usPageSize_);
   stMessage.aucData[2] = getByte_ulLE(1,ucAddressConfig_);

   if(SendFSCommand(&stMessage, MESG_MEMDEV_EEPROM_INIT_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;
      return TRUE;
   }
   else
      return FALSE;
}

////////////////////////////////////////////////////////////////////////
/* File System Commands*/
////////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::InitFSMemory(ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_INIT_MEMORY_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_INIT_MEMORY_ID & 0xFF);

   if(SendFSCommand(&stMessage, MESG_FS_INIT_MEMORY_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return TRUE;
   }
   else
   {
      ucFSResponse = ucResponse;
      return FALSE;
   }
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::FormatFSMemory(USHORT usNumberOfSectors_, USHORT usPagesPerSector_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_FORMAT_MEMORY_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_FORMAT_MEMORY_ID & 0xFF);
   stMessage.aucData[0] = getByte_ulLE(1,usNumberOfSectors_);
   stMessage.aucData[1] = getByte_ulLE(2,usNumberOfSectors_);
   stMessage.aucData[2] = getByte_ulLE(1,usPagesPerSector_);
   stMessage.aucData[3] = getByte_ulLE(2,usPagesPerSector_);

   if(SendFSCommand(&stMessage, MESG_FS_FORMAT_MEMORY_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return TRUE;
   }
   else
   {
      ucFSResponse = ucResponse;
      return FALSE;
   }
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SaveDirectory(ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_DIRECTORY_SAVE_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_DIRECTORY_SAVE_ID & 0xFF);

   if(SendFSCommand(&stMessage, MESG_FS_DIRECTORY_SAVE_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return TRUE;
   }
   else
   {
      ucFSResponse = ucResponse;
      return FALSE;
   }
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::DirectoryRebuild(ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_DIRECTORY_REBUILD_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_DIRECTORY_REBUILD_ID & 0xFF);

   if(SendFSCommand(&stMessage, MESG_FS_DIRECTORY_REBUILD_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return TRUE;
   }
   else
   {
      ucFSResponse = ucResponse;
      return FALSE;
   }
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::FileDelete(UCHAR ucFileHandle_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_FILE_DELETE_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_FILE_DELETE_ID & 0xFF);
   stMessage.aucData[0] = ucFileHandle_;

   if(SendFSCommand(&stMessage, MESG_FS_FILE_DELETE_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return TRUE;
   }
   else
   {
      ucFSResponse = ucResponse;
      return FALSE;
   }
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::FileClose(UCHAR ucFileHandle_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_FILE_CLOSE_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_FILE_CLOSE_ID & 0xFF);
   stMessage.aucData[0] = ucFileHandle_;

   if(SendFSCommand(&stMessage, MESG_FS_FILE_CLOSE_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return TRUE;
   }
   else
   {
      ucFSResponse = ucResponse;
      return FALSE;
   }
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetFileSpecificFlags(UCHAR ucFileHandle_, UCHAR ucFlags_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_FILE_SET_SPECIFIC_FLAGS_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_FILE_SET_SPECIFIC_FLAGS_ID & 0xFF);
   stMessage.aucData[0] = ucFileHandle_;
   stMessage.aucData[1] = ucFlags_;

   if(SendFSCommand(&stMessage, MESG_FS_FILE_SET_SPECIFIC_FLAGS_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return TRUE;
   }
   else
   {
      ucFSResponse = ucResponse;
      return FALSE;
   }
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::DirectoryReadLock(BOOL bLock_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_DIRECTORY_READ_LOCK_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_DIRECTORY_READ_LOCK_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR) bLock_;

   if(SendFSCommand(&stMessage, MESG_FS_DIRECTORY_READ_LOCK_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetSystemTime(ULONG ulTime_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_SYSTEM_TIME_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_SYSTEM_TIME_ID & 0xFF);
   stMessage.aucData[0] = getByte_ulLE(1,ulTime_);
   stMessage.aucData[1] = getByte_ulLE(2,ulTime_);
   stMessage.aucData[2] = getByte_ulLE(3,ulTime_);
   stMessage.aucData[3] = getByte_ulLE(4,ulTime_);

   if(SendFSCommand(&stMessage, MESG_FS_SYSTEM_TIME_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return TRUE;
   }
   else
   {
      ucFSResponse = ucResponse;
      return FALSE;
   }
}


///////////////////////////////////////////////////////////////////////
/* File System Requests */
///////////////////////////////////////////////////////////////////////
ULONG DSIFramerANT::GetUsedSpace(ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID  >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_GET_USED_SPACE_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_GET_USED_SPACE_ID & 0xFF);

   SendFSRequest(MESG_FS_GET_USED_SPACE_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];
   return ((ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD]) | ((ULONG)stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 1] << 8) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 2] << 16) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 3] << 24));
}

///////////////////////////////////////////////////////////////////////
ULONG DSIFramerANT::GetFreeFSSpace(ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;

   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_GET_FREE_SPACE_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_GET_FREE_SPACE_ID & 0xFF);

   SendFSRequest(MESG_FS_GET_FREE_SPACE_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];
   return (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD]) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 1] << 8) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 2] << 16) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 3] << 24);
}

///////////////////////////////////////////////////////////////////////
USHORT DSIFramerANT::FindFileIndex(UCHAR ucFileDataType_, UCHAR ucFileSubType_, USHORT usFileNumber_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_FIND_FILE_INDEX_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_FIND_FILE_INDEX_ID & 0xFF);
   stMessage.aucData[2] = ucFileDataType_;
   stMessage.aucData[3] = ucFileSubType_;
   stMessage.aucData[4] = getByte_ulLE(1,usFileNumber_);
   stMessage.aucData[5] = getByte_ulLE(2,usFileNumber_);


   SendFSRequest(MESG_FS_FIND_FILE_INDEX_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];
   return (USHORT)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD]) | (USHORT)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 1] << 8);
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::ReadDirectoryAbsolute(ULONG ulOffset_, UCHAR ucSize_, UCHAR *pucBuffer_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_DIRECTORY_READ_ABSOLUTE_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_DIRECTORY_READ_ABSOLUTE_ID & 0xFF);
   stMessage.aucData[2] = getByte_ulLE(1,ulOffset_);
   stMessage.aucData[3] = getByte_ulLE(2,ulOffset_);
   stMessage.aucData[4] = getByte_ulLE(3,ulOffset_);
   stMessage.aucData[5] = getByte_ulLE(4,ulOffset_);
   stMessage.aucData[6] = ucSize_;


   SendFSRequest(MESG_FS_DIRECTORY_READ_ABSOLUTE_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   //Now load pucBuffer_ with the Paylaod Array
   //Caller has to do validation of the response before reading the buffer
   for(int i = 0; i < ucSize_; i++)
      pucBuffer_[i] = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 1 + i];                 //add 1 to skip sizeRead byte

   return ucFSResponse;
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::DirectoryReadEntry(USHORT usFileIndex_, UCHAR *ucFileDirectoryBuffer_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_DIRECTORY_READ_ENTRY_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_DIRECTORY_READ_ENTRY_ID & 0xFF);
   stMessage.aucData[2] = getByte_ulLE(1, usFileIndex_);
   stMessage.aucData[3] = getByte_ulLE(2, usFileIndex_);

   SendFSRequest(MESG_FS_DIRECTORY_READ_ENTRY_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   //Now load pucBuffer_ with the Paylaod Array
   for(int i = 0; i < 16; i++)
      ucFileDirectoryBuffer_[i] = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 1 + i];                     //add 1 to skip sizeRead byte

   return ucFSResponse;
}

///////////////////////////////////////////////////////////////////////
ULONG DSIFramerANT::DirectoryGetSize(ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_DIRECTORY_GET_SIZE_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_DIRECTORY_GET_SIZE_ID & 0xFF);

   SendFSRequest(MESG_FS_DIRECTORY_GET_SIZE_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   return (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD]) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 1] << 8) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 2] << 16) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 3] << 24);
}

///////////////////////////////////////////////////////////////////////
USHORT DSIFramerANT::FileCreate(USHORT usFileIndex_, UCHAR ucFileDataType_, ULONG ulFileIdentifier_, UCHAR ucFileDataTypeSpecificFlags_, UCHAR ucGeneralFlags_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_FILE_CREATE_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_FILE_CREATE_ID & 0xFF);
   stMessage.aucData[2] = getByte_ulLE(1, usFileIndex_);
   stMessage.aucData[3] = getByte_ulLE(2, usFileIndex_);
   stMessage.aucData[4] = ucFileDataType_;
   stMessage.aucData[5] = getByte_ulLE(1, ulFileIdentifier_);
   stMessage.aucData[6] = getByte_ulLE(2, ulFileIdentifier_);
   stMessage.aucData[7] = getByte_ulLE(3, ulFileIdentifier_);
   stMessage.aucData[8] = ucFileDataTypeSpecificFlags_;
   stMessage.aucData[9] = ucGeneralFlags_;


   SendFSRequest(MESG_FS_FILE_CREATE_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   return (USHORT)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD]) | (USHORT)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 1] << 8);
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::FileOpen(USHORT usFileIndex_, UCHAR ucOpenFlags_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_FILE_OPEN_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_FILE_OPEN_ID & 0xFF);
   stMessage.aucData[2] = getByte_ulLE(1, usFileIndex_);
   stMessage.aucData[3] = getByte_ulLE(2, usFileIndex_);
   stMessage.aucData[4] = ucOpenFlags_;

   SendFSRequest(MESG_FS_FILE_OPEN_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   return (UCHAR)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD]);
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::FileReadAbsolute(UCHAR ucFileHandle_, ULONG ulOffset_, UCHAR ucReadSize_, UCHAR *pucReadBuffer_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_FILE_READ_ABSOLUTE_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_FILE_READ_ABSOLUTE_ID & 0xFF);
   stMessage.aucData[2] = ucFileHandle_;
   stMessage.aucData[3] = getByte_ulLE(1,ulOffset_);
   stMessage.aucData[4] = getByte_ulLE(2,ulOffset_);
   stMessage.aucData[5] = getByte_ulLE(3,ulOffset_);
   stMessage.aucData[6] = getByte_ulLE(4,ulOffset_);
   stMessage.aucData[7] = ucReadSize_;

   SendFSRequest(MESG_FS_FILE_READ_ABSOLUTE_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   //Now load pucBuffer_ with the Paylaod Array
   for(int i = 0; i < ucReadSize_; i++)
      pucReadBuffer_[i] = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 1 + i];                     //add 1 to skip SizeRead byte

   return ucFSResponse;
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::FileReadRelative(UCHAR ucFileHandle_, UCHAR ucReadSize_, UCHAR *pucReadBuffer_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_FILE_READ_RELATIVE_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_FILE_READ_RELATIVE_ID & 0xFF);
   stMessage.aucData[2] = ucFileHandle_;
   stMessage.aucData[3] = ucReadSize_;


   SendFSRequest(MESG_FS_FILE_READ_RELATIVE_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   //Now load pucBuffer_ with the Paylaod Array
   for(int i = 0; i < ucReadSize_; i++)
      pucReadBuffer_[i] = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 1 + i];                     //add 1 to skip SizeRead byte

   return ucFSResponse;
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::FileWriteAbsolute(UCHAR ucFileHandle_, ULONG ulFileOffset_, UCHAR ucWriteSize_, const UCHAR *pucWriteBuffer_, UCHAR *ucBytesWritten_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_FILE_WRITE_ABSOLUTE_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_FILE_WRITE_ABSOLUTE_ID & 0xFF);
   stMessage.aucData[2] = ucFileHandle_;
   stMessage.aucData[3] = getByte_ulLE(1,ulFileOffset_);
   stMessage.aucData[4] = getByte_ulLE(2,ulFileOffset_);
   stMessage.aucData[5] = getByte_ulLE(3,ulFileOffset_);
   stMessage.aucData[6] = getByte_ulLE(4,ulFileOffset_);
   stMessage.aucData[7] = ucWriteSize_;

   for(int i = 0; i < ucWriteSize_; i++)
      stMessage.aucData[MESG_FS_FILE_WRITE_ABSOLUTE_SIZE - 1 + i] = pucWriteBuffer_[i];

   SendFSRequest(MESG_FS_FILE_WRITE_ABSOLUTE_SIZE + ucWriteSize_, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   *ucBytesWritten_ = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD];
   return stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD];                             //return the Number of bytes successfully written
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::FileWriteRelative(UCHAR ucFileHandle_, UCHAR ucWriteSize_, const UCHAR *pucWriteBuffer_, UCHAR *ucBytesWritten_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_FILE_WRITE_RELATIVE_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_FILE_WRITE_RELATIVE_ID & 0xFF);
   stMessage.aucData[2] = ucFileHandle_;
   stMessage.aucData[3] = ucWriteSize_;

   for(int i = 0; i < ucWriteSize_; i++)
      stMessage.aucData[MESG_FS_FILE_WRITE_RELATIVE_SIZE - 1 + i] = pucWriteBuffer_[i];


   SendFSRequest(MESG_FS_FILE_WRITE_RELATIVE_SIZE + ucWriteSize_, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   *ucBytesWritten_ = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD];
   return stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD];                                             //return the Number of bytes successfully written
}

///////////////////////////////////////////////////////////////////////
ULONG DSIFramerANT::FileGetSize(UCHAR ucFileHandle_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_FILE_GET_SIZE_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_FILE_GET_SIZE_ID & 0xFF);
   stMessage.aucData[2] = ucFileHandle_;

   SendFSRequest(MESG_FS_FILE_GET_SIZE_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   return (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD]) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 1] << 8) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 2] << 16) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 3] << 24);
}

///////////////////////////////////////////////////////////////////////
ULONG DSIFramerANT::FileGetSizeInMem(UCHAR ucFileHandle_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_FILE_GET_SIZE_IN_MEM_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_FILE_GET_SIZE_IN_MEM_ID & 0xFF);
   stMessage.aucData[2] = ucFileHandle_;

   SendFSRequest(MESG_FS_FILE_GET_SIZE_IN_MEM_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   return (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD]) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 1] << 8) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 2] << 16) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 3] << 24);
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::FileGetSpecificFlags(UCHAR ucFileHandle_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_FILE_GET_SPECIFIC_FILE_FLAGS_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_FILE_GET_SPECIFIC_FILE_FLAGS_ID & 0xFF);
   stMessage.aucData[2] = ucFileHandle_;

   SendFSRequest(MESG_FS_FILE_GET_SPECIFIC_FILE_FLAGS_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   return stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD];
}

///////////////////////////////////////////////////////////////////////
ULONG DSIFramerANT::FileGetSystemTime(ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_SYSTEM_TIME_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_SYSTEM_TIME_ID & 0xFF);

   SendFSRequest(MESG_FS_SYSTEM_TIME_REQUEST_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   return (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD]) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 1] << 8) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 2] << 16) | (ULONG)(stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 3] << 24);
}

///////////////////////////////////////////////////////////////////////
/* FS-Crypto Commands */
///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::CryptoAddUserKeyIndex(UCHAR ucIndex_, UCHAR *pucKey_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_CRYPTO_ADD_USER_KEY_INDEX_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_CRYPTO_ADD_USER_KEY_INDEX_ID & 0xFF);
   stMessage.aucData[0] = ucIndex_;

   //copy Crypto Key into the aucData array starting at index 1 as index 0 is already used
   for(int i = 1; i < 33; i++)
      stMessage.aucData[i] = pucKey_[i - 1];

   if(SendFSCommand(&stMessage, MESG_FS_CRYPTO_ADD_USER_KEY_INDEX_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::CryptoSetUserKeyIndex(UCHAR ucIndex_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_CRYPTO_SET_USER_KEY_INDEX_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_CRYPTO_SET_USER_KEY_INDEX_ID & 0xFF);
   stMessage.aucData[0] = ucIndex_;

   if(SendFSCommand(&stMessage, MESG_FS_CRYPTO_SET_USER_KEY_INDEX_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::CryptoSetUserKeyVal(UCHAR *pucKey_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_CRYPTO_SET_USER_KEY_VAL_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_CRYPTO_SET_USER_KEY_VAL_ID & 0xFF);

   //copy Crypto Key into the aucData array starting at index 1 as index 0 is already used
   for(int i = 0; i < 33; i++)
      stMessage.aucData[i] = pucKey_[i];

   if(SendFSCommand(&stMessage, MESG_FS_CRYPTO_SET_USER_KEY_VAL_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}


///////////////////////////////////////////////////////////////////////
/* FIT Commands */
///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::FitFileIntegrityCheck(UCHAR ucFileHandle_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_FIT_FILE_INTEGRITY_CHECK_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_FIT_FILE_INTEGRITY_CHECK_ID & 0xFF);
   stMessage.aucData[0] = ucFileHandle_;

   if(SendFSCommand(&stMessage, MESG_FS_FIT_FILE_INTEGRITY_CHECK_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}



///////////////////////////////////////////////////////////////////////
/* ANTFS Commands */
///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::OpenBeacon(ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_ANTFS_OPEN_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_ANTFS_OPEN_ID & 0xFF);

   if(SendFSCommand(&stMessage, MESG_FS_ANTFS_OPEN_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::CloseBeacon(ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_ANTFS_CLOSE_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_ANTFS_CLOSE_ID & 0xFF);

   if(SendFSCommand(&stMessage, MESG_FS_ANTFS_CLOSE_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::ConfigBeacon(USHORT usDeviceType_, USHORT usManufacturer_, UCHAR ucAuthType_, UCHAR ucBeaconStatus_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_ANTFS_CONFIG_BEACON_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_ANTFS_CONFIG_BEACON_ID & 0xFF);
   stMessage.aucData[0] = getByte_ulLE(1,usDeviceType_);
   stMessage.aucData[1] = getByte_ulLE(2,usDeviceType_);
   stMessage.aucData[2] = getByte_ulLE(1,usManufacturer_);
   stMessage.aucData[3] = getByte_ulLE(2,usManufacturer_);
   stMessage.aucData[4] = ucAuthType_;
   stMessage.aucData[5] = ucBeaconStatus_;

   if(SendFSCommand(&stMessage, MESG_FS_ANTFS_CONFIG_BEACON_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::SetFriendlyName(UCHAR ucLength_, const UCHAR *pucString_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_ANTFS_SET_AUTH_STRING_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_ANTFS_SET_AUTH_STRING_ID & 0xFF);
   stMessage.aucData[0] = 0;                                                                            //send 0 for Friendlyname Command

   //Load up the String into the aucData starting at index 1
   for(int i = 1; i < ucLength_ + 1; i++)
      stMessage.aucData[i] = pucString_[i - 1];

   if(SendFSCommand(&stMessage, MESG_FS_ANTFS_SET_AUTH_STRING_SIZE + ucLength_, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::SetPasskey(UCHAR ucLength_, const UCHAR *pucString_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_ANTFS_SET_AUTH_STRING_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_ANTFS_SET_AUTH_STRING_ID & 0xFF);
   stMessage.aucData[0] = 1;                                                                            //send 1 for Passkey Command

   //Load up the String into the aucData starting at index 1
   for(int i = 1; i < ucLength_ + 1; i++)
      stMessage.aucData[i] = pucString_[i - 1];

   if(SendFSCommand(&stMessage, MESG_FS_ANTFS_SET_AUTH_STRING_SIZE + ucLength_, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::SetBeaconState(UCHAR ucBeaconStatus_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_ANTFS_SET_BEACON_STATE_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_ANTFS_SET_BEACON_STATE_ID & 0xFF);
   stMessage.aucData[0] = ucBeaconStatus_;

   if(SendFSCommand(&stMessage, MESG_FS_ANTFS_SET_BEACON_STATE_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::PairResponse(BOOL bAccept_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_ANTFS_PAIR_RESPONSE_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_ANTFS_PAIR_RESPONSE_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR) bAccept_;

   if(SendFSCommand(&stMessage, MESG_FS_ANTFS_PAIR_RESPONSE_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::SetLinkFrequency(UCHAR ucChannelNumber_, UCHAR ucFrequency_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_ANTFS_SET_LINK_FREQ_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_ANTFS_SET_LINK_FREQ_ID & 0xFF);
   stMessage.aucData[0] = ucChannelNumber_;
   stMessage.aucData[1] = ucFrequency_;

   if(SendFSCommand(&stMessage, MESG_FS_ANTFS_SET_LINK_FREQ_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::SetBeaconTimeout(UCHAR ucTimeout_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_ANTFS_SET_BEACON_TIMEOUT_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_ANTFS_SET_BEACON_TIMEOUT_ID & 0xFF);
   stMessage.aucData[0] = ucTimeout_;

   if(SendFSCommand(&stMessage, MESG_FS_ANTFS_SET_BEACON_TIMEOUT_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::SetPairingTimeout(UCHAR ucTimeout_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_ANTFS_SET_PAIRING_TIMEOUT_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_ANTFS_SET_PAIRING_TIMEOUT_ID & 0xFF);
   stMessage.aucData[0] = ucTimeout_;

   if(SendFSCommand(&stMessage, MESG_FS_ANTFS_SET_PAIRING_TIMEOUT_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::EnableRemoteFileCreate(BOOL bEnable_, ULONG ulResponseTime_)
{
   FS_MESSAGE stMessage;
   UCHAR ucResponse;

   stMessage.ucCommandID = (UCHAR)((MESG_FS_ANTFS_REMOTE_FILE_CREATE_EN_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_FS_ANTFS_REMOTE_FILE_CREATE_EN_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR) bEnable_;

   if(SendFSCommand(&stMessage, MESG_FS_ANTFS_REMOTE_FILE_CREATE_EN_SIZE, &ucResponse, ulResponseTime_))
   {
      ucFSResponse = ucResponse;                        //save FSResponse
      return ucFSResponse;
   }
   else
   {
      ucFSResponse = ucResponse;
      return ucFSResponse;
   }
}


///////////////////////////////////////////////////////////////////////
/* ANTFS REQUESTS */
///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::GetCmdPipe(UCHAR ucOffset_, UCHAR ucReadSize_, UCHAR *pucReadBuffer_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_ANTFS_GET_CMD_PIPE_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_ANTFS_GET_CMD_PIPE_ID & 0xFF);
   stMessage.aucData[2] = ucOffset_;
   stMessage.aucData[3] = ucReadSize_;


   SendFSRequest(MESG_FS_ANTFS_GET_CMD_PIPE_SIZE, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   //Now load pucBuffer_ with the Payload Array
   for(int i = 0; i < ucReadSize_; i++)
      pucReadBuffer_[i] = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD + 1 + i];                     //add 1 to skip SizeRead byte

   return stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD ];                    //return t
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::SetCmdPipe(UCHAR ucOffset_, UCHAR ucWriteSize_, const UCHAR *pucWriteBuffer_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   FS_MESSAGE stMessage;

   // Build the request message
   stMessage.ucCommandID = (UCHAR)((MESG_EXT_REQUEST_ID >> 8) & 0xFF);
   stMessage.ucMessageID = (UCHAR)(MESG_EXT_REQUEST_ID & 0xFF);
   stMessage.aucData[0] = (UCHAR)((MESG_FS_ANTFS_SET_CMD_PIPE_ID >> 8) & 0xFF);
   stMessage.aucData[1] = (UCHAR)(MESG_FS_ANTFS_SET_CMD_PIPE_ID & 0xFF);
   stMessage.aucData[2] = ucOffset_;
   stMessage.aucData[3] = ucWriteSize_;

   for(int i = 0; i < ucWriteSize_; i++)
      stMessage.aucData[MESG_FS_ANTFS_SET_CMD_PIPE_SIZE - 1 + i] = pucWriteBuffer_[i];

   SendFSRequest(MESG_FS_ANTFS_SET_CMD_PIPE_SIZE + ucWriteSize_, &stResponse, stMessage, ulResponseTime_);
   ucFSResponse = stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_FSRESPONSE];

   return stResponse.stANTMessage.aucData[OFFSET_REQUEST_RESPONSE_PAYLOAD];                             //return the Number of bytes successfully written
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIFramerANT::GetLastError()
{
   return ucFSResponse;
}


























