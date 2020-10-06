/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#include "dsi_thread.h"
#include "config.h"
#include "dsi_convert.h"

#include "antfs_host.hpp"

#include "dsi_debug.hpp"
#if defined(DEBUG_FILE)
   #include "macros.h"
#endif


//////////////////////////////////////////////////////////////////////////////////
// Private Definitions
//////////////////////////////////////////////////////////////////////////////////
#define DEFAULT_CHANNEL_NUMBER   ((UCHAR) 0)

//////////////////////////////////////////////////////////////////////////////////
// Public Functions
//////////////////////////////////////////////////////////////////////////////////
ANTFSHost::ANTFSHost()
{
   bInitFailed = FALSE;
   bOpen = FALSE;

   eWrappedState = ANTFS_STATE_IDLE;

   ulHostSerialNumber = 0;

   hANTFSThread = (DSI_THREAD_ID)NULL;                 // Handle for the ANTFS thread

   bKillThread = FALSE;
   bANTFSThreadRunning = FALSE;

   pclMsgHandler = new DSIANTDevicePolling();
   pclHost = new ANTFSHostChannel();

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

   if (DSIThread_CondInit(&stCondWaitForResponse) != DSI_THREAD_ENONE)
   {
      bInitFailed = TRUE;
   }
}

///////////////////////////////////////////////////////////////////////
ANTFSHost::~ANTFSHost()
{
   this->Close();

   delete pclHost;
   delete pclMsgHandler;

   if (bInitFailed == FALSE)
   {
      DSIThread_MutexDestroy(&stMutexCriticalSection);
      DSIThread_MutexDestroy(&stMutexResponseQueue);
      DSIThread_CondDestroy(&stCondANTFSThreadExit);
      DSIThread_CondDestroy(&stCondWaitForResponse);
   }
}

///////////////////////////////////////////////////////////////////////
UCHAR ANTFSHost::GetVersion(UCHAR *pucVersionString_, UCHAR ucBufferSize_)
{
   return pclHost->GetVersion(pucVersionString_, ucBufferSize_);
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHost::Init(UCHAR ucUSBDeviceNum_, USHORT usBaudRate_)
{
   return InitHost(ucUSBDeviceNum_, usBaudRate_, FALSE);
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHost::Init(void)
{
   return InitHost(0, 0 , TRUE);
}

///////////////////////////////////////////////////////////////////////
void ANTFSHost::GetCurrentConfig( ANTFS_CONFIG_PARAMETERS* pCfg_ )
{
   pclHost->GetCurrentConfig( pCfg_ );
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHost::SetCurrentConfig( const ANTFS_CONFIG_PARAMETERS* pCfg_ )
{
   return pclHost->SetCurrentConfig( pCfg_ );
}

///////////////////////////////////////////////////////////////////////
void ANTFSHost::SetChannelID(UCHAR ucDeviceType_, UCHAR ucTransmissionType_)
{
   pclHost->SetChannelID(ucDeviceType_, ucTransmissionType_);
}

///////////////////////////////////////////////////////////////////////
void ANTFSHost::SetMessagePeriod(USHORT usMessagePeriod_)
{
   pclHost->SetChannelPeriod(usMessagePeriod_);
}

///////////////////////////////////////////////////////////////////////
void ANTFSHost::SetNetworkkey(UCHAR ucNetworkkey[])
{
   pclHost->SetNetworkKey(0, ucNetworkkey);
}

///////////////////////////////////////////////////////////////////////
void ANTFSHost::SetProximitySearch(UCHAR ucSearchThreshold_)
{
   pclHost->SetProximitySearch(ucSearchThreshold_);
}

///////////////////////////////////////////////////////////////////////
ANTFS_STATE ANTFSHost::GetStatus(void)
{
   ANT_USB_STATE eUSBState;
   ANTFS_HOST_STATE eANTFSState;

   DSIThread_MutexLock(&stMutexCriticalSection);

   eUSBState = pclMsgHandler->GetStatus();
   switch(eUSBState)
   {
      case ANT_USB_STATE_OFF:
      case ANT_USB_STATE_IDLE:
         eWrappedState = ANTFS_STATE_IDLE;
         break;
      case ANT_USB_STATE_IDLE_POLLING_USB:
         eWrappedState = ANTFS_STATE_IDLE_POLLING_USB;
         break;
      case ANT_USB_STATE_OPEN:
         eWrappedState = ANTFS_STATE_OPEN;
         break;
   }

   if(eWrappedState < ANTFS_STATE_OPEN)
   {
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return eWrappedState;
   }

   eANTFSState = pclHost->GetStatus();
   switch(eANTFSState)
   {
      case ANTFS_HOST_STATE_OFF:
      case ANTFS_HOST_STATE_IDLE:
         // Keep state...
         break;
      case ANTFS_HOST_STATE_REQUESTING_SESSION: // We should not get this one
         // Keep as Open
         break;
      case ANTFS_HOST_STATE_DISCONNECTING:
         eWrappedState = ANTFS_STATE_DISCONNECTING;
         break;
      case ANTFS_HOST_STATE_SEARCHING:
         eWrappedState = ANTFS_STATE_SEARCHING;
         break;
      case ANTFS_HOST_STATE_CONNECTED:
         eWrappedState = ANTFS_STATE_CONNECTED;
         break;
      case ANTFS_HOST_STATE_AUTHENTICATING:
         eWrappedState = ANTFS_STATE_AUTHENTICATING;
         break;
      case ANTFS_HOST_STATE_TRANSPORT:
         eWrappedState = ANTFS_STATE_TRANSPORT;
         break;
      case ANTFS_HOST_STATE_DOWNLOADING:
         eWrappedState = ANTFS_STATE_DOWNLOADING;
         break;
      case ANTFS_HOST_STATE_UPLOADING:
         eWrappedState = ANTFS_STATE_UPLOADING;
         break;
      case ANTFS_HOST_STATE_ERASING:
         eWrappedState = ANTFS_STATE_ERASING;
         break;
      case ANTFS_HOST_STATE_SENDING:
         eWrappedState = ANTFS_STATE_SENDING;
         break;
      case ANTFS_HOST_STATE_RECEIVING:
         eWrappedState = ANTFS_STATE_RECEIVING;
         break;
   }

   DSIThread_MutexUnlock(&stMutexCriticalSection);
   return eWrappedState;
}

///////////////////////////////////////////////////////////////////////
UCHAR ANTFSHost::GetConnectedDeviceBeaconAntfsState(void)
{
    return pclHost->GetConnectedDeviceBeaconAntfsState();
}

///////////////////////////////////////////////////////////////////////
void ANTFSHost::Close(void)
{
   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHost::Close():  Closing ANTFS...");
   #endif

   DSIThread_MutexLock(&stMutexCriticalSection);

   if(bOpen == FALSE)
   {
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return;
   }

   // Stop the response thread
   bKillThread = TRUE;

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHost::Close():  SetEvent(stCondWaitForResponse).");
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
            DSIDebug::ThreadWrite("ANTFSHost::Close():  Killing thread.");
         #endif

         if (DSIThread_CondTimedWait(&stCondANTFSThreadExit, &stMutexCriticalSection, 9000) != DSI_THREAD_ENONE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHost::Close():  Thread not dead.");
               DSIDebug::ThreadWrite("ANTFSHost::Close():  Forcing thread termination...");
            #endif
            DSIThread_DestroyThread(hANTFSThread);
         }
         else
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHost::Close():  Thread terminated successfully.");
            #endif
         }
      }

      DSIThread_ReleaseThreadID(hANTFSThread);
      hANTFSThread = (DSI_THREAD_ID)NULL;
   }

   pclMsgHandler->RemoveMessageProcessor((DSIANTMessageProcessor*) pclHost);
   pclMsgHandler->Close();

   bOpen = FALSE;
   eWrappedState = ANTFS_STATE_IDLE;

   DSIThread_MutexUnlock(&stMutexCriticalSection);

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHost::Close():  Closed.");
   #endif
}

///////////////////////////////////////////////////////////////////////
void ANTFSHost::Cancel(void)
{
   pclHost->Cancel();
}

//////////////////////////////////////////////////////////////////////////////////
// ANTFS Link Layer
//////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
USHORT ANTFSHost::AddSearchDevice(ANTFS_DEVICE_PARAMETERS *pstDeviceSearchMask_, ANTFS_DEVICE_PARAMETERS *pstDeviceParameters_)
{
   return pclHost->AddSearchDevice(pstDeviceSearchMask_, pstDeviceParameters_);
}

///////////////////////////////////////////////////////////////////////
void ANTFSHost::RemoveSearchDevice(USHORT usHandle_)
{
   pclHost->RemoveSearchDevice(usHandle_);
}

///////////////////////////////////////////////////////////////////////
void ANTFSHost::ClearSearchDeviceList(void)
{
   pclHost->ClearSearchDeviceList();
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHost::SearchForDevice(UCHAR ucSearchRadioFrequency_, UCHAR ucConnectedRadioFrequency_, USHORT usRadioChannelID_, BOOL bUseRequestPage_)
{
   DSIThread_MutexLock(&stMutexCriticalSection);
   if ((pclMsgHandler->GetStatus() < ANT_USB_STATE_OPEN))
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHost::SearchForDevice():  Not in correct state.");
      #endif
      DSIThread_MutexUnlock(&stMutexCriticalSection);
      return ANTFS_RETURN_FAIL;
   }
   DSIThread_MutexUnlock(&stMutexCriticalSection);

   return pclHost->SearchForDevice(ucSearchRadioFrequency_, ucConnectedRadioFrequency_, usRadioChannelID_, bUseRequestPage_);
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHost::GetFoundDeviceParameters(ANTFS_DEVICE_PARAMETERS *pstDeviceParameters_, UCHAR *aucFriendlyName_, UCHAR *pucBufferSize_)
{
   return pclHost->GetFoundDeviceParameters(pstDeviceParameters_, aucFriendlyName_, pucBufferSize_);
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHost::GetFoundDeviceChannelID(USHORT *pusDeviceNumber_, UCHAR *pucDeviceType_, UCHAR *pucTransmitType_)
{
   return pclHost->GetFoundDeviceChannelID(pusDeviceNumber_, pucDeviceType_, pucTransmitType_);
}


//////////////////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHost::Authenticate(UCHAR ucAuthenticationType_, UCHAR *pucAuthenticationString_, UCHAR ucLength_, UCHAR *pucResponseBuffer_, UCHAR *pucResponseBufferSize_, ULONG ulResponseTimeout_)
{
   return pclHost->Authenticate(ucAuthenticationType_, pucAuthenticationString_, ucLength_, pucResponseBuffer_, pucResponseBufferSize_, ulResponseTimeout_);
}

//////////////////////////////////////////////////////////////////////////////////
// Always On Transport Layer
//////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHost::Disconnect(USHORT usBlackoutTime_, UCHAR ucDisconnectType_, UCHAR ucTimeDuration_, UCHAR ucAppSpecificDuration_)
{
   return pclHost->Disconnect(usBlackoutTime_, ucDisconnectType_, ucTimeDuration_, ucAppSpecificDuration_);
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHost::Blackout(ULONG ulDeviceID_, USHORT usManufacturerID_, USHORT usDeviceType_, USHORT usBlackoutTime_)
{
    return pclHost->Blackout(ulDeviceID_, usManufacturerID_, usDeviceType_, usBlackoutTime_);
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHost::RemoveBlackout(ULONG ulDeviceID_, USHORT usManufacturerID_, USHORT usDeviceType_)
{
    return pclHost->RemoveBlackout(ulDeviceID_, usManufacturerID_, usDeviceType_);
}

///////////////////////////////////////////////////////////////////////
void ANTFSHost::ClearBlackoutList(void)
{
    pclHost->ClearBlackoutList();
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHost::Download(USHORT usFileIndex_, ULONG ulDataOffset_, ULONG ulMaxDataLength_, ULONG ulMaxBlockSize_)
{
   return pclHost->Download(usFileIndex_, ulDataOffset_, ulMaxDataLength_, ulMaxBlockSize_);
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHost::Upload(USHORT usFileIndex_, ULONG ulDataOffset_, ULONG ulDataLength_, void *pvData_, BOOL bForceOffset_, ULONG ulMaxBlockSize_)
{
   return pclHost->Upload(usFileIndex_, ulDataOffset_, ulDataLength_, pvData_, bForceOffset_, ulMaxBlockSize_);
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHost::ManualTransfer(USHORT usFileIndex_, ULONG ulDataOffset_, ULONG ulDataLength_, void *pvData_)
{
   return pclHost->ManualTransfer(usFileIndex_, ulDataOffset_, ulDataLength_, pvData_);
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHost::GetDownloadStatus(ULONG *pulByteProgress_, ULONG *pulTotalLength_)
{
   return pclHost->GetDownloadStatus(pulByteProgress_, pulTotalLength_);
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHost::GetTransferData(ULONG *pulDataSize_ , void *pvData_)
{
   return pclHost->GetTransferData(pulDataSize_, pvData_);
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHost::RecoverTransferData(ULONG *pulDataSize_ , void *pvData_)
{
   return pclHost->RecoverTransferData(pulDataSize_, pvData_);
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHost::GetUploadStatus(ULONG *pulByteProgress_, ULONG *pulTotalLength_)
{
   return pclHost->GetUploadStatus(pulByteProgress_, pulTotalLength_);
}

///////////////////////////////////////////////////////////////////////
ANTFS_RETURN ANTFSHost::EraseData(USHORT usDataFileIndex_)
{
   return pclHost->EraseData(usDataFileIndex_);
}

//////////////////////////////////////////////////////////////////////
BOOL ANTFSHost::EnablePing(BOOL bEnable_)
{
   return pclHost->EnablePing(bEnable_);
}

#if defined(DEBUG_FILE)
///////////////////////////////////////////////////////////////////////
void ANTFSHost::SetDebug(BOOL bDebugOn_, const char *pcDirectory_)
{
   pclMsgHandler->SetDebug(bDebugOn_, pcDirectory_);
}
#endif


//////////////////////////////////////////////////////////////////////////////////
// Private Functions
//////////////////////////////////////////////////////////////////////////////////
DSI_THREAD_RETURN ANTFSHost::ANTFSThreadStart(void *pvParameter_)
{
   #if defined(DEBUG_FILE)
   DSIDebug::ThreadInit("ANTFSHostWrapper");
   #endif

   ((ANTFSHost *)pvParameter_)->ANTFSThread();

   return 0;
}
///////////////////////////////////////////////////////////////////////
// ANTFS Task Thread
///////////////////////////////////////////////////////////////////////
void ANTFSHost::ANTFSThread(void)
{
   ANTFS_RESPONSE eWrappedResponse;
   ANT_USB_RESPONSE eUSBResponse;
   ANTFS_HOST_RESPONSE eANTFSResponse;

   bANTFSThreadRunning = TRUE;

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHost::ANTFSThread():  Awaiting Responses..");
   #endif

   while (bKillThread == FALSE)
   {
      eWrappedResponse = ANTFS_RESPONSE_NONE;
      eUSBResponse = ANT_USB_RESPONSE_NONE;
      eANTFSResponse = ANTFS_HOST_RESPONSE_NONE;

      eANTFSResponse = pclHost->WaitForResponse(100);
      switch(eANTFSResponse)
      {
         case ANTFS_HOST_RESPONSE_NONE:
            eWrappedResponse = ANTFS_RESPONSE_NONE;
            break;
         case ANTFS_HOST_RESPONSE_INIT_PASS:
            eWrappedResponse = ANTFS_RESPONSE_NONE;
            break;
         case ANTFS_HOST_RESPONSE_SERIAL_FAIL:
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHost::ANTFSThread():  Serial fail from ANTFSHostChannel ..");
            #endif
            eWrappedResponse = ANTFS_RESPONSE_SERIAL_FAIL;
            break;
         case ANTFS_HOST_RESPONSE_REQUEST_SESSION_FAIL:
            // Should not get this one, as Request Session is not included from this wrapper
            break;
         case ANTFS_HOST_RESPONSE_CONNECT_PASS:
            eWrappedResponse = ANTFS_RESPONSE_CONNECT_PASS;
            break;
         case ANTFS_HOST_RESPONSE_DISCONNECT_PASS:
            eWrappedResponse = ANTFS_RESPONSE_DISCONNECT_PASS;
            break;
         case ANTFS_HOST_RESPONSE_CONNECTION_LOST:
            eWrappedResponse = ANTFS_RESPONSE_CONNECTION_LOST;
            break;
         case ANTFS_HOST_RESPONSE_AUTHENTICATE_NA:
            eWrappedResponse = ANTFS_RESPONSE_AUTHENTICATE_NA;
            break;
         case ANTFS_HOST_RESPONSE_AUTHENTICATE_PASS:
            eWrappedResponse = ANTFS_RESPONSE_AUTHENTICATE_PASS;
            break;
         case ANTFS_HOST_RESPONSE_AUTHENTICATE_REJECT:
            eWrappedResponse = ANTFS_RESPONSE_AUTHENTICATE_REJECT;
            break;
         case ANTFS_HOST_RESPONSE_AUTHENTICATE_FAIL:
            eWrappedResponse = ANTFS_RESPONSE_AUTHENTICATE_FAIL;
            break;
         case ANTFS_HOST_RESPONSE_DOWNLOAD_PASS:
            eWrappedResponse = ANTFS_RESPONSE_DOWNLOAD_PASS;
            break;
         case ANTFS_HOST_RESPONSE_DOWNLOAD_REJECT:
            eWrappedResponse = ANTFS_RESPONSE_DOWNLOAD_REJECT;
            break;
         case ANTFS_HOST_RESPONSE_DOWNLOAD_INVALID_INDEX:
            eWrappedResponse = ANTFS_RESPONSE_DOWNLOAD_INVALID_INDEX;
            break;
         case ANTFS_HOST_RESPONSE_DOWNLOAD_FILE_NOT_READABLE:
            eWrappedResponse = ANTFS_RESPONSE_DOWNLOAD_FILE_NOT_READABLE;
            break;
         case ANTFS_HOST_RESPONSE_DOWNLOAD_NOT_READY:
            eWrappedResponse =  ANTFS_RESPONSE_DOWNLOAD_NOT_READY;
            break;
         case ANTFS_HOST_RESPONSE_DOWNLOAD_FAIL:
            eWrappedResponse = ANTFS_RESPONSE_DOWNLOAD_FAIL;
            break;
         case ANTFS_HOST_RESPONSE_DOWNLOAD_CRC_REJECTED:
            eWrappedResponse = ANTFS_RESPONSE_DOWNLOAD_CRC_REJECTED;
            break;
         case ANTFS_HOST_RESPONSE_UPLOAD_PASS:
            eWrappedResponse = ANTFS_RESPONSE_UPLOAD_PASS;
            break;
         case ANTFS_HOST_RESPONSE_UPLOAD_REJECT:
            eWrappedResponse = ANTFS_RESPONSE_UPLOAD_REJECT;
            break;
         case ANTFS_HOST_RESPONSE_UPLOAD_INVALID_INDEX:
            eWrappedResponse = ANTFS_RESPONSE_UPLOAD_INVALID_INDEX;
            break;
         case ANTFS_HOST_RESPONSE_UPLOAD_FILE_NOT_WRITEABLE:
            eWrappedResponse = ANTFS_RESPONSE_UPLOAD_FILE_NOT_WRITEABLE;
            break;
         case ANTFS_HOST_RESPONSE_UPLOAD_INSUFFICIENT_SPACE:
            eWrappedResponse = ANTFS_RESPONSE_UPLOAD_INSUFFICIENT_SPACE;
            break;
         case ANTFS_HOST_RESPONSE_UPLOAD_FAIL:
            eWrappedResponse = ANTFS_RESPONSE_UPLOAD_FAIL;
            break;
         case ANTFS_HOST_RESPONSE_ERASE_PASS:
            eWrappedResponse = ANTFS_RESPONSE_ERASE_PASS;
            break;
         case ANTFS_HOST_RESPONSE_ERASE_FAIL:
            eWrappedResponse = ANTFS_RESPONSE_ERASE_FAIL;
            break;
         case ANTFS_HOST_RESPONSE_MANUAL_TRANSFER_PASS:
            eWrappedResponse = ANTFS_RESPONSE_MANUAL_TRANSFER_PASS;
            break;
         case ANTFS_HOST_RESPONSE_MANUAL_TRANSFER_TRANSMIT_FAIL:
            eWrappedResponse = ANTFS_RESPONSE_MANUAL_TRANSFER_TRANSMIT_FAIL;
            break;
         case ANTFS_HOST_RESPONSE_MANUAL_TRANSFER_RESPONSE_FAIL:
            eWrappedResponse = ANTFS_RESPONSE_MANUAL_TRANSFER_RESPONSE_FAIL;
            break;
         case ANTFS_HOST_RESPONSE_CANCEL_DONE:
            eWrappedResponse = ANTFS_RESPONSE_CANCEL_DONE;
            break;
      }

      if(eWrappedResponse != ANTFS_RESPONSE_NONE)
      {
         AddResponse(eWrappedResponse);
      }

      if (bKillThread)
         break;

      eUSBResponse = pclMsgHandler->WaitForResponse(100);
      switch(eUSBResponse)
      {
         case ANT_USB_RESPONSE_OPEN_FAIL:  // Using PollUSB, so should not get this event
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHost::ANTFSThread():  Invalid response ANT_USB_RESPONSE_OPEN_FAIL...");
            #endif
         case ANT_USB_RESPONSE_NONE:
            eWrappedResponse = ANTFS_RESPONSE_NONE;
            break;
         case ANT_USB_RESPONSE_OPEN_PASS:
            eWrappedResponse = ANTFS_RESPONSE_OPEN_PASS;
            if(ulHostSerialNumber == 0)
            {
               ulHostSerialNumber = pclMsgHandler->GetSerialNumber();
               pclHost->SetSerialNumber(ulHostSerialNumber);
            }
            break;
         case ANT_USB_RESPONSE_SERIAL_FAIL:
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("ANTFSHost::ANTFSThread():  Serial fail from DSIANTDevice ..");
            #endif
            if(eWrappedResponse != ANTFS_RESPONSE_SERIAL_FAIL) // We do this to avoid reporting a serial failure twice
               eWrappedResponse = ANTFS_RESPONSE_SERIAL_FAIL;
            else
               eWrappedResponse = ANTFS_RESPONSE_NONE;
            break;
      }

      if(eWrappedResponse != ANTFS_RESPONSE_NONE)
      {
         AddResponse(eWrappedResponse);
      }
   } // while()

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("ANTFSHost::ANTFSThread():  Exiting thread.");
   #endif

   DSIThread_MutexLock(&stMutexCriticalSection);
      bANTFSThreadRunning = FALSE;
      DSIThread_CondSignal(&stCondANTFSThreadExit);
   DSIThread_MutexUnlock(&stMutexCriticalSection);
}

///////////////////////////////////////////////////////////////////////
BOOL ANTFSHost::InitHost(UCHAR ucUSBDeviceNum_, USHORT usBaudRate_, BOOL bAutoInit_)
{
   if (bInitFailed == TRUE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("ANTFSHost::InitHost():  bInitFailed == TRUE");
      #endif
      return FALSE;
   }

   //Ensure we are closed before continuing
   this->Close();

   bKillThread = FALSE;

   if(bAutoInit_)
   {
      if(pclMsgHandler->Open() == FALSE) // Poll USB
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHost::InitHost():  Failed to init pclMsgHandler.");
         #endif
         return FALSE;
      }
   }
   else
   {
      if(pclMsgHandler->Open(ucUSBDeviceNum_, usBaudRate_) == FALSE) // Poll USB
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("ANTFSHost::InitHost():  Failed to init pclMsgHandler.");
         #endif
         return FALSE;
      }
   }

   pclMsgHandler->AddMessageProcessor(DEFAULT_CHANNEL_NUMBER, (DSIANTMessageProcessor*) pclHost);   // Initializes channel object

   if (hANTFSThread == NULL)
   {
      hANTFSThread = DSIThread_CreateThread(&ANTFSHost::ANTFSThreadStart, this);
      if (hANTFSThread == NULL)
         return FALSE;
   }

   DSIThread_MutexLock(&stMutexResponseQueue);
   clResponseQueue.Clear();
   DSIThread_MutexUnlock(&stMutexResponseQueue);

   bOpen = TRUE;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
void ANTFSHost::AddResponse(ANTFS_RESPONSE eResponse_)
{
   DSIThread_MutexLock(&stMutexResponseQueue);
   clResponseQueue.AddResponse(eResponse_);
   DSIThread_CondSignal(&stCondWaitForResponse);
   DSIThread_MutexUnlock(&stMutexResponseQueue);
}

///////////////////////////////////////////////////////////////////////
//Returns a response if there is one ready, otherwise waits the specified time for one to occur
ANTFS_RESPONSE ANTFSHost::WaitForResponse(ULONG ulMilliseconds_)
{
   ANTFS_RESPONSE stResponse = ANTFS_RESPONSE_NONE;

   if (bKillThread == TRUE)
      return ANTFS_RESPONSE_NONE;

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
               stResponse = ANTFS_RESPONSE_NONE;
               break;

            case DSI_THREAD_EOTHER:
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHost::WaitForResponse(): CondTimedWait() Failed!");
               #endif
               stResponse = ANTFS_RESPONSE_NONE;
               break;

            default:
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("ANTFSHost::WaitForResponse(): Error Unknown");
               #endif
               stResponse = ANTFS_RESPONSE_NONE;
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
