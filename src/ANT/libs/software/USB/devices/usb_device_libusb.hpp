/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef USB_DEVICE_LIBUSB_HPP
#define USB_DEVICE_LIBUSB_HPP

#include "types.h"

#include "dsi_libusb_library.hpp"
#include "usb_device.hpp"


//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////


//!!What happens if we release the list we got this device from?
class USBDeviceLibusb : public USBDevice
{
  public:
   USBDeviceLibusb(struct usb_device& stDevice_);
   USBDeviceLibusb(const USBDeviceLibusb& clDevice_);

   USBDeviceLibusb& operator=(const USBDeviceLibusb& clDevice_);

   struct usb_device& GetRawDevice() const { return *pstDevice; }



   //USBDevice Base//

   BOOL USBReset() const;

   USHORT GetVid() const { return usVid; }
   USHORT GetPid() const { return usPid; }

   ULONG GetSerialNumber() const { return ulSerialNumber; }
   BOOL GetProductDescription(UCHAR* pucProductDescription_, USHORT usBufferSize_) const; //guaranteed to be null-terminated
   BOOL GetSerialString(UCHAR* pucSerialString_, USHORT usBufferSize_) const;

   DeviceType::Enum GetDeviceType() const { return DeviceType::LIBUSB; }

   //std::auto_ptr<USBDevice> MakeCopy() const { return auto_ptr<USBDevice>(new USBDeviceSI(*this)); }

   ////


  private:

   BOOL GetDeviceSerialNumber(ULONG& ulSerialNumber_);

   struct usb_device* pstDevice; //should we copy instead?
   USHORT usVid;
   USHORT usPid;
   ULONG ulSerialNumber;
   UCHAR szProductDescription[USB_MAX_STRLEN];
   UCHAR szSerialString[USB_MAX_STRLEN];

};


#endif // !defined(USB_DEVICE_LIBUSB_HPP)

