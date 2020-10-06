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
#include "dsi_serial_si.hpp"
#include "macros.h"

#include "usb_device_handle_si.hpp"

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
DSISerialSI::DSISerialSI()
{
   hReceiveThread = NULL;
   pclDeviceHandle = NULL;
}

///////////////////////////////////////////////////////////////////////
// Destructor
///////////////////////////////////////////////////////////////////////
DSISerialSI::~DSISerialSI()
{
   Close();
}

///////////////////////////////////////////////////////////////////////
// Initializes and opens the object.
///////////////////////////////////////////////////////////////////////
//defines used by AutoInit
//#define USB_ANT_VID                    0x0FCF
//#define USB_ANT_PID                    0x1004
#define USB_ANT_PRODUCT_DESCRIPTION    "ANT USB Stick"
#define DEFAULT_BAUD_RATE              ((USHORT) 50000)
BOOL DSISerialSI::AutoInit()
{
   if(Init(DEFAULT_BAUD_RATE, (void*)USB_ANT_PRODUCT_DESCRIPTION,(USHORT) strlen(USB_ANT_PRODUCT_DESCRIPTION)) == FALSE)
   {
      this->USBReset();
      return FALSE;
   }

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Initializes the object.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialSI::Init(ULONG ulBaud_, UCHAR ucDeviceNumber_)
{
   Close();

   ulBaud = ulBaud_;
   ucDeviceNumber = ucDeviceNumber_;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Initializes the object.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialSI::Init(ULONG ulBaud_, void *pvProductDescription_, USHORT usSize_)
{
   UCHAR ucDeviceNumberTemp;

   Close();

   BOOL bSuccess = GetDeviceNumberByProductDescription(pvProductDescription_, usSize_, ucDeviceNumberTemp);
   if(!bSuccess)
   {
      Close();
      return FALSE;
   }

   ulBaud = ulBaud_;
   ucDeviceNumber = ucDeviceNumberTemp;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Initializes the object.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialSI::Init(ULONG ulBaud_, const USBDeviceSI& clDevice_)
{
   Close();

   ulBaud = ulBaud_;
   ucDeviceNumber = clDevice_.GetDeviceNumber();

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Returns the device serial number.
///////////////////////////////////////////////////////////////////////
ULONG DSISerialSI::GetDeviceSerialNumber()
{
   if(pclDeviceHandle == 0)
      return 0;

   return pclDeviceHandle->GetDevice().GetSerialNumber();
}

///////////////////////////////////////////////////////////////////////
// Returns the number of SILabs devices connected.
// Returns 0 on error.
///////////////////////////////////////////////////////////////////////
UCHAR DSISerialSI::GetNumberOfDevices()
{
   return USBDeviceHandleSI::GetNumberOfDevices();
}


///////////////////////////////////////////////////////////////////////
// Get USB enumeration info. Not necessarily connected to USB.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialSI::GetDeviceUSBInfo(UCHAR ucDevice_, UCHAR* pucProductString_, UCHAR* pucSerialString_, USHORT usBufferSize_)
{
   const USBDeviceListSI clDeviceList = USBDeviceHandleSI::GetAllDevices();
   if(clDeviceList.GetSize() <= ucDevice_)
      return FALSE;

   if(clDeviceList[ucDevice_]->GetProductDescription(pucProductString_, usBufferSize_) != TRUE)
      return FALSE;

   if(clDeviceList[ucDevice_]->GetSerialString(pucSerialString_, usBufferSize_) != TRUE)
      return FALSE;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Get USB PID. Need to be connected to USB device.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialSI::GetDevicePID(USHORT& usPid_)
{
   if(pclDeviceHandle == NULL)
      return FALSE;

   usPid_ = pclDeviceHandle->GetDevice().GetPid();
   return TRUE;
}


///////////////////////////////////////////////////////////////////////
// Get USB VID. Need to be connected to USB device.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialSI::GetDeviceVID(USHORT& usVid_)
{
   if(pclDeviceHandle == NULL)
      return FALSE;

   usVid_ = pclDeviceHandle->GetDevice().GetVid();
   return TRUE;
}



///////////////////////////////////////////////////////////////////////
// Opens port, starts receive thread.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialSI::Open(void)
{
   // Make sure all handles are reset before opening again.
   Close();

   if (pclCallback == NULL)
      return FALSE;

   bStopReceiveThread = FALSE;

   const USBDeviceListSI clDeviceList = USBDeviceHandleSI::GetAllDevices();
   if(clDeviceList.GetSize() <= ucDeviceNumber)
      return FALSE;

   if(USBDeviceHandleSI::Open(*(clDeviceList[ucDeviceNumber]), pclDeviceHandle, ulBaud) == FALSE)
   {
      pclDeviceHandle = NULL;
      Close();
      return FALSE;
   }


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

   hReceiveThread = DSIThread_CreateThread(&DSISerialSI::ProcessThread, this);
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
void DSISerialSI::Close(BOOL bReset_)
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

   if(pclDeviceHandle)
   {
      USBDeviceHandleSI::Close(pclDeviceHandle, bReset_);

      if(bReset_)
         DSIThread_Sleep(1750);           //Stall to allow the device to reset, trying to reopen the driver too soon can cause bad things to happen.

      pclDeviceHandle = NULL;
   }

   return;
}

///////////////////////////////////////////////////////////////////////
// Writes ucSize_ bytes to USB, returns number of bytes not written.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialSI::WriteBytes(void *pvData_, USHORT usSize_)
{
   ULONG ulActualWritten = 0;

   if(pclDeviceHandle == NULL || pvData_ == NULL)
      return FALSE;

   if(pclDeviceHandle->Write(pvData_, usSize_, ulActualWritten) != USBError::NONE)
   {
      pclCallback->Error(DSI_SERIAL_EWRITE);
      return FALSE;
   }

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Returns the device port number.
///////////////////////////////////////////////////////////////////////
UCHAR DSISerialSI::GetDeviceNumber()
{
   return ucDeviceNumber;
}

//////////////////////////////////////////////////////////////////////////////////
// Private Methods
//////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
void DSISerialSI::ReceiveThread(void)
{
   UCHAR ucRxByte;
   ULONG ulRxBytesRead;

   while(!bStopReceiveThread)
   {

      USBError::Enum eStatus = pclDeviceHandle->Read(&ucRxByte, 1, ulRxBytesRead, 1000);

      switch(eStatus)
      {
         case USBError::NONE:
            pclCallback->ProcessByte(ucRxByte);
            break;

         case USBError::DEVICE_GONE:
            pclCallback->Error(DSI_SERIAL_DEVICE_GONE);
            bStopReceiveThread = TRUE;
            break;

         case USBError::TIMED_OUT:
            break;

         default:
            pclCallback->Error(DSI_SERIAL_EREAD);
            bStopReceiveThread = TRUE;
            break;
      }
   }

   DSIThread_MutexLock(&stMutexCriticalSection);
      bStopReceiveThread = TRUE;
      DSIThread_CondSignal(&stEventReceiveThreadExit);                       // Set an event to alert the main process that Rx thread is finished and can be closed.
   DSIThread_MutexUnlock(&stMutexCriticalSection);
}

///////////////////////////////////////////////////////////////////////
DSI_THREAD_RETURN DSISerialSI::ProcessThread(void *pvParameter_)
{
   DSISerialSI *This = (DSISerialSI *) pvParameter_;
   This->ReceiveThread();
   return 0;
}

///////////////////////////////////////////////////////////////////////
// Finds the first availible device that matches the product
// description and returns the device number.
//
// Assumes SiDLL functions are loaded.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialSI::GetDeviceNumberByProductDescription(void* pvProductDescription_, USHORT usSize_, UCHAR& ucDeviceNumber_)
{
   const USBDeviceListSI clDeviceList = USBDeviceHandleSI::GetAvailableDevices();
   if(clDeviceList.GetSize() == 0)
   {
      Close();
      return FALSE;
   }

   ULONG ulNumOfDevices = clDeviceList.GetSize();
   BOOL bDeviceFound = FALSE;
   for(ULONG i=0; i<ulNumOfDevices && !bDeviceFound; i++)
   {
      const USBDeviceSI& clDevice = *(clDeviceList[i]);
      UCHAR aucDescription[SI_MAX_DEVICE_STRLEN];

      if(clDevice.GetProductDescription(aucDescription, sizeof(aucDescription)))
      {
         if(strncmp((const char*)aucDescription, (char*)pvProductDescription_, MIN(strlen((const char*)aucDescription), usSize_)) != 0)
            continue;
      }

      USBDeviceHandleSI* pclTempHandle;
      if(USBDeviceHandleSI::Open(clDevice, pclTempHandle, 0) == FALSE)
         continue;

      USBDeviceHandleSI::Close(pclTempHandle);
      ucDeviceNumber_ = (UCHAR)i;
      bDeviceFound = TRUE;
   }

   return bDeviceFound;
}

void DSISerialSI::USBReset(void)
{
#if defined(_MSC_VER)
   // !! Borland builder chokes on cfgmgr32
   TCHAR line1[64];
   TCHAR* argv_[2];

   //Only do the soft reset if we fail to open a device or it seems like theres no deivces to open (device in a "hosed" state)
   //The soft reset will have no effect on devices currently opened by other applications.
   argv_[0] = line1;
   SNPRINTF(&(line1[0]),sizeof(line1), "@USB\\VID_0FCF&PID_1004\\*");   //The string for all ANT USB Sticks
   WinDevice_Disable(1,argv_);
   WinDevice_Enable(1,argv_);

#endif
}
#endif //defined(DSI_TYPES_WINDOWS)