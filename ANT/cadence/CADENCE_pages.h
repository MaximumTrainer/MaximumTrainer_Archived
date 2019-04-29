#ifndef CADENCE_PAGES_H
#define CADENCE_PAGES_H

#include "types.h"


typedef struct
{
   USHORT usLastCadence1024;                         // Page 0
   USHORT usCumCadenceRevCount;                      // Page 0
} BCPage0_Data;

typedef struct
{
   ULONG ulOperatingTime;                            // Page 1
} BCPage1_Data;

typedef struct
{
   UCHAR ucManId;                                    // Page 2
   ULONG ulSerialNumber;                             // Page 2
} BCPage2_Data;

typedef struct
{
   UCHAR ucHwVersion;                                // Page 3
   UCHAR ucSwVersion;                                // Page 3
   UCHAR ucModelNumber;                              // Page 3
} BCPage3_Data;



#endif // CADENCE_PAGES_H
