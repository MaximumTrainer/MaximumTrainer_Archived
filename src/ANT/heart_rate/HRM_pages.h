#ifndef HRM_PAGES_H
#define HRM_PAGES_H

#include "types.h"


#define MAX_MSG_REPEAT_HR                    (10)

// Page structs
typedef struct
{
    USHORT usBeatTime;                                // All pages
    UCHAR ucBeatCount;                                // All pages
    UCHAR ucComputedHeartRate;                        // All pages
} HRMPage0_Data;

typedef struct
{
    ULONG ulOperatingTime;                            // Page 1
} HRMPage1_Data;

typedef struct
{
    UCHAR ucManId;                                    // Page 2
    ULONG ulSerialNumber;                             // Page 2
} HRMPage2_Data;

typedef struct
{
    UCHAR ucHwVersion;                                // Page 3
    UCHAR ucSwVersion;                                // Page 3
    UCHAR ucModelNumber;                              // Page 3
} HRMPage3_Data;

typedef struct
{
    UCHAR ucManufSpecific;                            // Page 4
    USHORT usPreviousBeat;                            // Page 4
} HRMPage4_Data;



#endif // HRM_PAGES_H




