/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef DSI_DEFINES_H
#define DSI_DEFINES_H

#include "types.h"


//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////

#define KELVIN                         273.15F
#define G                              9.80665F             // m/s2
#define PI                             3.14159265359F
#define RADTODEG(x)                    ((x) * 360.0F / (2.0F * PI))
#define DEGTORAD(x)                    ((x) * (2.0F * PI) / 360.0F)
#define CTOK(x)                        ((x) + KELVIN)
#define KTOC(x)                        ((x) - KELVIN)
#define MIN(x, y)                      (((x) < (y)) ? (x) : (y))
#define MAX(x, y)                      (((x) > (y)) ? (x) : (y))
#define SQR(x)                         ((x) * (x))
#define UINTDIV(x, y)                  (((x) + (y) / 2) / (y)) // Unsigned integer division, x / y rounded to the nearest integer

#define ROUND_BIAS(x)                  ((x) < 0 ? -0.5F : 0.5F)
#define ROUND_FLOAT(x)                 ((x) + ROUND_BIAS(x))

// The following macro computes offset (in bytes) of a member in a
// structure.  This compiles to a constant if a constant member is
// supplied as arg.
#define STRUCT_OFFSET(MEMBER, STRUCT_TYPE)   ( ((UCHAR *) &(((STRUCT_TYPE *) NULL)->MEMBER)) - ((UCHAR *) (NULL)) )

#endif // defined(DSI_DEFINES_H)

