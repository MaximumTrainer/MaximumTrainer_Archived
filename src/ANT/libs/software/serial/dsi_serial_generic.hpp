/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/

#if !defined(DSI_SERIAL_GENERIC_HPP)
#define DSI_SERIAL_GENERIC_HPP

#include "types.h"
#include "dsi_thread.h"
#include "dsi_serial.hpp"

#include "usb_device.hpp"
#include "usb_device_handle.hpp"
#include "usb_device_list.hpp"


//////////////////////////////////////////////////////////////////////////////////
// Public Class Prototypes
//////////////////////////////////////////////////////////////////////////////////

class DSISerialGeneric : public DSISerial
{
   private:

      DSI_THREAD_ID hReceiveThread;                         // Handle for the receive thread.
      DSI_MUTEX stMutexCriticalSection;                     // Mutex used with the wait condition
      DSI_CONDITION_VAR stEventReceiveThreadExit;           // Event to signal the receive thread has ended.
      BOOL bStopReceiveThread;                              // Flag to stop the receive thread.

      USBDeviceHandle* pclDeviceHandle;                     // Handle to the USB device.

      const USBDevice* pclDevice;
      UCHAR ucDeviceNumber;
      ULONG ulBaud;

      static time_t lastUsbResetTime;

      // Private Member Functions
      void ReceiveThread();
      static DSI_THREAD_RETURN ProcessThread(void *pvParameter_);

   public:
      DSISerialGeneric();
      ~DSISerialGeneric();

      BOOL Init(ULONG ulBaud_, UCHAR ucDeviceNumber_);
      BOOL Init(ULONG ulBaud_, const USBDevice& clDevice_, UCHAR ucDeviceNumber_);

      void USBReset();
      UCHAR GetNumberOfDevices();

      // Methods inherited from the base class:
      BOOL AutoInit();
      ULONG GetDeviceSerialNumber();

      BOOL Open();
      void Close(BOOL bReset = FALSE);
      BOOL WriteBytes(void *pvData_, USHORT usSize_);
      UCHAR GetDeviceNumber();

      BOOL GetDeviceUSBInfo(UCHAR ucDevice_, UCHAR* pucProductString_, UCHAR* pucSerialString_, USHORT usBufferSize_);
      BOOL GetDevicePID(USHORT& usPid_);
     BOOL GetDeviceVID(USHORT& usVid_);
      BOOL GetDeviceSerialString(UCHAR* pucSerialString_, USHORT usBufferSize_);

};

#endif // !defined(DSI_SERIAL_GENERIC_HPP)

