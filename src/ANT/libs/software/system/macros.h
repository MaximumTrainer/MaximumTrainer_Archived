/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/

// Standard Library function macros.
// Microsoft has decided to brand these functions with their own set of
// conventions, so I've included translations from the standard
// functions to the MS functions below.  The functions that should be
// used in this project have been renamed to the all-caps version of
// the standard names.

//All these macros are not compliant with the wide character set!

#ifndef MACROS_H
#define MACROS_H

#include "types.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
   #define FPRINTF                     fprintf_s
#else
   #define FPRINTF                     fprintf
#endif
   ////////////////////////////////////////////////////////////////////
   // Parameters:
   //    FILE* pfFile_:          file to write to
   //    const char* pcFormat_:  specifies how to format output
   //    ... :                   arguments to write
   //
   // On success, returns number of bytes/characters written.
   // On failure, returns negative number.
   // NOTE: Returned value does not include NULL characters on end of
   //       strings or at end of the string written to the file.
   ////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
//////////////////////////////////////////////////////////////////////////////////

int SNPRINTF(char* pcDst_, size_t uNum_, const char* pcFormat_, ...);
   ////////////////////////////////////////////////////////////////////
   // Parameters:
   //    pcDst_:                 destination address.
   //    uNum_:                  size of destination buffer.
   //    pcFormat_:              specifies how to format output
   //    ... :                   arguments to write
   //
   // On success, returns the number of characters written not including the NULL character.
   // On failure, returns -1.
   // NOTE: If the buffer is too small, the output will be truncated.
   // NOTE: SNPRINTF uses functions not compatible with older libraries but are C99 standard.
   // NOTE: Guaranteed to have a NULL terminating character.
   // NOTE: Uses functions in the C99 standard but not included in the C89/C90 standards.
   ////////////////////////////////////////////////////////////////////

size_t VSNPRINTF(char* pcDst_, size_t uNum_, const char* pcFormat_, va_list args);
   ////////////////////////////////////////////////////////////////////
   // Parameters:
   //    pcDst_:                 destination address.
   //    uNum_:                  size of destination buffer.
   //    pcFormat_:              specifies how to format output
   //    args :                   arguments to write
   //
   // On success, returns the number of characters written not including the NULL character.
   // On failure, returns -1.
   // NOTE: If the buffer is too small, the output will be truncated.
   // NOTE: SNPRINTF uses functions not compatible with older libraries but are C99 standard.
   // NOTE: Guaranteed to have a NULL terminating character.
   // NOTE: Uses functions in the C99 standard but not included in the C89/C90 standards.
   ////////////////////////////////////////////////////////////////////

BOOL STRNCPY(char* pcDst_, const char* pcSrc_, size_t num_);
   ////////////////////////////////////////////////////////////////////
   // Parameters:
   //    pcDst_:                 destination address.
   //    pcSrc_:                 source address.
   //    uNum_:                  size of destination buffer.
   //
   // Returns TRUE when there has been a complete copy.
   // Returns FALSE when dst or src are NULL or when buffer size is too small to do a complete copy.
   // NOTE: Guaranteed to have a NULL terminating character.
   // NOTE: When unsuccessful, pcDst_ is not modified.
   ////////////////////////////////////////////////////////////////////


BOOL STRNCAT(char* pcDst_, const char* pcSrc_, size_t num_);
   ////////////////////////////////////////////////////////////////////
   // Parameters:
   //    pcDst_:                 destination address.
   //    pcSrc_:                 source address.
   //    uNum_:                  size of destination buffer.
   //
   // Returns TRUE when there has been a complete concatenation.
   // Returns FALSE when dst or src are NULL or when buffer size is too small to do a complete concatenation.
   // NOTE: Guaranteed to have a NULL terminating character.
   // NOTE: When unsuccessful, pcDst_ is not modified.
   ////////////////////////////////////////////////////////////////////

FILE* FOPEN(const char* pcFilename_, const char* pcMode_);
   ////////////////////////////////////////////////////////////////////
   // Parameters:
   //    pcFilename_:            name of the file to open
   //    pcMode_:                the rights to the file needed
   //
   // On success, returns file pointer.
   // On failure, returns NULL.
   ////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif // !defined(MACROS_H)
