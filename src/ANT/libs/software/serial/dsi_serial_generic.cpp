/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/

#include "dsi_serial_generic.hpp"

#include "types.h"
#include "defines.h"
#include "macros.h"

#include "usb_device_handle.hpp"

#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(DSI_TYPES_WINDOWS)
   #include "WinDevice.h"
#endif


#include "usb_device_list.hpp"

//#define NUMBER_OF_DEVICES_CHECK_THREAD_KILL     //Only include this if we are sure we want to terminate the Rx thread if the number of devices are ever reduced.

//////////////////////////////////////////////////////////////////////////////////
// Private Definitions
//////////////////////////////////////////////////////////////////////////////////
time_t DSISerialGeneric::lastUsbResetTime = 0;


//////////////////////////////////////////////////////////////////////////////////
// Public Methods
//////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Constructor
///////////////////////////////////////////////////////////////////////
DSISerialGeneric::DSISerialGeneric()
{
   pclDeviceHandle = NULL;
   pclDevice = NULL;
   hReceiveThread = NULL;
   bStopReceiveThread = TRUE;
   ucDeviceNumber = 0xFF;
   ulBaud = 0;

   return;
}

///////////////////////////////////////////////////////////////////////
// Destructor
///////////////////////////////////////////////////////////////////////
DSISerialGeneric::~DSISerialGeneric()
{
   Close();

   if (pclDevice)
      delete pclDevice;
   pclDevice = NULL;
}

///////////////////////////////////////////////////////////////////////
// Initializes and opens the object.
///////////////////////////////////////////////////////////////////////
//defines used by AutoInit
#define ANT_USB_STICK_PID     0x1004
#define ANT_USB_DEV_BOARD_PID 0x1006
#define ANT_USB_STICK_BAUD    ((USHORT)50000)
#define ANT_DEFAULT_BAUD      ((USHORT)57600)

BOOL DSISerialGeneric::AutoInit()
{
   Close();
   if (pclDevice)
      delete pclDevice;
   pclDevice = NULL;
   ucDeviceNumber = 0xFF;

   const ANTDeviceList clDeviceList = USBDeviceHandle::GetAvailableDevices();  //saves a copy of the list and all of the elements

   if(clDeviceList.GetSize() == 0)
   {
      //When this is being polled repeatedly, as in the ANTFS host class, the USBReset calls to WinDevice Enable/Disable take a large amount if CPU, so we limit the
      //reset here to only be called a maximum of once every 30s
      time_t curTime = time(NULL);
      if(curTime - lastUsbResetTime > 30)
      {
         lastUsbResetTime = curTime;
         this->USBReset();
      }
      return FALSE;
   }

   USBDeviceHandle::CopyANTDevice(pclDevice, clDeviceList[0]);
   ucDeviceNumber = 0;            //initialize it to 0 for now (all devices that use autoinit will have a 0 device number) //TODO this devicenumber is useless because it can't be used to access the list again
   switch (pclDevice->GetPid())
   {
      case ANT_USB_STICK_PID:
         ulBaud = ANT_USB_STICK_BAUD;
         break;
      case ANT_USB_DEV_BOARD_PID:
      default:
         ulBaud = ANT_DEFAULT_BAUD;
         break;
   }

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Initializes the object.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialGeneric::Init(ULONG ulBaud_, UCHAR ucDeviceNumber_)
{
   Close();
   if (pclDevice)
      delete pclDevice;
   pclDevice = NULL;
   ucDeviceNumber = 0xFF;

   //Note: None of the other existing classes validate the devicenumber, so
   //    this was removed for consistency as a bonus we avoid some of the
   //    performance issues with GetAllDevices()
   ////const ANTDeviceList clDeviceList = USBDeviceHandle::GetAllDevices();
   ////if(clDeviceList.GetSize() <= ucDeviceNumber_)
   ////{
   ////   this->USBReset();
   ////   return FALSE;
   ////}

   ulBaud = ulBaud_;
   ucDeviceNumber = ucDeviceNumber_;

   return TRUE;
}


///////////////////////////////////////////////////////////////////////
// Initializes the object.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialGeneric::Init(ULONG ulBaud_, const USBDevice& clDevice_, UCHAR ucDeviceNumber_)
{
   Close();
   if (pclDevice)
      delete pclDevice;
   pclDevice = NULL;


   pclDevice = &clDevice_;  //!!Make copy?
   ulBaud = ulBaud_;
   ucDeviceNumber = ucDeviceNumber_;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Returns the device serial number.
///////////////////////////////////////////////////////////////////////
ULONG DSISerialGeneric::GetDeviceSerialNumber()
{
   if(pclDeviceHandle == 0)
      return 0;

   return pclDeviceHandle->GetDevice().GetSerialNumber();
}


///////////////////////////////////////////////////////////////////////
// Get USB enumeration info. Not necessarily connected to USB.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialGeneric::GetDeviceUSBInfo(UCHAR ucDevice_, UCHAR* pucProductString_, UCHAR* pucSerialString_, USHORT usBufferSize_)
{

   const ANTDeviceList clDeviceList = USBDeviceHandle::GetAllDevices();
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
BOOL DSISerialGeneric::GetDevicePID(USHORT& usPid_)
{
   if(pclDeviceHandle == NULL)
      return FALSE;

   usPid_ = pclDeviceHandle->GetDevice().GetPid();
   return TRUE;
}


///////////////////////////////////////////////////////////////////////
// Get USB VID. Need to be connected to USB device.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialGeneric::GetDeviceVID(USHORT& usVid_)
{
   if(pclDeviceHandle == NULL)
      return FALSE;

   usVid_ = pclDeviceHandle->GetDevice().GetVid();
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Get USB Serial String. Need to be connected to USB device.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialGeneric::GetDeviceSerialString(UCHAR* pucSerialString_, USHORT usBufferSize_)
{
   if(pclDeviceHandle == NULL)
      return FALSE;

   if(pclDeviceHandle->GetDevice().GetSerialString(pucSerialString_, usBufferSize_) != TRUE)
      return FALSE;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Opens port, starts receive thread.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialGeneric::Open(void)
{
   // Make sure all handles are reset before opening again.
   Close();

   if (pclCallback == NULL)
      return FALSE;


   //If the user specified a device number instead of a USBDevice instance, then grab it from the list
   const USBDevice* pclTempDevice = pclDevice;
   if(pclDevice == NULL)
   {
      const USBDeviceList<const USBDevice*> clDeviceList = USBDeviceHandle::GetAllDevices();
      if(clDeviceList.GetSize() <= ucDeviceNumber)
         return FALSE;

      pclTempDevice = clDeviceList[ucDeviceNumber];
   }

   if(USBDeviceHandle::Open(*pclTempDevice, pclDeviceHandle, ulBaud) == FALSE)
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

   bStopReceiveThread = FALSE;
   hReceiveThread = DSIThread_CreateThread(&DSISerialGeneric::ProcessThread, this);
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
void DSISerialGeneric::Close(BOOL bReset_)
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
      USBDeviceHandle::Close(pclDeviceHandle, bReset_);

      //if(bReset_)                         //Only done for specific serial implementations
      //   DSIThread_Sleep(1750);           //Stall to allow the device to reset, trying to reopen the driver too soon can cause bad things to happen.

      pclDeviceHandle = NULL;
   }

   return;
}

///////////////////////////////////////////////////////////////////////
// Writes ucSize_ bytes to USB, returns number of bytes not written.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialGeneric::WriteBytes(void *pvData_, USHORT usSize_)
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
UCHAR DSISerialGeneric::GetDeviceNumber()
{
   return ucDeviceNumber;
}

//////////////////////////////////////////////////////////////////////////////////
// Private Methods
//////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
void DSISerialGeneric::ReceiveThread(void)
{
   UCHAR aucData[255];
   while(!bStopReceiveThread)
   {
      ULONG ulRxBytesRead;
      USBError::Enum eStatus = pclDeviceHandle->Read(aucData, sizeof(aucData), ulRxBytesRead, 1000);

      switch(eStatus)
      {
         case USBError::NONE:
            for(ULONG i=0; i<ulRxBytesRead; i++)
               pclCallback->ProcessByte(aucData[i]);
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
DSI_THREAD_RETURN DSISerialGeneric::ProcessThread(void* pvParameter_)
{
   DSISerialGeneric* This = (DSISerialGeneric*)pvParameter_;
   This->ReceiveThread();
   return 0;
}


//NOTE: The USBReset was originally added due to issues in the USB1 (two processor) usb stick that would occasionally have synch issues requiring a reset
// to remedy. It appears this reset is of no use for USB2s, and in fact is documented in device_handle_libusb PClose() as potentially causing errors with usb2s.
// However, because the behavior is not fully understood and we aren't receiving complaints (beyond the cpu usage of this reset) we are leaving things 'as-is'
void DSISerialGeneric::USBReset(void)
{
   //If the user specified a device number instead of a USBDevice instance, then grab it from the list
   const USBDevice* pclTempDevice = pclDevice;
   if(pclDevice == NULL)
   {
      const ANTDeviceList clDeviceList = USBDeviceHandle::GetAllDevices();
      if((ucDeviceNumber == 0xFF) || (clDeviceList.GetSize() <= ucDeviceNumber))
      {
         #if defined(_MSC_VER)
            // !! Borland builder chokes on cfgmgr32
            TCHAR line1[64];
            TCHAR* argv_[2];

            //Only do the soft reset if we fail to open a device or it seems like theres no deivces to open (device in a "hosed" state)
            //The soft reset will have no effect on devices currently opened by other applications.
            argv_[0] = line1;
            SNPRINTF(&(line1[0]),sizeof(line1), "@USB\\VID_0FCF&PID_10*");   //The string for all ANT USB Devices
            WinDevice_Disable(1,argv_);
            WinDevice_Enable(1,argv_);

         #endif
         return;
      }

      pclTempDevice = clDeviceList[ucDeviceNumber];
   }

   pclTempDevice->USBReset();
   return;
}
