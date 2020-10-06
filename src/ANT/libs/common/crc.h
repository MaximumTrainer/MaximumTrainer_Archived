/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(CRC_H)
#define CRC_H

#include "types.h"

#if defined(__cplusplus)
   extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
//////////////////////////////////////////////////////////////////////////////////

// The functions below follow the pattern described here, where X is the
// type and x is the bit-width:

//X CRC_Calcx(const void *pvDataPtr_, ULONG ulSize_);
///////////////////////////////////////////////////////////////////////
// Calculate the CRC of a block of data
// Parameters:
//    *pvDataPtr_:               Pointer to block of data.
//    ulSize_:                   Length of the block of data.
// Returns the CRC.
///////////////////////////////////////////////////////////////////////

//X CRC_UpdateCRCx(X uxCRC_, const void *pvDataPtr_, ULONG ulSize_);
///////////////////////////////////////////////////////////////////////
// Calculate the CRC of a block of data given an initial CRC seed.
// Useful for continuing a CRC calculation from a previous one.
// Parameters:
//    uxCRC_:                    CRC seed.
//    *pvDataPtr_:               Pointer to block of data.
//    ulSize_:                   Length of the block of data.
// Returns the CRC.
///////////////////////////////////////////////////////////////////////

//X CRC_Getx(X uxCRC_, UCHAR ucByte_);
///////////////////////////////////////////////////////////////////////
// Get the CRC for a single byte.
// Parameters:
//    uxCRC_:                    CRC seed.
//    ucByte_:                   Byte on which to calculate the CRC.
// Returns the CRC.
///////////////////////////////////////////////////////////////////////

UCHAR CRC_Calc8(const void *pvDataPtr_, ULONG ulSize_);
UCHAR CRC_UpdateCRC8(UCHAR ucCRC_, const void *pvDataPtr_, ULONG ulSize_);
UCHAR CRC_Get8(UCHAR ucCRC_, UCHAR ucByte_);

USHORT CRC_Calc16(const void *pvDataPtr_, ULONG ulSize_);
USHORT CRC_UpdateCRC16(USHORT usCRC_, const volatile void *pvDataPtr_, ULONG ulSize_);
USHORT CRC_UpdateCRC16Short(USHORT usCRC_, UCHAR MEM_TYPE *pucDataPtr_, USHORT usSize_);
USHORT CRC_Get16(USHORT usCRC_, UCHAR ucByte_);

ULONG CRC_Calc32(const void *pvDataPtr_, ULONG ulSize_);
ULONG CRC_UpdateCRC32(ULONG ulCRC_, const void *pvDataPtr_, ULONG ulSize_);
ULONG CRC_Get32(UCHAR ucByte);

#if defined(__cplusplus)
   }
#endif

#endif // !defined(CRC_H)

