/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(__VERSION_H__)
#define __VERSION_H__

#include "build_version.h"

#ifdef _UNICODE
#define _T(x)      L ## x
#else
#define _T(x)      x
#endif

#define STRINGIZE2(s) _T(#s)
#define STRINGIZE(s) STRINGIZE2(s)

#define VER_FILE_DESCRIPTION_STR    _T(PRODUCT_STRING)
#define VER_FILE_VERSION_STR        STRINGIZE(VERSION_MAJOR)            \
                                    _T(".") STRINGIZE(VERSION_MINOR)

// Version Information
#define SW_VER_PPP                     "ALU"
#define SW_VER_NUM                     VER_FILE_VERSION_STR
#if !defined(EXT_FUNCTIONALITY)
   #define SW_VER_DEV                     "00"
   #define SW_VER_SUFFIX                  ""
#else
   #define SW_VER_DEV                     "0001"   //Add two digits which are the ext lib version of the corresponding non-extended version
   #define SW_VER_SUFFIX                  "_BAXEXT"
#endif

#define SW_VER                         SW_VER_PPP SW_VER_NUM SW_VER_DEV SW_VER_SUFFIX
#define SW_DESCR                       ""

#endif // !defined(CONFIG_H)
