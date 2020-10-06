/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef USB_DEVICE_HANDLE_HPP
#define USB_DEVICE_HANDLE_HPP


#include "types.h"

#include "usb_device.hpp"
#include "usb_device_list.hpp"


struct USBError
{
   enum Enum
   {
      NONE,
      DEVICE_GONE,
      INVALID_PARAM,
      FAILED,
      TIMED_OUT
   };

   private: USBError();
};

typedef USBDeviceList<const USBDevice*> ANTDeviceList;

//typedef void (*DeviceCallback)(UCHAR);  //!!Should we make this an error enum?

//NOTE: We assume that there are no devices plugged/unplugged between getting the list and opening a device.

//NOTE: USBDevice instances are invalid once a new device list is requested!  This applies to all derived classes' lists as well.

//!!Maybe USBDeviceHandle should have a GetDeviceList() function as well, and then USBDeviceList will only have to worry about being a constant container
//!!Or maybe Device should have GetDeviceList();
class USBDeviceHandle
{
  public:

   static const USHORT USB_ANT_VID = 0x0FCF;
   static const USHORT USB_ANT_VID_TWO = 0x1915;

   static BOOL CopyANTDevice(const USBDevice*& pclUSBDeviceCopy_, const USBDevice* pclUSBDeviceOrg_);
   static const ANTDeviceList GetAllDevices(ULONG ulDeviceTypeField_ = 0xFFFFFFFF); //!!copy!
   static const ANTDeviceList GetAvailableDevices(ULONG ulDeviceTypeField_ = 0xFFFFFFFF);  //!!copy!

   static BOOL Open(const USBDevice& clDevice_, USBDeviceHandle*& pclDeviceHandle_, ULONG ulBaudRate_);
   static BOOL Close(USBDeviceHandle*& pclDeviceHandle_, BOOL bReset_ = FALSE);


   virtual USBError::Enum Write(void* pvData_, ULONG ulSize_, ULONG& ulBytesWritten_) = 0;  //!!Need timeout?
   /////////////////////////////////////////////////////////////////
   // Writes bytes to the serial port.
   // Parameters:
   //    *pvData_:         A pointer to a block of data to be queued
   //                      for sending over the serial port.
   //    usSize_:          The length of the block of data that is
   //                      pointed to by *pvData_.
   // Returns TRUE if successful.  Otherwise, it returns FALSE.
   // ulBytesWritten_ is only valid when returns successful. //!!
   /////////////////////////////////////////////////////////////////

   virtual USBError::Enum Read(void* pvData_, ULONG ulSize_, ULONG& ulBytesRead_, ULONG ulWaitTime_) = 0;

   virtual const USBDevice& GetDevice() = 0;

  protected:
   USBDeviceHandle() {}
   virtual ~USBDeviceHandle() {}

  private:
     //!!virtual BOOL Open() = 0;
     /////////////////////////////////////////////////////////////////
     // Opens up the communication channel with the serial module.
     // Returns TRUE if successful.  Otherwise, it returns FALSE.
     /////////////////////////////////////////////////////////////////

     //!!virtual BOOL Close(BOOL bReset = FALSE) = 0;
     /////////////////////////////////////////////////////////////////
     // Closes down the communication channel with the serial module.
     /////////////////////////////////////////////////////////////////

};

#endif //USB_DEVICE_HANDLE_HPP