/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "dsi_ant_device_polling.hpp"

//////////////////////////////////////////////////////////////////////////////////
// Public Functions
//////////////////////////////////////////////////////////////////////////////////

DSIANTDevicePolling::DSIANTDevicePolling()
{
   BOOL bInitFailed = FALSE;

   bKillReqThread = FALSE;

   hRequestThread = (DSI_THREAD_ID)NULL;                 // Handle for the Request thread

   bRequestThreadRunning = FALSE;

   ucUSBDeviceNum = 0;
   usBaudRate = 0;

   bAutoInit = FALSE;

   eState = ANT_USB_STATE_OFF;
   eRequest = REQUEST_NONE;


   if (DSIThread_MutexInit(&stMutexResponseQueue) != DSI_THREAD_ENONE)
   {
      bInitFailed = TRUE;
   }

   if (DSIThread_CondInit(&stCondRequestThreadExit) != DSI_THREAD_ENONE)
   {
      bInitFailed = TRUE;
   }

   if (DSIThread_CondInit(&stCondRequest) != DSI_THREAD_ENONE)
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
      throw "AntDevicePolling constructor: init failed";
}

///////////////////////////////////////////////////////////////////////
DSIANTDevicePolling::~DSIANTDevicePolling()
{
   closePolling();

   DSIThread_MutexDestroy(&stMutexResponseQueue);
   DSIThread_CondDestroy(&stCondRequestThreadExit);
   DSIThread_CondDestroy(&stCondRequest);
   DSIThread_CondDestroy(&stCondWaitForResponse);
}

///////////////////////////////////////////////////////////////////////
BOOL DSIANTDevicePolling::Open()
{
   bAutoInit = TRUE;

   return ReInitDevice();
}

///////////////////////////////////////////////////////////////////////
BOOL DSIANTDevicePolling::Open(UCHAR ucUSBDeviceNum_, USHORT usBaudRate_)
{
   bAutoInit = FALSE;
   ucUSBDeviceNum = ucUSBDeviceNum_;
   usBaudRate = usBaudRate_;

   return ReInitDevice();
}

///////////////////////////////////////////////////////////////////////
void DSIANTDevicePolling::Close()
{
   closePolling();

   //Now call base class to clean everything else up
   DSIANTDevice::Close();
}

////////////////////////////////////////////////////////////////////////
void DSIANTDevicePolling::closePolling()
{
   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("DSIANTDevicePolling::ClosePolling():  Closing...");
   #endif

   DSIThread_MutexLock(&stMutexCriticalSection);

   // Stop the thread
   bKillReqThread = TRUE;

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("DSIANTDevicePolling::Close():  SetEvent(stCondWaitForResponse).");
   #endif

   DSIThread_MutexLock(&stMutexResponseQueue);
   clResponseQueue.Clear();
   DSIThread_CondBroadcast(&stCondWaitForResponse);
   DSIThread_MutexUnlock(&stMutexResponseQueue);

   if (hRequestThread)
   {
      if (bRequestThreadRunning == TRUE)
     {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("DSIANTDevicePolling::Close():  SetEvent(stCondRequest).");
         #endif
         DSIThread_CondSignal(&stCondRequest);

         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("DSIANTDevicePolling::Close():  Killing thread.");
         #endif

         if (DSIThread_CondTimedWait(&stCondRequestThreadExit, &stMutexCriticalSection, 9000) != DSI_THREAD_ENONE)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("DSIANTDevicePolling::Close():  Thread not dead.");
               DSIDebug::ThreadWrite("DSIANTDevicePolling::Close():  Forcing thread termination...");
            #endif
            DSIThread_DestroyThread(hRequestThread);
         }
         else
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("DSIANTDevicePolling::Close():  Thread terminated successfully.");
            #endif
         }
      }

      DSIThread_ReleaseThreadID(hRequestThread);
      hRequestThread = (DSI_THREAD_ID)NULL;
   }

   eState = ANT_USB_STATE_OFF;

   DSIThread_MutexUnlock(&stMutexCriticalSection);

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("DSIANTDevicePolling::ClosePolling():  Closed.");
   #endif
}

///////////////////////////////////////////////////////////////////////
ANT_USB_STATE DSIANTDevicePolling::GetStatus(void)
{
   return eState;
}

///////////////////////////////////////////////////////////////////////
ANT_USB_RESPONSE DSIANTDevicePolling::WaitForResponse(ULONG ulMilliseconds_)
{
   ANT_USB_RESPONSE eResponse = ANT_USB_RESPONSE_NONE;

   if (bKillReqThread == TRUE)
      return ANT_USB_RESPONSE_NONE;

   //Wait for response
   DSIThread_MutexLock(&stMutexResponseQueue);
      if(clResponseQueue.isEmpty())
      {
         UCHAR ucResult = DSIThread_CondTimedWait(&stCondWaitForResponse, &stMutexResponseQueue, ulMilliseconds_);
         switch(ucResult)
         {
            case DSI_THREAD_ENONE:
               eResponse = clResponseQueue.GetResponse();
               break;

            case DSI_THREAD_ETIMEDOUT:
               eResponse = ANT_USB_RESPONSE_NONE;
               break;

            case DSI_THREAD_EOTHER:
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("DSIANTDevicePolling:WaitForResponse():  CondTimedWait() Failed!");
               #endif
               eResponse = ANT_USB_RESPONSE_NONE;
               break;

            default:
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("DSIANTDevicePolling:WaitForResponse():  Error  Unknown...");
               #endif
               eResponse = ANT_USB_RESPONSE_NONE;
               break;
         }
      }
      else
      {
         eResponse = clResponseQueue.GetResponse();
      }
   DSIThread_MutexUnlock(&stMutexResponseQueue);

   return eResponse;
}

//////////////////////////////////////////////////////////////////////////////////
// Private Functions
//////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
BOOL DSIANTDevicePolling::ReInitDevice(void)
{
   //Make sure we are always closed
   this->Close();

   bKillReqThread = FALSE;

   if (hRequestThread == NULL)
   {
      hRequestThread = DSIThread_CreateThread(&DSIANTDevicePolling::RequestThreadStart, this);
      if (hRequestThread == NULL)
         return FALSE;
   }

   DSIThread_MutexLock(&stMutexResponseQueue);
   clResponseQueue.Clear();
   DSIThread_MutexUnlock(&stMutexResponseQueue);

   DSIThread_MutexLock(&stMutexCriticalSection);
   eRequest = REQUEST_OPEN;
   DSIThread_CondSignal(&stCondRequest);
   DSIThread_MutexUnlock(&stMutexCriticalSection);

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("DSIANTDevicePolling::ReInitDevice():  Initializing.");
   #endif

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL DSIANTDevicePolling::AttemptOpen(void)
{
   if (bAutoInit)
   {
      return DSIANTDevice::Open();
   }
   else
   {
      return DSIANTDevice::Open(ucUSBDeviceNum, usBaudRate);
   }
}

///////////////////////////////////////////////////////////////////////
//This class overrides the handling of the serial error to defer processing to the request thread
void DSIANTDevicePolling::HandleSerialError(void)
{
   DSIThread_MutexLock(&stMutexCriticalSection);
   //bCancel = TRUE;

   if (eRequest < REQUEST_HANDLE_SERIAL_ERROR)
      eRequest = REQUEST_HANDLE_SERIAL_ERROR;
   DSIThread_CondSignal(&stCondRequest);
   DSIThread_MutexUnlock(&stMutexCriticalSection);
}

///////////////////////////////////////////////////////////////////////
DSI_THREAD_RETURN DSIANTDevicePolling::RequestThreadStart(void *pvParameter_)
{
   #if defined(DEBUG_FILE)
   DSIDebug::ThreadInit("DSIANTDevicePolling");
   #endif

   ((DSIANTDevicePolling*)pvParameter_)->RequestThread();

   return 0;
}

///////////////////////////////////////////////////////////////////////
//TODO //TTA Why use a polling state machine here? It just makes things complicated to trace.
void DSIANTDevicePolling::RequestThread(void)
{
   ANT_USB_RESPONSE eResponse;
   bRequestThreadRunning = TRUE;

   while (bKillReqThread == FALSE)
   {
      eResponse = ANT_USB_RESPONSE_NONE;

      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("DSIANTDevicePolling::RequestThread():  Awaiting Requests...");
      #endif

      DSIThread_MutexLock(&stMutexCriticalSection);

      if ((eRequest == REQUEST_NONE) && (bKillReqThread == FALSE))
      {
         UCHAR ucResult = DSIThread_CondTimedWait(&stCondRequest, &stMutexCriticalSection, 1000);
         if (ucResult != DSI_THREAD_ENONE)  // Polling interval 1Hz
         {
            #if defined(DEBUG_FILE)
               if(ucResult == DSI_THREAD_EOTHER)
                  DSIDebug::ThreadWrite("DSIANTDevicePolling::RequestThread(): CondTimedWait() Failed!");
            #endif
            if (eRequest == REQUEST_NONE && eState == ANT_USB_STATE_IDLE_POLLING_USB)
            {
               eRequest = REQUEST_OPEN;
            }
            DSIThread_MutexUnlock(&stMutexCriticalSection);
            continue;
         }
      }

      DSIThread_MutexUnlock(&stMutexCriticalSection);

      if (bKillReqThread)
         break;

      if (eRequest != REQUEST_NONE)
      {
         #if defined(DEBUG_FILE)
            DSIDebug::ThreadWrite("DSIANTDevicePolling::RequestThread():  Request received");
         #endif

         switch (eRequest)
         {
            case REQUEST_OPEN:
               #if defined(DEBUG_FILE)
                  DSIDebug::ThreadWrite("DSIANTDevicePolling::RequestThread():  Opening...");
               #endif

               if (AttemptOpen())
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("DSIANTDevicePolling::RequestThread():  Open passed.");
                  #endif

                  eState = ANT_USB_STATE_OPEN;
                  eResponse = ANT_USB_RESPONSE_OPEN_PASS;
               }
               else
               {
                  #if defined(DEBUG_FILE)
                     DSIDebug::ThreadWrite("DSIANTDevicePolling::RequestThread():  Open failed.");
                  #endif

                  eState = ANT_USB_STATE_IDLE_POLLING_USB;
               }
               break;

            default:
               break;
         }

         //This is where to handle the internal requests, because they can happen asynchronously.
         //We will also clear the request here.
         DSIThread_MutexLock(&stMutexCriticalSection);

         if (eResponse != ANT_USB_RESPONSE_NONE)
            AddResponse(eResponse);

         if (eRequest == REQUEST_HANDLE_SERIAL_ERROR)
         {
            #if defined(DEBUG_FILE)
               DSIDebug::ThreadWrite("DSIANTDevicePolling::RequestThread():  Serial error!");
            #endif
               DSIThread_MutexUnlock(&stMutexCriticalSection);
               DSIANTDevice::HandleSerialError();
               DSIThread_MutexLock(&stMutexCriticalSection);

            eState = ANT_USB_STATE_IDLE_POLLING_USB;
            eRequest = REQUEST_OPEN;
         }
         else
         {
            eRequest = REQUEST_NONE;  //Clear any other request
         }

         DSIThread_MutexUnlock(&stMutexCriticalSection);
      }

   }//while()

   #if defined(DEBUG_FILE)
      DSIDebug::ThreadWrite("DSIANTDevicePolling::RequestThread():  Exiting thread.");
   #endif

   eRequest = REQUEST_NONE;


   DSIANTDevice::Close(); //Try to close the device if it was open.  This is done because the device was opened in this thread, we should try to close the device before the thread terminates.

   DSIThread_MutexLock(&stMutexCriticalSection);
      bRequestThreadRunning = FALSE;
      DSIThread_CondSignal(&stCondRequestThreadExit);
   DSIThread_MutexUnlock(&stMutexCriticalSection);


   #if defined(__cplusplus)
      return;
   #else
      ExitThread(0);
      #if defined(DEBUG_FILE)
         DSIDebug::ThreadWrite("DSIANTDevicePolling::RequestThread():  C code reaching return statement unexpectedly.");
      #endif
      return;                                            // Code should not be reached.
   #endif
}

///////////////////////////////////////////////////////////////////////
void DSIANTDevicePolling::AddResponse(ANT_USB_RESPONSE eResponse_)
{
   DSIThread_MutexLock(&stMutexResponseQueue);
   clResponseQueue.AddResponse(eResponse_);
   DSIThread_CondSignal(&stCondWaitForResponse);
   DSIThread_MutexUnlock(&stMutexResponseQueue);
}
