/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef USB_DEVICE_HANDLE_SI_HPP
#define USB_DEVICE_HANDLE_SI_HPP

#include "types.h"
#include "dsi_thread.h"
#include "macros.h"

#include "usb_device_handle.hpp"
#include "usb_device_si.hpp"
#include "usb_device_list.hpp"

#include "dsi_silabs_library.hpp"
#include "dsi_cm_library.hpp"


#include "dsi_ts_queue.hpp"

#include <windows.h>

#include <memory>


//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////

typedef USBDeviceList<const USBDeviceSI*> USBDeviceListSI;


//////////////////////////////////////////////////////////////////////////////////
// Public Class Prototypes
//////////////////////////////////////////////////////////////////////////////////

class USBDeviceHandleSI : public USBDeviceHandle
{
   private:

      // Dynamic Libraries
      SiLabsLibrary clSiLibrary;
      CMLibrary clCmLibrary;

      TSQueue<char> clRxQueue;

      // USB Variables
      HANDLE hUSBEvent;
      HANDLE hUSBDeviceHandle;                              // Handle to the USB device.

      UCHAR ucCTSCode;     //!!Can we get rid of these and just use local variables?
      UCHAR ucRTSCode;
      UCHAR ucDTRCode;
      UCHAR ucDSRCode;
      UCHAR ucDCDCode;

      // Thread Variables
      DSI_THREAD_ID hReceiveThread;                         // Handle for the receive thread.
      DSI_MUTEX stMutexCriticalSection;                     // Mutex used with the wait condition
      DSI_CONDITION_VAR stEventReceiveThreadExit;           // Event to signal the receive thread has ended.
      BOOL bStopReceiveThread;                              // Flag to stop the receive thread.

      // Device Variables
      const USBDeviceSI clDevice;
      const ULONG ulBaudRate;
      BOOL bDeviceGone;

      // Private Member Functions
      BOOL POpen();
      void PClose(BOOL bReset_ = FALSE);
      void ReceiveThread();
      static DSI_THREAD_RETURN ProcessThread(void *pvParameter_);

      BOOL DeviceIsGone();

      static USBDeviceList<const USBDeviceSI> clDeviceList;  //This holds only instances of USBDeviceSI (unless someone manually makes their own)

      //!!Const-correctness!
   public:

      static const USBDeviceListSI GetAllDevices();  //!!List copy!   //!!Should we make the list static instead and return a reference to it?
      static const USBDeviceListSI GetAvailableDevices();  //!!List copy!

      static BOOL Open(const USBDeviceSI& clDevice_, USBDeviceHandleSI*& pclDeviceHandle_, ULONG ulBaudRate_);  //should these be member functions?
      static BOOL Close(USBDeviceHandleSI*& pclDeviceHandle_, BOOL bReset_ = FALSE);
      static BOOL TryOpen(const USBDeviceSI& clDevice_);

      static UCHAR GetNumberOfDevices();


      // Methods inherited from the base class:
      USBError::Enum Write(void* pvData_, ULONG ulSize_, ULONG& ulBytesWritten_);
      USBError::Enum Read(void* pvData_, ULONG ulSize_, ULONG& ulBytesRead_, ULONG ulWaitTime_);

      const USBDevice& GetDevice() { return clDevice; }


   protected:

      USBDeviceHandleSI(const USBDeviceSI& clDevice_, ULONG ulBaudRate_);
      virtual ~USBDeviceHandleSI();

      const USBDeviceHandleSI& operator=(const USBDeviceHandleSI& clDevicehandle_) { return clDevicehandle_; }  //!!NOP

};

#endif // !defined(USB_DEVICE_HANDLE_SI_HPP)

