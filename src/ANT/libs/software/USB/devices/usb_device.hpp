/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef USB_DEVICE_HPP
#define USB_DEVICE_HPP


#include "types.h"

//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////
#define USB_MAX_STRLEN 256

#define USB_ANT_STICK_VID        ((USHORT)0x0FCF)
#define USB_ANT_STICK_PID        ((USHORT)0x1004)
#define USB_ANT_DEV_BOARD_PID    ((USHORT)0x1006)
#define USB_ANT_STICK2_PID       ((USHORT)0x1008)

struct DeviceType
{
   enum Enum
   {
      SI_LABS        = 1<<0,
      LIBUSB         = 1<<1,
      IO_KIT         = 1<<2,
      SI_LABS_IOKIT  = 1<<3
   };
};


class USBDevice
{
  public:
   //virtual BOOL Open(USBDeviceHandle*& pclDevicehandle) const = 0;  //!!Make private with friend?

   virtual USHORT GetVid() const = 0;
   virtual USHORT GetPid() const = 0;

   virtual ULONG GetSerialNumber() const = 0;
   virtual BOOL GetProductDescription(UCHAR* /*pucProductDescription*/, USHORT /*usBufferSize*/) const = 0;  //guaranteed to be null-terminated; pointer is valid until device is released
   virtual BOOL GetSerialString(UCHAR* /*pucSerialString*/, USHORT /*usBufferSize*/) const = 0;

   virtual DeviceType::Enum GetDeviceType() const = 0;  //!!Or we could use a private enum!

   virtual BOOL USBReset() const = 0;  //!!Should we change this to USBReEnumerate()?

   virtual ~USBDevice(){}

  protected:
   USBDevice(){}


};


#endif //USB_DEVICE_HPP
