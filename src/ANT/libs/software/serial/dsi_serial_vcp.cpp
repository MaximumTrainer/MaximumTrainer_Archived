/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#if defined(DSI_TYPES_WINDOWS)
#include "defines.h"
#include "dsi_serial_vcp.hpp"
#include "macros.h"

//#include "usb_device_handle_si.hpp"

#include <stdio.h>
#include <string.h>
#include <windows.h>

#if defined(_MSC_VER)
   #include "WinDevice.h"
#endif


#include <vector>

using namespace std;

//#define NUMBER_OF_DEVICES_CHECK_THREAD_KILL     //Only include this if we are sure we want to terminate the Rx thread if the number of devices are ever reduced.

//////////////////////////////////////////////////////////////////////////////////
// Private Definitions
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
// Public Methods
//////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Constructor
///////////////////////////////////////////////////////////////////////
DSISerialVCP::DSISerialVCP()
{
   hReceiveThread = NULL;
   hComm = INVALID_HANDLE_VALUE;
}

///////////////////////////////////////////////////////////////////////
// Destructor
///////////////////////////////////////////////////////////////////////
DSISerialVCP::~DSISerialVCP()
{
   Close();
}

///////////////////////////////////////////////////////////////////////
// Initializes and opens the object.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialVCP::AutoInit()
{
   return FALSE; // unsupported
}

///////////////////////////////////////////////////////////////////////
// Initializes the object.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialVCP::Init(ULONG ulBaud_, UCHAR ucDeviceNumber_)
{
   ulBaud = ulBaud_;

   ucDeviceNumber = ucDeviceNumber_; // make sure device number is in acceptable range for COM number translation

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Opens port, starts receive thread.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialVCP::Open(void)
{
   // Make sure all handles are reset before opening again.
   Close();

   if (pclCallback == NULL)
      return FALSE;

   bStopReceiveThread = FALSE;

   if(DSIThread_MutexInit(&stMutexCriticalSection) != DSI_THREAD_ENONE)
   {
      Close();
      return FALSE;
   }

   if(DSIThread_CondInit(&stEventReceiveThreadExit) != DSI_THREAD_ENONE)
   {
      DSIThread_MutexDestroy(&stMutexCriticalSection);
      Close();
      return FALSE;
   }

   //long enough to hold '\\.\COM255' and a null
   LPSTR lpstrComPort = new CHAR[11];
   SNPRINTF(lpstrComPort, 11, "\\\\.\\COM%u", ucDeviceNumber);

   hComm = CreateFile( lpstrComPort,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       0,
                       OPEN_EXISTING,
                       FILE_FLAG_OVERLAPPED,
                       0);
   //clean up after ourselves
   delete lpstrComPort;

   if (hComm == INVALID_HANDLE_VALUE)
   {
      DSIThread_CondDestroy(&stEventReceiveThreadExit);
     DSIThread_MutexDestroy(&stMutexCriticalSection);
      Close();
      return FALSE;
   }

   FillMemory(&dcb, sizeof(dcb), 0);
   dcb.DCBlength = sizeof(dcb);

   if (!GetCommState(hComm, &dcb))
   {
      DSIThread_CondDestroy(&stEventReceiveThreadExit);
     DSIThread_MutexDestroy(&stMutexCriticalSection);
      Close();
      return FALSE;
   }

   //if (!BuildCommDCB("57600,n,8,1", &dcb)) {
      // Couldn't build the DCB. Usually a problem
      // with the communications specification string.
   //   return FALSE;
   //}

   dcb.BaudRate = ulBaud;
   dcb.ByteSize = 8;
   dcb.fParity = 0;
   dcb.Parity = 0;//NOPARITY; // no parity
   dcb.StopBits = 0;//ONESTOPBIT; // one stop bit

   dcb.fOutxCtsFlow = FALSE;
   dcb.fOutxDsrFlow = FALSE;
   dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;

   if (!SetCommState(hComm, &dcb))
   {
      DSIThread_CondDestroy(&stEventReceiveThreadExit);
     DSIThread_MutexDestroy(&stMutexCriticalSection);
      Close();
      return FALSE;
   }

   COMMTIMEOUTS commTimeout;

   if (!GetCommTimeouts(hComm, &commTimeout))
   {
      DSIThread_CondDestroy(&stEventReceiveThreadExit);
     DSIThread_MutexDestroy(&stMutexCriticalSection);
      Close();
      return FALSE;
   }

   commTimeout.ReadIntervalTimeout = 0;
   commTimeout.ReadTotalTimeoutMultiplier = 0;
   commTimeout.ReadTotalTimeoutConstant = 0;

   if (!SetCommTimeouts(hComm, &commTimeout))
   {
      DSIThread_CondDestroy(&stEventReceiveThreadExit);
     DSIThread_MutexDestroy(&stMutexCriticalSection);
      Close();
      return FALSE;
   }

   hReceiveThread = DSIThread_CreateThread(&DSISerialVCP::ProcessThread, this);
   if(hReceiveThread == NULL)
   {
      DSIThread_MutexDestroy(&stMutexCriticalSection);
      DSIThread_CondDestroy(&stEventReceiveThreadExit);
      Close();
      return FALSE;
   }

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Closes the USB connection, kills receive thread.
///////////////////////////////////////////////////////////////////////
void DSISerialVCP::Close(BOOL /*bReset*/) //Commented to avoid compiler warning about unreferenced formal parameter.
{

   if(hReceiveThread)
   {
      DSIThread_MutexLock(&stMutexCriticalSection);
      if(bStopReceiveThread == FALSE)
      {
         bStopReceiveThread = TRUE;

         if (DSIThread_CondTimedWait(&stEventReceiveThreadExit, &stMutexCriticalSection, 3000) != DSI_THREAD_ENONE)
         {
            // We were unable to stop the thread normally.
            DSIThread_DestroyThread(hReceiveThread);
         }
      }
      DSIThread_MutexUnlock(&stMutexCriticalSection);

      DSIThread_ReleaseThreadID(hReceiveThread);
      hReceiveThread = NULL;

      DSIThread_MutexDestroy(&stMutexCriticalSection);
      DSIThread_CondDestroy(&stEventReceiveThreadExit);
   }

   if (hComm != INVALID_HANDLE_VALUE)
   {
      // lower DTR
      //
      if (!EscapeCommFunction(hComm, CLRDTR))
         pclCallback->Error(255);

      //
      // restore original comm timeouts
      //
      //if (!SetCommTimeouts(COMDEV(TTYInfo),  &(TIMEOUTSORIG(TTYInfo))))
      //   pclCallback->Error(255);

      //
      // Purge reads/writes, input buffer and output buffer
      //

      if (!PurgeComm(hComm, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR))
         pclCallback->Error(255);

      CloseHandle(hComm);
      hComm = INVALID_HANDLE_VALUE;
   }
   return;
}

///////////////////////////////////////////////////////////////////////
// Writes ucSize_ bytes to USB, returns number of bytes not written.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialVCP::WriteBytes(void *pvData_, USHORT usSize_)
{
   DWORD dwWritten;
   BOOL fRes;
   OVERLAPPED osWrite = {0};

   if(hComm == INVALID_HANDLE_VALUE || pvData_ == NULL)
      return FALSE;

   // Create this writes OVERLAPPED structure hEvent.
   osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
   if (osWrite.hEvent == NULL)
      // Error creating overlapped event handle.
      return FALSE;

   // Issue write.
   if (!WriteFile(hComm, pvData_, (DWORD)usSize_, &dwWritten, &osWrite)) {
      if (GetLastError() != ERROR_IO_PENDING) {
         // WriteFile failed, but it isn't delayed. Report error and abort.
         fRes = FALSE;
      }
      else {
         // Write is pending.
         if (!GetOverlappedResult(hComm, &osWrite, &dwWritten, TRUE))
            fRes = FALSE;
         else
            // Write operation completed successfully.
            fRes = TRUE;
      }
   }
   else
      // WriteFile completed immediately.
      fRes = TRUE;

   CloseHandle(osWrite.hEvent);

   if (!fRes)
      pclCallback->Error(DSI_SERIAL_EWRITE);

   return fRes;
}

///////////////////////////////////////////////////////////////////////
// Returns the device serial number.
///////////////////////////////////////////////////////////////////////
ULONG DSISerialVCP::GetDeviceSerialNumber()
{
   return 0xFFFFFFFF; // unsupported
}

///////////////////////////////////////////////////////////////////////
// Returns the device port number.
///////////////////////////////////////////////////////////////////////
UCHAR DSISerialVCP::GetDeviceNumber()
{
   return ucDeviceNumber;
}

//////////////////////////////////////////////////////////////////////////////////
// Private Methods
//////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
void DSISerialVCP::ReceiveThread(void)
{
   UCHAR ucRxByte;
   DWORD ulRxBytesRead;
   DWORD dwCommEvent;
   OVERLAPPED osRead = {0};

//   while (hComm == INVALID_HANDLE_VALUE);

   if (!SetCommMask(hComm, EV_RXCHAR))
      pclCallback->Error(DSI_SERIAL_EREAD);

   while(!bStopReceiveThread)
   {
      if (WaitCommEvent(hComm, &dwCommEvent, NULL))
      {
         do
         {
            osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (osRead.hEvent == NULL)               // Error creating overlapped event handle.
               break;

            if (ReadFile(hComm, &ucRxByte, 1, &ulRxBytesRead, &osRead))
            {
               pclCallback->ProcessByte(ucRxByte);
            }
            else
            {
               if (GetLastError() != ERROR_IO_PENDING)
                  pclCallback->Error(DSI_SERIAL_EREAD);
               else
               {
                  DWORD hRes;
                  do
                  {
                     hRes = WaitForSingleObject(osRead.hEvent, 999);
                  } while (hRes == WAIT_TIMEOUT);

                  if (hRes == WAIT_OBJECT_0)
                  {
                     if (!GetOverlappedResult(hComm, &osRead, &ulRxBytesRead, TRUE))
                        pclCallback->Error(DSI_SERIAL_EREAD);
                     else
                        pclCallback->ProcessByte(ucRxByte);

                  }
               }
            }

            CloseHandle(osRead.hEvent);

         } while (ulRxBytesRead);
      }
      else
      {
         pclCallback->Error(DSI_SERIAL_EREAD);
      }
   }

   DSIThread_MutexLock(&stMutexCriticalSection);
      bStopReceiveThread = TRUE;
      DSIThread_CondSignal(&stEventReceiveThreadExit);                       // Set an event to alert the main process that Rx thread is finished and can be closed.
   DSIThread_MutexUnlock(&stMutexCriticalSection);
}

///////////////////////////////////////////////////////////////////////
DSI_THREAD_RETURN DSISerialVCP::ProcessThread(void *pvParameter_)
{
   DSISerialVCP *This = (DSISerialVCP *) pvParameter_;
   This->ReceiveThread();
   return 0;
}

#endif //defined(DSI_TYPES_WINDOWS)