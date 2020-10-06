/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef USB_DEVICE_SI_HPP
#define USB_DEVICE_SI_HPP

#include "types.h"
#include "macros.h"

#include "DSI_SiUSBXp_3_1.h"
#include "dsi_silabs_library.hpp"
#include "usb_device.hpp"

#include <memory>


//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////
// Public Class Prototypes
//////////////////////////////////////////////////////////////////////////////////


//!!Should USBDeviceSI also hold the product description used to find device?
class USBDeviceSI : public USBDevice
{

  public:

   USBDeviceSI(UCHAR ucDeviceNumber_);
   USBDeviceSI(const USBDeviceSI& clDevice_);
   USBDeviceSI& operator=(const USBDeviceSI& clDevice_);

   UCHAR GetDeviceNumber() const { return ucDeviceNumber; }

   //std::auto_ptr<USBDevice> MakeCopy() const { return auto_ptr<USBDevice>(new USBDeviceSI(*this)); }  //!!

   //Implementation of Device Interface

   BOOL USBReset() const;
   USHORT GetVid() const { return usVid; }
   USHORT GetPid() const { return usPid; }
   ULONG GetSerialNumber() const { return ulSerialNumber; }
   BOOL GetProductDescription(UCHAR* pucProductDescription_, USHORT usBufferSize_) const;  //guaranteed to be null-terminated
   BOOL GetSerialString(UCHAR* pucSerialString_, USHORT usBufferSize_) const;

   DeviceType::Enum GetDeviceType() const { return DeviceType::SI_LABS; }


  private:

   BOOL GetDeviceSerialNumber(ULONG& ulSerialNumber_);

   UCHAR ucDeviceNumber;
   USHORT usVid;
   USHORT usPid;
   ULONG ulSerialNumber;

   UCHAR szProductDescription[SI_MAX_DEVICE_STRLEN];
   UCHAR szSerialString[SI_MAX_DEVICE_STRLEN];

};


#endif // !defined(USB_DEVICE_SI_HPP)

