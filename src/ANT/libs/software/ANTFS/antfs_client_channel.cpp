/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#include "dsi_framer_ant.hpp"
#include "dsi_serial_generic.hpp"
#include "dsi_thread.h"
#include "dsi_timer.hpp"
#include "dsi_convert.h"
#include "crc.h"

#include "antfs_client_channel.hpp"
#include "antfsmessage.h"
#include "config.h"

#include "dsi_debug.hpp"
#if defined(DEBUG_FILE)
   #include "macros.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


//////////////////////////////////////////////////////////////////////////////////
// Private Definitions
//////////////////////////////////////////////////////////////////////////////////

static const UCHAR caucNetworkKey[] = NETWORK_KEY;

//////////////////////////////////////////////////////////////////////////////////
// Public Functions
//////////////////////////////////////////////////////////////////////////////////
ANTFSClientChannel::ANTFSClientChannel()
{
   bInitFailed = FALSE;
   bTimerRunning = FALSE;

   bKillThread = FALSE;
   bANTFSThreadRunning = FALSE;

   pclANT = (DSIFramerANT*) NULL;
   pbCancel = &bCancel;
   *pbCancel = FALSE;

   // Default channel configuration
   ucChannelNumber = ANTFS_CHANNEL;
   ucNetworkNumber = ANTFS_NETWORK;
   usRadioChannelID = ANTFS_CLIENT_NUMBER;
   ucTheDeviceType = ANTFS_DEVICE_TYPE;
   ucTheTransmissionType = ANTFS_TRANSMISSION_TYPE;
   usTheMessagePeriod = ANTFS_MESSAGE_PERIOD;
   usBeaconChannelPeriod = ANTFS_MESSAGE_PERIOD;
   memcpy(aucTheNetworkkey,caucNetworkKey,8);

   ucLinkTxPower = RADIO_TX_POWER_LVL_3;
   ucSessionTxPower = RADIO_TX_POWER_LVL_3;
   bCustomTxPower = FALSE;

   // Default beacon configuration
   SetDefaultBeacon();
   memset(aucFriendlyName, 0, sizeof(aucFriendlyName));
   memset(aucPassKey, 0, sizeof(aucPassKey));
   ucPassKeySize = 0;
   ucFriendlyNameSize = 0;
   memset(&stHostDisconnectParams, 0, sizeof(stHostDisconnectParams));

   eANTFSState = ANTFS_CLIENT_STATE_OFF;
   ResetClientState();

   // Debugging is initialized by DSIANTDevice (or ANTDevice in Managed Lib)

   // Timer
   pclTimer = (DSITimer*)NULL;

   // Threading
   hANTFSThread = (DSI_THREAD_ID)NULL;    // Handle for the ANTFS thread

   if (DSIThread_MutexInit(&stMutexCriticalSection) != DSI_THREAD_ENONE)
   {
      bInitFailed = TRUE;
   }

   if (DSIThread_MutexInit(&stMutexResponseQueue) != DSI_THREAD_ENONE)
   {
      bInitFailed = TRUE;
   }

   if (DSIThread_CondInit(&stCondANTFSThreadExit) != DSI_THREAD_ENONE)
   {
      bInitFailed = TRUE;
   }

   if (DSIThread_CondInit(&stCondRequest) != DSI_THREAD_ENONE)
   {
      bInitFailed = TRUE;
   }

   if (DSIThread_CondInit(&stCondRxEvent) != DSI_THREAD_ENONE)
   {
      bInitFailed = TRUE;
   }

   if (DSIThread_CondInit(&stCondWaitForResponse) != DSI_THREAD_ENONE)
   {
      bInitFailed = TRUE;
   }
}

///////////////////////////////////////////////////////////////////////
ANTFSClientChannel::~ANTFSClientChannel()
{
   this->Close();

   if (bInitFailed == FALSE)
   {
      DSIThread_MutexDestroy(&stMutexCriticalSection);
      DSIThread_MutexDestroy(&stMutexResponseQueue);
      DSIThread_CondDestroy(&stCondANTFSThreadExit);
      DSIThread_CondDestroy(&stCondRequest);
      DSIThread_CondDestroy(&stCondRxEvent);
      DSIThread_CondDestroy(&stCondWaitForResponse);
   }
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSClientChannel::Init(DSIFramerANT* pclANT_, UCHAR ucChannel_)
{
   if (bInitFailed == TRUE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::Init():  bInitFailed == TRUE");
      #endif
      return FALSE;
   }

   ucChannelNumber = ucChannel_;
   pclANT = pclANT_;

   if(pclANT)
   {
      if(pclANT->GetCancelParameter() == (BOOL*) NULL)   // If cancel parameter has not been configured in framer, use internal
         pclANT->SetCancelParameter(pbCancel);
      else  // Use cancel parameter configured in framer
         pbCancel = pclANT->GetCancelParameter();
   }

   return ReInitDevice();
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::Close(void)
{
   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSClientChannel::Close():  Closing ANTFS...");
   #endif

   DSIThread_MutexLock(&stMutexCriticalSection);

   // Stop the threads.
   bKillThread = TRUE;
   *pbCancel = TRUE;

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSClientChannel::Close():  SetEvent(stCondWaitForResponse).");
   #endif

   DSIThread_MutexLock(&stMutexResponseQueue);
      DSIThread_CondSignal(&stCondWaitForResponse);
      clResponseQueue.Clear();
   DSIThread_MutexUnlock(&stMutexResponseQueue);

   if (hANTFSThread)
   {
      if (bANTFSThreadRunning == TRUE)
     {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::Close():  SetEvent(stCondRequest).");
         #endif
         DSIThread_CondSignal(&stCondRequest);

         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::Close():  SetEvent(stCondRxEvent).");
         #endif
         DSIThread_CondSignal(&stCondRxEvent);


         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::Close():  Killing thread.");
         #endif

         if (DSIThread_CondTimedWait(&stCondANTFSThreadExit, &stMutexCriticalSection, 9000) != DSI_THREAD_ENONE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSClientChannel::Close():  Thread not dead.");
               DSIDebug::ThreadWrite("ANTFSClientChannel::Close():  Forcing thread termination...");
            #endif
            DSIThread_DestroyThread(hANTFSThread);
         }
         else
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSClientChannel::Close():  Thread terminated successfully.");
            #endif
         }
      }

      DSIThread_ReleaseThreadID(hANTFSThread);
      hANTFSThread = (DSI_THREAD_ID)NULL;
   }

   DSIThread_MutexUnlock(&stMutexCriticalSection);

   if (bTimerRunning)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::Close():  Deleting Command Timer...");
      #endif

      delete pclTimer;
      pclTimer = (DSITimer*)NULL;
      DSIThread_MutexDestroy(&stMutexPairingTimeout);

      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::Close():  Deleted Timer.");
      #endif

      bTimerRunning = FALSE;
   }

   eANTFSState = ANTFS_CLIENT_STATE_OFF;
   pclANT = (DSIFramerANT*) NULL;

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSClientChannel::Close():  Closed.");
   #endif

   if (pucTransferBufferDynamic)
   {
     delete[] pucTransferBufferDynamic;
     pucTransferBufferDynamic = (UCHAR*)NULL;
   }
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::Cancel(void)
{
   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSClientChannel::Cancel(): Cancel current operation...");
   #endif

   DSIThread_MutexLock(&stMutexCriticalSection);

   *pbCancel = TRUE;

   DSIThread_CondSignal(&stCondRxEvent);
   DSIThread_CondSignal(&stCondRequest);

   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return;
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::ProcessDeviceNotification(ANT_DEVICE_NOTIFICATION eCode_, void* pvParameter_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if(eCode_ != ANT_DEVICE_NOTIFICATION_RESET &&
      eCode_ != ANT_DEVICE_NOTIFICATION_SHUTDOWN)
   {
      #if defined(DEBUG_FILE)
         UCHAR aucString[256];
         SNPRINTF((char *) aucString, 256, "ANTFSClientChannel::ProcessDeviceNotification():  Unknown code %0", eCode_);
         DSIDebug::ThreadWrite((char *) aucString);
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return;
   }

   if(eANTFSState <= ANTFS_CLIENT_STATE_IDLE)
   {
      // We do not need to do anything, since ANT-FS is already in idle state
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return;
   }

   if(pvParameter_ != NULL)
   {
      //Currently we don't have any codes we have data with, so log and ignore if we see something
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::ProcessDeviceNotification(): Received unrecognized pvParameter data");
      #endif
   }

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSClientChannel::ProcessDeviceNotification():  Resetting state...");
   #endif
   *pbCancel = TRUE;
   eANTFSRequest = ANTFS_REQUEST_INIT;
   DSIThread_CondSignal(&stCondRxEvent);
   DSIThread_CondSignal(&stCondRequest);
   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSClientChannel::ConfigureClientParameters(ANTFS_CLIENT_PARAMS* pstInitParams_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if(eANTFSState >= ANTFS_CLIENT_STATE_BEACONING)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::ConfigureClientParameters():  Incorrect state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   stInitParams = *pstInitParams_;
   ucActiveBeaconStatus1 = 0;
   ucActiveBeaconStatus1 |= ((stInitParams.ucLinkPeriod & BEACON_PERIOD_MASK) << BEACON_PERIOD_SHIFT);
   ucActiveBeaconStatus1 |= stInitParams.bPairingEnabled * PAIRING_AVAILABLE_FLAG_MASK;
   ucActiveBeaconStatus1 |= stInitParams.bUploadEnabled * UPLOAD_ENABLED_FLAG_MASK;
   ucActiveBeaconStatus1 |= stInitParams.bDataAvailable * DATA_AVAILABLE_FLAG_MASK;

   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN  ANTFSClientChannel::SetPairingEnabled(BOOL bEnable_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if(eANTFSState == ANTFS_CLIENT_STATE_AUTHENTICATING || eANTFSState == ANTFS_CLIENT_STATE_PAIRING_WAIT_FOR_RESPONSE ||
      (eANTFSState == ANTFS_CLIENT_STATE_CONNECTED && ucLinkCommandInProgress != ANTFS_CMD_NONE))
   {
      // Should not change the pairing capabilities while processing an authentication request
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SetPairingEnabled():  Busy processing an authentication request.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   stInitParams.bPairingEnabled = bEnable_;
   if(bEnable_)
      ucActiveBeaconStatus1 |= PAIRING_AVAILABLE_FLAG_MASK;
   else
      ucActiveBeaconStatus1 &= ~PAIRING_AVAILABLE_FLAG_MASK;

   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSClientChannel::SetUploadEnabled(BOOL bEnable_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);


   if(eANTFSState == ANTFS_CLIENT_STATE_UPLOADING || eANTFSState == ANTFS_CLIENT_STATE_UPLOADING_WAIT_FOR_RESPONSE ||
      (eANTFSState == ANTFS_CLIENT_STATE_TRANSPORT && ucLinkCommandInProgress != ANTFS_CMD_NONE))
   {
      // Should not change the upload capabilities while processing an upload
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SetUploadEnabled():  Busy processing a transport request.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   stInitParams.bUploadEnabled = bEnable_;
   if(bEnable_)
      ucActiveBeaconStatus1 |= UPLOAD_ENABLED_FLAG_MASK;
   else
      ucActiveBeaconStatus1 &= ~UPLOAD_ENABLED_FLAG_MASK;
   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSClientChannel::SetDataAvailable(BOOL bDataAvailable_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if(eANTFSState == ANTFS_CLIENT_STATE_DOWNLOADING || eANTFSState == ANTFS_CLIENT_STATE_DOWNLOADING_WAIT_FOR_DATA ||
      (eANTFSState == ANTFS_CLIENT_STATE_TRANSPORT && ucLinkCommandInProgress != ANTFS_CMD_NONE))
   {
      // Should not change the data available bit while processing a download
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SetDataAvailable():  Busy processing a transport request.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   stInitParams.bDataAvailable = bDataAvailable_;
   if(bDataAvailable_)
      ucActiveBeaconStatus1 |= DATA_AVAILABLE_FLAG_MASK;
   else
      ucActiveBeaconStatus1 &= ~DATA_AVAILABLE_FLAG_MASK;

   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::SetBeaconTimeout(UCHAR ucTimeout_)
{
   stInitParams.ucBeaconTimeout = ucTimeout_;
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::SetPairingTimeout(UCHAR ucTimeout_)
{
   stInitParams.ucPairingTimeout = ucTimeout_;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSClientChannel::SetFriendlyName(UCHAR* pucFriendlyName_, UCHAR ucFriendlyNameSize_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);
   if(eANTFSState == ANTFS_CLIENT_STATE_AUTHENTICATING)
   {
      // Should not change the friendly name while sending an authentication response
      #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSClientChannel::SetFriendlyName():  Busy authenticating.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   memset(aucFriendlyName, 0, sizeof(aucFriendlyName));

   if(pucFriendlyName_)
   {
      ucFriendlyNameSize = ucFriendlyNameSize_;
      memcpy(aucFriendlyName, pucFriendlyName_, ucFriendlyNameSize);
   }
   else
   {
      ucFriendlyNameSize = 0;
   }

   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSClientChannel::SetPassKey(UCHAR* pucPassKey_, UCHAR ucPassKeySize_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);
   if(eANTFSState == ANTFS_CLIENT_STATE_CONNECTED && ucLinkCommandInProgress != ANTFS_CMD_NONE)
   {
      // Should not change the passkey while we are processing a request and comparing the keys
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SetPassKey():  Busy processing an authentication request.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   memset(aucPassKey, 0, sizeof(aucPassKey));

   if(pucPassKey_)
   {
      ucPassKeySize = ucPassKeySize_;
      memcpy(aucPassKey, pucPassKey_, ucPassKeySize);
   }
   else
   {
      ucPassKeySize = 0;
   }

   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::SetChannelID(UCHAR ucDeviceType_, UCHAR ucTransmissionType_)
{
   ucTheDeviceType = ucDeviceType_;
   ucTheTransmissionType = ucTransmissionType_;
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::SetChannelPeriod(USHORT usChannelPeriod_)
{
   usBeaconChannelPeriod = usChannelPeriod_;
   // TODO: Should we also change the content of the beacon?
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::SetNetworkKey(UCHAR ucNetwork_, UCHAR ucNetworkkey[])
{
   ucNetworkNumber = ucNetwork_;
   memcpy(aucTheNetworkkey,ucNetworkkey,8);
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::SetTxPower(UCHAR ucPairingLv_, UCHAR ucConnectedLv_)
{
   ucLinkTxPower = ucPairingLv_;
   ucSessionTxPower = ucConnectedLv_;
   bCustomTxPower = TRUE;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSClientChannel::OpenBeacon()
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   #if defined(DEBUG_FILE)
   DSIDebug::ThreadInit("ANTCloseBeacon");
   #endif

   if (eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::OpenBeacon():  Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if (eANTFSState != ANTFS_CLIENT_STATE_IDLE)
   {
      #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSCient::OpenBeacon():  Not in correct state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   #if defined(DEBUG_FILE)
   DSIDebug::ThreadWrite("ANTFSClientChannel::OpenBeacon():  Beacon starting...");
   #endif

   memset(&stHostDisconnectParams, 0, sizeof(stHostDisconnectParams)); // Clear old disconnect parameters
   ConfigureClientParameters(&stInitParams);
   SetANTChannelPeriod(stInitParams.ucLinkPeriod);
   ucActiveBeaconFrequency = stInitParams.ucBeaconFrequency;
   if(stInitParams.ulSerialNumber & 0x0000FFFF)
   {
      usRadioChannelID = (USHORT) (stInitParams.ulSerialNumber & 0x0000FFFF);   // make sure ANT device number is not zero
   }

   eANTFSRequest = ANTFS_REQUEST_OPEN_BEACON;
   DSIThread_CondSignal(&stCondRequest);
   DSIThread_MutexUnlock(&stMutexCriticalSection);

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSClientChannel::OpenBeacon():  Open beacon request pending...");
   #endif

   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSClientChannel::CloseBeacon(BOOL bReturnToBroadcast_)
{
   ANTFS_RETURN eReturn = ANTFS_RETURN_PASS;
   DSIThread_MutexLock(&stMutexCriticalSection);

   if (eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::CloseBeacon():  Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if (eANTFSState < ANTFS_CLIENT_STATE_OPENING)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::CloseBeacon():  Beacon is already closed.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   #if defined(DEBUG_FILE)
   DSIDebug::ThreadWrite("ANTFSClientChannel::CloseBeacon():  Beacon closing...");
   #endif

   bReturnToBroadcast = bReturnToBroadcast_;
   eANTFSRequest = ANTFS_REQUEST_CLOSE_BEACON;

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSClientChannel::CloseBeacon():  Close beacon request pending...");
   #endif

   DSIThread_CondSignal(&stCondRequest);
   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return eReturn;
}

///////////////////////////////////////////////////////////////////////
#define MESG_CHANNEL_OFFSET                  0
#define MESG_EVENT_ID_OFFSET                 1
#define MESG_EVENT_CODE_OFFSET               2

void ANTFSClientChannel::ProcessMessage(ANT_MESSAGE* pstMessage_, USHORT usMesgSize_)
{
   UCHAR ucANTChannel;
   BOOL bProcessed = FALSE;

   #if defined(DEBUG_FILE)
   DSIDebug::ThreadInit("ANTReceive");
   #endif

   if(!GetEnabled())
      return; // Only process ANT messages if ANT-FS is on

   if (usMesgSize_ < DSI_FRAMER_TIMEDOUT)  //if the return isn't DSI_FRAMER_TIMEDOUT or DSI_FRAMER_ERROR
   {
      ucANTChannel = pstMessage_->aucData[MESG_CHANNEL_OFFSET] & CHANNEL_NUMBER_MASK;
      if(!FilterANTMessages(pstMessage_, ucANTChannel))
         return;

      switch (pstMessage_->ucMessageID)
      {
         case MESG_RESPONSE_EVENT_ID:
           if (pstMessage_->aucData[MESG_EVENT_ID_OFFSET] != MESG_EVENT_ID) // this is a response
           {
              memcpy(aucResponseBuf, pstMessage_->aucData, MESG_RESPONSE_EVENT_SIZE);
              bProcessed = ANTProtocolEventProcess(ucANTChannel, MESG_RESPONSE_EVENT_ID);
           }
           else // this is an event
           {
              memcpy(aucRxBuf, pstMessage_->aucData, usMesgSize_);
              bProcessed = ANTChannelEventProcess(ucANTChannel, pstMessage_->aucData[MESG_EVENT_CODE_OFFSET]); // pass through any events not handled here
           }
           break;

         case MESG_BROADCAST_DATA_ID:
           //Call channel event function with Broadcast message code
           memcpy(aucRxBuf, pstMessage_->aucData, usMesgSize_);
           bProcessed = ANTChannelEventProcess(ucANTChannel, EVENT_RX_BROADCAST);
           break;

         case MESG_ACKNOWLEDGED_DATA_ID:
           //Call channel event function with Acknowledged message code
           memcpy(aucRxBuf, pstMessage_->aucData, usMesgSize_);
           bProcessed = ANTChannelEventProcess(ucANTChannel, EVENT_RX_ACKNOWLEDGED);
           break;

         case MESG_BURST_DATA_ID:
           //Call channel event function with Burst message code
           memcpy(aucRxBuf, pstMessage_->aucData, usMesgSize_);
           bProcessed = ANTChannelEventProcess(ucANTChannel, EVENT_RX_BURST_PACKET);
           break;

         case MESG_EXT_BROADCAST_DATA_ID:
           //Call channel event function with Broadcast message code
           memcpy(aucRxBuf, pstMessage_->aucData, usMesgSize_);
           bProcessed = ANTChannelEventProcess(ucANTChannel, EVENT_RX_EXT_BROADCAST);
           break;

         case MESG_EXT_ACKNOWLEDGED_DATA_ID:
           //Call channel event function with Acknowledged message code
           memcpy(aucRxBuf, pstMessage_->aucData, usMesgSize_);
           bProcessed = ANTChannelEventProcess(ucANTChannel, EVENT_RX_EXT_ACKNOWLEDGED);
           break;

         case MESG_EXT_BURST_DATA_ID:
           //Call channel event function with Burst message code
           memcpy(aucRxBuf, pstMessage_->aucData, usMesgSize_);
           bProcessed = ANTChannelEventProcess(ucANTChannel, EVENT_RX_EXT_BURST_PACKET);
           break;

         case MESG_RSSI_BROADCAST_DATA_ID:
           //Call channel event function with Broadcast message code
           memcpy(aucRxBuf, pstMessage_->aucData, usMesgSize_);
           bProcessed = ANTChannelEventProcess(ucANTChannel, EVENT_RX_RSSI_BROADCAST);
           break;

         case MESG_RSSI_ACKNOWLEDGED_DATA_ID:
           //Call channel event function with Acknowledged message code
           memcpy(aucRxBuf, pstMessage_->aucData, usMesgSize_);
           bProcessed = ANTChannelEventProcess(ucANTChannel, EVENT_RX_RSSI_ACKNOWLEDGED);
           break;

         case MESG_RSSI_BURST_DATA_ID:
           //Call channel event function with Burst message code
           memcpy(aucRxBuf, pstMessage_->aucData, usMesgSize_);
           bProcessed = ANTChannelEventProcess(ucANTChannel, EVENT_RX_RSSI_BURST_PACKET);
           break;

         default:
           memcpy(aucResponseBuf, pstMessage_->aucData, usMesgSize_);
           bProcessed = ANTProtocolEventProcess(ucANTChannel, pstMessage_->ucMessageID );
           break;
       }
    }

   return;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSClientChannel::GetEnabled()
{
   if(eANTFSState < ANTFS_CLIENT_STATE_OPENING)
   {
      return FALSE;
   }

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
ANTFS_CLIENT_STATE ANTFSClientChannel::GetStatus(void)
{
   return eANTFSState;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSClientChannel::GetHostName(UCHAR *aucHostFriendlyName_, UCHAR *pucBufferSize_)
{
   if(!stHostFriendlyName.bNameSet)
   {
      *pucBufferSize_ = 0;
      return FALSE;
   }

   memset(aucHostFriendlyName_, 0, *pucBufferSize_);

   if (stHostFriendlyName.ucSize < *pucBufferSize_)
   {
      *pucBufferSize_ = stHostFriendlyName.ucSize;
   }

   memcpy(aucHostFriendlyName_, stHostFriendlyName.acFriendlyName, *pucBufferSize_);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSClientChannel::GetRequestParameters(ANTFS_REQUEST_PARAMS* pstRequestParams_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);
   if(ucLinkCommandInProgress == ANTFS_CMD_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::GetRequestParameters():  No request in progress.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return FALSE;
   }
   DSIThread_MutexUnlock(&stMutexCriticalSection);

   // Make a copy of the request parameters
   memcpy(pstRequestParams_, &stHostRequestParams, sizeof(stHostRequestParams));
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSClientChannel::GetRequestedFileIndex(USHORT* pusIndex_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);
   if(ucLinkCommandInProgress == ANTFS_CMD_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::GetRequestedFileIndex():  No request in progress.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return FALSE;
   }
   DSIThread_MutexUnlock(&stMutexCriticalSection);

   *pusIndex_ = stHostRequestParams.usFileIndex;
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSClientChannel::GetDownloadStatus(ULONG *pulByteProgress_, ULONG *pulTotalLength_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);
   if (eANTFSState < ANTFS_CLIENT_STATE_TRANSPORT)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::GetDownloadStatus(): Incorrect state.");
      #endif
      *pulTotalLength_ = 10;  // Avoid division by zero when calculating progress
      *pulByteProgress_ = 0;
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return FALSE;
   }

   if (ulTransferFileSize == 0)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::GetDownloadStatus():  Download not in progress.");
      #endif

      *pulTotalLength_ = 10;  // Avoid division by zero when calculating progress
      *pulByteProgress_ = 0;
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return FALSE;
   }

   *pulTotalLength_ = ulTransferBurstIndex + ulTransferBytesRemaining;
   *pulByteProgress_ = ulTransferBurstIndex;    // initialize with the current location within the data block

   if (ulDownloadProgress >= 24)
   {
      *pulByteProgress_ = *pulByteProgress_ + ulDownloadProgress - 24; // do not count the 24 bytes of the header
   }
   if (ulDownloadProgress >= ulTransferBytesRemaining + 24)
   {
      *pulByteProgress_ = *pulByteProgress_ - 8; // do not count the 8 byte footer
   }

   #if defined(DEBUG_FILE)
      UCHAR aucString[256];
      SNPRINTF((char*) aucString, 256, "ANTFSClientChannel::GetDownloadStatus(): %lu/%lu", *pulByteProgress_, *pulTotalLength_);
      DSIDebug::ThreadWrite((char*) aucString);
   #endif

   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSClientChannel::GetUploadStatus(ULONG *pulByteProgress_, ULONG *pulTotalLength_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);
   if (eANTFSState < ANTFS_CLIENT_STATE_TRANSPORT)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::GetUploadStatus(): Incorrect state.");
      #endif
      *pulTotalLength_ = 10;  // Avoid division by zero when calculating progress
      *pulByteProgress_ = 0;
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return FALSE;
   }

   *pulTotalLength_ = ulTransferBurstIndex + ulTransferBytesRemaining;
   *pulByteProgress_ = ulTransferBurstIndex;    // initialize with the current location within the data block

   if (*pulTotalLength_ == 0)
   {
      *pulTotalLength_ = 10;  // Avoid division by zero when calculating progress
      *pulByteProgress_ = 0;
   }

   /*
   if (ulDownloadProgress >= 24)
   {
      *pulByteProgress_ = *pulByteProgress_ + ulDownloadProgress - 24; // do not count the 24 bytes of the header
   }
   if (ulDownloadProgress >= ulTransferBytesRemaining + 24)
   {
      *pulByteProgress_ = *pulByteProgress_ - 8; // do not count the 8 byte footer
   }
   */
   #if defined(DEBUG_FILE)
      UCHAR aucString[256];
      SNPRINTF((char*) aucString, 256, "ANTFSClientChannel::GetUploadStatus(): %lu/%lu", *pulByteProgress_, *pulTotalLength_);
      DSIDebug::ThreadWrite((char*) aucString);
   #endif

   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSClientChannel::GetTransferData(ULONG *pulOffset_, ULONG *pulDataSize_ , void *pvData_)
{
   #if defined(DEBUG_FILE)
    DSIDebug::ThreadInit("ANTGetTransfer");
   #endif
   // TODO: Implement GetTransferData
   if (pucTransferBufferDynamic != NULL && ulTransferBurstIndex != 0)
   {
       *pulDataSize_ = ulTransferBurstIndex;
      *pulOffset_ = ulTransferBlockOffset;
      memcpy(pvData_, pucTransferBufferDynamic, ulTransferBurstIndex);
      return TRUE;
   }
   return FALSE; //SHOULD BE FALSE
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSClientChannel::GetDisconnectParameters(ANTFS_DISCONNECT_PARAMS* pstDisconnectParams_)
{
   // Make a copy of the requested parameters
   memcpy(pstDisconnectParams_, &stHostDisconnectParams, sizeof(stHostDisconnectParams));
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSClientChannel::SendPairingResponse(BOOL bAccept_)
{
   DSIThread_MutexLock(&stMutexPairingTimeout);
   if(bTimeoutEvent)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SendPairingResponse():  Pairing request timed out.");
      #endif
      DSIThread_MutexUnlock(&stMutexPairingTimeout);
      return ANTFS_RETURN_FAIL;
   }
   ucPairingTimeout = MAX_UCHAR;   // Disable timeout
   DSIThread_MutexUnlock(&stMutexPairingTimeout);


   DSIThread_MutexLock(&stMutexCriticalSection);

   if(stInitParams.bPairingEnabled != TRUE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SendPairingResponse():  Pairing not supported.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   if (eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SendPairingResponse():  Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if (eANTFSState != ANTFS_CLIENT_STATE_PAIRING_WAIT_FOR_RESPONSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SendPairingResponse():  Not in correct state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   bAcceptRequest = bAccept_;

   ucLinkCommandInProgress = ANTFS_AUTHENTICATE_ID;
   eANTFSState = ANTFS_CLIENT_STATE_AUTHENTICATING;
   eANTFSRequest = ANTFS_REQUEST_AUTHENTICATE;
   DSIThread_CondSignal(&stCondRequest);

   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSClientChannel::SendDownloadResponse(UCHAR ucResponse_, ANTFS_DOWNLOAD_PARAMS* pstDownloadInfo_, ULONG ulDataLength_, void *pvData_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if (eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SendDownloadResponse():   Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if (eANTFSState != ANTFS_CLIENT_STATE_DOWNLOADING_WAIT_FOR_DATA)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SendDownloadResponse():  Not in correct state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   if(pstDownloadInfo_ && usTransferDataFileIndex != pstDownloadInfo_->usFileIndex)
   {
      #if defined(DEBUG_FILE)
         UCHAR aucString[256];
         SNPRINTF((char*) aucString, 256, "ANTFSClientChannel::SendDownloadResponse(): This is not the file requested, expected %hu and got %hu",
            usTransferDataFileIndex, pstDownloadInfo_->usFileIndex);
         DSIDebug::ThreadWrite((char*) aucString);
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   ucRequestResponse = ucResponse_;
   if(ucRequestResponse != DOWNLOAD_RESPONSE_OK)
   {
      // Download is being rejected, there are no other parameters to check
      ulTransferBytesRemaining = 0;
      ulTransferBurstIndex = 0;
      ulTransferFileSize = 0;
      eANTFSRequest = ANTFS_REQUEST_DOWNLOAD_RESPONSE;
      DSIThread_CondSignal(&stCondRequest);
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_PASS;
   }

   ulTransferFileSize = ulDataLength_;      // File size of the requested download
   pucDownloadData = (UCHAR*)pvData_;   // Data block to download
   usTransferCrc = 0;   // Initialize to zero, as application only receives the initial request
   ulDownloadProgress = 0; // No data burst yet

   ulTransferBurstIndex = stHostRequestParams.ulOffset;   // Initialize current position in burst to offset requested by host
   if(ulTransferBurstIndex > ulTransferFileSize)
   {
      ulTransferBurstIndex = ulTransferFileSize;
   }

   ulTransferBytesRemaining = stHostRequestParams.ulBlockSize;   // Initialize number of remaining bytes to host specified maximum block size
   if((stHostRequestParams.ulBlockSize == 0) || (ulTransferFileSize < stHostRequestParams.ulBlockSize)) // If the host is not limiting download size or the file size does not exceed the host's download size limit
   {
      ulTransferBytesRemaining = ulTransferFileSize;   //  Number of bytes remaining to be downloaded in this block is the file size
   }

   if((ulTransferFileSize - ulTransferBurstIndex) < ulTransferBytesRemaining)
   {
      ulTransferBytesRemaining = ulTransferFileSize - ulTransferBurstIndex;  // Calculate number of remaining bytes in this block based on the offset
   }

   ulTransferBlockSize = pstDownloadInfo_->ulMaxBlockSize;
   if((pstDownloadInfo_->ulMaxBlockSize != 0) && (ulTransferBytesRemaining > pstDownloadInfo_->ulMaxBlockSize))   // If the application is limiting the block size
   {
      ulTransferBytesRemaining = pstDownloadInfo_->ulMaxBlockSize;   // Number of remaining bytes in this block is the application defined block size
   }


   eANTFSRequest = ANTFS_REQUEST_DOWNLOAD_RESPONSE;
   DSIThread_CondSignal(&stCondRequest);
   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSClientChannel::SendUploadResponse(UCHAR ucResponse_, ANTFS_UPLOAD_PARAMS* pstUploadInfo_, ULONG ulDataLength_, void *pvData_)
{
   //TODO: Implement this method fully. Will use ulDataLength_, pvData_ which are unreferenced right now.
   DSIThread_MutexLock(&stMutexCriticalSection);

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadInit("ANTFSUp/DownResponse");
   #endif

   if (eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SendUploadResponse():   Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if (eANTFSState != ANTFS_CLIENT_STATE_UPLOADING)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SendUploadResponse():  Not in correct state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   if(pstUploadInfo_ && usTransferDataFileIndex != pstUploadInfo_->usFileIndex)
   {
      #if defined(DEBUG_FILE)
         UCHAR aucString[256];
         SNPRINTF((char*) aucString, 256, "ANTFSClientChannel::SendUploadResponse(): This is not the file requested, expected %hu and got %hu",
            usTransferDataFileIndex, pstUploadInfo_->usFileIndex);
         DSIDebug::ThreadWrite((char*) aucString);
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   ucRequestResponse = ucResponse_;
   if(ucRequestResponse != UPLOAD_RESPONSE_OK)
   {
      // Upload is being rejected, there are no other parameters to check
      ulTransferBlockOffset = 0;
      ulTransferMaxIndex = 0;
      ulTransferBlockSize = 0;
      usTransferCrc = 0;
      eANTFSRequest = ANTFS_REQUEST_UPLOAD_RESPONSE;
      DSIThread_CondSignal(&stCondRequest);
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_PASS;
   }

   if(stHostRequestParams.ulMaxSize > pstUploadInfo_->ulMaxSize)
      ucRequestResponse = UPLOAD_RESPONSE_INSUFFICIENT_SPACE;

   if((stHostRequestParams.ulOffset > stHostRequestParams.ulMaxSize) && (stHostRequestParams.ulOffset != MAX_ULONG))
      ucRequestResponse = UPLOAD_RESPONSE_REQUEST_INVALID;

   ulTransferMaxIndex = pstUploadInfo_->ulMaxSize;

   if(pstUploadInfo_->ulMaxBlockSize)
      ulTransferBlockSize = pstUploadInfo_->ulMaxBlockSize;
   else
      ulTransferBlockSize = ulTransferMaxIndex;

  // ulTransferBurstIndex = 0;
  // ulTransferBlockOffset = 0;
  // usTransferCrc = 0;

   ulTransferBytesRemaining = ulTransferMaxIndex;
   if(stHostRequestParams.ulOffset != MAX_ULONG)
   {
      ulTransferBytesRemaining = stHostRequestParams.ulMaxSize - stHostRequestParams.ulOffset;
      ulTransferBlockOffset = stHostRequestParams.ulOffset;
      usTransferCrc = 0;   // TODO: Validate that data was provided and calculate CRC.
     usSavedTransferCrc = 0;


      if (pucTransferBufferDynamic != NULL)
     {
         delete [] pucTransferBufferDynamic;
     }
      pucTransferBufferDynamic = new UCHAR[ulTransferBytesRemaining];
     #if defined(DEBUG_FILE)
      char c2Buffer[256];
      SNPRINTF(c2Buffer, 256,"ANTFSClientChannel::SendUploadResponse():  Buffer Dynamic Size is: %d.",ulTransferBytesRemaining);
      DSIDebug::ThreadWrite(c2Buffer);
      #endif
   }
   else
   {
      usTransferCrc = usSavedTransferCrc;
      ulTransferBlockOffset += ulTransferBurstIndex;
     #if defined(DEBUG_FILE)
      char cBuffer[256];
      SNPRINTF(cBuffer, 256,"ANTFSClientChannel::SendUploadResponse():  Transfercrc is: %d.",usTransferCrc);
      DSIDebug::ThreadWrite(cBuffer);
      #endif
   }


   eANTFSRequest = ANTFS_REQUEST_UPLOAD_RESPONSE;
   DSIThread_CondSignal(&stCondRequest);
   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSClientChannel::SendEraseResponse(UCHAR ucResponse_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if (eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SendEraseResponse():  Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if (eANTFSState != ANTFS_CLIENT_STATE_ERASING)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SendEraseResponse():  Not in correct state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   ucRequestResponse = ucResponse_;

   ucLinkCommandInProgress = ANTFS_ERASE_ID;
   eANTFSState = ANTFS_CLIENT_STATE_ERASING;
   eANTFSRequest = ANTFS_REQUEST_ERASE_RESPONSE;
   DSIThread_CondSignal(&stCondRequest);

   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
//Returns a response if there is one ready, otherwise waits the specified time for one to occur
ANTFS_CLIENT_RESPONSE ANTFSClientChannel::WaitForResponse(ULONG ulMilliseconds_)
{
   ANTFS_CLIENT_RESPONSE stResponse = ANTFS_CLIENT_RESPONSE_NONE;

   if (bKillThread == TRUE)
      return ANTFS_CLIENT_RESPONSE_NONE;

   //Wait for response
   DSIThread_MutexLock(&stMutexResponseQueue);
      if(clResponseQueue.isEmpty())
      {
         UCHAR ucResult = DSIThread_CondTimedWait(&stCondWaitForResponse, &stMutexResponseQueue, ulMilliseconds_);
         switch(ucResult)
         {
            case DSI_THREAD_ENONE:
               stResponse = clResponseQueue.GetResponse();
               break;

            case DSI_THREAD_ETIMEDOUT:
               stResponse = ANTFS_CLIENT_RESPONSE_NONE;
               break;

            case DSI_THREAD_EOTHER:
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSClientChannel::WaitForResponse():  CondTimedWait() Failed!");
               #endif
               stResponse = ANTFS_CLIENT_RESPONSE_NONE;
               break;

            default:
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSClientChannel::WaitForResponse():  Error  Unknown...");
               #endif
               stResponse = ANTFS_CLIENT_RESPONSE_NONE;
               break;
         }
      }
      else
      {
         stResponse = clResponseQueue.GetResponse();
      }
   DSIThread_MutexUnlock(&stMutexResponseQueue);

   return stResponse;
}


//////////////////////////////////////////////////////////////////////////////////
// Private Functions
//////////////////////////////////////////////////////////////////////////////////
DSI_THREAD_RETURN ANTFSClientChannel::ANTFSThreadStart(void *pvParameter_)
{
   #if defined(DEBUG_FILE)
      DSIDebug::ThreadInit("ANTFSClient");
   #endif

   ((ANTFSClientChannel *)pvParameter_)->ANTFSThread();

   return 0;
}

///////////////////////////////////////////////////////////////////////
// ANTFS Task Thread
///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::ANTFSThread(void)
{
   ANTFS_CLIENT_RESPONSE eResponse;
   bANTFSThreadRunning = TRUE;

   while (bKillThread == FALSE)
   {
      eResponse = ANTFS_CLIENT_RESPONSE_NONE;

      DSIThread_MutexLock(&stMutexCriticalSection);

      if (*pbCancel)
      {
         *pbCancel = FALSE;

         if (eANTFSRequest != ANTFS_REQUEST_INIT && eANTFSRequest != ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
         {
            eANTFSRequest = ANTFS_REQUEST_NONE;                    //Clear any other request
         }

         AddResponse(ANTFS_CLIENT_RESPONSE_CANCEL_DONE);
      }
      if ((eANTFSRequest == ANTFS_REQUEST_NONE) && (bKillThread == FALSE))
      {
         UCHAR ucResult = DSIThread_CondTimedWait(&stCondRequest, &stMutexCriticalSection, (ULONG) (stInitParams.ucBeaconTimeout * 1000));
         if (ucResult != DSI_THREAD_ENONE)
         {
            #if defined(DEBUG_FILE)
               if(ucResult == DSI_THREAD_EOTHER)
                  DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): CondTimedWait() Failed!");
            #endif
            // If connected to a host, and we have not received any requests, go back to link state
            if ((eANTFSRequest == ANTFS_REQUEST_NONE)  && (stInitParams.ucBeaconTimeout != CMD_TIMEOUT_DISABLED) && (eANTFSState >= ANTFS_CLIENT_STATE_CONNECTED) && (ucLinkCommandInProgress == ANTFS_CMD_NONE))
            {
               eANTFSRequest = ANTFS_REQUEST_CONNECTION_LOST;
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): No requests received, dropping back to link");
               #endif
            }
            // If we are in link state, reload the beacon
            // We do this in order to be able to detect serial failures while in this state
            if((eANTFSState == ANTFS_CLIENT_STATE_BEACONING) && (eANTFSRequest == ANTFS_REQUEST_NONE))
            {
               if(SwitchToLink() == RETURN_SERIAL_ERROR)
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): Serial error while beaconing");
                  #endif

                  if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                     eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
               }
            }
            // If ping is disabled, go back to transport state if we were waiting for an application response
            // We need to do this to avoid getting stuck in busy state if the application never sends a response
            if((stInitParams.ucBeaconTimeout == CMD_TIMEOUT_DISABLED) && (eANTFSRequest == ANTFS_REQUEST_NONE))
            {
               if(eANTFSState == ANTFS_CLIENT_STATE_ERASING)
               {
                  ucRequestResponse = ERASE_RESPONSE_REJECT;
                  eANTFSRequest = ANTFS_REQUEST_ERASE_RESPONSE;
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): No response received for the erase request, rejecting...");
                  #endif
               }
               else if(eANTFSState == ANTFS_CLIENT_STATE_DOWNLOADING_WAIT_FOR_DATA)
               {
                  ucRequestResponse = DOWNLOAD_RESPONSE_NOT_READY;
                  ulTransferBytesRemaining = 0;
                  ulTransferBurstIndex = 0;
                  ulTransferFileSize = 0;
                  usTransferCrc = 0;
                  eANTFSRequest = ANTFS_REQUEST_DOWNLOAD_RESPONSE;
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): No response received for the download request, rejecting...");
                  #endif
               }
               else if(eANTFSState == ANTFS_CLIENT_STATE_UPLOADING_WAIT_FOR_RESPONSE)
               {
                  ucRequestResponse = UPLOAD_RESPONSE_NOT_READY;
                  ulTransferBlockOffset = 0;
                  ulTransferMaxIndex = 0;
                  ulTransferBlockSize = 0;
                  usTransferCrc = 0;
                  eANTFSRequest = ANTFS_REQUEST_UPLOAD_RESPONSE;
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): No response received for the upload request, rejecting...");
                  #endif
               }
            }
         }
      }
      DSIThread_MutexUnlock(&stMutexCriticalSection);

      if (bKillThread)
         break;

      if (eANTFSRequest != ANTFS_REQUEST_NONE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Request received");
         #endif

         switch (eANTFSRequest)
         {
            case ANTFS_REQUEST_INIT:
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): Idle.");
                  #endif

                  ResetClientState();
                  eANTFSState = ANTFS_CLIENT_STATE_IDLE;
                  eResponse = ANTFS_CLIENT_RESPONSE_INIT_PASS;
               }   // ANTFS_REQUEST_INIT
               break;

            case ANTFS_REQUEST_OPEN_BEACON:
               {
                  RETURN_STATUS eReturn;

                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Opening beacon...");
                  #endif

                  eANTFSState = ANTFS_CLIENT_STATE_OPENING;

                  eReturn = AttemptOpenBeacon();

                  if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Open beacon serial error.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Opening beacon failed.");
                     #endif
                     eANTFSState = ANTFS_CLIENT_STATE_IDLE;
                  }
                  else if (eReturn == RETURN_STOP)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): Opening beacon stopped.");
                     #endif
                     AttemptCloseBeacon();
                     eANTFSState = ANTFS_CLIENT_STATE_IDLE;
                  }
                  else if (eReturn == RETURN_PASS)
                  {
                     SwitchToLink();
                     eANTFSState = ANTFS_CLIENT_STATE_BEACONING;
                     eResponse = ANTFS_CLIENT_RESPONSE_BEACON_OPEN;

                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Beacon Open...");
                     #endif
                  }
               }   // ANTFS_REQUEST_OPEN_BEACON
               break;

            case ANTFS_REQUEST_CLOSE_BEACON:
               {
                  RETURN_STATUS eReturn;

                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Closing beacon...");
                  #endif

                  eReturn = AttemptCloseBeacon();

                  if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Close beacon serial error.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
                  else if (eReturn == RETURN_PASS)
                  {
                     ResetClientState();
                     eANTFSState = ANTFS_CLIENT_STATE_IDLE;
                     eResponse = ANTFS_CLIENT_RESPONSE_BEACON_CLOSED;
                  }
               }   // ANTFS_CLOSE_BEACON
               break;

            case ANTFS_REQUEST_CONNECT:
               {
                  RETURN_STATUS eReturn;

                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): Connecting to host device...");
                  #endif

                  eReturn = SwitchToAuthenticate();

                  if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): Serial error while connecting to host device.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Connecting to host device failed.");
                     #endif
                     eANTFSState = ANTFS_CLIENT_STATE_BEACONING;
                     eANTFSRequest = ANTFS_REQUEST_NONE;
                  }
                  else if (eReturn == RETURN_PASS)
                  {
                     eANTFSRequest = ANTFS_REQUEST_NONE;
                     eANTFSState = ANTFS_CLIENT_STATE_CONNECTED;
                     eResponse = ANTFS_CLIENT_RESPONSE_CONNECT_PASS;

                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Connected to host device...");
                     #endif
                  }
               }   // ANTFS_REQUEST_CONNECT
               break;

            case ANTFS_REQUEST_DISCONNECT:
               {
                  RETURN_STATUS eReturn;
                  ANTFS_CLIENT_STATE ePrevState = eANTFSState;

                  #if defined(DEBUG_FILE)
                      DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Disconnecting...");
                  #endif

                  eReturn = SwitchToLink();

                  if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): Serial error while disconnecting from host device.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Disconnecting from host device failed.");
                     #endif

                     eANTFSState = ePrevState;      // Remain in previous state
                     eANTFSRequest = ANTFS_REQUEST_NONE;
                  }
                  else if (eReturn == RETURN_PASS)
                  {
                     if(stHostDisconnectParams.ucCommandType == DISCONNECT_COMMAND_BROADCAST)
                     {
                        ResetClientState();
                        eANTFSRequest = ANTFS_REQUEST_NONE;
                        eANTFSState = ANTFS_CLIENT_STATE_IDLE;
                        eResponse = ANTFS_CLIENT_RESPONSE_DISCONNECT_PASS;

                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): End of ANT-FS session. Return to broadcast.");
                        #endif
                     }
                     else
                     {
                        eANTFSRequest = ANTFS_REQUEST_NONE;
                        eANTFSState = ANTFS_CLIENT_STATE_BEACONING;
                        eResponse = ANTFS_CLIENT_RESPONSE_DISCONNECT_PASS;

                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Disconnected from host device...");
                        #endif
                     }
                  }
               }   // ANTFS_REQUEST_DISCONNECT
               break;

            case ANTFS_REQUEST_PING:
               {
                  // Do nothing
                  eANTFSRequest = ANTFS_REQUEST_NONE;
               }   // ANTFS_REQUEST_PING
               break;

            case ANTFS_REQUEST_PAIR:
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Pairing request.");
                  #endif
                  DSIThread_MutexLock(&stMutexPairingTimeout);
                     eANTFSState = ANTFS_CLIENT_STATE_PAIRING_WAIT_FOR_RESPONSE;
                     eResponse = ANTFS_CLIENT_RESPONSE_PAIRING_REQUEST;
                     ucPairingTimeout = stInitParams.ucPairingTimeout;      // Changed from PAIRING_TIMEOUT to match the timeout at host, but this might be too short
                  DSIThread_MutexUnlock(&stMutexPairingTimeout);
               }   // ANTFS_REQUEST_PAIR
               break;

            case ANTFS_REQUEST_AUTHENTICATE:
               {
                  RETURN_STATUS eReturn;

                  DSIThread_MutexLock(&stMutexPairingTimeout);
                     if(bTimeoutEvent)
                     {
                        bAcceptRequest = FALSE;
                        bTimeoutEvent = FALSE;
                        AddResponse(ANTFS_CLIENT_RESPONSE_PAIRING_TIMEOUT);
                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Pairing timeout.");
                        #endif
                     }
                     eANTFSState = ANTFS_CLIENT_STATE_AUTHENTICATING;
                  DSIThread_MutexUnlock(&stMutexPairingTimeout);

                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Authenticating...");
                  #endif

                  eReturn = AttemptAuthenticateResponse();

                  if (eReturn == RETURN_PASS)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Authentication request accepted.");
                     #endif

                     SwitchToTransport(); // We passed auth, so go to transport state
                     eResponse = ANTFS_CLIENT_RESPONSE_AUTHENTICATE_PASS;
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Authentication failed.");
                     #endif
                     SwitchToAuthenticate();      // Stand by ready for retry
                  }
                  else if (eReturn == RETURN_REJECT)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Authentication request rejected.");
                     #endif

                     SwitchToLink();    // We failed auth, so go to link state
                     eResponse = ANTFS_CLIENT_RESPONSE_AUTHENTICATE_REJECT;
                  }
                  else if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Authentication serial error.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
                  else if (eReturn == RETURN_STOP)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Authentication stopped.");
                     #endif
                     SwitchToAuthenticate();
                  }
                  else //RETURN_NA
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Authentication NA.");
                     #endif

                     SwitchToAuthenticate();
                     eResponse = ANTFS_CLIENT_RESPONSE_AUTHENTICATE_NA;
                  }
               }   // ANTFS_REQUEST_AUTHENTICATE
               break;

            case ANTFS_REQUEST_CHANGE_LINK:
               {
                  RETURN_STATUS eReturn;

                  #if defined(DEBUG_FILE)
                      DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Changing radio frequency and channel period...");
                  #endif

                  eReturn = SwitchLinkParameters();
                  if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): Serial error while changing radio frequency/period.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Changing radio frequency and channel period failed.");
                     #endif
                     eANTFSRequest = ANTFS_REQUEST_NONE;
                  }
                  else if (eReturn == RETURN_PASS)
                  {
                     eANTFSRequest = ANTFS_REQUEST_NONE;
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Changed radio frequency and channel period.");
                     #endif
                  }
               }   // ANTFS_REQUEST_CHANGE_LINK
               break;

            case ANTFS_REQUEST_ERASE:
               {
                  #if defined(DEBUG_FILE)
                     UCHAR aucString[256];
                     SNPRINTF((char*) aucString, 256, "ANTFSClientChannel::ANTFSThread():  Erase request for index: %hu.", stHostRequestParams.usFileIndex);
                     DSIDebug::ThreadWrite((char*) aucString);
                  #endif
                  eANTFSState = ANTFS_CLIENT_STATE_ERASING;
                  eResponse = ANTFS_CLIENT_RESPONSE_ERASE_REQUEST;
               } // ANTFS_REQUEST_ERASE
               break;

            case ANTFS_REQUEST_ERASE_RESPONSE:
               {
                  RETURN_STATUS eReturn;

                  #if defined(DEBUG_FILE)
                     UCHAR aucString[256];
                     SNPRINTF((char*) aucString, 256, "ANTFSClientChannel::ANTFSThread(): Erasing... Response: %u.", ucRequestResponse);
                     DSIDebug::ThreadWrite((char*)aucString);
                  #endif

                  eReturn = AttemptEraseResponse();
                  if (eReturn == RETURN_PASS)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Erase complete.");
                     #endif

                     SwitchToTransport();
                     eResponse = ANTFS_CLIENT_RESPONSE_ERASE_PASS;
                  }
                  else if (eReturn == RETURN_REJECT)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Erase request rejected.");
                     #endif

                     SwitchToTransport();
                     eResponse = ANTFS_CLIENT_RESPONSE_ERASE_REJECT;
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Erase fail.");
                     #endif
                     SwitchToTransport();
                     eResponse = ANTFS_CLIENT_RESPONSE_ERASE_FAIL;
                  }
                  else if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Erase serial error.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);

                  }
                  else
                  {
                     SwitchToTransport();
                  }
               }   // ANTFS_REQUEST_ERASE_RESPONSE
               break;

            case ANTFS_REQUEST_DOWNLOAD:
               {
                  #if defined(DEBUG_FILE)
                     UCHAR aucString[256];
                     SNPRINTF((char*) aucString, 256, "ANTFSClientChannel::ANTFSThread():  Download request for index: %d.", stHostRequestParams.usFileIndex);
                     DSIDebug::ThreadWrite((char*) aucString);
                  #endif
                  usTransferDataFileIndex = stHostRequestParams.usFileIndex;
                  eANTFSState = ANTFS_CLIENT_STATE_DOWNLOADING_WAIT_FOR_DATA;
                  eResponse = ANTFS_CLIENT_RESPONSE_DOWNLOAD_REQUEST;
               } // ANTFS_REQUEST_DOWNLOAD
               break;

            case ANTFS_REQUEST_DOWNLOAD_RESPONSE:
               {
                  RETURN_STATUS eReturn;

                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): Downloading...");
                  #endif
                  eANTFSState = ANTFS_CLIENT_STATE_DOWNLOADING;
                  eReturn = AttemptDownloadResponse();

                  if (eReturn == RETURN_PASS)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Download completed.");
                     #endif

                     SwitchToTransport();
                     eResponse = ANTFS_CLIENT_RESPONSE_DOWNLOAD_PASS;
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Download failed.");
                     #endif
                     SwitchToTransport();
                     eResponse = ANTFS_CLIENT_RESPONSE_DOWNLOAD_FAIL;
                  }
                  else if (eReturn == RETURN_REJECT)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Download request rejected.");
                     #endif

                     SwitchToTransport();
                     switch(ucRequestResponse)
                     {
                        case DOWNLOAD_RESPONSE_DOES_NOT_EXIST:
                           eResponse = ANTFS_CLIENT_RESPONSE_DOWNLOAD_INVALID_INDEX;
                           break;
                        case DOWNLOAD_RESPONSE_NOT_DOWNLOADABLE:
                           eResponse = ANTFS_CLIENT_RESPONSE_DOWNLOAD_FILE_NOT_READABLE;
                           break;
                        case DOWNLOAD_RESPONSE_NOT_READY:
                           eResponse = ANTFS_CLIENT_RESPONSE_DOWNLOAD_NOT_READY;
                           break;
                        case DOWNLOAD_RESPONSE_REQUEST_INVALID:
                        default:
                           eResponse = ANTFS_CLIENT_RESPONSE_DOWNLOAD_REJECT;
                           break;
                     }
                  }
                  else if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): Download serial error.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
                  else
                  {
                     SwitchToTransport();
                  }

               } // ANTFS_REQUEST_DOWNLOAD_RESPONSE
               break;

               case ANTFS_REQUEST_UPLOAD:
               {
                  #if defined(DEBUG_FILE)
                     UCHAR aucString[256];
                     SNPRINTF((char*) aucString, 256, "ANTFSClientChannel::ANTFSThread():  Upload request for index: %d.", stHostRequestParams.usFileIndex);
                     DSIDebug::ThreadWrite((char*) aucString);
                  #endif

                usTransferDataFileIndex = stHostRequestParams.usFileIndex;
                     eANTFSState = ANTFS_CLIENT_STATE_UPLOADING;
                     eResponse = ANTFS_CLIENT_RESPONSE_UPLOAD_REQUEST;

                  // TODO: Implement uploads - for now, all upload requests are rejected
               } // ANTFS_REQUEST_UPLOAD
               break;

               case ANTFS_REQUEST_UPLOAD_RESPONSE:
               {
                  RETURN_STATUS eReturn;

                  eANTFSState = ANTFS_CLIENT_STATE_UPLOADING;
                  eReturn = AttemptUploadResponse();

                  if (eReturn == RETURN_PASS)
                  {
                     SwitchToTransport();
                     eANTFSState = ANTFS_CLIENT_STATE_UPLOADING_WAIT_FOR_RESPONSE;
                // no Response message yet?
                     //eResponse = ANTFS_CLIENT_RESPONSE_UPLOAD_PASS;
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Upload failed. UPLOAD RESPONSE");
                     #endif
                     SwitchToTransport();
                     eResponse = ANTFS_CLIENT_RESPONSE_UPLOAD_FAIL;
                  }
                  else if (eReturn == RETURN_REJECT)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Upload request rejected. UPLOAD RESPONSE");
                     #endif

                     SwitchToTransport();
                     switch(ucRequestResponse)
                     {
                        case UPLOAD_RESPONSE_DOES_NOT_EXIST:
                           eResponse = ANTFS_CLIENT_RESPONSE_UPLOAD_INVALID_INDEX;
                           break;
                        case UPLOAD_RESPONSE_NOT_WRITEABLE:
                           eResponse = ANTFS_CLIENT_RESPONSE_UPLOAD_FILE_NOT_WRITEABLE;
                           break;
                        case UPLOAD_RESPONSE_INSUFFICIENT_SPACE:
                           eResponse = ANTFS_CLIENT_RESPONSE_UPLOAD_INSUFFICIENT_SPACE;
                           break;
                        case UPLOAD_RESPONSE_REQUEST_INVALID:
                        default:
                           eResponse = ANTFS_CLIENT_RESPONSE_UPLOAD_REJECT;
                           break;
                     }
                  }
                  else if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): Upload serial error. UPLOAD RESPONSE");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
                  else
                  {
                     SwitchToTransport();
                  }
               } // ANTFS_REQUEST_UPLOAD_RESPONSE
               break;
         case ANTFS_REQUEST_UPLOAD_COMPLETE:
               {
                  RETURN_STATUS eReturn;

                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): Responding to Upload...");
                  #endif
                  eANTFSState = ANTFS_CLIENT_STATE_UPLOADING;
                  eReturn = AttemptUploadDataResponse();

                  if (eReturn == RETURN_PASS)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Upload response completed.");
                     #endif

                     SwitchToTransport();
                     //eANTFSState = ANTFS_CLIENT_STATE_UPLOADING_WAIT_FOR_RESPONSE;
                // no Response message yet?
                if (ucRequestResponse == UPLOAD_RESPONSE_OK)
                  eResponse = ANTFS_CLIENT_RESPONSE_UPLOAD_PASS;
                else
                  eResponse = ANTFS_CLIENT_RESPONSE_UPLOAD_FAIL;
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Upload failed. UPLOAD COMPLETE");
                     #endif
                     SwitchToTransport();
                     eResponse = ANTFS_CLIENT_RESPONSE_UPLOAD_FAIL;
                  }
                  else if (eReturn == RETURN_REJECT)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Upload request rejected. UPLOAD COMPLETE");
                     #endif

                     SwitchToTransport();
                     switch(ucRequestResponse)
                     {
                        case UPLOAD_RESPONSE_DOES_NOT_EXIST:
                           eResponse = ANTFS_CLIENT_RESPONSE_UPLOAD_INVALID_INDEX;
                           break;
                        case UPLOAD_RESPONSE_NOT_WRITEABLE:
                           eResponse = ANTFS_CLIENT_RESPONSE_UPLOAD_FILE_NOT_WRITEABLE;
                           break;
                        case UPLOAD_RESPONSE_INSUFFICIENT_SPACE:
                           eResponse = ANTFS_CLIENT_RESPONSE_UPLOAD_INSUFFICIENT_SPACE;
                           break;
                        case UPLOAD_RESPONSE_REQUEST_INVALID:
                        default:
                           eResponse = ANTFS_CLIENT_RESPONSE_UPLOAD_REJECT;
                           break;
                     }
                  }
                  else if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread(): Upload serial error. UPLOAD COMPLETE");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
                  else
                  {
                     SwitchToTransport();
                  }
               } // ANTFS_REQUEST_UPLOAD_RESPONSE

            break;
            default:
               break;
         }

         //This is where to handle the internal requests, because they can happen asyncronously.
         //We will also clear the request here.
         DSIThread_MutexLock(&stMutexCriticalSection);

         if (eResponse != ANTFS_CLIENT_RESPONSE_NONE)
            AddResponse(eResponse);

         if (eANTFSRequest == ANTFS_REQUEST_CONNECTION_LOST)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Connection lost.");
            #endif

            eANTFSRequest = ANTFS_REQUEST_NONE;

            if (eANTFSState >= ANTFS_CLIENT_STATE_CONNECTED)
            {
               SwitchToLink();
               AddResponse(ANTFS_CLIENT_RESPONSE_CONNECTION_LOST);
            }
            else
            {
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Connection lost - ignored.");
               #endif
            }
         }
         else if (eANTFSRequest == ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Serial error!");
            #endif
            HandleSerialError();
            AddResponse(ANTFS_CLIENT_RESPONSE_SERIAL_FAIL);
         }
         else if (eANTFSRequest == ANTFS_REQUEST_SERIAL_ERROR_HANDLED)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Serial error handled");
            #endif
            ResetClientState();
            eANTFSState = ANTFS_CLIENT_STATE_IDLE;
            eANTFSRequest = ANTFS_REQUEST_INIT;
         }
         else
         {
               eANTFSRequest = ANTFS_REQUEST_NONE;                    //Clear any other request
         }

         DSIThread_MutexUnlock(&stMutexCriticalSection);
      }

   }

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  Exiting thread.");
   #endif

   eANTFSRequest = ANTFS_REQUEST_NONE;

   DSIThread_MutexLock(&stMutexCriticalSection);
      bANTFSThreadRunning = FALSE;
      DSIThread_CondSignal(&stCondANTFSThreadExit);
   DSIThread_MutexUnlock(&stMutexCriticalSection);


   #if defined(__cplusplus)
      return;
   #else
      ExitThread(0);
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::ANTFSThread():  C code reaching return statement unexpectedly.");
      #endif
      return;                                            // Code should not be reached.
   #endif
}


/////////////////////////////////////////////////////////////////////
// Returns: TRUE if the message is for the ANT-FS channel
BOOL ANTFSClientChannel::FilterANTMessages(ANT_MESSAGE* pstMessage_, UCHAR ucANTChannel_)
{
   // Some messages do not include the channel number in the response, so
   // they might get processed incorrectly
   if(pstMessage_->ucMessageID == MESG_RESPONSE_EVENT_ID)
   {
      if(pstMessage_->aucData[MESG_EVENT_ID_OFFSET] == MESG_NETWORK_KEY_ID)
      {
         if(pstMessage_->aucData[MESG_CHANNEL_OFFSET] == ucNetworkNumber)
            return TRUE;  // this is the network we are using
      }
      else if(pstMessage_->aucData[MESG_EVENT_ID_OFFSET] == MESG_RADIO_TX_POWER_ID)
      {
         return TRUE; // configured by client if per channel settings not available
      }
   }
   else if(pstMessage_->ucMessageID == MESG_STARTUP_MESG_ID)
   {
      return TRUE;
   }

   if(ucANTChannel_ == ucChannelNumber)
      return TRUE;

   return FALSE;
}


///////////////////////////////////////////////////////////////////////
// Returns: TRUE if the message has been handled by ANT-FS client, FALSE otherwise
BOOL ANTFSClientChannel::ANTProtocolEventProcess(UCHAR ucChannel_, UCHAR ucMessageCode_)
{
   if ((ucMessageCode_ == MESG_RESPONSE_EVENT_ID) && (ucChannel_ == ucChannelNumber))
   {
      #if defined(DEBUG_FILE)
         UCHAR aucString[256];
         SNPRINTF((char *) aucString, 256, "ANTFSClientChannel::ANTProtocolEventProcess():  MESG_RESPONSE_EVENT_ID - 0x%02X", aucResponseBuf[1]);
         DSIDebug::ThreadWrite((char *) aucString);
      #endif

      if (aucResponseBuf[1] == MESG_BURST_DATA_ID)
      {
         if (aucResponseBuf[2] != RESPONSE_NO_ERROR)
         {
            #if defined(DEBUG_FILE)
               UCHAR aucString1[256];
               SNPRINTF((char *) aucString1, 256, "ANTFSClientChannel::ANTProtocolEventProcess():  Burst transfer error:  0x%02X.", aucResponseBuf[2]);
               DSIDebug::ThreadWrite((char *) aucString1);
            #endif
            bTxError = TRUE;
         }
      }
   }

   //else if (ucMessageCode_ == MESG_SERIAL_ERROR_ID)
   //{
   //   #if defined(DEBUG_FILE)
   //      {
   //         UCHAR aucString[256];
   //         SNPRINTF((char *) aucString, 256, "ANTFSClientChannel::ANTProtocolEventProcess():  Serial Error.");
   //         DSIDebug::ThreadWrite((char *) aucString);
   //      }
   //   #endif

   //   DSIThread_MutexLock(&stMutexCriticalSection);
   //   *pbCancel = TRUE;
   //   DSIThread_CondSignal(&stCondRxEvent);

   //   if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
   //      eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
   //   DSIThread_CondSignal(&stCondRequest);
   //   DSIThread_MutexUnlock(&stMutexCriticalSection);
   //}

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Returns: TRUE if the message has been handled by ANT-FS client, FALSE otherwise
BOOL ANTFSClientChannel::ANTChannelEventProcess(UCHAR ucChannel_, UCHAR ucMessageCode_)
{
   // Check that we're getting a message from the correct channel.
   if((ucChannel_ != ucChannelNumber) || ((aucRxBuf[0] & CHANNEL_NUMBER_MASK)  != ucChannelNumber))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::ANTChannelEventProcess():  Message received on wrong channel.");
      #endif
      return FALSE; // message can get passed on to the application
   }
   switch (ucMessageCode_)
   {
      case EVENT_RX_BROADCAST:
         break;   // we're not going to process broadcasts or pass them to the application

      case EVENT_RX_ACKNOWLEDGED:
         aucRxBuf[0] |= SEQUENCE_LAST_MESSAGE;   // mark it as being the last message and process as burst
      case EVENT_RX_BURST_PACKET:   // fall thru
         if (!bRxError)
         {
            if ((aucRxBuf[0] & SEQUENCE_NUMBER_ROLLOVER) == 0)  // Start of a burst.
            {
               // Check that this is an ANT-FS message
               if(aucRxBuf[ANTFS_CONNECTION_OFFSET + 1] != ANTFS_COMMAND_ID)
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::ANTChannelEventProcess():  Invalid ANT-FS message.");
                  #endif
                  bRxError = TRUE;
               }
               else
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::ANTChannelEventProcess():  Burst Rx started");
                  #endif
               }
               ulPacketCount = 1;
               bReceivedCommand = FALSE;

               if (aucRxBuf[0] & SEQUENCE_LAST_MESSAGE)
               {
                  if((aucRxBuf[ANTFS_COMMAND_OFFSET + 1] == ANTFS_DOWNLOAD_ID) ||
                     (aucRxBuf[ANTFS_COMMAND_OFFSET + 1] == ANTFS_UPLOAD_REQUEST_ID) ||
                     (aucRxBuf[ANTFS_COMMAND_OFFSET + 1] == ANTFS_UPLOAD_DATA_ID))    // These should always be longer than one packet
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTChannelEventProcess():  Premature end of burst transfer.");
                     #endif
                     bRxError = TRUE;
                  }
                  else
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::ANTChannelEventProcess():  Reception of burst complete. (0)");
                     #endif
                     bReceivedCommand = TRUE;
                  }
               }
               #if defined(DEBUG_FILE)
               else
               {
                  DSIDebug::ThreadWrite("ANTFSClientChannel::ANTChannelEventProcess():  Receiving burst... (0)");
               }
               #endif
            }
            else // Other packets in the burst
            {
               if (aucRxBuf[0] & SEQUENCE_LAST_MESSAGE)
               {
                  #if defined(DEBUG_FILE)
                     char szString[256];
                     SNPRINTF(szString, 256, "ANTFSClientChannel::ANTChannelEventProcess():  Reception of burst complete. (%lu).", ulPacketCount);
                     DSIDebug::ThreadWrite(szString);
                  #endif
                  bReceivedCommand = TRUE;
               }
               else
               {
                  #if defined(DEBUG_FILE)
                     char szString[256];
                     SNPRINTF(szString, 256, "ANTFSClientChannel::ANTChannelEventProcess():  Receiving burst... (%lu).", ulPacketCount);
                     DSIDebug::ThreadWrite(szString);
                  #endif
                  ulPacketCount++;
               }
            }

            // Process burst content
            if(eANTFSState == ANTFS_CLIENT_STATE_BEACONING)
            {
               DecodeLinkCommand(&aucRxBuf[1]);
            }
            else if((eANTFSState >= ANTFS_CLIENT_STATE_CONNECTED) && (eANTFSState < ANTFS_CLIENT_STATE_TRANSPORT))
            {
               DecodeAuthenticateCommand(aucRxBuf[0], &aucRxBuf[1]);
            }
            else if(eANTFSState == ANTFS_CLIENT_STATE_UPLOADING)
            {
               UploadInputData(aucRxBuf[0], &aucRxBuf[1]);
            }
            else if(eANTFSState >= ANTFS_CLIENT_STATE_TRANSPORT)
            {
               DecodeTransportCommand(aucRxBuf[0], &aucRxBuf[1]);
            }

         }   // if(!bRxError)

         if(aucRxBuf[0] & SEQUENCE_LAST_MESSAGE)
         {
            DSIThread_MutexLock(&stMutexCriticalSection);
            bReceivedBurst = TRUE;
            bNewRxEvent = TRUE;
            DSIThread_CondSignal(&stCondRxEvent);
            DSIThread_MutexUnlock(&stMutexCriticalSection);
         }
         break;

      case EVENT_TRANSFER_RX_FAILED:
         DSIThread_MutexLock(&stMutexCriticalSection);
            if(eANTFSRequest != ANTFS_REQUEST_NONE)
            {
               bRxError = TRUE;  // No need to signal an error, as no request is being processed
            }
            bReceivedBurst = FALSE;
            bReceivedCommand = FALSE;
            ucLinkCommandInProgress = ANTFS_CMD_NONE;   // Clear command, to allow the host to retry
            DSIThread_CondSignal(&stCondRxEvent);
         DSIThread_MutexUnlock(&stMutexCriticalSection);

         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::ANTChannelEventProcess():  EVENT_TRANSFER_RX_FAILED");
         #endif
         break;

      case EVENT_TRANSFER_TX_COMPLETED:
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::ANTChannelEventProcess():  EVENT_TRANSFER_TX_COMPLETED");
         #endif
         break;

      case EVENT_TRANSFER_TX_FAILED:
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::ANTChannelEventProcess():  EVENT_TRANSFER_TX_FAILED");
         #endif
         bTxError = TRUE;
         break;

      case EVENT_TRANSFER_TX_START:
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::ANTChannelEventProcess():  EVENT_TRANSFER_TX_START");
         #endif
         break;

      case EVENT_TX:
         LoadBeacon();
         pclANT->SendBroadcastData(ucChannelNumber, aucBeacon);
         #if defined(DEBUG_FILE2)
            UCHAR aucString[256];
            SNPRINTF((char *) aucString, 256, "ANTChannelEventProcess():  Beacon [0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X]",
               aucBeacon[0], aucBeacon[1], aucBeacon[2], aucBeacon[3], aucBeacon[4], aucBeacon[5], aucBeacon[6], aucBeacon[7]);
            DSIDebug::ThreadWrite((char*) aucString);
         #endif
         break;

      case EVENT_CHANNEL_CLOSED:
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::ANTChannelEventProcess():  EVENT_CHANNEL_CLOSED");
         #endif
         break;

      default:
         break;
   }
   return TRUE; // message has been handled, do not pass to application
}


///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::SetDefaultBeacon(void)
{
   stInitParams.ucBeaconFrequency = ANTFS_RF_FREQ;
   stInitParams.ucLinkPeriod = BEACON_PERIOD_8_HZ;
   stInitParams.ulSerialNumber = 0;   // Use the USB device serial number by default
   stInitParams.usBeaconDeviceType = 1;
   stInitParams.usBeaconDeviceManufID = 1;
   stInitParams.bPairingEnabled = TRUE;
   stInitParams.bUploadEnabled = FALSE;
   stInitParams.bDataAvailable = FALSE;
   stInitParams.ucAuthType = AUTH_COMMAND_PAIR;
   stInitParams.ucBeaconTimeout = (UCHAR) (COMMAND_TIMEOUT/1000);   // In seconds
   stInitParams.ucPairingTimeout = (UCHAR) (AUTH_TIMEOUT/1000);   // In seconds
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::ResetClientState(void)
{
   // Clear all state variables, while keeping the configuration
   *pbCancel = FALSE;
   ulPacketCount = 0;
   bTxError = FALSE;
   bRxError = FALSE;
   bReceivedBurst = FALSE;
   bReceivedCommand = FALSE;
   bNewRxEvent = FALSE;

   memset(aucResponseBuf, 0, sizeof(aucResponseBuf));
   memset(aucRxBuf, 0, sizeof(aucRxBuf));

   ucPairingTimeout = MAX_UCHAR;
   bTimeoutEvent = FALSE;
   bReturnToBroadcast = FALSE;

   ulHostSerialNumber = 0;
   stHostFriendlyName.bNameSet = FALSE;
   stHostFriendlyName.ucIndex = 0;
   stHostFriendlyName.ucSize = 0;
   memset(stHostFriendlyName.acFriendlyName, 0, FRIENDLY_NAME_MAX_LENGTH);
   ucPassKeyIndex = 0;
   ucAuthCommandType = MAX_UCHAR;
   bAcceptRequest = FALSE;

   memset(&stHostRequestParams, 0, sizeof(stHostRequestParams));
   ucRequestResponse = MAX_UCHAR;

   usTransferDataFileIndex = 0;
   ulTransferFileSize = 0;
   ulTransferBurstIndex = 0;
   ulTransferBytesRemaining = 0;
   ulTransferMaxIndex = 0;
   ulTransferBlockSize = 0;
   ulTransferBlockOffset = 0;
   usTransferCrc = 0;
   ulTransferBufferSize = 0;
   ulDownloadProgress = 0;
   pucDownloadData = (UCHAR*) NULL;

   if(eANTFSState == ANTFS_CLIENT_STATE_OFF)
   {
      pucTransferBufferDynamic = (UCHAR*) NULL;
   }
   else
   {
      // Deallocate dynamically allocated memory if we had an error during a transfer
      if (pucTransferBufferDynamic)
      {
         delete[] pucTransferBufferDynamic;
         pucTransferBufferDynamic = (UCHAR*)NULL;
      }

      eANTFSState = ANTFS_CLIENT_STATE_IDLE;
   }

   eANTFSRequest = ANTFS_REQUEST_NONE;
   ucLinkCommandInProgress = ANTFS_CMD_NONE;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSClientChannel::ReInitDevice(void)
{
   if (eANTFSState != ANTFS_CLIENT_STATE_OFF)
      this->Close();

   bKillThread = FALSE;

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSClientChannel::ReInitDevice(): Initializing");
   #endif

   if (hANTFSThread == NULL)
   {
      hANTFSThread = DSIThread_CreateThread(&ANTFSClientChannel::ANTFSThreadStart, this);
      if (hANTFSThread == NULL)
         return FALSE;
   }

   if (!bTimerRunning)
   {
      if (DSIThread_MutexInit(&stMutexPairingTimeout) != DSI_THREAD_ENONE)
      {
         return FALSE;
      }

      pclTimer = new DSITimer(&ANTFSClientChannel::TimerStart, this, 1000, TRUE);
      if (pclTimer->NoError() == FALSE)
      {
         DSIThread_MutexDestroy(&stMutexPairingTimeout);
         return FALSE;
      }
      bTimerRunning = TRUE;
   }

   DSIThread_MutexLock(&stMutexResponseQueue);
      clResponseQueue.Clear();                     // Should this be done in ResetClientState instead?
   DSIThread_MutexUnlock(&stMutexResponseQueue);

   DSIThread_MutexLock(&stMutexCriticalSection);
      eANTFSRequest = ANTFS_REQUEST_INIT;
      DSIThread_CondSignal(&stCondRequest);
   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return TRUE;
}


///////////////////////////////////////////////////////////////////////
// Frequency:  1 Hz
///////////////////////////////////////////////////////////////////////
DSI_THREAD_RETURN ANTFSClientChannel::TimerStart(void *pvParameter_)
{
   #if defined(DEBUG_FILE)
      DSIDebug::ThreadInit("ANTFSClient_Timer");
   #endif

   ((ANTFSClientChannel *)pvParameter_)->TimerCallback();

   return 0;
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::TimerCallback(void)
{
   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSClientChannel::TimerCallback():  Entering critical section.");
   #endif

   DSIThread_MutexLock(&stMutexPairingTimeout);

   if(eANTFSState == ANTFS_CLIENT_STATE_PAIRING_WAIT_FOR_RESPONSE)
   {
      if((ucPairingTimeout > 0) && (ucPairingTimeout != CMD_TIMEOUT_DISABLED))
      {
         ucPairingTimeout--;
      }

      if(ucPairingTimeout == 0) // Timeout
      {
         #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("Timeout event.");
         ucPairingTimeout = MAX_UCHAR;
         bTimeoutEvent = TRUE;
         #endif
      }
   }
   DSIThread_MutexUnlock(&stMutexPairingTimeout);

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSClientChannel::TimerCallback():  Left critical section.");
   #endif

   DSIThread_MutexLock(&stMutexCriticalSection);
   if((bTimeoutEvent == TRUE) && (eANTFSState == ANTFS_CLIENT_STATE_PAIRING_WAIT_FOR_RESPONSE))
   {
      // Reject the authentication request
      eANTFSRequest = ANTFS_REQUEST_AUTHENTICATE;
      DSIThread_CondSignal(&stCondRequest);
   }
   DSIThread_MutexUnlock(&stMutexCriticalSection);
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::HandleSerialError(void)
{
   // We ended up here because we did not receive the expected response to a serial message
   // Most likely, ANT was in the wrong state, so attempt to close the channel.
   // No errors raised from here, as we do not know what state we are in.

   UCHAR ucChannelStatus = 0;

   if(pclANT->CloseChannel(ucChannelNumber, ANT_CLOSE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::HandleSerialError():  Failed to close channel.");
      #endif
   }

   if(pclANT->UnAssignChannel(ucChannelNumber, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::HandleSerialError():  Failed to unassign channel.");
      #endif
   }

   if(pclANT->GetChannelStatus(ucChannelNumber, &ucChannelStatus, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::HandleSerialError():  Failed ANT_GetChannelStatus().");
      #endif
   }
   #if defined(DEBUG_FILE)
   else if ((ucChannelStatus & STATUS_CHANNEL_STATE_MASK) != STATUS_UNASSIGNED_CHANNEL)
   {
      char szString[256];
      SNPRINTF(szString, 256, "ANTFSClientChannel::HandleSerialError():  Channel state... 0x%x.", ucChannelStatus);
      DSIDebug::ThreadWrite(szString);
   }
   #endif

   DSIThread_MutexLock(&stMutexCriticalSection);

   eANTFSRequest = ANTFS_REQUEST_SERIAL_ERROR_HANDLED;   // Reset state machine
   DSIThread_CondSignal(&stCondRequest);

   DSIThread_MutexUnlock(&stMutexCriticalSection);
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::AddResponse(ANTFS_CLIENT_RESPONSE eResponse_)
{
   DSIThread_MutexLock(&stMutexResponseQueue);
      clResponseQueue.AddResponse(eResponse_);
      DSIThread_CondSignal(&stCondWaitForResponse);
   DSIThread_MutexUnlock(&stMutexResponseQueue);
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::LoadBeacon(void)
{
   UCHAR ucBeaconStatus2;

   // Status Byte 2
   if (ucLinkCommandInProgress == ANTFS_CMD_NONE)
   {
      if(eANTFSState < ANTFS_CLIENT_STATE_CONNECTED)
      {
          ucBeaconStatus2 = REMOTE_DEVICE_STATE_LINK ;
      }
      else if(eANTFSState < ANTFS_CLIENT_STATE_TRANSPORT)
      {
         ucBeaconStatus2 = REMOTE_DEVICE_STATE_AUTH;
      }
      else
      {
         ucBeaconStatus2 = REMOTE_DEVICE_STATE_TRANS;
      }
   }
   else
   {
      ucBeaconStatus2 = REMOTE_DEVICE_STATE_BUSY;
   }

   aucBeacon[ANTFS_CONNECTION_OFFSET] = ANTFS_BEACON_ID;    // ANT-FS Beacon ID
   aucBeacon[STATUS1_OFFSET] = ucActiveBeaconStatus1;         // Status Byte 1
   aucBeacon[STATUS2_OFFSET] = ucBeaconStatus2;               // Status Byte 2
   aucBeacon[AUTHENTICATION_TYPE_OFFSET] = stInitParams.ucAuthType;   // Authentication Type

   if (eANTFSState >= ANTFS_CLIENT_STATE_CONNECTED) // AUTH & TRANS
   {
      // Host serial number
      Convert_ULONG_To_Bytes(ulHostSerialNumber,
                           &aucBeacon[AUTH_HOST_SERIAL_NUMBER_OFFSET+3],
                           &aucBeacon[AUTH_HOST_SERIAL_NUMBER_OFFSET+2],
                           &aucBeacon[AUTH_HOST_SERIAL_NUMBER_OFFSET+1],
                           &aucBeacon[AUTH_HOST_SERIAL_NUMBER_OFFSET]);
   }
   else
   {
      // Device descriptor
      Convert_USHORT_To_Bytes(stInitParams.usBeaconDeviceType,
                           &aucBeacon[DEVICE_TYPE_OFFSET_HIGH],
                           &aucBeacon[DEVICE_TYPE_OFFSET_LOW]);   // Device type

      Convert_USHORT_To_Bytes(stInitParams.usBeaconDeviceManufID,
                           &aucBeacon[MANUFACTURER_ID_OFFSET_HIGH],
                           &aucBeacon[MANUFACTURER_ID_OFFSET_LOW]);   // Manufacturer
   }
}

///////////////////////////////////////////////////////////////////////
ANTFSClientChannel::RETURN_STATUS ANTFSClientChannel::AttemptOpenBeacon(void)
{
   UCHAR ucChannelStatus;

   if(eANTFSState != ANTFS_CLIENT_STATE_OPENING)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptOpenBeacon():  Device is not in correct state.");
      #endif

      return RETURN_FAIL;
   }

   if(pclANT->GetChannelStatus(ucChannelNumber, &ucChannelStatus, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptOpenBeacon():  Failed ANT_GetChannelStatus().");
      #endif
      #if defined (BLE_DEBUG)
       return RETURN_PASS;
      #else
         return RETURN_SERIAL_ERROR;
      #endif // BLE_DEBUG
   }

   if ((ucChannelStatus & STATUS_CHANNEL_STATE_MASK) == STATUS_TRACKING_CHANNEL)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptOpenBeacon(): ANT-FS Broadcast mode, skipping channel initialization");
      #endif
      return RETURN_PASS;
   }

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptOpenBeacon():  Full Init Begin");
   #endif

   if ((ucChannelStatus & STATUS_CHANNEL_STATE_MASK) == STATUS_ASSIGNED_CHANNEL)
   {
      if(pclANT->UnAssignChannel(ucChannelNumber, MESSAGE_TIMEOUT) == FALSE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptOpenBeacon():  Failed ANT_UnAssignChannel().");
         #endif
         return RETURN_SERIAL_ERROR;
      }
   }

   if (pclANT->SetNetworkKey(ucNetworkNumber, (UCHAR *) aucTheNetworkkey, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptOpenBeacon():  Failed ANT_SetNetworkKey().");
      #endif
      return RETURN_SERIAL_ERROR;
   }

   if (pclANT->AssignChannel(ucChannelNumber, 0x10, ucNetworkNumber, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptOpenBeacon():  Failed ANT_AssignChannel().");
      #endif
      return RETURN_SERIAL_ERROR;
   }

   if (pclANT->SetChannelPeriod(ucChannelNumber, usTheMessagePeriod, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptOpenBeacon():  Failed ANT_SetChannelPeriod().");
      #endif
      return RETURN_SERIAL_ERROR;
   }

   if (pclANT->SetChannelRFFrequency(ucChannelNumber, ucActiveBeaconFrequency, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptOpenBeacon():  Failed ANT_SetChannelRFFreq().");
      #endif
      return RETURN_SERIAL_ERROR;
   }

   if(bCustomTxPower)
   {
      if(pclANT->SetChannelTransmitPower(ucChannelNumber, ucLinkTxPower, MESSAGE_TIMEOUT) == FALSE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptOpenBeacon():  Failed ANT_SetChannelTransmitPower(), setting power level for all channels.");
         #endif

         if(pclANT->SetAllChannelsTransmitPower(ucLinkTxPower, MESSAGE_TIMEOUT) == FALSE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptOpenBeacon():  Failed ANT_SetAllChannelsTransmitPower().");
            #endif

            return RETURN_SERIAL_ERROR;
         }
      }
   }

   if (pclANT->SetChannelID(ucChannelNumber, usRadioChannelID, ucTheDeviceType, ucTheTransmissionType, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptOpenBeacon():  Failed ANT_SetChannelId().");
      #endif
      return RETURN_SERIAL_ERROR;
   }

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptOpenBeacon():  Opening channel...");
   #endif

   if (pclANT->OpenChannel(ucChannelNumber, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptOpenBeacon():  Failed ANT_OpenChannel().");
      #endif
      return RETURN_SERIAL_ERROR;
   }

   return RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFSClientChannel::RETURN_STATUS ANTFSClientChannel::AttemptCloseBeacon(void)
{
   if(bReturnToBroadcast == FALSE)
   {
      if(pclANT->CloseChannel(ucChannelNumber, ANT_CLOSE_TIMEOUT) == FALSE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptCloseBeacon(): Failed to close beacon channel");
         #endif
         return RETURN_SERIAL_ERROR;
      }
   }

   bReturnToBroadcast = FALSE;
   eANTFSState = ANTFS_CLIENT_STATE_IDLE;
   eANTFSRequest = ANTFS_REQUEST_NONE;

   return RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFSClientChannel::RETURN_STATUS ANTFSClientChannel::AttemptAuthenticateResponse(void)
{
   RETURN_STATUS eReturn = RETURN_FAIL;
   UCHAR aucTxAuth[8 + TX_PASSWORD_MAX_LENGTH]; // Response + auth string
   UCHAR ucTxPasswordLength = 0;
   UCHAR ucTxRetries;

   ANTFS_DATA stHeader = {8, aucBeacon};
   ANTFS_DATA stData;
   ANTFS_DATA stFooter = {0, NULL};

   ANTFRAMER_RETURN eTxComplete;

   if((eANTFSState < ANTFS_CLIENT_STATE_CONNECTED) || (eANTFSState >= ANTFS_CLIENT_STATE_TRANSPORT))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptAuthenticateResponse():  Device is not in correct state.");
      #endif

      return RETURN_FAIL;
   }

   memset(aucTxAuth,0x00,sizeof(aucTxAuth));

   LoadBeacon();

   aucTxAuth[ANTFS_CONNECTION_OFFSET] = ANTFS_RESPONSE_ID;
   aucTxAuth[ANTFS_RESPONSE_OFFSET] = ANTFS_RESPONSE_AUTH_ID ;
   if(ucAuthCommandType == AUTH_COMMAND_REQ_SERIAL_NUM)
   {
      aucTxAuth[AUTH_RESPONSE_OFFSET] = AUTH_RESPONSE_NA;
      if(ucFriendlyNameSize != 0)
      {
         ucTxPasswordLength = ucFriendlyNameSize;
         memcpy(&aucTxAuth[8], aucFriendlyName, ucTxPasswordLength);
      }
      eReturn = RETURN_NA;
   }
   else if(bAcceptRequest == TRUE)
   {
      aucTxAuth[AUTH_RESPONSE_OFFSET] = AUTH_RESPONSE_ACCEPT;
      if((ucAuthCommandType == AUTH_COMMAND_PAIR) && (ucPassKeySize != 0))
      {
         ucTxPasswordLength = ucPassKeySize;
         memcpy(&aucTxAuth[8], aucPassKey, ucTxPasswordLength);
      }
      eReturn = RETURN_PASS;
   }
   else
   {
      aucTxAuth[AUTH_RESPONSE_OFFSET] = AUTH_RESPONSE_REJECT;
      eReturn = RETURN_REJECT;
   }

   aucTxAuth[AUTH_FRIENDLY_NAME_LENGTH_OFFSET] = ucTxPasswordLength;

   Convert_ULONG_To_Bytes(stInitParams.ulSerialNumber,
                           &aucTxAuth[AUTH_REMOTE_SERIAL_NUMBER_OFFSET + 3],
                           &aucTxAuth[AUTH_REMOTE_SERIAL_NUMBER_OFFSET + 2],
                           &aucTxAuth[AUTH_REMOTE_SERIAL_NUMBER_OFFSET + 1],
                           &aucTxAuth[AUTH_REMOTE_SERIAL_NUMBER_OFFSET]);

   stData.ulSize = ucTxPasswordLength + 8;
   stData.pucData = aucTxAuth;

   ucTxRetries = 8;
   do{
      eTxComplete = pclANT->SendANTFSClientTransfer(ucChannelNumber, &stHeader, &stData, &stFooter, ACKNOWLEDGED_TIMEOUT, NULL);
      #if defined(DEBUG_FILE)
      if (eTxComplete == ANTFRAMER_FAIL)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptAuthenticateResponse():  Tx error.");
      else if (eTxComplete == ANTFRAMER_TIMEOUT)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptAuthenticateResponse():  Tx timeout.");
      #endif
    } while (eTxComplete == ANTFRAMER_FAIL && --ucTxRetries);

   if (eTxComplete != ANTFRAMER_PASS)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptAuthenticateResponse():  Tx failed.");
      #endif
      return RETURN_FAIL;
   }

   return eReturn;
}

///////////////////////////////////////////////////////////////////////
ANTFSClientChannel::RETURN_STATUS ANTFSClientChannel::AttemptEraseResponse(void)
{
   RETURN_STATUS eReturn = RETURN_FAIL;
   UCHAR aucBuffer[8];

   ANTFS_DATA stHeader = {8, aucBeacon};
   ANTFS_DATA stData = {8, aucBuffer};
   ANTFS_DATA stFooter = {0, NULL};

   ANTFRAMER_RETURN eTxComplete;

   if((eANTFSState < ANTFS_CLIENT_STATE_TRANSPORT))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptEraseResponse():  Device is not in correct state.");
      #endif

      return RETURN_FAIL;
   }

   LoadBeacon();

   memset(aucBuffer,0x00,sizeof(aucBuffer));
   aucBuffer[ANTFS_CONNECTION_OFFSET] = ANTFS_RESPONSE_ID;
   aucBuffer[ANTFS_RESPONSE_OFFSET] = ANTFS_RESPONSE_ERASE_ID;
   aucBuffer[ERASE_RESPONSE_OFFSET] = ucRequestResponse;

   if(ucRequestResponse == ERASE_RESPONSE_OK)
   {
      eReturn = RETURN_PASS;
   }
   else
   {
      eReturn = RETURN_REJECT;
   }

   eTxComplete = pclANT->SendANTFSClientTransfer(ucChannelNumber, &stHeader, &stData, &stFooter, ACKNOWLEDGED_TIMEOUT, NULL);

   if (eTxComplete != ANTFRAMER_PASS)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptEraseResponse():  Tx failed.");
         if (eTxComplete == ANTFRAMER_FAIL)
            DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptEraseResponse():  Tx error.");
         else if (eTxComplete == ANTFRAMER_TIMEOUT)
            DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptEraseResponse():  Tx timeout.");
      #endif
      return RETURN_FAIL;
   }

   return eReturn;
}

///////////////////////////////////////////////////////////////////////
ANTFSClientChannel::RETURN_STATUS ANTFSClientChannel::AttemptDownloadResponse()
{
   RETURN_STATUS eReturn = RETURN_FAIL;
   UCHAR aucDownloadHeader[24];   // Beacon + Response
   UCHAR aucDownloadFooter[8];   // CRC footer
   ANTFS_DATA stHeader = {24, aucDownloadHeader};
   ANTFS_DATA stFooter = {8, aucDownloadFooter};
   ANTFS_DATA stData;

   USHORT usSavedCrc = 0;
   ULONG ulSavedOffset = 0;

   ANTFRAMER_RETURN eTxComplete;
   UCHAR ucNoRxTicks;
   BOOL bDone = FALSE;
   BOOL bStatus = FALSE;

   if((eANTFSState < ANTFS_CLIENT_STATE_TRANSPORT))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptDownloadResponse():  Device is not in correct state.");
      #endif

      return RETURN_FAIL;
   }

   bReceivedCommand = FALSE;
   bReceivedBurst = FALSE;
   bRxError = FALSE;

   do
   {
      if(ucRequestResponse == DOWNLOAD_RESPONSE_OK)
      {
         // If this is not an initial request, verify the CRC
         if(stHostRequestParams.bInitialRequest == FALSE && stHostRequestParams.usCRCSeed != 0)
         {
            if(ulTransferBurstIndex < ulSavedOffset)
            {
               usTransferCrc = CRC_Calc16(pucDownloadData, ulTransferBurstIndex);
            }
            else
            {
               usTransferCrc = CRC_UpdateCRC16(usSavedCrc, &pucDownloadData[ulSavedOffset], ulTransferBurstIndex - ulSavedOffset);
            }

            if(usTransferCrc != stHostRequestParams.usCRCSeed)
            {
               #if defined(DEBUG_FILE)
                  UCHAR aucString[256];
                  SNPRINTF((char*) aucString, 256, "ANTFSClientChannel::AttemptDownloadResponse(): CRC Check Failed - Expected 0x%04X, Got 0x%04X", usTransferCrc, stHostRequestParams.usCRCSeed);
                  DSIDebug::ThreadWrite((char*) aucString);
               #endif
                  ucRequestResponse = DOWNLOAD_RESPONSE_CRC_FAILED;
            }
         }
         else   // Use seed provided by host
         {
            usTransferCrc = stHostRequestParams.usCRCSeed;
         }
      }

      if ((!bReceivedBurst) &&(!bReceivedCommand))  //prevents us from sending requests until the Rx bursts have stopped and been cleared.
      {
         bRxError = FALSE;

         // Send out the download response
         LoadBeacon();
         memset(aucDownloadHeader, 0, sizeof(aucDownloadHeader));
         memcpy(aucDownloadHeader, aucBeacon, 8);
         aucDownloadHeader[ANTFS_CONNECTION_OFFSET + 8] = ANTFS_RESPONSE_ID;
         aucDownloadHeader[ANTFS_RESPONSE_OFFSET + 8] = ANTFS_RESPONSE_DOWNLOAD_ID;
         aucDownloadHeader[DOWNLOAD_RESPONSE_OFFSET + 8] = ucRequestResponse;
         Convert_ULONG_To_Bytes(ulTransferBytesRemaining,
                                 &aucDownloadHeader[DOWNLOAD_RESPONSE_BLOCK_SIZE_OFFSET + 3 + 8],
                                 &aucDownloadHeader[DOWNLOAD_RESPONSE_BLOCK_SIZE_OFFSET + 2 + 8],
                                 &aucDownloadHeader[DOWNLOAD_RESPONSE_BLOCK_SIZE_OFFSET + 1 + 8],
                                 &aucDownloadHeader[DOWNLOAD_RESPONSE_BLOCK_SIZE_OFFSET + 8]);
         Convert_ULONG_To_Bytes(ulTransferBurstIndex,
                                 &aucDownloadHeader[DOWNLOAD_RESPONSE_DATA_OFFSET_OFFSET + 3 + 16],
                                 &aucDownloadHeader[DOWNLOAD_RESPONSE_DATA_OFFSET_OFFSET + 2 + 16],
                                 &aucDownloadHeader[DOWNLOAD_RESPONSE_DATA_OFFSET_OFFSET + 1 + 16],
                                 &aucDownloadHeader[DOWNLOAD_RESPONSE_DATA_OFFSET_OFFSET + 16]);
         Convert_ULONG_To_Bytes(ulTransferFileSize,
                                 &aucDownloadHeader[DOWNLOAD_RESPONSE_FILE_SIZE_OFFSET + 3 + 16],
                                 &aucDownloadHeader[DOWNLOAD_RESPONSE_FILE_SIZE_OFFSET + 2 + 16],
                                 &aucDownloadHeader[DOWNLOAD_RESPONSE_FILE_SIZE_OFFSET + 1 + 16],
                                 &aucDownloadHeader[DOWNLOAD_RESPONSE_FILE_SIZE_OFFSET + 16]);

         if(ucRequestResponse == DOWNLOAD_RESPONSE_OK)
         {
            ULONG ulDownloadTimeout;

            if(ulTransferBytesRemaining > 0)
            {
               usSavedCrc = usTransferCrc;
               ulSavedOffset = ulTransferBurstIndex;
               usTransferCrc = CRC_UpdateCRC16(usTransferCrc, &pucDownloadData[ulTransferBurstIndex], ulTransferBytesRemaining);
            }

            memset(aucDownloadFooter, 0, sizeof(aucDownloadFooter));
            Convert_USHORT_To_Bytes(usTransferCrc,
                                 &aucDownloadFooter[DOWNLOAD_RESPONSE_CRC_OFFSET + 1],
                                 &aucDownloadFooter[DOWNLOAD_RESPONSE_CRC_OFFSET]);

            stData.pucData = &pucDownloadData[ulTransferBurstIndex];
            stData.ulSize = ulTransferBytesRemaining;

            // Figure out our timeout value from the size of the transfer
            ulDownloadTimeout = BROADCAST_TIMEOUT + (ulTransferBytesRemaining * 2);
            if(ulTransferBytesRemaining > ((MAX_ULONG / BROADCAST_TIMEOUT)/2))
               ulDownloadTimeout = MAX_ULONG - 1;

            eTxComplete = pclANT->SendANTFSClientTransfer(ucChannelNumber, &stHeader, &stFooter, &stData, ulDownloadTimeout, &ulDownloadProgress);
            eReturn = RETURN_PASS;
         }
         else
         {
            eTxComplete = pclANT->SendTransfer(ucChannelNumber, aucDownloadHeader, 24, ACKNOWLEDGED_TIMEOUT);
            eReturn = RETURN_REJECT;
         }

         if (eTxComplete == ANTFRAMER_PASS)
       {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptDownloadResponse():  EXIT PASSED.");
         #endif
            return eReturn;   // Response transmitted successfully, we are done
       }

         #if defined(DEBUG_FILE)
            if (eTxComplete == ANTFRAMER_FAIL)
               DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptDownloadResponse():  Tx error sending download response.");
            else if (eTxComplete == ANTFRAMER_TIMEOUT)
               DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptDownloadResponse():  Tx timeout sending download response.");
            DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptDownloadResponse():  Waiting for host to attempt retry...");
         #endif
      }

      // Do not need to clear bReceivedBurst here because it will be done if the transfer fails or we timeout
      // Now we wait for a retry from the host
      ucLinkCommandInProgress = ANTFS_CMD_NONE;
      LoadBeacon();   // Reload beacon, to let host know we are ready for retry
      pclANT->SendBroadcastData(ucChannelNumber, aucBeacon);
      ucNoRxTicks = 5;
      bStatus = FALSE;
      while (bStatus == FALSE)
      {
         //Wait for an rxEvent before starting to check the data
         //Since this event is fired for many circumstances we manage all the error checking below and
         //just use this for the wait functionality.
         DSIThread_MutexLock(&stMutexCriticalSection);
         bNewRxEvent = FALSE;
         if ((bNewRxEvent == FALSE) && (*pbCancel == FALSE))
         {
            DSIThread_CondTimedWait(&stCondRxEvent, &stMutexCriticalSection, MESSAGE_TIMEOUT);
         }
         DSIThread_MutexUnlock(&stMutexCriticalSection);
         if (*pbCancel == TRUE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptDownloadResponse():  Stopped.");
            #endif
            return RETURN_STOP;
         }

         if (!bReceivedBurst)
         {
            #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("-->Waiting for download request...");
            #endif
            ucNoRxTicks--;
            if(ucNoRxTicks == 0)
            {
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptDownloadResponse():  Timeout while waiting for host request retry");
               #endif
               return RETURN_FAIL;   // Timeout
            }
         }

         if (bReceivedCommand)        //If a command has been received, process it.
         {
            bReceivedCommand = FALSE;  //Clear these for any potential retries
            bReceivedBurst = FALSE;     //Clearing these for retries

            // Process request
            if(ucLinkCommandInProgress == ANTFS_DOWNLOAD_ID)
            {
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptDownloadResponse(): Resuming download...");
               #endif
               bStatus = TRUE;
               if(stHostRequestParams.usFileIndex != usTransferDataFileIndex)
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptDownloadResponse(): Invalid index requested.");
                  #endif
                  ucRequestResponse = DOWNLOAD_RESPONSE_REQUEST_INVALID;
               }
               if(ucRequestResponse == DOWNLOAD_RESPONSE_OK)
               {
                  DSIThread_MutexLock(&stMutexCriticalSection);
                  if(stHostRequestParams.ulOffset > ulTransferFileSize)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptDownloadResponse(): Invalid offset requested");
                     #endif
                     ucRequestResponse = DOWNLOAD_RESPONSE_REQUEST_INVALID;
                  }
                  else
                  {
                     // Adjust response parameters
                     ulTransferBurstIndex = stHostRequestParams.ulOffset;
                     ulTransferBytesRemaining = ulTransferFileSize - ulTransferBurstIndex;
                     ulDownloadProgress = 0;
                     if((stHostRequestParams.ulBlockSize != 0) && (ulTransferBytesRemaining > stHostRequestParams.ulBlockSize))
                     {   // Host is limiting block size
                        ulTransferBytesRemaining = stHostRequestParams.ulBlockSize;
                     }
                     if((ulTransferBlockSize != 0) && (ulTransferBytesRemaining > ulTransferBlockSize))
                     {   // Client is limiting block size
                        ulTransferBytesRemaining = ulTransferBlockSize;
                     }
                  }
                  DSIThread_MutexUnlock(&stMutexCriticalSection);
               }   // ucRequestResponse == DOWNLOAD_RESPONSE_OK
            }   // ucLinkCommandInProgress == ANTFS_DOWNLOAD_ID
         } // bReceivedCommand

         if(bRxError)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptDownloadResponse():  Rx error");
            #endif
            bRxError = FALSE;
         }
      } // Rx command loop

   } while (!bDone);
   return eReturn;
}

///////////////////////////////////////////////////////////////////////
ANTFSClientChannel::RETURN_STATUS ANTFSClientChannel::AttemptUploadResponse()
{
   RETURN_STATUS eReturn = RETURN_FAIL;
   UCHAR aucBuffer[24];

   ANTFS_DATA stHeader = {sizeof(aucBeacon), aucBeacon};
   ANTFS_DATA stData = {sizeof(aucBuffer), aucBuffer};
   ANTFS_DATA stFooter = {0, NULL};

   ANTFRAMER_RETURN eTxComplete;

   if((eANTFSState < ANTFS_CLIENT_STATE_TRANSPORT))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptUploadResponse():  Device is not in correct state.");
      #endif
      return RETURN_FAIL;
   }

   if(ucRequestResponse == UPLOAD_RESPONSE_OK)
   {
      // TODO: Uploads not supported yet, so for now, always reject
      //ucRequestResponse = UPLOAD_RESPONSE_REQUEST_INVALID;
     eReturn = RETURN_PASS;
   }

   //Sleep(500);
   //ulTransferBlockOffset = 0;
   //ulTransferMaxIndex = 0;
   //ulTransferBlockSize = 0;
   //usTransferCrc = 0;
   //eReturn = RETURN_REJECT;
   LoadBeacon();
   memset(aucBuffer, 0, sizeof(aucBuffer));
   aucBuffer[ANTFS_CONNECTION_OFFSET] = ANTFS_RESPONSE_ID;
   aucBuffer[ANTFS_RESPONSE_OFFSET] = ANTFS_RESPONSE_UPLOAD_ID;
   aucBuffer[UPLOAD_RESPONSE_OFFSET] = ucRequestResponse;
   Convert_ULONG_To_Bytes(ulTransferBlockOffset,
                           &aucBuffer[UPLOAD_RESPONSE_LAST_OFFSET_OFFSET + 3],
                           &aucBuffer[UPLOAD_RESPONSE_LAST_OFFSET_OFFSET + 2],
                           &aucBuffer[UPLOAD_RESPONSE_LAST_OFFSET_OFFSET + 1],
                           &aucBuffer[UPLOAD_RESPONSE_LAST_OFFSET_OFFSET]);
   Convert_ULONG_To_Bytes(ulTransferMaxIndex,
                           &aucBuffer[UPLOAD_RESPONSE_MAX_SIZE_OFFSET + 3],
                           &aucBuffer[UPLOAD_RESPONSE_MAX_SIZE_OFFSET + 2],
                           &aucBuffer[UPLOAD_RESPONSE_MAX_SIZE_OFFSET + 1],
                           &aucBuffer[UPLOAD_RESPONSE_MAX_SIZE_OFFSET]);
   Convert_ULONG_To_Bytes(ulTransferBlockSize,
                           &aucBuffer[UPLOAD_RESPONSE_BLOCK_SIZE_OFFSET + 3],
                           &aucBuffer[UPLOAD_RESPONSE_BLOCK_SIZE_OFFSET + 2],
                           &aucBuffer[UPLOAD_RESPONSE_BLOCK_SIZE_OFFSET + 1],
                           &aucBuffer[UPLOAD_RESPONSE_BLOCK_SIZE_OFFSET]);
   Convert_USHORT_To_Bytes(usTransferCrc,
                           &aucBuffer[UPLOAD_RESPONSE_CRC_OFFSET + 1],
                           &aucBuffer[UPLOAD_RESPONSE_CRC_OFFSET]);
        #if defined(DEBUG_FILE)
         char cBuffer[256];
      SNPRINTF(cBuffer, 256,"ANTFSClientChannel::AttemptUploadResponse():  Transfercrc is: %d.",usTransferCrc);
      DSIDebug::ThreadWrite(cBuffer);
      #endif
     #if defined(DEBUG_FILE)
       char c2Buffer[256];
       SNPRINTF(c2Buffer, 256, "ANTFSClientChannel::AttemptUploadResponse():  ulTransferBlockOffset is: %d.",ulTransferBlockOffset);
       DSIDebug::ThreadWrite(c2Buffer);
      #endif

   eTxComplete = pclANT->SendANTFSClientTransfer(ucChannelNumber, &stHeader, &stFooter, &stData, MESSAGE_TIMEOUT, NULL);

   if (eTxComplete != ANTFRAMER_PASS)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptUploadResponse():  Tx failed.");
         if (eTxComplete == ANTFRAMER_FAIL)
            DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptUploadResponse():  Tx error.");
         else if (eTxComplete == ANTFRAMER_TIMEOUT)
            DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptUploadResponse():  Tx timeout.");
      #endif
      return RETURN_FAIL;
   }

   return eReturn;
}


///////////////////////////////////////////////////////////////////////
ANTFSClientChannel::RETURN_STATUS ANTFSClientChannel::AttemptUploadDataResponse()
{
     #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptUploadDataResponse():  ENTER. RALPH");
      #endif
   RETURN_STATUS eReturn = RETURN_FAIL;
        #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptUploadDataResponse():  ereturn FAIL. RALPH");
      #endif
   UCHAR aucBuffer[8];

   ANTFS_DATA stHeader = {sizeof(aucBeacon), aucBeacon};
   ANTFS_DATA stData = {sizeof(aucBuffer), aucBuffer};
   ANTFS_DATA stFooter = {0, NULL};

   ANTFRAMER_RETURN eTxComplete;

   if((eANTFSState < ANTFS_CLIENT_STATE_TRANSPORT))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptUploadDataResponse():  Device is not in correct state.");
      #endif
      return RETURN_FAIL;
   }

   if(ucRequestResponse == UPLOAD_RESPONSE_OK)
   {
      // TODO: Uploads not supported yet, so for now, always reject
      //ucRequestResponse = UPLOAD_RESPONSE_REQUEST_INVALID;
     eReturn = RETURN_PASS;
     #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptUploadDataResponse():  ereturn PASS. RALPH");
      #endif
   }

   //Sleep(500);
   //ulTransferBlockOffset = 0;
   //ulTransferMaxIndex = 0;
   //ulTransferBlockSize = 0;
   //usTransferCrc = 0;
   //eReturn = RETURN_REJECT;
   LoadBeacon();
   memset(aucBuffer, 0, sizeof(aucBuffer));
   aucBuffer[ANTFS_CONNECTION_OFFSET] = ANTFS_RESPONSE_ID;
   aucBuffer[ANTFS_RESPONSE_OFFSET] = ANTFS_RESPONSE_UPLOAD_COMPLETE_ID;
   aucBuffer[UPLOAD_RESPONSE_OFFSET] = ucRequestResponse;

   eTxComplete = pclANT->SendANTFSClientTransfer(ucChannelNumber, &stHeader, &stFooter, &stData, MESSAGE_TIMEOUT, NULL);

   if (eTxComplete != ANTFRAMER_PASS)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptUploadCompleteResponse():  Tx failed.");
         if (eTxComplete == ANTFRAMER_FAIL)
            DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptUploadCompleteResponse():  Tx error.");
         else if (eTxComplete == ANTFRAMER_TIMEOUT)
            DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptUploadCompleteResponse():  Tx timeout.");
      #endif
      return RETURN_FAIL;
   }
        #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::AttemptUploadDataResponse():  EXIT. RALPH");
      #endif
   return eReturn;
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::DecodeLinkCommand(UCHAR *pucLinkCommand_)
{
   UCHAR ucPeriod;

   if(pucLinkCommand_[ANTFS_CONNECTION_OFFSET] != ANTFS_COMMAND_ID)
      return;

   switch (pucLinkCommand_[ANTFS_COMMAND_OFFSET])
   {
      case ANTFS_CONNECT_ID:
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeLinkCommand(): Received LINK request.");
         #endif

         ucActiveBeaconFrequency = pucLinkCommand_[TRANSPORT_CHANNEL_FREQ_OFFSET];
         ucPeriod= pucLinkCommand_[TRANSPORT_CHANNEL_PERIOD];
         SetANTChannelPeriod(ucPeriod);
         ulHostSerialNumber = Convert_Bytes_To_ULONG(
            pucLinkCommand_[HOST_ID_OFFSET+3],
            pucLinkCommand_[HOST_ID_OFFSET+2],
            pucLinkCommand_[HOST_ID_OFFSET+1],
            pucLinkCommand_[HOST_ID_OFFSET]);

         DSIThread_MutexLock(&stMutexCriticalSection);
            eANTFSRequest = ANTFS_REQUEST_CONNECT;
            DSIThread_CondSignal(&stCondRequest);
         DSIThread_MutexUnlock(&stMutexCriticalSection);
         break;
      }   // CONNECT
      case ANTFS_DISCONNECT_ID:
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeLinkCommand(): Received DISCONNECT request");
         #endif

         stHostDisconnectParams.ucCommandType = pucLinkCommand_[DISCONNECT_COMMAND_TYPE_OFFSET];
         stHostDisconnectParams.ucTimeDuration = pucLinkCommand_[DISCONNECT_TIME_DURATION_OFFSET];
         stHostDisconnectParams.ucAppSpecificDuration = pucLinkCommand_[DISCONNECT_APP_DURATION_OFFSET];

         DSIThread_MutexLock(&stMutexCriticalSection);
            eANTFSRequest = ANTFS_REQUEST_DISCONNECT;
            DSIThread_CondSignal(&stCondRequest);
         DSIThread_MutexUnlock(&stMutexCriticalSection);
         break;
      }  // DISCONNECT
      default:
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeLinkCommand(): Invalid command.");
         #endif
         break;
      }
   }
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::DecodeAuthenticateCommand(UCHAR ucControlByte_, UCHAR *pucAuthCommand_)
{
   if (((ucControlByte_ & ~SEQUENCE_LAST_MESSAGE) == 0) && (ucLinkCommandInProgress != ANTFS_CMD_NONE))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeAuthenticateCommand(): Received new request, but client is busy processing something already.");
      #endif
      return;
   }

   if(pucAuthCommand_[ANTFS_CONNECTION_OFFSET] == ANTFS_COMMAND_ID)
   {
      if(pucAuthCommand_[ANTFS_COMMAND_OFFSET] == ANTFS_AUTHENTICATE_ID)
      {
         ucLinkCommandInProgress = ANTFS_AUTHENTICATE_ID;
         ucAuthCommandType = pucAuthCommand_[AUTH_COMMAND_TYPE_OFFSET];
      }
   }

   if(ucLinkCommandInProgress == ANTFS_AUTHENTICATE_ID)
   {
      if((ucControlByte_ & SEQUENCE_NUMBER_ROLLOVER) == 0) // first packet
      {
         ULONG ulRxHostSerialNumber = Convert_Bytes_To_ULONG(
               pucAuthCommand_[HOST_ID_OFFSET+3],
               pucAuthCommand_[HOST_ID_OFFSET+2],
               pucAuthCommand_[HOST_ID_OFFSET+1],
               pucAuthCommand_[HOST_ID_OFFSET]);
         bAcceptRequest = TRUE;
         if(ulRxHostSerialNumber != ulHostSerialNumber)   // Check host serial number
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeAuthenticateCommand(): Not the expected host");
            #endif
            bAcceptRequest = FALSE;
         }
      }

      switch(ucAuthCommandType)
      {
         case AUTH_COMMAND_REQ_SERIAL_NUM:
         {
            if(ucControlByte_ & SEQUENCE_LAST_MESSAGE) // wait until the burst completes
            {
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeAuthenticateCommand(): Received AUTH serial number request.");
               #endif
               DSIThread_MutexLock(&stMutexCriticalSection);
                  eANTFSRequest = ANTFS_REQUEST_AUTHENTICATE;
                  DSIThread_CondSignal(&stCondRequest);
               DSIThread_MutexUnlock(&stMutexCriticalSection);
            }
            break;
         }   // AUTH_COMMAND_REQ_SERIAL_NUM
         case AUTH_COMMAND_GOTO_TRANSPORT:
         {
            if(ucControlByte_ & SEQUENCE_LAST_MESSAGE) // wait until the burst completes
            {
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeAuthenticateCommand(): Received AUTH pass through request.");
               #endif
               if(stInitParams.ucAuthType != AUTH_COMMAND_GOTO_TRANSPORT)
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeAuthenticateCommand(): Pass through authentication not supported.");
                  #endif
                  bAcceptRequest = FALSE;
               }
               DSIThread_MutexLock(&stMutexCriticalSection);
                  eANTFSRequest = ANTFS_REQUEST_AUTHENTICATE;
                  DSIThread_CondSignal(&stCondRequest);
               DSIThread_MutexUnlock(&stMutexCriticalSection);
            }
            break;
         }   // AUTH_COMMAND_GOTO_TRANSPORT
         case AUTH_COMMAND_PAIR:
         {
            if((ucControlByte_ & SEQUENCE_NUMBER_ROLLOVER) == 0) // first packet
            {
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeAuthenticateCommand(): Received AUTH pairing request.");
               #endif
               stHostFriendlyName.bNameSet = FALSE;
               stHostFriendlyName.ucIndex = 0;
               stHostFriendlyName.ucSize = pucAuthCommand_[AUTH_FRIENDLY_NAME_LENGTH_OFFSET];
               if(stHostFriendlyName.ucSize > FRIENDLY_NAME_MAX_LENGTH)
                  stHostFriendlyName.ucSize = FRIENDLY_NAME_MAX_LENGTH;
               memset(stHostFriendlyName.acFriendlyName, 0, FRIENDLY_NAME_MAX_LENGTH);
            }
            else   // read host friendly name
            {
               if(stHostFriendlyName.ucIndex < FRIENDLY_NAME_MAX_LENGTH)
               {
                  UCHAR ucNumBytes = FRIENDLY_NAME_MAX_LENGTH - stHostFriendlyName.ucIndex;
                  if(ucNumBytes > 8)
                  {
                     ucNumBytes = 8;
                  }
                  memcpy((UCHAR*) &stHostFriendlyName.acFriendlyName[stHostFriendlyName.ucIndex], pucAuthCommand_, ucNumBytes);
                  stHostFriendlyName.ucIndex += ucNumBytes;
               }
            }
            if(ucControlByte_ & SEQUENCE_LAST_MESSAGE)   // last packet
            {
               ENUM_ANTFS_REQUEST eTheRequest;
               if(stInitParams.bPairingEnabled && (stInitParams.ucAuthType <= AUTH_COMMAND_PASSKEY) && bAcceptRequest)
               {
                  if(stHostFriendlyName.ucSize > 0)
                  {
                     stHostFriendlyName.bNameSet = TRUE;
                  }
                  eTheRequest = ANTFS_REQUEST_PAIR;
               }
               else
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeAuthenticateCommand(): Pairing authentication not supported.");
                  #endif
                  bAcceptRequest = FALSE;
                  eTheRequest = ANTFS_REQUEST_AUTHENTICATE;
               }
               DSIThread_MutexLock(&stMutexCriticalSection);
                  eANTFSRequest = eTheRequest;
               DSIThread_CondSignal(&stCondRequest);
               DSIThread_MutexUnlock(&stMutexCriticalSection);
            }
            break;
         }   // AUTH_COMMAND_PAIR
         case AUTH_COMMAND_PASSKEY:
         {
            if ((ucControlByte_ & SEQUENCE_NUMBER_ROLLOVER) == 0) // initial packet
            {
               UCHAR ucPasswordSize = pucAuthCommand_[AUTH_PASSWORD_LENGTH_OFFSET];   // Passkey length

               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeAuthenticateCommand(): Received AUTH passkey request.");
               #endif

               if(ucPasswordSize != ucPassKeySize)
               {
                  bAcceptRequest = FALSE;   // Reject if lengths do not match
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeAuthenticateCommand(): Incorrect string size");
                  #endif
               }
               else
               {
                  ucPassKeyIndex = 0;
               }
            }
            else
            {
               UCHAR ucCounter;

               for (ucCounter = 0; ucCounter < 8; ucCounter++)
               {
                  if (ucPassKeyIndex >= ucPassKeySize)
                     break;
                  if (aucPassKey[ucPassKeyIndex++] != pucAuthCommand_[ucCounter])
                  {
                     bAcceptRequest = FALSE;  // Reject if passkeys are different
                  }
               }
            }
            if (ucControlByte_ & SEQUENCE_LAST_MESSAGE)   // last packet
            {
               if((stInitParams.ucAuthType != AUTH_COMMAND_PASSKEY) && (stInitParams.ucAuthType != AUTH_COMMAND_GOTO_TRANSPORT) && (ucPassKeySize == 0))
               {
                  bAcceptRequest = FALSE;
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeAuthenticateCommand(): Passkey authentication not supported.");
                  #endif
               }
               if (ucPassKeyIndex < ucPassKeySize)
               {
                  bAcceptRequest = FALSE;           // Reject if we did not get the complete passkey
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeAuthenticateCommand(): Incomplete passkey.");
                  #endif
               }
               DSIThread_MutexLock(&stMutexCriticalSection);
                  eANTFSRequest = ANTFS_REQUEST_AUTHENTICATE;
               DSIThread_CondSignal(&stCondRequest);
               DSIThread_MutexUnlock(&stMutexCriticalSection);
            }
            break;
         }   // AUTH_COMMAND_PASSKEY
         default:
         {
            if (ucControlByte_ & SEQUENCE_LAST_MESSAGE)   // last packet
               {
                  #if defined(DEBUG_FILE)
                     UCHAR aucString[256];
                     SNPRINTF((char*) aucString, 256, "Received unknown AUTH request: %u", ucAuthCommandType);
                     DSIDebug::ThreadWrite((char*) aucString);
                  #endif
                  DSIThread_MutexLock(&stMutexCriticalSection);
                     eANTFSRequest = ANTFS_REQUEST_AUTHENTICATE;
                     bAcceptRequest = FALSE;   // Reject unknown auth requests
                  DSIThread_CondSignal(&stCondRequest);
                  DSIThread_MutexUnlock(&stMutexCriticalSection);
            }
            break;
         }
      }
   }   // ANTFS_AUTHENTICATE_ID
   else if (pucAuthCommand_[ANTFS_COMMAND_OFFSET] == ANTFS_DISCONNECT_ID)
   {
      if (ucControlByte_ & SEQUENCE_LAST_MESSAGE) // don't do anything until the burst completes
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeAuthenticateCommand(): Received DISCONNECT request");
         #endif

         stHostDisconnectParams.ucCommandType = pucAuthCommand_[DISCONNECT_COMMAND_TYPE_OFFSET];
         stHostDisconnectParams.ucTimeDuration = pucAuthCommand_[DISCONNECT_TIME_DURATION_OFFSET];
         stHostDisconnectParams.ucAppSpecificDuration = pucAuthCommand_[DISCONNECT_APP_DURATION_OFFSET];

         DSIThread_MutexLock(&stMutexCriticalSection);
            eANTFSRequest = ANTFS_REQUEST_DISCONNECT;
            DSIThread_CondSignal(&stCondRequest);
         DSIThread_MutexUnlock(&stMutexCriticalSection);
      }
   }   // ANTFS_DISCONNECT_ID
   else if (pucAuthCommand_[ANTFS_COMMAND_OFFSET] == ANTFS_PING_ID)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeAuthenticateCommand(): Ping!");
      #endif

      DSIThread_MutexLock(&stMutexCriticalSection);
         ucLinkCommandInProgress = ANTFS_CMD_NONE;
         eANTFSRequest = ANTFS_REQUEST_PING;
         DSIThread_CondSignal(&stCondRequest);
      DSIThread_MutexUnlock(&stMutexCriticalSection);
   }   // ANTFS_PING_ID
   else
   {
      #if defined(DEBUG_FILE)
         UCHAR aucString[256];
         SNPRINTF((char *) aucString, 256, "ANTFSClientChannel::DecodeAuthenticateCommand(): Received invalid request: %u", pucAuthCommand_[ANTFS_COMMAND_OFFSET]);
         DSIDebug::ThreadWrite((char*) aucString);
      #endif
      ucLinkCommandInProgress = ANTFS_CMD_NONE;
   }   // OTHER REQUESTS
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::DecodeTransportCommand(UCHAR ucControlByte_, UCHAR *pucTransCommand_)
{
   if (((ucControlByte_ & ~SEQUENCE_LAST_MESSAGE) == 0) && (ucLinkCommandInProgress != ANTFS_CMD_NONE))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeTransportCommand(): Received new request, but client is busy.");
      #endif
      return;
   }

   if(pucTransCommand_[ANTFS_CONNECTION_OFFSET] == ANTFS_COMMAND_ID)
   {
      ucLinkCommandInProgress = pucTransCommand_[ANTFS_COMMAND_OFFSET];
   }

   switch(ucLinkCommandInProgress)
   {
      case ANTFS_PING_ID:
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeTransportCommand(): Ping!");
            #endif
            DSIThread_MutexLock(&stMutexCriticalSection);
               ucLinkCommandInProgress = ANTFS_CMD_NONE;
               eANTFSRequest = ANTFS_REQUEST_PING;
               DSIThread_CondSignal(&stCondRequest);
            DSIThread_MutexUnlock(&stMutexCriticalSection);
         }   // ANTFS_PING_ID
         break;

      case ANTFS_DISCONNECT_ID:
         {
            if (ucControlByte_ & SEQUENCE_LAST_MESSAGE) // don't do anything until the burst completes
            {
               #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeTransportCommand(): Received DISCONNECT request");
               #endif

               stHostDisconnectParams.ucCommandType = pucTransCommand_[DISCONNECT_COMMAND_TYPE_OFFSET];
               stHostDisconnectParams.ucTimeDuration = pucTransCommand_[DISCONNECT_TIME_DURATION_OFFSET];
               stHostDisconnectParams.ucAppSpecificDuration = pucTransCommand_[DISCONNECT_APP_DURATION_OFFSET];

               DSIThread_MutexLock(&stMutexCriticalSection);
                  eANTFSRequest = ANTFS_REQUEST_DISCONNECT;
                  DSIThread_CondSignal(&stCondRequest);
               DSIThread_MutexUnlock(&stMutexCriticalSection);
            }
         }  // ANTFS_DISCONNECT_ID
         break;

      case ANTFS_LINK_ID:
         {
            ULONG ulRxHostSerialNumber;

            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeTransportCommand(): Received LINK request.");
            #endif

            ulRxHostSerialNumber = Convert_Bytes_To_ULONG(
               pucTransCommand_[HOST_ID_OFFSET+3],
               pucTransCommand_[HOST_ID_OFFSET+2],
               pucTransCommand_[HOST_ID_OFFSET+1],
               pucTransCommand_[HOST_ID_OFFSET]);

            if(ulHostSerialNumber == ulRxHostSerialNumber)
            {
               UCHAR ucPeriod= pucTransCommand_[TRANSPORT_CHANNEL_PERIOD];
               ucActiveBeaconFrequency = pucTransCommand_[TRANSPORT_CHANNEL_FREQ_OFFSET];
               SetANTChannelPeriod(ucPeriod);

               DSIThread_MutexLock(&stMutexCriticalSection);
                  eANTFSRequest = ANTFS_REQUEST_CHANGE_LINK;
                  DSIThread_CondSignal(&stCondRequest);
               DSIThread_MutexUnlock(&stMutexCriticalSection);
            }
            ucLinkCommandInProgress = ANTFS_CMD_NONE;
         }   // ANTFS_LINK_ID
         break;

      case ANTFS_ERASE_ID:
         {
            if(ucControlByte_ & SEQUENCE_LAST_MESSAGE) // don't do anything unless the burst completes
            {
               ucRequestResponse = ERASE_RESPONSE_REJECT;   // set initial request parameters

               stHostRequestParams.usFileIndex = Convert_Bytes_To_USHORT(
                  pucTransCommand_[DATA_INDEX_OFFSET + 1],
                  pucTransCommand_[DATA_INDEX_OFFSET]);
               stHostRequestParams.bInitialRequest = FALSE;
               stHostRequestParams.ulBlockSize = 0;
               stHostRequestParams.ulOffset = 0;
               stHostRequestParams.usCRCSeed = 0;

               #if defined(DEBUG_FILE)
               {
                  UCHAR aucString[256];
                  SNPRINTF((char*) aucString, 256, "ANTFSClientChannel::DecodeTransportCommand: Received ERASE request (%hu)", stHostRequestParams.usFileIndex);
                  DSIDebug::ThreadWrite((char*) aucString);
               }
               #endif

               DSIThread_MutexLock(&stMutexCriticalSection);
                  ucLinkCommandInProgress = ANTFS_ERASE_ID;
                  eANTFSRequest = ANTFS_REQUEST_ERASE;
                  DSIThread_CondSignal(&stCondRequest);
               DSIThread_MutexUnlock(&stMutexCriticalSection);
            }
         }   // ANTFS_ERASE_ID
         break;

      case ANTFS_DOWNLOAD_ID:
         {
            if((ucControlByte_ & SEQUENCE_NUMBER_ROLLOVER) == 0) // first packet
            {
               stHostRequestParams.usFileIndex = Convert_Bytes_To_USHORT(
                  pucTransCommand_[DATA_INDEX_OFFSET + 1],
                  pucTransCommand_[DATA_INDEX_OFFSET]);
               stHostRequestParams.ulOffset = Convert_Bytes_To_ULONG(
                  pucTransCommand_[DOWNLOAD_DATA_OFFSET_OFFSET + 3],
                  pucTransCommand_[DOWNLOAD_DATA_OFFSET_OFFSET + 2],
                  pucTransCommand_[DOWNLOAD_DATA_OFFSET_OFFSET + 1],
                  pucTransCommand_[DOWNLOAD_DATA_OFFSET_OFFSET]);
               #if defined(DEBUG_FILE)
               {
                  UCHAR aucString[256];
                  SNPRINTF((char*) aucString, 256, "ANTFSClientChannel::DecodeTransportCommand: Received DOWNLOAD request (%hu/%lu)", stHostRequestParams.usFileIndex, stHostRequestParams.ulOffset);
                  DSIDebug::ThreadWrite((char*) aucString);
               }
               #endif
            }
            else if (ucControlByte_ & SEQUENCE_LAST_MESSAGE)   // last (second) packet
            {
               stHostRequestParams.usCRCSeed = Convert_Bytes_To_USHORT(
                  pucTransCommand_[DOWNLOAD_DATA_CRC_OFFSET + 1],
                  pucTransCommand_[DOWNLOAD_DATA_CRC_OFFSET]);
               stHostRequestParams.ulBlockSize = Convert_Bytes_To_ULONG(
                  pucTransCommand_[DOWNLOAD_DATA_SIZE_OFFSET + 3],
                  pucTransCommand_[DOWNLOAD_DATA_SIZE_OFFSET + 2],
                  pucTransCommand_[DOWNLOAD_DATA_SIZE_OFFSET + 1],
                  pucTransCommand_[DOWNLOAD_DATA_SIZE_OFFSET]);
               stHostRequestParams.bInitialRequest = pucTransCommand_[DOWNLOAD_DATA_INITIAL_OFFSET] & 0x01;
               stHostRequestParams.ulMaxSize = 0;

               DSIThread_MutexLock(&stMutexCriticalSection);
               if(eANTFSState == ANTFS_CLIENT_STATE_TRANSPORT)
               {
                  ucLinkCommandInProgress = ANTFS_DOWNLOAD_ID;
                  eANTFSRequest = ANTFS_REQUEST_DOWNLOAD;
                  DSIThread_CondSignal(&stCondRequest);
               }
               DSIThread_MutexUnlock(&stMutexCriticalSection);
            }

         }   // ANTFS_DOWNLOAD_ID
         break;

      case ANTFS_UPLOAD_REQUEST_ID:
         {
            if(stInitParams.bUploadEnabled)   // Only process if we support uploads
            {
               if((ucControlByte_ & SEQUENCE_NUMBER_ROLLOVER) == 0) // first packet
               {
                  stHostRequestParams.bInitialRequest = FALSE;
                  stHostRequestParams.usFileIndex = Convert_Bytes_To_USHORT(
                     pucTransCommand_[DATA_INDEX_OFFSET + 1],
                     pucTransCommand_[DATA_INDEX_OFFSET]);
                  stHostRequestParams.ulMaxSize = Convert_Bytes_To_ULONG(
                     pucTransCommand_[UPLOAD_MAX_SIZE_OFFSET + 3],
                     pucTransCommand_[UPLOAD_MAX_SIZE_OFFSET + 2],
                     pucTransCommand_[UPLOAD_MAX_SIZE_OFFSET + 1],
                     pucTransCommand_[UPLOAD_MAX_SIZE_OFFSET]);
               }
               else if (ucControlByte_ & SEQUENCE_LAST_MESSAGE)   // last (second) packet
               {
                  stHostRequestParams.ulOffset = Convert_Bytes_To_ULONG(
                     pucTransCommand_[DATA_OFFSET_SMALL_OFFSET + 3],
                     pucTransCommand_[DATA_OFFSET_SMALL_OFFSET + 2],
                     pucTransCommand_[DATA_OFFSET_SMALL_OFFSET + 1],
                     pucTransCommand_[DATA_OFFSET_SMALL_OFFSET]);
                  stHostRequestParams.bInitialRequest = FALSE;
                  stHostRequestParams.ulBlockSize = 0;
                  stHostRequestParams.usCRCSeed = 0;

                  #if defined(DEBUG_FILE)
                  {
                     UCHAR aucString[256];
                     SNPRINTF((char*) aucString, 256, "ANTFSClientChannel::DecodeTransportCommand: Received UPLOAD request (%hu/%lu)", stHostRequestParams.usFileIndex, stHostRequestParams.ulOffset);
                     DSIDebug::ThreadWrite((char*) aucString);
                  }
                  #endif

                  DSIThread_MutexLock(&stMutexCriticalSection);
                  if(eANTFSState == ANTFS_CLIENT_STATE_TRANSPORT)
                  {
                     stHostRequestParams.bInitialRequest = TRUE;
                     ucLinkCommandInProgress = ANTFS_UPLOAD_REQUEST_ID;
                     //eANTFSRequest = ANTFS_REQUEST_UPLOAD;
                     // TODO: Uploads not implemented, should send upload request to application
                     // Reject the request, for now
                     ucRequestResponse = UPLOAD_RESPONSE_OK;
                     eANTFSRequest = ANTFS_REQUEST_UPLOAD;
                     DSIThread_CondSignal(&stCondRequest);
                  }
                  DSIThread_MutexUnlock(&stMutexCriticalSection);
               }
            }
            else
            {
            if (ucControlByte_ & SEQUENCE_LAST_MESSAGE)
            {
               // Reject if uploads are not supported
               DSIThread_MutexLock(&stMutexCriticalSection);
                 ucRequestResponse = UPLOAD_RESPONSE_REQUEST_INVALID;
                 eANTFSRequest = ANTFS_REQUEST_UPLOAD_RESPONSE;
                 DSIThread_CondSignal(&stCondRequest);
               DSIThread_MutexUnlock(&stMutexCriticalSection);
            }
            }
         }   // ANTFS_UPLOAD_ID
         break;

      case ANTFS_UPLOAD_DATA_ID:
         {
            if(stInitParams.bUploadEnabled && eANTFSState == ANTFS_CLIENT_STATE_UPLOADING_WAIT_FOR_RESPONSE)   // Only process if we support uploads
            {
               if((ucControlByte_ & SEQUENCE_NUMBER_ROLLOVER) == 0) // first packet
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeTransportCommand(): Received UPLOAD DATA.");
                  #endif
                  stHostRequestParams.usCRCSeed = Convert_Bytes_To_USHORT(
                     pucTransCommand_[UPLOAD_DATA_CRC_SEED_OFFSET + 1],
                     pucTransCommand_[UPLOAD_DATA_CRC_SEED_OFFSET]);
                  stHostRequestParams.ulOffset = Convert_Bytes_To_ULONG(
                     pucTransCommand_[UPLOAD_DATA_DATA_OFFSET_OFFSET + 3],
                     pucTransCommand_[UPLOAD_DATA_DATA_OFFSET_OFFSET + 2],
                     pucTransCommand_[UPLOAD_DATA_DATA_OFFSET_OFFSET + 1],
                     pucTransCommand_[UPLOAD_DATA_DATA_OFFSET_OFFSET]);

              ulTransferBytesRemaining = stHostRequestParams.ulMaxSize - stHostRequestParams.ulOffset;
              ulTransferBlockOffset = stHostRequestParams.ulOffset;

              //ulTransferBurstIndex = stHostRequestParams.ulMaxSize - stHostRequestParams.ulOffset;
              ulTransferBurstIndex = 0; //Should be zero as the starting index. Will be incremented inside UploadDataInput
              //ulTransferMaxIndex = ulTransferBufferSize;
              #if defined(DEBUG_FILE)
               char cBuffer[256];
               SNPRINTF(cBuffer, 256,"ANTFSClientChannel::DecodeTransportCommand():  CRCseed is: %d.",stHostRequestParams.usCRCSeed);
               DSIDebug::ThreadWrite(cBuffer);
              #endif
              eANTFSState = ANTFS_CLIENT_STATE_UPLOADING;
               }
               if(ucControlByte_ & SEQUENCE_LAST_MESSAGE)
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSClientChannel::DecodeTransportCommand(): Upload contains no data.");
                  #endif
               }
            }
         } // ANTFS_UPLOAD_DATA_ID:
         break;

      default:
         {
            #if defined(DEBUG_FILE)
               UCHAR aucString[256];
               SNPRINTF((char *) aucString, 256, "ANTFSClientChannel::DecodeTransportCommand(): Received invalid request: %u", ucLinkCommandInProgress);
               DSIDebug::ThreadWrite((char*) aucString);
            #endif
            ucLinkCommandInProgress = ANTFS_CMD_NONE;
         }   // DEFAULT
         break;
   }
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::UploadInputData(UCHAR ucControlByte_, UCHAR* pucMesg_)
{
     #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::UploadInputData(): ENTER.");
      #endif
   // TODO: Implement upload
      UCHAR ucBytesToCopy = 8;
   if (ulTransferBytesRemaining < ucBytesToCopy)
      ucBytesToCopy = (UCHAR)ulTransferBytesRemaining;


   if (ucControlByte_ & SEQUENCE_LAST_MESSAGE)
   {
    #if defined(DEBUG_FILE)
      UCHAR aucString[256];
      SNPRINTF((char *) aucString, 256, "ANTFSClientChannel::UploadInputData(): Upload received last packet: %u", eANTFSState);
      DSIDebug::ThreadWrite((char*) aucString);
    #endif
     #if defined(DEBUG_FILE)
      char cBuffer[256];
      SNPRINTF(cBuffer, 256,"ANTFSClientChannel::UploadInputData():  Transfercrc before last update is: %d.",usTransferCrc);
      DSIDebug::ThreadWrite(cBuffer);
      #endif
      usSavedTransferCrc = usTransferCrc;
      usTransferCrc = CRC_UpdateCRC16(usTransferCrc, &pucMesg_[6], 2);
     #if defined(DEBUG_FILE)
      char c2Buffer[256];
      SNPRINTF(c2Buffer, 256,"ANTFSClientChannel::UploadInputData():  Transfercrc after last update is: %d.",usTransferCrc);
      DSIDebug::ThreadWrite(c2Buffer);
      #endif
     #if defined(DEBUG_FILE)
      char c3Buffer[256];
      SNPRINTF(c3Buffer, 256,"ANTFSClientChannel::UploadInputData():  ulTransferBytesRemaining is: %d.",ulTransferBytesRemaining);
      DSIDebug::ThreadWrite(c3Buffer);
      #endif
        DSIThread_MutexLock(&stMutexCriticalSection);

      if(eANTFSState == ANTFS_CLIENT_STATE_UPLOADING)
        {
            //stHostRequestParams.bInitialRequest = TRUE;
            ucLinkCommandInProgress = ANTFS_UPLOAD_DATA_ID;
            //eANTFSRequest = ANTFS_REQUEST_UPLOAD;
            // TODO: Uploads not implemented, should send upload request to application
            // Reject the request, for now
         if (usTransferCrc)
         {
            ucRequestResponse = UPLOAD_RESPONSE_DOES_NOT_EXIST;  // substitute for failure
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::UploadInputData():  ucRequestResponse = UPLOAD_RESPONSE_DOES_NOT_EXIST.");
      #endif
         }
         else
         {
            ucRequestResponse = UPLOAD_RESPONSE_OK;
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::UploadInputData(): ucRequestResponse = UPLOAD_RESPONSE_OK.");
      #endif
         }

      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::UploadInputData(): Signalling ANTFS Thread.");
      #endif

         eANTFSRequest = ANTFS_REQUEST_UPLOAD_COMPLETE;
            DSIThread_CondSignal(&stCondRequest);
       }

       DSIThread_MutexUnlock(&stMutexCriticalSection);

    #if defined(DEBUG_FILE)
     if (usTransferCrc)
      DSIDebug::ThreadWrite("ANTFSClientChannel::UploadInputData(): Transfer CRC failed.");
      else
      DSIDebug::ThreadWrite("ANTFSClientChannel::UploadInputData(): Transfer CRC passed.");

    #endif
   }
   else
   {
       memcpy(&pucTransferBufferDynamic[ulTransferBurstIndex], pucMesg_, ucBytesToCopy);
      ulTransferBurstIndex += ucBytesToCopy;
      ulTransferBytesRemaining -= ucBytesToCopy;
      usTransferCrc = CRC_UpdateCRC16(usTransferCrc, pucMesg_, ucBytesToCopy);
   }
     #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::UploadInputData(): EXIT.");
      #endif
}

///////////////////////////////////////////////////////////////////////
ANTFSClientChannel::RETURN_STATUS ANTFSClientChannel::SwitchToLink(void)
{
   if(eANTFSState < ANTFS_CLIENT_STATE_IDLE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchToLink():  Device is not in correct state.");
      #endif

      return RETURN_FAIL;
   }

   if(eANTFSState > ANTFS_CLIENT_STATE_IDLE)
   {
      // Reload beacon link configuration
      ucActiveBeaconStatus1 = 0;
      ucActiveBeaconStatus1 |= ((stInitParams.ucLinkPeriod & BEACON_PERIOD_MASK) << BEACON_PERIOD_SHIFT);
      ucActiveBeaconStatus1 |= stInitParams.bPairingEnabled * PAIRING_AVAILABLE_FLAG_MASK;
      ucActiveBeaconStatus1 |= stInitParams.bUploadEnabled * UPLOAD_ENABLED_FLAG_MASK;
      ucActiveBeaconStatus1 |= stInitParams.bDataAvailable * DATA_AVAILABLE_FLAG_MASK;

      ucActiveBeaconFrequency = stInitParams.ucBeaconFrequency;
      if(pclANT->SetChannelRFFrequency(ucChannelNumber, ucActiveBeaconFrequency, MESSAGE_TIMEOUT) == FALSE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchToLink():  Failed ANT_SetChannelRFFreq().");
         #endif
         return RETURN_SERIAL_ERROR;
      }

      SetANTChannelPeriod(stInitParams.ucLinkPeriod);
      if (pclANT->SetChannelPeriod(ucChannelNumber, usTheMessagePeriod, MESSAGE_TIMEOUT) == FALSE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchToLink():  Failed ANT_SetChannelPeriod().");
         #endif
         return RETURN_SERIAL_ERROR;
      }

      if(bCustomTxPower)
      {
         if(pclANT->SetChannelTransmitPower(ucChannelNumber, ucLinkTxPower, MESSAGE_TIMEOUT) == FALSE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchToLink():  Failed ANT_SetChannelTransmitPower(), setting power level for all channels.");
            #endif

            if(pclANT->SetAllChannelsTransmitPower(ucLinkTxPower, MESSAGE_TIMEOUT) == FALSE)
            {
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchToLink():  Failed ANT_SetAllChannelsTransmitPower().");
               #endif

               return RETURN_SERIAL_ERROR;
            }
         }
      }

      if(bReturnToBroadcast) // No need to reload beacon if going back to broadcast
      {
         ucLinkCommandInProgress = ANTFS_CMD_NONE;
         return RETURN_PASS;
      }
   }

   eANTFSState = ANTFS_CLIENT_STATE_BEACONING;
   ucLinkCommandInProgress = ANTFS_CMD_NONE;

   bReceivedCommand = FALSE;
   bReceivedBurst = FALSE;
   bRxError = FALSE;

   LoadBeacon();
   if(pclANT->SendBroadcastData(ucChannelNumber, aucBeacon) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchToLink(): Failed to load beacon.");
      #endif
      return RETURN_SERIAL_ERROR;
   }

   return RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFSClientChannel::RETURN_STATUS ANTFSClientChannel::SwitchToAuthenticate(void)
{
   if(eANTFSState < ANTFS_CLIENT_STATE_BEACONING)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchToAuthenticate():  Device is not in correct state.");
      #endif

      return RETURN_FAIL;
   }

   if(eANTFSState < ANTFS_CLIENT_STATE_CONNECTED)
   {
      if(pclANT->SetChannelRFFrequency(ucChannelNumber, ucActiveBeaconFrequency, MESSAGE_TIMEOUT) == FALSE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchToAuthenticate():  Failed ANT_SetChannelRFFreq().");
         #endif
         return RETURN_SERIAL_ERROR;
      }

      if (pclANT->SetChannelPeriod(ucChannelNumber, usTheMessagePeriod, MESSAGE_TIMEOUT) == FALSE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchToAuthenticate():  Failed ANT_SetChannelPeriod().");
         #endif
         return RETURN_SERIAL_ERROR;
      }

      if(bCustomTxPower)
      {
         if(pclANT->SetChannelTransmitPower(ucChannelNumber, ucSessionTxPower, MESSAGE_TIMEOUT) == FALSE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchToAuthenticate():  Failed ANT_SetChannelTransmittPower(), setting power level for all channels.");
            #endif

            if(pclANT->SetAllChannelsTransmitPower(ucSessionTxPower, MESSAGE_TIMEOUT) == FALSE)
            {
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchToAuthenticate():  Failed ANT_SetAllChannelsTransmittPower().");
               #endif

               return RETURN_SERIAL_ERROR;
            }
         }
      }
   }

   ucPairingTimeout = MAX_UCHAR;
   eANTFSState = ANTFS_CLIENT_STATE_CONNECTED;
   ucLinkCommandInProgress = ANTFS_CMD_NONE;

   bReceivedCommand = FALSE;
   bReceivedBurst = FALSE;
   bRxError = FALSE;

   LoadBeacon();
   if(pclANT->SendBroadcastData(ucChannelNumber, aucBeacon) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchToAuthenticate(): Failed to load beacon.");
      #endif
      return RETURN_SERIAL_ERROR;
   }

   return RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFSClientChannel::RETURN_STATUS ANTFSClientChannel::SwitchToTransport(void)
{
   if(eANTFSState < ANTFS_CLIENT_STATE_CONNECTED)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchToTransport():  Device is not in correct state.");
      #endif

      return RETURN_FAIL;
   }

   eANTFSState = ANTFS_CLIENT_STATE_TRANSPORT;
   ucLinkCommandInProgress = ANTFS_CMD_NONE;

   bReceivedCommand = FALSE;
   bReceivedBurst = FALSE;
   bRxError = FALSE;

   LoadBeacon();
   if(pclANT->SendBroadcastData(ucChannelNumber, aucBeacon) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchToTransport(): Failed to load beacon.");
      #endif
      return RETURN_SERIAL_ERROR;
   }

   return RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFSClientChannel::RETURN_STATUS ANTFSClientChannel::SwitchLinkParameters(void)
{
   if(eANTFSState < ANTFS_CLIENT_STATE_TRANSPORT)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchLinkParameters():  Device is not in correct state.");
      #endif

      return RETURN_FAIL;
   }

   if(pclANT->SetChannelRFFrequency(ucChannelNumber, ucActiveBeaconFrequency, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchLinkParameters():  Failed ANT_SetChannelRFFreq().");
      #endif
      return RETURN_SERIAL_ERROR;
   }

   if (pclANT->SetChannelPeriod(ucChannelNumber, usTheMessagePeriod, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchLinkParameters():  Failed ANT_SetChannelPeriod().");
      #endif
      return RETURN_SERIAL_ERROR;
   }

   ucLinkCommandInProgress = ANTFS_CMD_NONE;

   LoadBeacon();
   if(pclANT->SendBroadcastData(ucChannelNumber, aucBeacon) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSClientChannel::SwitchLinkParameters(): Failed to load beacon.");
      #endif
      return RETURN_SERIAL_ERROR;
   }

   return RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
void ANTFSClientChannel::SetANTChannelPeriod(UCHAR ucLinkPeriod_)
{
   switch (ucLinkPeriod_)
   {
      default: // Shouldn't happen.
      case BEACON_PERIOD_0_5_HZ:
         usTheMessagePeriod = 65535;
         break;
      case BEACON_PERIOD_1_HZ:
         usTheMessagePeriod = 32768;
         break;
      case BEACON_PERIOD_2_HZ:
         usTheMessagePeriod = 16384;
         break;
      case BEACON_PERIOD_4_HZ:
         usTheMessagePeriod = 8192;
         break;
      case BEACON_PERIOD_8_HZ:
         usTheMessagePeriod = 4096;
         break;
      case BEACON_PERIOD_KEEP:
         usTheMessagePeriod = usBeaconChannelPeriod;
         break;
   }

   ucActiveBeaconStatus1 &= ~BEACON_PERIOD_MASK;
   ucActiveBeaconStatus1 |= ((ucLinkPeriod_ & BEACON_PERIOD_MASK) << BEACON_PERIOD_SHIFT);
}


