/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(DSI_SERIAL_HPP)
#define DSI_SERIAL_HPP

#include "types.h"
#include "dsi_serial_callback.hpp"


//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////

// Error codes.
#define DSI_SERIAL_ENONE            ((UCHAR) 0x00)
#define DSI_SERIAL_DEVICE_GONE      ((UCHAR) 0x01)
#define DSI_SERIAL_EWRITE           ((UCHAR) 0x02)
#define DSI_SERIAL_EREAD            ((UCHAR) 0x03)
#define DSI_SERIAL_EOTHER           ((UCHAR) 0xFF)


//////////////////////////////////////////////////////////////////////////////////
// Public Class Prototypes
//////////////////////////////////////////////////////////////////////////////////

class DSISerial
{
   protected:
      DSISerialCallback *pclCallback;                       // Local pointer to a DSISerialCallback object.

   public:
      // Constructor and Destructor
      DSISerial();
      virtual ~DSISerial();

      virtual BOOL AutoInit() = 0;

      virtual BOOL Init(ULONG ulBaud_, UCHAR ucDeviceNumber_) = 0;

      virtual ULONG GetDeviceSerialNumber() = 0;

      virtual BOOL GetDeviceUSBInfo(UCHAR /*ucDevice*/, UCHAR* /*pucProductString*/, UCHAR* /*pucSerialString_*/, USHORT /*usBufferSize_*/) { return FALSE; }

      virtual BOOL GetDevicePID(USHORT& /*usPid_*/) { return FALSE; }

      virtual BOOL GetDeviceVID(USHORT& /*usVid_*/) { return FALSE; }

      virtual void SetCallback(DSISerialCallback *pclCallback_);
      /////////////////////////////////////////////////////////////////
      // Sets the callback object.
      // Parameters:
      //    *pclCallback_:    A pointer to a DSISerialCallback object.
      /////////////////////////////////////////////////////////////////

      virtual BOOL Open(void) = 0;
      /////////////////////////////////////////////////////////////////
      // Opens up the communication channel with the serial module.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      /////////////////////////////////////////////////////////////////

      virtual void Close(BOOL bReset = FALSE) = 0;
      /////////////////////////////////////////////////////////////////
      // Closes down the communication channel with the serial module.
      /////////////////////////////////////////////////////////////////

      virtual BOOL WriteBytes(void *pvData_, USHORT usSize_) = 0;
      /////////////////////////////////////////////////////////////////
      // Writes bytes to the serial port.
      // Parameters:
      //    *pvData_:         A pointer to a block of data to be queued
      //                      for sending over the serial port.
      //    usSize_:          The length of the block of data that is
      //                      pointed to by *pvData_.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      /////////////////////////////////////////////////////////////////

      virtual UCHAR GetDeviceNumber(void) = 0;
      /////////////////////////////////////////////////////////////////
      // Returns the port number communication is on.
      /////////////////////////////////////////////////////////////////
};

#endif // !defined(DSI_SERIAL_HPP)
