/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(DSI_CONVERT_H)
#define DSI_CONVERT_H

#include "types.h"

#if defined(__cplusplus)
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
//////////////////////////////////////////////////////////////////////////////////

//For all functions below ucByte0 is LSB
void Convert_USHORT_To_Bytes(USHORT usNum, UCHAR* ucByte1, UCHAR* ucByte0);
USHORT Convert_Bytes_To_USHORT(UCHAR ucByte1, UCHAR ucByte0);
void Convert_ULONG_To_Bytes(ULONG ulNum, UCHAR* ucByte3, UCHAR* ucByte2, UCHAR* ucByte1, UCHAR* ucByte0);
ULONG Convert_Bytes_To_ULONG(UCHAR ucByte3, UCHAR ucByte2, UCHAR ucByte1, UCHAR ucByte0);

#if defined(__cplusplus)
}
#endif

#endif // !defined(DSI_DEBUG_H)

