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

#include "antfs_host_channel.hpp"
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

#define TRANSPORT_NETWORK              ((UCHAR) 0)

#define TRANSPORT_MESSAGE_PERIOD       ((USHORT) 4096)      // 8Hz default
#define TRANSPORT_MESSAGE_PERIOD_CODE  ((UCHAR) 4)          // 8Hz default
#define TRANSPORT_SEARCH_TIMEOUT       ((UCHAR) 3)          // ~7.5 Seconds.

#define HOST_SERIAL_NUMBER             ((ULONG) 1)          // Use if not configured otherwise

#define UPLOAD_PROGRESS_CHECK_BYTES    ((ULONG) 64)         // 64 bytes (8 packets) have to be transfered in order to reset the UL loop timeout.

#define ADDRESS_TYPE_PAGE_OFFSET       3
#define ADDRESS_TYPE_PAGE_SMALL_0      ((UCHAR) 0x00)       // Default; page contains both data offset and data block size.
#define ADDRESS_TYPE_PAGE_SMALL_1      ((UCHAR) 0x01)       // Contains total remaining data length (in response)
#define ADDRESS_TYPE_PAGE_BIG_0        ((UCHAR) 0x02)       // Page contains data offset.
#define ADDRESS_TYPE_PAGE_BIG_1        ((UCHAR) 0x03)       // Page contains data block size.
#define ADDRESS_TYPE_PAGE_BIG_2        ((UCHAR) 0x04)       // Page contains total remaining data length (in response)
#define DATA_OFFSET_OFFSET_LOW         4
#define DATA_OFFSET_OFFSET_HIGH        5
#define DATA_BLOCK_SIZE_OFFSET_LOW     6
#define DATA_BLOCK_SIZE_OFFSET_HIGH    7

// ID Command
#define ID_OFFSET_0                    4
#define ID_OFFSET_1                    5
#define ID_OFFSET_2                    6
#define ID_OFFSET_3                    7

#define ANTFS_RESPONSE_RETRIES         5

#define SEND_DIRECT_BURST_TIMEOUT      15000                // 15 seconds.

#define STRIKE_COUNT                   2

#define MAX_STALE_COUNT                ((UCHAR) 100)
#define MAJOR_STALE_COUNT              ((UCHAR) 50)
#define MINOR_STALE_COUNT              ((UCHAR) 25)
#define MIN_STALE_COUNT                ((UCHAR) 1)


typedef void (*CALLBACK_T)(void);

static const UCHAR caucNetworkKey[] = NETWORK_KEY;
static const char acSwVersion[] = SW_VER;

//////////////////////////////////////////////////////////////////////////////////
// Private Function Prototypes
//////////////////////////////////////////////////////////////////////////////////
static int ListCompare(const void *puvKeyVal, const void *puvDatum);
static int DeviceParametersItemCompare(const void *pvItem1, const void *pvItem2);

//////////////////////////////////////////////////////////////////////////////////
// Public Functions
//////////////////////////////////////////////////////////////////////////////////
ANTFSHostChannel::ANTFSHostChannel()
{
   bInitFailed = FALSE;
   bIgnoreListRunning = FALSE;
   bPingEnabled = FALSE;
   bRequestPageEnabled = FALSE;

   // Default channel configuration
   ucChannelNumber = ANTFS_CHANNEL;
   ucNetworkNumber = ANTFS_NETWORK;
   ucTheDeviceType = ANTFS_DEVICE_TYPE;
   ucTheTransmissionType = ANTFS_TRANSMISSION_TYPE;
   usTheMessagePeriod = ANTFS_MESSAGE_PERIOD;
   ucTheProxThreshold = 0; //0=Disabled
   memcpy(aucTheNetworkkey,caucNetworkKey,8);

   cfgParams.ul_cfg_auth_timeout = AUTH_TIMEOUT;
   cfgParams.ul_cfg_burst_check_timeout = BURST_CHECK_TIMEOUT;
   cfgParams.ul_cfg_erase_timeout = ERASE_TIMEOUT;
   cfgParams.ul_cfg_upload_request_timeout = UPLOAD_REQUEST_TIMEOUT;
   cfgParams.ul_cfg_upload_response_timeout = UPLOAD_RESPONSE_TIMEOUT;

   // Host configuration
   ulHostSerialNumber = HOST_SERIAL_NUMBER;

   hANTFSThread = (DSI_THREAD_ID)NULL;                 // Handle for the ANTFS thread
   bKillThread = FALSE;
   bANTFSThreadRunning = FALSE;

   pclANT = (DSIFramerANT*) NULL;
   pbCancel = &bCancel;
   *pbCancel = FALSE;

   // Ignore List variables
   usListIndex = 0;
   memset(astIgnoreList, 0, sizeof(astIgnoreList));
   pclQueueTimer = (DSITimer*)NULL;

   ucTransportFrequencyStaleCount = 0;
   ucCurrentTransportFreqElement = 0;
   memset(aucFrequencyTable, 0, sizeof(aucFrequencyTable));

   memset(asDeviceParametersList, 0, sizeof(asDeviceParametersList));
   usDeviceListSize = 0;

   eANTFSState = ANTFS_HOST_STATE_OFF;
   ResetHostState();

   // Debugging is initialized by DSIANTDevice (or ANTDevice in Managed Lib)

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

   //If the init fails, something is broken and nothing is going to work anyway
   //additionally the logging isn't even setup at this point
   //so we throw an exception so we know something is broken right away
   if(bInitFailed == TRUE)
      throw "ANTFS constructor: init failed";
}

///////////////////////////////////////////////////////////////////////
ANTFSHostChannel::~ANTFSHostChannel()
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
UCHAR ANTFSHostChannel::GetVersion(UCHAR *pucVersionString_, UCHAR ucBufferSize_)
{
   ucBufferSize_--;                                         // Allow for NULL cstring termination.

   if (ucBufferSize_ > sizeof(acSwVersion))
      ucBufferSize_ = sizeof(acSwVersion);

   memcpy(pucVersionString_, acSwVersion, ucBufferSize_);
   pucVersionString_[ucBufferSize_++] = '\0';               // Terminate the string and increment the size.

   return ucBufferSize_;                                    // Return the string size, including the terminator.
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHostChannel::Init(DSIFramerANT* pclANT_, UCHAR ucChannel_)
{
   if (bInitFailed == TRUE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Init():  bInitFailed == TRUE");
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
void ANTFSHostChannel::GetCurrentConfig( ANTFS_CONFIG_PARAMETERS* pCfg_ )
{
   memcpy( pCfg_, &cfgParams, sizeof( ANTFS_CONFIG_PARAMETERS ) );
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHostChannel::SetCurrentConfig( const ANTFS_CONFIG_PARAMETERS* pCfg_ )
{
   // validation can be performed here
   BOOL bValid = TRUE;

   // ignore the entire set if invalid
   if( bValid )
   {
      memcpy( &cfgParams, pCfg_, sizeof( ANTFS_CONFIG_PARAMETERS ) );
   }

   return bValid;
}

///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::SetChannelID(UCHAR ucDeviceType_, UCHAR ucTransmissionType_)
{
   ucTheDeviceType = ucDeviceType_;
   ucTheTransmissionType = ucTransmissionType_;
}

///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::SetChannelPeriod(USHORT usChannelPeriod_)
{
   usTheMessagePeriod = usChannelPeriod_;
}

///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::SetNetworkKey(UCHAR ucNetwork_, UCHAR ucNetworkkey[])
{
   ucNetworkNumber = ucNetwork_;
   memcpy(aucTheNetworkkey,ucNetworkkey,8);
}

///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::SetProximitySearch(UCHAR ucSearchThreshold_)
{
   if(ucSearchThreshold_ > 10)   //Depending on device higher thresholds can be ignored, cause undefined consequences, or even have different functionality assigned to them, so we cap at the valid 0-10 range
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::SetProximitySearch():  User threshold value > 10, setting to max 10.");
      #endif
      ucSearchThreshold_ = 10;
   }
   ucTheProxThreshold = ucSearchThreshold_;
}


///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::SetSerialNumber(ULONG ulSerialNumber_)
{
   ulHostSerialNumber = ulSerialNumber_;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHostChannel::GetEnabled()
{
   if(eANTFSState < ANTFS_HOST_STATE_DISCONNECTING)
   {
      return FALSE;
   }

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
ANTFS_HOST_STATE ANTFSHostChannel::GetStatus(void)
{
   return eANTFSState;
}

///////////////////////////////////////////////////////////////////////
UCHAR ANTFSHostChannel::GetConnectedDeviceBeaconAntfsState(void)
{
    if (eANTFSState < ANTFS_HOST_STATE_CONNECTED)
        return REMOTE_DEVICE_BEACON_NOT_FOUND;
    else
        return ucFoundDeviceState;
}

///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::Close(void)
{
   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHostChannel::Close():  Closing ANTFS...");
   #endif

   DSIThread_MutexLock(&stMutexCriticalSection);

   // Stop the threads.
   bKillThread = TRUE;
   *pbCancel = TRUE;

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHostChannel::Close():  SetEvent(stCondWaitForResponse).");
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
            DSIDebug::ThreadWrite("ANTFSHostChannel::Close():  SetEvent(stCondRequest).");
         #endif
         DSIThread_CondSignal(&stCondRequest);

         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::Close():  SetEvent(stCondRxEvent).");
         #endif
         DSIThread_CondSignal(&stCondRxEvent);


         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::Close():  Killing thread.");
         #endif

         if (DSIThread_CondTimedWait(&stCondANTFSThreadExit, &stMutexCriticalSection, 9000) != DSI_THREAD_ENONE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Close():  Thread not dead.");
               DSIDebug::ThreadWrite("ANTFSHostChannel::Close():  Forcing thread termination...");
            #endif
            DSIThread_DestroyThread(hANTFSThread);
         }
         else
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Close():  Thread terminated successfully.");
            #endif
         }
      }

      DSIThread_ReleaseThreadID(hANTFSThread);
      hANTFSThread = (DSI_THREAD_ID)NULL;
   }

   DSIThread_MutexUnlock(&stMutexCriticalSection);

   if (bIgnoreListRunning)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Close():  Deleting Timer Queue Timer...");
      #endif

      delete pclQueueTimer;
      pclQueueTimer = (DSITimer*)NULL;
      DSIThread_MutexDestroy(&stMutexIgnoreListAccess);

      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Close():  Deleted Timer Queue Timer.");
      #endif

      bIgnoreListRunning = FALSE;
   }

   eANTFSState = ANTFS_HOST_STATE_OFF;

   // Threads are cleaned, revert Cancel status.
   *pbCancel = FALSE;

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHostChannel::Close():  Closed.");
   #endif

   if (pucTransferBufferDynamic)
   {
      if (pucTransferBuffer == pucTransferBufferDynamic)
         pucTransferBuffer = (UCHAR*)NULL;

      delete[] pucTransferBufferDynamic;
      pucTransferBufferDynamic = (UCHAR*)NULL;
   }
}

///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::Cancel(void)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   *pbCancel = TRUE;

   DSIThread_CondSignal(&stCondRxEvent);
   DSIThread_CondSignal(&stCondRequest);

   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return;
}

///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::ProcessDeviceNotification(ANT_DEVICE_NOTIFICATION eCode_, void* pvParameter_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if(eCode_ != ANT_DEVICE_NOTIFICATION_RESET &&
      eCode_ != ANT_DEVICE_NOTIFICATION_SHUTDOWN)
   {
      #if defined(DEBUG_FILE)
         UCHAR aucString[256];
         SNPRINTF((char *) aucString, 256, "ANTFSHostChannel::ProcessDeviceNotification():  Unknown code %0", eCode_);
         DSIDebug::ThreadWrite((char *) aucString);
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return;
   }

   if(eANTFSState <= ANTFS_HOST_STATE_IDLE)
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
      DSIDebug::ThreadWrite("ANTFSHostChannel::ProcessDeviceNotification(): Resetting state...");
   #endif
   *pbCancel = TRUE;
   eANTFSRequest = ANTFS_REQUEST_INIT;
   DSIThread_CondSignal(&stCondRxEvent);
   DSIThread_CondSignal(&stCondRequest);
   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return;
}

///////////////////////////////////////////////////////////////////////
#define MESG_CHANNEL_OFFSET                  0
#define MESG_EVENT_ID_OFFSET                 1
#define MESG_EVENT_CODE_OFFSET               2

void ANTFSHostChannel::ProcessMessage(ANT_MESSAGE* pstMessage_, USHORT usMesgSize_)
{
   UCHAR ucANTChannel;
   BOOL bProcessed = FALSE;

   // Check that we are in the correct state to receive messages
   if(!GetEnabled())
      return; // Only process ANT messages if ANT-FS is on

   if (usMesgSize_ < DSI_FRAMER_TIMEDOUT)  //if the return isn't DSI_FRAMER_TIMEDOUT or DSI_FRAMER_ERROR
   {
      ucANTChannel = pstMessage_->aucData[MESG_CHANNEL_OFFSET] & CHANNEL_NUMBER_MASK;
      if(!FilterANTMessages(pstMessage_, ucANTChannel))
         return;  // Wrong channel, do not process

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

//////////////////////////////////////////////////////////////////////////////////
// ANTFS Link Layer
//////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
USHORT ANTFSHostChannel::AddSearchDevice(ANTFS_DEVICE_PARAMETERS *pstDeviceSearchMask_, ANTFS_DEVICE_PARAMETERS *pstDeviceParameters_)
{
   USHORT usHandle;

   if (usDeviceListSize >= SEARCH_DEVICE_LIST_MAX_SIZE)
      return 0;

   // Find the next available handle.
   usHandle = 0;
   while (usHandle < usDeviceListSize)
   {
      if ((usHandle + 1) < asDeviceParametersList[usHandle].usHandle)
         break;

      usHandle++;
   }

   asDeviceParametersList[usDeviceListSize].usHandle = ++usHandle;
   asDeviceParametersList[usDeviceListSize].sDeviceParameters = *pstDeviceParameters_;
   asDeviceParametersList[usDeviceListSize].sDeviceSearchMask = *pstDeviceSearchMask_;

   usDeviceListSize++;

   qsort(asDeviceParametersList, usDeviceListSize, sizeof(DEVICE_PARAMETERS_ITEM), &DeviceParametersItemCompare);

   return usHandle;
}

///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::RemoveSearchDevice(USHORT usHandle_)
{
   DEVICE_PARAMETERS_ITEM sDeviceParametersItem;
   DEVICE_PARAMETERS_ITEM *psDeviceParametersItem;

   if (usDeviceListSize == 0)
      return;                                               // Nothing to do.

   // Find the handle in the list.
   sDeviceParametersItem.usHandle = usHandle_;
   psDeviceParametersItem =(DEVICE_PARAMETERS_ITEM *) bsearch(&sDeviceParametersItem, asDeviceParametersList, usDeviceListSize,  sizeof(DEVICE_PARAMETERS_ITEM), &DeviceParametersItemCompare);

   if (psDeviceParametersItem != NULL)
   {
      psDeviceParametersItem->usHandle = MAX_USHORT;        // Make it invalid.
      qsort(asDeviceParametersList, usDeviceListSize, sizeof(DEVICE_PARAMETERS_ITEM), &DeviceParametersItemCompare); // Sorting will cause the large invalid item to drop to the end of the list.
      usDeviceListSize--;                                   // Reduce the size to reflect that the item has been removed.
   }
}

///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::ClearSearchDeviceList(void)
{
   usDeviceListSize = 0;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHostChannel::SearchForDevice(UCHAR ucSearchRadioFrequency_, UCHAR ucConnectedRadioFrequency_, USHORT usRadioChannelID_, BOOL bUseRequestPage_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if (eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::SearchForDevice():  Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if (eANTFSState != ANTFS_HOST_STATE_IDLE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::SearchForDevice():  Not in correct state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHostChannel::SearchForDevice():  Search starting...");
   #endif

   ucSearchRadioFrequency = ucSearchRadioFrequency_;
   ucTransportFrequencySelection = ucConnectedRadioFrequency_;
   usRadioChannelID = usRadioChannelID_;
   bRequestPageEnabled = bUseRequestPage_;
   eANTFSRequest = ANTFS_REQUEST_SEARCH;
   DSIThread_CondSignal(&stCondRequest);

   DSIThread_MutexUnlock(&stMutexCriticalSection);

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHostChannel::SearchForDevice():  Search request pending...");
   #endif

   return ANTFS_RETURN_PASS;
}


///////////////////////////////////////////////////////////////////////
BOOL ANTFSHostChannel::GetFoundDeviceParameters(ANTFS_DEVICE_PARAMETERS *pstDeviceParameters_, UCHAR *aucFriendlyName_, UCHAR *pucBufferSize_)
{
   if (eANTFSState < ANTFS_HOST_STATE_CONNECTED)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::GetFoundDeviceParameters():  Not in correct state.");
      #endif
      return FALSE;
   }

   memcpy(pstDeviceParameters_, &stFoundDeviceParameters, sizeof(ANTFS_DEVICE_PARAMETERS));

   if (ucRemoteFriendlyNameLength < *pucBufferSize_)
      *pucBufferSize_ = ucRemoteFriendlyNameLength;

   memcpy(aucFriendlyName_, aucRemoteFriendlyName, *pucBufferSize_);

   *pucBufferSize_ = ucRemoteFriendlyNameLength;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHostChannel::GetFoundDeviceChannelID(USHORT *pusDeviceNumber_, UCHAR *pucDeviceType_, UCHAR *pucTransmitType_)
{
   if (eANTFSState < ANTFS_HOST_STATE_CONNECTED)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::GetFoundDeviceChannelID():  Not in correct state.");
      #endif
      return FALSE;
   }

   if(pusDeviceNumber_ != NULL)
      *pusDeviceNumber_ = usFoundANTFSDeviceID;

   if(pucDeviceType_ != NULL)
      *pucDeviceType_ = ucFoundANTDeviceType;

   if(pucTransmitType_ != NULL)
      *pucTransmitType_ = ucFoundANTTransmitType;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHostChannel::RequestSession(UCHAR ucBroadcastRadioFrequency_, UCHAR ucConnectRadioFrequency_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if(eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::RequestSession():  Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if(eANTFSState != ANTFS_HOST_STATE_IDLE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::RequestSession():  Not in correct state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   ucTransportLayerRadioFrequency = ucConnectRadioFrequency_;
   ucSearchRadioFrequency = ucBroadcastRadioFrequency_;

   eANTFSRequest = ANTFS_REQUEST_SESSION_REQ;
   DSIThread_CondSignal(&stCondRequest);
   DSIThread_MutexUnlock(&stMutexCriticalSection);

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHostChannel::RequestSession():  Request for ANT-FS session pending...");
   #endif

   return ANTFS_RETURN_PASS;
}


///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHostChannel::Authenticate(UCHAR ucAuthenticationType_, UCHAR *pucAuthenticationString_, UCHAR ucLength_, UCHAR *pucResponseBuffer_, UCHAR *pucResponseBufferSize_, ULONG ulResponseTimeout_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if (eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Authenticate():  Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if (eANTFSState != ANTFS_HOST_STATE_CONNECTED)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Authenticate():  Not in correct state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   ucTxPasswordLength = ucLength_;
   ucAuthType = ucAuthenticationType_;

   pucResponseBuffer = pucResponseBuffer_;
   pucResponseBufferSize = pucResponseBufferSize_;
   ulAuthResponseTimeout = ulResponseTimeout_;


   memcpy(aucTxPassword, pucAuthenticationString_, ucTxPasswordLength);

   //may switch on Auth type here...
   eANTFSRequest = ANTFS_REQUEST_AUTHENTICATE;
   DSIThread_CondSignal(&stCondRequest);

   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHostChannel::Disconnect(USHORT usBlackoutTime_, UCHAR ucDisconnectType_, UCHAR ucTimeDuration_, UCHAR ucAppSpecificDuration_)
{
   ANTFS_RETURN eReturn = ANTFS_RETURN_PASS;
   DSIThread_MutexLock(&stMutexCriticalSection);

   if (eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Disconnect():  Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if (eANTFSState < ANTFS_HOST_STATE_REQUESTING_SESSION)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Disconnect():  Already disconnected.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   if (eANTFSState >= ANTFS_HOST_STATE_CONNECTED)
   {
      if (usBlackoutTime_ )
      {
         if(Blackout(usFoundANTFSDeviceID, usFoundANTFSManufacturerID, usFoundANTFSDeviceType, usBlackoutTime_) == FALSE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Disconnect():  Failed adding device to ignore list.");
            #endif
            eReturn = ANTFS_RETURN_FAIL;
         }
      }
   }

   ucDisconnectType = ucDisconnectType_;
   ucUndiscoverableTimeDuration = ucTimeDuration_;
   ucUndiscoverableAppSpecificDuration = ucAppSpecificDuration_;
   eANTFSRequest = ANTFS_REQUEST_DISCONNECT;

   DSIThread_CondSignal(&stCondRequest);
   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return eReturn;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHostChannel::SwitchFrequency(UCHAR ucRadioFrequency_, UCHAR ucChannelPeriod_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if (eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::SwitchFrequency():  Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if (eANTFSState != ANTFS_HOST_STATE_TRANSPORT)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::SwitchFrequency():  Not in transport state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   ucTransportFrequencySelection = ucRadioFrequency_;
   ucTransportChannelPeriodSelection = ucChannelPeriod_;

   eANTFSRequest = ANTFS_REQUEST_CHANGE_LINK;
   DSIThread_CondSignal(&stCondRequest);

   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return ANTFS_RETURN_PASS;

}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHostChannel::Download(USHORT usFileIndex_, ULONG ulDataOffset_, ULONG ulMaxDataLength_, ULONG ulMaxBlockSize_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if (eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Download():  Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if (eANTFSState != ANTFS_HOST_STATE_TRANSPORT)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Download():  Not in transport state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   bTransfer = FALSE;
   ulTransferTotalBytesRemaining = 0;
   ulTransferBytesInBlock = 0;

   usTransferDataFileIndex = usFileIndex_;
   ulTransferDataOffset = ulDataOffset_;
   ulTransferByteSize = ulMaxDataLength_;
   ulHostBlockSize = ulMaxBlockSize_;

   #if defined(DEBUG_FILE)
      {
         char szString[256];
         SNPRINTF(szString, 256, "ANTFSHostChannel::Download():\n   usTransferDataFileIndex = %u.\n   ulTransferDataOffset = %lu.\n   ulTransferByteSize = %lu.",
            usTransferDataFileIndex, ulTransferDataOffset, ulTransferByteSize);
         DSIDebug::ThreadWrite(szString);
      }
   #endif

   if ((usFoundANTFSManufacturerID == 1) && (usFoundANTFSDeviceType == 782))
      bLargeData = FALSE;  // !! Forerunner 50 only -- obsolete for all other devices.
   else
      bLargeData = TRUE;

   eANTFSRequest = ANTFS_REQUEST_DOWNLOAD;
   DSIThread_CondSignal(&stCondRequest);

   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHostChannel::Upload(USHORT usFileIndex_, ULONG ulDataOffset_, ULONG ulDataLength_, void *pvData_, BOOL bForceOffset_, ULONG ulMaxBlockSize_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if (eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if (eANTFSState != ANTFS_HOST_STATE_TRANSPORT)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Not in transport state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   bTransfer = FALSE;
   ulUploadIndexProgress = 0;

   usTransferDataFileIndex = usFileIndex_;
   ulTransferDataOffset = ulDataOffset_;
   ulTransferByteSize = ulDataLength_;

   pucUploadData = (UCHAR*)pvData_;
   bForceUploadOffset = bForceOffset_;
   if (ulDataOffset_)
      bForceUploadOffset = TRUE;  //Force the offset if the app is giving us an initial offset other than 0.

   if(ulMaxBlockSize_ == 0)
      ulHostBlockSize = MAX_ULONG;
   else
      ulHostBlockSize = ulMaxBlockSize_;

#if 0
   if ((ulTransferDataOffset > MAX_USHORT) || (ulTransferByteSize > MAX_USHORT))
      bLargeData = TRUE;
   else
      bLargeData = FALSE;
#endif

   bLargeData = TRUE;

   eANTFSRequest = ANTFS_REQUEST_UPLOAD;
   DSIThread_CondSignal(&stCondRequest);

   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHostChannel::ManualTransfer(USHORT usFileIndex_, ULONG ulDataOffset_, ULONG ulDataLength_, void *pvData_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if (eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::ManualTransfer():  Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if (eANTFSState != ANTFS_HOST_STATE_TRANSPORT)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::ManualTransfer():  Not in transport state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   if (ulDataOffset_ >= DIRECT_TRANSFER_SIZE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::ManualTransfer():  ulDataOffset) too large.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   if ((ulDataLength_ > DIRECT_TRANSFER_SIZE) || (ulDataLength_ == 0))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::ManualTransfer():  ulDataLength_ is 0 or too large.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   memcpy(&aucSendDirectBuffer[8], pvData_, ulDataLength_); //pvData_ must be little endian when sent to device!
   memset(&aucSendDirectBuffer[ulDataLength_ + 8], 0, 8 - (ulDataLength_ % 8) );  // Clear the rest of the last payload packet.

   #if defined(DEBUG_FILE)
      {
         char acString[256];

         SNPRINTF(acString, 256, "ANTFSHostChannel::ManualTransfer():  ulDataLength_ = %lu; ulDataOffset_ = %lu.", ulDataLength_, ulDataOffset_);
         DSIDebug::ThreadWrite(acString);
      }
   #endif

   bTransfer = FALSE;
   ulUploadIndexProgress = 0;
   ulTransferTotalBytesRemaining = 0;

   usTransferDataFileIndex = usFileIndex_;
   ulTransferDataOffset = ulDataOffset_;
   ulTransferByteSize = ulDataLength_;

   bLargeData = FALSE;
   eANTFSRequest = ANTFS_REQUEST_MANUAL_TRANSFER;
   DSIThread_CondSignal(&stCondRequest);

   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return ANTFS_RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHostChannel::GetDownloadStatus(ULONG *pulByteProgress_, ULONG *pulTotalLength_)
{
   ULONG ulOffset;

   if (ulTransferTotalBytesRemaining == 0)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::GetDownloadStatus():  Download not in progress.");
      #endif

      *pulTotalLength_ = 10;
      *pulByteProgress_ = 0;
      return FALSE;
   }

   *pulTotalLength_ = ulTransferTotalBytesRemaining;

   // The first 8 / 16 bytes of the array form the response packet.
   // Subtract them from the length count.
   if (bLargeData)
      ulOffset = 16;
   else
      ulOffset = 8;

   if (ulTransferArrayIndex >= ulOffset)
      *pulByteProgress_ = ulTransferArrayIndex - ulOffset;
   else
      *pulByteProgress_ = 0;

   *pulByteProgress_ += ulTransferDataOffset;

   if (*pulByteProgress_ > *pulTotalLength_)  //the index can go beyond the total length because we transfer with 8 byte packets
      *pulByteProgress_ = *pulTotalLength_;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////

BOOL ANTFSHostChannel::GetTransferData(ULONG *pulDataSize_ , void *pvData_)
{
   ULONG ulLength;
   int iOffset;

   if ((!bTransfer) || (pucTransferBufferDynamic == NULL))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::GetTransferData():  No valid data.");
      #endif
      return FALSE;
   }

   if (bLargeData)
      iOffset = 16;
   else
      iOffset = 8;

   ulLength = ulTransferArrayIndex - iOffset;

   if (ulLength > ulTransferTotalBytesRemaining)
      ulLength = ulTransferTotalBytesRemaining;

   if (pvData_ != NULL)
      memcpy(pvData_, pucTransferBufferDynamic + iOffset, ulLength);

   if (pulDataSize_ != NULL)
      *pulDataSize_ = ulLength;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////

BOOL ANTFSHostChannel::RecoverTransferData(ULONG *pulDataSize_ , void *pvData_)
{
   ULONG ulLength;
   int iOffset;

   if (pucTransferBufferDynamic == NULL)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::GetTransferData():  No valid data.");
      #endif
      return FALSE;
   }

   if (bLargeData)
      iOffset = 16;
   else
      iOffset = 8;

   ulLength = ulTransferArrayIndex - iOffset;

   if (ulLength > ulTransferTotalBytesRemaining)
      ulLength = ulTransferTotalBytesRemaining;

   if (pvData_ != NULL)
      memcpy(pvData_, pucTransferBufferDynamic + iOffset, ulLength);

   if (pulDataSize_ != NULL)
      *pulDataSize_ = ulLength;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHostChannel::GetUploadStatus(ULONG *pulByteProgress_, ULONG *pulTotalLength_)
{
   if (ulTransferByteSize == 0)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::GetDownloadStatus():  Upload not in progress.");
      #endif

      *pulTotalLength_ = 10;
      *pulByteProgress_ = 0;
      return FALSE;
   }

   *pulTotalLength_ = ulTransferByteSize;
   *pulByteProgress_ = ulUploadIndexProgress;

   if (*pulByteProgress_ >= 16)                     //take off the extra 8 byte overhead
      *pulByteProgress_ = *pulByteProgress_ - 16;



   return TRUE;
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHostChannel::EraseData(USHORT usDataFileIndex_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);

   if (eANTFSRequest != ANTFS_REQUEST_NONE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::EraseData():  Request Busy.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_BUSY;
   }

   if (eANTFSState != ANTFS_HOST_STATE_TRANSPORT)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::EraseData():  Not in transport state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }

   bTransfer = FALSE;

   usTransferDataFileIndex = usDataFileIndex_;

   eANTFSRequest = ANTFS_REQUEST_ERASE;
   DSIThread_CondSignal(&stCondRequest);

   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return ANTFS_RETURN_PASS;
}

//////////////////////////////////////////////////////////////////////
BOOL ANTFSHostChannel::EnablePing(BOOL bEnable_)
{
   bPingEnabled = bEnable_;
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////
// Private Functions
//////////////////////////////////////////////////////////////////////////////////
DSI_THREAD_RETURN ANTFSHostChannel::ANTFSThreadStart(void *pvParameter_)
{
   #if defined(DEBUG_FILE)
   DSIDebug::ThreadInit("ANTFSHost");
   #endif

   ((ANTFSHostChannel *)pvParameter_)->ANTFSThread();

   return 0;
}
///////////////////////////////////////////////////////////////////////
// ANTFS Task Thread
///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::ANTFSThread(void)
{
   ANTFS_HOST_RESPONSE eResponse;
   BOOL bPingNow = FALSE;
   bANTFSThreadRunning = TRUE;

   while (bKillThread == FALSE)
   {
      eResponse = ANTFS_HOST_RESPONSE_NONE;

      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Awaiting Requests...");
      #endif

      DSIThread_MutexLock(&stMutexCriticalSection);

      if (*pbCancel == TRUE)     //If there was a cancel, then return ANTFS_HOST_RESPONSE_CANCEL_DONE when we've reached this point
      {
         *pbCancel = FALSE;

         if (eANTFSRequest != ANTFS_REQUEST_INIT && eANTFSRequest != ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
         {
            eANTFSRequest = ANTFS_REQUEST_NONE;                    //Clear any other request
         }

         AddResponse(ANTFS_HOST_RESPONSE_CANCEL_DONE);
      }

      if ((eANTFSRequest == ANTFS_REQUEST_NONE) && (bKillThread == FALSE))
      {
         UCHAR ucResult = DSIThread_CondTimedWait(&stCondRequest, &stMutexCriticalSection, 2000);

         if (ucResult != DSI_THREAD_ENONE)
         {
            #if defined(DEBUG_FILE)
               if(ucResult == DSI_THREAD_EOTHER)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread(): CondTimedWait() Failed!");
            #endif

            if ((eANTFSRequest == ANTFS_REQUEST_NONE) && (bPingEnabled == TRUE) && (eANTFSState >= ANTFS_HOST_STATE_CONNECTED))
            {
               bPingNow = TRUE;
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Requesting Ping.");
               #endif
            }
         }
      }

      DSIThread_MutexUnlock(&stMutexCriticalSection);


      if (bPingNow == TRUE)
      {
         bPingNow = FALSE;
         Ping();
      }

      if (bKillThread)
         break;

      if (eANTFSRequest != ANTFS_REQUEST_NONE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Request received");
         #endif

         switch (eANTFSRequest)
         {
            case ANTFS_REQUEST_INIT:
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Idle.");
                  #endif

                  ResetHostState();
                  eANTFSState = ANTFS_HOST_STATE_IDLE;
                  eResponse = ANTFS_HOST_RESPONSE_INIT_PASS;
               }  // ANTFS_REQUEST_INIT
               break;

            case ANTFS_REQUEST_SESSION:
               {
                  RETURN_STATUS eReturn;
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Requesting ANT-FS session...");
                  #endif

                  eANTFSState = ANTFS_HOST_STATE_REQUESTING_SESSION;

                  eReturn = AttemptRequestSession();

                  if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Session request serial error.");
                     #endif
                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Session request failed.");
                     #endif
                     eANTFSState = ANTFS_HOST_STATE_IDLE;
                     eResponse = ANTFS_HOST_RESPONSE_REQUEST_SESSION_FAIL;
                  }
                  else if (eReturn == RETURN_STOP)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Session request stopped.");
                     #endif
                     eANTFSState = ANTFS_HOST_STATE_IDLE;
                  }
                  else if (eReturn == RETURN_PASS)
                  {
                     RETURN_STATUS eConnectStatus;

                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Connecting...");
                     #endif

                     eConnectStatus = AttemptConnect();

                     if (eConnectStatus == RETURN_FAIL)
                     {
                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Connect failed.");
                        #endif
                        eANTFSState = ANTFS_HOST_STATE_IDLE;
                        eResponse = ANTFS_HOST_RESPONSE_REQUEST_SESSION_FAIL;
                     }
                     else if (eConnectStatus == RETURN_SERIAL_ERROR)
                     {
                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Connect rejected.");
                        #endif

                        DSIThread_MutexLock(&stMutexCriticalSection);
                        if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                           eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                        DSIThread_MutexUnlock(&stMutexCriticalSection);
                     }
                     else if (eConnectStatus == RETURN_PASS)
                     {
                        // No need to request the serial number in this case, as
                        // we already know we are connected to the right device
                        eANTFSState = ANTFS_HOST_STATE_CONNECTED;
                        eResponse = ANTFS_HOST_RESPONSE_CONNECT_PASS;
                     }
                     else if (eConnectStatus == RETURN_STOP)
                     {
                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Session request stopped.");
                        #endif
                        eANTFSState = ANTFS_HOST_STATE_IDLE;
                     }
                     else
                     {
                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Session request failed.");
                        #endif

                        eANTFSState = ANTFS_HOST_STATE_IDLE;
                        eResponse = ANTFS_HOST_RESPONSE_REQUEST_SESSION_FAIL;
                     }
                  }
               } // ANTFS_REQUEST_SESSION
               break;

            case ANTFS_REQUEST_SEARCH:
               {
                  RETURN_STATUS eReturn;

                  eANTFSState = ANTFS_HOST_STATE_SEARCHING;

                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Searching...");
                  #endif

                  eReturn = AttemptSearch();

                  if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Search serial error.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Search failed.");
                     #endif
                     //eANTFSRequest should still be ANTFS_REQUEST_SEARCH; // Keep searching.
                     //stay in searching state so we keep retrying
                  }
                  else if (eReturn == RETURN_STOP)
                  {
                     RETURN_STATUS eDisconnectStatus;

                     eDisconnectStatus = AttemptDisconnect();

                     if(eDisconnectStatus == RETURN_PASS)
                     {
                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Search stopped.");
                        #endif

                        eANTFSState = ANTFS_HOST_STATE_IDLE;
                        //eResponse = ANTFS_HOST_RESPONSE_DISCONNECT_PASS;
                     }
                     else if(eDisconnectStatus == RETURN_SERIAL_ERROR)
                     {
                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Serial error stopping search.");
                        #endif

                        DSIThread_MutexLock(&stMutexCriticalSection);
                        if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                           eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                        DSIThread_MutexUnlock(&stMutexCriticalSection);
                     }
                  }
                  else if (eReturn == RETURN_PASS)
                  {
                     RETURN_STATUS eConnectStatus;

                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Connecting...");
                     #endif

                     eConnectStatus = AttemptConnect();

                     if (eConnectStatus == RETURN_FAIL)
                     {
                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Connect failed.");
                        #endif
                        //eANTFSRequest should still be ANTFS_REQUEST_SEARCH; // Keep searching.
                        eANTFSState = ANTFS_HOST_STATE_SEARCHING;
                     }
                     else if (eConnectStatus == RETURN_SERIAL_ERROR)
                     {
                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Connect rejected.");
                        #endif

                        DSIThread_MutexLock(&stMutexCriticalSection);
                        if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                           eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                        DSIThread_MutexUnlock(&stMutexCriticalSection);
                     }
                     else
                     {
                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Requesting ID...");
                        #endif

                        //eANTFSState = ANTFS_HOST_STATE_REQUESTING_ID;
                        //We do not want to set the state to connected until after we have sucessfully retrived the serial number
                        ucTxPasswordLength = 0;
                        ucAuthType = AUTH_COMMAND_REQ_SERIAL_NUM;
                        pucResponseBuffer = aucRemoteFriendlyName;
                        ucRemoteFriendlyNameLength = sizeof(aucRemoteFriendlyName);
                        pucResponseBufferSize = &ucRemoteFriendlyNameLength;
                        ulAuthResponseTimeout = cfgParams.ul_cfg_auth_timeout;

                        eReturn = AttemptAuthenticate();

                        if ((eReturn == RETURN_PASS) || (eReturn == RETURN_NA))
                        {
                           #if defined(DEBUG_FILE)
                              DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Connected.");
                           #endif
                           //grab and save the remote serial number
                           stFoundDeviceParameters.ulDeviceID = Convert_Bytes_To_ULONG(aucTransferBufferFixed[AUTH_REMOTE_SERIAL_NUMBER_OFFSET + 3],
                                                                                       aucTransferBufferFixed[AUTH_REMOTE_SERIAL_NUMBER_OFFSET + 2],
                                                                                       aucTransferBufferFixed[AUTH_REMOTE_SERIAL_NUMBER_OFFSET + 1],
                                                                                       aucTransferBufferFixed[AUTH_REMOTE_SERIAL_NUMBER_OFFSET]);


                           if (IsDeviceMatched(&stFoundDeviceParameters, FALSE) == TRUE)
                           {
                              eANTFSState = ANTFS_HOST_STATE_CONNECTED;
                              eResponse = ANTFS_HOST_RESPONSE_CONNECT_PASS;
                           }
                           else
                           {
                              //possibly blackout the device for a period of time here.
                       #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Full ID is not a match.");
                       #endif
                              RETURN_STATUS eDisconnectStatus;

                              eDisconnectStatus = AttemptDisconnect();

                              if(eDisconnectStatus == RETURN_PASS)
                              {
                                 eANTFSState = ANTFS_HOST_STATE_IDLE;
                              }
                              else if(eDisconnectStatus == RETURN_SERIAL_ERROR)
                              {
                                 #if defined(DEBUG_FILE)
                                    DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Serial error stopping search.");
                                 #endif

                                 DSIThread_MutexLock(&stMutexCriticalSection);
                                 if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                                    eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                                 DSIThread_MutexUnlock(&stMutexCriticalSection);
                              }
                           }
                        }
                        else if (eReturn == RETURN_STOP)
                        {
                           RETURN_STATUS eDisconnectStatus;
                           #if defined(DEBUG_FILE)
                              DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Search stopped.");
                           #endif
                           eDisconnectStatus = AttemptDisconnect();

                           if(eDisconnectStatus == RETURN_PASS)
                           {
                              eANTFSState = ANTFS_HOST_STATE_IDLE;
                           //eResponse = ANTFS_HOST_RESPONSE_DISCONNECT_PASS;
                           }
                           else if(eDisconnectStatus == RETURN_SERIAL_ERROR)
                           {
                              #if defined(DEBUG_FILE)
                                 DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Serial error getting ID.");
                              #endif

                              DSIThread_MutexLock(&stMutexCriticalSection);
                              if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                                 eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                              DSIThread_MutexUnlock(&stMutexCriticalSection);
                           }
                        }
                        else
                        {
                           RETURN_STATUS eDisconnectStatus;

                           #if defined(DEBUG_FILE)
                              DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Get ID failed.");
                           #endif

                           eDisconnectStatus = AttemptDisconnect();

                           if(eDisconnectStatus == RETURN_PASS)
                           {
                              // Keep searching.
                              eANTFSState = ANTFS_HOST_STATE_SEARCHING;
                           }
                           else if(eDisconnectStatus == RETURN_SERIAL_ERROR)
                           {
                              #if defined(DEBUG_FILE)
                                 DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Serial error getting ID.");
                              #endif

                              DSIThread_MutexLock(&stMutexCriticalSection);
                              if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                                 eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                              DSIThread_MutexUnlock(&stMutexCriticalSection);
                           }
                        }
                     }
                  }
               }
               break;

            case ANTFS_REQUEST_AUTHENTICATE:
               {
                  RETURN_STATUS eReturn;

                  eANTFSState = ANTFS_HOST_STATE_AUTHENTICATING;
                  eResponse = ANTFS_HOST_RESPONSE_AUTHENTICATE_FAIL; //set a default response

                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Authenticating...");
                  #endif

                  eReturn = AttemptAuthenticate();

                  pucResponseBuffer = (UCHAR*)NULL;  //clear response buffer pointers
                  pucResponseBufferSize = (UCHAR*)NULL;

                  if (eReturn == RETURN_PASS)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Authentication passed.");
                     #endif

                     eANTFSState = ANTFS_HOST_STATE_TRANSPORT;
                     eResponse = ANTFS_HOST_RESPONSE_AUTHENTICATE_PASS;
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Authentication failed.");
                     #endif
                     IncFreqStaleCount(MAJOR_STALE_COUNT);
                     eANTFSState = ANTFS_HOST_STATE_CONNECTED;
                     eResponse = ANTFS_HOST_RESPONSE_AUTHENTICATE_FAIL;
                  }
                  else if (eReturn == RETURN_REJECT)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Authentication rejected.");
                     #endif

                     eANTFSState = ANTFS_HOST_STATE_CONNECTED;
                     eResponse = ANTFS_HOST_RESPONSE_AUTHENTICATE_REJECT;
                  }
                  else if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Authentication serial error.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
                  else if (eReturn == RETURN_STOP)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Authentication stopped.");
                     #endif

                     eANTFSState = ANTFS_HOST_STATE_CONNECTED;
                     //eResponse = ANTFS_HOST_RESPONSE_DISCONNECT_PASS);
                  }
                  else //RETURN_NA
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Authentication NA.");
                     #endif

                     eANTFSState = ANTFS_HOST_STATE_CONNECTED;
                     eResponse = ANTFS_HOST_RESPONSE_AUTHENTICATE_NA;
                  }
               }
               break;

            case ANTFS_REQUEST_DISCONNECT:
               {
                  RETURN_STATUS eReturn;
                  eANTFSState = ANTFS_HOST_STATE_DISCONNECTING;

                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Disconnecting...");
                  #endif

                  eReturn = AttemptDisconnect();

                  if(eReturn == RETURN_PASS)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Disconnect passed.");
                     #endif

                     ResetHostState();
                     eANTFSState = ANTFS_HOST_STATE_IDLE;
                     eResponse = ANTFS_HOST_RESPONSE_DISCONNECT_PASS;
                  }
                  else if(eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Disconnect serial error.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
               }
               break;

            case ANTFS_REQUEST_CHANGE_LINK:
               {
                  RETURN_STATUS eReturn;

                  #if defined(DEBUG_FILE)
                      DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Changing radio frequency and channel period...");
                  #endif

                  eReturn = AttemptSwitchFrequency();
                  if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread(): Serial error while changing radio frequency/period.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     RETURN_STATUS eDisconnectStatus;
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Changing radio frequency and channel period failed.");
                     #endif

                     // We end up here if we lost the beacon when trying to change the channel parameters, so
                     // disconnect, so we can try to connect to it again
                     eDisconnectStatus = AttemptDisconnect();

                     if(eDisconnectStatus == RETURN_PASS)
                     {
                        eANTFSState = ANTFS_HOST_STATE_IDLE;
                        eResponse = ANTFS_HOST_RESPONSE_CONNECTION_LOST;
                     }
                     else if(eDisconnectStatus == RETURN_SERIAL_ERROR)
                     {
                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Serial error changing channel parameters.");
                        #endif

                        DSIThread_MutexLock(&stMutexCriticalSection);
                        if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                           eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                        DSIThread_MutexUnlock(&stMutexCriticalSection);
                     }
                  }
                  else if (eReturn == RETURN_PASS)
                  {
                     eANTFSRequest = ANTFS_REQUEST_NONE;
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Changed radio frequency and channel period.");
                     #endif
                     // TODO: Do we need a response as well?
                  }
               }   // ANTFS_REQUEST_CHANGE_LINK
               break;

            case ANTFS_REQUEST_DOWNLOAD:
               {
                  RETURN_STATUS eReturn;

                  eANTFSState = ANTFS_HOST_STATE_DOWNLOADING;
                  eResponse = ANTFS_HOST_RESPONSE_DOWNLOAD_FAIL;  //Set a default response

                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Downloading...");
                  #endif

                  eReturn = AttemptDownload();

                  if (eReturn == RETURN_PASS)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Download complete.");
                     #endif

                     eANTFSState = ANTFS_HOST_STATE_TRANSPORT;
                     eResponse = ANTFS_HOST_RESPONSE_DOWNLOAD_PASS;
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     RETURN_STATUS eDisconnectStatus;

                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Download failed.");
                     #endif
                     IncFreqStaleCount(MINOR_STALE_COUNT);

                     eDisconnectStatus = AttemptDisconnect();

                     if(eDisconnectStatus == RETURN_PASS)
                     {
                        eANTFSState = ANTFS_HOST_STATE_IDLE;
                        eResponse = ANTFS_HOST_RESPONSE_DOWNLOAD_FAIL;
                     }
                     else if(eDisconnectStatus == RETURN_SERIAL_ERROR)
                     {
                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Serial error on failed download.");
                        #endif

                        DSIThread_MutexLock(&stMutexCriticalSection);
                        if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                           eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                        DSIThread_MutexUnlock(&stMutexCriticalSection);
                     }
                  }
                  else if (eReturn == RETURN_REJECT)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Download rejected.");
                     #endif
                     eANTFSState = ANTFS_HOST_STATE_TRANSPORT;

                     switch (ucRejectCode)
                     {
                        case DOWNLOAD_RESPONSE_DOES_NOT_EXIST:
                           eResponse = ANTFS_HOST_RESPONSE_DOWNLOAD_INVALID_INDEX;
                           break;
                        case DOWNLOAD_RESPONSE_NOT_DOWNLOADABLE:
                           eResponse = ANTFS_HOST_RESPONSE_DOWNLOAD_FILE_NOT_READABLE;
                           break;
                        case DOWNLOAD_RESPONSE_NOT_READY:
                           eResponse = ANTFS_HOST_RESPONSE_DOWNLOAD_NOT_READY;
                           break;
                        case DOWNLOAD_RESPONSE_CRC_FAILED:
                           eResponse = ANTFS_HOST_RESPONSE_DOWNLOAD_CRC_REJECTED;
                           break;
                        case DOWNLOAD_RESPONSE_REQUEST_INVALID:
                        default:
                           eResponse = ANTFS_HOST_RESPONSE_DOWNLOAD_REJECT;
                           break;
                     }
                  }
                  else if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Download serial error.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);

                  }
                  else
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Download stopped.");
                     #endif

                     //Disconnect();
                     //eANTFSState = ANTFS_HOST_STATE_IDLE;
                     //eResponse = ANTFS_HOST_RESPONSE_DISCONNECT_PASS;
                     eANTFSState = ANTFS_HOST_STATE_TRANSPORT;
                  }
               }
               break;

            case ANTFS_REQUEST_UPLOAD:
               {
                  RETURN_STATUS eReturn;

                  eANTFSState = ANTFS_HOST_STATE_UPLOADING;
                  eResponse = ANTFS_HOST_RESPONSE_UPLOAD_FAIL;  //Set a default response

                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread(): Requesting upload.");
                  #endif

                  eReturn = UploadLoop();

                  if (eReturn == RETURN_PASS)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Upload complete.");
                     #endif

                     eANTFSState = ANTFS_HOST_STATE_TRANSPORT;
                     eResponse = ANTFS_HOST_RESPONSE_UPLOAD_PASS;
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     RETURN_STATUS eDisconnectStatus;

                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Upload failed.");
                     #endif
                     IncFreqStaleCount(MINOR_STALE_COUNT);

                     eDisconnectStatus = AttemptDisconnect();

                     if(eDisconnectStatus == RETURN_PASS)
                     {
                        eANTFSState = ANTFS_HOST_STATE_IDLE;
                        eResponse = ANTFS_HOST_RESPONSE_UPLOAD_FAIL;
                     }
                     else if(eDisconnectStatus == RETURN_SERIAL_ERROR)
                     {
                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Serial error on failed upload.");
                        #endif

                        DSIThread_MutexLock(&stMutexCriticalSection);
                        if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                           eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                        DSIThread_MutexUnlock(&stMutexCriticalSection);
                     }
                  }
                  else if (eReturn == RETURN_REJECT)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Upload rejected.");
                     #endif
                     eANTFSState = ANTFS_HOST_STATE_TRANSPORT;

                     switch (ucRejectCode)
                     {
                        case UPLOAD_RESPONSE_DOES_NOT_EXIST:
                           eResponse = ANTFS_HOST_RESPONSE_UPLOAD_INVALID_INDEX;
                           break;
                        case UPLOAD_RESPONSE_NOT_WRITEABLE:
                           eResponse = ANTFS_HOST_RESPONSE_UPLOAD_FILE_NOT_WRITEABLE;
                           break;
                        case UPLOAD_RESPONSE_INSUFFICIENT_SPACE:
                           eResponse = ANTFS_HOST_RESPONSE_UPLOAD_INSUFFICIENT_SPACE;
                           break;
                        default:
                           eResponse = ANTFS_HOST_RESPONSE_UPLOAD_REJECT;
                           break;
                     }
                  }
                  else if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Upload serial error.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);

                  }
                  else
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Upload stopped.");
                     #endif

                     //Disconnect();
                     //eANTFSState = ANTFS_HOST_STATE_IDLE;
                     //eResponse = ANTFS_HOST_RESPONSE_DISCONNECT_PASS;
                     eANTFSState = ANTFS_HOST_STATE_TRANSPORT;
                  }
               }
               break;

            case ANTFS_REQUEST_MANUAL_TRANSFER:
               {
                  RETURN_STATUS eReturn;

                  eANTFSState = ANTFS_HOST_STATE_SENDING;

                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread(): Sending Direct.");
                  #endif

                  eReturn = AttemptManualTransfer();

                  if (eReturn == RETURN_PASS)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  ManualTransfer complete.");
                     #endif

                     eANTFSState = ANTFS_HOST_STATE_TRANSPORT;
                     eResponse = ANTFS_HOST_RESPONSE_MANUAL_TRANSFER_PASS;
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     RETURN_STATUS eDisconnectStatus;

                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  ManualTransfer failed to retrieve data.");
                     #endif
                     IncFreqStaleCount(MINOR_STALE_COUNT);

                     eDisconnectStatus = AttemptDisconnect();

                     if(eDisconnectStatus == RETURN_PASS)
                     {
                        eANTFSState = ANTFS_HOST_STATE_IDLE;
                        eResponse = ANTFS_HOST_RESPONSE_MANUAL_TRANSFER_RESPONSE_FAIL;
                     }
                     else if(eDisconnectStatus == RETURN_SERIAL_ERROR)
                     {
                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Serial error on failed manual transfer.");
                        #endif

                        DSIThread_MutexLock(&stMutexCriticalSection);
                        if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                           eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                        DSIThread_MutexUnlock(&stMutexCriticalSection);
                     }
                  }
                  else if (eReturn == RETURN_REJECT)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  ManualTransfer rejected due to Tx error.");
                     #endif
                     IncFreqStaleCount(MIN_STALE_COUNT);  //Have to check if a NAK on the Garmin protocol will cause this condition
                     eANTFSState = ANTFS_HOST_STATE_TRANSPORT;
                     eResponse = ANTFS_HOST_RESPONSE_MANUAL_TRANSFER_TRANSMIT_FAIL;
                  }
                  else if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  ManualTransfer serial error.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);

                  }
                  else
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  ManualTransfer stopped.");
                     #endif

                     //Disconnect();
                     //eANTFSState = ANTFS_HOST_STATE_IDLE;
                     //eResponse = ANTFS_HOST_RESPONSE_DISCONNECT_PASS;
                     eANTFSState = ANTFS_HOST_STATE_TRANSPORT;
                  }
               }
               break;

            case ANTFS_REQUEST_ERASE:
               {
                  RETURN_STATUS eReturn;

                  eANTFSState = ANTFS_HOST_STATE_ERASING;
                  eResponse = ANTFS_HOST_RESPONSE_ERASE_FAIL;  //Set default response

                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Erasing...");
                  #endif

                  eReturn = AttemptErase();

                  if (eReturn == RETURN_PASS)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Erase complete.");
                     #endif

                     eANTFSState = ANTFS_HOST_STATE_TRANSPORT;
                     eResponse = ANTFS_HOST_RESPONSE_ERASE_PASS;
                  }
                  else if (eReturn == RETURN_FAIL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Erase fail.");
                     #endif
                     IncFreqStaleCount(MINOR_STALE_COUNT);
                     eANTFSState = ANTFS_HOST_STATE_TRANSPORT;
                     eResponse = ANTFS_HOST_RESPONSE_ERASE_FAIL;
                  }
                  else if (eReturn == RETURN_SERIAL_ERROR)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Erase serial error.");
                     #endif

                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
                        eANTFSRequest = ANTFS_REQUEST_HANDLE_SERIAL_ERROR;
                     DSIThread_MutexUnlock(&stMutexCriticalSection);

                  }
                  else
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Erase stopped.");
                     #endif

                     //Disconnect();
                     //eANTFSState = ANTFS_HOST_STATE_IDLE;
                     //eResponse = ANTFS_HOST_RESPONSE_DISCONNECT_PASS;
                     eANTFSState = ANTFS_HOST_STATE_TRANSPORT;
                  }
               }
               break;

            default:
               break;
         }

         //This is where to handle the internal requests, because they can happen asyncronously.
         //We will also clear the request here.
         DSIThread_MutexLock(&stMutexCriticalSection);

         if (eResponse != ANTFS_HOST_RESPONSE_NONE)
            AddResponse(eResponse);

         if (eANTFSRequest == ANTFS_REQUEST_CONNECTION_LOST)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Connection lost.");
            #endif

            eANTFSRequest = ANTFS_REQUEST_NONE;
            bFoundDevice = FALSE;

            if (eANTFSState >= ANTFS_HOST_STATE_CONNECTED)
            {
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Connection lost - ignored.");
               #endif

               ResetHostState();
               eANTFSState = ANTFS_HOST_STATE_IDLE;
               AddResponse(ANTFS_HOST_RESPONSE_CONNECTION_LOST);
            }
         }
         else if (eANTFSRequest == ANTFS_REQUEST_HANDLE_SERIAL_ERROR)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Serial error!");
            #endif
            HandleSerialError();
            AddResponse(ANTFS_HOST_RESPONSE_SERIAL_FAIL);
         }
         else if (eANTFSRequest == ANTFS_REQUEST_SERIAL_ERROR_HANDLED)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Serial error handled");
            #endif
            ResetHostState();
            eANTFSState = ANTFS_HOST_STATE_IDLE;
            eANTFSRequest = ANTFS_REQUEST_INIT;
         }
         else
         {
            if (eANTFSState == ANTFS_HOST_STATE_SEARCHING)
               eANTFSRequest = ANTFS_REQUEST_SEARCH;                  //Set the search request if we're still in search mode
            else
               eANTFSRequest = ANTFS_REQUEST_NONE;                    //Clear any other request
         }

         DSIThread_MutexUnlock(&stMutexCriticalSection);
      }

   }//while()

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  Exiting thread.");
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
         DSIDebug::ThreadWrite("ANTFSHostChannel::ANTFSThread():  C code reaching return statement unexpectedly.");
      #endif
      return;                                            // Code should not be reached.
   #endif
}

/////////////////////////////////////////////////////////////////////
// Returns: TRUE if the message is for the ANT-FS channel
BOOL ANTFSHostChannel::FilterANTMessages(ANT_MESSAGE* pstMessage_, UCHAR ucANTChannel_)
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
BOOL ANTFSHostChannel::ReportDownloadProgress(void)
{
// We used to perodically signal our transfer progress, we dont do that anymore.
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::ResetHostState(void)
{
   // Clear all state variables, while keeping the configuration:
   // channel parameters, search list, ignore list, frequency table selection

   *pbCancel = FALSE;
   bForceFullInit = FALSE;

   memset(aucResponseBuf, 0, sizeof(aucResponseBuf));
   memset(aucRxBuf, 0, sizeof(aucRxBuf));
   memset(aucTxBuf, 0, sizeof(aucTxBuf));

   memset(aucRemoteFriendlyName, 0, sizeof(aucRemoteFriendlyName));
   ucRemoteFriendlyNameLength = 0;

   ucAuthType = 0;
   memset(aucTxPassword, 0, sizeof(aucTxPassword));
   ucTxPasswordLength = 0;
   ucPasswordLength = 0;

   ucDisconnectType = DISCONNECT_COMMAND_LINK;
   ucUndiscoverableTimeDuration = 0;
   ucUndiscoverableAppSpecificDuration = 0;

   pucUploadData = (UCHAR*)NULL;
   pucResponseBuffer = (UCHAR*)NULL;
   pucResponseBufferSize = (UCHAR*)NULL;

   ulAuthResponseTimeout = cfgParams.ul_cfg_auth_timeout;   // reset to explicit configuration from SetCurrentConfig

   ucChannelStatus = 0;

   ulFoundBeaconHostID = 0;

   usFoundBeaconPeriod = 0;
   usFoundANTFSDeviceID = 0;
   usFoundANTFSManufacturerID = 0;
   usFoundANTFSDeviceType = 0;
   ucFoundANTDeviceType = 0;
   ucFoundANTTransmitType = 0;

   bFoundDeviceHasData = FALSE;
   bFoundDeviceUploadEnabled = FALSE;
   bFoundDeviceInPairingMode = FALSE;
   ucFoundDeviceAuthenticationType = 0;
   ucFoundDeviceState = REMOTE_DEVICE_BEACON_NOT_FOUND;
   bFoundDevice = FALSE;
   bFoundDeviceIsValid = FALSE;
   bNewRxEvent = FALSE;

   stFoundDeviceParameters.usDeviceType =  0;
   stFoundDeviceParameters.usManufacturerID =  0;
   stFoundDeviceParameters.ucAuthenticationType = 0;
   stFoundDeviceParameters.ucStatusByte1 = 0;
   stFoundDeviceParameters.ucStatusByte2 = 0;

   // Download Data
   pucTransferBuffer = (UCHAR*)NULL;
   memset(aucTransferBufferFixed, 0, sizeof(aucTransferBufferFixed));
   memset(aucSendDirectBuffer, 0, sizeof(aucSendDirectBuffer));
   ulTransferArrayIndex = 0;
   ulPacketCount = 0;
   ulUploadIndexProgress = 0;
   bTxError = FALSE;
   bRxError = FALSE;
   bReceivedBurst = FALSE;
   bReceivedResponse = FALSE;
   ulTransferTotalBytesRemaining = 0;
   ulTransferBytesInBlock = 0;
   bTransfer = FALSE;

   usRadioChannelID = 0;

   ucTransportFrequencySelection = ANTFS_AUTO_FREQUENCY_SELECTION;
   ucTransportLayerRadioFrequency = 0;
   ucSearchRadioFrequency = ANTFS_RF_FREQ;
   usTransferDataFileIndex = 0;
   ulTransferDataOffset = 0;
   ulTransferByteSize = 0;
   ulTransferBufferSize = 0;
   bLargeData = FALSE;

   ulHostBlockSize = MAX_ULONG;

   ucLinkResponseRetries = 0;

   ucStrikes = STRIKE_COUNT;

   if(eANTFSState == ANTFS_HOST_STATE_OFF)
   {
      pucTransferBufferDynamic = (UCHAR*) NULL;
   }
   else
   {
      eANTFSState = ANTFS_HOST_STATE_IDLE;

      if (pucTransferBufferDynamic)
      {
         if (pucTransferBuffer == pucTransferBufferDynamic)
            pucTransferBuffer = (UCHAR*)NULL;

         delete[] pucTransferBufferDynamic;
         pucTransferBufferDynamic = (UCHAR*)NULL;
      }
   }

   eANTFSRequest = ANTFS_REQUEST_NONE;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHostChannel::ReInitDevice(void)
{
   if (eANTFSState != ANTFS_HOST_STATE_OFF)
      this->Close();

   bKillThread = FALSE;

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHostChannel::ReInitDevice(): Initializing");
   #endif

   if (hANTFSThread == NULL)
   {
      hANTFSThread = DSIThread_CreateThread(&ANTFSHostChannel::ANTFSThreadStart, this);
      if (hANTFSThread == NULL)
         return FALSE;
   }

   if (!bIgnoreListRunning)
   {
      if (DSIThread_MutexInit(&stMutexIgnoreListAccess) != DSI_THREAD_ENONE)
      {
         return FALSE;
      }

      usListIndex = 0;
      bTimerThreadInitDone = FALSE;
      pclQueueTimer = new DSITimer(&ANTFSHostChannel::QueueTimerStart, this, 1000, TRUE);
      if (pclQueueTimer->NoError() == FALSE)
      {
         DSIThread_MutexDestroy(&stMutexIgnoreListAccess);
         return FALSE;
      }
      bIgnoreListRunning = TRUE;
   }

   PopulateTransportFreqTable();

   DSIThread_MutexLock(&stMutexResponseQueue);
      clResponseQueue.Clear();
   DSIThread_MutexUnlock(&stMutexResponseQueue);

   DSIThread_MutexLock(&stMutexCriticalSection);
      eANTFSRequest = ANTFS_REQUEST_INIT;
      DSIThread_CondSignal(&stCondRequest);
   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return TRUE;
}


///////////////////////////////////////////////////////////////////////
ANTFSHostChannel::RETURN_STATUS ANTFSHostChannel::AttemptSearch(void)
{
   IGNORE_LIST_ITEM stListItem;
   IGNORE_LIST_ITEM *pstListItem;
   BOOL bFullInit = TRUE;
   BOOL bFoundBroadcastDevice = FALSE;
   UCHAR ucFirstMesgRetries;
   BOOL bFirstMesgResult;

   while (eANTFSState == ANTFS_HOST_STATE_SEARCHING)
   {
      //if (!bFullInit)
      {
         if(pclANT->GetChannelStatus(ucChannelNumber, &ucChannelStatus, MESSAGE_TIMEOUT) == FALSE)
          {
            #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptOpenBeacon():  Failed ANT_GetChannelStatus().");
               #endif
            return RETURN_SERIAL_ERROR;
         }

         if ((ucChannelStatus & STATUS_CHANNEL_STATE_MASK) == STATUS_TRACKING_CHANNEL ||
            (ucChannelStatus & STATUS_CHANNEL_STATE_MASK) == STATUS_SEARCHING_CHANNEL)
         {
            if (pclANT->CloseChannel(ucChannelNumber, ANT_CLOSE_TIMEOUT) == FALSE)
               return RETURN_SERIAL_ERROR;
            //TODO?? Don't we have to wait for close event? Wouldn't unassign call below fail with wrong state?

            if(bFullInit || bForceFullInit)
            {
               if(pclANT->UnAssignChannel(ucChannelNumber) == FALSE)
               return RETURN_SERIAL_ERROR;

               bFullInit = TRUE;
            }

         }
         else if ((ucChannelStatus & STATUS_CHANNEL_STATE_MASK) == STATUS_UNASSIGNED_CHANNEL)
         {
            bFullInit = TRUE;
         }
         else if(bFullInit) // Status = ASSIGNED, we only need to unassign it for full configuration
         {
            if(pclANT->UnAssignChannel(ucChannelNumber) == FALSE)
               return RETURN_SERIAL_ERROR;

            bFullInit = TRUE;
         }

      }

      if (bFullInit)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Full Init Begin");
         #endif

         //if (pclANT->ResetSystem() == FALSE)
         //   return RETURN_SERIAL_ERROR;
         //DSIThread_Sleep(1000);

         ucFirstMesgRetries = 0;
         while (((bFirstMesgResult = pclANT->SetNetworkKey(ucNetworkNumber, (UCHAR *) aucTheNetworkkey,MESSAGE_TIMEOUT)) == FALSE) && (ucFirstMesgRetries++ < ANTFS_RESPONSE_RETRIES));
         #if defined(DEBUG_FILE)
            if (ucFirstMesgRetries)
            {
               UCHAR aucString[256];
               SNPRINTF((char*)&aucString[0], 256, "ANTFSHostChannel::AttemptSearch():  %d message retries on first message.", ucFirstMesgRetries);
               DSIDebug::ThreadWrite((char*)aucString);
            }
         #endif
         if (!bFirstMesgResult)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Failed ANT_SetNetworkKey().");
            #endif
            return RETURN_SERIAL_ERROR;
         }

         if (pclANT->AssignChannel(ucChannelNumber, 0x00, ucNetworkNumber, MESSAGE_TIMEOUT) == FALSE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Failed ANT_AssignChannel().");
            #endif
            return RETURN_SERIAL_ERROR;
         }

         if (pclANT->SetChannelPeriod(ucChannelNumber, usTheMessagePeriod, MESSAGE_TIMEOUT) == FALSE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Failed ANT_SetChannelPeriod().");
            #endif
            return RETURN_SERIAL_ERROR;
         }

         if (pclANT->SetChannelSearchTimeout(ucChannelNumber, ANTFS_SEARCH_TIMEOUT, MESSAGE_TIMEOUT) == FALSE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Failed ANT_SetChannelSearchTimeout().");
            #endif
            return RETURN_SERIAL_ERROR;
         }

         if (pclANT->SetChannelRFFrequency(ucChannelNumber, ucSearchRadioFrequency, MESSAGE_TIMEOUT) == FALSE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Failed ANT_SetChannelRFFreq().");
            #endif
            return RETURN_SERIAL_ERROR;
         }
         if (pclANT->SetFastSearch(ucChannelNumber, MESSAGE_TIMEOUT) == FALSE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Failed SetFastSearch().");
            #endif
            return RETURN_SERIAL_ERROR;
         }

         bForceFullInit = FALSE;
         bFullInit = FALSE;
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Full Init Complete");
         #endif
      }

      //This gets reset everytime a device is found, so we need to set it every time
      if(ucTheProxThreshold > 0)    //Don't need to send if it is default=0, saves sending command on unsupported devices, plus a little time
      {
         if(pclANT->SetProximitySearch(ucChannelNumber, ucTheProxThreshold, MESSAGE_TIMEOUT) == FALSE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Failed ANT_SetProximitySearch(). Ignored, possibly unsupported device.");
            #endif
            //don't return RETURN_SERIAL_ERROR, Ignore failure so we don't fail on unsupported devices and maintain compatibility.
         }
      }

      //This gets reset everytime a device is found, so we need to set it every time
      if (pclANT->SetChannelID(ucChannelNumber, usRadioChannelID, ucTheDeviceType, ucTheTransmissionType, MESSAGE_TIMEOUT) == FALSE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Failed ANT_SetChannelId().");
         #endif
         return RETURN_SERIAL_ERROR;
      }

      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Opening channel...");
      #endif
      bFoundDevice = FALSE;
      bFoundBroadcastDevice = FALSE;


      if (pclANT->OpenChannel(ucChannelNumber,MESSAGE_TIMEOUT) == FALSE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Failed ANT_OpenChannel().");
         #endif
         return RETURN_SERIAL_ERROR;
      }


      bNewRxEvent = FALSE;

      do
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Waiting for search results...");
         #endif

         if (pclANT->GetChannelStatus(ucChannelNumber, &ucChannelStatus, MESSAGE_TIMEOUT) == FALSE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Cannot get ANT Channel Status.");
            #endif
            return RETURN_FAIL;
            //return RETURN_SERIAL_ERROR;
         }

         if (ucChannelStatus < STATUS_SEARCHING_CHANNEL)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  ANT Channel not searching or tracking.");
            #endif
            return RETURN_FAIL;
         }


         DSIThread_MutexLock(&stMutexCriticalSection);

         if ((bNewRxEvent == FALSE) && (*pbCancel == FALSE))
         {
            DSIThread_CondTimedWait(&stCondRxEvent, &stMutexCriticalSection, SEARCH_STATUS_CHECK_TIMEOUT);
         }
         bNewRxEvent = FALSE;
         DSIThread_MutexUnlock(&stMutexCriticalSection);

         if (*pbCancel == TRUE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Searching stopped.");
            #endif
            return RETURN_STOP;
         }

      } while (!bFoundDevice);

      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Search results loop exitted.");
      #endif


      if (!bFoundDeviceIsValid)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch(): Device not sending a valid beacon.");
         #endif

         if(!bRequestPageEnabled)
            continue;

         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch(): Attempting to request an ANT-FS session...");
         #endif

         if(AttemptRequestSession() != RETURN_PASS)
            continue;

         bFoundBroadcastDevice = TRUE;
      }

      if (ucFoundDeviceState != REMOTE_DEVICE_STATE_LINK)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Device not in link mode.");
         #endif
         continue;
      }

      if (pclANT->GetChannelID(ucChannelNumber, (USHORT*)&usFoundANTFSDeviceID, (UCHAR*)&ucFoundANTDeviceType, (UCHAR*)&ucFoundANTTransmitType, MESSAGE_TIMEOUT) == FALSE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Cannot get ANT ID.");
         #endif
         continue;
      }

      // Save 2 bytes of the DeviceID
      stFoundDeviceParameters.ulDeviceID &= 0xFFFF0000;
      stFoundDeviceParameters.ulDeviceID |= (0x0000FFFF&usFoundANTFSDeviceID);

      // Check the Ignore list
      stListItem.usID = usFoundANTFSDeviceID;
      stListItem.usManufacturerID = usFoundANTFSManufacturerID;
      stListItem.usDeviceType = usFoundANTFSDeviceType;

      DSIThread_MutexLock(&stMutexIgnoreListAccess);

      pstListItem = (IGNORE_LIST_ITEM *) bsearch(&stListItem, astIgnoreList, usListIndex, sizeof(IGNORE_LIST_ITEM), &ListCompare);

      DSIThread_MutexUnlock(&stMutexIgnoreListAccess);

      if (pstListItem != NULL)
      {
         #if defined(DEBUG_FILE)
            UCHAR aucString[256];
            SNPRINTF((char *)&aucString[0], 256, "ANTFSHostChannel::AttemptSearch():  Device %u is on the ignore list.", usFoundANTFSDeviceID);
            DSIDebug::ThreadWrite((char *)aucString);
         #endif
         if(bFoundBroadcastDevice)
         {
            ucDisconnectType = DISCONNECT_COMMAND_BROADCAST;   // Device is in ignore list, let it go back to broadcast
            if(AttemptDisconnect() == RETURN_SERIAL_ERROR)
               return RETURN_SERIAL_ERROR;
            eANTFSState = ANTFS_HOST_STATE_SEARCHING;
         }
         continue;                                          // Continue searching if the device is on the ignore list.
      }

      #if defined(DEBUG_FILE)
         {
            UCHAR aucString[256];

            SNPRINTF((char *)aucString,256, "ANTFSHostChannel::AttemptSearch():  Found device %u (Manufacturer ID: %u  Device Type: %u  Beacon Period: %u/32768  Authentication Type: %u).", usFoundANTFSDeviceID, usFoundANTFSManufacturerID, usFoundANTFSDeviceType, usFoundBeaconPeriod, ucFoundDeviceAuthenticationType);
            DSIDebug::ThreadWrite((char *)aucString);

            if (bFoundDeviceHasData)
               DSIDebug::ThreadWrite("   Device has data.");
            else
               DSIDebug::ThreadWrite("   Device has no data.");

            if (bFoundDeviceUploadEnabled)
               DSIDebug::ThreadWrite("   Device has upload enabled.");
            else
               DSIDebug::ThreadWrite("   Device has upload disabled.");

            if (bFoundDeviceInPairingMode)
               DSIDebug::ThreadWrite("   Device is in pairing mode.");
            else
               DSIDebug::ThreadWrite("   Device is not in pairing mode.");
         }
      #endif

      // Check for a match.
      if (IsDeviceMatched(&stFoundDeviceParameters, TRUE) == TRUE)
      {
         // We have found a match.
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("   Device is a match.");
         #endif

         return RETURN_PASS;
      }

      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("   Device is not a match.");
      #endif
      // No match; continue searching.
      if(bFoundBroadcastDevice)
      {
         ucDisconnectType = DISCONNECT_COMMAND_BROADCAST;   // Device is in ignore list, let it go back to broadcast
         if(AttemptDisconnect() == RETURN_SERIAL_ERROR)
            return RETURN_SERIAL_ERROR;
         eANTFSState = ANTFS_HOST_STATE_SEARCHING;
      }
   }

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSearch():  Not searching anymore.");
   #endif

   return RETURN_FAIL;
}

///////////////////////////////////////////////////////////////////////
ANTFSHostChannel::RETURN_STATUS ANTFSHostChannel::AttemptConnect(void)
{
   UCHAR ucTxRetries;
   ANTFRAMER_RETURN eTxComplete;

   UCHAR ucRxRetries = 3;
   BOOL bStatus = FALSE;

   if (!bFoundDevice)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptConnect():  Device not found.");
      #endif

      return RETURN_FAIL;
   }

   if (ucTransportFrequencySelection == ANTFS_AUTO_FREQUENCY_SELECTION)
      ucTransportLayerRadioFrequency = CheckForNewTransportFreq();
   else
      ucTransportLayerRadioFrequency = ucTransportFrequencySelection;

   // Set up the command
   memset(aucTxBuf,0x00,sizeof(aucTxBuf));
   aucTxBuf[ANTFS_CONNECTION_OFFSET] = ANTFS_COMMAND_RESPONSE_ID;
   aucTxBuf[ANTFS_COMMAND_OFFSET] = ANTFS_CONNECT_ID;
   aucTxBuf[TRANSPORT_CHANNEL_FREQ_OFFSET] = ucTransportLayerRadioFrequency;
   aucTxBuf[TRANSPORT_CHANNEL_PERIOD] = TRANSPORT_MESSAGE_PERIOD_CODE;

   Convert_ULONG_To_Bytes(ulHostSerialNumber,
                    &aucTxBuf[HOST_ID_OFFSET+3],
                    &aucTxBuf[HOST_ID_OFFSET+2],
                    &aucTxBuf[HOST_ID_OFFSET+1],
                    &aucTxBuf[HOST_ID_OFFSET]);

#if defined(ACCESS_POINT)
   ucTxRetries = 2;
#else
   ucTxRetries = 8;
#endif
   do
   {
      eTxComplete = pclANT->SendAcknowledgedData(ucChannelNumber, aucTxBuf,CONNECT_TIMEOUT);

      #if defined(DEBUG_FILE)
         if (eTxComplete == ANTFRAMER_FAIL)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptConnect():  Tx error.");
         else if (eTxComplete == ANTFRAMER_TIMEOUT)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptConnect():  Tx timeout.");
      #endif

   } while (eTxComplete == ANTFRAMER_FAIL && --ucTxRetries);

   if (eTxComplete != ANTFRAMER_PASS)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptConnect():  Failed sending connect command.");
      #endif
      return RETURN_FAIL;
   }


   if (pclANT->SetChannelPeriod(ucChannelNumber, TRANSPORT_MESSAGE_PERIOD, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptConnect():  Failed ANT_SetChannelPeriod().");
      #endif
      return RETURN_SERIAL_ERROR;
   }


   if (pclANT->SetChannelSearchTimeout(ucChannelNumber, TRANSPORT_SEARCH_TIMEOUT, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptConnect():  Failed ANT_SetChannelSearchTimeout().");
      #endif
      return RETURN_SERIAL_ERROR;
   }


   if (pclANT->SetChannelRFFrequency(ucChannelNumber, ucTransportLayerRadioFrequency, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptConnect():  Failed ANT_SetChannelRFFreq().");
      #endif
      return RETURN_SERIAL_ERROR;
   }

   do
   {
      bNewRxEvent = FALSE;

      DSIThread_MutexLock(&stMutexCriticalSection);

      if ((bNewRxEvent == FALSE) && (*pbCancel == FALSE))
      {
         UCHAR ucResult = DSIThread_CondTimedWait(&stCondRxEvent, &stMutexCriticalSection, BROADCAST_TIMEOUT);
         if (ucResult != DSI_THREAD_ENONE)
         {
            #if defined(DEBUG_FILE)
               if(ucResult == DSI_THREAD_EOTHER)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptConnect(): CondTimedWait() Failed!");
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptConnect():  Rx timeout.");
            #endif
            IncFreqStaleCount(MAX_STALE_COUNT);
            DSIThread_MutexUnlock(&stMutexCriticalSection);
            return RETURN_FAIL;
         }
      }

      bNewRxEvent = FALSE;
      DSIThread_MutexUnlock(&stMutexCriticalSection);

      if (*pbCancel == TRUE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptConnect():  Stopped.");
         #endif
         return RETURN_STOP;
      }

      if (ucFoundDeviceState == REMOTE_DEVICE_STATE_AUTH)
         bStatus = TRUE;
   } while ((bStatus == FALSE) && ucRxRetries--);

   if (!bStatus)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptConnect():  Remote not in auth mode.");
      #endif
      IncFreqStaleCount(MINOR_STALE_COUNT);                 // Investigate why this happens, may be that the remote is not updating it's beacon fast enough.
      return RETURN_FAIL;
   }

   if (ulFoundBeaconHostID != ulHostSerialNumber)
   {
     #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptConnect():  Remote connected to another host.");
      #endif
      IncFreqStaleCount(MAX_STALE_COUNT);
      return RETURN_FAIL;
   }

   return RETURN_PASS;
}


///////////////////////////////////////////////////////////////////////
ANTFSHostChannel::RETURN_STATUS ANTFSHostChannel::AttemptRequestSession(void)
{
   ANTFRAMER_RETURN eTxComplete;

   UCHAR ucTxRetries = 4;
   UCHAR ucRxRetries = 3;
   BOOL bStatus = FALSE;

   USHORT usBroadcastANTDeviceID = 0;
   UCHAR ucBroadcastANTDeviceType = 0;
   UCHAR ucBroadcastANTTransmitType = 0;

   if (pclANT->GetChannelStatus(ucChannelNumber, &ucChannelStatus, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptRequestSession():  Cannot get ANT Channel Status.");
      #endif
      return RETURN_FAIL;
   }

   if ((ucChannelStatus & STATUS_CHANNEL_STATE_MASK) != STATUS_TRACKING_CHANNEL)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptRequestSession():  Not connected to a broadcast device");
      #endif
      return RETURN_FAIL;
   }

   if (pclANT->GetChannelID(ucChannelNumber, (USHORT*)&usBroadcastANTDeviceID, (UCHAR*)&ucBroadcastANTDeviceType, (UCHAR*)&ucBroadcastANTTransmitType, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptRequestSession():  Cannot get ANT ID.");
      #endif
   }
   else
   {
      #if defined(DEBUG_FILE)
         UCHAR aucString[256];
         SNPRINTF((char *)aucString,256, "ANTFSHostChannel::AttemptRequestSession():  Broadcast Device (Device Number: %u  Device Type: %u  Tx Type: %u).", usBroadcastANTDeviceID, ucBroadcastANTDeviceType, ucBroadcastANTTransmitType);
         DSIDebug::ThreadWrite((char *)aucString);
      #endif
   }


   memset(aucTxBuf, REQUEST_PAGE_INVALID, sizeof(aucTxBuf));
   aucTxBuf[ANTFS_CONNECTION_OFFSET] = ANTFS_REQUEST_PAGE_ID ;
   aucTxBuf[REQUEST_TX_RESPONSE_OFFSET] = (UCHAR) 0x00;
   aucTxBuf[REQUEST_PAGE_NUMBER_OFFSET] = ANTFS_BEACON_ID;
   aucTxBuf[REQUEST_COMMAND_TYPE_OFFSET] = ANTFS_REQUEST_SESSION;

   // Send request session message
   do
   {
      eTxComplete = pclANT->SendAcknowledgedData(ucChannelNumber, aucTxBuf, REQUEST_TIMEOUT);

      #if defined(DEBUG_FILE)
         if (eTxComplete == ANTFRAMER_FAIL)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptRequestSession():  Tx error.");
         else if (eTxComplete == ANTFRAMER_TIMEOUT)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptRequestSession():  Tx timeout.");
      #endif
   } while (eTxComplete == ANTFRAMER_FAIL && --ucTxRetries);

   if (eTxComplete != ANTFRAMER_PASS)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptRequestSession():  Failed sending request page.");
      #endif
      return RETURN_FAIL;
   }

   // Wait for beacon
   do
   {
      bFoundDevice = FALSE;
      bNewRxEvent = FALSE;

      DSIThread_MutexLock(&stMutexCriticalSection);
      if ((bNewRxEvent == FALSE) && (*pbCancel == FALSE))
      {
         UCHAR ucResult = DSIThread_CondTimedWait(&stCondRxEvent, &stMutexCriticalSection, BROADCAST_TIMEOUT);
         if(ucResult != DSI_THREAD_ENONE)
         {
            #if defined(DEBUG_FILE)
               if(ucResult == DSI_THREAD_EOTHER)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptRequestSession(): CondTimedWait() Failed!");
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptRequestSession():  Rx timeout.");
            #endif
            DSIThread_MutexUnlock(&stMutexCriticalSection);
            return RETURN_FAIL;
         }
      }
      bNewRxEvent = FALSE;
      DSIThread_MutexUnlock(&stMutexCriticalSection);

      if (*pbCancel == TRUE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptRequestSession(): ANT-FS session request stopped.");
         #endif
         return RETURN_STOP;
      }

      if(bFoundDevice && bFoundDeviceIsValid)
         bStatus = TRUE;

   } while ((bStatus == FALSE) && ucRxRetries--);

   if(!bStatus)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptRequestSession():  No response to ANT-FS Session request.");
      #endif

      // TODO: Possibly blackout the device for a period of time here? Might need a different blacklist for this, as we should
      // blacklist based on the Channel ID instead of beacon parameters
      // Blackout(usFoundANTFSDeviceID, 0, 0, 300); // 5 min?

      return RETURN_FAIL;
   }

   if(eANTFSState != ANTFS_HOST_STATE_SEARCHING)
   {
      usFoundANTFSDeviceID = usBroadcastANTDeviceID; // TODO: Is this needed?  Assumes device ID is derived from serial number
      ucFoundANTDeviceType = ucBroadcastANTDeviceType;
      ucFoundANTTransmitType = ucBroadcastANTTransmitType;
   }

   return RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFSHostChannel::RETURN_STATUS ANTFSHostChannel::AttemptDisconnect(void)
{
   UCHAR ucTxRetries;
   ANTFRAMER_RETURN eTxComplete;

   if(eANTFSState != ANTFS_HOST_STATE_SEARCHING)
      eANTFSState = ANTFS_HOST_STATE_DISCONNECTING;
   //We change the state here so that we don't get connection lost responses after we've decided to disconnect

   if (bFoundDevice)
   {
      bFoundDevice = FALSE;
      memset(aucTxBuf, 0x00, sizeof(aucTxBuf));
      aucTxBuf[ANTFS_CONNECTION_OFFSET] = ANTFS_COMMAND_RESPONSE_ID;
      aucTxBuf[ANTFS_COMMAND_OFFSET] = ANTFS_DISCONNECT_ID;
      aucTxBuf[DISCONNECT_COMMAND_TYPE_OFFSET] = ucDisconnectType;
      aucTxBuf[DISCONNECT_TIME_DURATION_OFFSET] = ucUndiscoverableTimeDuration;
      aucTxBuf[DISCONNECT_APP_DURATION_OFFSET] = ucUndiscoverableAppSpecificDuration;

      ucTxRetries = 8;
      do
      {
         eTxComplete = pclANT->SendAcknowledgedData(ucChannelNumber, aucTxBuf, DISCONNECT_TIMEOUT);

         #if defined(DEBUG_FILE)
            if (eTxComplete == ANTFRAMER_FAIL)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Disconnect():  Tx error.");
            else if(eTxComplete == ANTFRAMER_TIMEOUT)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Disconnect():  Tx timeout.");
         #endif

      } while (eTxComplete == ANTFRAMER_FAIL && --ucTxRetries);
   }

   if(ucDisconnectType == DISCONNECT_COMMAND_BROADCAST)
   {
      // Go back to link frequency and channel period
      if (pclANT->SetChannelPeriod(ucChannelNumber, usTheMessagePeriod, MESSAGE_TIMEOUT) == FALSE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptDisconnect():  Failed ANT_SetChannelPeriod().");
         #endif
         return RETURN_SERIAL_ERROR;
      }

      if (pclANT->SetChannelRFFrequency(ucChannelNumber, ucSearchRadioFrequency, MESSAGE_TIMEOUT) == FALSE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptDisconnect():  Failed ANT_SetChannelRFFreq().");
         #endif
         return RETURN_SERIAL_ERROR;
      }
   }
   else  // DISCONNECT_TYPE_LINK or custom disconnect types
   {
      if(ucDisconnectType != DISCONNECT_COMMAND_LINK)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptDisconnect():  Application specific disconnect type.");
         #endif
         // Disconnect anyway, and close the channel
      }

      // Close the channel
      if(pclANT->GetChannelStatus(ucChannelNumber, &ucChannelStatus, MESSAGE_TIMEOUT) == FALSE)
       {
         #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptDisconnect():  Failed ANT_GetChannelStatus().");
            #endif
         return RETURN_SERIAL_ERROR;
      }

      if ((ucChannelStatus & STATUS_CHANNEL_STATE_MASK) == STATUS_TRACKING_CHANNEL ||
         (ucChannelStatus & STATUS_CHANNEL_STATE_MASK) == STATUS_SEARCHING_CHANNEL)
      {
         if (pclANT->CloseChannel(ucChannelNumber, ANT_CLOSE_TIMEOUT) == FALSE)
            return RETURN_SERIAL_ERROR;

         if (pclANT->UnAssignChannel(ucChannelNumber, MESSAGE_TIMEOUT) == FALSE)
            return RETURN_SERIAL_ERROR;
      }
      else if ((ucChannelStatus & STATUS_CHANNEL_STATE_MASK) == STATUS_ASSIGNED_CHANNEL)
      {
         if (pclANT->UnAssignChannel(ucChannelNumber, MESSAGE_TIMEOUT) == FALSE)
            return RETURN_SERIAL_ERROR;
      }
   }

   usFoundANTFSManufacturerID = 0;
   usFoundANTFSDeviceType = 0;
   bFoundDeviceHasData = FALSE;
   ucDisconnectType = DISCONNECT_COMMAND_LINK;
   ucUndiscoverableTimeDuration = 0;
   ucUndiscoverableAppSpecificDuration = 0;

   return RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFSHostChannel::RETURN_STATUS ANTFSHostChannel::AttemptSwitchFrequency()
{
   USHORT usTransportChannelPeriod;
   UCHAR ucTxRetries;
   ANTFRAMER_RETURN eTxComplete;

   UCHAR ucRxRetries = 3;
   BOOL bStatus = FALSE;

   if (ucTransportFrequencySelection == ANTFS_AUTO_FREQUENCY_SELECTION)
      ucTransportLayerRadioFrequency = CheckForNewTransportFreq();
   else
      ucTransportLayerRadioFrequency = ucTransportFrequencySelection;

   switch(ucTransportChannelPeriodSelection)
   {
      case BEACON_PERIOD_0_5_HZ:
         usTransportChannelPeriod = 65535;
         break;
      case BEACON_PERIOD_1_HZ:
         usTransportChannelPeriod = 32768;
         break;
      case BEACON_PERIOD_2_HZ:
         usTransportChannelPeriod = 16384;
         break;
      case BEACON_PERIOD_4_HZ:
         usTransportChannelPeriod = 8192;
         break;
      case BEACON_PERIOD_8_HZ:
      default:
         usTransportChannelPeriod = 4096;
         break;
   }

   // Set up the command
   memset(aucTxBuf,0x00,sizeof(aucTxBuf));
   aucTxBuf[ANTFS_CONNECTION_OFFSET] = ANTFS_COMMAND_RESPONSE_ID;
   aucTxBuf[ANTFS_COMMAND_OFFSET] = ANTFS_LINK_ID;
   aucTxBuf[TRANSPORT_CHANNEL_FREQ_OFFSET] = ucTransportLayerRadioFrequency;
   aucTxBuf[TRANSPORT_CHANNEL_PERIOD] = ucTransportChannelPeriodSelection;

   Convert_ULONG_To_Bytes(ulHostSerialNumber,
                    &aucTxBuf[HOST_ID_OFFSET+3],
                    &aucTxBuf[HOST_ID_OFFSET+2],
                    &aucTxBuf[HOST_ID_OFFSET+1],
                    &aucTxBuf[HOST_ID_OFFSET]);

   ucTxRetries = 8;
   do
   {
      eTxComplete = pclANT->SendAcknowledgedData(ucChannelNumber, aucTxBuf,CONNECT_TIMEOUT);

      #if defined(DEBUG_FILE)
         if (eTxComplete == ANTFRAMER_FAIL)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSwitchFrequency():  Tx error.");
         else if (eTxComplete == ANTFRAMER_TIMEOUT)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSwitchFrequency():  Tx timeout.");
      #endif

   } while (eTxComplete == ANTFRAMER_FAIL && --ucTxRetries);

   if (eTxComplete != ANTFRAMER_PASS)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSwitchFrequency():  Failed sending connect command.");
      #endif
      return RETURN_FAIL;
   }


   // Change channel parameters

   if(ucTransportChannelPeriodSelection != BEACON_PERIOD_KEEP)
   {
      if (pclANT->SetChannelPeriod(ucChannelNumber, usTransportChannelPeriod, MESSAGE_TIMEOUT) == FALSE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSwitchFrequency():  Failed ANT_SetChannelPeriod().");
         #endif
         return RETURN_SERIAL_ERROR;
      }
   }

   if (pclANT->SetChannelRFFrequency(ucChannelNumber, ucTransportLayerRadioFrequency, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSwitchFrequency():  Failed ANT_SetChannelRFFreq().");
      #endif
      return RETURN_SERIAL_ERROR;
   }

   // Check that the remote device changed its channel parameters as well, and that it is still a valid beacon
   do
   {
      bNewRxEvent = FALSE;

      DSIThread_MutexLock(&stMutexCriticalSection);

      if ((bNewRxEvent == FALSE) && (*pbCancel == FALSE))
      {
         UCHAR ucResult = DSIThread_CondTimedWait(&stCondRxEvent, &stMutexCriticalSection, BROADCAST_TIMEOUT);
         if (ucResult != DSI_THREAD_ENONE)
         {
            #if defined(DEBUG_FILE)
               if(ucResult == DSI_THREAD_EOTHER)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSwitchFrequency(): CondTimedWait() Failed!");
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSwitchFrequency():  Rx timeout.");
            #endif
            DSIThread_MutexUnlock(&stMutexCriticalSection);
            return RETURN_FAIL;
         }
      }

      bNewRxEvent = FALSE;
      DSIThread_MutexUnlock(&stMutexCriticalSection);

      if (*pbCancel == TRUE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSwitchFrequency():  Stopped.");
         #endif
         return RETURN_STOP;
      }

      if (ucFoundDeviceState == REMOTE_DEVICE_STATE_TRANS)
         bStatus = TRUE;
   } while ((bStatus == FALSE) && ucRxRetries--);

   if (!bStatus)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptSwitchFrequency():  Remote device not in transport state.");
      #endif
      IncFreqStaleCount(MINOR_STALE_COUNT);                 // Investigate why this happens, may be that the remote is not updating it's beacon fast enough.
      return RETURN_FAIL;
   }

   return RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::Ping(void)
{
   UCHAR ucTxRetries;
   ANTFRAMER_RETURN eTxComplete;

   if (eANTFSState == ANTFS_HOST_STATE_TRANSPORT || eANTFSState == ANTFS_HOST_STATE_CONNECTED)
   {
      memset(aucTxBuf, 0x00, sizeof(aucTxBuf));
      aucTxBuf[ANTFS_CONNECTION_OFFSET] = ANTFS_COMMAND_RESPONSE_ID;
      aucTxBuf[ANTFS_COMMAND_OFFSET] = ANTFS_PING_ID;

      ucTxRetries = 8;

      do
      {
         eTxComplete = pclANT->SendAcknowledgedData(ucChannelNumber, aucTxBuf, PING_TIMEOUT);

         #if defined(DEBUG_FILE)
            if (eTxComplete == ANTFRAMER_FAIL)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Ping():  Tx error.");
            else if (eTxComplete == ANTFRAMER_TIMEOUT)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Ping():  Tx timeout.");
         #endif
      } while (eTxComplete == ANTFRAMER_FAIL && --ucTxRetries);
   }
}

///////////////////////////////////////////////////////////////////////
ANTFSHostChannel::RETURN_STATUS ANTFSHostChannel::AttemptDownload(void)
{
   UCHAR ucNoRxTicks = 4;
   UCHAR ucAttemptTick = 15;
   UCHAR aucDownloadRequest[16];
   ANTFRAMER_RETURN eTxComplete;
   UCHAR ucCRCReset = 1;
   USHORT usCRCCalc = 0;
   ULONG ulDataOffset;
   BOOL bDone = FALSE;
   ULONG ulLastTransferArrayIndex = 0;
   ULONG ulLastUpdateTime;

   /* //The found device state may not have updated yet if we attempt to ul/dl right after authentication
   if ((ucFoundDeviceState != REMOTE_DEVICE_STATE_TRANS) && (ucFoundDeviceState != REMOTE_DEVICE_STATE_BUSY))
      return RETURN_FAIL; // Not in the correct mode.
   */

   ulTransferArrayIndex = 0;
   bReceivedResponse = FALSE;
   bReceivedBurst = FALSE;

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHostChannel::Download():  Starting download...");
   #endif

   ulLastUpdateTime = DSIThread_GetSystemTime();

   do
   {
      ulDataOffset = ulTransferDataOffset;

      if (bLargeData)
      {
         if (ulTransferArrayIndex >= 16)                        // The first 16 bytes of the array form the response packet so don't include it in the data offset.
            ulDataOffset += (ulTransferArrayIndex - 16);
      }
      else
      {
         if (ulTransferArrayIndex >= 8)                        // The first 8 bytes of the array form the response packet so don't include it in the data offset.
            ulDataOffset += (ulTransferArrayIndex - 8);
      }


      if ((!bReceivedBurst) &&(!bReceivedResponse))  //prevents us from sending requests until the Rx bursts have stopped and been cleared.
      {
         if (ucAttemptTick-- == 0)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("Download():  Maximum retry attempts reached.");
            #endif
            return RETURN_FAIL;
         }

         bRxError = FALSE;

         DSIThread_MutexLock(&stMutexCriticalSection);
         bNewRxEvent = FALSE;
         if ((bNewRxEvent == FALSE) && (*pbCancel == FALSE))
         {
            UCHAR waitResult = DSIThread_CondTimedWait(&stCondRxEvent, &stMutexCriticalSection, DOWNLOAD_RESYNC_TIMEOUT);
            if(waitResult != DSI_THREAD_ENONE)
            {
               DSIThread_MutexUnlock(&stMutexCriticalSection);
               if (waitResult == DSI_THREAD_ETIMEDOUT)
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptDownload():  Fail syncing with remote device.");
                  #endif
                     return RETURN_FAIL;         //if we've waited 10 seconds and there is no incoming messages, something is wrong with the connection
               }
               else
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptDownload():  CondTimedWait() Failed!");
                  #endif
                  return RETURN_FAIL;   //If the condition variable is broken, we are in trouble
               }
            }

            if (bNewRxEvent == FALSE)  //The way our threads are setup, we should never see this anymore now that the locks are fixed
            {
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptDownload()::  CondTimedWait false alarm signal.");
               #endif
               DSIThread_MutexUnlock(&stMutexCriticalSection);

               continue;
            }
         }
         DSIThread_MutexUnlock(&stMutexCriticalSection);

         if (*pbCancel == TRUE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("AttemptDownload():  Stopped.");
            #endif
            return RETURN_STOP;
         }

         // Send out the download command.
         memset(aucDownloadRequest, 0x00, sizeof(aucDownloadRequest));

         if (bLargeData)
         {
            aucDownloadRequest[ANTFS_CONNECTION_OFFSET] = ANTFS_COMMAND_RESPONSE_ID;
            aucDownloadRequest[ANTFS_COMMAND_OFFSET] = ANTFS_DOWNLOAD_BIG_ID;

            Convert_USHORT_To_Bytes(usTransferDataFileIndex,
                           &aucDownloadRequest[DATA_INDEX_OFFSET + 1],
                           &aucDownloadRequest[DATA_INDEX_OFFSET]);

            Convert_ULONG_To_Bytes(ulDataOffset,
                           &aucDownloadRequest[DOWNLOAD_DATA_OFFSET_OFFSET + 3],
                           &aucDownloadRequest[DOWNLOAD_DATA_OFFSET_OFFSET + 2],
                           &aucDownloadRequest[DOWNLOAD_DATA_OFFSET_OFFSET + 1],
                           &aucDownloadRequest[DOWNLOAD_DATA_OFFSET_OFFSET]);

            if (ulTransferByteSize) //if the max file size is set
            {
               ULONG ulLocalBlockSize = MAX_ULONG;
               if(ulHostBlockSize)
                  ulLocalBlockSize = ulHostBlockSize;

               //We need to set the block size to the max transfer size - the offset.
               if(ulLocalBlockSize > (ulTransferByteSize - (ulDataOffset - ulTransferDataOffset)))
                  ulLocalBlockSize = ulTransferByteSize - (ulDataOffset - ulTransferDataOffset);

               Convert_ULONG_To_Bytes(ulLocalBlockSize,
                           &aucDownloadRequest[DOWNLOAD_MAX_BLOCK_SIZE_OFFSET + 3],
                           &aucDownloadRequest[DOWNLOAD_MAX_BLOCK_SIZE_OFFSET + 2],
                           &aucDownloadRequest[DOWNLOAD_MAX_BLOCK_SIZE_OFFSET + 1],
                           &aucDownloadRequest[DOWNLOAD_MAX_BLOCK_SIZE_OFFSET]);
            }

            if (ulTransferArrayIndex > 16)                              // If the download has progressed at least to the first data packet
               ucCRCReset = 0;                                          // Clear the Initial download request bit, so the client knows to start checking our CRC

            aucDownloadRequest[DOWNLOAD_INITIAL_REQUEST_OFFSET] = ucCRCReset;  // Set initial request byte
            if (ucCRCReset == 0)                                        // If this is not the initial request
            {                                                           // Calculate and send a non-zero CRC value

               if (pucTransferBuffer != NULL)                           // Just to make sure the transfer buffer has been created
                  usCRCCalc = CRC_Calc16(&pucTransferBuffer[16], ulDataOffset-ulTransferDataOffset);  //CRC_UpdateCRC16
               else
                  usCRCCalc = 0;

               Convert_USHORT_To_Bytes(usCRCCalc,
                              &aucDownloadRequest[DOWNLOAD_CRC_SEED_OFFSET + 1],
                              &aucDownloadRequest[DOWNLOAD_CRC_SEED_OFFSET]);
            }
         }
         else
         {
            aucDownloadRequest[ANTFS_CONNECTION_OFFSET] = ANTFS_COMMAND_RESPONSE_ID;
            aucDownloadRequest[ANTFS_COMMAND_OFFSET] = ANTFS_DOWNLOAD_SMALL_ID;

            Convert_USHORT_To_Bytes(usTransferDataFileIndex,
                           &aucDownloadRequest[DATA_INDEX_OFFSET + 1],
                           &aucDownloadRequest[DATA_INDEX_OFFSET]);

            Convert_ULONG_To_Bytes(ulDataOffset,
                           (UCHAR*)NULL,
                           (UCHAR*)NULL,
                           &aucDownloadRequest[DATA_OFFSET_SMALL_OFFSET + 1],
                           &aucDownloadRequest[DATA_OFFSET_SMALL_OFFSET]);

            Convert_ULONG_To_Bytes(ulTransferByteSize,
                           (UCHAR*)NULL,
                           (UCHAR*)NULL,
                           &aucDownloadRequest[MAX_BLOCK_SIZE_SMALL_OFFSET + 1],
                           &aucDownloadRequest[MAX_BLOCK_SIZE_SMALL_OFFSET]);
         }

         if (bLargeData)
            eTxComplete = pclANT->SendANTFSTransfer(ucChannelNumber, (UCHAR*)NULL, (UCHAR*)NULL, aucDownloadRequest, 16, ACKNOWLEDGED_TIMEOUT, (ULONG*)NULL);
         else
            eTxComplete = pclANT->SendAcknowledgedData(ucChannelNumber, aucDownloadRequest, ACKNOWLEDGED_TIMEOUT);

         #if defined(DEBUG_FILE)
            if (eTxComplete == ANTFRAMER_FAIL)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Download():  Tx error sending download command.");
            else if (eTxComplete == ANTFRAMER_TIMEOUT)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Download():  Tx timeout sending download command.");
         #endif

         //Removed the immediate retries to avoid the issue of queing up requests while we get a response for a request we thought we failed.

         if (eTxComplete == ANTFRAMER_TIMEOUT)
            return RETURN_FAIL;
      }

      //Do not need to clear bReceivedBurst here because it will be done if the transfer fails, if we timeout, or receive a broadcast.
      //Now we wait for a response...
      while (bDone == FALSE)
      {
         //Wait for an rxEvent before starting to check the data
         //Since this event is fired for many circumstances we manage all the error checking below and
         //just use this for the wait functionality.
         DSIThread_MutexLock(&stMutexCriticalSection);
         bNewRxEvent = FALSE;
         if ((bNewRxEvent == FALSE) && (*pbCancel == FALSE))
         {
            DSIThread_CondTimedWait(&stCondRxEvent, &stMutexCriticalSection, cfgParams.ul_cfg_burst_check_timeout);
         }
         DSIThread_MutexUnlock(&stMutexCriticalSection);
         if (*pbCancel == TRUE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptDownload():  Stopped.");
            #endif
            return RETURN_STOP;
         }

         if (bReceivedResponse)        //If a response has been received, process it.
         {
            bReceivedResponse = FALSE;  //Clear these for any potential retries
            bReceivedBurst = FALSE;     //Clearing this here allows for quicker retries, otherwise we would have to wait for an incoming broadcast to clear it
            bDone = TRUE; //Mark that we are done... for now.

            if ((pucTransferBufferDynamic) && ((pucTransferBufferDynamic[ANTFS_COMMAND_OFFSET] == ANTFS_RESPONSE_DOWNLOAD_SMALL_ID) || (pucTransferBufferDynamic[ANTFS_COMMAND_OFFSET] == ANTFS_RESPONSE_DOWNLOAD_BIG_ID)))
            {
             /*if (pucTransferBufferDynamic[DOWNLOAD_RESPONSE_OFFSET] == DOWNLOAD_RESPONSE_CRC_FAILED)
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::Download():  CRC failed, download failed.");
                  #endif
                  //clear variables to retry from start.
                  ucCRCReset = 1;
                  usCRCCalc = 0;
                  ulTransferArrayIndex = 16;
                  ulLastTransferArrayIndex = 16;
                  bDone = FALSE;
                  break;
               }
               else*/ //Removed Failed CRC response check and automatic retry.  Return the response to the application and allow it to decide to retry or to skip the file.  Can also use Recover Transfer data to recover partial files as this seems to be the most way a corrupted file will fail.

               if (pucTransferBufferDynamic[DOWNLOAD_RESPONSE_OFFSET] != DOWNLOAD_RESPONSE_OK)
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::Download():  Download request rejected.");
                  #endif
                  ucRejectCode = pucTransferBufferDynamic[DOWNLOAD_RESPONSE_OFFSET];
                  return RETURN_REJECT;
               }
            }
            else
            {
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::Download():  Unknown response during download.");
               #endif
               return RETURN_FAIL;
            }

            // Check if we need to check the CRC
            if ((bLargeData) &&
                (((ulDataOffset - ulTransferDataOffset) + ulTransferBytesInBlock + 8) < ulTransferArrayIndex)) //if there is one more packet beyond the data, we will process the CRC
            {
               ULONG ulCRCLocation, ulLength;
               USHORT usReceivedCRC;
               USHORT usCalcCRC;

               ulLength = (ulDataOffset - ulTransferDataOffset) + ulTransferBytesInBlock;   //find length of actual data that needs to be CRC checked

               ulCRCLocation = ulTransferArrayIndex - 2;   //Assume that the CRC will be the last 2 bytes of the last packet received

               ulTransferArrayIndex = ulLength + 16;  //correct ulTransferArrayIndex in case we are downloading in odd blocks.

               usReceivedCRC = pucTransferBufferDynamic[ulCRCLocation];
               usReceivedCRC |= ((USHORT)pucTransferBufferDynamic[ulCRCLocation + 1] << 8);

               usCalcCRC = CRC_Calc16(&pucTransferBufferDynamic[16], ulLength);  //Calculate the CRC of the received data from the start, this should always be what the CRC value is based in becaue we pass in the initial seed

               if (usCalcCRC != usReceivedCRC)
               {
                  #if defined(DEBUG_FILE)
                     char cBuffer[256];
                     SNPRINTF(cBuffer, 256, "ANTFSHostChannel::Download():  Failed CRC Check. Expected %d, Got %d",usCalcCRC, usReceivedCRC);
                     DSIDebug::ThreadWrite(cBuffer);
                  #endif
                  //return RETURN_FAIL;
                  //clear variables to retry from start.
                  ucCRCReset = 1;
                  usCRCCalc = 0;
                  ulTransferArrayIndex = 16;
                  ulLastTransferArrayIndex = 16;
                  bDone = FALSE;
               }
            }

            //Catch the cases where we get a completed transfer but there are more packets coming
            if ((bLargeData) && ((ulTransferArrayIndex -16) < ulTransferTotalBytesRemaining))
               bDone = FALSE;

            ucNoRxTicks = 4;                                // Reset counter
            ucAttemptTick = 15;                             // Reset counter for attempts of request
            break;
         }

         if (bRxError)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Download():  Rx error.");
            #endif
            break;
         }

         if (ulTransferArrayIndex > ulLastTransferArrayIndex)
         {
            ulLastUpdateTime = DSIThread_GetSystemTime();
            ulLastTransferArrayIndex = ulTransferArrayIndex;
         }
         else
         {
            if ((DSIThread_GetSystemTime() - ulLastUpdateTime) > DOWNLOAD_LOOP_TIMEOUT)
            {
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::Download():  Timeout receiving packets.");
               #endif
               return RETURN_FAIL;
            }
         }

         if (!bReceivedBurst)
         {
            if ((ucFoundDeviceState != REMOTE_DEVICE_STATE_BUSY)  && (ucNoRxTicks > 0))
               ucNoRxTicks--;

            if (ucNoRxTicks == 0)
            {
               ucNoRxTicks = 4;
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::Download():  Not receiving packets.");
               #endif
               break;   //If device didn't respond the command didn't make it through or the client device is ignoring it
            }
         }
         else
         {
            ucNoRxTicks = 4;                                // Reset counter
            ucAttemptTick = 15;                             // Reset counter for attempts of request
         }

         ReportDownloadProgress();

      } //while();

   } while (!bDone);



   bTransfer = TRUE;

   // Wait for resync.
   DSIThread_MutexLock(&stMutexCriticalSection);
   bNewRxEvent = FALSE;
   if ((bNewRxEvent == FALSE) && (*pbCancel == FALSE))
   {
      UCHAR ucResult = DSIThread_CondTimedWait(&stCondRxEvent, &stMutexCriticalSection, BROADCAST_TIMEOUT);
      if (ucResult != DSI_THREAD_ENONE)
      {
         #if defined(DEBUG_FILE)
            if(ucResult == DSI_THREAD_EOTHER)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptDownload(): CondTimedWait() Failed!");
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptDownload():  Rx timeout.");
         #endif
      }
   }
   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFSHostChannel::RETURN_STATUS ANTFSHostChannel::UploadLoop(void)
{
   RETURN_STATUS eReturn;
   ULONG ulLastProgressValue = 0;
   ULONG ulLastTimeWeGotDataThru = DSIThread_GetSystemTime();
   UCHAR ucFreshRetries = 3;
   #if defined(DEBUG_FILE)
      UCHAR aucString[256];
   #endif

   do
   {
      //ulLastProgressValue = ulUploadIndexProgress;                  // figure out where we thought we left off last time, this may be a little further than we actually got, but it will be okay unless the USB device has a massive amount of buffering or is broken.
      eReturn = AttemptUpload();

      switch (eReturn)
      {
         case RETURN_PASS:
            if ((ulUploadIndexProgress - UPLOAD_DATA_MESG_OVERHEAD) >= ulTransferByteSize)
               return RETURN_PASS;

            if (ulUploadIndexProgress > ulLastProgressValue + UPLOAD_DATA_MESG_OVERHEAD)  //we use the overhead size for the check here because the return was a pass
            {
               bForceUploadOffset = FALSE;                          //clear the force offset if we get any data across
               ulLastTimeWeGotDataThru = DSIThread_GetSystemTime();
               ulLastProgressValue = ulUploadIndexProgress;
            }

            #if defined(DEBUG_FILE)
               SNPRINTF((char *) aucString, 256, "ANTFSHostChannel::UploadLoop(): RETURN_PASS - %lu", ulLastTimeWeGotDataThru);
               DSIDebug::ThreadWrite((char *) aucString);
            #endif
            break;
         case RETURN_FAIL:
            if (ulUploadIndexProgress > ulLastProgressValue + UPLOAD_PROGRESS_CHECK_BYTES)  //we use the larger check size here because the return was a failure
            {
               bForceUploadOffset = FALSE;                          //clear the force offset if we get any data across
               ulLastTimeWeGotDataThru = DSIThread_GetSystemTime();
               ulLastProgressValue = ulUploadIndexProgress;
            }

            #if defined(DEBUG_FILE)
               SNPRINTF((char *) aucString, 256, "ANTFSHostChannel::UploadLoop(): RETURN_FAIL - %lu", ulLastTimeWeGotDataThru);
               DSIDebug::ThreadWrite((char *) aucString);
            #endif
            break;
         case RETURN_NA:  //This means we have failed CRC or the device returned a weird last offset.
            if (ucFreshRetries--)
            {
               ulUploadIndexProgress = 0;  //reset the progress
               ulLastProgressValue = 0;
               ulLastTimeWeGotDataThru = DSIThread_GetSystemTime();  //reset the time
            }
            else
            {
               return RETURN_FAIL;
            }
            break;
         case RETURN_REJECT:
         case RETURN_STOP:
         default:
            return eReturn;
         break;
      }
   }
   while ((DSIThread_GetSystemTime() - ulLastTimeWeGotDataThru) < UPLOAD_LOOP_TIMEOUT);

   return RETURN_FAIL;
}

///////////////////////////////////////////////////////////////////////
ANTFSHostChannel::RETURN_STATUS ANTFSHostChannel::AttemptUpload(void)
{
   BOOL bStatus = FALSE;
   UCHAR aucTxUpload[16];
   UCHAR aucUploadHeader[8];
   UCHAR aucUploadFooter[8];
   ULONG ulStartTime;
   UCHAR ucTxRetries;
   ANTFRAMER_RETURN eTxComplete;
   ULONG ulLastOffset;
   ULONG ulMaxBlockSize, ulMaxFileSize, ulUploadTimeout, ulLocalTransferSize;
   ANTFRAMER_RETURN eReturn;
   USHORT usReceivedCRC;
   USHORT usCalculatedCRC;
   USHORT usCRCSeed;

   UCHAR ucUploadResponse;
   //The found device state may not have updated yet if we attempt to ul/dl right after authentication, but addition of ULLoop allows us to do this check
   if ((ucFoundDeviceState != REMOTE_DEVICE_STATE_TRANS) && (ucFoundDeviceState != REMOTE_DEVICE_STATE_BUSY))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Remote device is not in correct state.");
      #endif

      return RETURN_FAIL;                                   // Not in the correct mode.
   }

   //ulTransferArrayIndex = 0;
   bReceivedResponse = FALSE;
   bRxError = FALSE;
   bReceivedBurst = FALSE;

   memset(aucTxUpload, 0x00, sizeof(aucTxUpload));

   ULONG ulMaxTransferIndex = ulTransferDataOffset + ulTransferByteSize;

   // Build upload request
   aucTxUpload[ANTFS_CONNECTION_OFFSET] = ANTFS_COMMAND_RESPONSE_ID;
   aucTxUpload[ANTFS_COMMAND_OFFSET] = ANTFS_UPLOAD_BIG_ID;

   Convert_USHORT_To_Bytes(usTransferDataFileIndex,
                           &aucTxUpload[DATA_INDEX_OFFSET + 1],
                           &aucTxUpload[DATA_INDEX_OFFSET]);

   Convert_ULONG_To_Bytes(ulMaxTransferIndex,
                           &aucTxUpload[UPLOAD_MAX_SIZE_OFFSET + 3],
                           &aucTxUpload[UPLOAD_MAX_SIZE_OFFSET + 2],
                           &aucTxUpload[UPLOAD_MAX_SIZE_OFFSET + 1],
                           &aucTxUpload[UPLOAD_MAX_SIZE_OFFSET]);

   if (bForceUploadOffset)
   {
      Convert_ULONG_To_Bytes(ulTransferDataOffset,
                       &aucTxUpload[UPLOAD_DATA_OFFSET_OFFSET + 3],
                       &aucTxUpload[UPLOAD_DATA_OFFSET_OFFSET + 2],
                       &aucTxUpload[UPLOAD_DATA_OFFSET_OFFSET + 1],
                       &aucTxUpload[UPLOAD_DATA_OFFSET_OFFSET]);
   }
   else
   {
      Convert_ULONG_To_Bytes(MAX_ULONG,
                       &aucTxUpload[UPLOAD_DATA_OFFSET_OFFSET + 3],
                       &aucTxUpload[UPLOAD_DATA_OFFSET_OFFSET + 2],
                       &aucTxUpload[UPLOAD_DATA_OFFSET_OFFSET + 1],
                       &aucTxUpload[UPLOAD_DATA_OFFSET_OFFSET]);
   }

   bNewRxEvent = FALSE;
   ucTxRetries = 8;

   // Try sending upload request until success or retries exhausted.
   eTxComplete = pclANT->SendANTFSTransfer(ucChannelNumber, (UCHAR*)NULL, (UCHAR*)NULL, aucTxUpload, 16, ACKNOWLEDGED_TIMEOUT, (ULONG*)NULL);

   #if defined(DEBUG_FILE)
      if (eTxComplete == ANTFRAMER_FAIL)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Tx error sending upload command.");
      else if (eTxComplete == ANTFRAMER_TIMEOUT)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Tx timeout sending upload command.");
   #endif

   //Removed the immediate retries to avoid the issue of queing up requests while we get a response for a request we thought we failed.

   if (eTxComplete == ANTFRAMER_TIMEOUT)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Tx failed.");
      #endif

      return RETURN_FAIL;
   }

   ulStartTime = DSIThread_GetSystemTime();

   ucLinkResponseRetries = ANTFS_RESPONSE_RETRIES;

   do
   {
      DSIThread_MutexLock(&stMutexCriticalSection);
      if ((bNewRxEvent == FALSE) && (*pbCancel == FALSE))
      {
         DSIThread_CondTimedWait(&stCondRxEvent, &stMutexCriticalSection, 1000);
      }
      bNewRxEvent = FALSE;
      DSIThread_MutexUnlock(&stMutexCriticalSection);

      if (*pbCancel == TRUE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptUpload():  Stopped.");
         #endif
         return RETURN_STOP;
      }

      if (bReceivedResponse)
      {
         if (aucTransferBufferFixed[ANTFS_COMMAND_OFFSET] == ANTFS_RESPONSE_UPLOAD_ID)
         {
            if (aucTransferBufferFixed[UPLOAD_RESPONSE_OFFSET] == UPLOAD_RESPONSE_OK)
            {
               bStatus = TRUE;
               bLargeData = TRUE;
            }
            else
            {
               ucRejectCode = aucTransferBufferFixed[UPLOAD_RESPONSE_OFFSET];
               return RETURN_REJECT;
            }
         }
      }
      #if defined(DEBUG_FILE)
         else
         {
            DSIDebug::ThreadWrite("-->Requesting Upload...");
         }
      #endif

      if ((bRxError) || (ucFoundDeviceState != REMOTE_DEVICE_STATE_BUSY))
      {
         #if defined(DEBUG_FILE)
            if (bRxError)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Rx error");
            else
               DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Non-busy beacon");
         #endif
         if (ucLinkResponseRetries == 0)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Retries exhausted");
            #endif
            return RETURN_FAIL;
         }
         else
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Retrying");
            #endif
            ucLinkResponseRetries--;
            bRxError = FALSE;
         }
      }
   } while ((bStatus == FALSE) && ((DSIThread_GetSystemTime() - ulStartTime) < cfgParams.ul_cfg_upload_request_timeout));

   if (!bStatus)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Timeout.");
      #endif
      return RETURN_FAIL;
   }

   bReceivedResponse = FALSE;
   bNewRxEvent = FALSE;

   ucUploadResponse = aucTransferBufferFixed[UPLOAD_RESPONSE_OFFSET];

   if (!ucUploadResponse)     // it's accepting the upload
   {
      // Decode response
      ulLastOffset = Convert_Bytes_To_ULONG(aucTransferBufferFixed[UPLOAD_RESPONSE_LAST_OFFSET_OFFSET + 3],
                                             aucTransferBufferFixed[UPLOAD_RESPONSE_LAST_OFFSET_OFFSET + 2],
                                             aucTransferBufferFixed[UPLOAD_RESPONSE_LAST_OFFSET_OFFSET + 1],
                                             aucTransferBufferFixed[UPLOAD_RESPONSE_LAST_OFFSET_OFFSET]);

      ulMaxBlockSize = Convert_Bytes_To_ULONG(aucTransferBufferFixed[UPLOAD_RESPONSE_BLOCK_SIZE_OFFSET + 3],
                                             aucTransferBufferFixed[UPLOAD_RESPONSE_BLOCK_SIZE_OFFSET + 2],
                                             aucTransferBufferFixed[UPLOAD_RESPONSE_BLOCK_SIZE_OFFSET + 1],
                                             aucTransferBufferFixed[UPLOAD_RESPONSE_BLOCK_SIZE_OFFSET]);

      usReceivedCRC = Convert_Bytes_To_USHORT(aucTransferBufferFixed[UPLOAD_RESPONSE_CRC_OFFSET + 1],
                                              aucTransferBufferFixed[UPLOAD_RESPONSE_CRC_OFFSET]);


      ulMaxFileSize = Convert_Bytes_To_ULONG(aucTransferBufferFixed[UPLOAD_RESPONSE_MAX_SIZE_OFFSET + 3],
                                             aucTransferBufferFixed[UPLOAD_RESPONSE_MAX_SIZE_OFFSET + 2],
                                             aucTransferBufferFixed[UPLOAD_RESPONSE_MAX_SIZE_OFFSET + 1],
                                             aucTransferBufferFixed[UPLOAD_RESPONSE_MAX_SIZE_OFFSET]);

      if (ulMaxTransferIndex > ulMaxFileSize)
      {
         #if defined(DEBUG_FILE)
               UCHAR aucString[256];
               SNPRINTF((char *) aucString, 256, "ANTFSHostChannel::Upload(): Upload size larger than Max Files size (%lu > %lu)", ulMaxTransferIndex, ulMaxFileSize);
               DSIDebug::ThreadWrite((char *) aucString);
         #endif
         ucRejectCode = UPLOAD_RESPONSE_INSUFFICIENT_SPACE;
         return RETURN_REJECT;
      }

      if (bForceUploadOffset)                                       // Override the suggested offset if we are instructed to.
      {
         usCRCSeed = 0;
         ulLastOffset = ulTransferDataOffset;                       // We will start from where the app told us to
         ulLocalTransferSize = ulTransferByteSize;                  // and set the next transfer size to the correct value
      }
      else  //we need to check the CRC
      {
         if (ulLastOffset > ulMaxTransferIndex) //The last offset we got is larger than our max, we are just going to force a restart from where we want to start
         {
            #if defined(DEBUG_FILE)
               UCHAR aucString[256];
               SNPRINTF((char *) aucString, 256, "ANTFSHostChannel::Upload(): ulLastOffset from device is larger than our max offset - Restarting UL (%lu > %lu)", ulLastOffset, ulMaxTransferIndex);
               DSIDebug::ThreadWrite((char *) aucString);
            #endif
            bForceUploadOffset = TRUE;
            return RETURN_NA;
         }

         usCRCSeed = CRC_Calc16(&pucUploadData[0], ulLastOffset-ulTransferDataOffset);
         if (usCRCSeed != usReceivedCRC)
         {
            #if defined(DEBUG_FILE)
               UCHAR aucString[256];
               SNPRINTF((char *) aucString, 256, "ANTFSHostChannel::Upload(): CRC Check Failed - Wanted 0x%04X, Got 0x%04X", usCRCSeed, usReceivedCRC);
               DSIDebug::ThreadWrite((char *) aucString);
            #endif
            //We are going to send no data.
            ulLocalTransferSize = 0;
            //ulLastOffset = 0;                                       // we're going to start over if the client doesn't have the right CRC
            bForceUploadOffset = TRUE;
            return RETURN_NA;
         }
      }

      if (ulLastOffset == ulMaxTransferIndex)
      {

         #if defined(DEBUG_FILE)
            UCHAR aucString[256];
            SNPRINTF((char *) aucString, 256, "ANTFSHostChannel::Upload(): We think we were sucessful Last time - UploadIndexProgress %lu, TransferByteSize %lu", ulUploadIndexProgress, ulTransferByteSize);
            DSIDebug::ThreadWrite((char *) aucString);
         #endif
         //ulLastOffset = 0;
         ulLocalTransferSize = 0;                                //Send only the header and try if we think we actually completed last time and only failed getting the response
      }
      else
      {
         if (ulLastOffset > ulTransferDataOffset)                      //Check if there is a suggested last offset and if it's further along than our original offset
         {
            ulLocalTransferSize = ulTransferByteSize - (ulLastOffset - ulTransferDataOffset);  //If it is then we'll use it, and adjust the size of the next burst
         }
         else
         {
            ulLastOffset = ulTransferDataOffset;                    // We will start from where the app told us to
            ulLocalTransferSize = ulTransferByteSize;               // and set the next transfer size to the correct value
         }
      }

      if (ulLocalTransferSize)                                      // If we are actually sending data
         ulUploadIndexProgress = ulLastOffset - ulTransferDataOffset; // Set the progress to start at the offset
      else
         ulUploadIndexProgress = ulTransferByteSize;                // Otherwise, we are already complete and just need confirmation, so set the progress back to the transfer size (the transfer function will keep trying to add to this because of the header)




      if(ulMaxBlockSize < ulLocalTransferSize)                      // Check if we exceed the max client block size
         ulLocalTransferSize = ulMaxBlockSize;

      if(ulHostBlockSize < ulLocalTransferSize)                     // Check if we exceed the max host block size
         ulLocalTransferSize = ulHostBlockSize;

      if (ulLocalTransferSize)
      {
         usCalculatedCRC = CRC_Calc16(&pucUploadData[0], (ulLastOffset - ulTransferDataOffset) + ulLocalTransferSize);  // compute the CRC from the start up till the end of the current block
      }
      else
      {
         usCalculatedCRC = usCRCSeed;
      }


      //Add CRC to the footer.
      memset(aucUploadFooter,0x00,sizeof(aucUploadFooter));
      Convert_USHORT_To_Bytes(usCalculatedCRC,
                      &aucUploadFooter[UPLOAD_DATA_CRC_OFFSET + 1],
                      &aucUploadFooter[UPLOAD_DATA_CRC_OFFSET]);                 //put CRC at the end of the footer packet


      // figure out our timeout value from the size of the transfer
      if (ulLocalTransferSize > ((MAX_ULONG - BROADCAST_TIMEOUT) / 2))
         ulUploadTimeout = (MAX_ULONG - 1);
      else
         ulUploadTimeout = BROADCAST_TIMEOUT + (ulTransferByteSize * 2);


      // fill in the header information
      aucUploadHeader[0] = ANTFS_COMMAND_RESPONSE_ID;
      aucUploadHeader[1] = ANTFS_UPLOAD_DATA_ID;
      Convert_USHORT_To_Bytes(usCRCSeed,
                      &aucUploadHeader[UPLOAD_DATA_CRC_SEED_OFFSET + 1],
                      &aucUploadHeader[UPLOAD_DATA_CRC_SEED_OFFSET]);


      if (ulLastOffset == ulMaxTransferIndex) // If we think we are done, we will fill zero into the offset but we send no data.
      {                                         // This should result in only a complete to the embedded application.
         Convert_ULONG_To_Bytes(0,
                              &aucUploadHeader[UPLOAD_DATA_DATA_OFFSET_OFFSET + 3],
                              &aucUploadHeader[UPLOAD_DATA_DATA_OFFSET_OFFSET + 2],
                              &aucUploadHeader[UPLOAD_DATA_DATA_OFFSET_OFFSET + 1],
                              &aucUploadHeader[UPLOAD_DATA_DATA_OFFSET_OFFSET]);
      }
      else
      {
         Convert_ULONG_To_Bytes(ulLastOffset,
                              &aucUploadHeader[UPLOAD_DATA_DATA_OFFSET_OFFSET + 3],
                              &aucUploadHeader[UPLOAD_DATA_DATA_OFFSET_OFFSET + 2],
                              &aucUploadHeader[UPLOAD_DATA_DATA_OFFSET_OFFSET + 1],
                              &aucUploadHeader[UPLOAD_DATA_DATA_OFFSET_OFFSET]);
      }

      //Send the Burst
      eReturn = pclANT->SendANTFSTransfer(ucChannelNumber, aucUploadHeader, aucUploadFooter, &pucUploadData[ulLastOffset], ulLocalTransferSize, ulUploadTimeout, &ulUploadIndexProgress);

      if (eReturn == ANTFRAMER_FAIL)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Failed Actual Upload Transfer.");
         #endif
         //fall thru to catch response
      }
      else if (eReturn == ANTFRAMER_CANCELLED)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptUpload():  Stopped2.");
         #endif
         return RETURN_STOP;
      }
      else if (eReturn == ANTFRAMER_TIMEOUT)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptUpload():  Tx transfer timeout.");
         #endif
         return RETURN_FAIL;
      }
   }
   else
   {
   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Rejected by Client.");
   #endif
      ucRejectCode = ucUploadResponse;
      return RETURN_FAIL;
   }


   ulStartTime = DSIThread_GetSystemTime();

   //ResetEvent(hEventRx);

   ucLinkResponseRetries = ANTFS_RESPONSE_RETRIES;
   bStatus = FALSE;

   do
   {
      DSIThread_MutexLock(&stMutexCriticalSection);
      if ((bNewRxEvent == FALSE) && (*pbCancel == FALSE))
      {
         DSIThread_CondTimedWait(&stCondRxEvent, &stMutexCriticalSection, 1000);
      }
      bNewRxEvent = FALSE;
      DSIThread_MutexUnlock(&stMutexCriticalSection);

      if (*pbCancel == TRUE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptUpload():  Stopped.");
         #endif
         return RETURN_STOP;
      }

      if ((bRxError) || (ucFoundDeviceState != REMOTE_DEVICE_STATE_BUSY))
      {
         #if defined(DEBUG_FILE)
            if (bRxError)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Rx error2");
            else
               DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Non-busy beacon2");
         #endif
         if (ucLinkResponseRetries == 0)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Retries exhausted2");
            #endif
            return RETURN_FAIL;
         }
         else
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Upload():  Retrying2");
            #endif
            ucLinkResponseRetries--;
            bRxError = FALSE;
         }
      }

      if (bReceivedResponse)
      {
         if (aucTransferBufferFixed[ANTFS_COMMAND_OFFSET] == ANTFS_RESPONSE_UPLOAD_COMPLETE_ID)
         {
            if (aucTransferBufferFixed[UPLOAD_RESPONSE_OFFSET] == UPLOAD_RESPONSE_OK)
            {
               return RETURN_PASS;
            }
            else
            {
               return RETURN_FAIL;
            }
         }
      }
      #if defined(DEBUG_FILE)
         else
         {
            DSIDebug::ThreadWrite("-->Waiting for upload complete...");
         }
      #endif
   } while ((bStatus == FALSE) && ((DSIThread_GetSystemTime() - ulStartTime) < cfgParams.ul_cfg_upload_response_timeout));

   return RETURN_FAIL;

}

///////////////////////////////////////////////////////////////////////
ANTFSHostChannel::RETURN_STATUS ANTFSHostChannel::AttemptManualTransfer(void)
{
   ULONG ulBlockSize;
   USHORT usBlockOffset;

   // Convert bytes remaining to blocks remaining.
   if (ulTransferByteSize % 8)
      ulBlockSize = 1;                                      // There is a remainder, so we need to send one extra payload to hold all the data.
   else
      ulBlockSize = 0;

   ulBlockSize += ulTransferByteSize / 8;

   // Convert byte offset to block offset.
   usBlockOffset = (USHORT) ulTransferDataOffset / 8;       // The remainder must be dropped when computing offset.

   ulTransferArrayIndex = 0;
   bReceivedBurst = FALSE;
   bReceivedResponse = FALSE;

   if (*pbCancel == TRUE)
      return RETURN_STOP;

   aucSendDirectBuffer[ANTFS_CONNECTION_OFFSET] = ANTFS_COMMAND_RESPONSE_ID;
   aucSendDirectBuffer[ANTFS_COMMAND_OFFSET] = ANTFS_SEND_DIRECT_ID;

   Convert_USHORT_To_Bytes(usTransferDataFileIndex,
                           &aucSendDirectBuffer[DATA_INDEX_OFFSET + 1],
                           &aucSendDirectBuffer[DATA_INDEX_OFFSET]);

   Convert_USHORT_To_Bytes(usBlockOffset,
                           &aucSendDirectBuffer[DATA_OFFSET_OFFSET_HIGH],
                           &aucSendDirectBuffer[DATA_OFFSET_OFFSET_LOW]);

   Convert_ULONG_To_Bytes(ulBlockSize - 1,               // Size - 1, as per the spec.
                           (UCHAR*)NULL,
                           (UCHAR*)NULL,
                           &aucSendDirectBuffer[DATA_BLOCK_SIZE_OFFSET_HIGH],
                           &aucSendDirectBuffer[DATA_BLOCK_SIZE_OFFSET_LOW]);

#if 0
   #if defined(DEBUG_FILE)
      {
         char acString[256];
         int iCounter;
         ULONG ulCounter2;

         SNPRINTF(acString, 256, "ANTFSHostChannel::ManualTransfer():  ulTransferByteSize = %lu; ulTransferDataOffset = %lu.", ulTransferByteSize, ulTransferDataOffset);
         DSIDebug::ThreadWrite(acString);

         for (ulCounter2 = 0; ulCounter2 < ulBlockSize; ulCounter2++)
         {
            SNPRINTF(acString, 256, "ANTFSHostChannel::ManualTransfer():      <%lu>", ulCounter2);

            for (iCounter = 0; iCounter < 8; iCounter++)
            {
               char acSubString[10];

               SNPRINTF(acSubString, 10, "[%02X]", aucSendDirectBuffer[ulCounter2 + (ULONG) iCounter]);
               SNPRINTF(acString, 256, "%s%s", acString, acSubString);
            }

            DSIDebug::ThreadWrite(acString);
         }
      }
   #endif
#endif

   ulUploadIndexProgress = 0;
   //pclANT->SendANTFSTransfer(ucChannelNumber, aucSendDirectBuffer, &aucSendDirectBuffer[8], (ulBlockSize + 1)*8, 120000, &ulUploadIndexProgress);  // Size + 1 to account for the command payload.
   pclANT->SendANTFSTransfer(ucChannelNumber, (UCHAR*)NULL, (UCHAR*)NULL, aucSendDirectBuffer, (ulBlockSize + 1)*8, 120000, &ulUploadIndexProgress);  // Size + 1 to account for the command payload.

   //pclANT->SendTransfer(ANTFS_CHANNEL, aucSendDirectBuffer, (ulBlockSize + 1)*8,120000);  // Size + 1 to account for the command payload.
   //allow the receive function to return with error, even if the Tx transfer failed

   return Receive();
}

///////////////////////////////////////////////////////////////////////
ANTFSHostChannel::RETURN_STATUS ANTFSHostChannel::Receive(void)
{
   ULONG ulNoRxTime = DSIThread_GetSystemTime();
   UCHAR ucTicks = ucTransportBeaconTicks;

   bNewRxEvent = FALSE;
   eANTFSState = ANTFS_HOST_STATE_RECEIVING;

   while (bKillThread == FALSE)
   {
      DSIThread_MutexLock(&stMutexCriticalSection);
      if ((bNewRxEvent == FALSE) && (*pbCancel == FALSE))
      {
         UCHAR ucResult = DSIThread_CondTimedWait(&stCondRxEvent, &stMutexCriticalSection, cfgParams.ul_cfg_burst_check_timeout);
         if (ucResult != DSI_THREAD_ENONE)
         {
            #if defined(DEBUG_FILE)
               if(ucResult == DSI_THREAD_EOTHER)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::Receive(): CondTimedWait() Failed!");
            #endif
            DSIThread_MutexUnlock(&stMutexCriticalSection);
            return RETURN_FAIL;
         }
      }
      bNewRxEvent = FALSE;
      DSIThread_MutexUnlock(&stMutexCriticalSection);

      if (*pbCancel == TRUE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::Receive():  Stopped.");
         #endif
         return RETURN_STOP;
      }

      if ((ucFoundDeviceState == REMOTE_DEVICE_STATE_TRANS) && ((ucTransportBeaconTicks - ucTicks) >= 5))
         return RETURN_REJECT;

      if (bReceivedResponse && (ucFoundDeviceState == REMOTE_DEVICE_STATE_TRANS))
      {
         if (pucTransferBufferDynamic == NULL)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Receive():  pucTransferBufferDynamic is an undefined pointer.");
            #endif
            break;
         }

         if (pucTransferBufferDynamic[ANTFS_COMMAND_OFFSET] == ANTFS_RESPONSE_SEND_DIRECT_ID)
         {
            // If the index, offset and block size bytes are all 0xFF, then the
            // sent data was rejected by the remote device.
            int i;
            BOOL bAll0xFF = TRUE;
            for (i = 2; i < 8; i++)
            {
               if (pucTransferBufferDynamic[i] != 0xFF)
               {
                  bAll0xFF = FALSE;
                  break;
               }
            }

            if (bAll0xFF)
               return RETURN_REJECT;

            bTransfer = TRUE;
            return RETURN_PASS;
         }
         else
         {
            #if defined(DEBUG_FILE)
               char acString[256];

               SNPRINTF(acString, 256, "ANTFSHostChannel::Receive():  Bad response: [%02X][%02X][%02X][%02X][%02X][%02X][%02X][%02X]",
                  pucTransferBufferDynamic[0],
                  pucTransferBufferDynamic[1],
                  pucTransferBufferDynamic[2],
                  pucTransferBufferDynamic[3],
                  pucTransferBufferDynamic[4],
                  pucTransferBufferDynamic[5],
                  pucTransferBufferDynamic[6],
                  pucTransferBufferDynamic[7]);

               DSIDebug::ThreadWrite(acString);
            #endif
            break;
         }
      }

      if (bRxError)
         bRxError = FALSE;

      if (!bReceivedBurst)
      {
         if ((DSIThread_GetSystemTime() - ulNoRxTime) > SEND_DIRECT_BURST_TIMEOUT)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Receive():  Timeout waiting for burst packets.");
            #endif
            break;
         }
         else
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Receive():  Burst packet not received.  Retrying...");
            #endif
         }
      }
      else
      {
         ulNoRxTime = DSIThread_GetSystemTime();                              // Reset timeout.

         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::Receive():  Burst packets received.  Reset timeout.");
         #endif
      }

      ReportDownloadProgress();
   }

   return RETURN_FAIL;  // Shouldn't reach here.
}

///////////////////////////////////////////////////////////////////////
ANTFSHostChannel::RETURN_STATUS ANTFSHostChannel::AttemptErase(void)
{
   BOOL bStatus = FALSE;
   ULONG ulStartTime;
   UCHAR ucTxRetries;
   ANTFRAMER_RETURN eTxComplete;

   if ((ucFoundDeviceState != REMOTE_DEVICE_STATE_TRANS) && (ucFoundDeviceState != REMOTE_DEVICE_STATE_BUSY))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::Erase():  Remote device is not in transport state.");
      #endif

      return RETURN_FAIL; // Not in the correct mode.
   }

   ulTransferArrayIndex = 0;
   bReceivedResponse = FALSE;
   bRxError = FALSE;

   memset(aucTxBuf,0x00,sizeof(aucTxBuf));
   aucTxBuf[ANTFS_CONNECTION_OFFSET] = ANTFS_COMMAND_RESPONSE_ID;
   aucTxBuf[ANTFS_COMMAND_OFFSET] = ANTFS_ERASE_ID;

   Convert_USHORT_To_Bytes(usTransferDataFileIndex,
                     &aucTxBuf[DATA_INDEX_OFFSET+1],
                     &aucTxBuf[DATA_INDEX_OFFSET]);

   ucTxRetries = 8;
   do
   {
      eTxComplete = pclANT->SendAcknowledgedData(ucChannelNumber, aucTxBuf, ACKNOWLEDGED_TIMEOUT);

      #if defined(DEBUG_FILE)
         if (eTxComplete == ANTFRAMER_FAIL)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptConnect():  Tx error.");
         else if (eTxComplete == ANTFRAMER_TIMEOUT)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptConnect():  Tx timeout.");
      #endif

   } while (eTxComplete == ANTFRAMER_FAIL && --ucTxRetries);

   if (eTxComplete != ANTFRAMER_PASS)
      return RETURN_FAIL;

   ulStartTime = DSIThread_GetSystemTime();

   bNewRxEvent = FALSE;
   ucLinkResponseRetries = ANTFS_RESPONSE_RETRIES;

   do
   {
      DSIThread_MutexLock(&stMutexCriticalSection);

      if ((bNewRxEvent == FALSE) && (*pbCancel == FALSE))
      {
         DSIThread_CondTimedWait(&stCondRxEvent, &stMutexCriticalSection, ERASE_WAIT_TIMEOUT);
      }
      bNewRxEvent = FALSE;
      DSIThread_MutexUnlock(&stMutexCriticalSection);

      if (*pbCancel == TRUE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::Erase():  Stopped.");
         #endif
         return RETURN_STOP;
      }


      if (bReceivedResponse)
      {
         if (aucTransferBufferFixed[ANTFS_COMMAND_OFFSET] == ANTFS_RESPONSE_ERASE_ID)
         {
            if (aucTransferBufferFixed[ERASE_RESPONSE_OFFSET] == ERASE_RESPONSE_OK)
            {
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::Erase():  Erase successful.");
               #endif
               bStatus = TRUE;
            }
            else
            {
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::Erase():  Request rejected.");
               #endif
               bRxError = TRUE;
            }
         }
      }
      #if defined(DEBUG_FILE)
         else
         {
            DSIDebug::ThreadWrite("ANTFSHostChannel::Erase():  Erasing...");
         }
      #endif

      if (bRxError)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::Erase():  Erase failed.");
         #endif

         if (ucLinkResponseRetries == 0)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Erase():  Retries exhausted");
            #endif
            break;
         }
         else
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::Erase():  Retrying");
            #endif
            ucLinkResponseRetries--;
            bRxError = FALSE;
         }
      }

   } while ((bStatus == FALSE) && ((DSIThread_GetSystemTime() - ulStartTime) < cfgParams.ul_cfg_erase_timeout));

   if (!bStatus)
      return RETURN_FAIL;

   return RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
ANTFSHostChannel::RETURN_STATUS ANTFSHostChannel::AttemptAuthenticate(void)
{
   BOOL bStatus = FALSE;
   UCHAR aucTxAuth[8 + TX_PASSWORD_MAX_LENGTH + 8];
   ULONG ulStartTime;
   UCHAR ucTxRetries;
   ANTFRAMER_RETURN eTxComplete;
   BOOL bBeaconStateIncorrect = FALSE;

   if ((ucFoundDeviceState != REMOTE_DEVICE_STATE_AUTH) && (ucFoundDeviceState != REMOTE_DEVICE_STATE_BUSY))
   {
      #if defined(DEBUG_FILE)
       DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptAuthenticate():  Remote device is not in auth state.");
      #endif

      return RETURN_FAIL; // Not in the correct mode.
   }

   ulTransferArrayIndex = 0;
   bReceivedResponse = FALSE;
   bRxError = FALSE;

   memset(aucTxAuth,0x00,sizeof(aucTxAuth));
   aucTxAuth[ANTFS_CONNECTION_OFFSET] = ANTFS_COMMAND_RESPONSE_ID;
   aucTxAuth[ANTFS_COMMAND_OFFSET] = ANTFS_AUTHENTICATE_ID;
   aucTxAuth[AUTH_COMMAND_TYPE_OFFSET] = ucAuthType;
   aucTxAuth[AUTH_FRIENDLY_NAME_LENGTH_OFFSET] = ucTxPasswordLength;

   Convert_ULONG_To_Bytes(ulHostSerialNumber,
                           &aucTxAuth[AUTH_HOST_SERIAL_NUMBER_OFFSET+3],
                           &aucTxAuth[AUTH_HOST_SERIAL_NUMBER_OFFSET+2],
                           &aucTxAuth[AUTH_HOST_SERIAL_NUMBER_OFFSET+1],
                           &aucTxAuth[AUTH_HOST_SERIAL_NUMBER_OFFSET]);

   memcpy(&aucTxAuth[8], aucTxPassword, ucTxPasswordLength);

   if (ucTxPasswordLength) //Do this to not leave the last 8 bytes of the password in the host Tx buffer, but we are not required to do it.
      ucTxPasswordLength = ucTxPasswordLength + 8;

#if defined(ACCESS_POINT)
   ucTxRetries = 2;
#else
   ucTxRetries = 8;
#endif
   do
   {
      if (ucTxPasswordLength)
         eTxComplete = pclANT->SendANTFSTransfer(ucChannelNumber, (UCHAR*)NULL, (UCHAR*)NULL, aucTxAuth, 8+ ucTxPasswordLength, ACKNOWLEDGED_TIMEOUT, (ULONG*)NULL);
      else
         eTxComplete = pclANT->SendAcknowledgedData(ucChannelNumber, aucTxAuth, ACKNOWLEDGED_TIMEOUT);

      #if defined(DEBUG_FILE)
         if (eTxComplete == ANTFRAMER_FAIL)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptAuthenticate():  Tx error.");
         else if (eTxComplete == ANTFRAMER_TIMEOUT)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptAuthenticate():  Tx timeout.");
      #endif
   } while (eTxComplete == ANTFRAMER_FAIL && --ucTxRetries);

   if (eTxComplete != ANTFRAMER_PASS)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptAuthenticate():  Tx failed.");
      #endif

      return RETURN_FAIL;
   }

   ulStartTime = DSIThread_GetSystemTime();

   bNewRxEvent = FALSE;
#if defined(ACCESS_POINT)
   ucLinkResponseRetries = 3; // Current client implementation does not retry, so there is no point in keeping waiting longer
#else
   ucLinkResponseRetries = ANTFS_RESPONSE_RETRIES;
#endif

   do
   {
      DSIThread_MutexLock(&stMutexCriticalSection);

      if ((bNewRxEvent == FALSE) && (*pbCancel == FALSE))
      {
         DSIThread_CondTimedWait(&stCondRxEvent, &stMutexCriticalSection, BROADCAST_TIMEOUT);
      }
      bNewRxEvent = FALSE;
      DSIThread_MutexUnlock(&stMutexCriticalSection);

      if (*pbCancel == TRUE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptAuthenticate():  Stopped.");
         #endif
         return RETURN_STOP;
      }

      if (ucFoundDeviceState != REMOTE_DEVICE_STATE_BUSY)
      {
         if (!((usFoundANTFSManufacturerID == 2) && (usFoundANTFSDeviceType == 1)))  //Make an exception for the FR405
            bBeaconStateIncorrect = TRUE;
      }

      if (bRxError || bBeaconStateIncorrect)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptAuthenticate():  Rx error.");
         #endif
         if (ucLinkResponseRetries == 0)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptAuthenticate():  Retries exhausted");
            #endif
            return RETURN_FAIL;
         }
         else
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptAuthenticate():  Retrying"); // Not actual retries, just wait longer!
            #endif
            ucLinkResponseRetries--;
            bRxError = FALSE;
            bBeaconStateIncorrect = FALSE;
         }
      }

      if (bReceivedResponse)
      {
         if (aucTransferBufferFixed[ANTFS_COMMAND_OFFSET] == ANTFS_RESPONSE_AUTH_ID)
         {
            if ((pucResponseBuffer != NULL) && (pucResponseBufferSize != NULL))
            {
               if (*pucResponseBufferSize > aucTransferBufferFixed[AUTH_PASSWORD_LENGTH_OFFSET])   //set the reponse length up to the max
                  *pucResponseBufferSize = aucTransferBufferFixed[AUTH_PASSWORD_LENGTH_OFFSET];

               memcpy(pucResponseBuffer, &aucTransferBufferFixed[8], *pucResponseBufferSize);
            }

            if (aucTransferBufferFixed[AUTH_RESPONSE_OFFSET] == AUTH_RESPONSE_ACCEPT)
            {
               bStatus = TRUE;
            }
            else if  (aucTransferBufferFixed[AUTH_RESPONSE_OFFSET] == AUTH_RESPONSE_REJECT)
            {
               return RETURN_REJECT;
            }
            else
            {
               return RETURN_NA;
            }
         }
      }
      #if defined(DEBUG_FILE)
         else
         {
            DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptAuthenticate():  Authenticating...");
         }
      #endif
   } while ((bStatus == FALSE) && ((DSIThread_GetSystemTime() - ulStartTime) < ulAuthResponseTimeout));

   if (!bStatus)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::AttemptAuthenticate():  Timeout.");
      #endif
      return RETURN_FAIL;
   }

   return RETURN_PASS;
}

///////////////////////////////////////////////////////////////////////
// Returns: TRUE if the message has been handled by ANT-FS host, FALSE otherwise
BOOL ANTFSHostChannel::ANTProtocolEventProcess(UCHAR ucChannel_, UCHAR ucMessageCode_)
{
#if 0
   #if defined(DEBUG_FILE)
      UCHAR aucString[256];
      SNPRINTF((char *)aucString,256, "ANTProtocolEventProcess():  Message 0x%02X",  ucMessageCode_);
      DSIDebug::ThreadWrite((char *)aucString);
   #endif
#endif

   if ((ucMessageCode_ == MESG_RESPONSE_EVENT_ID) && (ucChannel_ == ucChannelNumber))
   {
      #if defined(DEBUG_FILE)
         UCHAR aucString[256];
         SNPRINTF((char *) aucString, 256, "ANTFSHostChannel::ANTProtocolEventProcess():  MESG_RESPONSE_EVENT_ID - 0x%02X", aucResponseBuf[1]);
         DSIDebug::ThreadWrite((char *) aucString);
      #endif

      if (aucResponseBuf[1] == MESG_BURST_DATA_ID)
      {
         if (aucResponseBuf[2] != RESPONSE_NO_ERROR)
         {
            #if defined(DEBUG_FILE)
               UCHAR aucString1[256];
               SNPRINTF((char *) aucString1, 256, "ANTFSHostChannel::ANTProtocolEventProcess():  Burst transfer error:  0x%02X.", aucResponseBuf[2]);
               DSIDebug::ThreadWrite((char *) aucString1);
            #endif
            bTxError = TRUE;
         }
      }
      else if (aucResponseBuf[1] == MESG_ACKNOWLEDGED_DATA_ID)
      {
         if(aucResponseBuf[2] == TRANSFER_IN_PROGRESS)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::ANTProtocolEventProcess(): TRANSFER_IN_PROGRESS");
            #endif
            bForceFullInit = TRUE;  // Force assign/unassign channel as per "Uplink communication failure on slave channel" issue documented on AP2 Module Rev History
         }
      }
   }

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Returns: TRUE if the message has been handled by ANT-FS host, FALSE otherwise
BOOL ANTFSHostChannel::ANTChannelEventProcess(UCHAR ucChannel_, UCHAR ucMessageCode_)
{

   BOOL bIsValid = TRUE;

#if 0
   #if defined(DEBUG_FILE)
      UCHAR aucString[256];
      SNPRINTF((char *)aucString,256, "ANTFSHostChannel::ANTChannelEventProcess():  Channel %u, Event 0x%02X", ucChannel_, ucMessageCode_);
      DSIDebug::ThreadWrite((char *)aucString);
   #endif
#endif

   // Check that we're getting a message from the correct channel.
   if((ucChannel_ != ucChannelNumber) || ((aucRxBuf[0] & CHANNEL_NUMBER_MASK)  != ucChannelNumber))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Message received on wrong channel.");
      #endif
      return FALSE;
   }


   switch (ucMessageCode_)
   {
      case EVENT_RX_BROADCAST:
      case EVENT_RX_ACKNOWLEDGED:
#if 0
         #if defined(DEBUG_FILE)
            {
               UCHAR aucString[256];

               if (ucMessageCode_ == EVENT_RX_BROADCAST)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  EVENT_RX_BROADCAST");
               else
                  DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  EVENT_RX_ACKNOWLEDGED");

               SNPRINTF((char *)aucString,256, "ANTFSHostChannel::ANTChannelEventProcess():  Broadcast message received: {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X}.", aucRxBuf[0], aucRxBuf[1], aucRxBuf[2], aucRxBuf[3], aucRxBuf[4], aucRxBuf[5], aucRxBuf[6], aucRxBuf[7], aucRxBuf[8]);
               DSIDebug::ThreadWrite((char *)aucString);
            }
         #endif
#endif

         if (aucRxBuf[ANTFS_CONNECTION_OFFSET + 1] != ANTFS_BEACON_ID)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Invalid connection ID.");
            #endif
            bIsValid = FALSE;
         }
         else if (!bFoundDevice)
         {
            //Decode the payload
            usFoundBeaconPeriod = 0;
            switch (aucRxBuf[STATUS1_OFFSET + 1] & BEACON_PERIOD_MASK)
            {
               case BEACON_PERIOD_0_5_HZ:
                  usFoundBeaconPeriod = 65535;
                  break;

               case BEACON_PERIOD_1_HZ:
                  usFoundBeaconPeriod = 32768;
                  break;

               case BEACON_PERIOD_2_HZ:
                  usFoundBeaconPeriod = 16384;
                  break;

               case BEACON_PERIOD_4_HZ:
                  usFoundBeaconPeriod = 8192;
                  break;

               case BEACON_PERIOD_8_HZ:
                  usFoundBeaconPeriod = 4096;
                  break;

               case BEACON_PERIOD_KEEP:   // Custom period
                  usFoundBeaconPeriod = usTheMessagePeriod;
                  break;

               default:
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Invalid beacon period.");
                  #endif
                  bIsValid = FALSE;
                  break;
            }

            if (bIsValid)
            {
               if (aucRxBuf[STATUS1_OFFSET + 1] & DATA_AVAILABLE_FLAG_MASK)
                  bFoundDeviceHasData = TRUE;
               else
                  bFoundDeviceHasData = FALSE;

               if (aucRxBuf[STATUS1_OFFSET + 1] & UPLOAD_ENABLED_FLAG_MASK)
                  bFoundDeviceUploadEnabled = TRUE;
               else
                  bFoundDeviceUploadEnabled = FALSE;

               if (aucRxBuf[STATUS1_OFFSET + 1] & PAIRING_AVAILABLE_FLAG_MASK)
                  bFoundDeviceInPairingMode = TRUE;
               else
                  bFoundDeviceInPairingMode = FALSE;

               ucFoundDeviceAuthenticationType = aucRxBuf[AUTHENTICATION_TYPE_OFFSET + 1];

               usFoundANTFSDeviceType = Convert_Bytes_To_USHORT(aucRxBuf[DEVICE_TYPE_OFFSET_HIGH + 1], aucRxBuf[DEVICE_TYPE_OFFSET_LOW + 1]);

               usFoundANTFSManufacturerID = Convert_Bytes_To_USHORT(aucRxBuf[MANUFACTURER_ID_OFFSET_HIGH + 1], aucRxBuf[MANUFACTURER_ID_OFFSET_LOW + 1]);

               //Set Found device paramenters here
               stFoundDeviceParameters.usDeviceType = Convert_Bytes_To_USHORT(aucRxBuf[DEVICE_TYPE_OFFSET_HIGH + 1], aucRxBuf[DEVICE_TYPE_OFFSET_LOW + 1]);

               stFoundDeviceParameters.usManufacturerID = Convert_Bytes_To_USHORT(aucRxBuf[MANUFACTURER_ID_OFFSET_HIGH + 1], aucRxBuf[MANUFACTURER_ID_OFFSET_LOW + 1]);

               stFoundDeviceParameters.ucAuthenticationType = aucRxBuf[AUTHENTICATION_TYPE_OFFSET + 1];
               stFoundDeviceParameters.ucStatusByte1 = aucRxBuf[STATUS1_OFFSET + 1];
               stFoundDeviceParameters.ucStatusByte2 = aucRxBuf[STATUS2_OFFSET + 1];

            }
         }

         if (bIsValid)
         {
            USHORT usDeviceType;
            USHORT usManufID;
            ULONG ulHostID;

            UCHAR ucCurrentDeviceState = (aucRxBuf[STATUS2_OFFSET + 1] & REMOTE_DEVICE_STATE_MASK);

            usDeviceType = Convert_Bytes_To_USHORT(aucRxBuf[DEVICE_TYPE_OFFSET_HIGH + 1], aucRxBuf[DEVICE_TYPE_OFFSET_LOW + 1]);

            usManufID = Convert_Bytes_To_USHORT(aucRxBuf[MANUFACTURER_ID_OFFSET_HIGH + 1], aucRxBuf[MANUFACTURER_ID_OFFSET_LOW + 1]);

            ulHostID = Convert_Bytes_To_ULONG ( aucRxBuf[HOST_ID_OFFSET + 4],
                                                aucRxBuf[HOST_ID_OFFSET + 3],
                                                aucRxBuf[HOST_ID_OFFSET + 2],
                                                aucRxBuf[HOST_ID_OFFSET + 1]);


            if (ucCurrentDeviceState > REMOTE_DEVICE_STATE_LAST_VALID)
            {
               bIsValid = FALSE;

               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Current device state not in valid range.");
               #endif
            }
            else if ((ulHostSerialNumber != ulHostID) && ((usDeviceType != usFoundANTFSDeviceType) || (usManufID != usFoundANTFSManufacturerID)))
            {
               bIsValid = FALSE;
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Host ID does not match this host and the device descriptors match do not match the found device.");
               #endif
            }
            else if (((eANTFSState >= ANTFS_HOST_STATE_TRANSPORT) && (ucCurrentDeviceState < REMOTE_DEVICE_STATE_TRANS)) ||
                ((eANTFSState >= ANTFS_HOST_STATE_CONNECTED) && (ucCurrentDeviceState == REMOTE_DEVICE_STATE_LINK)))
            {
               if (ucStrikes > 0)
               {
                  ucStrikes--;

                  if (ucStrikes == 0)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Device state changed; connection lost.");
                     #endif

                     pclANT->CloseChannel(ucChannelNumber,ANT_CLOSE_TIMEOUT);

                     bIsValid = FALSE;
                     DSIThread_MutexLock(&stMutexCriticalSection);
                     if (eANTFSRequest < ANTFS_REQUEST_CONNECTION_LOST)
                     {
                        eANTFSRequest = ANTFS_REQUEST_CONNECTION_LOST;
                        DSIThread_CondSignal(&stCondRequest);
                     }
                     DSIThread_MutexUnlock(&stMutexCriticalSection);
                  }
                  #if defined(DEBUG_FILE)
                     else
                     {
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Device state changed; retrying.");
                     }
                  #endif

               }
            }
            else
            {
               ucStrikes = STRIKE_COUNT;

               ucFoundDeviceState = ucCurrentDeviceState;

               if (ucFoundDeviceState == REMOTE_DEVICE_STATE_TRANS)
               {
                  ucTransportBeaconTicks++;
                  stFoundDeviceParameters.ucStatusByte1 = aucRxBuf[STATUS1_OFFSET + 1];
               }

               if (ucFoundDeviceState == REMOTE_DEVICE_STATE_AUTH)
               {
                  ulFoundBeaconHostID = Convert_Bytes_To_ULONG(aucRxBuf[HOST_ID_OFFSET + 4],
                                                               aucRxBuf[HOST_ID_OFFSET + 3],
                                                               aucRxBuf[HOST_ID_OFFSET + 2],
                                                               aucRxBuf[HOST_ID_OFFSET + 1]);

               }
            }
         }

         DSIThread_MutexLock(&stMutexCriticalSection);
         bFoundDevice = TRUE;                         // Signal that we have found the device.
         bReceivedBurst = FALSE;
         bFoundDeviceIsValid = bIsValid;
         bNewRxEvent = TRUE;
         DSIThread_CondSignal(&stCondRxEvent);
         DSIThread_MutexUnlock(&stMutexCriticalSection);
         break;

      case EVENT_RX_BURST_PACKET:
#if 0
         #if defined(DEBUG_FILE)
            {
               UCHAR aucString[256];

               SNPRINTF((char *)aucString, 256, "ANTFSHostChannel::ANTChannelEventProcess():  EVENT_RX_BURST_PACKET received (%lu:%lu): {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X}.", ulPacketCount, ulTransferArrayIndex / 8, aucRxBuf[0], aucRxBuf[1], aucRxBuf[2], aucRxBuf[3], aucRxBuf[4], aucRxBuf[5], aucRxBuf[6], aucRxBuf[7], aucRxBuf[8]);

               DSIDebug::ThreadWrite((char *)aucString);
            }
         #endif
#endif
         if (!bRxError)
         {
            if ((aucRxBuf[0] & SEQUENCE_NUMBER_MASK) == 0)  // Start of a burst.
            {
               // Check that the message is coming from an ANTFS Device.
               if (aucRxBuf[ANTFS_CONNECTION_OFFSET + 1] != ANTFS_BEACON_ID)
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Invalid ID.");
                  #endif
                  bRxError = TRUE;
               }
               else
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Burst started");
                  #endif

                  ucFoundDeviceState = (aucRxBuf[STATUS2_OFFSET + 1] & REMOTE_DEVICE_STATE_MASK);
               }
               ulPacketCount = 1;
               bReceivedResponse = FALSE;
            }
            else
            {
               // Response packet.
               if (ulPacketCount == 1)
               {
                  // Check that the response contains the correct ID
                  if (aucRxBuf[ANTFS_CONNECTION_OFFSET + 1] != ANTFS_COMMAND_RESPONSE_ID)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Invalid ID2.");
                     #endif
                     bRxError = TRUE;
                  }
                  else
                  {
                     switch (aucRxBuf[ANTFS_COMMAND_OFFSET + 1])
                     {
                        case ANTFS_RESPONSE_AUTH_ID:
                        case ANTFS_RESPONSE_UPLOAD_ID:
                        case ANTFS_RESPONSE_ERASE_ID:
                        case ANTFS_RESPONSE_UPLOAD_COMPLETE_ID:
                           memcpy(aucTransferBufferFixed, &aucRxBuf[1], 8);  // Always copy the response to the start of the transfer buffer.

                           memset(&aucTransferBufferFixed[8],0x00,sizeof(aucTransferBufferFixed)-8);   //Zero the rest of the receive buffer
                           ulTransferArrayIndex = 8;

                           pucTransferBuffer = aucTransferBufferFixed;

                           if (aucRxBuf[0] & SEQUENCE_LAST_MESSAGE)
                           {
                              bReceivedResponse = TRUE;

                              #if defined(DEBUG_FILE)
                                 DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Reception of burst complete. (0)");
                              #endif
                           }
                           #if defined(DEBUG_FILE)
                              else
                              {
                                 DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Receiving burst... (0)");
                              }
                           #endif

                           ulTransferBufferSize = sizeof(aucTransferBufferFixed);
                           break;

                        case ANTFS_RESPONSE_DOWNLOAD_SMALL_ID:
                           if (ulTransferArrayIndex == 0)
                           {
                              ulTransferTotalBytesRemaining = Convert_Bytes_To_ULONG(0,
                                        0,
                                        aucRxBuf[DATA_BLOCK_SIZE_OFFSET_HIGH + 1],
                                        aucRxBuf[DATA_BLOCK_SIZE_OFFSET_LOW + 1]);

                              ulTransferBufferSize = ulTransferTotalBytesRemaining + 16 + 8;  //+16 for the extra packet at the start and rounding up to the next 8 byte packet, + 8 for additional buffering

                              if (pucTransferBufferDynamic)
                                 delete[] pucTransferBufferDynamic;

                              try
                              {
                                 pucTransferBufferDynamic = new UCHAR[ulTransferBufferSize];

                                 if (pucTransferBufferDynamic == NULL)
                                    throw "Memory allocation failure!";
                              }
                              catch(...)
                              {
                                 bRxError = TRUE;
                                 pucTransferBuffer = (UCHAR*) NULL;

                                 #if defined(DEBUG_FILE)
                                    DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Unable to allocate memory for download. (1)");
                                 #endif
                                 break;
                              }

                              ulTransferArrayIndex = 8;
                              pucTransferBuffer = pucTransferBufferDynamic;
                           }

                           if (pucTransferBufferDynamic == NULL)
                           {
                              #if defined(DEBUG_FILE)
                                 DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess(): pucTransferBufferDynamic is NULL, ignoring transfer Packet");
                              #endif
                              break;
                           }
                           memcpy(pucTransferBufferDynamic, &aucRxBuf[1], 8);  // Always copy the response to the start of the download buffer.

                           if (aucRxBuf[0] & SEQUENCE_LAST_MESSAGE)
                           {
                              bReceivedResponse = TRUE;

                              #if defined(DEBUG_FILE)
                                 DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Reception of burst complete. (1)");
                              #endif
                           }
                           #if defined(DEBUG_FILE)
                              else
                              {
                                 DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Receiving burst... (1)");
                              }
                           #endif
                           break;

                        case ANTFS_RESPONSE_DOWNLOAD_BIG_ID:
                           if (ulTransferArrayIndex == 0) //if this is the first packed of the this download session
                           {
                              ulTransferArrayIndex = 8;

                              pucTransferBuffer = aucTransferBufferFixed;            //Use the fixed buffer for now
                              ulTransferBufferSize = sizeof(aucTransferBufferFixed);
                           }

                           ulTransferBytesInBlock = Convert_Bytes_To_ULONG(aucRxBuf[DOWNLOAD_RESPONSE_BLOCK_SIZE_OFFSET + 3 + 1],
                                        aucRxBuf[DOWNLOAD_RESPONSE_BLOCK_SIZE_OFFSET + 2 + 1],
                                        aucRxBuf[DOWNLOAD_RESPONSE_BLOCK_SIZE_OFFSET + 1 + 1],
                                        aucRxBuf[DOWNLOAD_RESPONSE_BLOCK_SIZE_OFFSET + 1]);

                           memcpy(pucTransferBuffer, &aucRxBuf[1], 8);  // Always copy the response to the start of the transfer buffer.

                           if (aucRxBuf[0] & SEQUENCE_LAST_MESSAGE)
                           {
                              bRxError = TRUE;

                              #if defined(DEBUG_FILE)
                                 DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Premature end of burst transfer.");
                              #endif
                           }
                           break;

                        case ANTFS_RESPONSE_SEND_DIRECT_ID:
                           {

                              ulTransferArrayIndex = Convert_Bytes_To_ULONG(0,
                                        0,
                                        aucRxBuf[DATA_OFFSET_OFFSET_HIGH + 1],
                                        aucRxBuf[DATA_OFFSET_OFFSET_LOW + 1]);

                              ulTransferArrayIndex *= 8;

                              ulTransferTotalBytesRemaining = Convert_Bytes_To_ULONG(0,
                                        0,
                                        aucRxBuf[DATA_BLOCK_SIZE_OFFSET_HIGH + 1],
                                        aucRxBuf[DATA_BLOCK_SIZE_OFFSET_LOW + 1]);


                              ulTransferTotalBytesRemaining *= 8;
                           }

                           if (ulTransferArrayIndex == 0)
                           {
                              ulTransferBufferSize = DIRECT_TRANSFER_SIZE + 16;

                              if (pucTransferBufferDynamic)
                                 delete[] pucTransferBufferDynamic;

                              try
                              {
                                 pucTransferBufferDynamic = new UCHAR[ulTransferBufferSize];

                                 if (pucTransferBufferDynamic == NULL)
                                    throw "Memory allocation failure!";
                              }
                              catch(...)
                              {
                                 bRxError = TRUE;
                                 pucTransferBuffer = (UCHAR*) NULL;

                                 #if defined(DEBUG_FILE)
                                    DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Unable to allocate memory for download. (3)");
                                 #endif
                                 break;
                              }
                              pucTransferBuffer = pucTransferBufferDynamic;
                           }
                           else if (pucTransferBufferDynamic == NULL)
                           {
                              bRxError = TRUE;

                              #if defined(DEBUG_FILE)
                                 DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  ANTFS_RESPONSE_SEND_DIRECT index error.");
                              #endif
                              break;
                           }

                           ulTransferArrayIndex += 8;       // This offset due to the response packet.

                           memcpy(pucTransferBufferDynamic, &aucRxBuf[1], 8);  // Always copy the response to the start of the download buffer.

                           if (aucRxBuf[0] & SEQUENCE_LAST_MESSAGE)
                           {
                              bReceivedResponse = TRUE;

                              #if defined(DEBUG_FILE)
                                 DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Reception of burst complete. (2)");
                              #endif
                           }
                           #if defined(DEBUG_FILE)
                              else
                              {
                                 DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Receiving burst... (1)");
                              }
                           #endif
                           break;

                        default:
                           bRxError = TRUE;              // BIG DL/UL and everything else are not handled right now so we fail out here.
                           break;
                     }
                  }
               }
               else if ((ulPacketCount == 2) && (pucTransferBuffer != NULL) && (pucTransferBuffer[ANTFS_COMMAND_OFFSET] == ANTFS_RESPONSE_DOWNLOAD_BIG_ID) &&(pucTransferBuffer[ANTFS_CONNECTION_OFFSET] == ANTFS_COMMAND_RESPONSE_ID))
               {
                  if (ulTransferArrayIndex == 8)
                  {
                     ulTransferTotalBytesRemaining = Convert_Bytes_To_ULONG(aucRxBuf[DOWNLOAD_RESPONSE_FILE_SIZE_OFFSET + 3 + 1],
                               aucRxBuf[DOWNLOAD_RESPONSE_FILE_SIZE_OFFSET + 2 + 1],
                               aucRxBuf[DOWNLOAD_RESPONSE_FILE_SIZE_OFFSET + 1 + 1],
                               aucRxBuf[DOWNLOAD_RESPONSE_FILE_SIZE_OFFSET + 1]);

                     if (ulTransferByteSize && ulTransferByteSize < ulTransferTotalBytesRemaining)   //If our desired download size is less than the file size, set it.
                        ulTransferTotalBytesRemaining = ulTransferByteSize;

                     ulTransferBufferSize = ulTransferTotalBytesRemaining + 24 + 8 + 8;//+24 for the extra packets at the start and rounding up to the next 8 byte packet, + 8 for additional buffering +8 more for CRC
                     #if defined(DEBUG_FILE)
                        {
                           char szString[256];

                           SNPRINTF(szString, 256, "ANTFSHostChannel::ANTChannelEventProcess():  ulTransferBufferSize = %lu.", ulTransferBufferSize);
                           DSIDebug::ThreadWrite(szString);
                        }
                     #endif

                     if (pucTransferBufferDynamic)
                        delete[] pucTransferBufferDynamic;

                     try
                     {
                        pucTransferBufferDynamic = new UCHAR[ulTransferBufferSize];

                        if (pucTransferBufferDynamic == NULL)
                           throw "Memory allocation failure!";
                     }
                     catch(...)
                     {
                        bRxError = TRUE;
                        ulTransferArrayIndex = 0;
                        pucTransferBuffer = (UCHAR*) NULL;

                        #if defined(DEBUG_FILE)
                           DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Unable to allocate memory for download. (2)");
                        #endif
                        break;
                     }

                     memcpy(pucTransferBufferDynamic, aucTransferBufferFixed, 8);  // Copy over the first packet that we saved temporarily.

                     ulTransferArrayIndex = 16;
                     pucTransferBuffer = pucTransferBufferDynamic;
                  }



                  if (pucTransferBufferDynamic == NULL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess(): pucTransferBufferDynamic is NULL, ignoring transfer Packet");
                     #endif
                     break;
                  }

                  memcpy(&pucTransferBuffer[8], &aucRxBuf[1], 8);  // Always copy the response to the start of the transfer buffer.


                  ULONG ulReceivedDataOffset = Convert_Bytes_To_ULONG(aucRxBuf[DOWNLOAD_RESPONSE_DATA_OFFSET_OFFSET + 3 + 1],
                               aucRxBuf[DOWNLOAD_RESPONSE_DATA_OFFSET_OFFSET + 2 + 1],
                               aucRxBuf[DOWNLOAD_RESPONSE_DATA_OFFSET_OFFSET + 1 + 1],
                               aucRxBuf[DOWNLOAD_RESPONSE_DATA_OFFSET_OFFSET + 1]);

                  if ((pucTransferBuffer[DOWNLOAD_RESPONSE_OFFSET] == DOWNLOAD_RESPONSE_OK) && ((ulReceivedDataOffset - ulTransferDataOffset) != (ulTransferArrayIndex - 16)))
                  {
                     bRxError = TRUE;
                     ulTransferArrayIndex = 0;
                     pucTransferBuffer = (UCHAR*) NULL;

                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  DL offset does not match desired DL offset.");
                     #endif
                     break;
                  }


                  if (aucRxBuf[0] & SEQUENCE_LAST_MESSAGE)
                  {
                     bReceivedResponse = TRUE;

                     #if defined(DEBUG_FILE)
                     {
                        char szString[256];

                        SNPRINTF(szString, 256, "ANTFSHostChannel::ANTChannelEventProcess():  Reception of burst complete. (%lu).", ulTransferArrayIndex);
                        DSIDebug::ThreadWrite(szString);
                     }
                     #endif
                  }
               }
               else if ((ulTransferArrayIndex + 8) <= ulTransferBufferSize)
               {
                  #if defined(DEBUG_FILE_V1)
                  {
                     char szString[256];

                     SNPRINTF(szString, 256, "ANTFSHostChannel::ANTChannelEventProcess():  ulTransferArrayIndex = %lu.", ulTransferArrayIndex);
                     DSIDebug::ThreadWrite(szString);
                  }
                  #endif

                  if (pucTransferBuffer == NULL)
                  {
                     #if defined(DEBUG_FILE)
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess(): pucTransferBuffer is NULL, ignoring transfer Packet");
                     #endif
                     break;
                  }
                  memcpy(&pucTransferBuffer[ulTransferArrayIndex], &aucRxBuf[1], 8);

                  ulTransferArrayIndex += 8;

                  if (aucRxBuf[0] & SEQUENCE_LAST_MESSAGE)
                  {
                     bReceivedResponse = TRUE;

                     #if defined(DEBUG_FILE)
                     {
                        char szString[256];

                        SNPRINTF(szString, 256, "ANTFSHostChannel::ANTChannelEventProcess():  Reception of burst complete. (%lu).", ulTransferArrayIndex);
                        DSIDebug::ThreadWrite(szString);
                     }

                     #endif
                  }
                  #if defined(DEBUG_FILE_V1)
                     else
                     {
                        DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Receiving burst... (n)");
                     }
                  #endif
               }
               else
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  Burst Buffer Overflow");
                  #endif

                  bRxError = TRUE;
               }

               ulPacketCount++;
            }
         }

         DSIThread_MutexLock(&stMutexCriticalSection);
         bReceivedBurst = TRUE;
         bNewRxEvent = TRUE;
         DSIThread_CondSignal(&stCondRxEvent);
         DSIThread_MutexUnlock(&stMutexCriticalSection);

         #if defined(DEBUG_FILE_V1)
            DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  bReceivedBurst = TRUE");
         #endif

         break;

      case EVENT_TRANSFER_RX_FAILED:
         DSIThread_MutexLock(&stMutexCriticalSection);
         bRxError = TRUE;
         bReceivedBurst = FALSE;
         bReceivedResponse = FALSE;
         DSIThread_CondSignal(&stCondRxEvent);
         DSIThread_MutexUnlock(&stMutexCriticalSection);

         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  EVENT_TRANSFER_RX_FAILED");
         #endif

         break;

     case EVENT_RX_FAIL_GO_TO_SEARCH:
       if (eANTFSState == ANTFS_HOST_STATE_SEARCHING)
       {
          DSIThread_MutexLock(&stMutexCriticalSection);
          bFoundDeviceIsValid = FALSE;          // Signal that this is not a valid beacon
          bFoundDevice = TRUE;                         // Signal that we have found the device to allow the search loop to exit.
          bReceivedBurst = FALSE;
          bNewRxEvent = TRUE;
          DSIThread_CondSignal(&stCondRxEvent);
          DSIThread_MutexUnlock(&stMutexCriticalSection);

          #if defined(DEBUG_FILE)
             DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess(): EVENT_RX_FAIL_GO_TO_SEARCH");
          #endif
       }
       break;

      case EVENT_RX_SEARCH_TIMEOUT:
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  EVENT_RX_SEARCH_TIMEOUT");
         #endif

         if (eANTFSState == ANTFS_HOST_STATE_SEARCHING)
         {
            pclANT->OpenChannel(ucChannelNumber);              // Re-open the channel.
         }
         else if ((eANTFSState == ANTFS_HOST_STATE_DOWNLOADING) || (eANTFSState == ANTFS_HOST_STATE_UPLOADING) || (eANTFSState == ANTFS_HOST_STATE_SENDING) || (eANTFSState == ANTFS_HOST_STATE_RECEIVING))
         {
            pclANT->OpenChannel(ucChannelNumber);              // Re-open the channel.

            DSIThread_MutexLock(&stMutexCriticalSection);
            bRxError = TRUE;
            bTxError = TRUE;                                   // Can no longer transmit data, either.
            bReceivedBurst = FALSE;
            DSIThread_CondSignal(&stCondRxEvent);
            DSIThread_MutexUnlock(&stMutexCriticalSection);
         }
         else if (eANTFSState >= ANTFS_HOST_STATE_CONNECTED)
         {
            DSIThread_MutexLock(&stMutexCriticalSection);
            if (eANTFSRequest < ANTFS_REQUEST_CONNECTION_LOST)
            {
               eANTFSRequest = ANTFS_REQUEST_CONNECTION_LOST;
               DSIThread_CondSignal(&stCondRequest);
            }
            DSIThread_MutexUnlock(&stMutexCriticalSection);
         }
         // any other states we don't report the timeout
         break;

      case EVENT_TRANSFER_TX_COMPLETED:
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  EVENT_TRANSFER_TX_COMPLETED");
         #endif
         break;

      case EVENT_TRANSFER_TX_FAILED:
         bTxError = TRUE;
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  EVENT_TRANSFER_TX_FAILED");
         #endif
         break;

      case EVENT_TRANSFER_TX_START:
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  EVENT_TRANSFER_TX_START");
         #endif
         break;

      case EVENT_CHANNEL_CLOSED:
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHostChannel::ANTChannelEventProcess():  EVENT_CHANNEL_CLOSED");
         #endif
         break;

      default:
         break;
   }

   return TRUE;  // message has been handled, do not pass to application
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHostChannel::Blackout(ULONG ulDeviceID_, USHORT usManufacturerID_, USHORT usDeviceType_, USHORT usBlackoutTime_)
//BOOL ANTFSHost::IgnoreDevice(USHORT usBlackoutTime_)
{
   BOOL bRetVal = TRUE;
   IGNORE_LIST_ITEM stListItem;
   IGNORE_LIST_ITEM *pstListItem;

   if (usBlackoutTime_ == 0)
      return FALSE;

   DSIThread_MutexLock(&stMutexIgnoreListAccess);

   stListItem.usID = (USHORT)ulDeviceID_;
   stListItem.usManufacturerID = usManufacturerID_;
   stListItem.usDeviceType = usDeviceType_;

   pstListItem = (IGNORE_LIST_ITEM *) bsearch(&stListItem, astIgnoreList, usListIndex, sizeof(IGNORE_LIST_ITEM), &ListCompare);

   if (pstListItem != NULL)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadPrintf("ANTFSHostChannel::Blackout():  Device [%u-%u-%u](t:%u) on list already, setting time to %u",
            pstListItem->usID, pstListItem->usManufacturerID, pstListItem->usDeviceType, pstListItem->usTimeout, usBlackoutTime_);
      #endif

      // If the ID is already in the list, set the blackout time.
      pstListItem->usTimeout = usBlackoutTime_;
   }
   else
   {
      // If the ID is not on the list, add it.
      if (usListIndex < MAX_IGNORE_LIST_SIZE)
      {
         astIgnoreList[usListIndex].usID = (USHORT)ulDeviceID_;
         astIgnoreList[usListIndex].usManufacturerID = usManufacturerID_;
         astIgnoreList[usListIndex].usDeviceType = usDeviceType_;
         astIgnoreList[usListIndex].usTimeout = usBlackoutTime_;

         #if defined(DEBUG_FILE)
            DSIDebug::ThreadPrintf("ANTFSHostChannel::Blackout():  Adding Device [%u-%u-%u](t:%u) at index %u",
               astIgnoreList[usListIndex].usID, astIgnoreList[usListIndex].usManufacturerID, astIgnoreList[usListIndex].usDeviceType, astIgnoreList[usListIndex].usTimeout, usListIndex);
         #endif

         usListIndex++;
         qsort(astIgnoreList, usListIndex, sizeof(IGNORE_LIST_ITEM), &ListCompare);
      }
      else
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadPrintf("ANTFSHostChannel::Blackout():  Adding Device Error: List is full");
         #endif

         bRetVal = FALSE;                                   // Can't add any more devices to the list.
      }
   }

   DSIThread_MutexUnlock(&stMutexIgnoreListAccess);

   return bRetVal;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHostChannel::RemoveBlackout(ULONG ulDeviceID_, USHORT usManufacturerID_, USHORT usDeviceType_)
{
   #if defined(DEBUG_FILE)
      DSIDebug::ThreadPrintf("ANTFSHostChannel::RemoveBlackout():  Request to remove [%lu-%u-%u](t:%u)", ulDeviceID_, usManufacturerID_, usDeviceType_);
   #endif

   BOOL bRetVal = TRUE;
   IGNORE_LIST_ITEM stListItem;
   IGNORE_LIST_ITEM *pstListItem;

   DSIThread_MutexLock(&stMutexIgnoreListAccess);

   stListItem.usID = (USHORT)ulDeviceID_;
   stListItem.usManufacturerID = usManufacturerID_;
   stListItem.usDeviceType = usDeviceType_;

   pstListItem = (IGNORE_LIST_ITEM *) bsearch(&stListItem, astIgnoreList, usListIndex, sizeof(IGNORE_LIST_ITEM), &ListCompare);

   if (pstListItem != NULL)
   {
      USHORT usIndex = (USHORT)((pstListItem - astIgnoreList) / sizeof(IGNORE_LIST_ITEM));

      #if defined(DEBUG_FILE)
         DSIDebug::ThreadPrintf("ANTFSHostChannel::RemoveBlackout():  Removing Device [%u-%u-%u](t:%u) at index %u", pstListItem->usID, pstListItem->usManufacturerID, pstListItem->usDeviceType, pstListItem->usTimeout, usIndex);
      #endif

      // If the ID is found on the list, set the blackout time to 0 and remove it.
      astIgnoreList[usIndex].usTimeout = 0;

      for (usIndex = usIndex + 1; usIndex < usListIndex; usIndex++)
         astIgnoreList[usIndex - 1] = astIgnoreList[usIndex];

      usListIndex--;
   }
   else
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::RemoveBlackout():  Remove device error: not found.");
      #endif

      // If the ID is not on the list.
      bRetVal = FALSE;
   }

   DSIThread_MutexUnlock(&stMutexIgnoreListAccess);

   return bRetVal;
}
///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::ClearBlackoutList(void)
{
   DSIThread_MutexLock(&stMutexIgnoreListAccess);
      usListIndex = 0;

      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::ClearBlackoutList():  Blackout list cleared.");
      #endif
   DSIThread_MutexUnlock(&stMutexIgnoreListAccess);
}

///////////////////////////////////////////////////////////////////////
static int ListCompare(const void *puvKeyVal, const void *puvDatum)
{
   IGNORE_LIST_ITEM* pstKeyVal = (IGNORE_LIST_ITEM *) puvKeyVal;
   IGNORE_LIST_ITEM* pstDatum = (IGNORE_LIST_ITEM *) puvDatum;
   LONG diff;
   if( (diff = (LONG)pstKeyVal->usID - pstDatum->usID) != 0
      || (diff = (LONG)pstKeyVal->usDeviceType - pstDatum->usDeviceType) != 0
      || (diff = (LONG)pstKeyVal->usManufacturerID - pstDatum->usManufacturerID) != 0)
   {
      return (diff < 0)? -1 : 1;
   }

   return 0;   //Otherwise they are equal
}

///////////////////////////////////////////////////////////////////////
// Frequency:  1 Hz
///////////////////////////////////////////////////////////////////////
DSI_THREAD_RETURN ANTFSHostChannel::QueueTimerStart(void *pvParameter_)
{
   ((ANTFSHostChannel *)pvParameter_)->QueueTimerCallback();

   return 0;
}

///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::QueueTimerCallback(void)
{
   USHORT usCounter = 0;

   if(!bTimerThreadInitDone)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadInit("Timer");
      #endif
      bTimerThreadInitDone = TRUE;
   }

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHostChannel::TimerQueueTimerCall():  Entering ignore list critical section.");
   #endif

   DSIThread_MutexLock(&stMutexIgnoreListAccess);

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("Updating ignore list...");
   #endif

   while (usCounter < usListIndex)
   {
      if ((astIgnoreList[usCounter].usTimeout > 0) && (astIgnoreList[usCounter].usTimeout != MAX_USHORT))
         astIgnoreList[usCounter].usTimeout--;

      if (astIgnoreList[usCounter].usTimeout == 0) // Remove the List Item if it has expired.
      {
         USHORT usInsideLoopCounter;

         #if defined(DEBUG_FILE)
            {
               UCHAR aucString[256];
               SNPRINTF((char *)aucString,256, "Removing %u %u %u from ignore list.", astIgnoreList[usCounter].usID, astIgnoreList[usCounter].usDeviceType, astIgnoreList[usCounter].usManufacturerID);
               DSIDebug::ThreadWrite((char *)aucString);
            }
         #endif

         for (usInsideLoopCounter = usCounter + 1; usInsideLoopCounter < usListIndex; usInsideLoopCounter++)
            astIgnoreList[usInsideLoopCounter - 1] = astIgnoreList[usInsideLoopCounter];

         usListIndex--;

      }
      else
      {
         #if defined(DEBUG_FILE)
            {
               UCHAR aucString[256];
               SNPRINTF((char *)aucString,256, "%u %u %u is on ignore list. %us left.", astIgnoreList[usCounter].usID, astIgnoreList[usCounter].usDeviceType, astIgnoreList[usCounter].usManufacturerID, astIgnoreList[usCounter].usTimeout);
               DSIDebug::ThreadWrite((char *)aucString);
            }
         #endif

         usCounter++;
      }
   }

   DSIThread_MutexUnlock(&stMutexIgnoreListAccess);

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHostChannel::TimerQueueTimerCall():  Left ignore list critical section.");
   #endif

      /*
   if (++usSerialWatchdog > ANTFS_SERIAL_WATCHDOG_COUNT)
   {
      usSerialWatchdog = 0;
      HandleSerialError(); //! This is where we would want to potentially add a serial watch dog, if it makes sense to do so in the future
   }
*/
}

///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::HandleSerialError(void)
{
   // We ended up here because we did not receive the expected response to a serial message
   // Most likely, ANT was in the wrong state, so attempt to close the channel.
   // No errors raised from here, as we do not know what state we are in.

   UCHAR ucChannelStatus = 0;

   if(pclANT->CloseChannel(ucChannelNumber, ANT_CLOSE_TIMEOUT) != TRUE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::HandleSerialError():  Failed to close channel.");
      #endif
   }

   if(pclANT->UnAssignChannel(ucChannelNumber, MESSAGE_TIMEOUT) != TRUE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::HandleSerialError():  Failed to unassign channel.");
      #endif
   }

   // The following is just for information purposes...
   if(pclANT->GetChannelStatus(ucChannelNumber, &ucChannelStatus, MESSAGE_TIMEOUT) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHostChannel::HandleSerialError():  Failed ANT_GetChannelStatus().");
      #endif
   }
   #if defined(DEBUG_FILE)
   else if ((ucChannelStatus & STATUS_CHANNEL_STATE_MASK) != STATUS_UNASSIGNED_CHANNEL)
   {
      char szString[256];
      SNPRINTF(szString, 256, "ANTFSHostChannel::HandleSerialError():  Channel state... 0x%x.", ucChannelStatus);
      DSIDebug::ThreadWrite(szString);
   }
   #endif

   DSIThread_MutexLock(&stMutexCriticalSection);

   eANTFSRequest = ANTFS_REQUEST_SERIAL_ERROR_HANDLED;   // Reset state machine next
   DSIThread_CondSignal(&stCondRequest);

   DSIThread_MutexUnlock(&stMutexCriticalSection);
}


///////////////////////////////////////////////////////////////////////
// This function is called to increment the frequency stale counter
///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::IncFreqStaleCount(UCHAR ucInc)
{
   USHORT usTemp = (USHORT)ucTransportFrequencyStaleCount + (USHORT)ucInc;

   if (usTemp >= MAX_STALE_COUNT)
      ucTransportFrequencyStaleCount = MAX_STALE_COUNT;
   else
      ucTransportFrequencyStaleCount = (UCHAR)usTemp;
}

///////////////////////////////////////////////////////////////////////
// This function is called to populate/re-populate the frequency table randomly
///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::PopulateTransportFreqTable(void)
{
   UCHAR aucTemp[TRANSPORT_FREQUENCY_LIST_SIZE];
   int i = 0;

   ucTransportFrequencyStaleCount = 0;
   ucCurrentTransportFreqElement = 0;

   srand((unsigned int)time((time_t*)NULL));
   memcpy (aucTemp, aucTransportFrequencyList, TRANSPORT_FREQUENCY_LIST_SIZE);

   for (i=0; i < TRANSPORT_FREQUENCY_LIST_SIZE; i++)
   {
      UCHAR ucNewValue = (UCHAR)( 1 + (rand() % (TRANSPORT_FREQUENCY_LIST_SIZE-1)) );
      UCHAR j = 0;

      while (ucNewValue)
      {
         j = (j+1)%TRANSPORT_FREQUENCY_LIST_SIZE;
         if (aucTemp[j] != 0xFF)
         {
            ucNewValue--;
         }
      }

      aucFrequencyTable[i] = aucTemp[j];
      aucTemp[j] = 0xFF;
   }
}

///////////////////////////////////////////////////////////////////////
// This function is called to check if we need to change frequencies and return the new frequncy
///////////////////////////////////////////////////////////////////////
UCHAR ANTFSHostChannel::CheckForNewTransportFreq(void)
{
   if (ucTransportFrequencyStaleCount >= MAX_STALE_COUNT)
   {
      ucCurrentTransportFreqElement = ((ucCurrentTransportFreqElement+1) % TRANSPORT_FREQUENCY_LIST_SIZE);
     ucTransportFrequencyStaleCount = 0;
   }

   return aucFrequencyTable[ucCurrentTransportFreqElement];
}


///////////////////////////////////////////////////////////////////////
static int DeviceParametersItemCompare(const void *pvItem1, const void *pvItem2)
{
   if (((DEVICE_PARAMETERS_ITEM *) pvItem1)->usHandle < ((DEVICE_PARAMETERS_ITEM *) pvItem2)->usHandle)
      return -1;

   if (((DEVICE_PARAMETERS_ITEM *) pvItem1)->usHandle == ((DEVICE_PARAMETERS_ITEM *) pvItem2)->usHandle)
      return 0;

   return 1;
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHostChannel::IsDeviceMatched(ANTFS_DEVICE_PARAMETERS *psDeviceParameters_, BOOL bPartialID_)
{
   USHORT usDeviceIndex = 0;
   ULONG ulIDMask;

   if (usDeviceListSize == 0)
      return TRUE;                                          // There are no devices in the list, so any device is a match.

   if(bPartialID_)
      ulIDMask = MAX_USHORT;
   else
      ulIDMask = MAX_ULONG;

   while (usDeviceIndex < usDeviceListSize)
   {
      DEVICE_PARAMETERS_ITEM *psSearchDeviceItem = &asDeviceParametersList[usDeviceIndex];
      if (
            ((psDeviceParameters_->ulDeviceID & psSearchDeviceItem->sDeviceSearchMask.ulDeviceID & ulIDMask) == (psSearchDeviceItem->sDeviceParameters.ulDeviceID & psSearchDeviceItem->sDeviceSearchMask.ulDeviceID & ulIDMask))
               &&
            ((psDeviceParameters_->usManufacturerID & psSearchDeviceItem->sDeviceSearchMask.usManufacturerID) == (psSearchDeviceItem->sDeviceParameters.usManufacturerID & psSearchDeviceItem->sDeviceSearchMask.usManufacturerID))
               &&
            ((psDeviceParameters_->usDeviceType & psSearchDeviceItem->sDeviceSearchMask.usDeviceType) == (psSearchDeviceItem->sDeviceParameters.usDeviceType & psSearchDeviceItem->sDeviceSearchMask.usDeviceType))
               &&
            ((psDeviceParameters_->ucAuthenticationType & psSearchDeviceItem->sDeviceSearchMask.ucAuthenticationType) == (psSearchDeviceItem->sDeviceParameters.ucAuthenticationType & psSearchDeviceItem->sDeviceSearchMask.ucAuthenticationType))
               &&
            ((psDeviceParameters_->ucStatusByte1 & psSearchDeviceItem->sDeviceSearchMask.ucStatusByte1) == (psSearchDeviceItem->sDeviceParameters.ucStatusByte1 & psSearchDeviceItem->sDeviceSearchMask.ucStatusByte1))
               &&
            ((psDeviceParameters_->ucStatusByte2 & psSearchDeviceItem->sDeviceSearchMask.ucStatusByte2) == (psSearchDeviceItem->sDeviceParameters.ucStatusByte2 & psSearchDeviceItem->sDeviceSearchMask.ucStatusByte2))
         )
         return TRUE;


      usDeviceIndex++;
   }

   return FALSE;
}
///////////////////////////////////////////////////////////////////////
void ANTFSHostChannel::AddResponse(ANTFS_HOST_RESPONSE eResponse_)
{
   DSIThread_MutexLock(&stMutexResponseQueue);
   clResponseQueue.AddResponse(eResponse_);
   DSIThread_CondSignal(&stCondWaitForResponse);
   DSIThread_MutexUnlock(&stMutexResponseQueue);
}

///////////////////////////////////////////////////////////////////////
//Returns a response if there is one ready, otherwise waits the specified time for one to occur
ANTFS_HOST_RESPONSE ANTFSHostChannel::WaitForResponse(ULONG ulMilliseconds_)
{
   ANTFS_HOST_RESPONSE stResponse = ANTFS_HOST_RESPONSE_NONE;

   if (bKillThread == TRUE)
      return ANTFS_HOST_RESPONSE_NONE;

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
               stResponse = ANTFS_HOST_RESPONSE_NONE;
               break;

            case DSI_THREAD_EOTHER:
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::WaitForResponse():  CondTimedWait() Failed!");
               #endif
               stResponse = ANTFS_HOST_RESPONSE_NONE;
               break;

            default:
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHostChannel::WaitForResponse():  Error  Unknown...");
               #endif
               stResponse = ANTFS_HOST_RESPONSE_NONE;
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


