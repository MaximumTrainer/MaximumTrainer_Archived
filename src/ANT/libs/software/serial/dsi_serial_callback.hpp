/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(DSI_SERIAL_CALLBACK_HPP)
#define DSI_SERIAL_CALLBACK_HPP

#include "types.h"


//////////////////////////////////////////////////////////////////////////////////
// Public Class Prototypes
//////////////////////////////////////////////////////////////////////////////////

class DSISerialCallback
{
   public:
      virtual ~DSISerialCallback(){}

      virtual void ProcessByte(UCHAR ucByte_) = 0;
      /////////////////////////////////////////////////////////////////
      // Processes a single received byte.
      // Parameters:
      //    ucByte_:          The byte to process.
      /////////////////////////////////////////////////////////////////

      virtual void Error(UCHAR ucError_) = 0;
      /////////////////////////////////////////////////////////////////
      // Signals an error.
      // Parameters:
      //    ucError_:         The error that has occured.  This error
      //                      is defined by the DSISerial class
      //                      implementation.
      /////////////////////////////////////////////////////////////////
};

#endif // !defined(DSI_SERIAL_CALLBACK_HPP)

