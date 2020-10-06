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
#include "usb_device_handle_si.hpp"
#include "macros.h"
#include "dsi_debug.hpp"

#if defined(_MSC_VER)
   #include "WinDevice.h"
#endif

#include "usb_device_list.hpp"

#include <memory>

#include <stdio.h>
#include <string.h>
#include <windows.h>

using namespace std;

//#define NUMBER_OF_DEVICES_CHECK_THREAD_KILL     //Only include this if we are sure we want to terminate the Rx thread if the number of devices are ever reduced.

//////////////////////////////////////////////////////////////////////////////////
// Static declarations
//////////////////////////////////////////////////////////////////////////////////

USBDeviceList<const USBDeviceSI> USBDeviceHandleSI::clDeviceList;


//////////////////////////////////////////////////////////////////////////////////
// Private Definitions
//////////////////////////////////////////////////////////////////////////////////
#define INVALID_USB_DEVICE_NUMBER      MAX_UCHAR
#define SI_STOPBIT_1                   ((WORD) 0x00)
#define SI_STOPBIT_1_5                 ((WORD) 0x01)
#define SI_STOPBIT_2                   ((WORD) 0x02)

#define SI_PARITY_NONE                 ((WORD) 0x00)
#define SI_PARITY_ODD                  ((WORD) 0x10)
#define SI_PARITY_EVEN                 ((WORD) 0x20)
#define SI_PARITY_MARK                 ((WORD) 0x30)
#define SI_PARITY_SPACE                ((WORD) 0x40)

#define SI_BITS_5                      ((WORD) 0x500)
#define SI_BITS_6                      ((WORD) 0x600)
#define SI_BITS_7                      ((WORD) 0x700)
#define SI_BITS_8                      ((WORD) 0x800)

#define SI_WRITE_TIMEOUT               ((DWORD) 3000) //3s Write timeout



//////////////////////////////////////////////////////////////////////////////////
// Public Methods
//////////////////////////////////////////////////////////////////////////////////

const USBDeviceListSI USBDeviceHandleSI::GetAllDevices()
{
   USBDeviceListSI clList;

   clDeviceList = USBDeviceList<const USBDeviceSI>();  //clear device list

   //Get a reference to library
   auto_ptr<const SiLabsLibrary> pclAutoSiLibrary(NULL);
   if(SiLabsLibrary::Load(pclAutoSiLibrary) == FALSE)
      return clList;
   const SiLabsLibrary& clSiLibrary = *pclAutoSiLibrary;


   DWORD dwNumberOfDevices = 0;
   if(clSiLibrary.GetNumDevices(&dwNumberOfDevices) != SI_SUCCESS)  //!!cast!
      return clList;

   if (dwNumberOfDevices > MAX_UCHAR)
      dwNumberOfDevices = MAX_UCHAR;

   UCHAR ucNumberOfDevices = (UCHAR)dwNumberOfDevices;

   for(UCHAR i=0; i<ucNumberOfDevices; i++)
   {
      clDeviceList.Add(USBDeviceSI(i) );  //save the copies to the static private list
      clList.Add(clDeviceList.GetAddress(i) );  //save a pointer to the just added device
   }

   return clList;  //return the list of pointers
}


BOOL CanOpenDevice(const USBDeviceSI*const & pclDevice_)
{
   if(pclDevice_ == FALSE)
      return FALSE;

   return USBDeviceHandleSI::TryOpen(*pclDevice_);
}

const USBDeviceListSI USBDeviceHandleSI::GetAvailableDevices()
{
   /*
   class TryOpening : public USBDeviceListSI::CompareFunc
   {
      USBDeviceListSI::CompareFunc::result_type operator()(USBDeviceListSI::CompareFunc::argument_type pclDevice_)
      {
         if(pclDevice_ == FALSE)
            return FALSE;

         USBDeviceHandleSI* pclTempHandle;
         BOOL bOpened;

         bOpened = USBDeviceHandleSI::Open(*pclDevice_, pclTempHandle, 0);

         if(bOpened)
            USBDeviceHandleSI::Close(pclTempHandle);

         return bOpened;
      }
   };

   TryOpening blah;
   */
   return USBDeviceHandleSI::GetAllDevices().GetSubList(CanOpenDevice);
}



///////////////////////////////////////////////////////////////////////
// Constructor
///////////////////////////////////////////////////////////////////////
USBDeviceHandleSI::USBDeviceHandleSI(const USBDeviceSI& clDevice_, ULONG ulBaudRate_)
try
:
   USBDeviceHandle(),
   clDevice(clDevice_),
   ulBaudRate(ulBaudRate_),
   bDeviceGone(TRUE),

   ucCTSCode(SI_STATUS_INPUT),
   ucRTSCode(SI_HELD_INACTIVE),
   ucDTRCode(SI_HELD_INACTIVE),
   ucDSRCode(SI_STATUS_INPUT),
   ucDCDCode(SI_STATUS_INPUT)
{

   hReceiveThread = NULL;
   bStopReceiveThread = TRUE;
   hUSBDeviceHandle = NULL;
   hUSBEvent = NULL;

   if(POpen() == FALSE)
      throw 0; //!!We need something to throw

   return;
}
catch(...)
{
   throw;
}

///////////////////////////////////////////////////////////////////////
// Destructor
///////////////////////////////////////////////////////////////////////
USBDeviceHandleSI::~USBDeviceHandleSI()
{
   //!!Delete all the elements in the queue?
}


///////////////////////////////////////////////////////////////////////
// Returns the device serial number.
///////////////////////////////////////////////////////////////////////
/*
USBError::Enum USBDeviceHandleSI::GetDeviceSerialNumber(ULONG& ulSerialNumber_)
{
   if(bDeviceGone)
      return USBError::DEVICE_GONE;

   UCHAR aucSerialString[CP210x_MAX_SERIAL_STRLEN+1];
   UCHAR ucLength = CP210x_MAX_SERIAL_STRLEN;
   memset(aucSerialString, 0, sizeof(aucSerialString));

   if(clCmLibrary.GetDeviceSerialNumber(hUSBDeviceHandle, aucSerialString, &ucLength, TRUE) != CP210x_SUCCESS)
      return USBError::FAILED;

   ULONG ulSerial = ulSerial = atol((char*)aucSerialString);
   if(ulSerial == 0)
      return USBError::FAILED;

   if ((ulSerial > 10000) && (ulSerial < 10255))
      memcpy((UCHAR*)&ulSerial, &(aucSerialString[56]), 4);

   ulSerialNumber_ = ulSerial;
   return USBError::NONE;
}
*/

///////////////////////////////////////////////////////////////////////
// Returns the number of SILabs devices connected.
// Returns 0 on error.
///////////////////////////////////////////////////////////////////////
UCHAR USBDeviceHandleSI::GetNumberOfDevices()  //!!Should this be in USBDeviceSI instead?
{

   //Get a reference to library
   auto_ptr<const SiLabsLibrary> pclAutoSiLibrary(NULL);
   if(SiLabsLibrary::Load(pclAutoSiLibrary) == FALSE)
      return 0;

   ULONG ulUSBNumberOfDevices;
   if(pclAutoSiLibrary->GetNumDevices(&ulUSBNumberOfDevices) != SI_SUCCESS)
      return 0;

   return (UCHAR)ulUSBNumberOfDevices;
}


///////////////////////////////////////////////////////////////////////
// Get USB enumeration info. Not necessarily connected to USB.
///////////////////////////////////////////////////////////////////////
/*
USBError::Enum USBDeviceHandleSI::GetDeviceUSBInfo(UCHAR ucDevice, UCHAR* pucProductString, UCHAR* pucSerialString)
{
   if(bDeviceGone)
      return USBError::DEVICE_GONE;

   if(clSiLibrary.GetProductString(ucDevice , pucSerialString, SI_RETURN_SERIAL_NUMBER) != SI_SUCCESS)
      return USBError::FAILED;

   if(clSiLibrary.GetProductString(ucDevice , pucProductString, SI_RETURN_DESCRIPTION) != SI_SUCCESS)
      return USBError::FAILED;

   return USBError::NONE;
}
*/

///////////////////////////////////////////////////////////////////////
// Get USB PID. Need to be connected to USB device.
///////////////////////////////////////////////////////////////////////
/*
USBError::Enum USBDeviceHandleSI::GetDevicePID(USHORT& usPID_)  //!!Don't need this!
{
   if(bDeviceGone)
      return USBError::DEVICE_GONE;

   if(clCmLibrary.GetDevicePid(hUSBDeviceHandle, &usPID_) != SI_SUCCESS)
      return USBError::FAILED;

   return USBError::NONE;
}
*/

//A more efficient way to test if you can open a device.  For instance, this function won't create a receive loop, etc.)
//Doesn't test for correct baudrate.
BOOL USBDeviceHandleSI::TryOpen(const USBDeviceSI& clDevice_)
{
   //Get a reference to library
   auto_ptr<const SiLabsLibrary> pclAutoSiLibrary(NULL);
   if(SiLabsLibrary::Load(pclAutoSiLibrary) == FALSE)
      return 0;
   const SiLabsLibrary& clSiLibrary = *pclAutoSiLibrary;

   HANDLE hTempHandle;
   if(clSiLibrary.Open(clDevice_.GetDeviceNumber(), &hTempHandle) != SI_SUCCESS)
   {
      SI_STATUS eStatus = clSiLibrary.Close(hTempHandle);
      if(eStatus != SI_SUCCESS) {} //would be an error
      return FALSE;
   }

   SI_STATUS eStatus = clSiLibrary.Close(hTempHandle);
   if(eStatus != SI_SUCCESS) {} //would be an error

   return TRUE;
}


///////////////////////////////////////////////////////////////////////
// Opens port, starts receive thread.
///////////////////////////////////////////////////////////////////////
BOOL USBDeviceHandleSI::Open(const USBDeviceSI& clDevice_, USBDeviceHandleSI*& pclDeviceHandle_, ULONG ulBaudRate_)
{
   try
   {
      pclDeviceHandle_ = new USBDeviceHandleSI(clDevice_, ulBaudRate_);
   }
   catch(...)
   {
      pclDeviceHandle_ = NULL;
      return FALSE;
   }

   return TRUE;
}


BOOL USBDeviceHandleSI::POpen()
{
   DWORD ReadTimeout,WriteTimeout;

   // Make sure all handles are reset before opening again.
   PClose();

   if(clSiLibrary.Open(clDevice.GetDeviceNumber(), &hUSBDeviceHandle) != SI_SUCCESS)
   {
      PClose();
      return FALSE;
   }

   bDeviceGone = FALSE;

   if(clSiLibrary.GetTimeouts(&ReadTimeout, &WriteTimeout) != SI_SUCCESS)
   {
      PClose();
      return FALSE;
   }

   WriteTimeout = SI_WRITE_TIMEOUT;  //only overwrite the write timeout

   if(clSiLibrary.SetTimeouts(ReadTimeout, WriteTimeout) != SI_SUCCESS)
   {
      PClose();
      return FALSE;
   }

   if(clSiLibrary.SetBaudRate(hUSBDeviceHandle, ulBaudRate) != SI_SUCCESS)
   {
      PClose();
      return FALSE;
   }

   if(clSiLibrary.SetLineControl(hUSBDeviceHandle, SI_PARITY_NONE | SI_BITS_8 | SI_STOPBIT_1) != SI_SUCCESS)
   {
      PClose();
      return FALSE;
   }

   ucCTSCode = SI_HANDSHAKE_LINE;
   ucRTSCode = SI_HELD_ACTIVE;

   if(clSiLibrary.SetFlowControl(hUSBDeviceHandle, ucCTSCode, ucRTSCode, ucDTRCode, ucDSRCode, ucDCDCode, FALSE) != SI_SUCCESS)
   {
      PClose();
      return FALSE;
   }

   // Clear the RX and TX buffers
   if(clSiLibrary.FlushBuffers(hUSBDeviceHandle, TRUE, TRUE) != SI_SUCCESS)
   {
      PClose();
      return FALSE;
   }

   hUSBEvent = CreateEvent((LPSECURITY_ATTRIBUTES) NULL, TRUE, FALSE, (LPCTSTR) NULL); // Create a manual-reset event.
   if(hUSBEvent == NULL)
   {
      PClose();
      return FALSE;
   }


   if(DSIThread_MutexInit(&stMutexCriticalSection) != DSI_THREAD_ENONE)
   {
      PClose();
      return FALSE;
   }

   if(DSIThread_CondInit(&stEventReceiveThreadExit) != DSI_THREAD_ENONE)
   {
      DSIThread_MutexDestroy(&stMutexCriticalSection);
      PClose();
      return FALSE;
   }

   bStopReceiveThread = FALSE;
   hReceiveThread = DSIThread_CreateThread(&USBDeviceHandleSI::ProcessThread, this);
   if(hReceiveThread == NULL)
   {
      DSIThread_CondDestroy(&stEventReceiveThreadExit);
      DSIThread_MutexDestroy(&stMutexCriticalSection);
      PClose();
      return FALSE;
   }

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Closes the USB connection, kills receive thread.
///////////////////////////////////////////////////////////////////////

//!!User is still expected to close the handle if the device is gone
BOOL USBDeviceHandleSI::Close(USBDeviceHandleSI*& pclDeviceHandle_, BOOL bReset_)
{
   if(pclDeviceHandle_ == NULL)
      return FALSE;

   pclDeviceHandle_->PClose(bReset_);
   delete pclDeviceHandle_;
   pclDeviceHandle_ = NULL;

   return TRUE;
}

void USBDeviceHandleSI::PClose(BOOL bReset_)
{

   bDeviceGone = TRUE;

   if (hReceiveThread)
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

   if(hUSBEvent)
   {
      CloseHandle(hUSBEvent);
      hUSBEvent = NULL;
   }


   if (hUSBDeviceHandle)
   {
      SI_STATUS eStatus;

      ucCTSCode = SI_STATUS_INPUT;
      ucRTSCode = SI_HELD_INACTIVE;
      ucDTRCode = SI_HELD_INACTIVE;
      ucDSRCode = SI_STATUS_INPUT;
      ucDCDCode = SI_STATUS_INPUT;

      eStatus = clSiLibrary.SetFlowControl(hUSBDeviceHandle, ucCTSCode, ucRTSCode, ucDTRCode, ucDSRCode, ucDCDCode, FALSE);

      if (eStatus != SI_INVALID_HANDLE)
         eStatus = clSiLibrary.FlushBuffers(hUSBDeviceHandle, TRUE, TRUE);

      if (eStatus != SI_INVALID_HANDLE && bReset_ == TRUE)
      {
         clCmLibrary.ResetDevice(hUSBDeviceHandle);
      }

      if (eStatus != SI_INVALID_HANDLE)
         eStatus = clSiLibrary.Close(hUSBDeviceHandle);

      hUSBDeviceHandle = NULL;

      if(bReset_ == TRUE)
         Sleep(1750);                //Stall to allow the device to reset, trying to reopen the driver too soon can cause bad things to happen.
   }
}

///////////////////////////////////////////////////////////////////////
// Writes ucSize_ bytes to USB, returns number of bytes not written.
///////////////////////////////////////////////////////////////////////
USBError::Enum USBDeviceHandleSI::Write(void* pvData_, ULONG ulSize_, ULONG& ulBytesWritten_)  //!!Should these be ULONG?
{
   if(bDeviceGone)
      return USBError::DEVICE_GONE;

   if(pvData_ == NULL)
      return USBError::INVALID_PARAM;

   //!!is there a max message size that we should test for?

   if(clSiLibrary.Write(hUSBDeviceHandle, pvData_, ulSize_, (LPDWORD)&ulBytesWritten_, (OVERLAPPED*)NULL) != SI_SUCCESS)
      return USBError::FAILED;

   if(ulBytesWritten_ != ulSize_)
      return USBError::FAILED;

   return USBError::NONE;
}



//////////////////////////////////////////////////////////////////////////////////
// Private Methods
//////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
USBError::Enum USBDeviceHandleSI::Read(void* pvData_, ULONG ulSize_, ULONG& ulBytesRead_, ULONG ulWaitTime_)
{
   if(bDeviceGone)
      return USBError::DEVICE_GONE;

   ulBytesRead_ = clRxQueue.PopArray(reinterpret_cast<char* const>(pvData_), ulSize_, ulWaitTime_);
   if(ulBytesRead_ == 0)
      return USBError::TIMED_OUT;

   return USBError::NONE;
}

/*
void USBDeviceHandleSI::ReceiveThread()
{
   BOOL bRxDebug = DSIDebug::ThreadInit("ao_si_receive.txt");
   DSIDebug::ThreadEnable(TRUE);

   SI_STATUS eStatus = SI_SUCCESS;
   while (!bStopReceiveThread)
   {
      // Initialize the OVERLAPPED structure.
      OVERLAPPED stOverlapped;
      stOverlapped.Offset = 0;
      stOverlapped.OffsetHigh = 0;
      stOverlapped.hEvent = hUSBEvent;

      ResetEvent(hUSBEvent);


      UCHAR aucRxData[255];
      ULONG ulRxBytesRead = 0;

      switch (eStatus)
      {
         case SI_SUCCESS:
            break;

         case SI_IO_PENDING:
         {
            DWORD dwWaitObject;
            do
            {
               dwWaitObject = WaitForSingleObject(hUSBEvent, 1000);

               if (dwWaitObject == WAIT_TIMEOUT)
               {
                  if(DeviceIsGone() == TRUE)
                  {
                     bDeviceGone = TRUE;
                     bStopReceiveThread = TRUE;
                  }

               }
               else if (GetOverlappedResult(NULL, &stOverlapped, &ulRxBytesRead, FALSE))
               {
                  if(ulRxBytesRead == 1)
                  {
                     //ulTotalBytesRead += ulRxBytesRead;
                     //Grab as many as you can!
                     //clRxQueue.Push(ucRxByte);
                  }
                  else
                  {
                     //!!Do we want to do this?
                     if(bRxDebug)
                        DSIDebug::ThreadWrite("Error: Overlapped result has no data.\n");
                     bDeviceGone = TRUE;
                     bStopReceiveThread = TRUE;
                  }
               }
            } while (!bStopReceiveThread && (dwWaitObject == WAIT_TIMEOUT));
            break;
         }

         case SI_RX_QUEUE_NOT_READY:
            if(bRxDebug)
               DSIDebug::ThreadWrite("Error: SI_RX_QUEUE_NOT_READY\n");
            //thru
         case SI_READ_TIMED_OUT:
            //minor errors, just try again
            break;

         case SI_INVALID_REQUEST_LENGTH:
            //Should never happen (assert?)
            if(bRxDebug)
               DSIDebug::ThreadWrite("Error: SI_INVALID_REQUEST_LENGTH\n");
            bDeviceGone = TRUE;
            bStopReceiveThread = TRUE;
            break;

         case SI_INVALID_PARAMETER:
            //Should never happen (assert?)
            if(bRxDebug)
               DSIDebug::ThreadWrite("Error: SI_INVALID_PARAMETER\n");
            bDeviceGone = TRUE;
            bStopReceiveThread = TRUE;
            break;

         case SI_INVALID_HANDLE:
            //fatal error
            if(bRxDebug)
               DSIDebug::ThreadWrite("Error: SI_INVALID_HANDLE\n");
            bDeviceGone = TRUE;
            bStopReceiveThread = TRUE;
            break;

         default:
            //fatal error
            if(bRxDebug)
               DSIDebug::ThreadWrite("Error: UNKNOWN\n");
            bDeviceGone = TRUE;
            bStopReceiveThread = TRUE;
            break;
      }

      ULONG ulTotalBytesRead = ulRxBytesRead;
      ULONG ulMaxBytes = sizeof(aucRxData);
      if(!bDeviceGone)
      {
         do
         {
            eStatus = clSiLibrary.Read(hUSBDeviceHandle, &(aucRxData[ulTotalBytesRead]), 1, &ulRxBytesRead, &stOverlapped);
            if(eStatus == SI_SUCCESS)
               ulTotalBytesRead += ulRxBytesRead;
         } while(eStatus == SI_SUCCESS && ulTotalBytesRead < ulMaxBytes);
      }

      if(ulTotalBytesRead != 0)
         clRxQueue.PushArray((char*)aucRxData, ulTotalBytesRead);

   }

   bDeviceGone = TRUE;

   DSIThread_MutexLock(&stMutexCriticalSection);
      bStopReceiveThread = TRUE;
      DSIThread_CondSignal(&stEventReceiveThreadExit);                       // Set an event to alert the main process that Rx thread is finished and can be closed.
   DSIThread_MutexUnlock(&stMutexCriticalSection);
}
*/

//!!Old implementation!
void USBDeviceHandleSI::ReceiveThread()
{
   while (!bStopReceiveThread)
   {
      // Initialize the OVERLAPPED structure.
      OVERLAPPED stOverlapped;
      stOverlapped.Offset = 0;
      stOverlapped.OffsetHigh = 0;
      stOverlapped.hEvent = hUSBEvent;

      ResetEvent(hUSBEvent);

      ULONG ulRxBytesRead;
      UCHAR ucRxByte;
      SI_STATUS eStatus = clSiLibrary.Read(hUSBDeviceHandle, &ucRxByte, 1, &ulRxBytesRead, &stOverlapped);

      switch (eStatus)
      {
         case SI_SUCCESS:
            //!!Grab as many as you can!
            clRxQueue.Push(ucRxByte);
            break;

         case SI_IO_PENDING:
         {
            DWORD dwWaitObject;
            do
            {
               dwWaitObject = WaitForSingleObject(hUSBEvent, 1000);

               if (dwWaitObject == WAIT_TIMEOUT)
               {
                  if(DeviceIsGone() == TRUE)
                  {
                     bDeviceGone = TRUE;
                     bStopReceiveThread = TRUE;
                  }

               }
               else if (GetOverlappedResult(NULL, &stOverlapped, &ulRxBytesRead, FALSE))
               {
                  if (ulRxBytesRead == 1)
                  {
                     //!!Grab as many as you can!
                     clRxQueue.Push(ucRxByte);
                  }
                  else
                  {
                     //!!Do we want to do this?
                     bDeviceGone = TRUE;
                     bStopReceiveThread = TRUE;
                  }
               }
            } while (!bStopReceiveThread && (dwWaitObject == WAIT_TIMEOUT));
            break;
         }

         case SI_RX_QUEUE_NOT_READY:
         case SI_READ_TIMED_OUT:
            //minor errors, just try again
            break;

         case SI_INVALID_REQUEST_LENGTH:
         case SI_INVALID_PARAMETER:
            //!!Should never happen (assert?)
            bDeviceGone = TRUE;
            bStopReceiveThread = TRUE;
            break;

         case SI_INVALID_HANDLE:
         default:
            //fatal errors
            bDeviceGone = TRUE;
            bStopReceiveThread = TRUE;
            break;
      }
   }

   bDeviceGone = TRUE;

   DSIThread_MutexLock(&stMutexCriticalSection);
      bStopReceiveThread = TRUE;
      DSIThread_CondSignal(&stEventReceiveThreadExit);                       // Set an event to alert the main process that Rx thread is finished and can be closed.
   DSIThread_MutexUnlock(&stMutexCriticalSection);
}

///////////////////////////////////////////////////////////////////////
DSI_THREAD_RETURN USBDeviceHandleSI::ProcessThread(void* pvParameter_)
{
   USBDeviceHandleSI* This = reinterpret_cast<USBDeviceHandleSI*>(pvParameter_);
   This->ReceiveThread();

   return 0;
}

//Tries to talk to the device.  If there is a failure, we assume the device is detached.
BOOL USBDeviceHandleSI::DeviceIsGone()
{
   if(bDeviceGone)
      return TRUE;

   UCHAR aucSerialString[255];
   UCHAR ucLength = 255;
   CP210x_STATUS eStatus = clCmLibrary.GetDeviceSerialNumber(hUSBDeviceHandle, aucSerialString, &ucLength, TRUE);  //!!Or we could check make a device list and check if this device is in it.
   if(eStatus != CP210x_SUCCESS)
      return TRUE;

   return FALSE;
}

#endif //defined(DSI_TYPES_WINDOWS)