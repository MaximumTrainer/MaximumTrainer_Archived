/*
   Copyright 2012 Dynastream Innovations, Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "types.h"

#ifndef COMMON_PAGES_H
#define COMMON_PAGES_H


// Page structs
#define COMMON_PAGE_RESERVE_BYTE                           ((UCHAR) 0xFF)

typedef struct
{
   UCHAR ucPg80ReserveByte1;
   UCHAR ucPg80ReserveByte2;
   UCHAR  ucHwVersion;                               // Page 80
   USHORT usManId;                                   // Page 80
   USHORT usModelNumber;                             // Page 80
} CommonPage80_Data;

typedef struct
{
   UCHAR ucPg81ReserveByte1;
   UCHAR ucPg81ReserveByte2;
   UCHAR  ucSwVersion;                               // Page 81
   ULONG  ulSerialNumber;                            // Page 81
} CommonPage81_Data;

typedef struct
{
   UCHAR ucPg82ReserveByte1;
   UCHAR ucPg82ReserveByte2;
   ULONG ulCumOperatingTime;                        // Page 82
   UCHAR ucBattVoltage256;                          // Page 82
   UCHAR ucBattVoltage;                             // Page 82
   UCHAR ucBattStatus;                              // Page 82
   UCHAR ucCumOperatingTimeRes;                     // Page 82
   UCHAR ucDescriptiveField;					   //Used with WGT Display
} CommonPage82_Data;





////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
   ULONG  ulIntValue;
   USHORT usFracValue;
   USHORT usDeltaValue;
} RawValues;


typedef enum
{
   STATE_INIT_PAGE = 0,                                  // Initializing state
   STATE_STD_PAGE = 1,                                   // No extended messages to process
   STATE_EXT_PAGE = 2                                    // Extended messages to process
} StatePage;



#endif // COMMON_PAGES_H

