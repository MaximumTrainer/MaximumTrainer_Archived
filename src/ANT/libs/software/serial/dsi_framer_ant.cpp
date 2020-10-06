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


//////////////////////////////////////////////////////////////////////////////////
// Private Definitions
//////////////////////////////////////////////////////////////////////////////////

#define TX_FIFO_SIZE                          ((USHORT) 256)

#define ANT_DATA_CHANNEL_NUM_OFFSET           0
#define ANT_DATA_EVENT_ID_OFFSET              1
#define ANT_DATA_EVENT_CODE_OFFSET            2
//#define ANT_DATA_REQUESTED_MESG_ID_OFFSET     1

#define ANT_BASIC_CAPABILITIES_SIZE           4

//////////////////////////////////////////////////////////////////////////////////
// Public Class Functions
//////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
DSIFramerANT::DSIFramerANT()
{
   bInitOkay = TRUE;
   bClosing = FALSE;
   pbCancel = (volatile BOOL*)NULL;
   bSplitAdvancedBursts = FALSE;

   if (DSIThread_CondInit(&stCondMessageReady) != DSI_THREAD_ENONE)
      bInitOkay = FALSE;

   if (DSIThread_MutexInit(&stMutexCriticalSection) != DSI_THREAD_ENONE)
      bInitOkay = FALSE;

   if (DSIThread_MutexInit(&stMutexResponseRequest) != DSI_THREAD_ENONE)
      bInitOkay = FALSE;

   pclResponseListStart = (ANTMessageResponse*)NULL;

   Init((DSISerial*)NULL);
}

DSIFramerANT::DSIFramerANT(DSISerial *pclSerial_) : DSIFramer(pclSerial_)
{
   bInitOkay = TRUE;
   bClosing = FALSE;
   pbCancel = (volatile BOOL*)NULL;
   bSplitAdvancedBursts = FALSE;

   if (DSIThread_CondInit(&stCondMessageReady) != DSI_THREAD_ENONE)
      bInitOkay = FALSE;

   if (DSIThread_MutexInit(&stMutexCriticalSection) != DSI_THREAD_ENONE)
      bInitOkay = FALSE;

   if (DSIThread_MutexInit(&stMutexResponseRequest) != DSI_THREAD_ENONE)
      bInitOkay = FALSE;

   pclResponseListStart = (ANTMessageResponse*)NULL;

   Init(pclSerial_);
}
///////////////////////////////////////////////////////////////////////
DSIFramerANT::~DSIFramerANT()
{
   DSIThread_CondDestroy(&stCondMessageReady);
   DSIThread_MutexDestroy(&stMutexCriticalSection);
   DSIThread_MutexDestroy(&stMutexResponseRequest);
}

///////////////////////////////////////////////////////////////////////
void DSIFramerANT::SetSplitAdvBursts(BOOL bSplitAdvBursts_)
{
   bSplitAdvancedBursts = bSplitAdvBursts_;
}

///////////////////////////////////////////////////////////////////////
void DSIFramerANT::SetCancelParameter(volatile BOOL *pbCancel_)
{
   pbCancel = pbCancel_;
}

///////////////////////////////////////////////////////////////////////
volatile BOOL* DSIFramerANT::GetCancelParameter()
{
   return pbCancel;
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::Init(DSISerial *pclSerial_)
{
   ucRxIndex = 0;
   usMessageHead = 0;
   usMessageTail = 0;
   ucError = 0;

   if (pclSerial_ != NULL)
      pclSerial = pclSerial_;

   if (pclSerial == (DSISerial*)NULL)
      return FALSE;

   return bInitOkay;
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::WriteMessage(void *pvData_, USHORT usMessageSize_)
{
   UCHAR aucTxFifo[TX_FIFO_SIZE];
   UCHAR ucTotalSize;

   if (usMessageSize_ > MESG_MAX_SIZE_VALUE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("Framer->WriteMessage(): Failed, Msg Size > MESG_MAX_SIZE_VALUE.");
      #endif
      return FALSE;
   }

   ucTotalSize = (UCHAR) usMessageSize_ + MESG_HEADER_SIZE;
   aucTxFifo[0] = MESG_TX_SYNC;
   aucTxFifo[MESG_SIZE_OFFSET] = (UCHAR) usMessageSize_;
   aucTxFifo[MESG_ID_OFFSET] = ((ANT_MESSAGE *) pvData_)->ucMessageID;
   memcpy(&aucTxFifo[MESG_DATA_OFFSET], ((ANT_MESSAGE *) pvData_)->aucData, usMessageSize_);
   aucTxFifo[ucTotalSize] = CheckSum_Calc8(aucTxFifo, ucTotalSize);
   ++ucTotalSize;

   // Pad with two zeros.
   aucTxFifo[ucTotalSize++] = 0;
   aucTxFifo[ucTotalSize++] = 0;


   if (pclSerial->WriteBytes(aucTxFifo, ucTotalSize))
   {
      #if defined(SERIAL_DEBUG)
         if (aucTxFifo[MESG_ID_OFFSET] == 0x46)
            memset(&aucTxFifo[MESG_DATA_OFFSET+1],0x00,8);
         DSIDebug::SerialWrite(pclSerial->GetDeviceNumber(), "Tx", aucTxFifo, ucTotalSize);
      #endif
      return TRUE;
   }

   #if defined(SERIAL_DEBUG)
      if (aucTxFifo[MESG_ID_OFFSET] == 0x46)
         memset(&aucTxFifo[MESG_DATA_OFFSET+1],0x00,8);
      DSIDebug::SerialWrite(pclSerial->GetDeviceNumber(), "***Tx Error***", aucTxFifo, ucTotalSize);
   #endif

   return FALSE;
}

///////////////////////////////////////////////////////////////////////
USHORT DSIFramerANT::WaitForMessage(ULONG ulMilliseconds_)
{
   USHORT usMessageSize;

   DSIThread_MutexLock(&stMutexCriticalSection);

   usMessageSize = GetMessageSize();

   if ((usMessageSize == DSI_FRAMER_TIMEDOUT) && (ulMilliseconds_ != 0))
   {
      UCHAR ucStatus = DSIThread_CondTimedWait(&stCondMessageReady, &stMutexCriticalSection, ulMilliseconds_);
      if(ucStatus == DSI_THREAD_ENONE)
      {
         usMessageSize = GetMessageSize();
      }
      else if(ucStatus == DSI_THREAD_ETIMEDOUT)
      {
        usMessageSize = DSI_FRAMER_TIMEDOUT;
      }
      else //CondWait() failed
      {
        ucError = (UCHAR)(DSI_FRAMER_ERROR & 0xFF); //Set ucError so we can distinguish from a normal error if this ever occurs
        usMessageSize = DSI_FRAMER_ERROR;
      }
   }

   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return usMessageSize;
}

///////////////////////////////////////////////////////////////////////
USHORT DSIFramerANT::GetMessage(void *pvData_, USHORT usSize_)
{
   USHORT usRetVal;

   DSIThread_MutexLock(&stMutexCriticalSection);

   if (ucError)
   {
      ((ANT_MESSAGE *) pvData_)->ucMessageID = ucError;

      if (ucError == DSI_FRAMER_ANT_ESERIAL)
         ((ANT_MESSAGE *) pvData_)->aucData[0] = ucSerialError;

      ucError = 0;
      usRetVal = DSI_FRAMER_ERROR;
   }
   else
   {
      if ((usMessageHead - usMessageTail) != 0)
      {
         // Determine the number of bytes to copy.
         usRetVal = astMessageBuffer[usMessageTail].ucSize; // The reported number of bytes in the queue.

         if (usSize_ != 0)
            usRetVal = MIN(usRetVal, usSize_);              // If the usSize_ parameter is non-zero, limit the number of bytes copied from the queue to usSize_.

         if (usRetVal > MESG_MAX_SIZE_VALUE)                // Check to make sure we are not copying beyond the end of the message buffers
         {
            ((ANT_MESSAGE *) pvData_)->ucMessageID = DSI_FRAMER_ANT_EINVALID_SIZE;
            usRetVal = DSI_FRAMER_ERROR;
         }
         else
         {
            ((ANT_MESSAGE *) pvData_)->ucMessageID = astMessageBuffer[usMessageTail].stANTMessage.ucMessageID;
            memcpy(((ANT_MESSAGE *) pvData_)->aucData, astMessageBuffer[usMessageTail].stANTMessage.aucData, usRetVal);
         }

         usMessageTail++;                                   // Rollover of usMessageTail happens automagically because our buffer size is MAX_USHORT + 1.
      }
      else
      {
         usRetVal = DSI_FRAMER_TIMEDOUT;
      }
   }

   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return usRetVal;
}

///////////////////////////////////////////////////////////////////////
#define MESG_CHANNEL_OFFSET                  0
#define MESG_EVENT_ID_OFFSET                 1
UCHAR DSIFramerANT::GetChannelNumber(ANT_MESSAGE* pstMessage)
{
   // Get the channel number
   // Returns MAX_UCHAR if this message does not have a channel field
   UCHAR ucANTchannel = pstMessage->aucData[MESG_CHANNEL_OFFSET] & CHANNEL_NUMBER_MASK;

   // Some messages do not include the channel number in the response, so
   // they might get processed incorrectly
   if(pstMessage->ucMessageID == MESG_RESPONSE_EVENT_ID)
   {
      if(pstMessage->aucData[MESG_EVENT_ID_OFFSET] == MESG_NETWORK_KEY_ID ||  // we would need to look at the network number to figure it out
         pstMessage->aucData[MESG_EVENT_ID_OFFSET] == MESG_RADIO_TX_POWER_ID ||  // this affects all channels
         pstMessage->aucData[MESG_EVENT_ID_OFFSET] == MESG_RX_EXT_MESGS_ENABLE_ID)  // this affects all channels
      {
         return MAX_UCHAR;
      }
   }
   else if(pstMessage->ucMessageID == MESG_STARTUP_MESG_ID ||
      pstMessage->ucMessageID == MESG_CAPABILITIES_ID ||
      pstMessage->ucMessageID == MESG_VERSION_ID ||
      pstMessage->ucMessageID == MESG_GET_SERIAL_NUM_ID)
   {
      return MAX_UCHAR;
   }

   return ucANTchannel;
}


///////////////////////////////////////////////////////////////////////
void DSIFramerANT::ProcessByte(UCHAR ucByte_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if (ucRxIndex == 0)                                      // If we are looking for the start of a message.
   {
      if (ucByte_ == MESG_TX_SYNC)                          // If it is a valid first byte.
      {
         aucRxFifo[ucRxIndex++] = ucByte_;                  // Save it.
         ucCheckSum = ucByte_;                              // Initialize the checksum.
         ucRxSize = 2;                                      // We have to init high so we can read enough bytes to determine real length
      }
   }
   else if (ucRxIndex == 1)                                 // Determine RX message size.
   {
      aucRxFifo[ucRxIndex++] = ucByte_;                     // Save it.
      ucRxSize = ucByte_ + (MESG_FRAME_SIZE - MESG_SYNC_SIZE);  // We just got the length.
      ucCheckSum ^= ucByte_;                                // Calculate checksum.

      if ((USHORT)ucRxSize > RX_FIFO_SIZE)                          // If our buffer can't handle this message, turf it.
      {
         #if defined(SERIAL_DEBUG)
            DSIDebug::SerialWrite(pclSerial->GetDeviceNumber(), "ERROR: size > RX_FIFO_SIZE", aucRxFifo, ucRxIndex);
         #endif
         if (ucByte_ == MESG_TX_SYNC)
         {
            aucRxFifo[0] = ucByte_;                         // Save the byte.
            ucCheckSum = ucByte_;                           // Initialize the checksum.
            ucRxSize = 2;                                   // We have to init high so we can read enough bytes to determine real length
            ucRxIndex = 1;                                  // Set the Rx Index for the next iteration.
         }
         else
         {
            ucRxIndex = 0;                                  // Invalid size, so restart.
         }

      }
   }
   else
   {
      aucRxFifo[ucRxIndex] = ucByte_;                       // Save the byte.
      ucCheckSum ^= ucByte_;                                // Calculate checksum.

      if (ucRxIndex >= ucRxSize)                            // If we have received the whole message.
      {
         if (ucCheckSum == 0)                               // The CRC passed.
         {
            ProcessMessage();                               // Process the ANT message.
         }
         else
         {
            // Set a serial error for the bad crc.
            ucSerialError = DSI_FRAMER_ANT_CRC_ERROR;
            ucError = DSI_FRAMER_ANT_ESERIAL;
            DSIThread_CondSignal(&stCondMessageReady);
            #if defined(SERIAL_DEBUG)
               DSIDebug::SerialWrite(pclSerial->GetDeviceNumber(), "Bad CRC",aucRxFifo,ucRxIndex);
            #endif
         }
         ucRxIndex = 0;                                     // Reset the index.
      }
      else
      {
         ucRxIndex++;
      }
   }

   DSIThread_MutexUnlock(&stMutexCriticalSection);
}

///////////////////////////////////////////////////////////////////////
void DSIFramerANT::Error(UCHAR ucError_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   ucSerialError = ucError_;
   ucError = DSI_FRAMER_ANT_ESERIAL;

   DSIThread_CondSignal(&stCondMessageReady);

   DSIThread_MutexUnlock(&stMutexCriticalSection);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetNetworkKey(UCHAR ucNetworkNumber_, UCHAR *pucKey_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_NETWORK_KEY_ID;
   stMessage.aucData[0] = ucNetworkNumber_;
   memcpy(&stMessage.aucData[1], pucKey_, 8);

   return SendCommand(&stMessage, MESG_NETWORK_KEY_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::UnAssignChannel(UCHAR ucANTChannel_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_UNASSIGN_CHANNEL_ID;
   stMessage.aucData[0] = ucANTChannel_;

   return SendCommand(&stMessage, MESG_UNASSIGN_CHANNEL_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::AssignChannel(UCHAR ucANTChannel_, UCHAR ucChannelType_, UCHAR ucNetworkNumber_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_ASSIGN_CHANNEL_ID;
   stMessage.aucData[0] = ucANTChannel_;
   stMessage.aucData[1] = ucChannelType_;
   stMessage.aucData[2] = ucNetworkNumber_;

   return SendCommand(&stMessage, MESG_ASSIGN_CHANNEL_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::AssignChannelExt(UCHAR ucANTChannel_, UCHAR* pucChannelType_, UCHAR ucSize_, UCHAR ucNetworkNumber_, ULONG ulResponseTime_)
{
   if( (pucChannelType_ == NULL) || (ucSize_ < 1) )
      return FALSE;


   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_ASSIGN_CHANNEL_ID;
   stMessage.aucData[0] = ucANTChannel_;
   stMessage.aucData[1] = pucChannelType_[0];
   stMessage.aucData[2] = ucNetworkNumber_;

   for(UCHAR i=1; i<ucSize_; i++)
      stMessage.aucData[2+i] = pucChannelType_[i];

   return SendCommand(&stMessage, ucSize_ + 2, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetChannelID(UCHAR ucANTChannel_, USHORT usDeviceNumber_, UCHAR ucDeviceType_, UCHAR ucTransmitType_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_CHANNEL_ID_ID;
   stMessage.aucData[0] = ucANTChannel_;
   stMessage.aucData[1] = (UCHAR)(usDeviceNumber_ & 0xFF);
   stMessage.aucData[2] = (UCHAR)((usDeviceNumber_ >>8) & 0xFF);
   stMessage.aucData[3] = ucDeviceType_;
   stMessage.aucData[4] = ucTransmitType_;

   return SendCommand(&stMessage, MESG_CHANNEL_ID_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetChannelPeriod(UCHAR ucANTChannel_, USHORT usMessagePeriod_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_CHANNEL_MESG_PERIOD_ID;
   stMessage.aucData[0] = ucANTChannel_;
   stMessage.aucData[1] = (UCHAR)(usMessagePeriod_ & 0xFF);
   stMessage.aucData[2] = (UCHAR)((usMessagePeriod_ >>8) & 0xFF);

   return SendCommand(&stMessage, MESG_CHANNEL_MESG_PERIOD_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetFastSearch(UCHAR ucANTChannel_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = 0x49;
   stMessage.aucData[0] = ucANTChannel_;
   stMessage.aucData[1] = 86;
   stMessage.aucData[2] = 0;

   return SendCommand(&stMessage, 3, ulResponseTime_);
}




///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetLowPriorityChannelSearchTimeout(UCHAR ucANTChannel_, UCHAR ucSearchTimeout_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_SET_LP_SEARCH_TIMEOUT_ID;
   stMessage.aucData[0] = ucANTChannel_;
   stMessage.aucData[1] = ucSearchTimeout_;

   return SendCommand(&stMessage, MESG_SET_LP_SEARCH_TIMEOUT_SIZE, ulResponseTime_);
}


///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetChannelSearchTimeout(UCHAR ucANTChannel_, UCHAR ucSearchTimeout_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_CHANNEL_SEARCH_TIMEOUT_ID;
   stMessage.aucData[0] = ucANTChannel_;
   stMessage.aucData[1] = ucSearchTimeout_;

   return SendCommand(&stMessage, MESG_CHANNEL_SEARCH_TIMEOUT_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetChannelRFFrequency(UCHAR ucANTChannel_, UCHAR ucRFFrequency_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_CHANNEL_RADIO_FREQ_ID;
   stMessage.aucData[0] = ucANTChannel_;
   stMessage.aucData[1] = ucRFFrequency_;

   return SendCommand(&stMessage, MESG_CHANNEL_RADIO_FREQ_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::CrystalEnable(ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_XTAL_ENABLE_ID;
   stMessage.aucData[0]  = 0;

   return SendCommand(&stMessage, MESG_XTAL_ENABLE_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetLibConfig(UCHAR ucLibConfigFlags_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_ANTLIB_CONFIG_ID;
   stMessage.aucData[0]  = 0;
   stMessage.aucData[1]  = ucLibConfigFlags_;

   return SendCommand(&stMessage, MESG_ANTLIB_CONFIG_SIZE, ulResponseTime_);
}


///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetProximitySearch(UCHAR ucANTChannel_, UCHAR ucSearchThreshold_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_PROX_SEARCH_CONFIG_ID;
   stMessage.aucData[0]  = ucANTChannel_;
   stMessage.aucData[1]  = ucSearchThreshold_;

   return SendCommand(&stMessage, MESG_PROX_SEARCH_CONFIG_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ConfigEventBuffer(UCHAR ucConfig_, USHORT usSize_, USHORT usTime_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_EVENT_BUFFERING_CONFIG_ID;
   stMessage.aucData[0] = 0;
   stMessage.aucData[1] = ucConfig_;
   stMessage.aucData[2] = (UCHAR)(usSize_ & 0xFF);
   stMessage.aucData[3] = (UCHAR)((usSize_ >>8) & 0xFF);
   stMessage.aucData[4] = (UCHAR)(usTime_ & 0xFF);
   stMessage.aucData[5] = (UCHAR)((usTime_ >>8) & 0xFF);

   return SendCommand(&stMessage, MESG_EVENT_BUFFERING_CONFIG_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ConfigEventFilter(USHORT usEventFilter_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_EVENT_FILTER_CONFIG_ID;
   stMessage.aucData[0] = 0;
   stMessage.aucData[1] = (UCHAR)(usEventFilter_ & 0xFF);
   stMessage.aucData[2] = (UCHAR)((usEventFilter_ >>8) & 0xFF);

   return SendCommand(&stMessage, MESG_EVENT_FILTER_CONFIG_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ConfigHighDutySearch(UCHAR ucEnable_, UCHAR ucSuppressionCycles_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_HIGH_DUTY_SEARCH_MODE_ID;
   stMessage.aucData[0] = 0;
   stMessage.aucData[1] = ucEnable_;
   stMessage.aucData[2] = ucSuppressionCycles_;

   return SendCommand(&stMessage, MESG_HIGH_DUTY_SEARCH_MODE_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ConfigSelectiveDataUpdate(UCHAR ucANTChannel_, UCHAR ucSduConfig_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_SDU_CONFIG_ID;
   stMessage.aucData[0] = ucANTChannel_;
   stMessage.aucData[1] = ucSduConfig_;

   return SendCommand(&stMessage, MESG_SDU_CONFIG_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetSelectiveDataUpdateMask(UCHAR ucMaskNumber_, UCHAR* pucSduMask_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_SDU_SET_MASK_ID;
   stMessage.aucData[0] = ucMaskNumber_;
   memcpy(&stMessage.aucData[1], pucSduMask_, 8);

   return SendCommand(&stMessage, MESG_SDU_SET_MASK_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ConfigUserNVM(USHORT usAddress_, UCHAR* pucData_, UCHAR ucSize_, ULONG ulResponseTime_)
{
   if ((ucSize_ + MESG_USER_CONFIG_PAGE_SIZE) <= MESG_MAX_SIZE_VALUE)
   {
      ANT_MESSAGE stMessage;

      stMessage.ucMessageID = MESG_USER_CONFIG_PAGE_ID;
      stMessage.aucData[0] = 0;
      // Addr is passed in LE
      stMessage.aucData[1] = (UCHAR)(usAddress_ & 0xFF);
      stMessage.aucData[2] = (UCHAR)((usAddress_ >>8) & 0xFF);
      memcpy(&stMessage.aucData[3], pucData_, ucSize_);

      return SendCommand(&stMessage, MESG_USER_CONFIG_PAGE_SIZE+ucSize_, ulResponseTime_);
   }
   else
      return false;
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetChannelSearchPriority(UCHAR ucANTChannel_, UCHAR ucPriorityLevel_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_SET_SEARCH_CH_PRIORITY_ID;
   stMessage.aucData[0] = ucANTChannel_;
   stMessage.aucData[1] = ucPriorityLevel_;

   return SendCommand(&stMessage, MESG_SET_SEARCH_CH_PRIORITY_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetSearchSharingCycles(UCHAR ucANTChannel_, UCHAR ucSearchSharingCycles_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_ACTIVE_SEARCH_SHARING_ID;
   stMessage.aucData[0] = ucANTChannel_;
   stMessage.aucData[1] = ucSearchSharingCycles_;

   return SendCommand(&stMessage, MESG_ACTIVE_SEARCH_SHARING_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SleepMessage(ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_SLEEP_ID;
   stMessage.aucData[0]  = 0;

   return SendCommand(&stMessage, MESG_SLEEP_SIZE, ulResponseTime_);
}



///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ConfigFrequencyAgility(UCHAR ucANTChannel_, UCHAR ucFreq1_, UCHAR ucFreq2_, UCHAR ucFreq3_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_AUTO_FREQ_CONFIG_ID;
   stMessage.aucData[0]  = ucANTChannel_;
   stMessage.aucData[1]  = ucFreq1_;
   stMessage.aucData[2]  = ucFreq2_;
   stMessage.aucData[3]  = ucFreq3_;

   return SendCommand(&stMessage, MESG_AUTO_FREQ_CONFIG_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetAllChannelsTransmitPower(UCHAR ucTransmitPower_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_RADIO_TX_POWER_ID;
   stMessage.aucData[0]  = 0;
   stMessage.aucData[1]  = ucTransmitPower_;

   return SendCommand(&stMessage, MESG_RADIO_TX_POWER_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetChannelTransmitPower(UCHAR ucANTChannel_,UCHAR ucTransmitPower_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_CHANNEL_RADIO_TX_POWER_ID;
   stMessage.aucData[0]  = ucANTChannel_;
   stMessage.aucData[1]  = ucTransmitPower_;

   return SendCommand(&stMessage, MESG_CHANNEL_RADIO_TX_POWER_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::InitCWTestMode(ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_RADIO_CW_INIT_ID;
   stMessage.aucData[0]  = 0;

   return SendCommand(&stMessage, MESG_RADIO_CW_INIT_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetCWTestMode(UCHAR ucTransmitPower_, UCHAR ucRFFreq_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_RADIO_CW_MODE_ID;
   stMessage.aucData[0]  = 0;
   stMessage.aucData[1]  = ucTransmitPower_;
   stMessage.aucData[2]  = ucRFFreq_;

   return SendCommand(&stMessage, MESG_RADIO_CW_MODE_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::AddChannelID(UCHAR ucANTChannel_, USHORT usDeviceNumber_,UCHAR ucDeviceType_, UCHAR ucTransmissionType_, UCHAR ucListIndex_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_ID_LIST_ADD_ID;
   stMessage.aucData[0]  = ucANTChannel_;
   stMessage.aucData[1] = (UCHAR)(usDeviceNumber_ & 0xFF);
   stMessage.aucData[2] = (UCHAR)((usDeviceNumber_ >>8) & 0xFF);
   stMessage.aucData[3] = ucDeviceType_;
   stMessage.aucData[4] = ucTransmissionType_;
   stMessage.aucData[5] = ucListIndex_;


   return SendCommand(&stMessage, MESG_ID_LIST_ADD_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::AddCryptoID(UCHAR ucANTChannel_, UCHAR* pucData_, UCHAR ucListIndex_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_CRYPTO_ID_LIST_ADD_ID;
   stMessage.aucData[0]  = ucANTChannel_;
   memcpy(&stMessage.aucData[1], pucData_, 4);
   stMessage.aucData[5] = ucListIndex_;

   return SendCommand(&stMessage, MESG_CRYPTO_ID_LIST_ADD_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ConfigList(UCHAR ucANTChannel_, UCHAR ucListSize_, UCHAR ucExclude_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_ID_LIST_CONFIG_ID;
   stMessage.aucData[0]  = ucANTChannel_;
   stMessage.aucData[1] = ucListSize_;
   stMessage.aucData[2] = ucExclude_;

   return SendCommand(&stMessage, MESG_CRYPTO_ID_LIST_CONFIG_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ConfigCryptoList(UCHAR ucANTChannel_, UCHAR ucListSize_, UCHAR ucBlacklist_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_CRYPTO_ID_LIST_CONFIG_ID;
   stMessage.aucData[0]  = ucANTChannel_;
   stMessage.aucData[1] = ucListSize_;
   stMessage.aucData[2] = ucBlacklist_;

   return SendCommand(&stMessage, MESG_ID_LIST_CONFIG_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::OpenRxScanMode( ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_OPEN_RX_SCAN_ID;
   stMessage.aucData[0]  = 0;

   return SendCommand(&stMessage, MESG_OPEN_RX_SCAN_SIZE, ulResponseTime_);
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to construct and send an extended broadcast data message.
// This message will be broadcast on the next synchronous channel period.
///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SendExtBroadcastData(UCHAR ucANTChannel_, UCHAR *pucData_)
{

   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_EXT_BROADCAST_DATA_ID;
   stMessage.aucData[0] = ucANTChannel_;
   memcpy(&stMessage.aucData[1],pucData_, MESG_EXT_DATA_SIZE-1);

   return WriteMessage(&stMessage, MESG_EXT_DATA_SIZE);



}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to construct and send an extended acknowledged data
// mesg.  This message will be transmitted on the next synchronous channel
// period.
///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SendExtAcknowledgedData(UCHAR ucANTChannel_, UCHAR *pucData_, ULONG ulResponseTime_)
{

   return(SetupAckDataTransfer(
      MESG_EXT_ACKNOWLEDGED_DATA_ID,
      ucANTChannel_,
      pucData_,
      MESG_EXT_DATA_SIZE,
      ulResponseTime_));
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Used to send extended burst data with individual packets.  Proper sequence number
// of packet is maintained by the application.
///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SendExtBurstTransferPacket(UCHAR ucANTChannelSeq_, UCHAR *pucData_)
{

   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_EXT_BURST_DATA_ID;
   stMessage.aucData[0] = ucANTChannelSeq_;
   memcpy(&stMessage.aucData[1],pucData_, MESG_EXT_DATA_SIZE-1);

   return WriteMessage(&stMessage, MESG_EXT_DATA_SIZE);
}



///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Used to send extended burst data using a block of data.  Proper sequence number
// of packet is maintained by the function.  Useful for testing purposes.
///////////////////////////////////////////////////////////////////////
ANTFRAMER_RETURN DSIFramerANT::SendExtBurstTransfer(UCHAR ucANTChannel_, UCHAR *pucData_, ULONG ulSize_, ULONG ulResponseTime_)
{
   return(SetupBurstDataTransfer(
      MESG_EXT_BURST_DATA_ID,
      ucANTChannel_,
      pucData_,
      ulSize_,
      MESG_EXT_DATA_SIZE,
      ulResponseTime_));
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Used to force the module to use extended rx messages all the time
///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::RxExtMesgsEnable(UCHAR ucEnable_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_RX_EXT_MESGS_ENABLE_ID;
   stMessage.aucData[0]  = 0;
   stMessage.aucData[1]  = ucEnable_;

   return SendCommand(&stMessage, MESG_RX_EXT_MESGS_ENABLE_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Used to set a channel device ID to the module serial number
///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetSerialNumChannelId(UCHAR ucANTChannel_, UCHAR ucDeviceType_, UCHAR ucTransmissionType_,  ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_SERIAL_NUM_SET_CHANNEL_ID_ID;
   stMessage.aucData[0]  = ucANTChannel_;
   stMessage.aucData[1]  = ucDeviceType_;
   stMessage.aucData[2]  = ucTransmissionType_;

   return SendCommand(&stMessage, MESG_SERIAL_NUM_SET_CHANNEL_ID_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Enables the module LED to flash on RF activity
///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::EnableLED(UCHAR ucEnable_, ULONG ulResponseTime_)
{

   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_ENABLE_LED_FLASH_ID;
   stMessage.aucData[0]  = 0;
   stMessage.aucData[1]  = ucEnable_;

   return SendCommand(&stMessage, MESG_ENABLE_LED_FLASH_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ConfigAdvancedBurst_ext (BOOL bEnable_, UCHAR ucMaxPacketLength_,
                                        ULONG ulRequiredFields_, ULONG ulOptionalFields_,
                                        USHORT usStallCount_, UCHAR ucRetryCount_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;
   stMessage.ucMessageID = MESG_CONFIG_ADV_BURST_ID;
   stMessage.aucData[0] = 0;
   stMessage.aucData[1] = (bEnable_) ? 1 : 0;
   stMessage.aucData[2] = ucMaxPacketLength_;
   stMessage.aucData[3] = (UCHAR)(ulRequiredFields_ & 0xFF);
   stMessage.aucData[4] = (UCHAR)((ulRequiredFields_ >> 8) & 0xFF);
   stMessage.aucData[5] = (UCHAR)((ulRequiredFields_ >> 16) & 0xFF);
   stMessage.aucData[6] = (UCHAR)(ulOptionalFields_ & 0xFF);
   stMessage.aucData[7] = (UCHAR)((ulOptionalFields_ >> 8) & 0xFF);
   stMessage.aucData[8] = (UCHAR)((ulOptionalFields_ >> 16) & 0xFF);
   stMessage.aucData[9] = (UCHAR)(usStallCount_ & 0xFF);
   stMessage.aucData[10] = (UCHAR)((usStallCount_ >> 8) & 0xFF);
   stMessage.aucData[11] = ucRetryCount_;

   return SendCommand(&stMessage, MESG_CONFIG_ADV_BURST_SIZE_EXT, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ConfigAdvancedBurst (BOOL bEnable_, UCHAR ucMaxPacketLength_,
                                        ULONG ulRequiredFields_, ULONG ulOptionalFields_,
                                        ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;
   stMessage.ucMessageID = MESG_CONFIG_ADV_BURST_ID;
   stMessage.aucData[0] = 0;
   stMessage.aucData[1] = (bEnable_) ? 1 : 0;
   stMessage.aucData[2] = ucMaxPacketLength_;
   stMessage.aucData[3] = (UCHAR)(ulRequiredFields_ & 0xFF);
   stMessage.aucData[4] = (UCHAR)((ulRequiredFields_ >> 8) & 0xFF);
   stMessage.aucData[5] = (UCHAR)((ulRequiredFields_ >> 16) & 0xFF);
   stMessage.aucData[6] = (UCHAR)(ulOptionalFields_ & 0xFF);
   stMessage.aucData[7] = (UCHAR)((ulOptionalFields_ >> 8) & 0xFF);
   stMessage.aucData[8] = (UCHAR)((ulOptionalFields_ >> 16) & 0xFF);

   return SendCommand(&stMessage, MESG_CONFIG_ADV_BURST_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::EncryptedChannelEnable(UCHAR ucANTChannel_, UCHAR ucMode_,
                                          UCHAR ucVolatileKeyIndex_, UCHAR ucDecimationRate_,
                                          ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;
   stMessage.ucMessageID = MESG_ENCRYPT_ENABLE_ID;
   stMessage.aucData[0] = ucANTChannel_;
   stMessage.aucData[1] = ucMode_;
   stMessage.aucData[2] = ucVolatileKeyIndex_;
   stMessage.aucData[3] = ucDecimationRate_;

   return SendCommand(&stMessage, MESG_ENCRYPT_ENABLE_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetCryptoKey(UCHAR ucVolatileKeyIndex_, UCHAR *pucKey_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;
   stMessage.ucMessageID = MESG_SET_CRYPTO_KEY_ID;
   stMessage.aucData[0] = ucVolatileKeyIndex_;
   memcpy(&stMessage.aucData[1], pucKey_, 16);

   return SendCommand(&stMessage, MESG_SET_CRYPTO_KEY_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetCryptoID(UCHAR *pucData_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;
   stMessage.ucMessageID = MESG_SET_CRYPTO_INFO_ID;
   stMessage.aucData[0] = 0;
   memcpy(&stMessage.aucData[1], pucData_, 4);

   return SendCommand(&stMessage, MESG_SET_CRYPTO_ID_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetCryptoUserInfo(UCHAR *pucData_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;
   stMessage.ucMessageID = MESG_SET_CRYPTO_INFO_ID;
   stMessage.aucData[0] = 1;
   memcpy(&stMessage.aucData[1], pucData_, 19);

   return SendCommand(&stMessage, MESG_SET_CRYPTO_USER_INFO_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetCryptoRNGSeed(UCHAR *pucData_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;
   stMessage.ucMessageID = MESG_SET_CRYPTO_INFO_ID;
   stMessage.aucData[0] = 2;
   memcpy(&stMessage.aucData[1], pucData_, 16);

   return SendCommand(&stMessage, MESG_SET_CRYPTO_RNG_SEED_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetCryptoInfo(UCHAR ucSetParameter_, UCHAR *pucData_, ULONG ulResponseTime_)
{
   switch(ucSetParameter_)
   {
      case 0:
      default:
         return SetCryptoID(pucData_, ulResponseTime_);
      case 1:
         return SetCryptoUserInfo(pucData_, ulResponseTime_);
      case 2:
         return SetCryptoRNGSeed(pucData_, ulResponseTime_);
   }
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::LoadCryptoKeyNVMOp(UCHAR ucNVMKeyIndex_, UCHAR ucVolatileKeyIndex_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;
   stMessage.ucMessageID = MESG_NVM_CRYPTO_KEY_OPS_ID;
   stMessage.aucData[0] = 0;
   stMessage.aucData[1] = ucNVMKeyIndex_;
   stMessage.aucData[2] = ucVolatileKeyIndex_;

   return SendCommand(&stMessage, MESG_NVM_CRYPTO_KEY_LOAD_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::StoreCryptoKeyNVMOp(UCHAR ucNVMKeyIndex_, UCHAR *pucKey_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;
   stMessage.ucMessageID = MESG_NVM_CRYPTO_KEY_OPS_ID;
   stMessage.aucData[0] = 1;
   stMessage.aucData[1] = ucNVMKeyIndex_;
   memcpy(&stMessage.aucData[2], pucKey_, 16);

   return SendCommand(&stMessage, MESG_NVM_CRYPTO_KEY_STORE_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::CryptoKeyNVMOp(UCHAR ucOperation_, UCHAR ucNVMKeyIndex_, UCHAR *pucData_, ULONG ulResponseTime_)
{
   switch(ucOperation_)
   {
      case 0:
      default:
         return LoadCryptoKeyNVMOp(ucNVMKeyIndex_, *pucData_, ulResponseTime_);
      case 1:
         return StoreCryptoKeyNVMOp(ucNVMKeyIndex_, pucData_, ulResponseTime_);
   }
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::RequestMessage(UCHAR ucChannel_, UCHAR ucMessageID_)
{
   ANT_MESSAGE_ITEM stResponse;

   if (SendRequest(ucMessageID_, ucChannel_, &stResponse, 0) == FALSE)
      return FALSE;

   return TRUE;
}


///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ScriptWrite( UCHAR ucSize_, UCHAR *pucCmdData_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_SCRIPT_DATA_ID;

   memcpy(stMessage.aucData, pucCmdData_, ucSize_);
   return SendCommand(&stMessage, ucSize_, ulResponseTime_);
}



///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ScriptClear(ULONG ulResponseTime_ )
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_SCRIPT_CMD_ID;
   stMessage.aucData[0]  = 0;
   stMessage.aucData[1]  = SCRIPT_CMD_FORMAT;
   stMessage.aucData[2]  = 0;

   return SendCommand(&stMessage, MESG_SCRIPT_CMD_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ScriptSetDefaultSector( UCHAR ucSectNumber_,  ULONG ulResponseTime_ )
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_SCRIPT_CMD_ID;
   stMessage.aucData[0]  = 0;
   stMessage.aucData[1]  = SCRIPT_CMD_SET_DEFAULT_SECTOR;
   stMessage.aucData[2]  = ucSectNumber_;

   return SendCommand(&stMessage, MESG_SCRIPT_CMD_SIZE, ulResponseTime_);
}


///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ScriptEndSector( ULONG ulResponseTime_ )
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_SCRIPT_CMD_ID;
   stMessage.aucData[0]  = 0;
   stMessage.aucData[1]  = SCRIPT_CMD_END_SECTOR;
   stMessage.aucData[2]  = 0;

   return SendCommand(&stMessage, MESG_SCRIPT_CMD_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ScriptDump( ULONG ulResponseTime_ )
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_SCRIPT_CMD_ID;
   stMessage.aucData[0]  = 0;
   stMessage.aucData[1]  = SCRIPT_CMD_DUMP;
   stMessage.aucData[2]  = 0;

   return SendCommand(&stMessage, MESG_SCRIPT_CMD_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ScriptLock( ULONG ulResponseTime_ )
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_SCRIPT_CMD_ID;
   stMessage.aucData[0]  = 0;
   stMessage.aucData[1]  = SCRIPT_CMD_LOCK;
   stMessage.aucData[2]  = 0;

   return SendCommand(&stMessage, MESG_SCRIPT_CMD_SIZE, ulResponseTime_);
}


///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::FITSetFEState(UCHAR ucFEState_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_FIT1_SET_EQUIP_STATE_ID;
   stMessage.aucData[0] = 0;
   stMessage.aucData[1] = ucFEState_;

   return SendCommand(&stMessage, MESG_FIT1_SET_EQUIP_STATE_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::FITAdjustPairingSettings(UCHAR ucSearchLv_, UCHAR ucPairLv_, UCHAR ucTrackLv_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_FIT1_SET_AGC_ID;
   stMessage.aucData[0] = 0;
   stMessage.aucData[1] = ucSearchLv_;
   stMessage.aucData[2] = ucPairLv_;
   stMessage.aucData[3] = ucTrackLv_;

   return SendCommand(&stMessage, MESG_FIT1_SET_AGC_SIZE, ulResponseTime_);
}


///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::GetCapabilities(UCHAR *pucCapabilities_, ULONG ulResponseTime_)
{
   //Get Capabilities using new Extended method
   UCHAR ucBasicCapabilities = ANT_BASIC_CAPABILITIES_SIZE;
   return GetCapabilitiesExt(pucCapabilities_, &ucBasicCapabilities, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::GetCapabilitiesExt(UCHAR *pucCapabilities_, UCHAR *pucMaxBytes_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;
   UCHAR ucBytes;
   UCHAR ucCount;

  //Sanity check because we can't populate the array if we don't know how big it is.
  if ( pucCapabilities_ != NULL && pucMaxBytes_ == NULL )
      return FALSE;

   //Send request, if it returns successfully copy the array
   if (SendRequest(MESG_CAPABILITIES_ID, 0, &stResponse, ulResponseTime_) == FALSE)
      return FALSE;

   if (pucCapabilities_ == NULL || ulResponseTime_ == 0)
      return TRUE;

   //If Max Bytes > Response then use response size, else limit the size based on how many bytes the buffer can handle
   ucBytes = (*pucMaxBytes_ > stResponse.ucSize) ? stResponse.ucSize : *pucMaxBytes_;

   for (ucCount = 0; ucCount < ucBytes; ucCount++)
   {
      *pucCapabilities_++ = stResponse.stANTMessage.aucData[ucCount];
   }

   //Update the pointer to indicate how many bytes we copied.
   *pucMaxBytes_ = ucBytes;
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::GetChannelID(UCHAR ucANTChannel_, USHORT *pusDeviceNumber_, UCHAR *pucDeviceType_, UCHAR *pucTransmitType_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;

   if (SendRequest(MESG_CHANNEL_ID_ID, ucANTChannel_, &stResponse, ulResponseTime_) == FALSE)
      return FALSE;

   if(pusDeviceNumber_ == NULL || pucDeviceType_ == NULL || pucTransmitType_ == NULL || ulResponseTime_ == 0)
      return TRUE;

   *pusDeviceNumber_ = ((USHORT)stResponse.stANTMessage.aucData[1]|(USHORT)stResponse.stANTMessage.aucData[2]<<8);
   *pucDeviceType_   = stResponse.stANTMessage.aucData[3];
   *pucTransmitType_ = stResponse.stANTMessage.aucData[4];
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::GetChannelStatus(UCHAR ucANTChannel_, UCHAR *pucStatus_, ULONG ulResponseTime_)
{
   ANT_MESSAGE_ITEM stResponse;

   if (SendRequest(MESG_CHANNEL_STATUS_ID, ucANTChannel_, &stResponse, ulResponseTime_) == FALSE)
      return FALSE;

   if (pucStatus_ == NULL || ulResponseTime_ == 0)
      return TRUE;

   *pucStatus_ = stResponse.stANTMessage.aucData[1];
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::ResetSystem(ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_SYSTEM_RESET_ID;
   stMessage.aucData[0] = 0;


   //// Modified SendCommand() function. ////

   ANTMessageResponse *pclCommandResponse = (ANTMessageResponse*)NULL;

   // If we are going to be waiting for a response setup the Response object
   if (ulResponseTime_ != 0)
   {
      pclCommandResponse = new ANTMessageResponse();
      pclCommandResponse->Attach(MESG_STARTUP_MESG_ID, (UCHAR*)NULL, 0, this);
   }

   // Write the command message.
   if (!WriteMessage(&stMessage, MESG_SYSTEM_RESET_SIZE))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("Framer->ResetSystem():  WriteMessage Failed.");
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
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("Framer->ResetSystem():  Timeout.");
      #endif

      delete pclCommandResponse;
      return FALSE;
   }

   delete pclCommandResponse;
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::OpenChannel(UCHAR ucANTChannel_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_OPEN_CHANNEL_ID;
   stMessage.aucData[0]  = ucANTChannel_;

   return SendCommand(&stMessage, MESG_OPEN_CHANNEL_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetRSSISearchThreshold(UCHAR ucANTChannel_, UCHAR ucSearchThreshold_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_RSSI_SEARCH_THRESHOLD_ID;
   stMessage.aucData[0]  = ucANTChannel_;
   stMessage.aucData[1]  = ucSearchThreshold_;

   return SendCommand(&stMessage, MESG_RSSI_SEARCH_THRESHOLD_SIZE, ulResponseTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::CloseChannel(UCHAR ucANTChannel_, ULONG ulResponseTime_)
{
   BOOL bReturn;
   ANT_MESSAGE stMessage;
   ANTMessageResponse *pclEventResponse = (ANTMessageResponse*)NULL;

   stMessage.ucMessageID = MESG_CLOSE_CHANNEL_ID;
   stMessage.aucData[0]  = ucANTChannel_;

   // If we are going to be waiting for a response setup the Response object to wait for the channel closed event
   if (ulResponseTime_ != 0)
   {
      UCHAR aucDesiredData[3];

      aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = ucANTChannel_;
      aucDesiredData[ANT_DATA_EVENT_ID_OFFSET] = MESG_EVENT_ID;
      aucDesiredData[ANT_DATA_EVENT_CODE_OFFSET] = EVENT_CHANNEL_CLOSED;

      pclEventResponse = new ANTMessageResponse();
      pclEventResponse->Attach(MESG_RESPONSE_EVENT_ID, aucDesiredData, sizeof(aucDesiredData), this);
   }

   bReturn = SendCommand(&stMessage, MESG_CLOSE_CHANNEL_SIZE, ulResponseTime_);

   if (ulResponseTime_ == 0)
      return bReturn;

   if (bReturn == TRUE)
   {
      // Wait for the close event.
      bReturn = pclEventResponse->WaitForResponse(ulResponseTime_);
   }

   pclEventResponse->Remove();                                               //detach from list
   delete pclEventResponse;

   return bReturn;
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SendBroadcastData(UCHAR ucANTChannel_, UCHAR *pucData_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_BROADCAST_DATA_ID;
   stMessage.aucData[0] = ucANTChannel_;
   memcpy(&stMessage.aucData[1],pucData_, ANT_STANDARD_DATA_PAYLOAD_SIZE);

   return WriteMessage(&stMessage, ANT_STANDARD_DATA_PAYLOAD_SIZE + MESG_CHANNEL_NUM_SIZE);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SendBurstDataPacket(UCHAR ucANTChannelSeq_, UCHAR *pucData_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_BURST_DATA_ID;
   stMessage.aucData[0] = ucANTChannelSeq_;
   memcpy(&stMessage.aucData[1],pucData_, MESG_DATA_SIZE-1);

   return WriteMessage(&stMessage, MESG_DATA_SIZE);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SendAdvancedBurstDataPacket(UCHAR ucANTChannelSeq_, UCHAR *pucData_, UCHAR ucStdPcktsPerSerialMsg_)
{
   ANT_MESSAGE* stMessage = (ANT_MESSAGE*) NULL;
   USHORT packetDataSize = (MESG_DATA_SIZE-1) * ucStdPcktsPerSerialMsg_;
   if(!CreateAntMsg_wOptExtBuf(&stMessage, packetDataSize+1))  //reqMinSize = packetSize + seqNum
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("Framer->SendAdvancedBurstDataPacket(): Failed, ucStdPcktsPerSerialMsg too big for this part");
      #endif
      return FALSE;
   }

   stMessage->ucMessageID = MESG_ADV_BURST_DATA_ID;
   stMessage->aucData[0] = ucANTChannelSeq_;
   memcpy(&(stMessage->aucData[1]),pucData_, packetDataSize);

   BOOL result = WriteMessage(stMessage, 1 + packetDataSize);  //seqNum + (payloadLength*pcktsPerSerial)
   delete[] stMessage;
   return result;
}

///////////////////////////////////////////////////////////////////////
ANTFRAMER_RETURN DSIFramerANT::SendAcknowledgedData(UCHAR ucANTChannel_, UCHAR *pucData_, ULONG ulResponseTime_)
{
   return(SetupAckDataTransfer(
      MESG_ACKNOWLEDGED_DATA_ID,
      ucANTChannel_,
      pucData_,
      MESG_DATA_SIZE,
      ulResponseTime_));
}


ANTFRAMER_RETURN DSIFramerANT::SetupAckDataTransfer(UCHAR ucMessageID_, UCHAR ucANTChannel_, UCHAR *pucData_, UCHAR ucMaxDataSize_, ULONG ulResponseTime_)
{

   ANTFRAMER_RETURN eReturn = ANTFRAMER_PASS;
   ANT_MESSAGE stMessage;
   ULONG ulStartTime = DSIThread_GetSystemTime();
   ANTMessageResponse *pclPassResponse = (ANTMessageResponse*)NULL;
   ANTMessageResponse *pclFailResponse = (ANTMessageResponse*)NULL;
   ANTMessageResponse *pclErrorResponse = (ANTMessageResponse*)NULL;

   volatile BOOL *pbCancel_;
   BOOL bDummyCancel = FALSE;

   if (pbCancel == NULL)
      pbCancel_ = &bDummyCancel;
   else
      pbCancel_ = pbCancel;

   stMessage.ucMessageID = ucMessageID_;
   stMessage.aucData[0] = ucANTChannel_;
   memcpy(&stMessage.aucData[1],pucData_, ucMaxDataSize_-1);

   // If we are going to be waiting for a response setup the Response objects
   if (ulResponseTime_ != 0)
   {
     UCHAR aucDesiredData[3];

     aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = ucANTChannel_;   //Setup response to catch tx complete
     aucDesiredData[ANT_DATA_EVENT_ID_OFFSET] = MESG_EVENT_ID;
     aucDesiredData[ANT_DATA_EVENT_CODE_OFFSET] = EVENT_TRANSFER_TX_COMPLETED;

     pclPassResponse = new ANTMessageResponse();
     pclPassResponse->Attach(MESG_RESPONSE_EVENT_ID, aucDesiredData, sizeof(aucDesiredData), this);

     aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = ucANTChannel_;   //Setup response to catch tx fail
     aucDesiredData[ANT_DATA_EVENT_ID_OFFSET] = MESG_EVENT_ID;
     aucDesiredData[ANT_DATA_EVENT_CODE_OFFSET] = EVENT_TRANSFER_TX_FAILED;

     pclFailResponse = new ANTMessageResponse();
     pclFailResponse->Attach(MESG_RESPONSE_EVENT_ID, aucDesiredData, sizeof(aucDesiredData), this, pclPassResponse->pstCondResponseReady);

     aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = ucANTChannel_;   //Setup response to catch any errors like transfer in progress.
     aucDesiredData[ANT_DATA_EVENT_ID_OFFSET] = ucMessageID_;

     pclErrorResponse = new ANTMessageResponse();
     pclErrorResponse->Attach(MESG_RESPONSE_EVENT_ID, aucDesiredData, 2, this, pclPassResponse->pstCondResponseReady);
   }

   if (WriteMessage(&stMessage, ucMaxDataSize_) == FALSE)
      eReturn = ANTFRAMER_FAIL;

   // Return immediately if we aren't waiting for the response.
   if (ulResponseTime_ == 0)
      return ANTFRAMER_PASS;

   DSIThread_MutexLock(&stMutexResponseRequest);

   if (eReturn == ANTFRAMER_PASS)                                                                  //Only try to wait if we haven't failed yet
   {
      while((pclPassResponse->bResponseReady == FALSE) &&
           (pclFailResponse->bResponseReady == FALSE) &&
           (pclErrorResponse->bResponseReady == FALSE) &&
           (*pbCancel_ == FALSE) &&
           ((DSIThread_GetSystemTime() - ulStartTime) < ulResponseTime_))
      {
         DSIThread_CondTimedWait(pclPassResponse->pstCondResponseReady, &stMutexResponseRequest, 1000);
      }

      if (pclPassResponse->bResponseReady == FALSE)                     //The only time we are sucessful is if we get a tx transfer complete
      {
         //figure out the reason why we failed/stopped
         if ((pclErrorResponse->bResponseReady == TRUE) || (pclFailResponse->bResponseReady == TRUE))
            eReturn = ANTFRAMER_FAIL;
         else if (*pbCancel_ == TRUE)
            eReturn = ANTFRAMER_CANCELLED;
         else
            eReturn = ANTFRAMER_TIMEOUT;
      }
   }

   pclPassResponse->Remove();
   pclFailResponse->Remove();
   pclErrorResponse->Remove();

   DSIThread_MutexUnlock(&stMutexResponseRequest);

   delete pclPassResponse;
   delete pclFailResponse;
   delete pclErrorResponse;

   return eReturn;
}



ANTFRAMER_RETURN DSIFramerANT::SetupBurstDataTransfer(UCHAR ucMessageID_, UCHAR ucANTChannel_, UCHAR * pucData_, ULONG ulSize_,UCHAR ucMaxDataSize_, ULONG ulResponseTime_)

{
   ANTFRAMER_RETURN eReturn = ANTFRAMER_PASS;
   ULONG ulStartTime = DSIThread_GetSystemTime();

   ANTMessageResponse *pclPassResponse = (ANTMessageResponse*)NULL;
   ANTMessageResponse *pclFailResponse = (ANTMessageResponse*)NULL;
   ANTMessageResponse *pclErrorResponse = (ANTMessageResponse*)NULL;

   ANT_MESSAGE* stMessage = (ANT_MESSAGE*)NULL;
   if(!CreateAntMsg_wOptExtBuf(&stMessage, ucMaxDataSize_))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("Framer->SetupBurstDataTransfer(): Failed, ucMaxDataSize_ too big for this part");
      #endif
      return ANTFRAMER_INVALIDPARAM;
   }

   volatile BOOL *pbCancel_;
   BOOL bDummyCancel = FALSE;

   if (pbCancel == NULL)
      pbCancel_ = &bDummyCancel;
   else
      pbCancel_ = pbCancel;

   UCHAR aucDesiredData[3];

   aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = ucANTChannel_; //Setup response to catch tx complete
   aucDesiredData[ANT_DATA_EVENT_ID_OFFSET] = MESG_EVENT_ID;
   aucDesiredData[ANT_DATA_EVENT_CODE_OFFSET] = EVENT_TRANSFER_TX_COMPLETED;

   pclPassResponse = new ANTMessageResponse();
   pclPassResponse->Attach(MESG_RESPONSE_EVENT_ID, aucDesiredData, sizeof(aucDesiredData), this);

   aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = ucANTChannel_; //Setup response to catch tx fail
   aucDesiredData[ANT_DATA_EVENT_ID_OFFSET] = MESG_EVENT_ID;
   aucDesiredData[ANT_DATA_EVENT_CODE_OFFSET] = EVENT_TRANSFER_TX_FAILED;

   pclFailResponse = new ANTMessageResponse();
   pclFailResponse->Attach(MESG_RESPONSE_EVENT_ID, aucDesiredData, sizeof(aucDesiredData), this, pclPassResponse->pstCondResponseReady);

   aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = ucANTChannel_; //Setup response to catch any errors like transfer in progress.
   aucDesiredData[ANT_DATA_EVENT_ID_OFFSET] = ucMessageID_;

   pclErrorResponse = new ANTMessageResponse();
   pclErrorResponse->Attach(MESG_RESPONSE_EVENT_ID, aucDesiredData, 2, this, pclPassResponse->pstCondResponseReady);

   //getting error Rx will also effectively lose the transfer, but only on an AP1


   stMessage->ucMessageID = ucMessageID_;
   stMessage->aucData[0] = ucANTChannel_ & CHANNEL_NUMBER_MASK;        // Clear the sequence bits

   while ((eReturn == ANTFRAMER_PASS) && ulSize_)
   {
     if (ulSize_ > (UCHAR)(ucMaxDataSize_-1))
     {
        memcpy (&stMessage->aucData[1],pucData_,ucMaxDataSize_-1);
        ulSize_ -= ucMaxDataSize_-1;
        pucData_ += ucMaxDataSize_-1;
     }
     else
     {
        while((UCHAR)(ucMaxDataSize_-9) >= ulSize_)
           ucMaxDataSize_ -= 8; //Shorten the last packet by 8-bytes if possible.
        stMessage->aucData[0] |= SEQUENCE_LAST_MESSAGE;
        memset (&stMessage->aucData[1], 0x00, ucMaxDataSize_-1);
        memcpy (&stMessage->aucData[1],pucData_,ulSize_);
        ulSize_ = 0;
     }

     if (WriteMessage(stMessage,ucMaxDataSize_) == FALSE)
        eReturn = ANTFRAMER_FAIL;

      //Adjust sequence number
      if ((stMessage->aucData[0] & SEQUENCE_NUMBER_MASK) == SEQUENCE_NUMBER_ROLLOVER)
         stMessage->aucData[0] = SEQUENCE_NUMBER_INC | ucANTChannel_;
      else
         stMessage->aucData[0] += SEQUENCE_NUMBER_INC;

     if (*pbCancel_ == TRUE)
        eReturn = ANTFRAMER_CANCELLED;

      //Abort transfer on errors, don't keep trying to send
     if ((pclFailResponse->bResponseReady == TRUE) || (pclErrorResponse->bResponseReady == TRUE))
        eReturn = ANTFRAMER_FAIL;

     if (ulResponseTime_ != 0)                                                                         //Check for errors
     {
       if ((DSIThread_GetSystemTime() - ulStartTime) > ulResponseTime_)
           eReturn = ANTFRAMER_TIMEOUT;
     }
   }

   DSIThread_MutexLock(&stMutexResponseRequest);
   if (ulResponseTime_ != 0)                                                                         //Check for errors
   {
     if (eReturn == ANTFRAMER_PASS)                                                                  //Only try to wait if we haven't failed yet
     {
         while((pclPassResponse->bResponseReady == FALSE) &&
              (pclFailResponse->bResponseReady == FALSE) &&
              (pclErrorResponse->bResponseReady == FALSE) &&
              (*pbCancel_ == FALSE) &&
              ((DSIThread_GetSystemTime() - ulStartTime) < ulResponseTime_))
         {
            DSIThread_CondTimedWait(pclPassResponse->pstCondResponseReady, &stMutexResponseRequest, 1000);
         }

         if (pclPassResponse->bResponseReady == FALSE)                     //The only time we are sucessful is if we get a tx transfer complete
         {
            //figure out the reason why we failed/stopped
            if ((pclErrorResponse->bResponseReady == TRUE) || (pclFailResponse->bResponseReady == TRUE))
               eReturn = ANTFRAMER_FAIL;
            else if (*pbCancel_ == TRUE)
               eReturn = ANTFRAMER_CANCELLED;
            else
               eReturn = ANTFRAMER_TIMEOUT;
         }
      }
   }

   pclPassResponse->Remove();
   pclFailResponse->Remove();
   pclErrorResponse->Remove();

   DSIThread_MutexUnlock(&stMutexResponseRequest);

   delete pclPassResponse;
   delete pclFailResponse;
   delete pclErrorResponse;
   delete[] stMessage;

   //Always return true with no timeout, so nobody relies on this return value
   if(ulResponseTime_ == 0)
      eReturn = ANTFRAMER_PASS;

   return eReturn;
}



///////////////////////////////////////////////////////////////////////
ANTFRAMER_RETURN DSIFramerANT::SendTransfer(UCHAR ucANTChannel_, UCHAR * pucData_, ULONG ulSize_, ULONG ulResponseTime_)
{
   return(SetupBurstDataTransfer(
      MESG_BURST_DATA_ID,
      ucANTChannel_,
      pucData_,
      ulSize_,
      MESG_DATA_SIZE,
      ulResponseTime_));
}

ANTFRAMER_RETURN DSIFramerANT::SendAdvancedTransfer(UCHAR ucANTChannel_, UCHAR * pucData_, ULONG ulSize_, UCHAR ucStdPcktsPerSerialMsg_, ULONG ulResponseTime_)
{
   return(SetupBurstDataTransfer(
      MESG_ADV_BURST_DATA_ID,
      ucANTChannel_,
      pucData_,
      ulSize_,
      1 + ((MESG_DATA_SIZE-1) * ucStdPcktsPerSerialMsg_),   //seqNum + (payloadLength*pcktsPerSerial)
      ulResponseTime_));
}

///////////////////////////////////////////////////////////////////////
ANTFRAMER_RETURN DSIFramerANT::SendANTFSTransfer(UCHAR ucANTChannel_, UCHAR* pucHeader_, UCHAR* pucFooter_, UCHAR * pucData_, ULONG ulSize_, ULONG ulResponseTime_, volatile ULONG *pulProgress_)
{
   ANTFRAMER_RETURN eReturn = ANTFRAMER_PASS;
   ANT_MESSAGE stMessage;
   ULONG ulStartTime = DSIThread_GetSystemTime();
   UCHAR *pucDataSource;

   ANTMessageResponse *pclPassResponse = (ANTMessageResponse*)NULL;
   ANTMessageResponse *pclFailResponse = (ANTMessageResponse*)NULL;
   ANTMessageResponse *pclErrorResponse = (ANTMessageResponse*)NULL;

#if defined(WAIT_TO_FEED_TRANSFER)
   ANTMessageResponse *pclBroadcastResponse = (ANTMessageResponse*)NULL;
   ANTMessageResponse *pclAcknowledgeResponse = (ANTMessageResponse*)NULL;
   BOOL bFirstPacket = TRUE;
   UCHAR ucSyncMesgCount = 0;
   UCHAR ucChannelStatus;
#endif

   ULONG ulDummyProgress = 0;
   volatile BOOL *pbCancel_;
   BOOL bDummyCancel = FALSE;

   if (pbCancel == NULL)
      pbCancel_ = &bDummyCancel;
   else
      pbCancel_ = pbCancel;

   if (pulProgress_ == NULL)
      pulProgress_ = &ulDummyProgress;

   if (pucHeader_)                          //if the header is not NULL, set the data pointer and add 8 to the size
   {
      pucDataSource = pucHeader_;
      ulSize_ += 8;
   }
   else
   {
      pucDataSource = pucData_;
   }

   #if defined(WAIT_TO_FEED_TRANSFER)
      if(GetChannelStatus(ucANTChannel_,&ucChannelStatus, 2000) == FALSE)
         return ANTFRAMER_FAIL;

      if ((ucChannelStatus & STATUS_CHANNEL_STATE_MASK) != STATUS_TRACKING_CHANNEL)
         return ANTFRAMER_FAIL;
   #endif



   // If we are going to be waiting for a response setup the Response objects
   if (ulResponseTime_ != 0)
   {
     UCHAR aucDesiredData[3];

     aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = ucANTChannel_;   //Setup response to catch tx complete
     aucDesiredData[ANT_DATA_EVENT_ID_OFFSET] = MESG_EVENT_ID;
     aucDesiredData[ANT_DATA_EVENT_CODE_OFFSET] = EVENT_TRANSFER_TX_COMPLETED;

     pclPassResponse = new ANTMessageResponse();
     pclPassResponse->Attach(MESG_RESPONSE_EVENT_ID, aucDesiredData, sizeof(aucDesiredData), this);

     aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = ucANTChannel_;   //Setup response to catch tx fail
     aucDesiredData[ANT_DATA_EVENT_ID_OFFSET] = MESG_EVENT_ID;
     aucDesiredData[ANT_DATA_EVENT_CODE_OFFSET] = EVENT_TRANSFER_TX_FAILED;

     pclFailResponse = new ANTMessageResponse();
     pclFailResponse->Attach(MESG_RESPONSE_EVENT_ID, aucDesiredData, sizeof(aucDesiredData), this, pclPassResponse->pstCondResponseReady);

     aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = ucANTChannel_;   //Setup response to catch any errors like transfer in progress.
     aucDesiredData[ANT_DATA_EVENT_ID_OFFSET] = MESG_BURST_DATA_ID;

     pclErrorResponse = new ANTMessageResponse();
     pclErrorResponse->Attach(MESG_RESPONSE_EVENT_ID, aucDesiredData, 2, this, pclPassResponse->pstCondResponseReady);

     //getting error Rx will also effectively lose the transfer, but only on an AP1
#if defined(WAIT_TO_FEED_TRANSFER)
     aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = ucANTChannel_;   //Setup response to catch only broadcast/acknowledged messages for this channel

     pclBroadcastResponse = new ANTMessageResponse();
     pclBroadcastResponse->Attach(MESG_BROADCAST_DATA_ID, aucDesiredData, 1, this, pclPassResponse->pstCondResponseReady);

     pclAcknowledgeResponse = new ANTMessageResponse();
     pclAcknowledgeResponse->Attach(MESG_ACKNOWLEDGED_DATA_ID, aucDesiredData, 1, this, pclPassResponse->pstCondResponseReady);
#endif
   }


   stMessage.ucMessageID = MESG_BURST_DATA_ID;
   stMessage.aucData[0] = ucANTChannel_ & CHANNEL_NUMBER_MASK;        // Clear the sequence bits

   while (eReturn == ANTFRAMER_PASS && ulSize_)
   {

     if (*pbCancel_ == TRUE)
        eReturn = ANTFRAMER_CANCELLED;

     if (ulSize_ > 8)
     {
        memcpy (&stMessage.aucData[1],pucDataSource,8);
        *pulProgress_ += 8;
        ulSize_ -= 8;
     }
     else
     {
        if (pucFooter_ == (UCHAR*)NULL)
           stMessage.aucData[0] |= SEQUENCE_LAST_MESSAGE;

        memset (&stMessage.aucData[1], 0x00, 8);
        memcpy (&stMessage.aucData[1],pucDataSource,ulSize_);
        *pulProgress_ += ulSize_;
        ulSize_ = 0;
     }

     if (ulSize_)
     {
        if (pucDataSource == pucHeader_)
           pucDataSource = pucData_;
        else
           pucDataSource += 8;
     }

     if (WriteMessage(&stMessage,9) == FALSE)
        eReturn = ANTFRAMER_FAIL;

      //Adjust sequence number
      if ((stMessage.aucData[0] & SEQUENCE_NUMBER_MASK) == SEQUENCE_NUMBER_ROLLOVER)
         stMessage.aucData[0] = SEQUENCE_NUMBER_INC | ucANTChannel_;
      else
         stMessage.aucData[0] += SEQUENCE_NUMBER_INC;


#if defined(WAIT_TO_FEED_TRANSFER)
     if (bFirstPacket)
     {
        bFirstPacket = FALSE;

        DSIThread_MutexLock(&stMutexResponseRequest);

        if (eReturn == ANTFRAMER_PASS)                                                                  //Only try to wait if we haven't failed yet
        {
            if ((pclBroadcastResponse->bResponseReady == FALSE) &&
               (pclAcknowledgeResponse->bResponseReady == FALSE) &&
               (pclPassResponse->bResponseReady == FALSE) &&
               (pclFailResponse->bResponseReady == FALSE) &&
               (pclErrorResponse->bResponseReady == FALSE) &&
               (*pbCancel_ == FALSE) &&
               ((DSIThread_GetSystemTime() - ulStartTime) < ulResponseTime_))
            {
               UCHAR ucStatus = DSIThread_CondTimedWait(pclBroadcastResponse->pstCondResponseReady, &stMutexResponseRequest, 3000);  //Try to wait for the next syncronous event to send out the next packet.

               if (ucStatus != DSI_THREAD_ENONE)   //If we timeout
               {
                  #if defined(DEBUG_FILE)
                     if(ucStatus == DSI_THREAD_EOTHER)
                        DSIDebug::ThreadWrite("Framer->SendFSTransfer(): CondTimedWait() Failed!");
                     DSIDebug::ThreadWrite("Framer->SendFSTransfer():  Wait for sync mesg failed.");
                  #endif
                  //if we didn't get a syncronous event,
                  stMessage.aucData[0] += SEQUENCE_NUMBER_INC; // mess up the sequence number on purpose, this will result in the transfer being cleared off of the device and will result in us exiting this function due to the sync error.
               }
            }

            pclBroadcastResponse->bResponseReady = FALSE;
            pclAcknowledgeResponse->bResponseReady = FALSE;
            //reset the variables and continue on
        }
        DSIThread_MutexUnlock(&stMutexResponseRequest);
     }
#endif

     if (ulResponseTime_ != 0)                                                                         //Check for errors
     {
        if ((pclFailResponse->bResponseReady == TRUE) || (pclErrorResponse->bResponseReady == TRUE))
          eReturn = ANTFRAMER_FAIL;

       if ((DSIThread_GetSystemTime() - ulStartTime) > ulResponseTime_)
           eReturn = ANTFRAMER_TIMEOUT;
     }
   }

   if ((eReturn == ANTFRAMER_PASS) && (pucFooter_))
   {
      stMessage.aucData[0] |= SEQUENCE_LAST_MESSAGE;
      memcpy (&stMessage.aucData[1],pucFooter_,8);
      *pulProgress_ += 8;

      if (WriteMessage(&stMessage,9) == FALSE)
        eReturn = ANTFRAMER_FAIL;
   }

   if (ulResponseTime_ != 0)                                                                         //Check for errors
   {
     DSIThread_MutexLock(&stMutexResponseRequest);

     if (eReturn == ANTFRAMER_PASS)                                                                  //Only try to wait if we haven't failed yet
     {
         while((pclPassResponse->bResponseReady == FALSE) &&
              (pclFailResponse->bResponseReady == FALSE) &&
              (pclErrorResponse->bResponseReady == FALSE) &&
              (*pbCancel_ == FALSE) &&
              ((DSIThread_GetSystemTime() - ulStartTime) < ulResponseTime_))
         {
            DSIThread_CondTimedWait(pclPassResponse->pstCondResponseReady, &stMutexResponseRequest, 1000);

            #if defined(WAIT_TO_FEED_TRANSFER)
               if ((pclBroadcastResponse->bResponseReady == TRUE) ||
               (pclAcknowledgeResponse->bResponseReady == TRUE))
               {
                  pclBroadcastResponse->bResponseReady = FALSE;
                  pclAcknowledgeResponse->bResponseReady = FALSE;

                  if (ucSyncMesgCount++ > 2)                   //If we get more than 2 sync events (3 or 4) when we're waiting for our transfer to complete
                  {
                     pclErrorResponse->bResponseReady = TRUE;  //Set the error response flag
                  }
               }
            #endif
         }

         if (pclPassResponse->bResponseReady == FALSE)                     //The only time we are sucessful is if we get a tx transfer complete
         {
            //figure out the reason why we failed/stopped
            if ((pclErrorResponse->bResponseReady == TRUE) || (pclFailResponse->bResponseReady == TRUE))
               eReturn = ANTFRAMER_FAIL;
            else if (*pbCancel_ == TRUE)
               eReturn = ANTFRAMER_CANCELLED;
            else
               eReturn = ANTFRAMER_TIMEOUT;
         }
     }

      pclPassResponse->Remove();
      pclFailResponse->Remove();
      pclErrorResponse->Remove();
   #if defined(WAIT_TO_FEED_TRANSFER)
      pclBroadcastResponse->Remove();
      pclAcknowledgeResponse->Remove();
   #endif

      DSIThread_MutexUnlock(&stMutexResponseRequest);

      delete pclPassResponse;
      delete pclFailResponse;
      delete pclErrorResponse;
   #if defined(WAIT_TO_FEED_TRANSFER)
      delete pclBroadcastResponse;
      delete pclAcknowledgeResponse;
   #endif
   }

   return eReturn;
}

///////////////////////////////////////////////////////////////////////
ANTFRAMER_RETURN DSIFramerANT::SendANTFSClientTransfer(UCHAR ucANTChannel_, ANTFS_DATA* pstHeader_, ANTFS_DATA* pstFooter_, ANTFS_DATA* pstData_, ULONG ulResponseTime_, volatile ULONG *pulProgress_)
{
   ANTFRAMER_RETURN eReturn = ANTFRAMER_PASS;
   ANT_MESSAGE stMessage;
   ULONG ulStartTime = DSIThread_GetSystemTime();
   UCHAR *pucDataSource;
   ULONG ulBlockSize;
   ULONG ulTotalSize;

   ANTMessageResponse *pclPassResponse = (ANTMessageResponse*)NULL;
   ANTMessageResponse *pclFailResponse = (ANTMessageResponse*)NULL;
   ANTMessageResponse *pclErrorResponse = (ANTMessageResponse*)NULL;

   ULONG ulDummyProgress = 0;
   volatile BOOL *pbCancel_;
   BOOL bDummyCancel = FALSE;

   if (pbCancel == NULL)
      pbCancel_ = &bDummyCancel;
   else
      pbCancel_ = pbCancel;

   if (pulProgress_ == NULL)
      pulProgress_ = &ulDummyProgress;

   if (pstHeader_->pucData)   //if there is a header, set the data pointer
   {
      pucDataSource = pstHeader_->pucData;
      ulBlockSize = pstHeader_->ulSize;
   }
   else
   {
      pucDataSource = pstData_->pucData;
      ulBlockSize = pstData_->ulSize;
   }

   ulTotalSize = pstHeader_->ulSize + pstData_->ulSize + pstFooter_->ulSize;

   // If we are going to be waiting for a response setup the Response objects
   if (ulResponseTime_ != 0)
   {
     UCHAR aucDesiredData[3];

     aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = ucANTChannel_;   //Setup response to catch tx complete
     aucDesiredData[ANT_DATA_EVENT_ID_OFFSET] = MESG_EVENT_ID;
     aucDesiredData[ANT_DATA_EVENT_CODE_OFFSET] = EVENT_TRANSFER_TX_COMPLETED;

     pclPassResponse = new ANTMessageResponse();
     pclPassResponse->Attach(MESG_RESPONSE_EVENT_ID, aucDesiredData, sizeof(aucDesiredData), this);

     aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = ucANTChannel_;   //Setup response to catch tx fail
     aucDesiredData[ANT_DATA_EVENT_ID_OFFSET] = MESG_EVENT_ID;
     aucDesiredData[ANT_DATA_EVENT_CODE_OFFSET] = EVENT_TRANSFER_TX_FAILED;

     pclFailResponse = new ANTMessageResponse();
     pclFailResponse->Attach(MESG_RESPONSE_EVENT_ID, aucDesiredData, sizeof(aucDesiredData), this, pclPassResponse->pstCondResponseReady);

     aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = ucANTChannel_;   //Setup response to catch any errors like transfer in progress.
     aucDesiredData[ANT_DATA_EVENT_ID_OFFSET] = MESG_BURST_DATA_ID;

     pclErrorResponse = new ANTMessageResponse();
     pclErrorResponse->Attach(MESG_RESPONSE_EVENT_ID, aucDesiredData, 2, this, pclPassResponse->pstCondResponseReady);
   }

   stMessage.ucMessageID = MESG_BURST_DATA_ID;
   stMessage.aucData[0] = ucANTChannel_ & CHANNEL_NUMBER_MASK;        // Clear the sequence bits

   while (eReturn == ANTFRAMER_PASS && ulTotalSize)
   {
      UCHAR ucCurrentBlockSize;

     if (*pbCancel_ == TRUE)
        eReturn = ANTFRAMER_CANCELLED;

     if (ulBlockSize > 8)
     {
        ucCurrentBlockSize = 8;
        memcpy (&stMessage.aucData[1],pucDataSource, ucCurrentBlockSize);
        *pulProgress_ += ucCurrentBlockSize;
        ulTotalSize -= ucCurrentBlockSize;
        ulBlockSize -= ucCurrentBlockSize;
        pucDataSource += ucCurrentBlockSize;
     }
     else
     {
        ucCurrentBlockSize = (UCHAR)ulBlockSize;
        memset (&stMessage.aucData[1], 0x00, 8);
        memcpy (&stMessage.aucData[1],pucDataSource, ucCurrentBlockSize);
        *pulProgress_ += ucCurrentBlockSize;
        ulTotalSize -= ucCurrentBlockSize;
        ulBlockSize = 0;
     }

     if (ulBlockSize == 0)
     {
        if(pucDataSource == (pstHeader_->pucData + pstHeader_->ulSize - ucCurrentBlockSize))
        {
           if(pstData_->pucData)
           {
              pucDataSource = pstData_->pucData;
              ulBlockSize = pstData_->ulSize;
           }
           else if(pstFooter_->pucData)
           {
              pucDataSource = pstFooter_->pucData;
              ulBlockSize = pstFooter_->ulSize;
           }
           else
           {
              stMessage.aucData[0] |= SEQUENCE_LAST_MESSAGE;
           }
        }
        else if(pucDataSource == (pstData_->pucData + pstData_->ulSize - ucCurrentBlockSize))
        {
           if(pstFooter_->pucData)
           {
              pucDataSource = pstFooter_->pucData;
              ulBlockSize = pstFooter_->ulSize;
           }
           else
           {
              stMessage.aucData[0] |= SEQUENCE_LAST_MESSAGE;
           }
        }
        else   // footer
        {
           stMessage.aucData[0] |= SEQUENCE_LAST_MESSAGE;
        }
     }

     if (WriteMessage(&stMessage,9) == FALSE)
        eReturn = ANTFRAMER_FAIL;

      //Adjust sequence number
      if ((stMessage.aucData[0] & SEQUENCE_NUMBER_MASK) == SEQUENCE_NUMBER_ROLLOVER)
         stMessage.aucData[0] = SEQUENCE_NUMBER_INC | ucANTChannel_;
      else
         stMessage.aucData[0] += SEQUENCE_NUMBER_INC;

     if (ulResponseTime_ != 0)                                                                         //Check for errors
     {
        if ((pclFailResponse->bResponseReady == TRUE) || (pclErrorResponse->bResponseReady == TRUE))
          eReturn = ANTFRAMER_FAIL;

       if ((DSIThread_GetSystemTime() - ulStartTime) > ulResponseTime_)
           eReturn = ANTFRAMER_TIMEOUT;
     }
   } // while loop

   if (ulResponseTime_ != 0)                                                                         //Check for errors
   {
     DSIThread_MutexLock(&stMutexResponseRequest);

     if (eReturn == ANTFRAMER_PASS)                                                                  //Only try to wait if we haven't failed yet
     {
         while((pclPassResponse->bResponseReady == FALSE) &&
              (pclFailResponse->bResponseReady == FALSE) &&
              (pclErrorResponse->bResponseReady == FALSE) &&
              (*pbCancel_ == FALSE) &&
              ((DSIThread_GetSystemTime() - ulStartTime) < ulResponseTime_))
         {
            DSIThread_CondTimedWait(pclPassResponse->pstCondResponseReady, &stMutexResponseRequest, 1000);
         }

         if (pclPassResponse->bResponseReady == FALSE)                     //The only time we are sucessful is if we get a tx transfer complete
         {
            //figure out the reason why we failed/stopped
            if ((pclErrorResponse->bResponseReady == TRUE) || (pclFailResponse->bResponseReady == TRUE))
               eReturn = ANTFRAMER_FAIL;
            else if (*pbCancel_ == TRUE)
               eReturn = ANTFRAMER_CANCELLED;
            else
               eReturn = ANTFRAMER_TIMEOUT;
         }
     }

      pclPassResponse->Remove();
      pclFailResponse->Remove();
      pclErrorResponse->Remove();

      DSIThread_MutexUnlock(&stMutexResponseRequest);

      delete pclPassResponse;
      delete pclFailResponse;
      delete pclErrorResponse;
   }

   return eReturn;
}

//////////////////////////////////////////////////////////////////////////////////
// Private Class Functions
//////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// stMutexCriticalSection must be locked before calling this function.
///////////////////////////////////////////////////////////////////////
USHORT DSIFramerANT::GetMessageSize(void)
{
   USHORT usRetVal;

   if (ucError)
      usRetVal = DSI_FRAMER_ERROR;
   else if ((usMessageHead - usMessageTail) != 0)
      usRetVal = astMessageBuffer[usMessageTail].ucSize;
   else
      usRetVal = DSI_FRAMER_TIMEDOUT;

   return usRetVal;
}

///////////////////////////////////////////////////////////////////////
void DSIFramerANT::ProcessMessage(void)
{
   UCHAR ucMessageID = aucRxFifo[MESG_ID_OFFSET];
   UCHAR ucSize = aucRxFifo[MESG_SIZE_OFFSET];                    // Set size as reported by message.

   CheckResponseList();

   if(ucMessageID == MESG_BURST_DATA_ID || ucMessageID == MESG_EXT_BURST_DATA_ID)
   {
      ucPrevSequenceNum = aucRxFifo[MESG_DATA_OFFSET] & SEQUENCE_NUMBER_MASK;

      //If the burst flag only holds the SEQUENCE_LAST_MESSAGE and no sequence number
      //we recode it as an acknowledged message. This is because there is a timing bug in some
      //versions of the chips that had the first implementation of advanced burst support,
      //such as the USBm, that cause a received ACK to be sent in this wrong format.
      //Fixing it here means nobody else has to work around this bug, and this is essentially
      //the same logic that is used in the ANT stack anyway.
      if(ucPrevSequenceNum == SEQUENCE_LAST_MESSAGE)
      {
         #if defined(SERIAL_DEBUG)
            DSIDebug::SerialWrite(pclSerial->GetDeviceNumber(), "Rx - Recevied ack msg as burst packet, recoding to ack.", (UCHAR*)NULL, 0);
         #endif
         aucRxFifo[MESG_ID_OFFSET] = MESG_ACKNOWLEDGED_DATA_ID;
         aucRxFifo[MESG_DATA_OFFSET] &= ~SEQUENCE_NUMBER_MASK; //Also remove the burst flags
         ProcessMessage(); //Recurse to handle the recoded message
         return;  //Now we are done
      }
   }

   if(ucMessageID == MESG_ADV_BURST_DATA_ID && bSplitAdvancedBursts) // split into normal burst messages.
   {
      #if defined(SERIAL_DEBUG)
         DSIDebug::SerialWrite(pclSerial->GetDeviceNumber(), "Decomposing", aucRxFifo, ucSize+4);
      #endif
      for(UCHAR i = 0; i * 8 < ucSize - 1; i++) //For each 8-byte packet
      {
         if(ucPrevSequenceNum == SEQUENCE_NUMBER_ROLLOVER) //Increment sequence num
            ucPrevSequenceNum = 0;
         ucPrevSequenceNum += SEQUENCE_NUMBER_INC;
         if((aucRxFifo[MESG_DATA_OFFSET] & SEQUENCE_LAST_MESSAGE) != 0 && (i+1)*8 == ucSize - 1) //If the last packet.
            ucPrevSequenceNum |= SEQUENCE_LAST_MESSAGE;
         // Add message to the queue.
         if ((USHORT)(usMessageHead - usMessageTail) < (USHORT)(sizeof(astMessageBuffer) / sizeof(ANT_MESSAGE_ITEM) - 1))
         {
            astMessageBuffer[usMessageHead].ucSize = 9;
            astMessageBuffer[usMessageHead].stANTMessage.ucMessageID = MESG_BURST_DATA_ID;
            astMessageBuffer[usMessageHead].stANTMessage.aucData[0] = ucPrevSequenceNum | (aucRxFifo[MESG_DATA_OFFSET] & CHANNEL_NUMBER_MASK);
            memcpy(astMessageBuffer[usMessageHead].stANTMessage.aucData + 1, &aucRxFifo[MESG_DATA_OFFSET + 1 + i*8], 8);
            usMessageHead++;                                   // Rollover of usMessageHead happens automagically because our buffer size is MAX_USHORT + 1.
         }
         else
         {
            ucError = DSI_FRAMER_ANT_EQUEUE_OVERFLOW;
         }

         DSIThread_CondSignal(&stCondMessageReady);

         #if defined(SERIAL_DEBUG)
            DSIDebug::SerialWrite(pclSerial->GetDeviceNumber(), "Simulated Rx", astMessageBuffer[usMessageHead-1].stANTMessage.aucData, astMessageBuffer[usMessageHead-1].ucSize);
         #endif
      }
   }
   else
   {
      // Add message to the queue.
      if ((USHORT)(usMessageHead - usMessageTail) < (USHORT)(sizeof(astMessageBuffer) / sizeof(ANT_MESSAGE_ITEM) - 1))
      {
         astMessageBuffer[usMessageHead].ucSize = ucSize;
         astMessageBuffer[usMessageHead].stANTMessage.ucMessageID = ucMessageID;
         memcpy(astMessageBuffer[usMessageHead].stANTMessage.aucData, &aucRxFifo[MESG_DATA_OFFSET], ucSize);
         usMessageHead++;                                   // Rollover of usMessageHead happens automagically because our buffer size is MAX_USHORT + 1.
      }
      else
      {
         ucError = DSI_FRAMER_ANT_EQUEUE_OVERFLOW;
      }

      DSIThread_CondSignal(&stCondMessageReady);

      #if defined(SERIAL_DEBUG)
         DSIDebug::SerialWrite(pclSerial->GetDeviceNumber(), "Rx", aucRxFifo, ucSize + 4);
      #endif
   }
}

///////////////////////////////////////////////////////////////////////
void DSIFramerANT::CheckResponseList(void)
{
   ANTMessageResponse *pclResponseList;
   BOOL bMatch;

   DSIThread_MutexLock(&stMutexResponseRequest);

   pclResponseList = pclResponseListStart;

   while (pclResponseList != NULL)
   {
      bMatch = TRUE;

      if (pclResponseList->bResponseReady == FALSE && pclResponseList->stMessageItem.stANTMessage.ucMessageID == aucRxFifo[MESG_ID_OFFSET])
      {
         for(int i=0; i < pclResponseList->ucBytesToMatch; i++)
         {
            if (pclResponseList->stMessageItem.stANTMessage.aucData[i] != aucRxFifo[MESG_DATA_OFFSET + i])
            {
               bMatch = FALSE;                                                             // Data byte did not match
            }
         }
      }
      else
      {
         bMatch = FALSE;                                                                     // Mesg ID did not match
      }

      if (bMatch)
      {
        int i = pclResponseList->ucBytesToMatch;
        pclResponseList->stMessageItem.ucSize = aucRxFifo[MESG_SIZE_OFFSET];
        memcpy(&(pclResponseList->stMessageItem.stANTMessage.aucData[i]), &(aucRxFifo[MESG_DATA_OFFSET + i]), MESG_MAX_SIZE_VALUE - i);   // Copy the rest of the message

        pclResponseList->bResponseReady = TRUE;
        DSIThread_CondSignal(pclResponseList->pstCondResponseReady);
      }

      pclResponseList = pclResponseList->pclNext;
   }

   DSIThread_MutexUnlock(&stMutexResponseRequest);
}


///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SendCommand(ANT_MESSAGE *pstANTMessage_, USHORT usMessageSize_, ULONG ulResponseTime_)
{
   ANTMessageResponse *pclCommandResponse = (ANTMessageResponse*)NULL;

   // If we are going to be waiting for a response setup the Response object
   if (ulResponseTime_ != 0)
   {
      UCHAR aucDesiredData[2];
      UCHAR bytesToMatch = 2;

      aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = pstANTMessage_->aucData[ANT_DATA_CHANNEL_NUM_OFFSET];
      aucDesiredData[ANT_DATA_EVENT_ID_OFFSET] = pstANTMessage_->ucMessageID;

      //Script dump success can be determined by looking for the script cmd 0x04 dump complete code
      if(pstANTMessage_->ucMessageID == MESG_SCRIPT_CMD_ID && pstANTMessage_->aucData[ANT_DATA_EVENT_ID_OFFSET] == SCRIPT_CMD_DUMP)
      {
         aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] = SCRIPT_CMD_END_DUMP;
         bytesToMatch = 1; //The second byte is the number of commands returned, which we can't guess so only match the first byte
      }
      else if(pstANTMessage_->ucMessageID == MESG_SCRIPT_DATA_ID)
      {
         //The first byte of script write is the id of the message being written, not the channel, and it is not overwritten but it is returned with the burst mask, so we need to ensure that is what we are looking for
         aucDesiredData[ANT_DATA_CHANNEL_NUM_OFFSET] &= 0x1F;
      }

      pclCommandResponse = new ANTMessageResponse();
      pclCommandResponse->Attach(MESG_RESPONSE_EVENT_ID, aucDesiredData, bytesToMatch, this);
   }

   // Write the command message.
   if (!WriteMessage(pstANTMessage_, usMessageSize_))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("Framer->SendCommand():  WriteMessage Failed.");
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
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("Framer->SendCommand():  Timeout.");
      #endif
      return FALSE;
   }

   // Check the response.
   if (pclCommandResponse->stMessageItem.stANTMessage.aucData[ANT_DATA_EVENT_CODE_OFFSET] != RESPONSE_NO_ERROR)
   {
      delete pclCommandResponse;
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("Framer->SendCommand():  Response != RESPONSE_NO_ERROR.");
      #endif
      return FALSE;
   }

   delete pclCommandResponse;
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SendRequest(UCHAR ucRequestedMesgID_, UCHAR ucANTChannel_ , ANT_MESSAGE_ITEM *pstANTResponse_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;
   ANTMessageResponse *pclRequestResponse = (ANTMessageResponse*)NULL;

   // Build the request message
   stMessage.ucMessageID = MESG_REQUEST_ID;
   stMessage.aucData[0] = ucANTChannel_;
   stMessage.aucData[1] = ucRequestedMesgID_;

   // If we are going to be waiting for a response setup the Response object
   if ((ulResponseTime_ != 0) && (pstANTResponse_ != NULL))
   {
      pclRequestResponse = new ANTMessageResponse();
     pclRequestResponse->Attach(ucRequestedMesgID_, (UCHAR*)NULL, 0, this);
   }

   // Write the command message.
   if (!WriteMessage(&stMessage, MESG_REQUEST_SIZE))
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

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SendUserNvmRequest(UCHAR ucRequestedMesgID_, UCHAR ucANTChannel_ , ANT_MESSAGE_ITEM *pstANTResponse_, USHORT usAddress_, UCHAR ucSize_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;
   ANTMessageResponse *pclRequestResponse = (ANTMessageResponse*)NULL;

   // Build the request message
   stMessage.ucMessageID = MESG_REQUEST_ID;
   stMessage.aucData[0] = ucANTChannel_;
   stMessage.aucData[1] = ucRequestedMesgID_;
   stMessage.aucData[2] = (UCHAR)(usAddress_ & 0xFF);
   stMessage.aucData[3] = (UCHAR)((usAddress_ >>8) & 0xFF);
   stMessage.aucData[4] = ucSize_;

   // If we are going to be waiting for a response setup the Response object
   if ((ulResponseTime_ != 0) && (pstANTResponse_ != NULL))
   {
      pclRequestResponse = new ANTMessageResponse();
      pclRequestResponse->Attach(ucRequestedMesgID_, (UCHAR*)NULL, 0, this);
   }

   // Write the command message.
   if (!WriteMessage(&stMessage, MESG_REQUEST_USER_NVM_SIZE))
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

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::SetUSBDescriptorString(UCHAR ucStringNum_, UCHAR *pucDescString_, UCHAR ucStringSize_, ULONG ulResponseTime_)
{
   ANT_MESSAGE stMessage;

   stMessage.ucMessageID = MESG_SET_USB_INFO_ID;
   stMessage.aucData[0] = ucStringNum_;

   UCHAR ucMesgSize = ucStringSize_ + 2;  // Message size depends on string length

   if(ucMesgSize > MESG_MAX_SIZE_VALUE)
      ucMesgSize = MESG_MAX_SIZE_VALUE;   // Check size does not exceed buffer space

   // Copy the descriptor string
   memcpy(&stMessage.aucData[1], pucDescString_, ucMesgSize - 2);

   // Check that string is NULL terminated
   if(ucStringNum_ != USB_DESCRIPTOR_VID_PID) // except for string VID/PID
   {
      if(stMessage.aucData[ucMesgSize-2] != 0)
      {
         if(ucMesgSize < MESG_MAX_SIZE_VALUE)
         {
            ucMesgSize++;  // append NULL character at the end and adjust message size, or truncate string if too big
         }
         stMessage.aucData[ucMesgSize-2] = 0;
      }
   }

   stMessage.aucData[ucMesgSize - 1] = ucMesgSize - 2;

   return SendCommand(&stMessage, ucMesgSize, ulResponseTime_);

}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::GetDeviceUSBInfo(UCHAR ucDeviceNum_, UCHAR* pucProductString_, UCHAR* pucSerialString_, USHORT usBufferSize_)
{
   return(pclSerial->GetDeviceUSBInfo(ucDeviceNum_, pucProductString_, pucSerialString_, usBufferSize_));
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::GetDeviceUSBPID(USHORT& usPid_)
{
   return(pclSerial->GetDevicePID(usPid_));
}

///////////////////////////////////////////////////////////////////////
BOOL DSIFramerANT::GetDeviceUSBVID(USHORT& usVid_)
{
   return(pclSerial->GetDeviceVID(usVid_));
}

///Allocates a new standard ANT_MESSAGE struct which must be deleted after use.
BOOL DSIFramerANT::CreateAntMsg_wOptExtBuf(ANT_MESSAGE **ppstExtBufAntMsg_, ULONG ulReqMinDataSize_)
{
   if(ulReqMinDataSize_ > MESG_MAX_SIZE_VALUE)
   {
      *ppstExtBufAntMsg_ = (ANT_MESSAGE*) NULL;
      return FALSE;
   }
   else
   {
      *ppstExtBufAntMsg_ = (ANT_MESSAGE*)new UCHAR[sizeof(ANT_MESSAGE)];
      return TRUE;
   }
}


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
ANTMessageResponse::ANTMessageResponse()
{
   pstCondResponseReady = &stCondResponseReady;
   bResponseReady = FALSE;
   pclNext = (ANTMessageResponse*)NULL;
   pclFramer = (DSIFramerANT*)NULL;
   if (DSIThread_CondInit(pstCondResponseReady) != DSI_THREAD_ENONE)                       //Init the wait object
      return; //need to think of a different way to handle the failure
}

///////////////////////////////////////////////////////////////////////
ANTMessageResponse::~ANTMessageResponse()
{
   Remove();
   DSIThread_CondDestroy(&stCondResponseReady);
}

///////////////////////////////////////////////////////////////////////
BOOL ANTMessageResponse::Attach(UCHAR ucMessageID_, UCHAR *pucData_, UCHAR ucBytesToMatch_, DSIFramerANT * pclFramer_, DSI_CONDITION_VAR *pstCondResponseReady_)
{
   ANTMessageResponse *pclResponse;

   bResponseReady = FALSE;                                                                 //Init ResponseReady
   stMessageItem.stANTMessage.ucMessageID = ucMessageID_;                                  //Set mesg ID to look for
   ucBytesToMatch = ucBytesToMatch_;                                                       //Set number of data bytes to match
   memcpy( &(stMessageItem.stANTMessage.aucData[0]), pucData_, ucBytesToMatch_);           //Set data bytes to match


   if (pstCondResponseReady_ != NULL)
      pstCondResponseReady = pstCondResponseReady_;
   else
      pstCondResponseReady = &stCondResponseReady;


   pclFramer = pclFramer_;

   if (pclFramer == NULL)
      return FALSE;

   DSIThread_MutexLock(&(pclFramer->stMutexResponseRequest));                              // Lock the mutex and begin list manipulation

   pclResponse = pclFramer->pclResponseListStart;

   if (pclFramer->pclResponseListStart == NULL)                                            // Check if the list is empty
   {
      pclFramer->pclResponseListStart = this;                                              // Add this reponse object to the list
   }
   else
   {
      pclResponse = pclFramer->pclResponseListStart;                                       // If the list is not empty, walk through the list
     while (pclResponse->pclNext != NULL)
      {
         pclResponse = pclResponse->pclNext;
      }
     pclResponse->pclNext = this;                                                          // Add ourself to the end of the list
   }

   DSIThread_MutexUnlock(&(pclFramer->stMutexResponseRequest));                            // Unlock mutex when we're done

   return TRUE;
}

///////////////////////////////////////////////////////////////////////

void ANTMessageResponse::Remove()
{
   ANTMessageResponse **ppclResponse;

   if (pclFramer == NULL)
      return;

   DSIThread_MutexLock(&(pclFramer->stMutexResponseRequest));                              // Lock the mutex and begin list manipulation

   ppclResponse = &(pclFramer->pclResponseListStart);                                      // Set the ppointer to point to pclResponseListStart

   while (*ppclResponse != NULL)                                                           // While the pclNext is not NULL
   {
      if (*ppclResponse == this)                                                           // Check if pclNext is pointing to us
     {
        *ppclResponse = pclNext;                                                       // Remove this object from the List by changing the pointer to point to the element behind us

       if (pclNext == NULL)                                                              // If we are at the end of the list, then break because we are done.
           break;
     }
     else
     {
        ppclResponse = &((*ppclResponse)->pclNext);                                       // Advance the ppointer to point to the pclNext element of the next object in the list
     }
   }

   DSIThread_MutexUnlock(&(pclFramer->stMutexResponseRequest));                            // Unlock mutex when we're done
}

///////////////////////////////////////////////////////////////////////
BOOL ANTMessageResponse::WaitForResponse(ULONG ulMilliseconds_)
{
   DSIThread_MutexLock(&(pclFramer->stMutexResponseRequest));

   if ((bResponseReady == FALSE) && (ulMilliseconds_ != 0))
   {
      DSIThread_CondTimedWait(pstCondResponseReady, &(pclFramer->stMutexResponseRequest), ulMilliseconds_);
   }

   DSIThread_MutexUnlock(&(pclFramer->stMutexResponseRequest));

   return bResponseReady;
}

