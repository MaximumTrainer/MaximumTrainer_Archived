/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#if defined(DSI_TYPES_WINDOWS)

#include "antmessage.h"

#include "usb_device_handle.hpp"

#include "usb_device_handle_si.hpp"
#include "usb_device_handle_libusb.hpp"

#include "usb_device_si.hpp"
#include "usb_device_libusb.hpp"

#include "usb_device_list.hpp"


BOOL SiDeviceMatch(const USBDeviceSI* const & pclDevice_)
{
   USHORT usVid = pclDevice_->GetVid();
   return (usVid == USBDeviceHandle::USB_ANT_VID);
}

BOOL LibusbDeviceMatch(const USBDeviceLibusb* const & pclDevice_)
{
   //!!Can also find device by it's description string?
   USHORT usVid = pclDevice_->GetVid();
   return (usVid == USBDeviceHandle::USB_ANT_VID || usVid == USBDeviceHandle::USB_ANT_VID_TWO);
}

BOOL USBDeviceHandle::CopyANTDevice(const USBDevice*& pclUSBDeviceCopy_, const USBDevice* pclUSBDeviceOrg_)
{
   if (pclUSBDeviceOrg_ == NULL)
      return FALSE;

   if (pclUSBDeviceCopy_ != NULL)
      return FALSE;


   switch(pclUSBDeviceOrg_->GetDeviceType())
   {
      case DeviceType::SI_LABS:
      {
         const USBDeviceSI& clDeviceSi = dynamic_cast<const USBDeviceSI&>(*pclUSBDeviceOrg_);
         pclUSBDeviceCopy_ = new USBDeviceSI(clDeviceSi);
         break;
      }

      case DeviceType::LIBUSB:
      {
         const USBDeviceLibusb& clDeviceLibusb = dynamic_cast<const USBDeviceLibusb&>(*pclUSBDeviceOrg_);
         pclUSBDeviceCopy_ = new USBDeviceLibusb(clDeviceLibusb);
         break;
      }

      default:
      {
         return FALSE;
      }
   }

   return TRUE;
}

//!!Just here temporarily until we get ANTDeviceList to do it.
const ANTDeviceList USBDeviceHandle::GetAllDevices(ULONG ulDeviceTypeField_)
{
   ANTDeviceList clDeviceList;

   if( (ulDeviceTypeField_ & DeviceType::SI_LABS) != 0)
   {
      const USBDeviceListSI clDeviceSIList = USBDeviceHandleSI::GetAllDevices();  //!!There is a list copy here!
      clDeviceList.Add(clDeviceSIList.GetSubList(SiDeviceMatch) );
   }

   if( (ulDeviceTypeField_ & DeviceType::LIBUSB) != 0)
   {
      const USBDeviceListLibusb clDeviceLibusbList = USBDeviceHandleLibusb::GetAllDevices();  //!!There is a list copy here!
      clDeviceList.Add(clDeviceLibusbList.GetSubList(LibusbDeviceMatch) );
   }

   return clDeviceList;
}

//!!Just here temporarily until we get ANTDeviceList to do it.
const ANTDeviceList USBDeviceHandle::GetAvailableDevices(ULONG ulDeviceTypeField_)
{
   ANTDeviceList clDeviceList;

   if( (ulDeviceTypeField_ & DeviceType::SI_LABS) != 0)
   {
      const USBDeviceListSI clDeviceSIList = USBDeviceHandleSI::GetAvailableDevices();  //!!There is a list copy here!
      clDeviceList.Add(clDeviceSIList.GetSubList(SiDeviceMatch) );
   }

   if( (ulDeviceTypeField_ & DeviceType::LIBUSB) != 0)
   {
      const USBDeviceListLibusb clDeviceLibusbList = USBDeviceHandleLibusb::GetAvailableDevices();  //!!There is a list copy here!
      clDeviceList.Add(clDeviceLibusbList.GetSubList(LibusbDeviceMatch) );
   }

   return clDeviceList;
}


//!!Polymorphism would be easier to implement!
BOOL USBDeviceHandle::Open(const USBDevice& clDevice_, USBDeviceHandle*& pclDeviceHandle_, ULONG ulBaudRate_)
{
   //dynamic_cast does not handle *& types

   BOOL bSuccess;
   switch(clDevice_.GetDeviceType())
   {
      case DeviceType::SI_LABS:
      {
         const USBDeviceSI& clDeviceSi = dynamic_cast<const USBDeviceSI&>(clDevice_);

         USBDeviceHandleSI* pclDeviceHandleSi;
         bSuccess = USBDeviceHandleSI::Open(clDeviceSi, pclDeviceHandleSi, ulBaudRate_);

         //pclDeviceHandle_ = dynamic_cast<USBDeviceHandle*>(pclDeviceHandleSi);
         pclDeviceHandle_ = pclDeviceHandleSi;
         break;
      }

      case DeviceType::LIBUSB:
      {
         const USBDeviceLibusb& clDeviceLibusb = dynamic_cast<const USBDeviceLibusb&>(clDevice_);

         USBDeviceHandleLibusb* pclDeviceHandleLibusb;
         bSuccess = USBDeviceHandleLibusb::Open(clDeviceLibusb, pclDeviceHandleLibusb);

         //pclDeviceHandle_ = dynamic_cast<USBDeviceHandle*>(pclDeviceHandleLibusb);
         pclDeviceHandle_ = pclDeviceHandleLibusb;
         break;
      }

      default:
      {
         pclDeviceHandle_ = NULL;
         bSuccess = FALSE;
         break;
      }
   }

   return bSuccess;
}

BOOL USBDeviceHandle::Close(USBDeviceHandle*& pclDeviceHandle_, BOOL bReset_)
{
   if(pclDeviceHandle_ == NULL)
      return FALSE;

   //dynamic_cast does not handle *& types

   BOOL bSuccess;
   switch(pclDeviceHandle_->GetDevice().GetDeviceType())
   {
      case DeviceType::SI_LABS:
      {
         USBDeviceHandleSI* pclDeviceHandleSi = reinterpret_cast<USBDeviceHandleSI*>(pclDeviceHandle_);
         bSuccess = USBDeviceHandleSI::Close(pclDeviceHandleSi, bReset_);

         pclDeviceHandle_ = pclDeviceHandleSi;
         break;
      }

     case DeviceType::LIBUSB:
      {
         USBDeviceHandleLibusb* pclDeviceHandleLibusb = reinterpret_cast<USBDeviceHandleLibusb*>(pclDeviceHandle_);
         bSuccess = USBDeviceHandleLibusb::Close(pclDeviceHandleLibusb, bReset_);

         pclDeviceHandle_ = pclDeviceHandleLibusb;
         break;
      }

      default:
      {
         pclDeviceHandle_ = NULL;
         bSuccess = FALSE;
         break;
      }
   }

   return bSuccess;
}



#endif //defined(DSI_TYPES_WINDOWS)