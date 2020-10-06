/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#include "version.h"
#include "dsi_framer_ant.hpp"
#include "dsi_serial_generic.hpp"
#include "dsi_thread.h"

#include "dsi_debug.hpp"
#if defined(DEBUG_FILE)
   #include "macros.h"
#endif

#include "dsi_ant_device.hpp"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

//////////////////////////////////////////////////////////////////////////////////
// Public Functions
//////////////////////////////////////////////////////////////////////////////////

DSIANTDevice::DSIANTDevice()
{
   #if defined(DEBUG_FILE)
      DSIDebug::ThreadInit("Application");
      DSIDebug::ThreadWrite(SW_VER);
   #endif

   BOOL bInitFailed = FALSE;
   bOpened = FALSE;

   hReceiveThread = (DSI_THREAD_ID)NULL;
   bKillThread = FALSE;
   bReceiveThreadRunning = FALSE;
   bCancel = FALSE;

   ulUSBSerialNumber = 0;

   memset(apclChannelList, 0, sizeof(apclChannelList));

   #if defined(DEBUG_FILE)
      DSIDebug::Init();
   #endif

   pclSerialObject = new DSISerialGeneric();
   pclANT = new DSIFramerANT();
   if (pclANT->Init(pclSerialObject) == FALSE)
   {
      bInitFailed = TRUE;
   }

   pclANT->SetCancelParameter(&bCancel);

   pclSerialObject->SetCallback(pclANT);


   if (DSIThread_MutexInit(&stMutexCriticalSection) != DSI_THREAD_ENONE)
   {
      bInitFailed = TRUE;
   }

   if (DSIThread_MutexInit(&stMutexChannelListAccess) != DSI_THREAD_ENONE)
   {
      bInitFailed = TRUE;
   }

   if (DSIThread_CondInit(&stCondReceiveThreadExit) != DSI_THREAD_ENONE)
   {
      bInitFailed = TRUE;
   }

   //If the init fails, something is broken and nothing is going to work anyway
   //additionally the logging isn't even setup at this point
   //so we throw an exception so we know something is broken right away
   if(bInitFailed == TRUE)
      throw "AntDevice constructor: init failed";
}

///////////////////////////////////////////////////////////////////////
DSIANTDevice::~DSIANTDevice()
{
   DisconnectFromDevice();

   ClearManagedChannelList();

   delete pclSerialObject;
   delete pclANT;

   DSIThread_MutexDestroy(&stMutexCriticalSection);
   DSIThread_MutexDestroy(&stMutexChannelListAccess);
   DSIThread_CondDestroy(&stCondReceiveThreadExit);

   #if defined(DEBUG_FILE)
      DSIDebug::Close();
   #endif
}

///////////////////////////////////////////////////////////////////////
void DSIANTDevice::Close()
{
   DisconnectFromDevice();
}

///////////////////////////////////////////////////////////////////////
void DSIANTDevice::DisconnectFromDevice()
{
   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("DSIANTDevice::DisconnectFromDevice():  Cleaning up...");
   #endif

   DSIThread_MutexLock(&stMutexCriticalSection);

   // Stop the threads.
   bKillThread = TRUE;
   bCancel = TRUE;

   if (hReceiveThread)
   {
      if (bReceiveThreadRunning == TRUE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("DSIANTDevice::DisconnectFromDevice():  Killing receive thread.");
         #endif


         if (DSIThread_CondTimedWait(&stCondReceiveThreadExit, &stMutexCriticalSection, 2000) != DSI_THREAD_ENONE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("DSIANTDevice::DisconnectFromDevice():  Receive thread not dead. Forcing thread termination...");
            #endif
            //Note: Destroying this thread can possibly trigger a deadlock against LibUsbLibrary::FreeLibrary() in one of the destructor paths for some reason.
            //Possible that it is leaking a system lock used by freelibrary when it is forcefully closed.
            DSIThread_DestroyThread(hReceiveThread);
         }
         else
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("DSIANTDevice::DisconnectFromDevice():  Thread terminated successfully.");
            #endif
         }
      }

      DSIThread_ReleaseThreadID(hReceiveThread);
      hReceiveThread = (DSI_THREAD_ID)NULL;
   }

   if (bOpened == TRUE)
   {
      // Send reset command to ANT.
      pclANT->ResetSystem(200);

      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("DSIANTDevice::DisconnectFromDevice():  ANT_ResetSystem() complete.");
      #endif

      pclSerialObject->Close(TRUE);
      bOpened = FALSE;
   }

   bCancel = FALSE; //Now that we have done a reset, it is safe to restore the cancel parameter

   DSIThread_MutexUnlock(&stMutexCriticalSection);


   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("DSIANTDevice::DisconnectFromDevice():  Closed.");
   #endif
}

///////////////////////////////////////////////////////////////////////
BOOL DSIANTDevice::Open()
{
   //Always make sure we are not currently open
   DisconnectFromDevice();

   if (pclSerialObject->AutoInit() == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("DSIANTDevice::Open():  AutoInit Failed.");
      #endif
      return FALSE;
   }
   else
   {
      return ConnectToDevice();
   }
}

///////////////////////////////////////////////////////////////////////
BOOL DSIANTDevice::Open(UCHAR ucUSBDeviceNum_, USHORT usBaudRate_)
{
   //Always make sure we are not currently open
   DisconnectFromDevice();

   if(pclSerialObject->Init(usBaudRate_, ucUSBDeviceNum_) == FALSE)
      return FALSE;
   else
      return ConnectToDevice();
}


///////////////////////////////////////////////////////////////////////
BOOL DSIANTDevice::ConnectToDevice(void)
{
   //NOTE: Serial object must already be set up

   BOOL failedConnect = FALSE;

   bCancel = FALSE;
   bKillThread = FALSE;

   #if defined(DEBUG_FILE)
      DSIDebug::ResetTime();
   #endif

   if (pclANT->Init(pclSerialObject) == FALSE)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("DSIANTDevice::ConnectToDevice():  Failed to re-init pclANT.");
      #endif
      failedConnect = TRUE;
   }

   if(!failedConnect)
   {
      bOpened = pclSerialObject->Open();
      if (bOpened == FALSE)
         failedConnect = TRUE;
   }

   if(!failedConnect)
   {
      ulUSBSerialNumber = pclSerialObject->GetDeviceSerialNumber();

      //Reset so we know we have a clean state
      pclANT->ResetSystem(200);
      //If this fails it is probably connected at the wrong baud rate

      if(pclANT->GetCapabilities(NULL, 200) == FALSE)
         failedConnect = TRUE;
      //TODO could get number of channels and use dynamic sized channel array
   }

   if(!failedConnect)
   {
      //Flush all messages before we start the message receiver
      ANT_MESSAGE stMessage;
      while(pclANT->GetMessage(&stMessage, MESG_MAX_SIZE_VALUE) != DSI_FRAMER_TIMEDOUT)
      {
         //Continue until all messages are cleared from the buffer
      }

      if (hReceiveThread == NULL)
      {
         hReceiveThread = DSIThread_CreateThread(&DSIANTDevice::ReceiveThreadStart, this);
         if (hReceiveThread == NULL)
            failedConnect = TRUE;
      }
   }

   if(failedConnect)
      this->DisconnectFromDevice();

   return !(failedConnect);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIANTDevice::AddMessageProcessor(UCHAR ucChannelNumber_, DSIANTMessageProcessor* pclChannel_)
{
   DSIThread_MutexLock(&stMutexChannelListAccess);

   if(ucChannelNumber_ >= MAX_ANT_CHANNELS)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("DSIANTDevice::AddManagedChannel():  Invalid channel number");
      #endif
      DSIThread_MutexUnlock(&stMutexChannelListAccess);
      return FALSE;
   }

   if(apclChannelList[ucChannelNumber_] != (DSIANTMessageProcessor*) NULL)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("DSIANTDevice::AddManagedChannel(): Managed channel already registered for this channel number. Unregister first.");
      #endif
      DSIThread_MutexUnlock(&stMutexChannelListAccess);
      return FALSE;
   }

   apclChannelList[ucChannelNumber_] = pclChannel_;
   pclChannel_->Init(pclANT, ucChannelNumber_);

   DSIThread_MutexUnlock(&stMutexChannelListAccess);

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL DSIANTDevice::RemoveMessageProcessor(DSIANTMessageProcessor* pclChannel_)
{
   UCHAR ucChannel;

   DSIThread_MutexLock(&stMutexChannelListAccess);

   if(pclChannel_ == (DSIANTMessageProcessor*) NULL)
   {
      DSIThread_MutexUnlock(&stMutexChannelListAccess);
      return FALSE;
   }

   // Find the handle in the list
   ucChannel = pclChannel_->GetChannelNumber();

   if(apclChannelList[ucChannel] == (DSIANTMessageProcessor*) NULL)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("DSIANTDevice::RemoveMessageProcessor(): No message processor registered.");
      #endif
      DSIThread_MutexUnlock(&stMutexChannelListAccess);
      return FALSE;
   }

   if(apclChannelList[ucChannel] != pclChannel_)
   {
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("DSIANTDevice::RemoveMessageProcessor(): Message processor mismatch during removal.");
      #endif
      DSIThread_MutexUnlock(&stMutexChannelListAccess);
      return FALSE;
   }

   pclChannel_->Close();
   apclChannelList[ucChannel] = (DSIANTMessageProcessor*) NULL;

   DSIThread_MutexUnlock(&stMutexChannelListAccess);

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("DSIANTDevice::RemoveMessageProcessor(): Message processor removed.");
   #endif

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
void DSIANTDevice::ClearManagedChannelList(void)
{
   DSIThread_MutexLock(&stMutexChannelListAccess);
      memset(apclChannelList, 0, sizeof(apclChannelList));
   DSIThread_MutexUnlock(&stMutexChannelListAccess);
}

#if defined(DEBUG_FILE)
///////////////////////////////////////////////////////////////////////
void DSIANTDevice::SetDebug(BOOL bDebugOn_, const char *pcDirectory_)
{
   if (pcDirectory_)
      DSIDebug::SetDirectory(pcDirectory_);

   DSIDebug::SetDebug(bDebugOn_);
}
#endif

///////////////////////////////////////////////////////////////////////
ULONG DSIANTDevice::GetSerialNumber(void)
{
   return ulUSBSerialNumber;
}


///////////////////////////////////////////////////////////////////////
DSI_THREAD_RETURN DSIANTDevice::ReceiveThreadStart(void *pvParameter_)
{
   #if defined(DEBUG_FILE)
   DSIDebug::ThreadInit("ANTReceive");
   #endif

   ((DSIANTDevice*)pvParameter_)->ReceiveThread();

   return 0;
}

///////////////////////////////////////////////////////////////////////
// ANT Receive Thread
///////////////////////////////////////////////////////////////////////
#define MESG_CHANNEL_OFFSET                  0
#define MESG_EVENT_ID_OFFSET                 1
#define MESG_EVENT_CODE_OFFSET               2

void DSIANTDevice::ReceiveThread(void)
{
   ANT_MESSAGE stMessage;
   USHORT usMesgSize = 0;

   bReceiveThreadRunning = TRUE;

   while (bKillThread == FALSE)
   {
      usMesgSize = pclANT->WaitForMessage(1000);

      if (bKillThread)
         break;

      switch(usMesgSize)
      {
         case DSI_FRAMER_TIMEDOUT:
            break;

         case DSI_FRAMER_ERROR:
            {
               usMesgSize = pclANT->GetMessage(&stMessage, MESG_MAX_SIZE_VALUE);  //get the message to clear the error
               #if defined(DEBUG_FILE)
               {
                  UCHAR aucString[256];

                  SNPRINTF((char *)aucString,256, "DSIANTDevice::RecvThread():  Framer Error %u .", stMessage.ucMessageID);
                  DSIDebug::ThreadWrite((char *)aucString);

                  if(stMessage.ucMessageID == DSI_FRAMER_ANT_ESERIAL)
                  {
                     SNPRINTF((char *)aucString,256, "DSIANTDevice::RecvThread():  Serial Error %u .", stMessage.aucData[0]);
                     DSIDebug::ThreadWrite((char *)aucString);
                  }
               }
               #endif

               //bCancel = TRUE;

               HandleSerialError();

               continue;
            }
            break;

         default:
            //if (usMesgSize < DSI_FRAMER_TIMEDOUT)  //if the return isn't DSI_FRAMER_TIMEDOUT or DSI_FRAMER_ERROR
            {
               UCHAR ucANTChannel;
               usMesgSize = pclANT->GetMessage(&stMessage, MESG_MAX_SIZE_VALUE);

               #if defined(DEBUG_FILE)
               if (usMesgSize == 0)
               {
                  UCHAR aucString2[256];

                  SNPRINTF((char *)aucString2,256, "Rx msg reported size 0, dump:%u[0x%02X]...[0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X]", usMesgSize , stMessage.ucMessageID, stMessage.aucData[0], stMessage.aucData[1], stMessage.aucData[2], stMessage.aucData[3], stMessage.aucData[4], stMessage.aucData[5], stMessage.aucData[6], stMessage.aucData[7]);
                  DSIDebug::ThreadWrite((char *)aucString2);
               }
               #endif

               if(stMessage.ucMessageID == MESG_SERIAL_ERROR_ID)
               {
                  #if defined(DEBUG_FILE)
                  {
                     UCHAR aucString[256];
                     SNPRINTF((char *) aucString, 256, "DSIANTDevice::RecvThread():  Serial Error.");
                     DSIDebug::ThreadWrite((char *) aucString);
                  }
                  #endif

                  HandleSerialError();

                  continue;
               }

               // Figure out channel
               //ucANTChannel = stMessage.aucData[MESG_CHANNEL_OFFSET] & CHANNEL_NUMBER_MASK;
               ucANTChannel = pclANT->GetChannelNumber(&stMessage);

               // Send messages to appropriate handler
               if(ucANTChannel != 0xFF)
               {
                  DSIThread_MutexLock(&stMutexChannelListAccess);

                  // TODO: Add general channel and protocol event callbacks?
                  if(apclChannelList[ucANTChannel] != (DSIANTMessageProcessor*) NULL)
                  {
                     if(apclChannelList[ucANTChannel]->GetEnabled())
                        apclChannelList[ucANTChannel]->ProcessMessage(&stMessage, usMesgSize);
                  }

                  DSIThread_MutexUnlock(&stMutexChannelListAccess);
               }

             }
      }

   } // while()

   DSIThread_MutexLock(&stMutexCriticalSection);
   bReceiveThreadRunning = FALSE;
   DSIThread_CondSignal(&stCondReceiveThreadExit);
   DSIThread_MutexUnlock(&stMutexCriticalSection);
}

///////////////////////////////////////////////////////////////////////
//TODO //TTA Should change this to be like the managed side
void DSIANTDevice::HandleSerialError(void)
{
   UCHAR i;

   DSIThread_MutexLock(&stMutexChannelListAccess);

   // Notify callbacks
   for(i = 0; i< MAX_ANT_CHANNELS; i++)
   {
      if(apclChannelList[i] != (DSIANTMessageProcessor*) NULL)
         apclChannelList[i]->ProcessDeviceNotification(ANT_DEVICE_NOTIFICATION_SHUTDOWN, (void*) NULL);
   }

   DSIThread_MutexUnlock(&stMutexChannelListAccess);

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("DSIANTDevice::HandleSerialError(): Closing USB Device");
   #endif
   DisconnectFromDevice();
}
