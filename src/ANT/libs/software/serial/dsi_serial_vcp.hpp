/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(DSI_SERIAL_VCP_HPP)
#define DSI_SERIAL_VCP_HPP

#if defined(DSI_TYPES_WINDOWS) // The VCP module is currently only supported on windows
#include "types.h"
#include "dsi_thread.h"
#include "dsi_serial.hpp"
#include "dsi_serial_callback.hpp"

#include <windows.h>

//////////////////////////////////////////////////////////////////////////////////
// Public Class Prototypes
//////////////////////////////////////////////////////////////////////////////////

class DSISerialVCP : public DSISerial
{
   private:

      DSI_THREAD_ID hReceiveThread;                         // Handle for the receive thread.
      DSI_MUTEX stMutexCriticalSection;                     // Mutex used with the wait condition
      DSI_CONDITION_VAR stEventReceiveThreadExit;           // Event to signal the receive thread has ended.
      BOOL bStopReceiveThread;                              // Flag to stop the receive thread.

      volatile HANDLE hComm;
      DCB dcb;
      UCHAR ucDeviceNumber;
      ULONG ulBaud;

      // Private Member Functions
      void ReceiveThread();
      static DSI_THREAD_RETURN ProcessThread(void *pvParameter_);
      BOOL GetDeviceNumberByProductDescription(void* pvProductDescription_, USHORT usSize_, UCHAR& ucDeviceNumber_);

   public:
      DSISerialVCP();
      ~DSISerialVCP();

      // Methods inherited from the base class:
      BOOL AutoInit();
      ULONG GetDeviceSerialNumber();

      BOOL Init(ULONG ulBaud_, UCHAR ucDeviceNumber_);
      BOOL Open();
      void Close(BOOL bReset = FALSE);
      BOOL WriteBytes(void *pvData_, USHORT usSize_);
      UCHAR GetDeviceNumber();
};

#endif // defined(DSI_TYPES_WINDOWS)

#endif // !defined(DSI_SERIAL_VCP_HPP)

