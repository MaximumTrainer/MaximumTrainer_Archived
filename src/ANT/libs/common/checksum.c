/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#include "checksum.h"


//////////////////////////////////////////////////////////////////////////////////
// Public Functions
//////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
UCHAR CheckSum_Calc8(const volatile void *pvDataPtr_, USHORT usSize_)
{
   const UCHAR *pucDataPtr = (UCHAR *)pvDataPtr_;
   UCHAR ucCheckSum = 0;
   int i;

   // Calculate the CheckSum value (XOR of all bytes in the buffer).
   for (i = 0; i < usSize_; i++)
      ucCheckSum ^= pucDataPtr[i];

   return ucCheckSum;
}
