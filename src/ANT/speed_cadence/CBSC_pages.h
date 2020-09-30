#ifndef CBSC_PAGES_H
#define CBSC_PAGES_H

#include "types.h"


typedef struct
{
   USHORT usLastCadence1024;                         // Page 0 CBSC
   USHORT usCumCadenceRevCount;                      // Page 0 CBSC
   USHORT usLastTime1024;                            // Page 0 CBSC
   USHORT usCumSpeedRevCount;                        // Page 0 CBSC
} CBSCPage0_Data;




#endif // CBSC_PAGES_H
