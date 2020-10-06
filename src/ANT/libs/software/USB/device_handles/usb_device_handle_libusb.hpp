/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef USB_DEVICE_HANDLE_LIBUSB_HPP
#define USB_DEVICE_HANDLE_LIBUSB_HPP

#include "types.h"
#include "dsi_thread.h"
#include "dsi_libusb_library.hpp"

#include "usb_device_handle.hpp"
#include "usb_device_libusb.hpp"
#include "dsi_ts_queue.hpp"

#include "usb_device_list.hpp"

#include <deque>
#include <list>


//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////

typedef USBDeviceList<const USBDeviceLibusb*> USBDeviceListLibusb;

/*
//for internal use only!
struct SerialData
{
   SerialData()
   {
      iSize = 0;
      iDataStart = 0;
   }

   char data[4096];
   int iSize;

   int iDataStart;
};
*/

//////////////////////////////////////////////////////////////////////////////////
// Public Class Prototypes
//////////////////////////////////////////////////////////////////////////////////

class USBDeviceHandleLibusb : public USBDeviceHandle
{
  private:

   LibusbLibrary clLibusbLibrary;
   //std::deque<SerialData*> clOverflowQueue;  //used if the user does not specify a big enough array
   TSQueue<UCHAR> clRxQueue;

   const USBDeviceLibusb clDevice;
   usb_dev_handle* device_handle;


   // Thread Variables
   DSI_THREAD_ID hReceiveThread;                         // Handle for the receive thread.
   DSI_MUTEX stMutexCriticalSection;                     // Mutex used with the wait condition
   DSI_CONDITION_VAR stEventReceiveThreadExit;           // Event to signal the receive thread has ended.
   BOOL bStopReceiveThread;                              // Flag to stop the receive thread.

   BOOL bDeviceGone;


   BOOL POpen();
   void PClose(BOOL bReset_ = FALSE);
   void ReceiveThread();
   static DSI_THREAD_RETURN ProcessThread(void* pvParameter_);

   static USBDeviceList<const USBDeviceLibusb> clDeviceList;  //This holds only instances of USBDeviceLibusb (unless someone manually makes their own)

   //!!Const-correctness!
  public:

   static const USBDeviceListLibusb GetAllDevices();  //!!have static list?
   static const USBDeviceListLibusb GetAvailableDevices();

   static BOOL Open(const USBDeviceLibusb& clDevice_, USBDeviceHandleLibusb*& pclDeviceHandle_);  //should these be member functions?
   static BOOL Close(USBDeviceHandleLibusb*& pclDeviceHandle_, BOOL bReset_ = FALSE);
   static BOOL TryOpen(const USBDeviceLibusb& clDevice_);


   //USBDeviceHandle Base Class//

   USBError::Enum Write(void* pvData_, ULONG ulSize_, ULONG& ulBytesWritten_);
   USBError::Enum Read(void* pvData_, ULONG ulSize_, ULONG& ulBytesRead_, ULONG ulWaitTime_);

   const USBDevice& GetDevice() { return clDevice; }

   ////

  protected:

   USBDeviceHandleLibusb(const USBDeviceLibusb& clDevice_);
   virtual ~USBDeviceHandleLibusb();

   const USBDeviceHandleLibusb& operator=(const USBDeviceHandleLibusb& clDevicehandle_) { return clDevicehandle_; }  //!!NOP

};

#endif // !defined(USB_DEVICE_HANDLE_LIBUSB_HPP)

