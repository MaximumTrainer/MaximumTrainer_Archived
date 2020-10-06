/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#if defined(DSI_TYPES_WINDOWS)

#include "dsi_serial_libusb.hpp"
#include "defines.h"
#include "macros.h"

#include "usb_device_handle_libusb.hpp"

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
DSISerialLibusb::DSISerialLibusb()
{
   pclDevice = NULL;
   hReceiveThread = NULL;
   pclDeviceHandle = NULL;
}

///////////////////////////////////////////////////////////////////////
// Destructor
///////////////////////////////////////////////////////////////////////
DSISerialLibusb::~DSISerialLibusb()
{
   Close();
}

///////////////////////////////////////////////////////////////////////
// Initializes and opens the object.
///////////////////////////////////////////////////////////////////////
//defines used by AutoInit
//#define USB_ANT_VID                    ((USHORT)0x1915)
//#define USB_ANT_PID                    ((USHORT)0x0102)
BOOL DSISerialLibusb::AutoInit()
{
   if(Init(0, USBDeviceHandle::USB_ANT_VID_TWO) == FALSE)
   {
      this->USBReset();
      return FALSE;
   }

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Initializes the object.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialLibusb::Init(ULONG ulBaud_, UCHAR ucDeviceNumber_)
{
   Close();

   pclDevice = NULL;
   ulBaud = ulBaud_;
   ucDeviceNumber = ucDeviceNumber_;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Initializes the object.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialLibusb::Init(ULONG ulBaud_, USHORT usVid_)
{
   UCHAR ucDeviceNumberTemp;

   Close();

   BOOL bSuccess = GetDeviceNumberByVendorId(usVid_, ucDeviceNumberTemp);
   if(!bSuccess)
   {
      Close();
      return FALSE;
   }

   pclDevice = NULL;
   ulBaud = ulBaud_;
   ucDeviceNumber = ucDeviceNumberTemp;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Initializes the object.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialLibusb::Init(ULONG ulBaud_, const USBDeviceLibusb& clDevice_, UCHAR ucDeviceNumber_)
{
   Close();

   pclDevice = &clDevice_; //!!
   ulBaud = ulBaud_;
   ucDeviceNumber = ucDeviceNumber_;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Returns the device serial number.
///////////////////////////////////////////////////////////////////////
ULONG DSISerialLibusb::GetDeviceSerialNumber()
{
   if(pclDeviceHandle == 0)
      return 0;

   return pclDeviceHandle->GetDevice().GetSerialNumber();
}


///////////////////////////////////////////////////////////////////////
// Get USB enumeration info. Not necessarily connected to USB.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialLibusb::GetDeviceUSBInfo(UCHAR ucDevice_, UCHAR* pucProductString_, UCHAR* pucSerialString_, USHORT usBufferSize_)
{
   const USBDeviceListLibusb clDeviceList = USBDeviceHandleLibusb::GetAllDevices();
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
BOOL DSISerialLibusb::GetDevicePID(USHORT& usPid_)
{
   if(pclDeviceHandle == NULL)
      return FALSE;

   usPid_ = pclDeviceHandle->GetDevice().GetPid();
   return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Get USB VID. Need to be connected to USB device.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialLibusb::GetDeviceVID(USHORT& usVid_)
{
   if(pclDeviceHandle == NULL)
      return FALSE;

   usVid_ = pclDeviceHandle->GetDevice().GetVid();
   return TRUE;
}


///////////////////////////////////////////////////////////////////////
// Opens port, starts receive thread.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialLibusb::Open()
{
   // Make sure all handles are reset before opening again.
   Close();

   if (pclCallback == NULL)
      return FALSE;

   bStopReceiveThread = FALSE;

   //If the user specified a device number instead of a USBDevice instance, then grab it from the list
   const USBDeviceLibusb* pclTempDevice = pclDevice;
   if(pclDevice == NULL)
   {
      const USBDeviceListLibusb clDeviceList = USBDeviceHandleLibusb::GetAllDevices();
      if(clDeviceList.GetSize() <= ucDeviceNumber)
         return FALSE;

      pclTempDevice = clDeviceList[ucDeviceNumber];
   }

   if(USBDeviceHandleLibusb::Open(*pclTempDevice, pclDeviceHandle) == FALSE)
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

   hReceiveThread = DSIThread_CreateThread(&DSISerialLibusb::ProcessThread, this);
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
void DSISerialLibusb::Close(BOOL bReset_)
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
      USBDeviceHandleLibusb::Close(pclDeviceHandle, bReset_);

      //if(bReset_)  //Note: The reset is disabled in device_handle_libusb, so this is a useless wait
      //   DSIThread_Sleep(1750);           //Stall to allow the device to reset, trying to reopen the driver too soon can cause bad things to happen.

      pclDeviceHandle = NULL;
   }

   return;
}

///////////////////////////////////////////////////////////////////////
// Writes ucSize_ bytes to USB, returns number of bytes not written.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialLibusb::WriteBytes(void *pvData_, USHORT usSize_)
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
UCHAR DSISerialLibusb::GetDeviceNumber()
{
   return ucDeviceNumber;
}

//////////////////////////////////////////////////////////////////////////////////
// Private Methods
//////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
void DSISerialLibusb::ReceiveThread()
{

   while(!bStopReceiveThread)
   {

      UCHAR ucRxByte;
      ULONG ulRxBytesRead;
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
DSI_THREAD_RETURN DSISerialLibusb::ProcessThread(void *pvParameter_)
{
   DSISerialLibusb *This = (DSISerialLibusb *) pvParameter_;
   This->ReceiveThread();
   return 0;
}

///////////////////////////////////////////////////////////////////////
// Finds the first availible device that matches the vendor id
// and returns the device number.
///////////////////////////////////////////////////////////////////////
BOOL DSISerialLibusb::GetDeviceNumberByVendorId(USHORT usVid_, UCHAR& ucDeviceNumber_)
{

   const USBDeviceListLibusb clDeviceList = USBDeviceHandleLibusb::GetAvailableDevices();
   ULONG ulNumOfDevices = clDeviceList.GetSize();
   if(ulNumOfDevices == 0)
   {
      Close();
      return FALSE;
   }


   BOOL bDeviceFound = FALSE;
   for(ULONG i=0; i<ulNumOfDevices && !bDeviceFound; i++)
   {
      const USBDeviceLibusb& clDevice = *(clDeviceList[i]);
      USHORT usVid = clDevice.GetVid();
      if(usVid != usVid_)
         continue;

      USBDeviceHandleLibusb* pclTempHandle;
      if(USBDeviceHandleLibusb::Open(clDevice, pclTempHandle) == FALSE)
         continue;

      USBDeviceHandleLibusb::Close(pclTempHandle);
      ucDeviceNumber_ = (UCHAR)i;
      bDeviceFound = TRUE;
   }

   return bDeviceFound;
}

//!!
void DSISerialLibusb::USBReset(void)
{
#if defined(_MSC_VER)
   // !! Borland builder chokes on cfgmgr32
   TCHAR line1[64];
   TCHAR* argv_[2];

   //Only do the soft reset if we fail to open a device or it seems like theres no deivces to open (device in a "hosed" state)
   //The soft reset will have no effect on devices currently opened by other applications.
   argv_[0] = line1;
   SNPRINTF(&(line1[0]),sizeof(line1), "@USB\\VID_0FCF&PID_1008\\*");   //The string for all ANT USB Stick2s
   WinDevice_Disable(1,argv_);
   WinDevice_Enable(1,argv_);

#endif
}

#endif //defined(DSI_TYPES_WINDOWS)