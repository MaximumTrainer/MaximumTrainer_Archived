/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(DSI_ANT_DEVICE_POLLING_HPP)
#define DSI_ANT_DEVICE_POLLING_HPP

#include "dsi_ant_device.hpp"

#define MAX_ANT_CHANNELS  8

//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////

typedef enum
{
   ANT_USB_RESPONSE_NONE = 0,
   ANT_USB_RESPONSE_OPEN_PASS,
   ANT_USB_RESPONSE_OPEN_FAIL,
   ANT_USB_RESPONSE_SERIAL_FAIL
} ANT_USB_RESPONSE;

// ANT-FS Host States
typedef enum
{
   ANT_USB_STATE_OFF = 0,
   ANT_USB_STATE_IDLE,
   ANT_USB_STATE_IDLE_POLLING_USB,
   ANT_USB_STATE_OPEN
} ANT_USB_STATE;

//////////////////////////////////////////////////////////////////////////////////
// Public Class Prototypes
//////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
// This class extends dsi_ant_device to add polling for usb devices in the case
// the desired device is not plugged in or is removed/dead/etc.
//
// IMPORTANT: this class is under development and subject to change.
// It is not recommended to use this class for custom applications.
/////////////////////////////////////////////////////////////////
class DSIANTDevicePolling: public DSIANTDevice
{
   private:

      //////////////////////////////////////////////////////////////////////////////////
      // Private Definitions
      //////////////////////////////////////////////////////////////////////////////////

      enum ENUM_REQUEST
      {
         REQUEST_NONE = 0,
         REQUEST_OPEN,
         REQUEST_HANDLE_SERIAL_ERROR
      };

      enum RETURN_STATUS
      {
         RETURN_FAIL = 0,
         RETURN_PASS,
         RETURN_STOP,
         RETURN_REJECT,
         RETURN_NA,
         RETURN_SERIAL_ERROR
      };

      //////////////////////////////////////////////////////////////////////////////////
      // Private Variables
      //////////////////////////////////////////////////////////////////////////////////

      DSIResponseQueue<ANT_USB_RESPONSE> clResponseQueue;

      DSI_THREAD_ID hRequestThread;                         // Handle for the request thread.
      DSI_MUTEX stMutexResponseQueue;                       // Mutex used with the response queue
      DSI_CONDITION_VAR stCondRequestThreadExit;            // Event to signal the request thread has ended.
      DSI_CONDITION_VAR stCondRequest;                      // Event to signal there is a new request
      DSI_CONDITION_VAR stCondWaitForResponse;              // Event to signal there is a new response to the application

      volatile BOOL bRequestThreadRunning;
      volatile BOOL bKillReqThread;

      UCHAR ucUSBDeviceNum;
      USHORT usBaudRate;

      ULONG ulUSBSerialNumber;

      volatile BOOL bAutoInit;

      volatile ANT_USB_STATE eState;
      volatile ENUM_REQUEST eRequest;

      //////////////////////////////////////////////////////////////////////////////////
      // Private Function Prototypes
      //////////////////////////////////////////////////////////////////////////////////

      void RequestThread(void);
      static DSI_THREAD_RETURN RequestThreadStart(void *pvParameter_);
      BOOL ReInitDevice(void);
      BOOL AttemptOpen(void);

      void AddResponse(ANT_USB_RESPONSE eResponse_);

      void closePolling();

   protected:
      /// OVERRIDE
      virtual void HandleSerialError();   //<-Note needs 'virtual' because it is called internally but needs to be overridden by subclasses

   public:

      DSIANTDevicePolling();
      ~DSIANTDevicePolling();

      /////////////////////////////////////////////////////////////////
      /// OVERRIDE
      // Begins to initialize the DSIANTDevice object.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      //
      // Operation:
      //    If it fails to initialize the USBstick,
      //    it will continue to retry the intitialization once a second until
      //    it is sucessful.  Once everything has be sucessfully initialized
      //    a response of ANT_USB_RESPONSE_OPEN_PASS will be sent.
      //
      // This version of Init will only work for the ANT USB stick with
      // USB device descriptor "ANT USB Stick"
      /////////////////////////////////////////////////////////////////
      virtual BOOL Open();

      /////////////////////////////////////////////////////////////////
      /// OVERRIDE
      // Begins to initialize the DSIANTDevice object.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // Parameters:
      //    ucUSBDeviceNum_:  USB port number
      //    usBaudRate_:      Serial baud rate
      //
      // Operation:
      //    If it fails to initialize the USBstick,
      //    it will continue to retry the intitialization once a second until
      //    it is sucessful.  Once everything has be sucessfully initialized
      //    a response of ANT_USB_RESPONSE_OPEN_PASS will be sent.
      /////////////////////////////////////////////////////////////////
      virtual BOOL Open(UCHAR ucUSBDeviceNum_, USHORT usBaudRate_);

      /////////////////////////////////////////////////////////////////
      /// OVERRIDE
      // Stops any pending actions, closes device down and cleans
      // up any resources being used by the library.
      /////////////////////////////////////////////////////////////////
      virtual void Close(void);


      /////////////////////////////////////////////////////////////////
      // Returns the current library status.
      /////////////////////////////////////////////////////////////////
      ANT_USB_STATE GetStatus(void);

      /////////////////////////////////////////////////////////////////
      // Wait for a response from the library
      // Parameters:
      //    ulMilliseconds_:  Set this value to the minimum time to
      //                      wait before returning.  If the value is
      //                      0, the function will return immediately.
      //                      If the value is DSI_THREAD_INFINITE, the
      //                      function will not time out.
      // If one or more responses are pending before the timeout
      // expires the function will return the first response that
      // occurred.  If no response is pending at the time the timeout
      // expires, ANT_USB_RESPONSE_NONE is returned.
      /////////////////////////////////////////////////////////////////
      ANT_USB_RESPONSE WaitForResponse(ULONG ulMilliseconds_);
};

#endif  // DSI_ANT_DEVICE_POLLING_HPP