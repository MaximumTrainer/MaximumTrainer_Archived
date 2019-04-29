#ifndef SPEED_PAGES_H
#define SPEED_PAGES_H

#include "types.h"



typedef struct
{
   USHORT usLastTime1024;                            // Page 0
   USHORT usCumSpeedRevCount;                        // Page 0
} BSPage0_Data;

typedef struct
{
   ULONG ulOperatingTime;                            // Page 1
} BSPage1_Data;

typedef struct
{
   UCHAR ucManId;                                    // Page 2
   ULONG ulSerialNumber;                             // Page 2
} BSPage2_Data;

typedef struct
{
   UCHAR ucHwVersion;                                // Page 3
   UCHAR ucSwVersion;                                // Page 3
   UCHAR ucModelNumber;                              // Page 3
} BSPage3_Data;



#endif // SPEED_PAGES_H
