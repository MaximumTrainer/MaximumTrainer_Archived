/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(CHECKSUM_H)
#define CHECKSUM_H

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
//////////////////////////////////////////////////////////////////////////////////

#define CHECKSUM_VERSION   "AQB0.009"


//////////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
//////////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)
   extern "C" {
#endif

UCHAR CheckSum_Calc8(const volatile void *pvDataPtr_, USHORT usSize_);

#if defined(__cplusplus)
   }
#endif

#endif // !defined(CHECKSUM_H)
