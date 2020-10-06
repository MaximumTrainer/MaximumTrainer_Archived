/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef DSI_TYPES_H
#define DSI_TYPES_H

#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(__TOS_WIN__) || defined(__WINDOWS__)  //Windows Platform
   #define DSI_TYPES_WINDOWS

   #if defined(_WIN32_WINNT)
      #if (_WIN32_WINNT < 0x501)
         #undef _WIN32_WINNT
         #define _WIN32_WINNT          0x0501
      #endif
   #else
      #define _WIN32_WINNT             0X0501
   #endif

   #include <windows.h>

#elif defined(macintosh) || defined (Macintosh) || defined(__APPLE__) || defined(__MACH__)
   // Apple platform (first two defines are for Mac OS 9; last two are for Mac OS X)
   #define DSI_TYPES_MACINTOSH
#elif defined(linux) || defined (__linux)                   // Linux platform
   #define DSI_TYPES_LINUX
#else
   #define DSI_TYPES_OTHER_OS
#endif


//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////

#if !defined(TRUE)
   #define TRUE                        ((BOOL) 1)
#endif
#if !defined(FALSE)
   #define FALSE                       ((BOOL) 0)
#endif

#if !defined(NULL)                                          // <windef.h> compatibility.
   #define NULL                        ((void *) 0)
#endif

#define NUL                            '\0'

#define MAX_SCHAR                      ((SCHAR) 0x7F)
#define MIN_SCHAR                      ((SCHAR) 0x80)
#define MAX_UCHAR                      ((UCHAR) 0xFF)
#define MIN_UCHAR                      ((UCHAR) 0)

#define MAX_SSHORT                     ((SSHORT) 0x7FFF)
#define MIN_SSHORT                     ((SSHORT) 0x8000)
#define MAX_USHORT                     ((USHORT) 0xFFFF)
#define MIN_USHORT                     ((USHORT) 0)

#define MAX_SLONG                      ((SLONG) 0x7FFFFFFF)
#define MIN_SLONG                      ((SLONG) 0x80000000)
#define MAX_ULONG                      ((ULONG) 0xFFFFFFFF)
#define MIN_ULONG                      ((ULONG) 0)

#if defined (__ICC8051__)
   #define C_TYPE                      __code
   #define MEM_TYPE                    __xdata
   #define MEM_TYPE_S                  static __xdata
   #define MEM_TYPE_P                  __xdata *
   #define BIT_TYPE                    __bdata
   #define DIR_TYPE                    __data
#else
   #define C_TYPE
   #define MEM_TYPE
   #define MEM_TYPE_S
   #define MEM_TYPE_P
   #define BIT_TYPE
   #define DIR_TYPE
#endif

#if defined(__IAR_SYSTEMS_ICC__)
   #define BITFIELD_START              struct {
   #define BITFIELD_END                }
   #define BITS_1                      : 1
   #define BITS_2                      : 2
   #define BITS_3                      : 3
   #define BITS_4                      : 4
   #define BITS_5                      : 5
   #define BITS_6                      : 6
   #define BITS_7                      : 7
#else
   #define BITFIELD_START
   #define BITFIELD_END
   #define BITS_1
   #define BITS_2
   #define BITS_3
   #define BITS_4
   #define BITS_5
   #define BITS_6
   #define BITS_7
#endif

#if !defined(DSI_TYPES_WINDOWS)
   #if !defined(BASETYPES) && !defined(_WinToMac_h_)
      #if !defined(OBJC_BOOL_DEFINED)
         typedef signed char           BOOL;
      #endif
      typedef char                     BYTE;                // 1-byte int. Sign is processor/compiler-dependent.
   #endif
#endif

typedef unsigned char                  BOOL_SAFE;           // BOOL that doesn't get redefined to signed int in windows to result in TRUE=-1 when used in bit fields.

typedef signed char                    SCHAR;               // Signed 1-byte int.
typedef unsigned char                  UCHAR;               // Unsigned 1-byte int.

typedef signed short                   SSHORT;              // Signed 2-byte int.
typedef unsigned short                 USHORT;              // Unsigned 2-byte int.

#if !defined(__LP64__)
   typedef signed long                    SLONG;               // Signed 4-byte int.
   typedef unsigned long                  ULONG;               // Unsigned 4-byte int.

   // 4-byte int.  Sign is processor/compiler-dependent.  Commonly assumed
   // to be signed, however this is not guaranteed.  Suggested use for
   // this type is for efficient data passing and bit manipulation.
   // Assuming any sign for math may be problematic, especially on code
   // intended to be cross-platform.
   typedef long                           LONG;

#else
   // __LP64__ is defined when compilation is for a target in which
   // integers are 32-bit quantities, and long integers and pointers are 64-bit quantities
   // For example: Mac OS X, 64-Bit
   // https://developer.apple.com/library/mac/documentation/Darwin/Conceptual/64bitPorting/transition/transition.html
   typedef signed int                     SLONG;               // Signed 4-byte int.
   typedef unsigned int                   ULONG;               // Unsigned 4-byte int.
   typedef int                            LONG;
#endif

#if !defined(__BORLANDC__)
   typedef signed long long               SLLONG;              // Signed 8-byte int.
   typedef unsigned long long             ULLONG;              // Unsigned 8-byte int.
#endif

typedef float                          FLOAT;               // 2-byte floating point.
typedef double                         DOUBLE;              // 4-byte floating point.

///////////////////////////////////////////////////////////////////////
// !!NOTE:  The structures below assume little endian architecture!!
///////////////////////////////////////////////////////////////////////
typedef union
{
   USHORT usData;
   struct
   {
      UCHAR ucLow;
      UCHAR ucHigh;
   } stBytes;
} USHORT_UNION;

#define USHORT_HIGH(X)  (((USHORT_UNION *)(X))->stBytes.ucHigh)
#define USHORT_LOW(X)   (((USHORT_UNION *)(X))->stBytes.ucLow)

typedef union
{
   signed short ssData;
   struct
   {
      UCHAR ucLow;
      UCHAR ucHigh;
   } stBytes;
} SSHORT_UNION;

#define SSHORT_HIGH(X)  (((SSHORT_UNION *)(X))->stBytes.ucHigh)
#define SSHORT_LOW(X)   (((SSHORT_UNION *)(X))->stBytes.ucLow)

typedef union
{
   UCHAR aucData[4];
   ULONG ulData;
   struct
   {
      // The least significant byte of the ULONG in this structure is
      // referenced by ucByte0.
      UCHAR ucByte0;
      UCHAR ucByte1;
      UCHAR ucByte2;
      UCHAR ucByte3;
   } stBytes;
} ULONG_UNION;

#define ULONG_LOW_0(X)  (((ULONG_UNION *)(X))->stBytes.ucByte0)
#define ULONG_MID_1(X)  (((ULONG_UNION *)(X))->stBytes.ucByte1)
#define ULONG_MID_2(X)  (((ULONG_UNION *)(X))->stBytes.ucByte2)
#define ULONG_HIGH_3(X) (((ULONG_UNION *)(X))->stBytes.ucByte3)


#endif // defined(DSI_TYPES_H)
