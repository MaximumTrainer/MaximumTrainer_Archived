/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(DSI_FRAMER_HPP)
#define DSI_FRAMER_HPP

#include "types.h"
#include "dsi_serial.hpp"


//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////

#define DSI_FRAMER_TIMEDOUT            (MAX_USHORT-1)
#define DSI_FRAMER_ERROR               MAX_USHORT


//////////////////////////////////////////////////////////////////////////////////
// Public Class Prototypes
//////////////////////////////////////////////////////////////////////////////////

class DSIFramer : public DSISerialCallback
{
   protected:
      DSISerial *pclSerial;                                 // Local pointer to a DSISerial object.

   public:
      // Constuctor and Destructor
      DSIFramer();
      DSIFramer(DSISerial *pclSerial_);
      /////////////////////////////////////////////////////////////////
      // Paramaters:
      //    *pclSerial_:      A pointer to a DSISerial object.
      /////////////////////////////////////////////////////////////////

      virtual ~DSIFramer();

      virtual BOOL WriteMessage(void *pvData_, USHORT usSize_) = 0;
      /////////////////////////////////////////////////////////////////
      // Writes bytes to the device.
      // Parameters:
      //    *pvData_:         A pointer to message data, including ID,
      //                      to be queued for sending over the serial
      //                      port.
      //    usSize_:          The message size.  Each implementation
      //                      must define the meaning of this size.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      /////////////////////////////////////////////////////////////////

      virtual USHORT WaitForMessage(ULONG ulMilliseconds_) = 0;
      /////////////////////////////////////////////////////////////////
      // Waits for a message to be received by the receive handler.
      // Parameters:
      //    ulMilliseconds_:  Set this value to the minimum time to
      //                      wait before returning.
      //                      If the value is 0, the function will
      //                      return immediately.
      //                      If the value is MAX_ULONG, the function
      //                      will not time out.
      // Returns the size of the next message.  If the return value is
      // DSI_FRAMER_TIMEDOUT (0xFFFE), then there is no message.
      // If there is an error in the message handling or there is a serial
      // error, then the return value is DSI_FRAMER_ERROR (0xFFFF)
      /////////////////////////////////////////////////////////////////

      virtual USHORT GetMessage(void *pvData_, USHORT usSize_ = 0) = 0;
      /////////////////////////////////////////////////////////////////
      // Gets the next message.
      // Parameters:
      //    *pvData_:         This is a pointer to a local buffer
      //                      to which this method will copy the
      //                      message.  The buffer must be large
      //                      enough to hold the message.  In many
      //                      cases, the size will be known by
      //                      virtue of the messaging scheme used.
      //                      Otherwise, the size returned by
      //                      WaitForMessage() may be used.
      //                      If the function returns DSI_FRAMER_ERROR
      //                      (0xFFFF), then *pvData_ will contain a
      //                      byte-length number indicating what error
      //                      occurred.  This number is implementation-
      //                      dependent.
      //   usSize_:           This is the maximum length this method
      //                      will copy to the buffer.  Each
      //                      implementation must define the meaning of
      //                      this size.  If set to 0, the method will
      //                      copy the entire message.
      //                      *Note that the message will be
      //                      dequeued even if the length is limited
      //                      by this parameter.  Therefore, if
      //                      GetMessage() is called and usSize_
      //                      is non-zero, and usSize_ is < the
      //                      message size, the remaining message
      //                      bytes will not be recoverable.
      // Returns the number of bytes copied to the buffer.  If the
      // return value is DSI_FRAMER_TIMEDOUT (0xFFFE), then there is no
      // message.  If there is an error in the message handling or there
      // is a serial error, then the return value is DSI_FRAMER_ERROR (0xFFFF).
      /////////////////////////////////////////////////////////////////
};

#endif // !defined(DSI_FRAMER_HPP)


