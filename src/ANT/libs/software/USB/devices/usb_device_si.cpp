/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#if defined(DSI_TYPES_WINDOWS)

#if defined(_MSC_VER)
   #include "WinDevice.h"
#endif

#include "usb_device_si.hpp"

#include "macros.h"

#include <windows.h>

#include <memory>

using namespace std;


USBDeviceSI::USBDeviceSI(UCHAR ucDeviceNumber_)
:
   ucDeviceNumber(ucDeviceNumber_),
   usVid(0),
   usPid(0),
   ulSerialNumber(0)
{
   //initialize array
   szProductDescription[0] = '\0';
   szSerialString[0] = '\0';

   auto_ptr<const SiLabsLibrary> pclAutoSiLibrary(NULL);
   if(SiLabsLibrary::Load(pclAutoSiLibrary) == FALSE)
      return;
   const SiLabsLibrary& clSiLibrary = *pclAutoSiLibrary;


   char acTempString[SI_MAX_DEVICE_STRLEN];

   //Get device VID
   if(clSiLibrary.GetProductString(ucDeviceNumber_, acTempString, SI_RETURN_VID) == SI_SUCCESS)
   {
      usVid = (USHORT)strtoul(acTempString, NULL, 16);
   }
   else
      usVid = 0;

   //Get device PID
   if(clSiLibrary.GetProductString(ucDeviceNumber_, acTempString, SI_RETURN_PID) == SI_SUCCESS)
   {
      usPid = (USHORT)strtoul(acTempString, NULL, 16);
   }
   else
      usPid = 0;

   if(clSiLibrary.GetProductString(ucDeviceNumber_, szSerialString, SI_RETURN_SERIAL_NUMBER) == SI_SUCCESS)
   {
      USBDeviceSI::GetDeviceSerialNumber(ulSerialNumber);
   }
   else
   {
      szSerialString[0] = '\0';
      ulSerialNumber = 0;
   }

   if(clSiLibrary.GetProductString(ucDeviceNumber_, szProductDescription, SI_RETURN_DESCRIPTION) == SI_SUCCESS)  //!!null terminated?
   {
   }
   else
   {
      szProductDescription[0] = '\0';
   }

   /*
   if(clSiLibrary.GetProductString(ucDeviceNumber_, acTempString, SI_RETURN_LINK_NAME) != SI_SUCCESS)
   {
      szLinkName[0] = '\0';
   }
   */

   /* //!!What do these give us?
   CP210x_RETURN_SERIAL_NUMBER         0x00
   CP210x_RETURN_DESCRIPTION         0x01
   CP210x_RETURN_FULL_PATH            0x02
   */

   return;
}

USBDeviceSI::USBDeviceSI(const USBDeviceSI& clDevice_)
:
   ucDeviceNumber(clDevice_.ucDeviceNumber),
   usVid(clDevice_.usVid),
   usPid(clDevice_.usPid),
   ulSerialNumber(clDevice_.ulSerialNumber)
{
   STRNCPY((char*)szProductDescription, (char*)clDevice_.szProductDescription, sizeof(szProductDescription));
   memcpy(szSerialString, clDevice_.szSerialString, sizeof(szSerialString));
   return;
}

USBDeviceSI& USBDeviceSI::operator=(const USBDeviceSI& clDevice_)
{
   if(this == &clDevice_)
      return *this;

   ucDeviceNumber = clDevice_.ucDeviceNumber;
   usVid = clDevice_.usVid;
   usPid = clDevice_.usPid;
   ulSerialNumber = clDevice_.ulSerialNumber;
   STRNCPY((char*)szProductDescription, (char*)clDevice_.szProductDescription, sizeof(szProductDescription));
   memcpy(szSerialString, clDevice_.szSerialString, sizeof(szSerialString));
   return *this;
}

BOOL USBDeviceSI::GetProductDescription(UCHAR* pucProductDescription_, USHORT usBufferSize_) const
{
   return(STRNCPY((char*) pucProductDescription_, (char*) szProductDescription, usBufferSize_));
}

BOOL USBDeviceSI::GetSerialString(UCHAR* pucSerialString_, USHORT usBufferSize_) const
{
   if(sizeof(szSerialString) > usBufferSize_)
   {
      memcpy(pucSerialString_, szSerialString, usBufferSize_);
      return FALSE;
   }

   memcpy(pucSerialString_, szSerialString, sizeof(szSerialString));
   return TRUE;
}

BOOL USBDeviceSI::USBReset() const  //!!make static?
{
#if defined(_MSC_VER)
   // !! Borland builder chokes on cfgmgr32
   TCHAR line1[64];
   TCHAR* argv_[2];

   //Only do the soft reset if we fail to open a device or it seems like theres no devices to open (device in a "hosed" state)
   //The soft reset will have no effect on devices currently opened by other applications.
   argv_[0] = line1;
   SNPRINTF(&(line1[0]),sizeof(line1), "@USB\\VID_%04X&PID_%04X\\*", usVid, usPid);
   WinDevice_Disable(1,argv_);
   WinDevice_Enable(1,argv_);

#endif

   return TRUE;
}


//The serial number actually is not limited to a ULONG by USB specs,
//so, our range here is determined by whatever we do in our products.
//For now we have it defined as 1 to (ULONG_MAX-1)
BOOL USBDeviceSI::GetDeviceSerialNumber(ULONG& ulSerialNumber_)
{
   ULONG ulSerial = strtoul((char*)szSerialString, NULL, 10);
   if(ulSerial == 0 || ulSerial == ULONG_MAX)
      return FALSE;

   if((ulSerial > 10000) && (ulSerial < 10255)) //The real serial number is kept in the string
      memcpy(&ulSerial, &(szSerialString[56]), 4);

   ulSerialNumber_ = ulSerial;
   return TRUE;
}


#endif //defined(DSI_TYPES_WINDOWS)